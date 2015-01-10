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
#include <stdlib.h>
#include <fnmatch.h>
#include <stdio.h>
#include <ctype.h>
#include <SupportDefs.h>
#include <unistd.h>
#include "StringUtils.h"

// Reserved URI Characters
static const char *kReservedChars = ";/?:@&=+ \"#%<>";
static const int32 kReservedCount = 14; // The number of reserved characters

bool match_pattern(const char *pattern, const char *text)
{
	return fnmatch(pattern, text, 0) == 0;	
}

const char *get_next_field(const char *fieldStr, char *fieldName, char *fieldValue)
{
	if (*fieldStr == 0)
		return 0;

	while (isspace(*fieldStr)) fieldStr++;

	int index = (int)(strchr(fieldStr, '=') - fieldStr);
	strncpy(fieldName, fieldStr, index);
	fieldName[index] = 0;
	fieldStr += index + 1;
	if (*fieldStr == '"')
		fieldStr++;

	char *loc = strchr(fieldStr, '"');
	if (loc)
		index = (int)(loc - fieldStr);
	else
		index = strlen(fieldStr);
	strncpy(fieldValue, fieldStr, index);
	fieldValue[index] = 0;
	fieldStr += index;
	if (loc)
		fieldStr++;

	if (*fieldStr == '"')
		fieldStr++;

	return fieldStr;
}


size_t uri_esc_str( char *dst, const char *src, size_t bufSize, bool usePlus, const char *protectedChars )
{
	const char	*snext = src;
	char		*dend = dst+bufSize, *dnext = dst;
	char		escStr[4];
	
	while( dnext < dend )
	{
		// Look for reserved characters
		// If it's in the reserved set but not in the protected set, escape it.
		if( strchr( kReservedChars, *snext )&&(!strchr( protectedChars, *snext )) )
		{
			if( usePlus && (*snext == ' ') ) // can sub '+' for ' '
			{
				*dnext++ = '+';
				snext++;
				continue;
			}
			else
			{
				sprintf( escStr, "%%%.2hx", *snext );
				if( dnext+3 < dend ) // Is there room in the buffer?
				{
					*dnext++ = escStr[0];
					*dnext++ = escStr[1];
					*dnext++ = escStr[2];
				}
				else
				{
					*dnext = 0;
					return dnext-dst;
				}
				snext++;
				continue;
			}
		}
		if( *snext == 0 ) // End of string?
		{
			*dnext = 0;
			return dnext-dst;
		}
		else // Just copy it
			*dnext++ = *snext++;
	}
	*dnext = 0;
	return dnext-dst;
}

size_t uri_unescape_str( char *dst, const char *src, size_t bufSize )
{
	const char	*snext = src;
	char		*dend = dst+bufSize, *dnext = dst;
	char		escStr[4];
	escStr[3] = 0;
	
	while( dnext < dend )
	{
		if( *snext == '%' ) // escape character?
		{
			snext++;
			escStr[0] = *snext++;
			escStr[1] = *snext++;
			
			*dnext++ = strtoul( escStr, NULL, 16 );
		}
		else if( *snext == 0 ) // NULL character?
		{
			*dnext = 0;
			return dnext-dst;
		}
		else if( *snext == '+' )
		{
			snext++;
			*dnext++ = ' ';
		}
		else // Copy
			*dnext++ = *snext++;
	}
	*dnext = 0;
	return dnext-dst;
}

const char *get_next_token( char *dest, const char *source, size_t size, const char *delim, const char *quote )
{
	const char 	*nexts = source;
	char 		*nextd = dest, *end = dest+size-1;
	int32		quoteLevel = 0;
	
	// Find start of next token and remove LWS
	while( true )
	{
		if( *nexts == 0 ) // EOF?
		{
			if( dest != NULL ) 
				*nextd = 0;
			return nexts;
		}
		
		// If space or delim, advance next ptr.
		if( strchr( delim, *nexts )||isspace( *nexts ) )
			nexts++;
		else // or start reading the token
			break;
	}
	// Read token
	while( ((!strchr( delim, *nexts ))||(quoteLevel>0))&&(*nexts != 0)&&(!dest||(nextd < end)) )
	{
		if( *quote && (*nexts == quote[0]) )
		{
			if( quote[1] )
				quoteLevel++;
			else if( quoteLevel )
				quoteLevel = 0;
			else
				quoteLevel = 1;
		}
		else if( *quote && (*nexts == quote[1]) )
			quoteLevel--;
		// if dest, copy
		if( dest )
			*nextd++ = *nexts;
		nexts++;
	}
	if( dest )
		*nextd = 0;
	return nexts;
}

// ascii to base64 table
// Starts at ascii #43
static const char ascii_to_base64T[80] = { 62, -1, -1, -1, 63, // + ... / ( 5 )
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, // 0 - 9 ( 13 )
0, -1, -1, -1, // = ( 4 )
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, // A - U ( 21 )
21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, // V - Z ( 11 )
26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, // a - r ( 18 )
44, 45, 46, 47, 48, 49, 50, 51 }; // s - z ( 8 )

// Yes, the soruce can be the same as the destination
size_t decode_base64( char *dst, const char *src, size_t length )
{
	uint8		inGroup[4];
	
	const char	*srcPtr;
	char		*dstPtr, c, b64;
	
	int32		i, last = 0, blocks;
	
	// Get source bytes in groups of four until we reach the size specified in length
	for( srcPtr = src, dstPtr = dst, blocks = 0; ((uint32)(srcPtr-src) <= length); srcPtr += 4, dstPtr += 3, blocks++ )
	{
		// Get next group of four
		for( i = 0; i < 4; i++ )
		{
			c = srcPtr[i];
			if( (c >= 43) && (c < 123 ) ) // is it in range?
			{
				b64 = ascii_to_base64T[c-43]; // translate ascii to base64
				if( b64 == -1 )
					inGroup[i] = 0;
				else
					inGroup[i] = (uint8)b64;
				if( c == '=' ) // Make note if pad character encountered
				{
					if( i == 2 )
						last = 2;
					else if( i == 3 )
						last = 1;
				}
			}
			else
				inGroup[i] = 0;
		}
		// Translate 4 6-bit characters to 3 8-bit bytes
		dstPtr[0] = ((inGroup[0]&0x3F)<<2) | ((inGroup[1]&0x30)>>4);
		dstPtr[1] = ((inGroup[1]&0x3F)<<4) | ((inGroup[2]&0x3C)>>2);
		dstPtr[2] = ((inGroup[2]&0x03)<<6) | (inGroup[3]&0x3F);
	}
	return( blocks*3 - last ); // return size of decoded data
}

// Why? strncpy() sucks!
char *strxcpy (char *to, const char *from, size_t size)
{
	*to = 0;
	while( (size--)&&(*to++ = *from++) ) {  }
	
	return to-1;
}
