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
    int err = 0;

    //if(num)

    /* allocate region */
    err = alloc_chrdev_region(&first/*where to put*/, 0/*baseminor*/, num/*count*/, BASENAME/*name*/);
    if(err < 0) {
        pr_err("unable to allocate %d chrdev regions: %d", num, err);
        return err;
    }

    /* create class */
    poums_class = class_create(THIS_MODULE/*owner*/, BASENAME/*name*/);
    if(IS_ERR(poums_class)) {
        pr_err("unable to create sysfs class\n");
        err = PTR_ERR(poums_class);
        goto out_class;
    }

    /* init and register cdevs */
    poums_devices = (struct poums_device *)kcalloc(num,
        sizeof(struct poums_device), GFP_KERNEL);

    if(!poums_devices) {
        pr_err("unable to allocate %d devices: out of free memory\n", num);
        err = -ENOMEM;
        goto out_devcreate;
    }

    int init_num;
    for(init_num = 0; init_num < num; ++init_num) {

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
        destroy_created_devices(created_num, poums_class);

    out_devinit:
        cleanup_initialized_devices(init_num);
        
    out_class:
        class_destroy(poums_class);

    out_reg:
        unregister_chrdev_region(first, num);

    return err;
}

static void __exit
task21_exit(void) {
	pr_info("Xlab's device exit\n");
    destroy_created_devices(num, poums_class);
    class_destroy(poums_class);
    unregister_chrdev_region(first, num);
}

static void
destroy_created_devices(unsigned int num, struct class *dev_class) {
    dev_t curr;

    for(; num--;) {
        pr_debug("cleanup_created_devices, wiping %d\n", num);
        curr = MKDEV(MAJOR(first), num);
        device_destroy(dev_class, curr);
    }
}

static void deinit_poums_devices(unsigned int num) {
    struct poums_device *dev;

    if(poums_devices) {
        for(; num--;) {
            dev = poums_devices[num];
            cdev_del(&dev->cdev); /* deinit cdev */
            kfree(dev->data); /* cleanup allocated storage */
        }
    } else {
        pr_debug("Nothing to deinit. Wat?\n");
    }    
}

static int
poums_init_device(struct poums_device *dev, unsigned int minor) {
    BUG_ON(dev == NULL)
    int err = 0;

    dev->data = NULL;
    dev->size = 0;
    cdev_init(&dev->cdev, &poums_fops);
    dev->cdev.owner = THIS_MODULE;

    err = cdev_add(&dev->cdev, MKDEV(MAJOR(first), minor), 1/*count*/);
    if(err < 0) {
        pr_ // XXX: TODO
    }
}

module_init(task21_init);
module_exit(task21_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.1");

