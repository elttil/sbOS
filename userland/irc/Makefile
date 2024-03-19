CC="i686-sb-gcc"
CFLAGS = -ggdb -O2 -Wall -Wextra -Werror -Wno-pointer-sign -Wno-int-conversion -static
LIB=-L../libgui -lgui
INC=-I../libgui
BINS=irc
all: $(BINS)

sb.o: sb.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

sv.o: sv.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

irc.o: irc.c
	$(CC) $(CFLAGS) $(INC) $(LIB) -o $@ -c $<

irc: irc.o sb.o sv.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

clean:
	rm $(BINS) *.o
