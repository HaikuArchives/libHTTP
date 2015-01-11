#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include "LibHTTPBuild.h"

#include <OS.h>
#include <Locker.h>

// Used to protect the struct *tm returned by standard time functions
// from other threads.
// struct tm * localtime (const time_t *time)
// struct tm * gmtime (const time_t *time)

extern BLocker brTimeLock;

const char *get_status_str( int32 status_code );

enum http_method { METHOD_POST, METHOD_GET, METHOD_HEAD, METHOD_OPTIONS, 
	METHOD_PUT, METHOD_DELETE, METHOD_TRACE, METHOD_UNKNOWN };
	
http_method http_find_method( const char *method );
const char *http_find_method( http_method method );

enum URI_Type { absolueURI, relativeURI };
enum Path_Type { abs_path, rel_path, empty_path };

struct brokenURI
{
	URI_Type 	URIType;
	
	char		scheme[16];
	char		host[64];
	char		port[8];
	
	Path_Type	PathType;
	
	char		path[2048];
	char		params[2048];
	char		query[2048];
};

void parse_URI( const char *URI, brokenURI *brURI );
char *URI_to_string( brokenURI *brURI, char *s, int32 size, bool full=true );
bool basic_authenticate( const char *basic_cookie, const char *uid, const char *pass, bool encrypted=true );

char *http_to_cgi_header( char *header );

extern const char *kHEAD_ALLOW;
extern const char *kHEAD_AUTHORIZATION;
extern const char *kHEAD_ENCODING;
extern const char *kHEAD_ACCEPT_ENCODING;
extern const char *kHEAD_CONNECTION;
extern const char *kHEAD_LENGTH;
extern const char *kHEAD_TYPE;
extern const char *kHEAD_DATE;
extern const char *kHEAD_EXPIRES;
extern const char *kHEAD_FROM;
extern const char *kHEAD_IF_MODIFIED;
extern const char *kHEAD_IF_UNMODIFIED;
extern const char *kHEAD_LAST_MODIFIED;
extern const char *kHEAD_LOCATION;
extern const char *kHEAD_PRAGMA;
extern const char *kHEAD_REFRESHER;
extern const char *kHEAD_SERVER;
extern const char *kHEAD_HOST;
extern const char *kHEAD_AGENT;
extern const char *kHEAD_AUTHENTICATE;
extern const char *kHEAD_CONTENT_RANGE;
extern const char *kHEAD_RANGE;
extern const char *kCRLF;
extern const char *kHTTP_DATE;
#endif
