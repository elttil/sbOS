CC="i686-sb-gcc"
CFLAGS = -ggdb -ffreestanding -O0 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -static -fsanitize=shift,signed-integer-overflow,bounds
LIB=-L../libgui -lgui -L../json -ljson -L../libc -lc -lgcc
INC=-I../libc/ -I../json/ -I../libgui/
BINS=snake
all: $(BINS)

snake.o: snake.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

snake: snake.o
	$(CC) -nostdlib $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm $(BINS) *.o
