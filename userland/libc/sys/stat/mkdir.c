#include <sys/stat.h>
#include <assert.h>

int mkdir(const char *path, mode_t mode) {
	(void)path;
	(void)mode;
	assert(0); // TODO: Implement
		   return 0;
}
