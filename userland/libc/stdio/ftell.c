#include <stdio.h>

long ftell(FILE *stream) {
    return stream->offset_in_file;
}
