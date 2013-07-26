/*
 * task24.h
 *
 *      Author: Maxim Kouprianov
 */

#include <linux/ioctl.h>
#include <stddef.h>

#define DEVNAME "poums.StringPlugin"

/* plugins namespace */
#define PLUGIN_REVERSE 0
#define PLUGIN_TOLOWER 1
#define PLUGIN_TOCAPS 2
#define PLUGIN_SLOWPOKE 63


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

