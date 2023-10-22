CC="i686-sb-gcc"
CFLAGS = -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -static -fsanitize=shift,signed-integer-overflow,bounds
LIB=-L../libgui -lgui -L../json -ljson -L../libc -lc -lgcc
INC=-I../libc/ -I../json/ -I../libgui/
BINS=term
all: $(BINS)

term.o: term.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

term: term.o
	$(CC) -nostdlib $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm $(BINS) *.o
