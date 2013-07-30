# Legitimate kernel makefile

ifneq ($(KERNELRELEASE),)
	obj-m += task25.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: test
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test:
	gcc decompress_lzo.c -o decompress_lzo -llzo2

endif
