CC="i686-sb-gcc"
AR="i686-sb-ar"
CFLAGS = -O2 -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -nostdlib -static -Wno-undef
BINS=libgui.a
all: $(BINS)
LIBS=-L../libc -lc -L../json -ljson -lgcc 
OBJ=libgui.o

libgui.o: libgui.c
	$(CC) $(CFLAGS) -c libgui.c -I../libc/ -I../json/ $(LIBS)

libgui.a: $(OBJ)
	$(AR) rcs libgui.a $^

install:
	mkdir -p ../../sysroot/usr/include
	cp libgui.h ../../sysroot/usr/include/

clean:
	rm $(OBJ) $(BINS)
