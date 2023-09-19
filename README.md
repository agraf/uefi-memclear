# uefi-memclear
UEFI memory clearing application. Effectively does memset(0) on all free memory, then exits.

## Prerequisites

To build uefi-memclear, you need to install

  * make
  * clang
  * lld
  * gnu-efi
  * qemu-system-x86\_64 (optional)
  * ovmf (optional)

On Ubuntu, you can just run this:
```sh
$ apt install make clang lld gnu-efi qemu-system-x86_64 ovmf
```

## Build

Building is simple. You call make and are good to go!

```sh
$ make
```

## Run

You can run the memory scrubber in a test environment with QEMU to quickly check whether changes work successfully;:

```
$ ./run.sh
BdsDxe: failed to load Boot0001 "UEFI QEMU DVD-ROM QM00003 " from PciRoot(0x0)/Pci(0x1,0x1)/Ata(Secondary,Master,0x0): Not Found

>>Start PXE over IPv4.
  Station IP address is 10.0.2.15

  Server IP address is 10.0.2.2
  NBP filename is memclear.efi
  NBP filesize is 2560 Bytes
 Downloading NBP file...

  NBP file downloaded successfully.
BdsDxe: loading Boot0002 "UEFI PXEv4 (MAC:525400123456)" from PciRoot(0x0)/Pci(0x3,0x0)/MAC(525400123456,0x1)/IPv4(0.0.0.0,0x0,DHCP,0.0.0.0,0.0.0.0,0.0.0.0)
BdsDxe: starting Boot0002 "UEFI PXEv4 (MAC:525400123456)" from PciRoot(0x0)/Pci(0x3,0x0)/MAC(525400123456,0x1)/IPv4(0.0.0.0,0x0,DHCP,0.0.0.0,0.0.0.0,0.0.0.0)
Successfully cleared memory
```

On a real target system, you need to add the memclear.efi binary to the UEFI boot order:

```sh
$ sudo cp memclear.efi /boot/efi/EFI/
$ sudo efibootmgr -c -L "Clear Memory" -l '\EFI\memclear.efi' -d /dev/nvme0n1 -p 15
```

This will prepend a new "Clear Memory" entry to the boot order. That way, the next time the system boots, it will clear all memory before it boots Linux. Please make sure that you adjust the `-d` and `-p` parameters according to the EFI System Partition of your environment.

```sh

```
