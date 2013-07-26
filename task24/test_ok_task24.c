/*
 *  Task 2.4
 *  Tests in C
 *  Should work fine
 */

#include "task24.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

#define LOG "test_task24: "
#define MAXLEN 256

static int
ioctl_handle_string(int fd,
		struct string_plugin_call_params *params) {
	return ioctl(fd, IOCTL_HANDLE_STRING, params);
}

int main(int argc, char **argv) {
	int fd, i, total, err = 0;
	char buffer[MAXLEN];

	if(argc != 2) {
		printf("usage:\n"
				"./test_ok_task24 \"Hello world\"\n");
		return 0;
	}

	struct string_plugin_call_params
	reverse_params = {
			.id = PLUGIN_REVERSE,
			.string = argv[1],
			.buffer = buffer,
			.bufsize = MAXLEN
	};

	struct string_plugin_call_params
	tolower_params = {
			.id = PLUGIN_TOLOWER,
			.string = argv[1],
			.buffer = buffer,
			.bufsize = MAXLEN
	};

	struct string_plugin_call_params
	tocaps_params = {
			.id = PLUGIN_TOCAPS,
			.string = argv[1],
			.buffer = buffer,
			.bufsize = MAXLEN
	};

	fd = open("/dev/" DEVNAME, 0);
	if (fd < 0) {
		printf(LOG "can't open plugin manager"
				"device file: %s\n", "/dev/" DEVNAME);
		return -1;
	}

	printf(LOG "reverse test: %s\n", reverse_params.string);
	err = ioctl_handle_string(fd, &reverse_params);

	if (err < 0) {
		printf(LOG "ioctl failed: %d\n", err);
	} else {
		printf(LOG "result: %s\n", reverse_params.buffer);
	}

	printf(LOG "tolower test: %s\n", tolower_params.string);
	err = ioctl_handle_string(fd, &tolower_params);

	if (err < 0) {
		printf(LOG "ioctl failed: %d\n", err);
	} else {
		printf(LOG "result: %s\n", tolower_params.buffer);
	}

	printf(LOG "tocaps test: %s\n", tocaps_params.string);
	err = ioctl_handle_string(fd, &tocaps_params);

	if (err < 0) {
		printf(LOG "ioctl failed: %d\n", err);
	} else {
		printf(LOG "result: %s\n", tocaps_params.buffer);
	}

	close(fd);
	return err;
}
