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
#include "ThreadCaffeine.h"

struct thread_item
{
	thread_id	thread;
	bigtime_t	user_time;
	bigtime_t 	kernel_time;
	bigtime_t	timeout;
	bigtime_t	expires;
};

ThreadCaffeine::ThreadCaffeine( bigtime_t sleep )
{
	running = false;
	this->sleep = sleep;
	
	threadList = new BList();
	lLock = new BLocker();
	
	thread_info info;
	get_thread_info( find_thread(NULL), &info );
	team = info.team;
}

status_t ThreadCaffeine::SetTeam( team_id team )
{
	team_info		info;
	
	if( get_team_info( team, &info) == B_NO_ERROR )
	{
		this->team = team;
		return B_NO_ERROR;
	}
	return B_BAD_TEAM_ID;
}

ThreadCaffeine::~ThreadCaffeine( void )
{
	lLock->Lock();
	void *anItem;
	for ( int32 i = 0; (anItem = threadList->ItemAt(i)); i++ )
		free( anItem );
	delete threadList;
	lLock->Unlock();
	delete lLock;
}

thread_id ThreadCaffeine::Run( void )
{
	if( !running )
	{
		running = true;
		main_thread = spawn_thread( thread_entry, "Caffeine", B_LOW_PRIORITY, this );
		resume_thread( main_thread );
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t ThreadCaffeine::Quit( void )
{
	if( !running )
		return B_ERROR;
	running = false;
	status_t status;
	suspend_thread( main_thread );
	snooze(1000);
	resume_thread( main_thread );
	wait_for_thread( main_thread, &status );
	return status;
}

void ThreadCaffeine::WaitForThreads( bool wake )
{
	thread_item 	*item;
	thread_id		thread;
	status_t		status;
	
	lLock->Lock();
	for ( int32 i = 0; (item = (thread_item *)threadList->ItemAt(i)); i++ )
	{
		thread = item->thread;
		if( wake )
		{
			suspend_thread( thread );
			snooze(1000);
			resume_thread( thread );
		}
		wait_for_thread( thread, &status );
	}
	lLock->Unlock();
}

status_t ThreadCaffeine::AddThread( thread_id thread, bigtime_t timeout )
{
	lLock->Lock();
	thread_info		info;
	
	if( get_thread_info( thread, &info ) == B_NO_ERROR )
	{
		thread_item *item = (thread_item *)malloc( sizeof(thread_item) );
		item->thread = thread;
		item->timeout = timeout;
		item->kernel_time = info.kernel_time;
		item->user_time = info.user_time;
		item->expires = system_time()+timeout;
		threadList->AddItem( item );
		lLock->Unlock();
		return B_NO_ERROR;
	}
	lLock->Unlock();
	return B_ERROR;
}

status_t ThreadCaffeine::RemoveThread( thread_id thread )
{
	lLock->Lock();
	thread_item *item;
	for ( int32 i = 0; (item = (thread_item *)threadList->ItemAt(i)); i++ ) 
	{
		if( item->thread == thread )
		{
			threadList->RemoveItem( item );
			free( item );
			lLock->Unlock();
			return B_NO_ERROR;
		}
	}
	lLock->Unlock();
	return B_ERROR;
}

int32 ThreadCaffeine::thread_entry( void *arg )
{
	ThreadCaffeine *obj = (ThreadCaffeine *)arg;
	return (obj->MainLoop());
}

int32 ThreadCaffeine::MainLoop( void )
{
	thread_info 	info;
	thread_item 	*item;
	int32 			cookie;
	bigtime_t 		now;
  	
	while( running )
	{
		lLock->Lock();
		now = system_time();
		for ( int32 i = 0; (item = (thread_item *)threadList->ItemAt(i)); i++ )
		{
			cookie = 0;
			while( get_next_thread_info( team, &cookie, &info ) != B_BAD_VALUE )
			{
				if( info.thread == item->thread )
				{
					// Has the thread done anything?
					if( (info.kernel_time != item->kernel_time)||(info.user_time != item->user_time)||(info.state == B_THREAD_RUNNING) )
					{
						// Reset timer
						item->kernel_time = info.kernel_time;
						item->user_time = info.user_time;
						item->expires = now+item->timeout;
					}
					else if( now >= item->expires ) // Has it expired?
					{
						// Wake it up!
						suspend_thread( item->thread );
						snooze(1000);
						resume_thread( item->thread );
						// Reset timer
						item->expires = now+item->timeout;
					}
					goto next_item;
				}
			}
			// Thread not found. Remove it from list
			threadList->RemoveItem(i--);
			free( item );
			next_item:
			continue;
		}
		lLock->Unlock();
		
		snooze( sleep );
	}
	return B_NO_ERROR;
}

