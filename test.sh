#!/bin/bash
#
# Test script to run memclear.efi on boot
#

qemu-system-x86_64 -nographic -m 5G -bios /usr/share/ovmf/OVMF.fd -netdev user,id=n,tftp=.,bootfile=memclear.efi -device virtio-net-pci,netdev=n -net none
