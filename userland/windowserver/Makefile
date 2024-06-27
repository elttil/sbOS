CC="i686-sb-gcc"
CFLAGS = -ggdb -Ofast -flto -Wall -Wextra -Wimplicit-fallthrough
LDFLAGS=-flto -Ofast
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
	$(CC) -o $(BIN) $(LDFLAGS) $(OBJ) $(LIB)
