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
#include <ctype.h>
#include "FieldList.h"

FieldList::~FieldList( void )
{
	FieldList::MakeEmpty();
}

bool FieldList::AddField(const char *field, int32 size)
{
	if( size == -1 )
		size = strlen( field );
	char	*item = (char *)malloc( size+1 );
	strcpy( item, field );
	BList::AddItem( item );
	return true;
}

bool FieldList::AddField(const char *fieldName, const char *fieldValue )
{
	char	*item = (char *)malloc( strlen(fieldName)+strlen(fieldValue)+3 );
	sprintf( item, "%s: %s", fieldName, fieldValue );
	BList::AddItem( item );
	return true;
}

void FieldList::MakeEmpty(void)
{
	void *anItem;
	for ( int32 i = 0; (anItem = ItemAt(i)); i++ )
		free( anItem );
	BList::MakeEmpty();
}

bool FieldList::RemoveField( char *fieldPtr )
{
	if( BList::RemoveItem( fieldPtr ) )
	{
		free( fieldPtr );
		return true;
	}
	else
		return false;
}

bool FieldList::RemoveFieldByName( const char *fieldName )
{
	return RemoveField( (char *)FindField( fieldName ) );
}
		
const char *FieldList::FindField(const char *fieldName, char *fieldValue, size_t n)
{
	const char *item;
	int32 nameLength = strlen( fieldName );
	
	for ( int32 i = 0; (item = (char *)ItemAt(i)); i++ )
	{
		if( strncasecmp( fieldName, item, nameLength ) == 0 )
		{
			if( fieldValue != NULL )
			{
				const char	*fieldPtr = item+nameLength;
				while( !isspace( *fieldPtr ) ) // find the field delimiter (LWS)
					fieldPtr++;
					
				while( isspace( *fieldPtr ) ) // ignore leading LWS
					fieldPtr++;
				strncpy( fieldValue, fieldPtr, n );
			}
			return item;
		}
	}
	if( fieldValue )
		*fieldValue = 0;
	return NULL;
}