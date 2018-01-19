typedef struct kbdReport_t kbdReport_t;
struct kbdReport_t
{
	uchar modifier;
	uchar reserved;
	uchar keycode[6];
};

extern char *typing;
extern void typing_handle(void);
