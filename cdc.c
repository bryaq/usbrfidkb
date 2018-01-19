#include <stddef.h>

#include "types.h"
#include "usbdrv/usbdrv.h"
#include "cdc.h"
#include "hw.h"

uchar sendEmptyFrame;
uchar intr3Status;	/* used to control interrupt endpoint transmissions */

enum{
	CDC_STATE_PROMPT_RFID,
	CDC_STATE_WAIT_RFID,
};

static const char *prompt_rfid = "Attach your RFID tag:\r\n";

void
cdcpoll(void)
{
	static char *print;
	static uchar state;
	uchar i;
	uchar buf[HW_CDC_BULK_IN_SIZE];
	
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
		break;
	}
	if(usbInterruptIsReady()){
		if(sendEmptyFrame){
			usbSetInterrupt(buf, 0);
			sendEmptyFrame = 0;
		}else if(print){
			for(i = 0; i < HW_CDC_BULK_IN_SIZE; i++){
				if(*print == '\0'){
					print = NULL;
					state++;
					break;
				}
				buf[i] = *print++;
			}
			usbSetInterrupt(buf, i);
		}
	}
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
