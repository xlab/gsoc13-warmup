/*
 * task21.h
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

#include <asm/uaccess.h>

#define NUM_MAX 8 /* maximum number of devices */
#define BUFFSIZE 4096 /* default buffer size to store char-data */
#define BASENAME "poums" /* defines is a good thing xD */
#define LOG "task21: "

/* device representation */
struct poums_device {
	char *data; /* char buf */
	ssize_t size; /* amount of data stored in buf */
	dev_t devt; /* numbers for debug purposes */
	struct mutex mutex; /* mutex lock */
	struct cdev cdev;
};

/* helpers */
static int
init_poums_device(struct poums_device *dev, unsigned int minor);
static void
deinit_poums_devices(unsigned int num);
static void
destroy_created_devices(unsigned int num, struct class *dev_class);
/* =============================================== */

/* fops */
static int
poums_open(struct inode *, struct file *);
static int
poums_close(struct inode *, struct file *);
static ssize_t
poums_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t
poums_write(struct file *, const char __user *, size_t, loff_t *);
static loff_t
poums_llseek(struct file *, loff_t, int);

struct file_operations poums_fops = { .owner = THIS_MODULE, .open = poums_open,
		.release = poums_close, .read = poums_read, .write = poums_write,
		.llseek = no_llseek };

