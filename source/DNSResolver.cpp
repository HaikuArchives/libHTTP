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
#include "DNSResolver.h"
#include <string.h>
#include <netdb.h>

DNSResolver dns_resolver;

DNSResolver::DNSResolver( void )
{
	resolveLock = new BLocker();
	lastHost[0] = 0;
}

DNSResolver::~DNSResolver( void )
{
	delete resolveLock;
}

status_t DNSResolver::ResolveName( const char *hostAddrStr, int32 *ip_addr )
{
	// Check cache
	if( strcmp( hostAddrStr, lastHost ) == 0 )
	{
		*ip_addr = last_ip;
		return B_NO_ERROR;
	}
	
	// Make sure it's only accessed by one thread at a time
	resolveLock->Lock();
	
	hostent		*theHost;
	status_t	status;
	
	if( !(theHost = gethostbyname( hostAddrStr )) )
	{
		status = h_errno;
		resolveLock->Unlock();
		return status;
	}
	*ip_addr = *(int32 *)theHost->h_addr_list[0];
	
	// Cache it
	strncpy( lastHost, hostAddrStr, 255 );
	last_ip = *ip_addr;
	resolveLock->Unlock();
	
	return B_NO_ERROR;
}

