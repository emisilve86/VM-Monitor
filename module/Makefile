obj-m = vm-monitor.o

vm-monitor-objs := vmm-device.o vmm-walker.o vmm-main.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean