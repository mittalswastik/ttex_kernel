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

static struct kprobe kp;
static struct mutex map_lock;

#define max_threads 50
int32_t value = 0;

typedef struct modified_timer{
        int32_t id;
        unsigned long int time_val;
} updated_timer;

typedef struct receive_timer {
    int32_t id;
    unsigned long int time_val_to_sub;
    unsigned long int time_val;
} receive_timer;

typedef struct thread_data {
    struct timer_list thread_timer;
    unsigned long pending_jiffies;
    unsigned long paused_time;
    unsigned long total_paused_time;
    atomic_ulong mod_msecs;
    atomic_bool timer_set;
    atomic_bool mod_timer;
    atomic_bool timer_triggered;
    atomic_bool context_switch_start;
} thread_data;

typedef struct map_node {
    int32_t key; // thread id will be the key
    thread_data info;
    struct map_node *next;
} map_node;

typedef struct thread_map_struct {
   map_node* array[max_threads];
   int arr_size;
} thread_map_struct;

int32_t hashFunction(int32_t key){
    return key % max_threads;
}

// Function to create a new node
map_node* createNode(int key, thread_data val) {
    struct map_node* newNode = kmalloc(sizeof(map_node), GFP_KERNEL);
    if (!newNode) {
        printk(KERN_ALERT "Failed to allocate memory for a new node\n");
        return NULL;
    }
    newNode->key = key;
    newNode->info = val;
    newNode->next = NULL;
    return newNode;
}

// Function to insert a key-value pair into the hash map
void insert(thread_map_struct* map, int key, thread_data val) {
    int index = hashFunction(key);
   map_node* newNode = createNode(key, val);

    if (map->array[index] == NULL) {
        map->array[index] = newNode;
        map->arr_size = map->arr_size + 1;
    } else {
        // Collision: append the newmap_node to the existing linked list
        map_node* curr = map->array[index];
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = newNode;
    }
}

// Function to retrieve the value associated with a given key from the hash map
thread_data* get(thread_map_struct* map, int key) {
    int32_t index = hashFunction(key);
    map_node* curr = map->array[index];

    while (curr != NULL) {
        if (curr->key == key) {
            return &(curr->info);
        }
        curr = curr->next;
    }

    // Key not found
    return NULL;
}

thread_map_struct *thread_map;

#define START_TIMER _IOW('S',2,int32_t*) // start the timer
#define MOD_TIMER _IOW('U',3,int32_t*) // modify the timer
#define DEL_TIMER _IOW('D',3,int32_t*) // delete the timer
#define MOD_TIMER_NEW _IOW('UT',3,updated_timer*)
#define GET_TIMER _IOWR('R',3, receive_timer*) //
 
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

void timer_callback(struct timer_list* data){
        printk(KERN_INFO "timer_callback\n");

        // set timer triggered callback to false
        int i;
        for(i = 0; i < max_threads; i++) {
            thread_data *temp = get(thread_map,i);
            if(temp != NULL && data == &(temp->thread_timer)){
                // there should exist an i for which this is true
                __atomic_store_n(&(temp->timer_triggered), 1, __ATOMIC_RELAXED);
            }
        }
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
         unsigned long int get_kern_time;
         updated_timer timer_test;
         receive_timer timer_return;
         thread_data *temp_thread_data;
         //temp_thread_data = kmalloc(sizeof(thread_data), GFP_KERNEL);

         switch(cmd) {
                case START_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Write : Err!\n");
                        }
                        pr_info("Start Timer = %d\n", value);
			//thread_data *temp_thread_data;
			temp_thread_data=kmalloc(sizeof(thread_data),GFP_KERNEL);
                        __atomic_store_n(&(temp_thread_data->timer_set), 1, __ATOMIC_RELAXED);
                        __atomic_store_n(&(temp_thread_data->mod_timer), 0, __ATOMIC_RELAXED);
                        __atomic_store_n(&(temp_thread_data->mod_msecs), 0, __ATOMIC_RELAXED);
                        __atomic_store_n(&(temp_thread_data->context_switch_start), 0,__ATOMIC_RELAXED);
                        __atomic_store_n(&(temp_thread_data->timer_triggered), 0, __ATOMIC_RELAXED);
                        temp_thread_data->pending_jiffies = 0;
                        temp_thread_data->paused_time = 0;
                        temp_thread_data->total_paused_time = 0;
                        printk(KERN_INFO "Inserting a new timer node to the map\n");
                        //mutex_lock(&map_lock);
                        timer_setup(&(temp_thread_data->thread_timer), timer_callback, 0);
                        insert(thread_map, value, *temp_thread_data);
                        printk(KERN_INFO "Inserted the timer node\n");
                        //mutex_unlock(&map_lock);
                        pr_info("timer has been setup \n");
                        //mod_timer(&thread_timer[timer_id], jiffies + msecs_to_jiffies(10000));
                        pr_info("Timer Started\n");
                        if(__atomic_load_n(&temp_thread_data->context_switch_start, __ATOMIC_RELAXED)){
                            schedule();
                            __atomic_store_n(&(temp_thread_data->context_switch_start), 1,__ATOMIC_RELAXED); // it will be reset to 1 at schedule so reset after it
                        }
                        break;
                case MOD_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }

                        temp_thread_data = get(thread_map,value); 
                        if(temp_thread_data == NULL) {
                         pr_info("it should not be null as the timer has started");   
                        }
                        else {
                            __atomic_store_n(&(temp_thread_data->mod_timer), 1,__ATOMIC_RELAXED);
                        }
                        pr_info("modifying the timer for id %d\n", value);
                        break;
                case DEL_TIMER:
                        if( copy_from_user(&value ,(int32_t*) arg, sizeof(value)) )
                        {
                                pr_err("Data Read : Err!\n");
                        }
                        temp_thread_data = get(thread_map,value); 
                        if(temp_thread_data == NULL) {
                         pr_info("it should not be null as the timer has started");   
                        }
                        else {
                            del_timer(&(temp_thread_data->thread_timer));
                            __atomic_store_n(&(temp_thread_data->timer_set), 0,__ATOMIC_RELAXED);
                            // kfree the memory here if needed
                        }
                        break;
                case MOD_TIMER_NEW:
                        if(copy_from_user(&timer_test, (updated_timer*) arg, sizeof(timer_test))){
                                pr_err("mod timer error\n");
                        }
                       // printk(KERN_INFO "--------mod timer called with an updated time value------%d\n", timer_test.time_val);
                        temp_thread_data = get(thread_map,timer_test.id); 
                        if(temp_thread_data == NULL) {
                         //pr_info("it should not be null as the timer has started"); 
                        //  exit(0);
				pr_err("Mod timer cannot be called outside any thread");
                        }

                        else {
                            //try with a sleep
                            __atomic_store_n(&(temp_thread_data->mod_msecs), timer_test.time_val,__ATOMIC_RELAXED);
                            if(__atomic_load_n(&temp_thread_data->mod_msecs, __ATOMIC_RELAXED) != 0){ // context switch has not been triggered so we trigger it
                                //pr_info("---------- context switch did not modify the timer ############ %d\n", task_cpu(current));
                                schedule();
                                //msleep(3000);
                            }

                           // pr_info("----------------------- checking for context switch ############### %d\n", task_cpu(current));
                        }
                        //printk(KERN_INFO "--------mod timer with time value------\n");
                        break;
                case GET_TIMER:
                        if(copy_from_user(&timer_return, (receive_timer*) arg, sizeof(timer_return))){
                                pr_err("mod timer error\n");
                        }
                        temp_thread_data = get(thread_map,timer_return.id);
                        if(temp_thread_data == NULL){
                            pr_info("Get timer cannot be called outside a thread\n");// do nothing just return time
                        }
                        
                        else {
                            get_kern_time = ktime_get_ns()-timer_return.time_val_to_sub-temp_thread_data->total_paused_time;
                            __atomic_store_n(&(timer_return.time_val), get_kern_time, __ATOMIC_RELAXED);
                            if( copy_to_user((receive_timer*) arg, &timer_return, sizeof(timer_return)))
                            {
                                    pr_err("Data Sent : Err!\n");
                            }
                        }

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

//     struct task_struct *pre_task = regs->di;
//     struct task_struct *post_task = regs->si;

//     if(pre_task->rt_priority != 0){
//         //if(mutex_trylock(&map_lock) == 0){
//         if(thread_map->arr_size != 0){
//             thread_data *temp = get(thread_map,pre_task->pid);
//             if(temp == NULL) {
//                 //pr_info("thread timer not yet formed");   
//             }
            
//             else {
//                 //pr_info("context switch pre handler\n");
//                 if(__atomic_load_n(&(temp->timer_set), __ATOMIC_RELAXED)){
//                 //if(temp->thread_timer.function != NULL){
//                     //pr_info("pre handler timer set is already enabled\n");
//                     //if(timer_pending(&(temp->thread_timer))){
//                     if(!__atomic_load_n(&(temp->timer_triggered), __ATOMIC_RELAXED)){  
//                         temp->pending_jiffies = temp->thread_timer.expires - jiffies;
//                         temp->paused_time = ktime_get_ns();
//                         del_timer(&(temp->thread_timer));
//                         //pr_info("stopping the timer\n");
//                     }
//                 }
//             }
//         }
//         //}
//    }

    /* Return 0 to indicate success */
    return 0;
}

/* Post-handler function */
static void post_handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    /* You can add your custom code here to execute after the original function */

        // struct task_struct *pre_task = regs->di;
        // struct task_struct *post_task = regs->si;

        // if(post_task->prio == 10){
        //         printk(KERN_INFO "post context switch out priority\n");
        // }

        // if(pre_task->rt_priority != 0){
        //     if(thread_map->arr_size != 0){
        //         thread_data *temp = get(thread_map,post_task->pid);
        //         if(temp == NULL) {
        //            // pr_info("thread timer not yet formed");   
        //         }
        //         else {
        //             // pr_info("context switch post handler\n");
        //             if(__atomic_load_n(&(temp->timer_set), __ATOMIC_RELAXED)){
        //               //if(temp->thread_timer.function != NULL){
        //                // pr_info("post handler timer set is already enabled\n");
        //                 //if(timer_pending(&(temp->thread_timer))){
        //                 if(!__atomic_load_n(&(temp->timer_triggered), __ATOMIC_RELAXED)){  
        //                     if(__atomic_load_n(&(temp->mod_msecs), __ATOMIC_RELAXED) != 0){
        //                         del_timer(&temp->thread_timer);
        //                         mod_timer(&temp->thread_timer, jiffies + msecs_to_jiffies(temp->mod_msecs));
        //                         temp->total_paused_time = 0;
        //                         __atomic_store_n(&temp->mod_msecs, 0, __ATOMIC_RELAXED);
        //                         //printk(KERN_INFO "----- modified the timer ----- %d\n", task_cpu(post_task));
        //                     }

        //                     else {
        //                         temp->total_paused_time += ktime_get_ns()-temp->paused_time;
        //                         mod_timer(&temp->thread_timer, jiffies + temp->pending_jiffies);
        //                        // printk(KERN_INFO "----- just update the timer ----\n");
        //                     }
                        
        //                     __atomic_store_n(&temp->context_switch_start, 1, __ATOMIC_RELAXED);

        //                 }
        //             }
        //         }
        //     }
        // }

    /* Print a message to the kernel log */
   // pr_info("Post-handler: context_switch function intercepted!\n");
}

/* Module initialization function */
static int __init ttex_kernel_init(void)
{
    int ret, i;

    /* Initialize the kprobe structure */
    kp.pre_handler = pre_handler;
    kp.post_handler = post_handler;
    kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name("__switch_to");
    
    mutex_init(&map_lock);
    thread_map = kmalloc(sizeof(thread_map_struct), GFP_KERNEL);
    thread_map->arr_size = 0;

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
    mutex_destroy(&map_lock);

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
