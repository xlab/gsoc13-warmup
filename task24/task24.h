/*
 * task24.h
 *
 *      Author: Maxim Kouprianov
 */

#include <linux/ioctl.h>
#include <stddef.h>

#define DEVNAME "poums.StringPlugin"

struct string_plugin {
	struct module *owner;
	unsigned int id;
	const char *name;
	int (*handler)(const char *in, char *out, size_t out_size);
};

struct string_plugin_call_params {
	unsigned int id;
	const char *string;
	char *buffer;
	unsigned int bufsize;
};

extern int
string_op_plugin_register(struct string_plugin *plugin);

extern int
string_op_plugin_unregister(struct string_plugin *plugin);

#define IOC_MAGIC ('h')
#define IOCTL_HANDLE_STRING _IOWR(IOC_MAGIC, 0x01, \
		struct string_plugin_call_params *)

