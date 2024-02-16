#include <linux/init.h>
#include <linux/module.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to measure ktime_sub execution time");
MODULE_VERSION("1.0");

static int __init ktime_sub_module_init(void) {
    ktime_t start_time, end_time, elapsed_time;
    
    printk(KERN_INFO "ktime_sub_module: Module initialized\n");

    int i ; 
	for(i=0 ; i < 10 ; i++){

		    
// Get the start time
    start_time = ktime_get();

    // Perform the ktime_sub operation (replace this with your specific use case)
    // For example, subtract two ktime values
    ktime_t time1 = ktime_set(0, 1000); // Replace this with your ktime values
    ktime_t time2 = ktime_set(0, 500);  // Replace this with your ktime values
    ktime_sub(time1, time2);

    // Get the end time
    end_time = ktime_get();

    // Calculate elapsed time
    elapsed_time = ktime_sub(end_time, start_time);

    // Print the result
    printk(KERN_INFO "ktime_sub_module: Time taken: %lld ns\n", ktime_to_ns(elapsed_time));
	}

    return 0;
}

static void __exit ktime_sub_module_exit(void) {
    printk(KERN_INFO "ktime_sub_module: Module exited\n");
}

module_init(ktime_sub_module_init);
module_exit(ktime_sub_module_exit);
