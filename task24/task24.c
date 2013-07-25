/*
 * task24.c
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
#include <linux/atomic.h>
#include <linux/delay.h>


/* =============================================== */
#include "task24.h"

#define PLUGINS_MAX 64 /* maximum number of registered plugins */
#define LOG "task24: "

static dev_t first; /* first device number for driver */
static struct class *poums_class = NULL;/* ptr to device's class object */
static struct cdev *device = NULL; /* device */

struct string_plugin **plugins = NULL; /* array to store plugins info */

static int
poums_open(struct inode* inode, struct file* filp);
static int
poums_release(struct inode* inode, struct file* filp);
static long
poums_ioctl_func(struct file* filp, unsigned int cmd, unsigned long arg);

struct file_operations fops = {
	.open = poums_open,
	.unlocked_ioctl = poums_ioctl_func,
	.release = poums_release
};

/* =============================================== */
static int
exec_plugin(struct string_plugin_call_params *params) {
	int id, err = 0;
	struct string_plugin *active;

	if (params == NULL ) {
		pr_err(LOG "params ptr is NULL\n");
		return -EINVAL;
	}

	id = params->id;
	if (id < 0 || id >= PLUGINS_MAX) {
		pr_err(LOG "illegal feature id=%d (range 0-%d)\n",
				id, PLUGINS_MAX - 1);
		return -EINVAL;
	}

	active = plugins[id];
	if(active == NULL) {
		pr_err(LOG "no such plugin to handle feature id=%d\n", id);
		return -EINVAL;
	}

	/* plugin found, lock it */
	if(!try_module_get(plugins[id]->owner)) {
		pr_err(LOG "unable to lock module of %s (id: %d)\n", active->name, id);
		return -EINVAL;
	}

	pr_info(LOG "manager locked %s (id: %d)\n", active->name, id);

	if(params->string == NULL) {
		pr_err(LOG "no suitable input string provided (NULL)"
				"for feature id=%d\n", id);
		err = -EINVAL;
		goto out;
	}

	if(params->buffer == NULL) {
		pr_err(LOG "no suitable output buffer provided (NULL)"
				"for feature id=%d\n", id);
		err = -EINVAL;
		goto out;
	}

	if(params->bufsize < 1) {
		pr_err(LOG "no suitable output buffer provided (illegal size)"
					"for feature id=%d\n", id);
		err = -EINVAL;
		goto out;
	}

	err = plugins[id]->handler(params->string,
			params->buffer, params->bufsize);

	out: /* operations complete, unlock plugin */
		module_put(plugins[id]->owner);
		pr_info(LOG "manager unlocked %s (id: %d)\n", active->name, id);
		return err;
}

static int
check_plugin(struct string_plugin *plugin) {
	if (plugin == NULL ) {
		pr_err(LOG "plugin ptr is NULL\n");
		return -EINVAL;
	}

	if (plugin->id < 0 || plugin->id >= PLUGINS_MAX) {
		pr_err(LOG "plugin %s has illegal id (range 0-%d)\n", plugin->name,
				PLUGINS_MAX - 1);
		return -EINVAL;
	}

	if (plugin->owner == NULL ) {
		pr_err(LOG "plugin %s has no declared owner (NULL)\n", plugin->name);
		return -EINVAL;
	}

	if (plugin->handler == NULL ) {
		pr_err(LOG "plugin %s has no handler function (NULL)\n", plugin->name);
		return -EINVAL;
	}

	return 0;
}

extern int
string_op_plugin_register(struct string_plugin *plugin) {
	if (check_plugin(plugin) > 0) {
		pr_err(LOG "unable to register plugin (see above)\n");
	}

	if (plugins[plugin->id] != NULL) {
		pr_err(LOG "such plugin is already registered: %s\n", plugin->name);
		return -EINVAL;
	}

	// TODO: check name collision (unnecessary imho)

	plugins[plugin->id] = plugin;
	pr_info(LOG "registered plugin: %s (id: %d)\n", plugin->name, plugin->id);
	return 0;
}

extern int
string_op_plugin_unregister(struct string_plugin *plugin) {
	int id;
	char *name;
	struct string_plugin *pref;

	if (check_plugin(plugin) > 0) {
		pr_err(LOG "unable to unregister plugin (see above)\n");
	}

	id = plugin->id;
	pref = plugins[id];
	if (pref == NULL ) {
		pr_err(LOG "such plugin is not registered: %s (id: %d)\n",
				plugin->name, id);
		return -EINVAL;
	}

	if (pref->handler != plugin->handler) {
		pr_err(
				LOG "plugin tried to unregister foreign instance of %s (id: %d)\n",
				name, id);
		return -EINVAL;
	}

	pr_info(LOG "unregistered plugin: %s (id: %d)\n", plugin->name, id);
	plugins[id] = NULL;
	return 0;
}

static void
deinit_device(struct cdev *dev) {
	cdev_del(dev);
	kfree(dev);
}

static int
init_device(struct cdev *dev) {
	int err = 0;

	cdev_init(dev, &fops);
	dev->owner = THIS_MODULE;

	err = cdev_add(dev, MKDEV(MAJOR(first), 0), 1);
	if (err < 0) {
		pr_warn(LOG "failed to add cdev for device\n");
		return err;
	}

	return 0;
}

static int task24_create_device(void) {
	int err = 0;
	struct device *dev;

	err = alloc_chrdev_region(&first, 0, 1, DEVNAME);
	if (err < 0) {
		pr_err(LOG "unable to allocate chrdev region: %d\n", err);
		return err;
	}

	/* create class */
	poums_class = class_create(THIS_MODULE, DEVNAME);
	if (IS_ERR(poums_class)) {
		pr_err(LOG "unable to create sysfs class\n");
		err = PTR_ERR(poums_class);
		goto out_reg;
	}

	/* init and register @num cdevs (expose to kernel) */
	device = (struct cdev *) kzalloc(sizeof(struct cdev), GFP_KERNEL);

	if (!device) {
		pr_err(LOG "unable to allocate device: out of memory\n");
		err = -ENOMEM;
		goto out_class;
	}

	err = init_device(device);
	if (err < 0) {
		pr_err(LOG "unable to allocate device\n");
		goto out_devinit;
	}

	dev = device_create(poums_class, NULL,
			MKDEV(MAJOR(first), 0), NULL, DEVNAME);

	if (IS_ERR(dev)) {
		pr_err(LOG "unable to allocate device\n");
		err = -ENODEV;
		goto out_devcreate;
	}

	return 0;

	out_devcreate: device_destroy(poums_class, MKDEV(MAJOR(first), 0));
	out_devinit: deinit_device(device);
	out_class: class_destroy(poums_class);
	out_reg: unregister_chrdev_region(first, 1);
	return err;
}

static void task24_destroy_device(void) {
	device_destroy(poums_class, MKDEV(MAJOR(first), 0));
	deinit_device(device);
	class_destroy(poums_class);
	unregister_chrdev_region(first, 1);
}

static int __init task24_init(void) {
	int err = 0;
	pr_info(LOG "plugin manager started\n");

	plugins = (struct string_plugin **) kcalloc(PLUGINS_MAX,
			sizeof(struct string_plugin *), GFP_KERNEL);

	if (plugins == NULL ) {
		pr_err(LOG "unable to allocate storage for handlers\n");
		return -ENOMEM;
	}

	err = task24_create_device();
	if(err) {
		pr_err(LOG "unable to create plugin's interface in dev\n");
		err = -ENODEV;
		goto out;
	}

	return 0;
	out:
		kfree(plugins);
		return err;
}

static void __exit task24_exit(void) {
	task24_destroy_device();
	kfree(plugins);
	pr_info(LOG "plugin manager exit\n");
}

/* =============================================== */

static int
poums_open(struct inode* inode, struct file* filp) {
    return 0;
}

static int
poums_release(struct inode* inode, struct file* filp) {
    return 0;
}

static long
poums_ioctl_func(struct file* filp, unsigned int cmd, unsigned long arg) {
    int err = 0;
    struct string_plugin_call_params *from =
    		(struct string_plugin_call_params __user *)arg;

    struct string_plugin_call_params *params =
    		(struct string_plugin_call_params *)
    		kzalloc(sizeof(struct string_plugin_call_params), GFP_KERNEL);

    if(params == NULL) {
    	pr_err(LOG "unable to allocate space to store data from userspace\n");
    	return -ENOMEM;
    }

    copy_from_user(params, from,
    		sizeof(struct string_plugin_call_params));

    switch (cmd) {
        case IOCTL_HANDLE_STRING:
        	err = exec_plugin(params);
            break;
    }

    if(!err) {
        copy_to_user(from, params,
        		sizeof(struct string_plugin_call_params));
    }

    return err;
}

/* =============================================== */

module_init(task24_init);
module_exit(task24_exit);

EXPORT_SYMBOL(string_op_plugin_register);
EXPORT_SYMBOL(string_op_plugin_unregister);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.4");

