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
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/slab.h>

/* =============================================== */
#include "task24.h"

#define PLUGINS_MAX 64 /* maximum number of registered plugins */
#define LOG "task24: "

struct string_plugin **plugins;
atomic_t **counters;
/* =============================================== */

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

	counters[plugin->id] = &((atomic_t) { (0) });
	plugins[plugin->id] = plugin;
	pr_info(LOG "registered plugin: %s (id: %d)\n", plugin->name, plugin->id);
	return 0;
}

extern int
string_op_plugin_unregister(struct string_plugin *plugin) {
	if (check_plugin(plugin) > 0) {
		pr_err(LOG "unable to unregister plugin (see above)\n");
	}

	int id = plugin->id;
	struct string_plugin *pref = plugins[id];

	if (pref == NULL ) {
		pr_err(LOG "such plugin is not registered: %s (id: %d)\n", plugin->name,
				id);
		return -EINVAL;
	}

	if (pref->handler != plugin->handler) {
		pr_err(
				LOG "plugin tried to unregister foreign instance of %s (id: %d)\n",
				plugin->name, id);
		return -EINVAL;
	}

	while(atomic_read(counters[id]) > 0) {
		pr_info("Waiting until %s (id: %d) finish its work", plugin->name, id);
		ssleep(1);
	}

	pr_info(LOG "unregistered plugin: %s (id: %d)\n", plugin->name, id);
	kzfree(counters[id]);
	kzfree(plugins[id]);
	return 0;
}

static int __init task24_init(void) {
	pr_info(LOG "plugin manager started\n");
	int err = 0;

	plugins = (struct string_plugin **) kcalloc(PLUGINS_MAX,
			sizeof(struct string_plugin *), GFP_KERNEL);

	if (plugins == NULL ) {
		pr_err(LOG "unable to allocate storage for handlers\n");
		return -ENOMEM;
	}

	counters = (atomic_t **) kcalloc(PLUGINS_MAX,
								sizeof(atomic_t *), GFP_KERNEL);

	if (counters == NULL ) {
		pr_err(LOG "unable to allocate storage for counters\n");
		err = -ENOMEM;
		goto out;
	}

	return 0;
	out:
		kfree(plugins);
		return err;
}

static void __exit task24_exit(void) {
	kfree(plugins);
	kfree(counters);
	pr_info(LOG "plugin manager exit\n");
}

module_init(task24_init);
module_exit(task24_exit);

EXPORT_SYMBOL(string_op_plugin_register);
EXPORT_SYMBOL(string_op_plugin_unregister);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.4");

