/*
 * task25.c
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
#include <linux/delay.h>
#include <asm/uaccess.h>


/* =============================================== */
#include "task25.h"

#define BUFSIZE 64 /* bufsize, KB */
#define LOG "task25: "

static int
compressor_open(struct inode* inode, struct file* filp);
static int
compressor_release(struct inode* inode, struct file* filp);

struct file_operations fops = {
	.open = compressor_open,
	.release = compressor_release
};

/* =============================================== */
static int __init task25_init(void) {
	int err = 0;
	pr_info(LOG "compressor started\n");

	return 0;
}

static void __exit task25_exit(void) {

	pr_info(LOG "compressor exit\n");
}

/* =============================================== */

static int
compressor_open(struct inode* inode, struct file* filp) {
    return 0;
}

static int
compressor_release(struct inode* inode, struct file* filp) {
    return 0;
}

/* =============================================== */

module_init(task25_init);
module_exit(task25_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.5");

