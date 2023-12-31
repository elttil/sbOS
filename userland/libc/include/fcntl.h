// FIXME: Is there some standard value for this?
#define O_NONBLOCK (1 << 0)
#define O_READ (1 << 1)
#define O_WRITE (1 << 2)
#define O_CREAT (1 << 3)
#define O_TRUNC (1 << 4)
#define O_APPEND (1 << 5)

#define O_RDONLY O_READ
#define O_WRONLY O_WRITE
#define O_RDWR (O_WRITE | O_READ)

int open(const char *file, int flags, ...);
