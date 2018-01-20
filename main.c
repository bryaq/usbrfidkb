#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stddef.h>
#include <string.h>
#include <util/delay.h>

#include "types.h"
#include "usbdrv/usbdrv.h"
#include "keyboard.h"
#include "usb.h"
#include "hw.h"
#include "cdc.h"

#define RADIO_FREQ	125000ul
#define ROUND		4

volatile static ushort t;
volatile static uchar out, wait;
static uchar userid[5];
static char pwd[16];

enum{
	COMP_STATE_STOP,
	COMP_STATE_START,
	COMP_STATE_DATA,
};

static void
comp_handle(void)
{
	ushort n;
	static uchar state, start, cnt, parity;
	static uchar *data;
	static uchar id[6];
	
	cli();
	n = t;
	sei();
	n = (n * 171) >> 8;
	n = (n + (1 << ROUND)) & ~((1 << (ROUND + 1)) - 1);	/* round */
	
	switch(state){
	case COMP_STATE_STOP:
		if(n == 64 && out == 1){
			start = 0;
			state = COMP_STATE_START;
		}
		break;
	case COMP_STATE_START:
		if(n != 32)
			state = COMP_STATE_STOP;
		else if(out == 1){
			start = (start << 1) | 0x01;
			if(start == 0xff){
				cnt = 0;
				data = id;
				parity = 0;
				state = COMP_STATE_DATA;
			}
		}
		break;
	case COMP_STATE_DATA:
		if(n == 32)
			cnt++;
		else if(n == 64){
			if(cnt & 0x01){
				state = COMP_STATE_STOP;
				break;
			}
			cnt += 2;
		}else{
			state = COMP_STATE_STOP;
			break;
		}
		if((cnt & 0x01) == 0){
			if(cnt == 10 || cnt == 20){
				if(data == &id[5]){
					if((*data & 0x0f) != parity || out != 0){
						state = COMP_STATE_STOP;
						break;
					}
					events |= EVENT_DETECT;
					memcpy(detected, id , 5);
					if(memcmp(id, userid, 5) == 0){
						if(wait == 0)
							typing = pwd;
						wait = 255;
					}
					state = COMP_STATE_STOP;
					break;
				}
				parity ^= *data & 0x0f;;
				if(cnt == 20){
					data++;
					cnt = 0;
				}
			}else
				*data = (*data << 1) | out;
		}
		break;
	}
}

ISR(TIMER1_CAPT_vect)
{
	static ushort then;
	
	out = (ACSR >> ACO) & 1;	/* comparator output */
	if(out)
		TCCR1B &= ~_BV(ICES1);
	else
		TCCR1B |= _BV(ICES1);
	t = ICR1 - then;
	then += t;
	events |= EVENT_COMP;
	return;
}

ISR(TIMER1_COMPA_vect)
{
	static uchar cnt;
	
	OCR1A += 1875;	/* wake up every 10 ms */
	if(wait)
		wait--;
	cnt++;
	if(cnt == 0)
		events |= EVENT_TIMER;
	
	return;
}

int
main(void)
{
	uchar i = 0;
	
	PORTB = UNCONNECTED_B;
	PORTC = LED | BUTTON | UNCONNECTED_C;
	PORTD = UNCONNECTED_D;
	DDRB = OUT125;
	DDRC = LED;
	
	TCCR1B = _BV(ICNC1) | _BV(CS11) | _BV(CS10);
	OCR2 = F_CPU / (RADIO_FREQ * 2 * 1) - 1;
	TCCR2 = _BV(WGM21) | _BV(COM20) | _BV(CS20);
	TIMSK = _BV(TICIE1) | _BV(OCIE1A);
	
	ACSR = _BV(ACIC);
	
	if(button_pressed())
		mode = MODE_CDC;
	else
		mode = MODE_KBD;
	if(mode == MODE_CDC)
		led_on();
	else{
		eeprom_read_block(userid, 0, sizeof(userid));
		eeprom_read_block(pwd, (void *)sizeof(userid), sizeof(pwd));
	}
	
	usbDeviceDisconnect();
	while(--i)
		_delay_ms(1);
	usbDeviceConnect();
	usbInit();
	events = 0;
	sei();
	
	set_sleep_mode(SLEEP_MODE_IDLE);
	while(1){
		usbPoll();
		if(events & EVENT_COMP){
			comp_handle();
			events &= ~EVENT_COMP;
		}
		switch(mode){
		case MODE_KBD:
			if(typing && usbInterruptIsReady())
				typing_handle();
			break;
		case MODE_CDC:
			cdcpoll();
			break;
		}
		sleep_mode();
	}
}
