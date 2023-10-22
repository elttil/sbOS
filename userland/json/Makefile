CC="i686-sb-gcc"
AR="i686-sb-ar"
CFLAGS = -O2 -Wall -Wextra -pedantic -Wimplicit-fallthrough -static -Wno-undef
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
