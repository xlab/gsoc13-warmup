# Legitimate kernel makefile

ifneq ($(KERNELRELEASE),)
	ccflags-y := -std=gnu99 -Wno-declaration-after-statement -Wno-unused-label -Wno-maybe-uninitialized
	obj-m += task21.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: test
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test:
	gcc test_task21.c -o test_task21

endif
