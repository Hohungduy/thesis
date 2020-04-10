SHELL = /bin/bash

topdir := $(shell cd $(src)/.. && pwd)

TARGET_MODULE:=mylibxdma

EXTRA_CFLAGS := -I$(topdir)/include
EXTRA_CFLAGS += -D__LIBXDMA_MOD__

ifneq ($(KERNELRELEASE),)
	obj-m := $(TARGET_MODULE).o testmodule.o
	$(TARGET_MODULE)-objs := xdma_thread.o libxdma.o
else
	BUILDSYSTEM_DIR:=/lib/modules/$(shell uname -r)/build
	PWD:=$(shell pwd)
all :
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean

install: all
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules_install

endif
