CFLAGS = -target x86_64-unknown-windows \
	  -ffreestanding \
	  -fshort-wchar \
	  -mno-red-zone \
	  -I/usr/include/efi \
	  -I/usr/include/efi/x86_64
LDFLAGS = -target x86_64-unknown-windows \
	 -nostdlib \
	 -Wl,-entry:efi_main \
	 -Wl,-subsystem:efi_application \
	 -fuse-ld=lld-link

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: memclear.efi

clean:
	rm -f *.o *.efi

memclear.efi: $(OBJS)
	clang $(LDFLAGS) $^ -o $@

%.o : %.c
	clang $(CFLAGS) -c -o $@ $<
