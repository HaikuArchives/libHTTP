#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "LibHTTPBuild.h"

#include <OS.h>

bool match_pattern( const char *pattern, const char *test );
const char *get_next_field( const char *fieldStr, char *fieldName, char *fieldValue );
size_t uri_esc_str( char *dst, const char *src, size_t bufSize, bool usePlus = false, const char *protectedChars = "" );
size_t uri_unescape_str( char *dst, const char *src, size_t bufSize );
const char *get_next_token( char *dest, const char *source, size_t size, const char *delim = " ", const char *quote = "" );
char *strxcpy (char *to, const char *from, size_t size);
size_t decode_base64( char *dst, const char *src, size_t length );

#endif
