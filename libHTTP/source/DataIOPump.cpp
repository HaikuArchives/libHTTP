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
#include "DataIOPump.h"
#include <stdio.h>
#include <string.h>

DataIOPump::DataIOPump( size_t bufferSize )
{
	atomic_open = 0;
	totalBytes = 0;
	this->bufferSize = bufferSize;
	close_sem = create_sem( 0, "Close Sem" );
	
	ReadFunc = NULL;
}

DataIOPump::~DataIOPump( void )
{
	StopPump();
	delete_sem( close_sem );
}

status_t DataIOPump::StartPump( BDataIO *input, BDataIO *output, ssize_t contentLength )
{
	// Make sure we are only opened by one thread at a time
	int32		previous;
	previous = atomic_or( &atomic_open, 0x00000001 );
	if( previous & 0x00000001 ) // Is pump running?
		return B_ERROR;
	
	pump_thread = find_thread( NULL );
	
	status_t 	status = B_NO_ERROR;
	char		*buffer;
	ssize_t		size, writeSize;
	
	// Allocate buffer
	buffer = (char *)malloc( bufferSize );
	totalBytes = 0;
	ssize_t readSize;
	
	// Main Loop
	running = true;
	do
	{
		if( contentLength && (int32(bufferSize) > (contentLength - totalBytes)) )
			readSize = contentLength - totalBytes;
		else
			readSize = bufferSize;
		// Read Stuff
		size = input->Read( buffer, readSize ); // Reserve space for null character at end of buffer
		// Are we done?
		if( (size <= 0)||(contentLength&&(totalBytes == contentLength)) )
		{
			status = size;
			break;
		}
		totalBytes += size;
		
		// If there is a Read Callback, Call it
		if( ReadFunc != NULL )
		{
			if( ReadFunc( cookie, buffer, size ) != B_NO_ERROR )
				break;
		}
		
		// Write Stuff
		writeSize = output->Write( buffer, size );
		// Are we done?
		if( writeSize <= 0 )
		{
			status = writeSize;
			break;
		}
		
	}while( running );
	running = false;
	// Clean up
	free( buffer );
	
	previous = atomic_and( &atomic_open, 0x00000000 ); // clear bits
	if( previous == 0x00000003 ) // is StopPump in progress?
		release_sem( close_sem ); // We are done, release StopPump
	return status;
}

status_t DataIOPump::StopPump( void )
{
	// Is the pipe open?
	int32		previous;
	previous = atomic_or( &atomic_open, 0x00000002 );
	if( previous == 0x00000000 ) // Pump was not running: clear bits
	{
		atomic_and( &atomic_open, 0x00000000 );
		return B_ERROR;
	}
	else if( previous == 0x00000003 ) // Pump was running and a StopPump() is still in progress
		return B_ERROR;
	else if( previous == 0x00000002 ) // Pump was not running and another thead needs to clear the bits
		return B_ERROR;
	// else previous == 0x00000001: Pump was not running and StopPump is not in progress
	
	running = false;
	
	// Unblock pump_thread
	suspend_thread( pump_thread );
	snooze(1000);
	resume_thread( pump_thread );
	acquire_sem( close_sem ); // Wait until pump_thread is done with cleanup
	return B_NO_ERROR;
}

int32 DataIOPump::GetTotalBytes( void )
{
	return totalBytes;
}

void DataIOPump::SetReadCallback( status_t (*ReadFunc)(void *cookie, char *buffer, int32 size), void *cookie )
{
	this->ReadFunc = ReadFunc;
	this->cookie = cookie;
}