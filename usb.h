enum{
	MODE_KBD,
	MODE_CDC
};

typedef union usbDWord{
	ulong dword;
	uchar bytes[4];
}usbDWord_t;

extern uchar mode;
