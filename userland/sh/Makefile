CC="i686-sb-gcc"
CFLAGS = -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -fsanitize=shift,signed-integer-overflow,bounds
BINS=sh
all: $(BINS)

sh.o: sh.c
	$(CC) $(CFLAGS) -L../libc/ -lc -c sh.c -I../libc/
#	$(CC) $(CFLAGS) ../libc/libc.o ../libc/crt0.o sh.c -I../libc/

clean:
	rm sh sh.o
sh: sh.o
	$(CC) -shared -o sh -ffreestanding -nostdlib $(CFLAGS) sh.o -L../libc/ -lc -lgcc #-L../libc/c.a 
