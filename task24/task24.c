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

#include "task24.h"

#define PLUGINS_MAX 64 /* maximum number of loaded plugins */
#define BUFFSIZE 4096 /* default buffer size to store char-data */
#define LOG "task24: "



/* =============================================== */

static int __init task24_init(void) {
	pr_info(LOG "Plugin manager started\n");
	int err = 0;

	return err;
}

static void __exit task24_exit(void) {
	pr_info(LOG "Plugin manager exit\n");
}

extern int string_op_plugin_register(char *plugin_name, int(*handle)(int *i, int arg)) {
	pr_info(LOG "Registered plugin: %s\n", plugin_name);

	if(handle == NULL) {
		pr_err(LOG "Unable to register plugin %s: NULL handler\n", plugin_name);
		return -EINVAL;
	}

	int i = 5;
	int arg = 10;
	pr_info(LOG "Unaffected i: %d\n", i);
	handle(&i, arg);
	pr_info(LOG "Affected i: %d\n", i);
}

static int string_op_plugin_unregister();

module_init(task24_init);
module_exit(task24_exit);

EXPORT_SYMBOL(string_op_plugin_register);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Implementation module for the Task 2.4");

