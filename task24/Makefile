# Legitimate kernel makefile

ifneq ($(KERNELRELEASE),)
	obj-m += task24.o
	obj-m += task24_plugin_reverse.o
	obj-m += task24_plugin_tolower.o
	obj-m += task24_plugin_tocaps.o
	obj-m += task24_plugin_slowpoke.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: test
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

test:
	gcc test_ok_task24.c -o test_ok_task24
	gcc test_slow_task24.c -o test_slow_task24

endif
