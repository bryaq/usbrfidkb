#include "types.h"
#include "usbdrv/usbdrv.h"
#include "keyboard.h"
#include "usb.h"
#include "hw.h"
#include "cdc.h"

uchar mode;

PROGMEM static const char kbd_usbDescriptorDevice[] = {    /* USB device descriptor */
	18,         /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
	USBDESCR_DEVICE,        /* descriptor type */
	0x10, 0x01,             /* USB version supported */
	USB_CFG_DEVICE_CLASS,
	USB_CFG_DEVICE_SUBCLASS,
	0,                      /* protocol */
	8,                      /* max packet size */
	/* the following two casts affect the first byte of the constant only, but
	 * that's sufficient to avoid a warning with the default values.
	 */
	(char)USB_CFG_VENDOR_ID,/* 2 bytes */
	(char)USB_CFG_DEVICE_ID,/* 2 bytes */
	USB_CFG_DEVICE_VERSION, /* 2 bytes */
	USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 ? 1 : 0,         /* manufacturer string index */
	USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 ? 2 : 0,        /* product string index */
	USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 ? 3 : 3,  /* serial number string index */
	1,          /* number of configurations */
};

PROGMEM static const char cdc_usbDescriptorDevice[] = {    /* USB device descriptor */
	18,         /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
	USBDESCR_DEVICE,        /* descriptor type */
	0x10, 0x01,             /* USB version supported */
	USB_CFG_DEVICE_CLASS,
	USB_CFG_DEVICE_SUBCLASS,
	0,                      /* protocol */
	8,                      /* max packet size */
	/* the following two casts affect the first byte of the constant only, but
	 * that's sufficient to avoid a warning with the default values.
	 */
	(char)USB_CFG_VENDOR_ID,/* 2 bytes */
	(char)0xdd, 0x27,/* 2 bytes */
	USB_CFG_DEVICE_VERSION, /* 2 bytes */
	USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 ? 1 : 0,         /* manufacturer string index */
	USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 ? 2 : 0,        /* product string index */
	USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 ? 3 : 3,  /* serial number string index */
	1,          /* number of configurations */
};

PROGMEM const char usbDescriptorConfiguration[] = {    /* USB configuration descriptor */
	9,          /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
	USBDESCR_CONFIG,    /* descriptor type */
	18 + 7 * USB_CFG_HAVE_INTRIN_ENDPOINT + 7 * 0 +
	            (9 & 0xff), 0,
	            /* total length of data returned (including inlined descriptors) */
	1,          /* number of interfaces in this configuration */
	1,          /* index of this configuration */
	0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
	(1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
	(1 << 7) | USBATTR_REMOTEWAKE,      /* attributes */
#endif
	USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */
/* interface descriptor follows inline: */
	9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
	USBDESCR_INTERFACE, /* descriptor type */
	0,          /* index of this interface */
	0,          /* alternate setting for this interface */
	USB_CFG_HAVE_INTRIN_ENDPOINT + 0, /* endpoints excl 0: number of endpoint descriptors to follow */
	USB_CFG_INTERFACE_CLASS,
	USB_CFG_INTERFACE_SUBCLASS,
	USB_CFG_INTERFACE_PROTOCOL,
	0,          /* string index for interface */
#if (9 & 0xff)    /* HID descriptor */
	9,          /* sizeof(usbDescrHID): length of descriptor in bytes */
	USBDESCR_HID,   /* descriptor type: HID */
	0x01, 0x01, /* BCD representation of HID version */
	0x00,       /* target country code */
	0x01,       /* number of HID Report (or other HID class) Descriptor infos to follow */
	0x22,       /* descriptor type: report */
	(USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH & 0xFF), /* descriptor length (low byte) */
	((USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH >> 8) & 0xFF), /*            (high byte) */
#endif
#if USB_CFG_HAVE_INTRIN_ENDPOINT    /* endpoint descriptor for endpoint 1 */
	7,          /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	(char)0x81, /* IN endpoint number 1 */
	0x03,       /* attrib: Interrupt endpoint */
	8, 0,       /* maximum packet size */
	USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
#if 0   /* endpoint descriptor for endpoint 3 */
	7,          /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	(char)(0x80 | USB_CFG_EP3_NUMBER), /* IN endpoint number 3 */
	0x03,       /* attrib: Interrupt endpoint */
	8, 0,       /* maximum packet size */
	USB_CFG_INTR_POLL_INTERVAL, /* in ms */
#endif
};

PROGMEM static const char configDescrCDC[] = {   /* USB configuration descriptor */
	9,          /* sizeof(usbDescrConfig): length of descriptor in bytes */
	USBDESCR_CONFIG,    /* descriptor type */
	67,
	0,          /* total length of data returned (including inlined descriptors) */
	2,          /* number of interfaces in this configuration */
	1,          /* index of this configuration */
	0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
	(1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
	(1 << 7),                           /* attributes */
#endif
	USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */

	/* interface descriptor follows inline: */
	9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
	USBDESCR_INTERFACE, /* descriptor type */
	0,          /* index of this interface */
	0,          /* alternate setting for this interface */
	USB_CFG_HAVE_INTRIN_ENDPOINT,   /* endpoints excl 0: number of endpoint descriptors to follow */
	2,
	2,
	1,
	0,          /* string index for interface */

	/* CDC Class-Specific descriptor */
	5,           /* sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes */
	0x24,        /* descriptor type */
	0,           /* header functional descriptor */
	0x10, 0x01,

	4,           /* sizeof(usbDescrCDC_AcmFn): length of descriptor in bytes */
	0x24,        /* descriptor type */
	2,           /* abstract control management functional descriptor */
	0x02,        /* SET_LINE_CODING,    GET_LINE_CODING, SET_CONTROL_LINE_STATE    */

	5,           /* sizeof(usbDescrCDC_UnionFn): length of descriptor in bytes */
	0x24,        /* descriptor type */
	6,           /* union functional descriptor */
	0,           /* CDC_COMM_INTF_ID */
	1,           /* CDC_DATA_INTF_ID */

	5,           /* sizeof(usbDescrCDC_CallMgtFn): length of descriptor in bytes */
	0x24,        /* descriptor type */
	1,           /* call management functional descriptor */
	3,           /* allow management on data interface, handles call management by itself */
	1,           /* CDC_DATA_INTF_ID */

	/* Endpoint Descriptor */
	7,           /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	0x80|USB_CFG_EP3_NUMBER,        /* IN endpoint number */
	0x03,        /* attrib: Interrupt endpoint */
	8, 0,        /* maximum packet size */
	USB_CFG_INTR_POLL_INTERVAL,        /* in ms */

	/* Interface Descriptor  */
	9,           /* sizeof(usbDescrInterface): length of descriptor in bytes */
	USBDESCR_INTERFACE,           /* descriptor type */
	1,           /* index of this interface */
	0,           /* alternate setting for this interface */
	2,           /* endpoints excl 0: number of endpoint descriptors to follow */
	0x0A,        /* Data Interface Class Codes */
	0,
	0,           /* Data Interface Class Protocol Codes */
	0,           /* string index for interface */

	/* Endpoint Descriptor */
	7,           /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	0x01,        /* OUT endpoint number 1 */
	0x02,        /* attrib: Bulk endpoint */
	8, 0,        /* maximum packet size */
	0,           /* in ms */

	/* Endpoint Descriptor */
	7,           /* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
	0x81,        /* IN endpoint number 1 */
	0x02,        /* attrib: Bulk endpoint */
	8, 0,        /* maximum packet size */
	0,           /* in ms */
};

PROGMEM const char usbHidReportDescriptor[63] = {   /* USB report descriptor */
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,                    // USAGE (Keyboard)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x95, 0x08,                    //   REPORT_COUNT (8)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
	0x95, 0x05,                    //   REPORT_COUNT (5)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x05, 0x08,                    //   USAGE_PAGE (LEDs)
	0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
	0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
	0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
	0x95, 0x01,                    //   REPORT_COUNT (1)
	0x75, 0x03,                    //   REPORT_SIZE (3)
	0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
	0x95, 0x06,                    //   REPORT_COUNT (6)
	0x75, 0x08,                    //   REPORT_SIZE (8)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
	0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
	0xc0                           // END_COLLECTION
};

usbMsgLen_t
usbFunctionDescriptor(struct usbRequest *rq)
{
	usbMsgLen_t len = 0;
	
	switch(rq->wValue.bytes[1]){
	case USBDESCR_DEVICE:
		len = USB_PROP_LENGTH(18);
		if(mode == MODE_KBD)
			usbMsgPtr = (usbMsgPtr_t)kbd_usbDescriptorDevice;
		else
			usbMsgPtr = (usbMsgPtr_t)cdc_usbDescriptorDevice;
		break;
	case USBDESCR_CONFIG:
		if(mode == MODE_KBD){
			len = USB_PROP_LENGTH(sizeof(usbDescriptorConfiguration));
			usbMsgPtr = (usbMsgPtr_t)usbDescriptorConfiguration;
		}else{
			len = USB_PROP_LENGTH(sizeof(configDescrCDC));
			usbMsgPtr = (usbMsgPtr_t)configDescrCDC;
		}
		break;
	}
	return len;
}

enum{
	SEND_ENCAPSULATED_COMMAND = 0,
	GET_ENCAPSULATED_RESPONSE,
	SET_COMM_FEATURE,
	GET_COMM_FEATURE,
	CLEAR_COMM_FEATURE,
	SET_LINE_CODING = 0x20,
	GET_LINE_CODING,
	SET_CONTROL_LINE_STATE,
	SEND_BREAK
};

usbMsgLen_t
usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *)data;
	
	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){
		if(rq->bRequest == USBRQ_HID_GET_REPORT)
			return sizeof(kbdReport_t);
		if(rq->bRequest == GET_LINE_CODING || rq->bRequest == SET_LINE_CODING)
			return USB_NO_MSG;
		if(rq->bRequest == SET_CONTROL_LINE_STATE){
#if USB_CFG_HAVE_INTRIN_ENDPOINT3
			/* Report serial state (carrier detect). On several Unix platforms,
			 * tty devices can only be opened when carrier detect is set.
			 */
			if(intr3Status == 0)
				intr3Status = 2;
#endif
		}
		if((rq->bmRequestType & USBRQ_DIR_MASK) == USBRQ_DIR_HOST_TO_DEVICE)
			sendEmptyFrame = 1;
	}
	return 0;
}

static uchar stopbit, parity, databit = 8;
static usbDWord_t baud = {.dword = 9600};

uchar
usbFunctionRead(uchar *data, uchar len)
{
	data[0] = baud.bytes[0];
	data[1] = baud.bytes[1];
	data[2] = baud.bytes[2];
	data[3] = baud.bytes[3];
	data[4] = stopbit;
	data[5] = parity;
	data[6] = databit;
	
	return 7;
}

uchar
usbFunctionWrite(uchar *data, uchar len)
{
	baud.bytes[0] = data[0];
	baud.bytes[1] = data[1];
	baud.bytes[2] = data[2];
	baud.bytes[3] = data[3];
	stopbit = data[4];
	parity = data[5];
	databit = data[6];
	if(parity > 2)
		parity = 0;
	if(stopbit == 1)
	 	stopbit = 0;
	
	return 1;
}

void
usbFunctionWriteOut(uchar *data, uchar len)
{
	if(*data == '1')
		led_off();
}
