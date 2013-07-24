# Legitimate kernel makefile

ifneq ($(KERNELRELEASE),)
	obj-m += task24.o
	obj-m += task24_plugin_reverse.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: test
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test:
	gcc test_task24.c -o test_task24

endif
