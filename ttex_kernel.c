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

static struct kprobe kp;

#define PARENT_ID _IOW('P',2,int32_t*) // start the timer
#define START_TIMER _IOW('S',2,int32_t*) // start the timer
#define MOD_TIMER _IOW('U',3,int32_t*) // modify the timer
#define DEL_TIMER _IOW('D',3,int32_t*) // delete the timer

#define max_threads 50

int32_t value = 0;
int32_t parent_id = 0;
 
dev_t dev;
static struct class *dev_class;
static struct cdev etx_cdev;
/*
** Function Prototypes
*/
static int      __init ttex_kernel_init(void);
static void     __exit ttex_kernel_exit(void);
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

struct timer_list thread_timers[max_threads];
unsigned long pending_jiffies[max_threads];
atomic_bool timer_set[max_threads];
atomic_bool modified_timer[max_threads];
atomic_bool context_switch_started_timer[max_threads];

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
         int timer_id;

         switch(cmd) {
                case PARENT_ID:
                        if( copy_from_user(&parent_id ,(int32_t*) arg, sizeof(parent_id)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Parent Value = %d\n", parent_id);
                        break;
                case START_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Start Timer = %d\n", value);
                        timer_id = (int) value-parent_id;
                        pr_info("timer id value is = %d\n", timer_id);
                        timer_setup(&thread_timers[timer_id], timer_callback, 0);
                        //mod_timer(&thread_timers[timer_id], jiffies + msecs_to_jiffies(10000));
                        pr_info("timer has been setup \n");
                        __atomic_store_n(&timer_set[timer_id],1,__ATOMIC_RELAXED);
                        //mod_timer(&thread_timers[timer_id], jiffies + msecs_to_jiffies(10000));
                        pr_info("Timer Started\n");
                        break;
                case MOD_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        timer_id = (int) value-parent_id;
                        pr_info("modifying the timer for id %d\n", timer_id);
                        __atomic_store_n(&modified_timer[timer_id], 1,__ATOMIC_RELAXED);
                        break;
                case DEL_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        timer_id = (int) value-parent_id;
                        __atomic_store_n(&timer_set[timer_id],0,__ATOMIC_RELAXED);
                        del_timer(&thread_timers[timer_id]);
                        break;
                default:
                        pr_info("Default\n");
                        break;
        }
        return 0;
}

/* Pre-handler function */
static int pre_handler(struct kprobe *p, struct pt_regs *regs) // kprobe handlers are sequential
{
    /* You can add your custom code here to execute before the original function */

    struct task_struct *pre_task = regs->di;
    struct task_struct *post_task = regs->si;

    if(pre_task->rt_priority == 10){
        
        int timer_id = (int) (pre_task->pid - parent_id);
        //if(__atomic_load_n(&timer_set[timer_id], __ATOMIC_RELAXED)){
        if(thread_timers[timer_id].function != NULL){
                if(timer_pending(&thread_timers[timer_id])){
                        printk(KERN_INFO "task context switching out\n");
                        // __atomic_store_n(&timer_set[timer_id],0,__ATOMIC_RELAXED);
                        // store timers expiry time here
                        pending_jiffies[timer_id] = thread_timers[timer_id].expires - jiffies;
                        del_timer(&thread_timers[timer_id]);
                }
        }
   }

    /* Return 0 to indicate success */
    return 0;
}

/* Post-handler function */
static void post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    /* You can add your custom code here to execute after the original function */

        struct task_struct *pre_task = regs->di;
        struct task_struct *post_task = regs->si;

        // if(post_task->prio == 10){
        //         printk(KERN_INFO "post context switch out priority\n");
        // }

        if(post_task->rt_priority == 10){ // this will be updated to check task policy or something else as task can have any priority
                printk(KERN_INFO "task context switching out\n");
                int timer_id = (int) (post_task->pid - parent_id);
                //if(__atomic_load_n(&timer_set[timer_id], __ATOMIC_RELAXED)){
                if(thread_timers[timer_id].function != NULL) {
                        // if(!__atomic_load_n(&context_switch_started_timer[timer_id], __ATOMIC_RELAXED)){
                        //         // first need to start the timer
                        //         __atomic_store_n(&context_switch_started_timer[timer_id],1,__ATOMIC_RELAXED);
                        //         mod_timer(&thread_timers[timer_id], jiffies + msecs_to_jiffies(10000));
                        //         // if atomic for timer_setup was not set to 1 and it context switched out and then back in then we
                        //         // would have not started the timer hence need to start it outside of context switch
                        // }

                        //else
                        if(__atomic_load_n(&modified_timer[timer_id], __ATOMIC_RELAXED)){
                                // restart the timer
                                printk(KERN_INFO "task context switching in\n");
                                if(timer_pending(&thread_timers[timer_id])){
                                        __atomic_store_n(&modified_timer[timer_id],0,__ATOMIC_RELAXED);
                                        del_timer(&thread_timers[timer_id]);
                                        mod_timer(&thread_timers[timer_id], jiffies + msecs_to_jiffies(10000));
                                }
                        }

                        else {
                                if(timer_pending(&thread_timers[timer_id])){
                                        // just add pending timer
                                        mod_timer(&thread_timers[timer_id], jiffies + pending_jiffies[timer_id]);
                                }
                        }
                }
        }

    /* Print a message to the kernel log */
   // pr_info("Post-handler: context_switch function intercepted!\n");
}

/* Module initialization function */
static int __init ttex_kernel_init(void)
{
    int ret, i;

    for(i = 0 ; i < max_threads ; i++) {
        __atomic_store_n(&timer_set[i],0,__ATOMIC_RELAXED);
        __atomic_store_n(&modified_timer[i], 0, __ATOMIC_RELAXED);
        __atomic_store_n(&context_switch_started_timer[i], 0, __ATOMIC_RELAXED);
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
        //unregister_chrdev_region(dev,1);
        //return -1;
r_class:
        //class_destroy(dev_class);
        unregister_chrdev_region(dev,1);
        return -1;
}

/* Module cleanup function */
static void __exit ttex_kernel_exit(void)
{
    int ret;

    /* Unregister the kprobe */
    device_destroy(dev_class,dev); // &dev kept giving kernel panics as device never got destroyed
    //class_unregister(dev_class);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);

//     ret = cdev_add(&etx_cdev, dev, 1);
//     if (ret < 0) {
//         pr_err("Failed to add character device\n");
//         // 
//         return ret;
//     }

    unregister_kprobe(&kp);
    pr_info("Kprobe unregistered\n");
}

module_init(ttex_kernel_init);
module_exit(ttex_kernel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Swastik");
MODULE_DESCRIPTION("Ttex Module");