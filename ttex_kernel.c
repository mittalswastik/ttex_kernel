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

static struct kprobe kp;

#define WR_VALUE _IOR('S',2,int) // start the timer
#define WR_VALUE _IOR('U',3,int) // modify the timer
#define WR_VALUE _IOR('D',3,int) // delete the timer

int32_t value = 0;
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
/*
** Function Prototypes
*/
static int      __init etx_driver_init(void);
static void     __exit etx_driver_exit(void);
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
         switch(cmd) {
                case WR_VALUE:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Value = %d\n", value);
                        break;
                // case RD_VALUE:
                //         if( copy_to_user((int32_t*) arg, &value, sizeof(value)) )
                //         {
                //                 pr_err("Data Read : Err!\n");
                //         }
                //         break;
                default:
                        pr_info("Default\n");
                        break;
        }
        return 0;
}

/* Pre-handler function */
static int pre_handler(struct kprobe *p, struct pt_regs *regs)
{
    /* You can add your custom code here to execute before the original function */

    /* Print a message to the kernel log */
    pr_info("Pre-handler: context_switch function intercepted!\n");

    /* Return 0 to indicate success */
    return 0;
}

/* Post-handler function */
static void post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    /* You can add your custom code here to execute after the original function */

    /* Print a message to the kernel log */
    pr_info("Post-handler: context_switch function intercepted!\n");
}

/* Module initialization function */
static int __init ttex_kernel_init(void)
{
    int ret;

    /* Initialize the kprobe structure */
    kp.pre_handler = pre_handler;
    kp.post_handler = post_handler;
    kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name("__switch_to");

    if (!kp.addr) {
        pr_err("Failed to find the address of context_switch function\n");
        return -EINVAL;
    }

    /* Register the kprobe */
    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("Failed to register kprobe (%d)\n", ret);
        return ret;
    }

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
r_class:
        unregister_chrdev_region(dev,1);
        return -1;
}

/* Module cleanup function */
static void __exit ttex_kernel_exit(void)
{
    int ret;

    /* Unregister the kprobe */
    unregister_kprobe(&kp);

    ret = cdev_add(&etx_cdev, dev, 1);
    if (ret < 0) {
        pr_err("Failed to add character device\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    pr_info("Kprobe unregistered\n");
}

module_init(ttex_kernel_init);
module_exit(ttex_kernel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Swastik");
MODULE_DESCRIPTION("Ttex Module");