#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>

MODULE_LICENSE("Dual BSD/GPL");

#define REQ_SIZE 32
struct f_garmin {
	struct list_head req_head_in;
	struct list_head req_head_out;
	struct list_head req_head_int;
	wait_queue_head_t wait;
	int data_received;
};

static struct f_garmin *_garmin_dev;
struct usb_request req_array_in[REQ_SIZE];
struct usb_request req_array_out[REQ_SIZE];
struct usb_request req_array_int[REQ_SIZE];

char global_buf[64];

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
#if 0
static int hello_init2(void)
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
#endif

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

static int doremi_open(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "[doremi] open\n");
	return 0;
}

static int doremi_release(struct inode *inode, struct file *file)
{
	//printk(KERN_ALERT "[doremi] release\n");
	return 0;
}

static ssize_t doremi_read(struct file *file, char __user *buffer,
                             size_t count, loff_t *ppos)
{
	//printk(KERN_ALERT "[doremi] read\n");
	wait_event_interruptible(_garmin_dev->wait, (_garmin_dev->data_received == 1));

	_garmin_dev->data_received = 0;
	printk(KERN_ALERT "We will return\n");
	if (copy_to_user(buffer, global_buf, 4))
		return -EFAULT;
//	_garmin_dev->data_received = 0;
	return 4;
}

static ssize_t doremi_write(struct file *file, const char __user *buffer,
                                size_t count, loff_t *ppos)
{
	//printk(KERN_ALERT "[doremi] write\n");
	if (copy_from_user(global_buf, buffer, 4))
		return -EFAULT;
	_garmin_dev->data_received = 1;
	wake_up(&_garmin_dev->wait);
	return 4;
}

static const struct file_operations doremi_fops = {
        .owner          = THIS_MODULE,
	.read		= doremi_read,
        .write          = doremi_write,
        .open           = doremi_open,
        .release        = doremi_release,
};

static struct miscdevice doremi_miscdev = {
        .minor          = 20,
        .name           = "doremi",
        .fops           = &doremi_fops,
};

static void hello_exit(void)
{
        free_request(&_garmin_dev->req_head_in);
        free_request(&_garmin_dev->req_head_out);
        free_request(&_garmin_dev->req_head_int);

	misc_deregister(&doremi_miscdev);
}

static int hello_init(void)
{
	int ret;

	ret = misc_register(&doremi_miscdev);
	if (ret) {
		printk(KERN_ALERT "Can't register\n");
		return ret;
	}
	_garmin_dev = kzalloc(sizeof(struct f_garmin), GFP_KERNEL);
	init_waitqueue_head(&_garmin_dev->wait);
	return 0;
}

module_init(hello_init);
module_exit(hello_exit);
