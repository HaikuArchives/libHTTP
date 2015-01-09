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
#include <stdio.h>
#include <time.h>
#include <File.h>
#include <Directory.h>
#include <NodeInfo.h>
#include "HTTPFileServer.h"
#include "StringUtils.h"

static const int32 kPathSize = 2048;

HTTPFileServer::HTTPFileServer( const char *defaultFileName )
{
	strxcpy( this->defaultFileName, defaultFileName, 63 );
}

HTTPFileServer::~HTTPFileServer(void)
{
	
}

HTTPHandler *HTTPFileServer::NewCopy( void )
{
	HTTPHandler *handler;
	handler = new HTTPFileServer( defaultFileName );
	return handler;
	// The following causes the compiler to choke!?
	//return new HTTPFileServer( defaultFileName );
}

status_t HTTPFileServer::SetWebDirectory( const char *webDirectory )
{
	return this->webDirectory.SetTo( webDirectory );
}

bool HTTPFileServer::MessageReceived( HTTPRequest *request )
{
	http_method 		method;
	bool				persistant = true;
	
	this->request = request;
	
	// Allocate Stuff
	response = new HTTPResponse();
	brURI = new brokenURI();
	
	// Parse URI into its sub-components
	request->ParseURI( brURI );
	
	// Unescape path
	int32			slength;
	slength = uri_unescape_str( brURI->path, brURI->path, kPathSize );
	
	// Append default file name if not present in path
	if( *defaultFileName )
	{
		if( ((brURI->path[0] == 0)||(brURI->path[slength-1] == '/'))&&(slength+11<kPathSize) )
			strcat( brURI->path, defaultFileName );
	}
	
	// Have they attempted a security violation?
	if( strstr( brURI->path, ".." ) )
	{
		response->SetHTMLMessage( 403 ); // Forbidden
		request->SendReply( response );
		goto bailout;
	}
	
	method = request->GetMethod();
	switch( method )
	{
		case METHOD_GET:
			persistant = HandleGet();
			break;
			
		case METHOD_HEAD:
			persistant = HandleHead();
			break;
			
		case METHOD_POST:
			persistant = HandlePost();
			break;
		
		case METHOD_PUT:
			persistant = HandlePut();
			break;
		
		case METHOD_DELETE:
			persistant = HandleDelete();
			break;
		
		case METHOD_OPTIONS:
			persistant = HandleOptions();
			break;
			
		case METHOD_TRACE:
			persistant = HandleTrace();
			break;
			
		default:
			persistant = HandleDefault();
			break;
	}
	
	bailout:
	// Delete stuff
	delete response;
	delete brURI;
	return persistant;
}

bool HTTPFileServer::HandleDefault( void )
{
	HTTPHandler::MessageReceived( request );
	return true;
}

bool HTTPFileServer::HTTPFileServer::HandleGet( void )
{
	// Temporary buffers
	char			fieldValue[1024];
	char			headBuffer[1024];
	
	BFile			theFile;
	BPath			absPath( webDirectory.Path(), brURI->path );
	// Was the file found?
	if( theFile.SetTo( absPath.Path(), B_READ_ONLY ) == B_NO_ERROR )
	{
		int32		statusCode = 200;
		
		// Get file length and mime type
		off_t		length;
		BNodeInfo	theNode( &theFile );
		char		mimeType[1024];
		
		theFile.GetSize( &length );
		theNode.GetType( mimeType );
		
		// Add Date header
		time_t			now;
		struct tm 		*brokentime;
		
		now = time( NULL );
		brTimeLock.Lock();
		brokentime = gmtime( &now );
		strftime( fieldValue, 256, kHTTP_DATE, brokentime );
		brTimeLock.Unlock();
		
		response->AddHeader( kHEAD_DATE, fieldValue );
		
		// Set content length
		response->SetContentLength( length );
		
		// Add content length header
		sprintf( headBuffer, "%s: %ld", kHEAD_LENGTH, int32(length) );
		response->AddHeader( headBuffer );
		
		// Add mime type header
		response->AddHeader( kHEAD_TYPE, mimeType );
		
		// Set message body
		if( request->GetMethod() == METHOD_GET )
			response->SetMessageBody( &theFile );
		
		// Set status line
		response->SetStatusLine( statusCode );
		
		// Send HTTPResonse
		request->SendReply( response );
	}
	else
	{
		response->SetHTMLMessage( 404 ); // Not Found
		request->SendReply( response );
	}
	return true;
}
