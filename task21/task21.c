/*
 * task21.c
 *
 *      Author: Maxim Kouprianov
 */

#include "task21.h"

/* params */
static unsigned int num = 1; /* number of devices to create */
static unsigned long buffsize = BUFFSIZE;

module_param(num, uint, S_IRUGO);
module_param(buffsize, ulong, S_IRUGO);
MODULE_PARM_DESC(num, "number of devices to create (1-8)(default: 1)");
MODULE_PARM_DESC(buffsize, "size of device's buffer in bytes (default: 4096)");
/* end params */

static dev_t first; /* first device number for driver */
static struct class *poums_class = NULL;/* ptr to device's class object */
static struct poums_device *poums_devices = NULL; /* array to store devices */

/* =============================================== */

static int
poums_open(struct inode *inode, struct file *fp) {
	struct poums_device *dev = NULL;
	int ret = 0;

	unsigned int minor = iminor(inode);
	unsigned int major = imajor(inode);

	if(major != MAJOR(first) || minor < 0 || minor >= num) {
		pr_err(LOG "no such device for major=%d, minor=%d\n", major, minor);
		return -ENODEV;
	}

	dev = &poums_devices[minor];
	fp->private_data = dev;

	if(inode->i_cdev != &dev->cdev) {
		pr_err(LOG "wrong device found, internal error\n");
		return -ENODEV;
	}

	/* lock thread */
	if(mutex_lock_interruptible(&dev->mutex)) {
		return -EINTR;
	}

	/* truncate if needed */
	if(fp->f_flags & O_TRUNC) {
		pr_info(LOG "truncating\n");
		kfree(dev->data);
		dev->data = NULL;
		dev->size = 0;
	}

	if (dev->data == NULL ) {
		/* allocate storage for the first time */
		dev->data = (char *) kzalloc(buffsize, GFP_KERNEL);
		if (dev->data == NULL) {
			pr_err(
					LOG "unable to allocate %zd bytes for the storage (minor=%d)\n",
					buffsize, minor);
			ret = -ENOMEM;
		} else {
			dev->size = 0;
		}
	}

	mutex_unlock(&dev->mutex);
	pr_info(LOG "opening /dev/poums%d: %d\n", minor, ret);
	return ret;
}

static int
poums_close(struct inode *inode, struct file *fp) {
	unsigned int minor = iminor(inode);
	pr_info(LOG "closing /dev/poums%d: 0\n", minor);
	return 0;
}

static ssize_t
poums_read(struct file *fp, char __user *buff, size_t count, loff_t *pos) {
	unsigned int minor = iminor(fp->f_inode);
	struct poums_device *dev = fp->private_data;
	ssize_t ret = 0;

	/* lock thread */
	if(mutex_lock_interruptible(&dev->mutex)) {
		return -EINTR;
	}

	/* boundary check #1 */
	if(*pos >= dev->size) {
		goto out;
	}

	/* count adjustment */
	if(*pos + count >= dev->size) {
		count = dev->size - *pos; /* partial read */
	}

	BUG_ON(dev->data == NULL);

	/* copy data to the user */
	if(copy_to_user(buff, &(dev->data[*pos]), count)) {
		/* something left to read i.e. fail */
		ret = -EFAULT;
		goto out;
	}

	/* advance marker */
	*pos += count;
	ret = count;

	out:
		mutex_unlock(&dev->mutex);
		pr_info(LOG "reading /dev/poums%d, %zd\n", minor, ret);
		return ret;
}

static ssize_t
poums_write(struct file *fp, const char __user *buff, size_t count, loff_t *pos) {
	unsigned int minor = iminor(fp->f_inode);
	struct poums_device *dev = fp->private_data;
	ssize_t ret = -ENOMEM; /* default err */

	/* lock thread */
	if(mutex_lock_interruptible(&dev->mutex)) {
		return -EINTR;
	}

	BUG_ON(dev->data == NULL);

	/* truncate if needed */
	if(fp->f_flags & O_APPEND) {
		pr_info(LOG "appending\n");
		*pos = dev->size;
	}

	if(*pos + count > buffsize) {
		count = buffsize - *pos; /* write up to end */
	}

	if(copy_from_user(&(dev->data[*pos]), buff, count)) {
		ret = -EFAULT;
		goto out;
	}

	/* advance marker */
	*pos += count;
	ret = count;

	/* update size */
	if(dev->size < *pos) {
		dev->size = *pos;
	}

	out:
		mutex_unlock(&dev->mutex);
		pr_info(LOG "writing /dev/poums%d: %zd\n", minor, ret);
		return ret;
}

static loff_t
poums_llseek(struct file *fp, loff_t off, int whence) {
	unsigned int minor = iminor(fp->f_inode);
	struct poums_device *dev = fp->private_data;
	loff_t newpos = 0;

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;
	case SEEK_CUR:
		newpos = fp->f_pos + off;
		break;
	case SEEK_END:
		newpos = dev->size + off;
		break;
	default:
		return -EINVAL;
		break;
	}

	if (newpos < 0 || newpos > buffsize) {
		return -EINVAL;
	}

	fp->f_pos = newpos;
	pr_info(LOG "seek /dev/poums%d: %lld\n", minor, newpos);
	return newpos;
}

/* =============================================== */

static int __init task21_init(void) {
	pr_info(LOG "device init\n");
	int err = 0;

	/* check parameters */
	if(num < 1 || num > NUM_MAX) {
		pr_err(LOG "invalid value of `num` argument: must be 1-%d\n", NUM_MAX);
		return -EINVAL;
	}

	if(buffsize < 1 || buffsize > ULONG_MAX) {
		pr_err(LOG "invalid value of `buffsize` argument: must be 1-%lu\n", ULONG_MAX);
		return -EINVAL;
	}

	pr_info(LOG "buffsize: %lu\n", buffsize);

	 /* allocate region for @num devices */
	err = alloc_chrdev_region(&first/*where to put*/, 0/*baseminor*/,
			num/*count*/, BASENAME/*name*/);
	if (err < 0) {
		pr_err(LOG "unable to allocate %d chrdev regions: %d\n", num, err);
		return err;
	}

	/* create class */
	poums_class = class_create(THIS_MODULE/*owner*/, BASENAME/*name*/);
	if (IS_ERR(poums_class)) {
		pr_err(LOG "unable to create sysfs class\n");
		err = PTR_ERR(poums_class);
		goto out_class;
	}

	/* init and register @num cdevs (expose to kernel) */
	poums_devices = (struct poums_device *) kcalloc(num,
			sizeof(struct poums_device), GFP_KERNEL);

	if (!poums_devices) {
		pr_err(LOG "unable to allocate %d devices: out of free memory\n", num);
		err = -ENOMEM;
		goto out_devcreate;
	}

	unsigned int init_num;

	for (init_num = 0; init_num < num; ++init_num) {
		err = init_poums_device(&poums_devices[init_num], init_num);
		if (err < 0) {
			pr_err(LOG "unable to allocate %d devices, failed at %d\n", num,
					init_num);
			goto out_devinit;
		}
	}

	BUG_ON(init_num != num);/* should be all */
	pr_info(LOG "initialized: %d/%d\n", init_num, num);

	unsigned int created_num;
	dev_t curr;

	/* create @num devices in sysfs (expose to user) */
	for (created_num = 0; created_num < num; ++created_num) {
		curr = MKDEV(MAJOR(first), created_num);
		struct device *dev = device_create(poums_class/*class*/, NULL/*parent*/,
				curr/*devt*/, NULL/*data*/, "poums%d", created_num);

		if (IS_ERR(dev)) {
			/* fail at create */
			pr_err(LOG "unable to allocate %d devices, failed at %d\n", num,
					created_num);
			err = -ENODEV;
			goto out_devcreate;
		}
	}

	BUG_ON(created_num != num); /* should be all */
	pr_info(LOG "created: %d/%d\n", created_num, num);

	pr_info(LOG "driver registered successfully\n");
	return 0;

	out_devcreate: destroy_created_devices(created_num, poums_class);
	out_devinit: deinit_poums_devices(init_num);
	out_class: class_destroy(poums_class);
	out_reg: unregister_chrdev_region(first, num);

	return err;
}

static void __exit task21_exit(void) {
	pr_info(LOG "driver exit\n");
	destroy_created_devices(num, poums_class); /* unexpose from user */
	deinit_poums_devices(num); /* unexpose from kernel & free mem */
	class_destroy(poums_class);
	unregister_chrdev_region(first, num);
}

static void
destroy_created_devices(unsigned int num, struct class *dev_class) {
	dev_t curr;

	for (; num--;) {
		curr = MKDEV(MAJOR(first), num);
		device_destroy(dev_class, curr);
	}
}

static void
deinit_poums_devices(unsigned int num) {
	struct poums_device *dev;

	if (poums_devices) {
		for (; num--;) {
			dev = &poums_devices[num];
			cdev_del(&dev->cdev); /* deinit cdev */
			kfree(dev->data); /* cleanup allocated storage */
		}
		kfree(poums_devices); /* cleanup devices array */
	} else {
		pr_warn(LOG "nothing to deinit. Wat?\n");
	}
}

static int
init_poums_device(struct poums_device *dev, unsigned int minor) {
	BUG_ON(dev == NULL);
	int err = 0;

	dev->data = NULL;
	dev->size = 0;
	cdev_init(&dev->cdev, &poums_fops);
	mutex_init(&dev->mutex);
	dev->cdev.owner = THIS_MODULE;

	err = cdev_add(&dev->cdev, MKDEV(MAJOR(first), minor), 1/*count*/);
	if (err < 0) {
		pr_warn(LOG "failed to add cdev #%d\n", minor);
		return err;
	}

	return 0;
}

module_init(task21_init);
module_exit(task21_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.1");

