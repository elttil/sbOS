CC=i686-sb-gcc
OBJ = sftp.o handle.o
CFLAGS = -Wall -Wextra -pedantic -Werror
LDFLAGS=

all: sftp

%.o: %.c
	clang-format -i $<
	$(CC) -c -o $@ $< $(CFLAGS)

sftp: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $^

clean:
	rm sftp $(OBJ)
