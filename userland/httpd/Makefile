CC="i686-sb-gcc"
CFLAGS = -ggdb -O3 -flto -Wall -Wextra -Werror -Wno-pointer-sign -Wno-int-conversion -static
BINS=httpd
all: $(BINS)

httpd.o: httpd.c
	$(CC) $(CFLAGS) -o $@ -c $<

httpd: httpd.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(BINS) *.o
