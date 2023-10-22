CC="/home/anton/opt/cross/bin/i686-elf-gcc"
CFLAGS = -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough
BINS=cat
all: $(BINS)

cat.o: cat.c
	$(CC) $(CFLAGS) -L../libc/ -lc -c cat.c -I../libc/

cat: cat.o
	$(CC) -shared -o cat -ffreestanding -nostdlib $(CFLAGS) cat.o -L../libc/ -lc -lgcc #-L../libc/c.a 
