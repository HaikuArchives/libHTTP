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
#include "DataIOUtils.h"

int io_printf( BDataIO *io, const char *format, ... )
{
	char		s[4096];
	int 		n;
	va_list 	argList;
	va_start( argList, format );
	n = vsprintf( s, format, argList );
	va_end( argList );
	if( n > 0 )
		n = io->Write( s, n );
	return n;
}

char *io_getline( BDataIO *io, char *ptr, int32 len, char delim )
{
	char		*next = ptr, *end = ptr+len-1;
	
	while( next < end )
	{
		if( io->Read( next, 1 ) < 1 )
		{
			*next = 0;
			return NULL;
		}
		if( *next == delim )
		{
			*next = 0;
			return ptr;
		}
		next++;
	}
	*next = 0;
	return ptr;
}