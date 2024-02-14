#!/bin/bash

#
# Run this scrip in the target system
#

# Prompt the user for confirming secure boot is enabled
echo -n "Is Secure Boot enabled? (Y/n): "
read answer

# Check the user's answer
if [[ "$answer" = "n" ]] || [[ "$answer" = "N" ]]; then
    echo "Abort installing the memory cleaner"
		exit -1
fi


#
# Install required packages and tools
#
sudo apt update
sudo apt install make clang lld gnu-efi qemu-system ovmf

if [ $? -eq 0 ]; then
  echo "Installed required programs."
else
  echo "Failed to install required programs."
  exit -1
fi


#
# Now build the memory cleaner
#

make

if [ $? -eq 0 ]; then
  echo "Built the memory cleaner."
else
  echo "Failed to build the memory cleaner."
  exit -1
fi

#
# Install the memory cleaner by adding the memclear.efi binary to the
# UEFI boot order
#

sudo cp memclear.efi /boot/efi/EFI/

if [ $? -ne 0 ]; then
  echo "Failed to copy the cleaner to `/boot/efi/EFI/`."
  exit -1
fi

esp_device=$(findmnt -n -o SOURCE /boot/efi)
echo "EFI System Partition device: $esp_device"

partition_number=$(echo $esp_device | grep -o '[0-9]*$')
echo "Partition number: $partition_number"

# Finally create a boot entry for the memory cleaner
sudo efibootmgr -c -L "MemoryCleaner" -l '\EFI\memclear.efi' -d $esp_device -p $partition_number

if [ $? -eq 0 ]; then
  echo "Created the boot entry for the cleaner."
else
  echo "Failed to create the boot entry."
 54   echo "Created the boot entry for the cleaner."  exit -1
fi
