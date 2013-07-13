#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#define DEVICE "/dev/poums0"

int main() {
	unsigned int fd;
	const char *buf = "Hello!";

	if ((fd = open(DEVICE, O_WRONLY | O_APPEND)) < 0) {
		printf("Error: unable to open device %s\n", DEVICE);
		return -1;
	}

	/* try write */
	int err;
	if ((err = write(fd, buf, strlen(buf))) < 0) {
		close(fd);
		printf("Error: unable to write %d bytes into file #%d: %d\n",
				strlen(buf), fd, err);
		return -1;
	}

	close(fd);
	return 0;
}

/* alternate
 FILE *fp;
 const char *buf = "Hello!";

 fp = fopen(DEVICE, "a");
 fwrite(buf, sizeof(char), strlen(buf), fp);
 fclose(fp);
 */
