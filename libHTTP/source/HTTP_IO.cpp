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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "DNSResolver.h"
#include "HTTP_IO.h"
#include "TCP_IO.h"
#include "HTTPUtils.h"
#include "StringUtils.h"
#include "HTTPMessage.h"

HTTP_Io::HTTP_Io( void )
{
	Init();
}

HTTP_Io::HTTP_Io( const char *URL, uint32 openMode, bool head )
{
	Init();
	SetTo( URL, openMode, head );
}

HTTP_Io::~HTTP_Io( void )
{
	Unset();
	CloseConnection();
}

bool HTTP_Io::IsReadable( void )
{
	if( (initStatus == B_OK)&&(accessMode & (B_READ_ONLY | B_READ_WRITE)) )
		return true;
	else
		return false;
}

bool HTTP_Io::IsWritable( void )
{
	if( (initStatus == B_OK)&&(accessMode & (B_WRITE_ONLY | B_READ_WRITE)) )
		return true;
	else
		return false;
}


ssize_t HTTP_Io::Read( void *buffer, size_t numBytes )
{
	status_t	status;
	
	if( (status = CheckConnection()) != B_OK )
		return -1;
	
	if( !isGet )
	{
		if( position != 0 )
		{
			char		range[32];
			
			sprintf( range, "bytes=%ld-", int32(position) );
			if( MakeRequest( "GET", range ) != B_OK )
				return -1;
			if( statusCode != 206 )
				position = 0;
		}
		else if( MakeRequest( "GET" ) != B_OK )
		{
			return -1;
		}
	}
	
	if( (contentRemaining == 0)&&((strcmp( response->GetVersion(), "HTTP/1.1" ) != 0)
		||(response->FindHeader( "Connection: close" ))) )
	{
		CloseConnection();
		return -1;
	}
	
	isGet = true;
	ssize_t 	size;
	
	// Don't allow more than content-length to be read
	if( (contentLength != -1)&&(contentRemaining-int32(numBytes) < 0) )
	{
		numBytes = contentRemaining;
	}
	if( numBytes == 0 )
		return 0;
	
	if( (size = connection->Read( buffer, numBytes )) < 0 )
	{
		CloseConnection();
		return -1;
	}
	if( contentRemaining != -1 )
		contentRemaining -= size;
	position += size;
	
	if( (contentRemaining == 0)&&((strcmp( response->GetVersion(), "HTTP/1.1" ) != 0)
		||(response->FindHeader( "Connection: close" ))) )
	{
		CloseConnection();
	}
	return size;
}

ssize_t HTTP_Io::ReadAt( off_t position, void *buffer, size_t numBytes )
{
	Seek( position, SEEK_SET );
	if( this->position != position )
		return -1;
	return Read( buffer, numBytes );
}

ssize_t HTTP_Io::Write( const void *buffer, size_t numBytes )
{
	return -1;
}

ssize_t HTTP_Io::WriteAt( off_t position, const void *buffer, size_t numBytes )
{
	return -1;
}


off_t HTTP_Io::Seek( off_t position, uint32 mode )
{
	off_t		size;
	seekMode = mode;
	
	switch( mode )
	{
		case SEEK_SET:
			this->position = position;
			break;
			
		case SEEK_CUR:
			this->position += position;
			break;
			
		case SEEK_END:
			if( GetSize( &size ) == B_OK )
				this->position = size - position;
			break;
	}
	if( isGet )
	{
		if( contentRemaining != 0 )
			CloseConnection();
		isGet = false;
	}
	Read( NULL, 0 );
	return this->position;
}

off_t HTTP_Io::Position( void ) const
{
	return this->position;
}


status_t HTTP_Io::SetTo( const char *URL, uint32 openMode, bool head )
{
	if( InitCheck() == B_OK )
		Unset();
	
	if( openMode != B_READ_ONLY )
		return B_PERMISSION_DENIED;
	this->URL = (char *)malloc( strlen( URL )+1 );
	strcpy( this->URL, URL );
	brURI = new brokenURI;
	request = new HTTPRequest;
	response = new HTTPResponse;
	
	parse_URI( URL, brURI );
	
	// Set port
	if( *brURI->port == 0 )
		port = 80;
	else
		port = strtoul( brURI->port, NULL, 10 );
	
	// if not http method or host name missing
	if( (strcmp( "http", brURI->scheme ) != 0)||(*brURI->host == 0) )
	{
		Unset();
		return B_BAD_VALUE;
	}
	// Set default content type
	strcpy( contentType, "application/octet-stream" );
	
	// if asked to verify file and it does not exist...
	initStatus = B_OK;
	if( head && !Exists() )
	{
		Unset();
		return B_ENTRY_NOT_FOUND;
	}
	else
		return B_OK;
}

void HTTP_Io::Unset( void )
{
	if( URL )
		free( URL );
	if( brURI )
		delete brURI;
	if( request )
		delete request;
	if( response )
		delete response;
	InitHTTP();
}

const char *HTTP_Io::GetURL( void )
{
	return URL;
}

status_t HTTP_Io::GetSize( off_t *size )
{
	if( InitCheck() != B_OK )
		return B_NO_INIT;
	status_t status;
	if( !statusCode && ((status = DoHEAD()) != B_OK) )
		return status;
	if( contentLength != -1 )
	{
		*size = contentLength;
		return B_OK;
	}
	else
		return B_BAD_VALUE;
}

status_t HTTP_Io::GetType( char *type )
{
	if( InitCheck() != B_OK )
		return B_NO_INIT;
	status_t status;
	if( !statusCode && ((status = DoHEAD()) != B_OK) )
		return status;
	strcpy( type, contentType );
	return B_OK;
}

void HTTP_Io::InitHTTP( void )
{
	initStatus = B_NO_INIT;
	URL = NULL;
	request = NULL;
	response = NULL;
	brURI = NULL;
	*contentType = 0;
	statusCode = 0;
	position = 0;
	accessMode = 0;
	contentLength = -1;
	contentRemaining = -1;
	if( contentRemaining != 0 )
		CloseConnection();
}

void HTTP_Io::InitConnection( void )
{
	requestSN = 0;
	needsContinue = false;
	isGet = false;
	contentRemaining = -1;
	connection = NULL;
}
		
void HTTP_Io::Init( void )
{
	*lastHost = 0;
	lastPort = 0;
	*proxyHost = 0;
	InitHTTP();
	InitConnection();
}

status_t HTTP_Io::CheckConnection( void )
{
	status_t 	status;
	
	if( InitCheck() != B_OK )
		return B_NO_INIT;
	
	if( connection )
	{
		// Close connection of host or port has changed.
		if( (strcasecmp( lastHost, brURI->host ) != 0)||(lastPort != port) )
			CloseConnection();
	}
	// If no connection, attempt to connect
	if( !connection )
	{
		connection = new TCP_IO;
		if( *proxyHost )
			status = connection->Connect( proxyHost, proxyPort );
		else
			status = connection->Connect( brURI->host, port );
		strxcpy( lastHost, brURI->host, 63 );
		lastPort = port;
		if( status != B_NO_ERROR )
		{
			CloseConnection();
			return status;
		}
	}
	return B_OK;
}

void HTTP_Io::CloseConnection( void )
{
	if( connection )
	{
		delete connection;
		InitConnection();
	}
}

status_t HTTP_Io::DoHEAD( void )
{
	status_t		status;
	
	if( (status = CheckConnection()) != B_OK )
		return status;
	
	if( position != 0 )
	{
		char		range[32];
		
		sprintf( range, "bytes=%ld-", int32(position) );
		status = MakeRequest( "HEAD", range );
		if( (status == B_OK)&&(statusCode != 206) )
			position = 0;
	}
	else
		status = MakeRequest( "HEAD" );
	
	if( (status != B_OK)||(strcmp( response->GetVersion(), "HTTP/1.1" ) != 0)
		||(response->FindHeader( "Connection: close" )) )
	{
		CloseConnection();
	}
	contentRemaining = 0;
	return status;
}

status_t HTTP_Io::GetStatusCode( int32 *statusCode )
{
	if( InitCheck() != B_OK )
		return B_NO_INIT;
	if( this->statusCode )
	{
		*statusCode = this->statusCode;
		return B_OK;
	}
	else
	{
		status_t status;
		if( (status = DoHEAD()) == B_OK )
		{
			*statusCode = this->statusCode;
			return B_OK;
		}
		else
			return status;
	}
}

status_t HTTP_Io::GetStatusLine( char *statusLine, int32 size )
{
	if( InitCheck() != B_OK )
		return B_NO_INIT;
	if( this->statusCode )
	{
		strxcpy( statusLine, response->GetStatusLine(), size-1 );
		return B_OK;
	}
	else
	{
		status_t status;
		if( (status = DoHEAD()) == B_OK )
		{
			strxcpy( statusLine, response->GetStatusLine(), size-1 );
			return B_OK;
		}
		else
			return status;
	}
}

bool HTTP_Io::Exists( void )
{
	if( InitCheck() != B_OK )
		return false;
	if( !statusCode && (DoHEAD() != B_OK) )
		return false;
	else
		return true;
}

status_t HTTP_Io::MakeRequest( const char *method, const char *range )
{
	status_t 	status;
	
	if( requestSN )
	{
		// If it's not HTTP/1.1, don't expect a persistant connection
		if( strcmp( response->GetVersion(), "HTTP/1.1" ) != 0 )
		{
			// Close connection and open new connection
			CloseConnection();
			if( (status = CheckConnection()) != B_OK )
				return status;
		}
		else if( needsContinue )
		{
			// Wait for continue
			response->InitMessage();
			if( response->ReceiveMessage( connection ) < 0 )
				return B_IO_ERROR;
			else if( (statusCode = response->GetStatusCode()) != 100 )
			{
				CloseConnection();
				return B_ERROR;
			}
		}
	}
	
	request->InitMessage();
	
	if( (*proxyHost)&&(requestSN == 0) )
		request->SetRequestLine( method, URL );
	else
	{
		char	partialURL[4096];
		URI_to_string( brURI, partialURL, 4096, false );
		request->SetRequestLine( method, partialURL );
	}
	
	if( range )
		request->AddHeader( "Range", range );
	request->AddHeader( "Host", brURI->host );
	AddHeaders();
	
	if( request->SendMessage( connection ) < 0 )
	{
		CloseConnection();
		return B_IO_ERROR;
	}
	
	needsContinue = true;
	
	do
	{
		response->InitMessage();
		if( response->ReceiveMessage( connection ) < 0 )
		{
			CloseConnection();
			return B_IO_ERROR;
		}
		
		if( (statusCode = response->GetStatusCode()) == 100 )
			needsContinue = false;
	}while( statusCode == 100 );
	
	char		fieldValue[1024];
	
	if( response->FindHeader( kHEAD_LENGTH, fieldValue, 1024 ) )
	{
		contentLength = strtol( fieldValue, NULL, 10 );
		contentRemaining = contentLength;
	}
	if( response->FindHeader( kHEAD_TYPE, fieldValue, 256 ) )
		strcpy( contentType, fieldValue );
	
	requestSN++;
	return B_OK;
}

void HTTP_Io::AddHeaders( void )
{
	request->AddHeader( "User-Agent: libHTTP" );
}

void HTTP_Io::UseProxy( const char *proxyHost, uint16 proxyPort )
{
	strxcpy( this->proxyHost, proxyHost, 63 );
	this->proxyPort = proxyPort;
}

const char *HTTP_Io::FindHeader( const char *fieldName, char *fieldValue, size_t n )
{
	if( request )
		return request->FindHeader( fieldName, fieldValue, n );
	else
		return NULL;
}
const char *HTTP_Io::HeaderAt( int32 index )
{
	if( request )
		return request->HeaderAt( index );
	else
		return NULL;
}

int32 HTTP_Io::CountHeaders( void )
{
	if( request )
		return request->CountHeaders();
	else
		return 0;
}
