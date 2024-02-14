#!/usr/bin/python3

import subprocess
import struct


# Define the path to your binary file
efivar_path = '/sys/firmware/efi/efivars/MyVariableName-12345678-1234-1234-1234-56789abcdef0'
bin_path = "map.bin"



def read_binary_file(file_path):

    start_addr = 0
    end_addr = 0
    size = 0
    memmap_str = ""
    # Open the file in binary read mode
    with open(file_path, 'rb') as file:
        while True:
            # Read 8 bytes from the file
            bytes_read = file.read(8)
            # If less than 8 bytes are read, it means we're at the end of the file
            if len(bytes_read) < 8:
                break
            # Unpack the 64-bit integer from the bytes
            integer = struct.unpack('<q', bytes_read)[0]
            if start_addr == 0:
                start_addr = integer
            else:
                end_addr = integer
                size = end_addr - start_addr
                memmap_str += 'memmap=' + hex(size) + '!' + hex(start_addr) + ' '
                start_addr = 0
                end_addr = 0

    return memmap_str

def main():
    command = ['dd', 'if=' + efivar_path, 'bs=1', 'skip=4', '>', bin_path]
    result = subprocess.run(command, capture_output=True, text=False)
    # Call the function to read the file
    file_content = read_binary_file(bin_path)

    
    
    # Print the content of the file
    print(file_content)

if __name__ == '__main__':
    main()

