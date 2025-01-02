CC="i686-sb-gcc"
CFLAGS = -ggdb -O3 -static -Wall -Wextra -Werror -Wno-pointer-sign -Wno-int-conversion -static
INC=-I../BearSSL/inc/
BINS=tunneld
all: $(BINS)

%.o: %.c
	clang-format -i $<
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<

tunneld: tunneld.o
	$(CC) $(CFLAGS) -o $@ $^ ../BearSSL/build/libbearssl.a

clean:
	rm $(BINS) *.o
