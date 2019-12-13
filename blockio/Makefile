#!/bin/bash

obj-m += blockio_driver.o

KDIR := /home/topeet/4412-tree/itop4412_kernel_4_14_2_bsp/linux-4.14.2_iTop-4412_scp

PWD ?= $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	rm -rf *.o *.mod.c *.o *.order *.ko *.mod.o *.symvers
 
