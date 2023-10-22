CC="/home/anton/prj/osdev/sysroot/bin/i686-sb-gcc"
CFLAGS = -ggdb -O2 -Wall -Wextra -pedantic -static
LIB=-L../libgui -lgui -lgcc
INC=-I../libgui/
BINS=ante
all: $(BINS)

ante.o: ante.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

ante: ante.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm $(BINS) *.o
