CC="i686-sb-gcc"
CFLAGS = -ggdb -O2 -Wall -Wextra -Werror -Wno-pointer-sign -Wno-int-conversion -static
LIB=-L../libgui -lgui
INC=-I../libgui
OBJS=sb.o sv.o irc.o
BINS=irc
all: $(BINS)

%.o: %.c
	clang-format -i $<
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

irc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm $(BINS) $(OBJS)
