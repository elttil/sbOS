CC="/home/anton/opt/cross/bin/i686-elf-gcc"
CFLAGS = -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough
BINS=init
all: $(BINS)

init.o: init.c
	$(CC) $(CFLAGS) -I../libc/ -L../libc/ -lc -c init.c

init: init.o
	$(CC) -shared -o init -ffreestanding -nostdlib $(CFLAGS) init.o -L../libc/ -lc -lgcc #-L../libc/c.a 
