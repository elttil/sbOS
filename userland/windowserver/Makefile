CC="i686-sb-gcc"
CFLAGS = -ggdb -O0 -Wall -Wextra -Wimplicit-fallthrough
BIN=ws
LIB=-lgcc
INC=-I../libgui/
all: $(BIN)
OBJ=ws.o draw.o

%.o: %.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

clean:
	rm $(OBJ) ws

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(CFLAGS) $(OBJ) $(LIB)
