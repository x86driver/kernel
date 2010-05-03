#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

MODULE_LICENSE("Dual BSD/GPL");

#define REQ_SIZE 32
struct f_garmin {
	struct list_head req_head;
};

static struct f_garmin *_garmin_dev;
struct usb_request req_array[REQ_SIZE];

static inline void QUEUE(struct usb_request *p)
{
	list_add_tail(&p->list, &_garmin_dev->req_head);
}

static inline struct usb_request *DEQUE(void)
{
	struct usb_request *tmp;
        tmp = list_first_entry(&_garmin_dev->req_head, struct usb_request, list);
	list_del(&tmp->list);
	return tmp;
}

static void init_request(void)
{
	int i;
	unsigned char *ptr;
	for (i = 0; i < REQ_SIZE; ++i) {
		req_array[i].buf = kmalloc(4096, GFP_KERNEL);
		ptr = req_array[i].buf;
		*ptr = (unsigned char)i;
	}
}

static void show(void)
{
	struct list_head *p, *n;
	struct usb_request *req;
	unsigned char *ptr;

	list_for_each_safe(p, n, &_garmin_dev->req_head) {
		req = list_entry(p, struct usb_request, list);
		ptr = req->buf;
		printk(KERN_ALERT "%d\n", *ptr);
	}
}

static int hello_init(void)
{
	int order[] = {1,3,5,2,4};
	int i;
//	memset(&_garmin_dev, 0, sizeof(garmin));
	_garmin_dev = kzalloc(sizeof(struct f_garmin), GFP_KERNEL);
	INIT_LIST_HEAD(&_garmin_dev->req_head);
	
	init_request();

	for (i = 0; i < sizeof(order)/sizeof(int); ++i) {
		QUEUE(&req_array[order[i]]);
	}

	printk(KERN_ALERT "Size: %d\n", sizeof(req_array));
	show();

        return 0;

}

static void hello_exit(void)
{
	struct list_head *p, *n;
	struct usb_request *req;
	unsigned char *ptr;

	list_for_each_safe(p, n, &_garmin_dev->req_head) {
		req = list_entry(p, struct usb_request, list);
		ptr = req->buf;
		printk(KERN_ALERT "We'll free %d\n", *ptr);
		list_del(&req->list);
		kfree(req->buf);
	}
}

module_init(hello_init);
module_exit(hello_exit);
