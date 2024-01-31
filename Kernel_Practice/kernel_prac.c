#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/kprobes.h>
#include <linux/timer.h>
#include <stdatomic.h>
#include <linux/jiffies.h>

int add_numbers(void) {
    int result;
    int a = 5;
    int b = 3;

    asm volatile(
        "addl %2, %0"
        : "=r" (result)
        : "0" (a), "r" (b)
    );

    printk(KERN_INFO "Result: %d, %d, %d\n", a,b,result);
    return result;
}

static int (*functionPtr)(void);


static int __init kernel_init(void)
{
    functionPtr = add_numbers;
    int test = functionPtr();
}

/* Module cleanup function */
static void __exit kernel_exit(void)
{
    printk(KERN_INFO "kernel exit\n");
}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Swastik");
MODULE_DESCRIPTION("Prac Module");