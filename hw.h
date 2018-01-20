#define UNCONNECTED_B	(_BV(PB0) | _BV(PB1) | _BV(PB2) | _BV(PB4) | _BV(PB5))
#define UNCONNECTED_C	(_BV(PC2) | _BV(PC3) | _BV(PC4) | _BV(PC5))
#define UNCONNECTED_D	(_BV(PD0) | _BV(PD4))
#define LED		(_BV(PC0))
#define BUTTON		(_BV(PC1))
#define OUT125		(_BV(PB3))
#define led_on()	(PORTC &= ~LED)
#define led_off()	(PORTC |= LED)
#define button_pressed()	((PINC & BUTTON) == 0)

enum{
	EVENT_COMP =	1 << 0,
	EVENT_TIMER =	1 << 1,
	EVENT_DETECT =	1 << 2,
	EVENT_USB =	1 << 3,
};
__extension__ volatile register uchar events asm("r16");
