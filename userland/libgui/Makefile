CC="/home/anton/prj/osdev/sysroot/bin/i686-sb-gcc"
AR="/home/anton/prj/osdev/sysroot/bin/i686-sb-ar"
#CFLAGS = -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -nostdlib -static -Wno-undef -fsanitize=shift,signed-integer-overflow,bounds
CFLAGS = -O2 -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -nostdlib -static -Wno-undef
BINS=libgui.a
all: $(BINS)
LIBS=-L../libc -lc -L../json -ljson -lgcc 
OBJ=libgui.o

libgui.o: libgui.c
	$(CC) $(CFLAGS) -c libgui.c -I../libc/ -I../json/ $(LIBS)

libgui.a: $(OBJ)
	$(AR) rcs libgui.a $^

clean:
	rm $(OBJ) $(BINS)
