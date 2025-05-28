# Pree Simphliphan
# U01702082
# I used sample from class folder I modify kernel path to my own workspace
ifneq ($(KERNELRELEASE),)
	obj-m := mytimer.o
else
	KERNELDIR := $(WORKSPACE)/stock-linux-4.19.82-ti-rt-r33
	PWD := $(shell pwd)
	ARCH := arm
	CROSS := arm-linux-gnueabihf-

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) ARCH=$(ARCH) clean

endif
