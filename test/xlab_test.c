/*
 * xlab_test.c
 *
 *  Created on: 26 Jun 2013
 *      Author: xlab
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static int num = 1; // number of devices

static int __init test_init(void) {
	printk(KERN_ALERT "Xlab's device allocator: allocating %d devices\n", num);
	return 0;
}

static void __exit test_exit(void) {
	printk(KERN_ALERT "Xlab's module exit\n");
}

module_param(num, int, S_IRUGO);

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("A test module to play around with krn dev");

