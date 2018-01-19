#define HW_CDC_BULK_OUT_SIZE	8
#define HW_CDC_BULK_IN_SIZE	8

extern uchar sendEmptyFrame;
extern uchar intr3Status;	/* used to control interrupt endpoint transmissions */

extern void cdcpoll(void);
