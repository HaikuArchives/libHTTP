#ifndef DATA_IO_UTILS_H
#define DATA_IO_UTILS_H

#include "LibHTTPBuild.h"

#include <DataIO.h>

int io_printf( BDataIO *io, const char *format, ... );
char *io_getline( BDataIO *io, char *dest, int32 len, char delim = '\n' );

#endif