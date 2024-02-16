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
#include <linux/delay.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to measure ktime_sub execution time");
MODULE_VERSION("1.0");

int32_t value = 0;

typedef struct receive_timer {
    unsigned long int time_val_to_sub;
    unsigned long int time_val;
} receive_timer;

#define GET_TIMER _IOWR('R',3, receive_timer*)
#define START_TIMER _IOW('S',2,int32_t*) // start the timer
#define RET_TIMER _IOR('W',1,int64_t*) //

dev_t dev;
static struct class *dev_class;
static struct cdev etx_cdev;
/*
** Function Prototypes
*/
static int      __init ktime_sub_module_init(void);
static void     __exit ktime_sub_module_exit(void);
static int      etx_open(struct inode *inode, struct file *file);
static int      etx_release(struct inode *inode, struct file *file);
static ssize_t  etx_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t  etx_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long     etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .unlocked_ioctl = etx_ioctl,
        .release        = etx_release,
};

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("Device File Opened...!!!\n");
        return 0;
}
/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("Device File Closed...!!!\n");
        return 0;
}
/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read Function\n");
        return 0;
}
/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write function\n");
        return len;
}

static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
         int timer_id;
         receive_timer timer_return;
         unsigned long int get_kern_time;

         printk(KERN_INFO "etx_ioctl\n");

         switch(cmd) {
                
                case START_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Start Timer = %d\n", value);
                        break;

                case GET_TIMER:
                        if(copy_from_user(&timer_return, (receive_timer*) arg, sizeof(timer_return))){
                                pr_err("mod timer error\n");
                        }

                        printk(KERN_INFO "Evaluate timer%d\n", timer_return.time_val_to_sub);
                        get_kern_time = ktime_get_ns()-timer_return.time_val_to_sub;
                        __atomic_store_n(&(timer_return.time_val), get_kern_time, __ATOMIC_RELAXED);
                        if( copy_to_user((receive_timer*) arg, &timer_return, sizeof(timer_return)))
                        {
                                pr_err("Data Sent : Err!\n");
                        }
                        break;
                case RET_TIMER:;
                        ktime_t test;
                        ktime_t start = ktime_get();
                        test = ktime_get();
                        int64_t time_val;
                        time_val = ktime_to_ns(test);    
                        if(copy_to_user((int64_t*) arg, &(time_val), sizeof(time_val))){
                            pr_err("Data Sent : Err!\n");
                        }
                        ktime_t end = ktime_get();
                        printk(KERN_INFO "Evaluate timer%d\n", ktime_to_ns(ktime_sub(end,start)));
                        break;
                default:
                        pr_info("Default\n");
                        break;
        }
        return 0;
}

static int __init ktime_sub_module_init(void) {
    
    if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

    /*Creating cdev structure*/
    cdev_init(&etx_cdev,&fops);

    /*Adding character device to the system*/
    if((cdev_add(&etx_cdev,dev,1)) < 0){
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }

    /*Creating struct class*/
    if(IS_ERR(dev_class = class_create(THIS_MODULE,"etx_class"))){
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }

    /*Creating device*/
    if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
        pr_err("Cannot create the Device 1\n");
        goto r_device;
    }
    pr_info("Device Driver Insert...Done!!!\n");
return 0;
 
r_device:
        class_destroy(dev_class);
        //unregister_chrdev_region(dev,1);
        //return -1;
r_class:
        //class_destroy(dev_class);
        unregister_chrdev_region(dev,1);
        return -1;


    return 0;
}

static void __exit ktime_sub_module_exit(void) {
    device_destroy(dev_class,dev); 
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "ktime_sub_module: Module exited\n");
}

module_init(ktime_sub_module_init);
module_exit(ktime_sub_module_exit);
