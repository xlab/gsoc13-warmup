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
#include <linux/delay.h>

#include "task24.h"

#define PLUGIN_NAME "plugin_reverse"
#define LOG "task24_plugin_reverse: "

static int handle(const char *in, char *out, size_t out_size) {
	int i = 0;
	char c;

	pr_info(LOG "handling string: %s\n", in);
	msleep_interruptible(60000);

	while(i < out_size && (c = in[i++]) != '\0') {
		out[(out_size - 1) - i] = c;
	}

	return 0;
}

static struct string_plugin plugin = {
		.owner = THIS_MODULE,
		.id = 0x01,
		.name = PLUGIN_NAME,
		.handler = &handle
};

static int __init plugin_init(void) {
	int err = 0;

	pr_info(LOG "plugin init\n");
	pr_info(LOG "trying to register in manager\n");

	err = string_op_plugin_register(&plugin);

	if(err) {
		pr_info(LOG "Error: %d\n", err);
	}

	return err;
}

static void __exit plugin_exit(void) {
	string_op_plugin_unregister(&plugin);
	pr_info(LOG "plugin exit\n");
}

module_init(plugin_init);
module_exit(plugin_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maxim Kouprianov");
MODULE_DESCRIPTION("Sample 'increment' plugin for Task 2.4");

