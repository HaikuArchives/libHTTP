// libHTTP - A high-level HTTP API for the BeOS
// Copyright (C) 1999 Joe Kloss

// This library is free software; you can redistribute it and/or 
// modify it under the terms of the GNU Library General Public 
// License as published by the Free Software Foundation; either 
// version 2 of the License, or (at your option) any later version. 

// This library is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
// Library General Public License for more details. 

// You should have received a copy of the GNU Library General Public 
// License along with this library; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
// Boston, MA  02111-1307, USA.

// Contact Info:
// Author: Joe Kloss
// E-mail: axly@deltanet.com
// Postal Address: 25002 Ravenswood, Lake Forest, CA 92630, USA

#define _BUILDING_LIB_HTTP
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <SupportDefs.h>
#include <unistd.h>
#include "HTTPUtils.h"
#include "StringUtils.h"

// Status Codes
static const char *kHTTP_Continue = "Continue"; // HTTP/1.1 only!
static const char *kHTTP_OK = "OK";
static const char *kHTTP_Created = "Created";
static const char *kHTTP_Accepted = "Accepted";
static const char *kHTTP_No_Content = "No Content";
static const char *kHTTP_Partial_Content = "Partial Content";
static const char *kHTTP_Moved_Permanently = "Moved Permanently";
static const char *kHTTP_Moved_Temporarily = "Moved Temporarily";
static const char *kHTTP_Not_Modified = "Not Modified";
static const char *kHTTP_Bad_Request = "Bad Request";
static const char *kHTTP_Unauthorized = "Unauthorized";
static const char *kHTTP_Forbidden = "Forbidden";
static const char *kHTTP_Not_Found = "Not Found";
static const char *kHTTP_Internal_Server_Error = "Internal Server Error";
static const char *kHTTP_Not_Implemented = "Not Implemented";
static const char *kHTTP_Bad_Gateway = "Bad Gateway";
static const char *kHTTP_Service_Unavailable = "Service Unavailable";
static const char *kHTTP_URI_Too_Long = "Request-URI Too Long";
static const char *kHTTP_Precondition_Failed = "Precondition Failed";
static const char *kHTTP_Default = "Undefined Response";

// HTTP Header Strings
const char *kHEAD_ALLOW = "Allow";
const char *kHEAD_AUTHORIZATION = "Authorization";
const char *kHEAD_ENCODING = "Content-Encoding";
const char *kHEAD_ACCEPT_ENCODING = "Accept-Encoding";
const char *kHEAD_CONNECTION = "Connection";
const char *kHEAD_LENGTH = "Content-Length";
const char *kHEAD_TYPE = "Content-Type";
const char *kHEAD_DATE = "Date";
const char *kHEAD_EXPIRES = "Expires";
const char *kHEAD_FROM = "From";
const char *kHEAD_IF_MODIFIED = "If-Modified-Since";
const char *kHEAD_IF_UNMODIFIED = "If-Unmodified-Since";
const char *kHEAD_LAST_MODIFIED = "Last-Modified";
const char *kHEAD_LOCATION = "Location";
const char *kHEAD_PRAGMA = "Pragma";
const char *kHEAD_REFRESHER = "Refresher";
const char *kHEAD_SERVER = "Server";
const char *kHEAD_HOST = "Host";
const char *kHEAD_AGENT = "User-Agent";
const char *kHEAD_AUTHENTICATE = "WWW-Authenticate";
const char *kHEAD_RANGE = "Range";
const char *kHEAD_CONTENT_RANGE = "Content-Range";
const char *kCRLF = "\r\n";
const char *kHTTP_DATE = "%a, %d %b %Y %H:%M:%S GMT";

//Methods
static const char *kHTTP_GET = "GET";
static const char *kHTTP_POST = "POST";
static const char *kHTTP_HEAD = "HEAD";
static const char *kHTTP_PUT = "PUT";
static const char *kHTTP_DELETE = "DELETE";
static const char *kHTTP_OPTIONS = "OPTIONS";
static const char *kHTTP_TRACE = "TRACE";
static const char *kHTTP_UNKNOWN = "UNKNOWN";

BLocker		brTimeLock;

const char *get_status_str( int32 status_code )
{
	switch( status_code )
	{
		case 100:
			return kHTTP_Continue;
			break;
			
		case 200:
			return kHTTP_OK;
			break;
		
		case 201:
			return kHTTP_Created;
			break;
		
		case 202:
			return kHTTP_Accepted;
			break;
		
		case 204:
			return kHTTP_No_Content;
			break;
		
		case 206:
			return kHTTP_Partial_Content;
			break;
		
		case 301:
			return kHTTP_Moved_Permanently;
			break;
		
		case 302:
			return kHTTP_Moved_Temporarily;
			break;
		
		case 304:
			return kHTTP_Not_Modified;
			break;
		
		case 400:
			return kHTTP_Bad_Request;
			break;
		
		case 401:
			return kHTTP_Unauthorized;
			break;
		
		case 403:
			return kHTTP_Forbidden;
			break;
		
		case 404:
			return kHTTP_Not_Found;
			break;
		
		case 412:
			return kHTTP_Precondition_Failed;
			break;
		
		case 414:
			return kHTTP_URI_Too_Long;
			break;
		
		case 500:
			return kHTTP_Internal_Server_Error;
			break;
		
		case 501:
			return kHTTP_Not_Implemented;
			break;
		
		case 502:
			return kHTTP_Bad_Gateway;
			break;
		
		case 503:
			return kHTTP_Service_Unavailable;
			break;
		
		default:
			return kHTTP_Default;
			break;
	}
}

http_method http_find_method( const char *method )
{
	if( strcmp( method, kHTTP_GET ) == 0 )
		return METHOD_GET;
	else if( strcmp( method, kHTTP_POST ) == 0 )
		return METHOD_POST;
	else if( strcmp( method, kHTTP_HEAD ) == 0 )
		return METHOD_HEAD;
	else if( strcmp( method, kHTTP_OPTIONS ) == 0 )
		return METHOD_OPTIONS;
	else if( strcmp( method, kHTTP_PUT ) == 0 )
		return METHOD_PUT;
	else if( strcmp( method, kHTTP_DELETE ) == 0 )
		return METHOD_DELETE;
	else if( strcmp( method, kHTTP_TRACE ) == 0 )
		return METHOD_TRACE;
	else
		return METHOD_UNKNOWN;
}

const char *http_find_method( http_method method )
{
	switch( method )
	{
		case METHOD_GET:
			return kHTTP_GET;
			break;
		case METHOD_POST:
			return kHTTP_POST;
			break;
		case METHOD_HEAD:
			return kHTTP_HEAD;
			break;
		case METHOD_OPTIONS:
			return kHTTP_OPTIONS;
			break;
		case METHOD_PUT:
			return kHTTP_PUT;
			break;
		case METHOD_DELETE:
			return kHTTP_DELETE;
			break;
		case METHOD_TRACE:
			return kHTTP_TRACE;
			break;
		default:
			return kHTTP_UNKNOWN;
			break;
	}
}

bool basic_authenticate( const char *basic_cookie, const char *uid, const char *pass, bool encrypted )
{
	int32 			slength;
	char			token[1024];
	char			cookie[1024];
	const char		*sPtr;
	
	sPtr = get_next_token( token, basic_cookie, 1024, " " );
	if( !strstr( token, "Basic" ) )
		return false;
	sPtr = get_next_token( token, sPtr, 1024, " " );
	
	slength = strlen( token );
	decode_base64( cookie, token, slength );
	
	char	cusername[256], cpassword[256];
	sPtr = get_next_token( cusername, cookie, 256, ":" );
	sPtr = get_next_token( cpassword, sPtr, 256, ":" );
	
	if( encrypted )
	{
		if( (strcmp(cusername, uid)==0)&&(strcmp(pass, crypt( cpassword, pass ))==0) )
			return true;
		else
			return false;
	}
	else
	{
		if( (strcmp(cusername, uid)==0)&&(strcmp(pass, cpassword)==0) )
			return true;
		else
			return false;
	}
}

// This thing gives me a headache to read... but it works
void parse_URI( const char *URI, brokenURI *brURI )
{
	brURI->URIType = relativeURI;
	brURI->PathType = empty_path;
	
	brURI->scheme[0] = 0;
	brURI->host[0] = 0;
	brURI->port[0] = 0;
	brURI->path[0] = 0;
	brURI->params[0] = 0;
	brURI->query[0] = 0;
	
	const char *sPtr = URI, *ePtr;
	int32 size;
	
	if( (ePtr = strchr( sPtr, ':' )) )
	{
		brURI->URIType = absolueURI;
		if( (size = ePtr-sPtr) > 15 )
			size = 15;
		strxcpy( brURI->scheme, sPtr, size );
		sPtr = ePtr+3;
		
		if( !(ePtr = strpbrk (sPtr, ":/")) )
		{
			strxcpy( brURI->host, sPtr, 63 );
			return;
		}
		else
		{
			if( (size = ePtr-sPtr) > 63 )
				size = 63;
			strxcpy( brURI->host, sPtr, size );
		}
		
		if( *ePtr == ':' )
		{
			sPtr = ePtr+1;
			if( !(ePtr = strchr( sPtr, '/' )) )
			{
				strxcpy( brURI->port, sPtr, 7 );
				return;
			}
			else
			{
				if( (size = ePtr-sPtr) > 7 )
					size = 7;
				strxcpy( brURI->port, sPtr, size );
				sPtr = ePtr;
			}
		}
		else
			sPtr = ePtr+1;	
	}
	else
		brURI->URIType = relativeURI;
	if( *sPtr == '/' )
	{
		brURI->PathType = abs_path;
		sPtr++;
	}
	else if( *sPtr == 0 )
		return;
	else
		brURI->PathType = rel_path;
	if( !(ePtr = strpbrk (sPtr, ";?#")) )
	{
		strxcpy( brURI->path, sPtr, 2047 );
		return;
	}
	else
	{
		if( (size = ePtr-sPtr) > 2047 )
			size = 2047;
		strxcpy( brURI->path, sPtr, size );
		if( *ePtr == '#' ) // Some browsers will forget to remove the anchor name
			return;
		else if( *ePtr == ';' )
		{
			sPtr = ePtr+1;
			if( !(ePtr = strchr( sPtr, '?' )) )
			{
				strxcpy( brURI->params, sPtr, 2047 );
				return;
			}
			else
			{
				if( (size = ePtr-sPtr) > 2047 )
					size = 2047;
				strxcpy( brURI->params, sPtr, size );
			}
		}
		sPtr = ePtr+1;
		strxcpy( brURI->query, sPtr, 2047 );
	}
}

char *URI_to_string( brokenURI *uri, char *s, int32 size, bool full )
{
	size--;
	char		*strPtr = s;
	if( full && uri->scheme[0] )
	{
		//strPtr = strxcpy( strPtr, src, s+size-strPtr );
		strPtr = strxcpy( strPtr, uri->scheme, s+size-strPtr );
		strPtr = strxcpy( strPtr, "://", s+size-strPtr );
		strPtr = strxcpy( strPtr, uri->host, s+size-strPtr );
		
		if( uri->port[0] )
		{
			strPtr = strxcpy( strPtr, ":", s+size-strPtr );
			strPtr = strxcpy( strPtr, uri->port, s+size-strPtr );
		}
	}
	if( *uri->path != '/' )
		strPtr = strxcpy( strPtr, "/", s+size-strPtr );
	strPtr = strxcpy( strPtr, uri->path, s+size-strPtr );
	if( uri->params[0] )
	{
		strPtr = strxcpy( strPtr, ";", s+size-strPtr );
		strPtr = strxcpy( strPtr, uri->params, s+size-strPtr );
	}
	if( uri->query[0] )
	{
		strPtr = strxcpy( strPtr, "?", s+size-strPtr );
		strPtr = strxcpy( strPtr, uri->query, s+size-strPtr );
	}
	return strPtr;
}

char *http_to_cgi_header( char *header )
{
	char		*s = header;
	for( s = header; *s != 0; s++ )
		*s = toupper( *s );
	for( s = header; *s != 0; s++ )
	{
		if( *s == '-' )
			*s = '_';
	}
	return header;
}
