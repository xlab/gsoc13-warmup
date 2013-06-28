/*
 * task21.c
 *
 *  Created on: 26 Jun 2013
 *      Author: Maxim Kouprianov
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static int num = 1; // number of devices

static int __init task21_init(void) {
	printk(KERN_DEBUG "Xlab's device allocator: allocating %d devices\n", num);
	return 0;
}

static void __exit task21_exit(void) {
	printk(KERN_DEBUG "Xlab's task21 exit\n");
}

module_param(num, int, 0);
MODULE_PARM_DESC(num, "number of devices to create");

module_init(task21_init);
module_exit(task21_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.1");

