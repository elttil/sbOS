#include "../include/string.h"

unsigned long strlen(const char *s)
{
	const char * tmp = s;
	for(;*tmp++;);
	return (tmp - s)-1;
}
