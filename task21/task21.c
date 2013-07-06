/*
 * task21.c
 *
 *      Author: Maxim Kouprianov
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

/* params */
static unsigned int num = 1; /* number of devices to create */

module_param(num, int, S_IRUGO);
MODULE_PARM_DESC(num, "number of devices to create (1-8)");
/* end params */

static dev_t first; /* first device number for driver */
static struct class *poums_class = NULL;/* ptr to device's class object */
static struct poums_device *poums_devices = NULL; /* array to store devices */

static int __init
task21_init(void) {
    pr_info("Xlab's device init\n");
    int err;

    if(num)

    err = alloc_chrdev_region(&first/*where to put*/, 0/*baseminor*/, num/*count*/, BASENAME/*name*/);
    if(err < 0) {
        pr_err("unable to allocate %d chrdev regions: %d", num, err);
        return err;
    }

    poums_class = class_create(THIS_MODULE/*owner*/, BASENAME/*name*/);
    if(IS_ERR(poums_class)) {
        pr_err("unable to create sysfs class\n");
        err = PTR_ERR(poums_class);
        goto out_class;
    }

    unsigned int created_num;
    dev_t curr;

    /* create @num devices */
    for(created_num = 1; created_num <= num; ++created_num) {
        curr = MKDEV(MAJOR(first), created_num - 1);
        struct device *dev = device_create(poums_class/*class*/, NULL/*parent*/,
            curr/*devt*/, NULL/*data*/, "poums%d", created_num - 1)
        
        if(IS_ERR(dev)) {
            /* fail at create */
            pr_err("unable to allocate %d devices, failed at %d\n", num, created_num);
            err = -ENODEV;
            goto out_devcreate;
        }
    }

    BUG_ON(created_num != num);

    pr_info("Xlab's device registered successfully\n");
	return 0;

    out_devcreate:
        cleanup_created_devices(created_num);
        
    out_class:
        class_destroy(poums_class);

    out_reg:
        unregister_chrdev_region(first, num);

    return err;
}

static void __exit
task21_exit(void) {
	pr_info("Xlab's device exit\n");
    cleanup_created_devices(num);
    class_destroy(poums_class);
    unregister_chrdev_region(first, num);
}

static void
cleanup_created_devices(unsigned int num) {
    dev_t curr;

    for(; num--;) {
        pr_debug("cleanup_created_devices, wiping %d\n", num);
        curr = MKDEV(MAJOR(first), num);
        device_destroy(poums_class, curr);
    }
}

module_init(task21_init);
module_exit(task21_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.1");

