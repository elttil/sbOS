CC="i686-sb-gcc"
CFLAGS = -O2 -Wall -Wextra -pedantic -Wimplicit-fallthrough -static
BINS=test
all: $(BINS)

test.o: test.c
	$(CC) $(CFLAGS) -o $@ -c $<

test: test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm test.o test
