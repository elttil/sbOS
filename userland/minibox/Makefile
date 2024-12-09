CC="i686-sb-gcc"
CFLAGS=-Wall -Wextra -pedantic -Wimplicit-fallthrough -g -O0
OBJ=minibox.o utilities/cat.o utilities/echo.o utilities/yes.o utilities/minibox.o utilities/ascii.o utilities/wc.o utilities/init.o utilities/ls.o utilities/touch.o utilities/ed.o utilities/sh/sh.o utilities/sh/lexer.o utilities/sh/ast.o utilities/kill.o utilities/sha1sum.o utilities/rdate.o utilities/true.o utilities/false.o utilities/lock.o

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -c $< -o $@

minibox: $(OBJ)
	$(CC) $(INCLUDE) -o $@ $^ $(CFLAGS) $(LIBS) 

clean:
	rm minibox $(OBJ)
