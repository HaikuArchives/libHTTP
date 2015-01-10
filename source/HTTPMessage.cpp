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
#include <ctype.h>
#include <stdio.h>
#include "HTTPMessage.h"
#include "BufferIO.h"
#include "DataIOPump.h"
#include "HTMLError.h"
#include "DataIOUtils.h"
#include "StringUtils.h"

static const char *kDEFAULT_VERSION = "HTTP/1.1";

//*******
// HTTPMessage
//******

HTTPMessage::HTTPMessage( void )
{
	InitMessage();
}

HTTPMessage::~HTTPMessage( void )
{
	
}

void HTTPMessage::InitMessage( void )
{
	start_line[0] = 0;
	SetVersion( kDEFAULT_VERSION );
	contentLength = 0;
	buffered = false;
	body = NULL;
	FreeHeaders();
}

void HTTPMessage::SetStartLine( const char *start_line )
{
	strxcpy( this->start_line, start_line, 4095 );
}

const char *HTTPMessage::GetStartLine( void )
{
	return start_line;
}

void HTTPMessage::SetVersion( const char *version )
{
	strxcpy( this->version, version, 15 );
}

const char *HTTPMessage::GetVersion( void )
{
	return version;
}

void HTTPMessage::AddHeader( const char *header )
{
	headers.AddField( header );
}

void HTTPMessage::AddHeader( const char *fieldName, const char *fieldValue )
{
	headers.AddField( fieldName, fieldValue );
}

bool HTTPMessage::RemoveHeader( char *headerPtr )
{
	return headers.RemoveField( headerPtr );
}

bool HTTPMessage::RemoveHeaderByName( const char *fieldName )
{
	return headers.RemoveFieldByName( fieldName );
}
		
const char *HTTPMessage::FindHeader( const char *fieldName, char *fieldValue, size_t n )
{
	return headers.FindField( fieldName, fieldValue, n );
}

const char *HTTPMessage::HeaderAt( int32 index )
{
	return (const char *)headers.ItemAt( index );
}

int32 HTTPMessage::CountHeaders( void )
{
	return headers.CountItems();
}

void HTTPMessage::FreeHeaders( void )
{
	headers.MakeEmpty();
}

void HTTPMessage::SetContentLength( int64 length )
{
	contentLength = length;
}

int64 HTTPMessage::GetContentLength( void )
{
	return contentLength;
}

void HTTPMessage::SetMessageBody( BDataIO *body )
{
	this->body = body;
}

BDataIO *HTTPMessage::GetMessageBody( void )
{
	return body;
}

status_t HTTPMessage::DeleteMessageBody( void )
{
	if( body )
	{
		delete body;
		return B_NO_ERROR;
	}
	else
		return B_ERROR;
}

int32 HTTPMessage::SendMessage( BDataIO *io, bool simple )
{
	int32			size, totalBytes = 0;
	
	if( (size = SendHeaders( io, simple )) < 0 )
		return -1;
	totalBytes += size;
	if( !simple )
	{
		if( (size = SendBody( io )) < 0 )
			return -1;
		totalBytes += size;
	}
	return totalBytes;
}

int32 HTTPMessage::SendHeaders( BDataIO *io, bool simple )
{
	int32			size, totalBytes = 0;
	
	BufferedIO		bufio( io );
	bufio.DoAllocate();
	
	if( (size = io_printf( &bufio, "%s\r\n", start_line )) < 0 )
		return -1;
	totalBytes += size;
	
	if( !simple )
	{
		char *responseHeader;
		for ( int32 i = 0; (responseHeader = (char *)headers.ItemAt(i)); i++ )
		{
			if( (size = io_printf( &bufio, "%s\r\n", responseHeader )) < 0 )
				return -1;
			totalBytes += size;
		}
		io_printf( &bufio, kCRLF );
	}
	if( bufio.Sync() < 0  )
		return -1;
	return totalBytes;
}

int32 HTTPMessage::SendBody( BDataIO *io )
{
	if( body == NULL )
		return 0;
	DataIOPump		ioPump;
	
	if( buffered )
	{
		BufferedIO		bufio( io );
		bufio.DoAllocate();
		
		ioPump.StartPump( body, &bufio, contentLength );
		bufio.Sync();
	}
	else
		ioPump.StartPump( body, io, contentLength );
	return ioPump.GetTotalBytes();
}

void HTTPMessage::SetBodyBuffering( bool buffered )
{
	this->buffered = buffered;
}

int32 HTTPMessage::ReceiveStartLine( BDataIO *io )
{
	int32			size;
	
	do
	{
		if( !io_getline( io, start_line, 4096 ) )
			return -1;
	}while( (start_line[0] == '\r')||(start_line[0] == '\n') ); // ignore empty lines before request
	
	// Strip the '\r' character if there is one
	size = strlen( start_line )-1;
	if( start_line[size] == '\r' )
		start_line[size] = 0;
	else
		size++;
	return size+2; // + the CRLF
}

int32 HTTPMessage::ReceiveHeaders( BDataIO *io )
{
	int32			bufferSize = 4096;
	int32			size, totalBytes = 0;
	char			buffer[4096];
	char 			headBuffer[4096];
	
	headBuffer[0] = 0;
	
	while( true )
	{
		if( !io_getline( io, (char *)buffer, bufferSize ) )
			return -1;
		// Strip the '\r' character if there is one
		size = strlen( buffer )-1;
		if( buffer[size] == '\r' )
			buffer[size] = 0;
		else
			size++;
		totalBytes += size+2; // + the CRLF
		
		if( (*buffer == ' ')||(*buffer == '\t') )
		{
			strncat( headBuffer, buffer+1, bufferSize-1 );
		}
		else
		{
			if( *headBuffer != 0 )
			{
				headers.AddField(headBuffer, strlen(headBuffer));
				*headBuffer = 0;
			}
			if( (buffer[1] == 0)||(buffer[0] == 0) ) // is last header?
				break; // break from read header loop
			strxcpy( headBuffer, buffer, bufferSize-1 );
		}
	}
	return totalBytes;
}

int32 HTTPMessage::ReceiveBody( BDataIO *io )
{
	SetMessageBody( io );
	return 0;
}

//*******
// HTTPRequest
//******

HTTPRequest::HTTPRequest( void )
	: HTTPMessage()
{
	replyIO = NULL;
	port = 80;
	*remoteHost = 0;
}

HTTPRequest::~HTTPRequest( void )
{
	
}

void HTTPRequest::SetRequestLine( const char *requestLine )
{
	SetStartLine( requestLine );
	
	char			token[16];
	const char		*strPtr = start_line;
	
	strPtr = get_next_token( token, strPtr, 16 );
	method = http_find_method( token );
	strPtr = get_next_token( NULL, strPtr, 4096 );
	strPtr = get_next_token( version, strPtr, 16 );
}

void HTTPRequest::SetRequestLine( const char *method, const char *uri )
{
	sprintf( start_line, "%s %s %s", method, uri, version );
	this->method = http_find_method( method );
}

void HTTPRequest::SetRequestLine( http_method method, brokenURI *uri )
{
	this->method = method;
	char		*strPtr = start_line;
	
	strPtr = strxcpy( strPtr, http_find_method(method), start_line+4095-strPtr );
	strPtr = strxcpy( strPtr, " ", start_line+4095-strPtr );
	strPtr = URI_to_string( uri, strPtr, start_line+4095-strPtr );
	strPtr = strxcpy( strPtr, " ", start_line+4095-strPtr );
	strPtr = strxcpy( strPtr, version, start_line+4095-strPtr );
}

void HTTPRequest::GetRequestLine( char *method, char *uri, char *version )
{
	const char		*strPtr = start_line;
	
	strPtr = get_next_token( method, strPtr, 16 );
	strPtr = get_next_token( uri, strPtr, 4096 );
	strPtr = get_next_token( version, strPtr, 16 );
}

http_method HTTPRequest::GetMethod( void )
{
	return method;
}

void HTTPRequest::ParseURI( brokenURI *uri )
{
	char			uriString[4096];
	const char		*strPtr = start_line;
	
	strPtr = get_next_token( NULL, strPtr, 16 );
	strPtr = get_next_token( uriString, strPtr, 4096 );
	
	parse_URI( uriString, uri );
}

int32 HTTPRequest::ReceiveMessage( BDataIO *io )
{
	int32			size, totalBytes = 0;
	
	if( (size = ReceiveStartLine( io )) < 0 )
		return -1;
	totalBytes += size;
	
	char			token[4096];
	const char		*strPtr = start_line;
	
	strPtr = get_next_token( token, strPtr, 4096 );
	method = http_find_method( token );
	strPtr = get_next_token( token, strPtr, 4096 );
	strPtr = get_next_token( version, strPtr, 16 );
	
	if( version[0] != 0 ) // If it's not a simple request...
	{
		if( (size = ReceiveHeaders( io )) < 0 )
			return -1;
		totalBytes += size;
		
		// If content length header...
		if( headers.FindField( kHEAD_LENGTH, token, 4096 ) )
		{
			contentLength = strtol( token, (char **)&strPtr, 10 );
			if( (size = ReceiveBody( io )) < 0 )
				return -1;
			totalBytes += size;
		}
	}
	return totalBytes;
}

void HTTPRequest::SetReplyIO( BDataIO *replyIO )
{
	this->replyIO = replyIO;
}

BDataIO *HTTPRequest::GetReplyIO( void )
{
	return replyIO;
}

int32 HTTPRequest::SendReply( HTTPResponse *response )
{
	if( !replyIO )
		return B_ERROR;
		
	bool		simple;
	if( version[0] == 0 )
		simple = true;
	else
		simple = false;
	return response->SendMessage( replyIO, simple );
}

void HTTPRequest::SetPort( unsigned short port )
{
	this->port = port;
}

unsigned short HTTPRequest::GetPort( void )
{
	return port;
}

void HTTPRequest::SetRemoteHost( const char *remoteHost )
{
	strxcpy( this->remoteHost, remoteHost, 63 );
}

const char *HTTPRequest::GetRemoteHost( void )
{
	return remoteHost;
}

//*******
// HTTPResponse
//******

HTTPResponse::HTTPResponse( void )
	: HTTPMessage()
{
	internalMessage = NULL;
}

void HTTPResponse::SetStatusLine( const char *status )
{
	sprintf( start_line, "%s %s", version, status );
	
	char			token[16];
	const char		*strPtr = status;
	
	strPtr = get_next_token( token, strPtr, 16 );
	statusCode = strtol( token, (char **)&strPtr, 10 );
}

HTTPResponse::~HTTPResponse( void )
{
	if( internalMessage )
		delete internalMessage;
}

void HTTPResponse::SetStatusLine( int32 statusCode )
{
	this->statusCode = statusCode;
	sprintf( start_line, "%s %ld %s", version, statusCode, get_status_str( statusCode ) );
}

int32 HTTPResponse::GetStatusCode( void )
{
	return statusCode;
}

int32 HTTPResponse::SendMessage( BDataIO *io, bool simple )
{
	int32			size = 0, totalBytes = 0;
	
	if( !simple )
	{
		if( (SendHeaders( io )) < 0 )
			return -1;
		totalBytes += size;
	}
	if( (size = SendBody( io )) < 0 )
		return -1;
	totalBytes += size;
	return totalBytes;
}

int32 HTTPResponse::ReceiveMessage( BDataIO *io )
{
	int32			size, totalBytes = 0;
	
	if( (size = ReceiveStartLine( io )) < 0 )
		return -1;
	totalBytes += size;
	
	char			token[4096];
	const char		*strPtr = start_line;
	
	strPtr = get_next_token( version, strPtr, 16 );
	strPtr = get_next_token( token, strPtr, 4096 );
	statusCode = strtol( token, (char **)&strPtr, 10 );
	
	if( (size = ReceiveHeaders( io )) < 0 )
		return -1;
	totalBytes += size;
	
	// If content length header...
	if( headers.FindField( kHEAD_LENGTH, token, 4096 ) )
	{
		contentLength = strtol( token, (char **)&strPtr, 10 );
		if( (size = ReceiveBody( io )) < 0 )
			return -1;
		totalBytes += size;
	}
	
	return totalBytes;
}

void HTTPResponse::SetHTMLMessage( int32 statusCode, const char *msg )
{
	SetStatusLine( statusCode );
	if( internalMessage )
	{
		delete internalMessage;
		internalMessage = NULL;
	}
	HTMLError		*msgBody = NULL;
	
	if( msg )
		msgBody = new HTMLError( msg );
	else if( statusCode >= 400 )
	{
		char			s[1024];
		
		sprintf( s, "%s %ld %s", GetVersion(), statusCode, get_status_str( statusCode ) );
		msgBody = new HTMLError( s );
	}
	
	internalMessage = msgBody;
	
	int32		length = 0;
	if( internalMessage )
	{
		char		s[32];
		
		length = msgBody->BufferLength();
		sprintf( s, "%ld", length );
		AddHeader( kHEAD_LENGTH, s );
		AddHeader( kHEAD_TYPE, "text/html" );
	}
	
	SetContentLength( length );
	SetMessageBody( msgBody );
}
