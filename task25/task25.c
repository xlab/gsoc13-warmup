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

#define BUFSIZE 64 /* bufsize, KB */
#define LOG "task25: "

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
    .open = compressor_open,
    .release = compressor_release,
    .read = compressor_read,
    .write = compressor_write,
    .poll = compressor_poll
};

/* =============================================== */
static int __init task25_init(void) {
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
        return -ENODEV;
    } else if(dir < 0) {
        pr_err(LOG "there is no support for debugfs in kernel\n");
        return -ENODEV;
    }

    file = debugfs_create_file(COMPRESSOR_FILE, 0644,
                           dir, NULL, &compressor_fops);

    if(file == NULL || file < 0) {
        pr_err(LOG "unable to create compressor's file in debugfs\n");
        debugfs_remove(dir);
        return -ENODEV;
    }

    pr_info(LOG "compressor started\n");
    return 0;
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
    if (mutex_lock_interruptible(&mutex)) {
        return -ERESTARTSYS;
    }

    if (!buffer) {
        buffer = kmalloc(BUFSIZE, GFP_KERNEL);
        if (!buffer) {
            mutex_unlock(&mutex);
            pr_err("no memory for buffer\n");
            return -ENOMEM;
        }

        rp = wp = buffer;
    }

    end = buffer + BUFSIZE;

    if (filp->f_mode & FMODE_READ) {
        nreaders++;
    }
    if (filp->f_mode & FMODE_WRITE) {
        nwriters++;
    }
    mutex_unlock(&mutex);

    return nonseekable_open(inode, filp);
}

static int
compressor_release(struct inode* inode, struct file* filp) {
    mutex_lock(&mutex);
    if (filp->f_mode & FMODE_READ) {
        nreaders--;
    }
    if (filp->f_mode & FMODE_WRITE) {
        nwriters--;
    }
    mutex_unlock(&mutex);
    return 0;
}

static ssize_t
compressor_read(struct file *fp, char __user *buf, size_t count,
        loff_t *pos) {
    if (mutex_lock_interruptible(&mutex)) {
        return -ERESTARTSYS;
    }

    while(rp == wp) { /* no data */
    	pr_info(LOG "no data on read\n");
        mutex_unlock(&mutex);
        if(fp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        pr_info(LOG "waiting for data (blocking)\n");
        if(wait_event_interruptible(inq, rp != wp)) {
            return -ERESTARTSYS;
        }
        if(mutex_lock_interruptible(&mutex)) {
            return -ERESTARTSYS;
        }
    }

    pr_info(LOG "data arrived\n");
    if(wp > rp) {
        count = min(count, (size_t)(wp - rp));
    } else {
        count = min(count, (size_t)(end - rp));
    }

    if(copy_to_user(buf, rp, count)) {
        mutex_unlock(&mutex);
        return -EFAULT;
    }

    rp += count;
    if(rp == end) {
        rp = buffer;
    }

    pr_info(LOG "copied to user %d bytes, wp-rp=%d\n", count, wp-rp);

    mutex_unlock(&mutex);
    wake_up_interruptible(&outq);
    return count;
}

static int freespace(void) {
    if (rp == wp) {
        return BUFSIZE - 1;
    }
    return ((rp + BUFSIZE - wp) % BUFSIZE) - 1;
}

static int waitspace(struct file *fp)
{
    while (freespace() == 0) { /* full */
        DEFINE_WAIT(wait);

        mutex_unlock(&mutex);
        if (fp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        prepare_to_wait(&outq, &wait, TASK_INTERRUPTIBLE);
        if (freespace() == 0) {
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
        mask |= POLLIN | POLLRDNORM;    /* something to read */
    }
    if (freespace()) {
        mask |= POLLOUT | POLLWRNORM;   /* has space to write */
    }
    mutex_unlock(&mutex);
    return mask;
}

static ssize_t
compressor_write(struct file *fp, const char __user *buf, size_t count,
        loff_t *pos) {
    int result;
    char *lzobuf;
    size_t compressed;
    int err;


    if (mutex_lock_interruptible(&mutex))
        return -ERESTARTSYS;

    result = waitspace(fp);
    if (result) {
    	pr_info(LOG "no space\n");
        return result;
    }

    count = min(count, (size_t)freespace());
    if (wp >= rp) {
        count = min(count, (size_t)(end - wp));
    }
    else {
        count = min(count, (size_t)(rp - wp - 1));
    }

    lzobuf = (char *)kzalloc(count, GFP_KERNEL);
    if(lzobuf == NULL) {
    	pr_err(LOG "no memory for temporary lz buffer (size=%d)\n", count);
    	mutex_unlock (&mutex);
    	return -ENOMEM;
    }

    if (copy_from_user(lzobuf, buf, count)) {
    	kzfree(lzobuf);
        mutex_unlock (&mutex);
        return -EFAULT;
    }

    err = lzo1x_1_compress(lzobuf, count, wp, &compressed, lzo_wrkmem);
    kzfree(lzobuf);

    if(err < 0) {
    	pr_err(LOG "error compressing input: %d\n", err);
        mutex_unlock (&mutex);
        return -EFAULT;
    }

    wp += compressed;
    if (wp == end) {
        wp = buffer;
    }

    pr_info(LOG "copied from user %d bytes, compressed to %d, wp-rp=%d\n", count, compressed, wp-rp);
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

