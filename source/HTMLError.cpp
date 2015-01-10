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
#include "HTMLError.h"

static const char *kMessageHead = 
"<HTML>\r\n"
"<HEAD><TITLE>Error</TITLE></HEAD>\r\n"
"<BODY>\r\n<H1>";

static const char *kMessageTail = 
"</H1>\r\n"
"</BODY>\r\n"
"</HTML>\r\n";

HTMLError::HTMLError( const char *message )
{
	int32		size;
	
	size = strlen( kMessageHead );
	Write( kMessageHead, size );
	size = strlen( message );
	Write( message, size );
	size = strlen( kMessageTail );
	Write( kMessageTail, size );
	Seek( 0, SEEK_SET );
}
