/*
 * task24_plugin_reverse.c
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

#define PLUGIN_NAME "increment_plugin"
#define LOG "task24_increment_plugin: "

static int handle(const char *in, char *out, size_t out_size) {
	return 0;
}


static int __init plugin_init(void) {
	pr_info(LOG "%s init\n", PLUGIN_NAME);
	int err = 0;
	pr_info(LOG "trying to register in manager\n");

	struct string_plugin plugin = {
			.id = 12,
			.name = PLUGIN_NAME,
			.handler = &handle
	};

	err = string_op_plugin_register(&plugin);

	if(err) {
		pr_info(LOG "Error: %d\n", err);
	}

	return err;
}

static void __exit plugin_exit(void) {
	pr_info(LOG "%s exit\n", PLUGIN_NAME);
}

module_init(plugin_init);
module_exit(plugin_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Sample 'increment' plugin for Task 2.4");

