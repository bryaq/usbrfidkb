#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <string.h>

#include "types.h"
#include "usbdrv/usbdrv.h"
#include "cdc.h"
#include "hw.h"

#define hex(c)	(((c) < 0xa) ? ((c) + '0') : ((c) - 0xa + 'a'))

uchar sendEmptyFrame;
uchar intr3Status;	/* used to control interrupt endpoint transmissions */

uchar detected[5];
static char rfid[13];
uchar pbuf[HW_CDC_BULK_SIZE];

static char *
printid(uchar *id)
{
	uchar i;
	
	for(i = 0; i < 5; i++){
		rfid[i * 2] = hex(id[i] >> 4);
		rfid[i * 2 + 1] = hex(id[i] & 0x0f);
	}
	rfid[10] = '\r';
	rfid[11] = '\n';
	rfid[12] = '\0';
	
	return rfid;
}

enum{
	CDC_STATE_PROMPT_RFID,
	CDC_STATE_WAIT_RFID,
	CDC_STATE_PRINT_RFID,
	CDC_STATE_PROMPT_PASSWORD,
	CDC_STATE_ENTER_PASSWORD,
	CDC_STATE_ENTER_PASSWORD_PRINT,
	CDC_STATE_PROMPT_PASSWORD_AGAIN,
	CDC_STATE_ENTER_PASSWORD_AGAIN,
	CDC_STATE_ENTER_PASSWORD_AGAIN_PRINT,
	CDC_STATE_MISMATCH,
	CDC_STATE_MISMATCH_GOTO,
	CDC_STATE_MATCH,
	CDC_STATE_EEPROM,
	CDC_STATE_REBOOT,
};

static const char *prompt_rfid = "Attach RFID tag:\r\n";
static const char *prompt_password = "Enter password:\r\n";
static const char *prompt_password_again = "\r\nEnter password again:\r\n";
static const char *mismatch = "\r\nPassword mismatch!\r\n";
static const char *match = "\r\nSaving RFID and password to EEPROM...";
static const char *reboot = "done\r\nRebooting...\r\n";

void
cdcpoll(void)
{
	static char *print;
	static uchar state;
	static uchar userid[5];
	static uchar pwd[16], again[16];
	static uchar p;
	uchar i;
	uchar buf[HW_CDC_BULK_SIZE], tbuf[HW_CDC_BULK_SIZE];
	
	switch(state){
	case CDC_STATE_PROMPT_RFID:
		if(print == NULL)
			print = (char *)prompt_rfid;
		break;
	case CDC_STATE_WAIT_RFID:
		if(events & EVENT_TIMER){
			state = CDC_STATE_PROMPT_RFID;
			events &= ~EVENT_TIMER;
		}
		if(events & EVENT_DETECT){
			memcpy(userid, detected, 5);
			state = CDC_STATE_PRINT_RFID;
			events &= ~EVENT_DETECT;
		}
		break;
	case CDC_STATE_PRINT_RFID:
		if(print == NULL)
			print = printid(userid);
		break;
	case CDC_STATE_PROMPT_PASSWORD:
		events &= ~EVENT_USB;
		if(print == NULL)
			print = (char *)prompt_password;
		break;
	case CDC_STATE_ENTER_PASSWORD:
		if(events & EVENT_USB){
			for(i = 0; i < HW_CDC_BULK_SIZE; i++){
				if(pbuf[i] == '\r' || pbuf[i] == '\n'){
					pwd[p++] = pbuf[i];
					pwd[p] = '\0';
					tbuf[i] = '\0';
					break;
				}else if(pbuf[i] != '\0' && p < sizeof(pwd) - 2){
					pwd[p++] = pbuf[i];
					tbuf[i] = '*';
				}else{
					tbuf[i] = '\0';
					break;
				}
			}
			print = (char *)tbuf;
			events &= ~EVENT_USB;
		}
		break;
	case CDC_STATE_ENTER_PASSWORD_PRINT:
		if(pwd[p - 1] == '\r' || pwd[p - 1] == '\n')
			state = CDC_STATE_PROMPT_PASSWORD_AGAIN;
		else
			state = CDC_STATE_ENTER_PASSWORD;
		break;
	case CDC_STATE_PROMPT_PASSWORD_AGAIN:
		events &= ~EVENT_USB;
		p = 0;
		if(print == NULL)
			print = (char *)prompt_password_again;
		break;
	case CDC_STATE_ENTER_PASSWORD_AGAIN:
		if(events & EVENT_USB){
			for(i = 0; i < HW_CDC_BULK_SIZE; i++){
				if(pbuf[i] == '\r' || pbuf[i] == '\n'){
					again[p++] = pbuf[i];
					again[p] = '\0';
					tbuf[i] = '\0';
					break;
				}else if(pbuf[i] != '\0' && p < sizeof(pwd) - 2){
					again[p++] = pbuf[i];
					tbuf[i] = '*';
				}else{
					tbuf[i] = '\0';
					break;
				}
			}
			print = (char *)tbuf;
			events &= ~EVENT_USB;
		}
		break;
	case CDC_STATE_ENTER_PASSWORD_AGAIN_PRINT:
		if(again[p - 1] == '\r' || again[p - 1] == '\n'){
			if(strncmp((char *)pwd, (char *)again, sizeof(pwd)) == 0)
				state = CDC_STATE_MATCH;
			else	
				state = CDC_STATE_MISMATCH;
		}else
			state = CDC_STATE_ENTER_PASSWORD_AGAIN;
		break;
	case CDC_STATE_MISMATCH:
		if(print == NULL)
			print = (char *)mismatch;
		break;
	case CDC_STATE_MISMATCH_GOTO:
		state = CDC_STATE_PROMPT_PASSWORD;
		break;
	case CDC_STATE_MATCH:
		p = 0;
		if(print == NULL)
			print = (char *)match;
		break;
	case CDC_STATE_EEPROM:
		if(EECR & _BV(EEWE))
			break;
		EEAR = p;
		if(p < sizeof(userid))
			EEDR = userid[p];
		else
			EEDR = pwd[p - sizeof(userid)];
		cli();
		EECR |= _BV(EEMWE);
		EECR |= _BV(EEWE);
		sei();
		p++;
		if(p == sizeof(userid) + sizeof(pwd))
			state = CDC_STATE_REBOOT;
		break;
	case CDC_STATE_REBOOT:
		wdt_reset();
		if(print == NULL){
			wdt_enable(WDTO_15MS);
			print = (char *)reboot;
		}
		break;
	}
	if(usbInterruptIsReady()){
		if(sendEmptyFrame){
			sendEmptyFrame = 0;
			usbSetInterrupt(buf, 0);
		}else if(print){
			for(i = 0; i < HW_CDC_BULK_SIZE; i++){
				if(*print == '\0'){
					print = NULL;
					state++;
					break;
				}
				buf[i] = *print++;
			}
			usbSetInterrupt(buf, i);
		}
	}else
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
	/* We need to report rx and tx carrier after open attempt */
	if(intr3Status != 0 && usbInterruptIsReady3()){
		static uchar serialStateNotification[10] = {0xa1, 0x20, 0, 0, 0, 0, 2, 0, 3, 0};
		
		if(intr3Status == 2)
			usbSetInterrupt3(serialStateNotification, 8);
		else
			usbSetInterrupt3(serialStateNotification + 8, 2);
		intr3Status--;
	}
#endif
}
