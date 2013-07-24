/*
 * task24.h
 *
 *      Author: Maxim Kouprianov
 */

struct string_plugin {
	int id;
	const char *name;
	int (*handler)(const char *in, char *out, size_t out_size);
};

struct string_plugin_call_params {
	int id;
	char *string;
	char *buffer;
	int bufsize;
};

extern int
string_op_plugin_register(struct string_plugin *plugin);

extern int
string_op_plugin_unregister(struct string_plugin *plugin);

