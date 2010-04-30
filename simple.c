#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/compat.h>
#include <linux/list.h>

MODULE_LICENSE("Dual BSD/GPL");

struct request_list {
	struct list_head list;
	int data;
};

struct request_list req_head;

struct request_list *alloc(void)
{
	struct request_list *p = kmalloc(sizeof(struct request_list), GFP_KERNEL);
	if (likely(p))
		return p;
	else {
		printk(KERN_ALERT "error\n");
		return NULL;
	}
}

void show(void)
{
	struct list_head *p;
	struct request_list *my;

	list_for_each(p, &req_head.list) {
		my = list_entry(p, struct request_list, list);
		printk(KERN_ALERT "%d", my->data);
	}
	printk(KERN_ALERT "\n");
}

static inline void PUSH(struct request_list *p)
{
	list_add(&p->list, &req_head.list);
}

static inline void POP(void)
{
}

static int hello_init(void)
{
	struct request_list *p;

	memset(&req_head, 0, sizeof(req_head));
	INIT_LIST_HEAD(&req_head.list);

	p = alloc();
	if (p) {
		p->data = 1;
		PUSH(p);
	}

	p = alloc();
	if (p) {
		p->data = 2;
		PUSH(p);
	}

	p = alloc();
	if (p) {
		p->data = 3;
		PUSH(p);
	}
	
	show();
        return 0;

}

static void hello_exit(void)
{
	struct list_head *p, *n;
	struct request_list *my;

	list_for_each_safe(p, n, &req_head.list) {
		my = list_entry(p, struct request_list, list);
		printk(KERN_ALERT "We'll free %d\n", my->data);
//		list_del(&my->list);
//		kfree(my);
	}
	int *ptr = kmalloc(1048576*128, GFP_KERNEL);
	if (ptr) {
		printk(KERN_ALERT "成功\n");
	} else {
		printk(KERN_ALERT "失敗\n");
	}
}

module_init(hello_init);
module_exit(hello_exit);
