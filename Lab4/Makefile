# Adjust KERNEL_BUILD_DIR to the path where your kernel "build" artifacts are,
# or where you have your kernel sources configured. In the labs, you might have
# a symlink to "stock-linux-4.19.82-ti-rt-r33" or something similar.

KERNEL_BUILD_DIR := $(WORKSPACE)/stock-linux-4.19.82-ti-rt-r33

# CROSS_COMPILE is typically something like "arm-linux-gnueabihf-" or "arm-linux-gnueabi-"
# or "arm-none-linux-gnueabihf-", depending on your environment. If you have set this
# in your environment, you might not need to specify it here. For labs, you might have
# something like:
CROSS_COMPILE ?= arm-linux-gnueabihf-

obj-m := mytraffic.o

all:
	$(MAKE) -C $(KERNEL_BUILD_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNEL_BUILD_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) clean

#KERNEL_BUILD_DIR := $(WORKSPACE)/stock-linux-4.19.82-ti-rt-r33\nCROSS_COMPILE ?= arm-linux-gnueabihf-\nobj-m := mytraffic.o\nall:\n\t$(MAKE) -C $(KERNEL_BUILD_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules\nclean:\n\t$(MAKE) -C $(KERNEL_BUILD_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) clean