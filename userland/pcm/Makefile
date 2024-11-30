CC="i686-sb-gcc"
CFLAGS=-ggdb -O0 -Wall -Wextra -pedantic
LDFLAGS=
BIN=pcm
OBJ=pcm.o
all: $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm $(BIN) $(OBJ)
