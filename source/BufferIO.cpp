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
#include <stdio.h>
#include <stdlib.h>
#include "BufferIO.h"
#include "stdarg.h"


BufferedIO::BufferedIO( BDataIO *childIO, size_t bufferSize )
{
	this->childIO = childIO;
	this->bufferSize = bufferSize;
	pbase = pnext = pend = NULL;
}

BufferedIO::~BufferedIO( void )
{
	if( pbase != NULL )
		free( pbase );
}

status_t BufferedIO::DoAllocate( void )
{
	if( bufferSize <= 0 )
		return B_ERROR;
	if( !(pbase = (char *)malloc( bufferSize )) )
		return B_ERROR;
	pnext = pbase;
	pend = pbase+bufferSize;
	return B_NO_ERROR;
}

void BufferedIO::SetChildIO( BDataIO *childIO )
{
	this->childIO = childIO;
}

ssize_t BufferedIO::Read( void *buffer, size_t numBytes )
{
	return childIO->Read( buffer, numBytes );
}

ssize_t BufferedIO::Write( const void *buffer, size_t n )
{
	if( pbase == NULL )
		return childIO->Write( buffer, n );
	if( pnext+n > pend ) // Will it fit in the buffer?
	{
		if( Sync() < 1 )	
			return EOF;
		// If it's bigger than the buffer, send it directly from "buffer"
		else if( n > bufferSize )
			return childIO->Write( buffer, n );
	}
		
	if( n == 1 ) // Do not bother with memcpy() if n == 1
		*pnext = *((char *)buffer);
	else
		memcpy (pnext, buffer, n);
	pnext += n;
	return n;
}

int BufferedIO::Sync( void )
{
	if( pbase == NULL )
		return 0;
	
	int32		n = pnext-pbase, size;
	
	if( n <= 0 ) // is there anything in the buffer?
		return 0;
	if ( (size = childIO->Write( pbase, n )) < 0 )
		return -1;
	pnext = pbase;
	return size;
}
