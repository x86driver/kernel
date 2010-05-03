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
	struct list_head req_head_in;
	struct list_head req_head_out;
	struct list_head req_head_int;
};

static struct f_garmin *_garmin_dev;
struct usb_request req_array_in[REQ_SIZE];
struct usb_request req_array_out[REQ_SIZE];
struct usb_request req_array_int[REQ_SIZE];

static inline void QUEUE(struct usb_request *p, struct list_head *head)
{
	list_add_tail(&p->list, head);
}

static inline struct usb_request *DEQUE(struct list_head *head)
{
	struct usb_request *tmp;
        tmp = list_first_entry(head, struct usb_request, list);
	list_del(&tmp->list);
	return tmp;
}

static void init_request(void)
{
	int i;

        INIT_LIST_HEAD(&_garmin_dev->req_head_in);
        INIT_LIST_HEAD(&_garmin_dev->req_head_out);
        INIT_LIST_HEAD(&_garmin_dev->req_head_int);

	for (i = 0; i < REQ_SIZE; ++i) {
		req_array_in[i].buf = kmalloc(4096, GFP_KERNEL);
		req_array_out[i].buf = kmalloc(4096, GFP_KERNEL);
		req_array_int[i].buf = kmalloc(4096, GFP_KERNEL);
	}
}

static void show(struct list_head *head)
{
	struct list_head *p, *n;
	struct usb_request *req;
	unsigned char *ptr;

	list_for_each_safe(p, n, head) {
		req = list_entry(p, struct usb_request, list);
		ptr = req->buf;
		printk(KERN_ALERT "%d\n", *ptr);
	}
}

static int hello_init(void)
{
	int order[] = {1,3,5,2,4};
	int i;
	_garmin_dev = kzalloc(sizeof(struct f_garmin), GFP_KERNEL);
	
	init_request();

	for (i = 0; i < sizeof(order)/sizeof(int); ++i) {
		unsigned char *ptr = req_array_out[order[i]].buf;
		*ptr = (unsigned char)order[i];
		QUEUE(&req_array_out[order[i]], &_garmin_dev->req_head_out);
	}

	printk(KERN_ALERT "\003[1;32mBefore:\033[0;37m\n");
	show(&_garmin_dev->req_head_out);

	struct usb_request *req = DEQUE(&_garmin_dev->req_head_out);
	printk(KERN_ALERT "We deque: %d\n", *(unsigned char*)req->buf);
	printk(KERN_ALERT "After:\n");
	show(&_garmin_dev->req_head_out);



        return 0;

}

static void free_request(struct list_head *head)
{
	struct list_head *p, *n;
	struct usb_request *req;
	unsigned char *ptr;

	list_for_each_safe(p, n, head) {
		req = list_entry(p, struct usb_request, list);
		ptr = req->buf;
		printk(KERN_ALERT "We'll free %d\n", *ptr);
		list_del(&req->list);
		kfree(req->buf);
	}
}

static void hello_exit(void)
{
	free_request(&_garmin_dev->req_head_in);
	free_request(&_garmin_dev->req_head_out);
	free_request(&_garmin_dev->req_head_int);
}

module_init(hello_init);
module_exit(hello_exit);
