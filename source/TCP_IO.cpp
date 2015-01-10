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
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "TCP_IO.h"
#include "DNSResolver.h"

//***
// Socket_IO
//***

Socket_IO::Socket_IO( int socket )
{
	peername[0] = 0;
	closeOnDelete = false;
	sock = socket;
}

Socket_IO::~Socket_IO( void )
{
	if( closeOnDelete )
		Close();
}

ssize_t Socket_IO::Read( void *buffer, size_t numBytes )
{
	return recv( sock, buffer, numBytes, 0 );
}

ssize_t Socket_IO::Write( const void *buffer, size_t numBytes )
{
	return send( sock, buffer, numBytes, 0 );
}

status_t Socket_IO::Connect( const char *IPname, unsigned short port )
{
	sockaddr_in		remote_interface;
	int32			ip_addr;
	status_t		status;
	
	if( (status = dns_resolver.ResolveName( IPname, &ip_addr )) != B_NO_ERROR )
		return status;
	
	remote_interface.sin_family = AF_INET;
	remote_interface.sin_port = htons( port );
	remote_interface.sin_addr.s_addr = ip_addr;
	memset( remote_interface.sin_zero, 0, sizeof(remote_interface.sin_zero) );
	
	if( this->Connect( &remote_interface ) < 0 )
		return errno;
	else
		return B_NO_ERROR;
}

int Socket_IO::Connect( const struct sockaddr_in *remote_interface )
{
	return connect( sock, (sockaddr *)remote_interface, sizeof( sockaddr_in ) );
}

int Socket_IO::Bind( unsigned short port, int32 address )
{
	sockaddr_in interface = {0};
	
	interface.sin_family = AF_INET;
	interface.sin_port = htons( port );
	interface.sin_addr.s_addr = htonl(address);

	return( bind( sock, (sockaddr *)&interface, sizeof( interface ) ) );
}

int Socket_IO::Close( void )
{
	return close( sock );
}

void Socket_IO::SetCOD( bool flag )
{
	closeOnDelete = flag;
}

void Socket_IO::SetBlocking( bool shouldBlock )
{
	char	data;
	
	if( shouldBlock )
		data = 0x00;
	else
		data = 0xFF;
	setsockopt( sock, SOL_SOCKET, SO_NONBLOCK, (void *)&data, 1 );
}

int Socket_IO::GetSocket( void )
{
	return sock;
}

const char *Socket_IO::GetPeerName( void )
{
	if( peername[0] == 0 )
	{
		struct sockaddr_in interface;
		socklen_t isize = sizeof(interface);
	   
		getpeername( sock, (sockaddr *)&interface, &isize );
		strcpy( peername, inet_ntoa(interface.sin_addr) );
	}
	return peername;
}

int Socket_IO::Listen( int acceptance_count )
{
	return listen( sock, acceptance_count );
}

int Socket_IO::Accept( struct sockaddr *client_interface, int *client_size )
{
	return accept( sock, client_interface, (socklen_t *)client_size);
}

//***
// TCP_IO
//***
TCP_IO::TCP_IO( void ) :
	Socket_IO( 0 )
{
	sock = socket( AF_INET, SOCK_STREAM, 0 );
	closeOnDelete = true;
}

TCP_IO::TCP_IO( int socket ) :
	Socket_IO( socket )
{
	
}

int TCP_IO::Accept( void )
{
	sockaddr_in interface;
	return( Accept( &interface ) );
}

int TCP_IO::Accept( struct sockaddr_in *client_interface )
{
	socklen_t size;
	return( accept( sock, (sockaddr *)client_interface, &size) );
}

//***
// UDP_IO
//***

UDP_IO::UDP_IO( int socket ) :
	Socket_IO( socket )
{
	
}

UDP_IO::UDP_IO( void ) :
	Socket_IO( 0 )
{
	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	closeOnDelete = true;
}

UDP_IO::UDP_IO( unsigned short port ) :
	Socket_IO( 0 )
{
	sock = socket( AF_INET, SOCK_DGRAM, 0 );
	Bind( port );
}

//***
// TCP_Listener
//***

TCP_Listener::TCP_Listener( unsigned short port, int acceptance_count )
	: TCP_IO()
{
	this->port = port;
	this->acceptance_count = acceptance_count;
	
	Bind( port );
	Listen( acceptance_count );
}

TCP_Listener::~TCP_Listener( void )
{
	
}

unsigned short TCP_Listener::GetPort( void )
{
	return port;
}

int TCP_Listener::GetAcceptanceCount( void )
{
	return acceptance_count;
}
