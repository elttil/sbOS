#CC="/home/anton/opt/cross/bin/i686-elf-gcc"
CC="/home/anton/prj/osdev/sysroot/bin/i686-sb-gcc"
CFLAGS = -O2 -Wall -Wextra -pedantic -Wimplicit-fallthrough -static
LIB=-L../json -L../json/hashmap -ljson -lhashmap
INC=-I../json/
BINS=test
all: $(BINS)

test.o: test.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

test: test.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm test.o test
