CC="/home/anton/prj/osdev/sysroot/bin/i686-sb-gcc"
CFLAGS = -ggdb -ffreestanding -O2 -Wall -Wextra -pedantic -mgeneral-regs-only -Wimplicit-fallthrough -fsanitize=shift,signed-integer-overflow,bounds
BIN=ws
LIB=-L../json -ljson -L../json/hashmap -lhashmap -L../libc -lc -lgcc
INC=-I../json/ -I../libgui/
all: $(BIN)
OBJ=ws.o draw.o

%.o: %.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

clean:
	rm $(OBJ) ws

$(BIN): $(OBJ)
	$(CC) -o $(BIN) -ffreestanding -nostdlib $(CFLAGS) $(OBJ) $(LIB)
