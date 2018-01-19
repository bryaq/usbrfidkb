#include "types.h"
#include "usbdrv/usbdrv.h"
#include "keyboard.h"

char *typing;

/* USB keycodes; bit 7 is Shift */
static const uchar key[] = {
	['\n']	= 0x28,
	['\r']	= 0x28,
	
	[' ']	= 0x2c,
	['!']	= 0x9e,
	['"']	= 0xb4,
	['#']	= 0xa0,
	['$']	= 0xa1,
	['%']	= 0xa2,
	['&']	= 0xa4,
	['\'']	= 0x34,
	['(']	= 0xa6,
	[')']	= 0xa7,
	['*']	= 0xa5,
	['+']	= 0xae,
	[',']	= 0x36,
	['-']	= 0x2d,
	['.']	= 0x37,
	['/']	= 0x38,
	
	['0']	= 0x27,
	['1']	= 0x1e,
	['2']	= 0x1f,
	['3']	= 0x20,
	['4']	= 0x21,
	['5']	= 0x22,
	['6']	= 0x23,
	['7']	= 0x24,
	['8']	= 0x25,
	['9']	= 0x26,
	[':']	= 0xb3,
	[';']	= 0x33,
	['<']	= 0xb6,
	['=']	= 0x2e,
	['>']	= 0xb7,
	['?']	= 0xb8,
	
	['@']	= 0x9f,
	['A']	= 0x84,
	['B']	= 0x85,
	['C']	= 0x86,
	['D']	= 0x87,
	['E']	= 0x88,
	['F']	= 0x89,
	['G']	= 0x8a,
	['H']	= 0x8b,
	['I']	= 0x8c,
	['J']	= 0x8d,
	['K']	= 0x8e,
	['L']	= 0x8f,
	['M']	= 0x90,
	['N']	= 0x91,
	['O']	= 0x92,
	
	['P']	= 0x93,
	['Q']	= 0x94,
	['R']	= 0x95,
	['S']	= 0x96,
	['T']	= 0x97,
	['U']	= 0x98,
	['V']	= 0x99,
	['W']	= 0x9a,
	['X']	= 0x9b,
	['Y']	= 0x9c,
	['Z']	= 0x9d,
	['[']	= 0x2f,
	['\\']	= 0x31,
	[']']	= 0x30,
	['^']	= 0xa3,
	['_']	= 0xad,
	
	['`']	= 0x35,
	['a']	= 0x04,
	['b']	= 0x05,
	['c']	= 0x06,
	['d']	= 0x07,
	['e']	= 0x08,
	['f']	= 0x09,
	['g']	= 0x0a,
	['h']	= 0x0b,
	['i']	= 0x0c,
	['j']	= 0x0d,
	['k']	= 0x0e,
	['l']	= 0x0f,
	['m']	= 0x10,
	['n']	= 0x11,
	['o']	= 0x12,
	
	['p']	= 0x13,
	['q']	= 0x14,
	['r']	= 0x15,
	['s']	= 0x16,
	['t']	= 0x17,
	['u']	= 0x18,
	['v']	= 0x19,
	['w']	= 0x1a,
	['x']	= 0x1b,
	['y']	= 0x1c,
	['z']	= 0x1d,
	['{']	= 0xaf,
	['|']	= 0xb1,
	['}']	= 0xb0,
	['~']	= 0xb5,
};

enum{
	TYPING_STATE_RELEASE,
	TYPING_STATE_PRESS,
};

void
typing_handle(void)
{
	static uchar state;
	static kbdReport_t kbdr;
	
	switch(state){
	case TYPING_STATE_RELEASE:
		kbdr.modifier = (key[(uchar)*typing] & 0x80) >> 2;
		kbdr.keycode[0] = key[(uchar)*typing] & 0x7f;
		state = TYPING_STATE_PRESS;
		break;
	case TYPING_STATE_PRESS:
		kbdr.modifier = 0;
		kbdr.keycode[0] = 0;
		typing++;
		if(*typing == '\0')
			typing = 0;
		state = TYPING_STATE_RELEASE;
		break;
	}
	usbSetInterrupt((uchar *)&kbdr, sizeof(kbdr));
}
