#CC="/home/anton/opt/cross/bin/i686-elf-gcc"
#AR="/home/anton/opt/cross/bin/i686-elf-ar"
CC="/home/anton/prj/osdev/sysroot/bin/i686-sb-gcc"
AR="/home/anton/prj/osdev/sysroot/bin/i686-sb-ar"
CFLAGS = -O0 -Wall -Wextra -pedantic -Wimplicit-fallthrough -static -Wno-undef
BINS=libjson.a
all: $(BINS)
LIBS=-L./hashmap -lhashmap
OBJ=json.o

json.o: json.c
	$(CC) $(CFLAGS) $(LIBS) -c json.c

libjson.a: $(OBJ)
	$(AR) rcs libjson.a $^

clean:
	rm libjson.a json.o
