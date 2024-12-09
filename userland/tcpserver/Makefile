CC="i686-sb-gcc"
CFLAGS=-Wall -Wextra -pedantic -Werror -g
LDFLAGS=
OBJS=tcpserver.o
BIN=tcpserver
all: $(BIN)

%.o: %.c
	clang-format -i $<
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm $(BIN) $(OBJS)
