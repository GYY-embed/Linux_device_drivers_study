KERNELDIR := /home/gyy/linux/linux-imx-4.1.15
CURRENT_PATH := $(shell pwd)

obj-m := ap3216c_driver.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean