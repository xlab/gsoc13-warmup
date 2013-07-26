/*
 * task24_plugin_tolower.c
 *
 *      Author: Maxim Kouprianov
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>

#include "task24.h"

#define PLUGIN_NAME "plugin_tolower"
#define LOG "task24_plugin_tolower: "

static int handle(const char *in, char *out, size_t out_size) {
	int limit, i = 0;

	pr_info(LOG "handling string: %s\n", in);
	limit = strlen(in) < out_size - 1 ? strlen(in) : out_size - 1;
	while(i < limit) {
		char c = in[i];

		if(c >= 'A' && c <= 'Z') {
			out[i++] = c + 32;
		} else {
			out[i++] = c;
		}

		if(i == limit) {
			out[i] = '\0';
		}
	}

	return 0;
}

static struct string_plugin plugin = {
		.owner = THIS_MODULE,
		.id = PLUGIN_TOLOWER,
		.name = PLUGIN_NAME,
		.handler = &handle
};

static int __init plugin_init(void) {
	int err = 0;

	pr_info(LOG "plugin init\n");
	pr_info(LOG "trying to register in manager\n");

	err = string_op_plugin_register(&plugin);

	if(err) {
		pr_info(LOG "register error: %d\n", err);
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
MODULE_DESCRIPTION("Sample " PLUGIN_NAME " for Task 2.4");
