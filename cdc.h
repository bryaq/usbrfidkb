#define HW_CDC_BULK_SIZE	8

extern uchar sendEmptyFrame;
extern uchar intr3Status;	/* used to control interrupt endpoint transmissions */
extern uchar detected[5];
extern uchar pbuf[HW_CDC_BULK_SIZE];

extern void cdcpoll(void);
