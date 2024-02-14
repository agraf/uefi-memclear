obj-m += readmem.o

# Path to the Linux kernel source
# Adjust this to the correct path of your kernel source
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target to build the module
default:
        $(MAKE) -C $(KDIR) M=$(PWD) modules

# Target for cleaning up build artifacts
clean:
        $(MAKE) -C $(KDIR) M=$(PWD) clean
