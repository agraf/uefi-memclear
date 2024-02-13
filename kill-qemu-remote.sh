#!/bin/bash

# VM SSH details
VM_USER="azureuser"
VM_HOST="20.163.175.211"

# Command to find and kill the qemu-system-x86_64 process
CMD="pgrep qemu-system- | xargs -r kill && sleep 1 && pgrep qemu-system- | xargs -r kill -9"

# Execute the command on the VM
ssh ${VM_USER}@${VM_HOST} "${CMD}"

echo "Attempted to kill qemu-system-x86_64 processes on ${VM_HOST}"
