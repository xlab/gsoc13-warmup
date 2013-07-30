#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <lzo/lzo1x.h>

#define LOG "decompress_lzo: "
#define CHUNK  (64 * 1024)
#define UNCOMPRESSED_CHUNK  (5 * 1024 * 1024)

/*
 *  Task 2.5
 *  Helper - decompress LZO-compressed
 *  data chunks.
 *  ================
 *  Chunk structure:
 *   	+ uncompressed_len - 4 bytes
 *  	+ compressed_len - 4 bytes
 *  	+ compressed data - `compressed_len` bytes
 */

static unsigned int read32(unsigned char *in) {
	unsigned char b[4];
	unsigned int n;

	memcpy(b, in, 4);
	n = (unsigned int) b[3] << 0;
	n |= (unsigned int) b[2] << 8;
	n |= (unsigned int) b[1] << 16;
	n |= (unsigned int) b[0] << 24;
	return n;
}

int main(int argc, char **argv) {
	FILE *fpin, *fpout;
	const char *fin;
	int err = 0;
	size_t len, in_len, out_len, new_len = 0;
	lzo_bytep in;
	lzo_bytep out;
	lzo_voidp wrkmem;

	if (argc != 2) {
		fprintf(stderr, "Decompress the given file to stdout.\n"
				"usage:\n"
				"./decompress_lzo <compressed_file>\n");
		return 0;
	} else {
		fin = argv[1];
	}

	fpin = fopen(fin, "r");
	if ((err = ferror(fpin)) < 0) {
		fprintf(stderr, LOG "error opening input file %s: %d\n", fin, err);
		return -1;
	}

	if (lzo_init() != LZO_E_OK) {
		fprintf(stderr, LOG "error initializing LZO\n");
		return -1;
	}

	in = (lzo_bytep) malloc(CHUNK);
	out = (lzo_bytep) malloc(UNCOMPRESSED_CHUNK);
	wrkmem = (lzo_voidp) malloc(LZO1X_1_MEM_COMPRESS );
	if (in == NULL || out == NULL || wrkmem == NULL ) {
		printf("out of memory\n");
		return -1;
	}

	for (;;) {
		len = fread(in, 1, 4, fpin);

		if (len == 0) {
			break; /* EOF */
		} else if (len != 4) {
			err = -1;
			goto error;
		}

		in_len = read32(in); /* uncompressed length header */
		// fprintf(stderr, "%d uncompressed => ", in_len);
		len = fread(in, 1, 4, fpin);
		if (len != 4) {
			err = -2;
			goto error;
		}

		out_len = read32(in); /* compressed length header */
		// fprintf(stderr, "%d compressed\n", out_len);
		if (out_len < 1 || out_len > CHUNK) {
			err = -3;
			goto error;
		}

		len = fread(in, 1, out_len, fpin);
		if (len != out_len) {
			err = -4;
			goto error;
		}

		/* ready to decompress chunk */
		new_len = in_len;
		err = lzo1x_decompress_safe(in, out_len, out, &new_len, wrkmem);

		if (err != LZO_E_OK) {
			fprintf(stderr, LOG "problems while decompressing %s: %d\n", fin,
					err);
			goto out;
		} else {
			fwrite(out, 1, new_len, stdout);
		}
	}
	fflush(stdout);
	goto out;

	error: fprintf(stderr, LOG "corrupted data: %d\n", err);
	out: fclose(fpin);
	free(in);
	free(out);
	free(wrkmem);
	return err;
}

