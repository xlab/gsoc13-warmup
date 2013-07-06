/*
 * task21.h
 *
 *      Author: Maxim Kouprianov
 */

#define NUM_MAX 8 /* maximum number of devices */
#define BUFFSIZE 4096 /* default buffer size to store char-data */
#define BASENAME "poums" /* defines is a good thing xD */

/* helpers */
static void
cleanup_registered_devices(unsigned int num);
static int
poums_init_device(struct poums_device *dev);

/* fops */
static ssize_t
poums_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t
poums_write(struct file *, const char __user *, size_t, loff_t *);
static loff_t
poums_llseek(struct file *, loff_t, int);
static int
poums_open(struct inode *, struct file *);
static int
poums_release(struct inode *, struct file *);

struct file_operations poums_fops = {
    .owner = THIS_MODULE,
    .open = poums_open,
    .release = poums_release,
    .read = poums_read,
    .write = poums_write,
    .llseek = no_llseek
};
/* end fops */

/* device representation */
struct poums_device {
    char *data; /* char buf */
    unsigned long size; /* amount of data stored in buf */
    struct cdev cdev;
};