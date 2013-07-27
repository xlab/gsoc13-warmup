# Legitimate kernel makefile

ifneq ($(KERNELRELEASE),)
	obj-m += task25.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:# test
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

#test:
#	gcc test_task21.c -o test_task21

endif
