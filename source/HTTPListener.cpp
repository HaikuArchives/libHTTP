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
#include "TCP_IO.h"
#include "BufferIO.h"
#include "HTTPListener.h"
#include "ThreadCaffeine.h"
#include "DataIOUtils.h"
#include "HTTPUtils.h"

static const char *kVERSION = "HTTP/1.1";
static const char *kSTATUS_F = "%s %ld %s\r\n";

//*******
// HTTPListener
//******

HTTPListener::HTTPListener( HTTPHandler *handler, unsigned short port, int acceptance_count )
	: TCP_Listener( port, acceptance_count )
{
	this->port = port;
	this->handler = handler;
	this->maxConnections = maxConnections;
	running = false;
}

HTTPListener::~HTTPListener( void )
{
	
}

thread_id HTTPListener::Run(void)
{
	int32		previous;
	previous = atomic_or( &running, 0x00000001 );
	if( previous & 0x00000001 )
		return B_ERROR;
	
	char		name[256];
	sprintf( name, "HTTP Listen on port %d", port );
	accept_thread = spawn_thread( accept_loop_entry, name, B_NORMAL_PRIORITY, this );
	if( accept_thread < B_NO_ERROR )
		return accept_thread;
	resume_thread( accept_thread );
	return accept_thread;
}

void HTTPListener::Quit(void)
{
	if( !running )
		return;
	running = false;
	status_t status;
	suspend_thread( accept_thread );
	snooze(1000);
	resume_thread( accept_thread );
	wait_for_thread( accept_thread, &status );
}

int32 HTTPListener::accept_loop_entry(void *arg)
{
	HTTPListener *obj = (HTTPListener *)arg;
	return (obj->AcceptLoop());
}

int32 HTTPListener::AcceptLoop(void)
{
	thread_id		connection_t;
	ThreadCaffeine	cafe;
	thread_id		cafe_thread;
	int				socket;
	HTTPConnection	*connection;
	
	cafe_thread = cafe.Run();
	while( running )
	{
		// Wait for connection
		if( (socket = Accept()) < 0 )
			break;
			
		// Pass the connection to a new HTTPConnection object/thread
		connection = new HTTPConnection( handler->NewCopy(), socket, port );
		connection_t = connection->Run();
		
		// Add thread to cafe list
		// This will wake the thread up if it's idle for more than 30 seconds
		cafe.AddThread( connection_t, 30000000 );
	}
	// Stop cafe thread and wait for all HTTPConnection threads to terminate
	cafe.Quit();
	cafe.WaitForThreads( true );
	return B_NO_ERROR;
}

//*******
// HTTPConnection
//******

HTTPConnection::HTTPConnection( HTTPHandler *handler, int32 socket, uint16 acceptPort )
{
	this->handler = handler;
	this->socket = socket;
	this->acceptPort = acceptPort;
}

HTTPConnection::~HTTPConnection( void )
{
	
}

thread_id HTTPConnection::Run( void )
{
	thread_id		connection_t;
	connection_t = spawn_thread( connection_loop_entry, "http thread", B_NORMAL_PRIORITY, this );
	resume_thread( connection_t );
	return connection_t;
}

int32 HTTPConnection::connection_loop_entry( void *arg )
{
	status_t	status;
	
	HTTPConnection *obj = (HTTPConnection *)arg;
	status = obj->ConnectionLoop();
	delete obj;
	return status;
}
		
int32 HTTPConnection::ConnectionLoop( void )
{
	TCP_IO				socketio( socket );
	BufferedIO			io( &socketio );
	io.DoAllocate();
	
	HTTPRequest		request;
	
	bool 			persistent;
	bool			simple;
	bool			oneone;
	
	const char		*version;
	int32			bufferSize = 64;
	char			buffer[64];
	
	// Call handler hook
	handler->ConnectionOpened( socketio.GetPeerName() );
	
	do // receive messages while persistent connection
	{
		// Allocate new request and receive message
		if( request.ReceiveMessage( &socketio ) < 0 )
			break; // receive failure
		
		version = request.GetVersion();
		
		// Set flag defaults
		oneone = false;
		persistent = false;
		
		// If no version, then the request is a HTTP simple request
		if( version[0] == 0 )
			simple = true;
		else // we can expect headers
		{
			simple = false;
			// If HTTP/1.1, then use persistent connection by default
			if( strcasecmp( version, "HTTP/1.1" ) == 0 )
			{
				oneone = true;
				persistent = true;
			}
			
			// If "Connection: close" header, then explicit request for non-persistant connection
			if( (request.FindHeader( kHEAD_CONNECTION, buffer, bufferSize ))&&
				(strcasecmp( buffer, "close" ) == 0) )
			{
				persistent = false;
			}
			
			/*if( (request.FindHeader( kHEAD_CONNECTION, buffer, bufferSize ))&&
				(strcasecmp( buffer, "Keep-Alive" ) == 0) )
			{
				persistent = true;
			}*/
		}
		
		if( persistent && oneone )
		{
			// Send "HTTP/1.1 100 Continue" status line
			// This tells client it's ok to send the next request
			io_printf( &io, kSTATUS_F, kVERSION, 100, get_status_str( 100 ) );
			io_printf( &io, kCRLF );
			io.Sync();
		}
		
		// Set BDataIO for use by HTTPRequest::SendReply()
		request.SetReplyIO( &socketio );
		request.SetPort( acceptPort );
		request.SetRemoteHost( socketio.GetPeerName() );
		// Call handler to deal with request
		if( handler->MessageReceived( &request ) == false )
			persistent = false;
		
		
		// Reset the message
		request.InitMessage();
	}while( persistent );
	// Call handler hook
	handler->ConnectionClosed( B_NO_ERROR );
	
	socketio.Close();
	delete handler;
	return B_NO_ERROR; 
}



