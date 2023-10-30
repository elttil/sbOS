CC="i686-sb-gcc"
CFLAGS = -ggdb -O3 -Wall -Wextra -pedantic
LIB=-L../libgui -lgui
INC=-I../libgui/
BINS=ppm
all: $(BINS)

ppm.o: ppm.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

ppm: ppm.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm $(BINS) *.o
