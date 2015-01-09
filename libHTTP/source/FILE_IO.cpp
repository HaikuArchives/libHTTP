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
#include <unistd.h>
#include "FILE_IO.h"

FileIO::FileIO( FILE *stream )
{
	inStream = stream;
	outStream = stream;
}

FileIO::FileIO( FILE *inStream, FILE *outStream )
{
	this->inStream = inStream;
	this->outStream = outStream;
}
	
ssize_t FileIO::Read( void *buffer, size_t numBytes )
{
	return fread ( buffer, numBytes, 1, inStream );
}

ssize_t FileIO::Write( const void *buffer, size_t numBytes )
{
	return fwrite ( buffer, numBytes, 1, outStream );
}

DesIO::DesIO( int fileDes )
{
	inDes = fileDes;
	outDes = fileDes;
}

DesIO::DesIO( int inDes, int outDes )
{
	this->inDes = inDes;
	this->outDes = outDes;
}

ssize_t DesIO::Read( void *buffer, size_t numBytes )
{
	return read( inDes, buffer, numBytes );
}

ssize_t DesIO::Write( const void *buffer, size_t numBytes )
{
	return write( outDes, buffer, numBytes );
}
