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

#define PARENT_ID _IOW('P',2,int32_t) // start the timer
#define START_TIMER _IOW('S',2,int32_t) // start the timer
#define MOD_TIMER _IOW('U',3,int32_t) // modify the timer
#define DEL_TIMER _IOW('D',3,int32_t) // delete the timer

int32_t value = 0;
int32_t parent_id = 0;
 
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

struct timer_list *thread_timers[20];
bool timer_set[20];

void timer_callback(struct timer_list* data){
        printk(KERN_INFO "timer_callback\n");
}

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
         int32_t timer_id;

         switch(cmd) {
                case PARENT_ID:
                        if( copy_from_user(&parent_id ,(int32_t*) arg, sizeof(parent_id)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Value = %d\n", parent_id);
                        break;
                case START_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Value = %d\n", value);
                        timer_id = value-parent_id;
                        timer_setup(thread_timers[timer_id], timer_callback, 0);
                        timer_set[timer_id] = true;
                        break;
                case MOD_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }

                        timer_id = value-parent_id;
                        break;
                case DEL_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        timer_id = value-parent_id;
                        del_timer(thread_timers[timer_id]);
                        timer_set[timer_id] = false;
                        break;
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

    struct task_struct *pre_task = regs->di;
    struct task_struct *post_task = regs->si;
    

    if(pre_task->rt_priority == 10){
        printk(KERN_INFO "task context switching out\n");
    }

    /* Print a message to the kernel log */
   // pr_info("Pre-handler: context_switch function intercepted!\n");

    /* Return 0 to indicate success */
    return 0;
}

/* Post-handler function */
static void post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    /* You can add your custom code here to execute after the original function */

    /* Print a message to the kernel log */
   // pr_info("Post-handler: context_switch function intercepted!\n");
}

/* Module initialization function */
static int __init ttex_kernel_init(void)
{
    int ret, i;

    for(i = 0 ; i < 20 ; i++) {
        timer_set[i] = false;
    }

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