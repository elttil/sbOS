CC="i686-sb-gcc"
AR="i686-sb-ar"
AS="i686-sb-as"
CFLAGS = -ggdb -ffreestanding -nostdlib -Ofast -Wall -Wextra -pedantic -Werror -Wimplicit-fallthrough -I./include/ -static -I../../include/ -Wno-int-conversion -Wno-unused-parameter
OBJ=crt0.o libc.o malloc/malloc.o pty.o sys/mman/mmap.o sys/mman/munmap.o memset.o assert.o stdio/snprintf.o stdio/vfprintf.o string/memcpy.o string/memcmp.o string/strcmp.o ubsan.o string/strcpy.o isspace.o stdio/puts.o stdio/putchar.o dirent/opendir.o dirent/readdir.o dirent/closedir.o unistd/getopt.o dirent/scandir.o dirent/alphasort.o stdio/printf.o stdio/vdprintf.o stdio/vprintf.o stdio/dprintf.o stdio/vprintf.o string/strlen.o string/strnlen.o stdio/stdin.o stdio/getchar.o stdio/fgetc.o arpa/inet/htons.o arpa/inet/htonl.o stdio/fread.o stdio/fwrite.o stdio/fopen.o stdio/fclose.o stdio/fseek.o ctype/isascii.o stdio/fprintf.o stdlib/atoi.o stdlib/strtol.o ctype/toupper.o ctype/tolower.o string/strcat.o string/strchr.o string/sscanf.o sys/stat/stat.o stdlib/getenv.o string/strrchr.o stdio/ftell.o stdio/tmpfile.o stdio/fgets.o stdio/feof.o stdio/fscanf.o stdio/ungetc.o string/strncmp.o stdio/fputc.o string/strncpy.o stdio/remove.o stdio/ferror.o stdio/fputs.o stdlib/rand.o stdlib/srand.o unistd/getpid.o stdlib/strtoul.o stdio/fflush.o stdlib/abort.o string/strcspn.o time/localtime.o time/time.o time/clock_gettime.o time/gmtime.o time/strftime.o string/strpbrk.o ctype/isdigit.o ctype/isalpha.o ctype/isxdigit.o ctype/ispunct.o stdio/setvbuf.o stdio/fileno.o stdio/putc.o stdio/sprintf.o stdlib/abs.o string/strspn.o stdlib/qsort.o string/memmove.o setjmp/longjmp.o setjmp/setjmp.o libgen/basename.o string/strdup.o string/strndup.o string/strlcpy.o stdlib/atexit.o stdio/open_memstream.o libgen/dirname.o unistd/unlink.o string/strstr.o string/strcasecmp.o string/strncasecmp.o stdlib/mkstemp.o string/strtok.o unistd/execvp.o unistd/_exit.o ctype/isalnum.o time/ctime_r.o stdlib/strtold.o sys/time/gettimeofday.o stdio/fgetpos.o stdio/fsetpos.o ctype/isprint.o stdlib/system.o stdio/tmpnam.o unistd/msleep.o stdlib/atof.o stdlib/strtod.o stdio/rename.o sys/stat/mkdir.o unistd/uptime.o unistd/ftruncate.o sys/socket/recvfrom.o sys/socket/sendto.o signal/kill.o signal/sigaction.o unistd/chdir.o unistd/getcwd.o stdio/getdelim.o stdio/getline.o unistd/isatty.o sys/socket/listen.o stdlib/realpath.o systemcall.o sys/random/randomfill.o fcntl/open.o unistd/write.o unistd/pwrite.o fcntl/open_process.o tb/sb.o tb/sv.o string/memchr.o stdlib/atol.o stdlib/atoll.o stdlib/strtoll.o sys/stat/fstat.o unistd/lseek.o ctype/isupper.o ctype/islower.o ctype/isblank.o ctype/isgraph.o ctype/iscntrl.o math/ldexp.o sys/socket/connect.o sys/socket/setsockopt.o arpa/inet/ntohl.o arpa/inet/ntohs.o netdb/getaddrinfo.o tb/sha1.o sys/socket/getpeername.o fcntl/fcntl.o queue.o sys/sendfile.o
all: libc.a

%.o: %.c
	$(CC) $(CFLAGS) -I. -o $@ -c $< -lgcc

%.o: %.s
	$(AS) $< -o $@

libc.a: $(OBJ)
	$(AR) rcs libc.a $(OBJ)

install:
	mkdir -p ../../sysroot/usr/
	cp -r ../../include ../../sysroot/usr/
	cp -r include ../../sysroot/usr/
	cp crt0.o ../../sysroot/lib/
	cp libc.a ../../sysroot/lib/

clean:
	rm libc.a *.o */*.o */*/*.o
