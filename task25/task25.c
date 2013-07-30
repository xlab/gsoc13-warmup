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
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <linux/debugfs.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/lzo.h>


/* =============================================== */
#include "task25.h"

#define BUFSIZE (64 * 1024) /* bufsize */
#define LOG "task25: "
#define LZO_MINIMUM_CHUNK (4 * 1024) /* 4 KB */

#define lzo1x_worst_backwards(y) \
	((16 * ((y) - 64 - 3) / 17) - 4 - 4) /* with space for header */

struct dentry *file;
struct dentry *dir;

wait_queue_head_t inq, outq;
char *buffer, *end = 0;
char *rp, *wp = 0;
int nreaders, nwriters = 0;
struct mutex mutex;
void * lzo_wrkmem;

static int
compressor_open(struct inode* inode, struct file* filp);
static int
compressor_release(struct inode* inode, struct file* filp);
static ssize_t
compressor_read(struct file *fp, char __user *buf, size_t count,
        loff_t *pos);
static ssize_t
compressor_write(struct file *fp, const char __user *buf, size_t count,
        loff_t *pos);
static unsigned int
compressor_poll(struct file *filp, poll_table *wait);

struct file_operations compressor_fops = {
	.owner = THIS_MODULE,
    .open = compressor_open,
    .release = compressor_release,
    .read = compressor_read,
    .write = compressor_write,
    .poll = compressor_poll
};

static void write32(char *out, unsigned int n)
{
    unsigned char b[4];
    b[3] = (unsigned char) ((n >>  0) & 0xff);
    b[2] = (unsigned char) ((n >>  8) & 0xff);
    b[1] = (unsigned char) ((n >> 16) & 0xff);
    b[0] = (unsigned char) ((n >> 24) & 0xff);
    memcpy(out, b, 4);
}

/* =============================================== */
static int __init task25_init(void) {
	int err = 0;
    mutex_init(&mutex);
    init_waitqueue_head(&inq);
    init_waitqueue_head(&outq);

    lzo_wrkmem = kzalloc(LZO1X_1_MEM_COMPRESS, GFP_KERNEL);
    if(lzo_wrkmem == NULL) {
    	pr_err(LOG "unable to alloc working memory for LZO compression\n");
    	return -ENOMEM;
    }

    dir = debugfs_create_dir(COMPRESSOR_DIR, NULL);
    if(dir == NULL) {
        pr_err(LOG "unable to create compressor's dir in debugfs\n");
        err = -ENODEV;
        goto out;
    } else if(dir < 0) {
        pr_err(LOG "there is no support for debugfs in kernel\n");
        err = -ENODEV;
        goto out;
    }

    file = debugfs_create_file(COMPRESSOR_FILE, 0644,
                           dir, NULL, &compressor_fops);

    if(file == NULL || file < 0) {
        pr_err(LOG "unable to create compressor's file in debugfs\n");
        debugfs_remove(dir);
        err = -ENODEV;
        goto out;
    }

	buffer = kzalloc(BUFSIZE, GFP_KERNEL);
	if (!buffer) {
		pr_err("no memory for buffer\n");
	    debugfs_remove_recursive(dir);
        err = -ENOMEM;
        goto out;
	}

	rp = wp = buffer;
    end = buffer + BUFSIZE;

    pr_info(LOG "compressor started\n");
    return 0;

    out:
    	kfree(lzo_wrkmem);
    	return err;
}

static void __exit task25_exit(void) {
	kfree(lzo_wrkmem);
	kfree(buffer);
    debugfs_remove_recursive(dir);
    pr_info(LOG "compressor exit\n");
}

/* =============================================== */

static int
compressor_open(struct inode* inode, struct file* filp) {
    return nonseekable_open(inode, filp);
}

static int
compressor_release(struct inode* inode, struct file* filp) {
    return 0;
}

static ssize_t
compressor_read(struct file *fp, char __user *buf, size_t count,
        loff_t *pos) {
	int err = 0;

    if (mutex_lock_interruptible(&mutex)) {
        return -ERESTARTSYS;
    }

    while(rp == wp) { /* no data */
        mutex_unlock(&mutex);
        if(fp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        /* waiting for data & block */

        if(wait_event_interruptible(inq, rp != wp)) {
            return -ERESTARTSYS;
        }
        if(mutex_lock_interruptible(&mutex)) {
            return -ERESTARTSYS;
        }
    }

    /* data arrived */
    if(wp > rp) {
        count = min(count, (size_t)(wp - rp));
    } else {
        count = min(count, (size_t)(end - rp));
    }

    err = copy_to_user(__user buf, rp, count);
    if(err) {
        mutex_unlock(&mutex);
        pr_err(LOG "unable to write to user, error: %d\n", err);
        return -EFAULT;
    }

    rp += count;
    if(rp == end) {
        rp = buffer;
    }

    mutex_unlock(&mutex);
    wake_up_interruptible(&outq);
    return count;
}

static int freespace(void) {
    if (wp == rp) {
    	/* reset heads */
    	wp = rp = buffer;
        return BUFSIZE - 1;
    } else if (wp > rp) {
    	return end - wp;
    }

    return ((rp + BUFSIZE - wp) % BUFSIZE) - 1;
}

static int waitspace(struct file *fp)
{
    while (freespace() < LZO_MINIMUM_CHUNK) { /* no enough room */
        DEFINE_WAIT(wait);

        mutex_unlock(&mutex);
        if (fp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        prepare_to_wait(&outq, &wait, TASK_INTERRUPTIBLE);
        if (freespace() < LZO_MINIMUM_CHUNK) {
            schedule();
        }
        finish_wait(&outq, &wait);
        if (signal_pending(current)) {
            return -ERESTARTSYS;
        }
        if (mutex_lock_interruptible(&mutex)) {
            return -ERESTARTSYS;
        }
    }
    return 0;
}

static unsigned int compressor_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;

    mutex_lock(&mutex);
    poll_wait(filp, &inq,  wait);
    poll_wait(filp, &outq, wait);
    if (rp != wp) {
    	/* something to read */
        mask |= POLLIN | POLLRDNORM;
    }
    if (freespace() >= LZO_MINIMUM_CHUNK) {
    	/* has space to write */
        mask |= POLLOUT | POLLWRNORM;
    }
    mutex_unlock(&mutex);
    return mask;
}

static ssize_t
compressor_write(struct file *fp, const char __user *buf, size_t count,
        loff_t *pos) {
    int result;
    char *lzo_in = NULL;
    size_t max_worst, end_wp, rp_wp_1 = 0;
    size_t compressed = 0;
    int err = 0;

    if (mutex_lock_interruptible(&mutex)) {
        return -ERESTARTSYS;
    }

    result = waitspace(fp);
    if (result) {
    	pr_info(LOG "no space\n");
        return result;
    }

    max_worst = lzo1x_worst_backwards(freespace());
    end_wp = lzo1x_worst_backwards(end - wp);
    rp_wp_1 = lzo1x_worst_backwards(rp - wp - 1);

    count = min(count, (size_t)max_worst);
    if (wp >= rp) {
        count = min(count, (size_t)end_wp);
    } else {
        count = min(count, (size_t)rp_wp_1);
    }

    lzo_in = (char *)kzalloc(count, GFP_KERNEL);
    if(lzo_in == NULL) {
        mutex_unlock (&mutex);
        return -ENOMEM;
    }

    err = copy_from_user(lzo_in, __user buf, count);
    if (err) {
    	kfree(lzo_in);
        mutex_unlock (&mutex);
        pr_err(LOG "unable to copy from user, error: %d\n", err);
        return -EFAULT;
    }

    err = lzo1x_1_compress(lzo_in, count, wp + 8, &compressed, lzo_wrkmem);
    kfree(lzo_in);

    if(err != LZO_E_OK) {
    	mutex_unlock (&mutex);
    	pr_err(LOG "compression failed\n");
    	return -EFAULT;
    }

    write32(wp, count); /* write uncompressed size */
    write32(wp + 4, compressed); /* write compressed size */
    wp += 4 + 4 + compressed; /* header + compressed data */
    if (wp > end) { /* circular buffer */
        wp = buffer;
    }

    // pr_info(LOG "Got %d bytes from user, compressed to %d, wp-rp=%d\n", count, compressed, wp-rp);
    // pr_info(LOG "free bytes: %d\n", freespace());

    mutex_unlock(&mutex);
    wake_up_interruptible(&inq);
    return count;
}

/* =============================================== */

module_init(task25_init);
module_exit(task25_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.5");

