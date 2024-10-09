///////////////////////////////////////////////////////////////////////////
//
// Filename    -  network_mac.C
//
// Description -  Network event handling for MAC(BSD) using kevent 
//
// Author      -  James Buckeyne
//
// Create Date -  2019-06-26
//
///////////////////////////////////////////////////////////////////////////

//
//  DEBUG FLAGS IN netstruc.h
//
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE  // for features.h
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#define FIX_RELEASE_COM_COLLISION
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <stddef.h>
#include <ctype.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sqlgetoption.h>

#include "netstruc.h"
#include <network.h>

//#define DO_LOGGING // override no _DEBUG def to do loggings...
//#define NO_LOGGING // force neverlog....

#include <logging.h>
#include <procreg.h>
#ifdef __LINUX__
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#endif

#include <sharemem.h>
#include <timers.h>
#include <idle.h>

//for GetMacAddress
#ifdef __LINUX__
#include <net/if.h>
//#include <sys/timeb.h>

//*******************8
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/un.h>
#include <arpa/inet.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#else
#include "android_ifaddrs.h"
#define EPOLLRDHUP EPOLLHUP
#define EPOLL_CLOEXEC 0
#endif
#ifdef __MAC__
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
//*******************8

#endif
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#ifdef __CYGWIN__
#include <mingw/tchar.h>
#else
#include <tchar.h>
#endif
#include <wincrypt.h>
#include <iphlpapi.h>
#endif

SACK_NETWORK_NAMESPACE


//static uintptr_t CPROC NetworkThreadProc( PTHREAD thread );

void RemoveThreadEvent( PCLIENT pc ) {
	struct peer_thread_info *thread = pc->this_thread;
	// could be closed (accept, initial read, protocol causes close before ever completing getting scheduled)
	if( !thread ) {
	 	lprintf( "didn't have one? %p", pc );
		return;
	}
	{
#  ifdef __EMSCRIPTEN__
#  else
#      ifdef __64__
		struct kevent64_s ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET64( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		} else {
			EV_SET64( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uintptr_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
			EV_SET64( &ev, pc->Socket, EVFILT_WRITE, EV_DELETE, 0, 0, (uintptr_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
		if( pc->SocketBroadcast ) {
			EV_SET64( &ev, pc->SocketBroadcast, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc, NULL, NULL );
			kevent64( thread->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
#      else
		struct kevent ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		} else {
			EV_SET( &ev, pc->Socket, EVFILT_READ, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
			EV_SET( &ev, pc->Socket, EVFILT_WRITE, EV_DELETE, 0, 0, (uintptr_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		}
		if( pc->SocketBroadcast ) {
			EV_SET( &ev, pc->SocketBroadcsat, EVFILT_READ, EV_DELETE, 0, 0, (uint64_t)pc );
			kevent( thread->kqueue, &ev, 1, 0, 0, 0 );
		}
#      endif
#  endif // __EMSCRIPTEN__
	}
	LockedDecrement( &thread->nEvents );
#    ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "peer %p now has %d events", thread, thread->nEvents );
#    endif
	// don't bubble sort root thread
	if( thread->parent_peer )
		while( (thread->nEvents < thread->parent_peer->nEvents) && thread->parent_peer->parent_peer ) {
#    ifdef LOG_NETWORK_EVENT_THREAD
			//lprintf( "swapping this with parent peer ... this events %d  parent %d", thread->nEvents, thread->parent_peer->nEvents );
#    endif
			if( thread->child_peer )
				thread->child_peer->parent_peer = thread->parent_peer;
			thread->parent_peer->child_peer = thread->child_peer;
			struct peer_thread_info *tmp = thread->parent_peer;
			tmp->parent_peer->child_peer = thread;
			thread->parent_peer = tmp->parent_peer;
			tmp->parent_peer = thread;
			thread->child_peer = tmp;
		}
}

struct event_data {
	PCLIENT pc;
	int broadcast;
};

void AddThreadEvent( PCLIENT pc, int broadcast )
{
	struct peer_thread_info *peer = globalNetworkData.root_thread;
	LOGICAL addPeer = FALSE;
	if( pc->Socket <= 0 ) {
		lprintf( "SAVED FROM A FATAL INFINITE EVENT LOOP");
		return;
	}
#ifdef LOG_NOTICES
	//if( globalNetworkData.flags.bLogNotices )
		lprintf( "Add thread event %p %d %08x  %s", pc, broadcast?pc->SocketBroadcast:pc->Socket, pc->dwFlags, broadcast?"broadcast":"direct" );
#endif
	if( !broadcast ) {
		for( ; peer; peer = peer->child_peer ) {
			if( !peer->child_peer ) {
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "On last peer..." );
#endif
				if( peer->nEvents > globalNetworkData.nPeers ) {
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "global peers is %d, this has %d", globalNetworkData.nPeers, peer->nEvents );
#endif
					addPeer = TRUE;
					break;
				}
				if( peer->nEvents >= 2560 ) {
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "this has max events already.... %d %d", globalNetworkData.nPeers, peer->nEvents );
#endif
					addPeer = TRUE;
					break;
				}
			}
			if( peer->nEvents < 2560 ) {
												// last thread.
				if( !peer->child_peer )
					break;
				if( peer->nEvents < peer->child_peer->nEvents ) {
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( "this event has fewer than the next thread's events %d  %d", peer->nEvents, peer->child_peer->nEvents );
#endif
					break;
				}
			}
		}
		if( addPeer ) {
#ifdef LOG_NOTICES
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "Creating a new thread...." );
#endif
			AddLink( (PLIST*)&globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)peer ) );
			globalNetworkData.nPeers++;
			while( !peer->child_peer )
				Relinquish();
			if( globalNetworkData.root_thread != peer ) {
				// relink to be higher in list of peers so it's found earlier.
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "Relinking thread to be after root peer (no events, so it must be first)" );
#endif
				peer->child_peer->child_peer = globalNetworkData.root_thread->child_peer;
				globalNetworkData.root_thread->child_peer->parent_peer = peer->child_peer;
				globalNetworkData.root_thread->child_peer = peer->child_peer;
				peer->child_peer->parent_peer = globalNetworkData.root_thread;
				peer->child_peer = NULL;
				peer = globalNetworkData.root_thread->child_peer;
			}
			else
				peer = peer->child_peer;
		}
	} else {
		peer = pc->this_thread; // add broadcast to the same event as the original.
	}
	// make sure to only add this handle when the first peer will also be added.
	// this means the list can be 61 and at this time no more.
#ifdef __EMSCRIPTEN__
#else

	{
		struct event_data *data = New( struct event_data );
		data->pc = pc;
		data->broadcast = broadcast;
#    ifdef __64__
		struct kevent64_s ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)data, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
		else {
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)data, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
			EV_SET64( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, (uintptr_t)data, NULL, NULL );
			kevent64( peer->kqueue, &ev, 1, 0, 0, 0, 0 );
		}
#    else
		struct kevent ev;
		if( pc->dwFlags & CF_LISTEN ) {
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)data );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
		}
		else {
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (uintptr_t)data );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
			EV_SET( &ev, broadcast?pc->SocketBroadcast:pc->Socket, EVFILT_WRITE, EV_ADD|EV_ENABLE|EV_CLEAR, 0, 0, (uintptr_t)data );
			kevent( peer->kqueue, &ev, 1, 0, 0, 0 );
		}
#    endif
	}
#endif // __EMSCRIPTEN__
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "added thread: %p  %p  %p  ", pc, pc->this_thread, peer );
#endif
	if( !pc->this_thread ) {

		LockedIncrement( &peer->nEvents );
		pc->this_thread = peer;
		pc->flags.bAddedToEvents = 1;
	}
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "peer %p now has %d events", peer, peer->nEvents );
#endif
	// scheduler thread already awake do not wake him.
}

int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t unused )
{
	int cnt;
	struct timeval time;
	if( globalNetworkData.bQuit )
		return -1;
#ifdef __EMSCRIPTEN__
		lprintf( "Need Old Select Code Here" );
		return 1;
#else
	{
#    ifdef __64__
		struct kevent64_s events[10];
		cnt = kevent64( thread->kqueue, NULL, 0, events, 10, 0, NULL );
#    else
		kevent events[10];
		cnt = kevent( thread->kqueue, NULL, 0, events, 10, NULL );
#    endif
		if( cnt < 0 )
		{
			int err = errno;
			if( err == EINTR )
				return 1;
			Log1( "Sorry epoll_pwait/kevent call failed... %d", err );
			return 1;
		}
		if( cnt > 0 )
		{
			int closed;
			int n;
			struct event_data *event_data;
			THREAD_ID prior = 0;
			PCLIENT next;
#  ifdef LOG_NETWORK_EVENT_THREAD
			lprintf( "process %d events", cnt );
#  endif
			for( n = 0; n < cnt; n++ ) {
				closed = 0;
				event_data = (struct event_data*)events[n].udata;
#  ifdef LOG_NOTICES
				lprintf( "Process %d %x %x"
				       , ((uintptr_t)event_data == 1) ?0:event_data->broadcast?event_data->pc->SocketBroadcast:event_data->pc->Socket
				       , ((uintptr_t)event_data == 1) ?0:event_data->pc->dwFlags
				       , events[n].filter );
#  endif
				if( event_data == (struct event_data*)1 ) {
					//char buf;
					//int stat;
					//stat = read( GetThreadSleeper( thread->thread ), &buf, 1 );
					//call wakeable sleep to just clear the sleep; because this is an event on the sleep pipe.
					//WakeableSleep( 1 );
					//lprintf( "This should sleep forever..." );
					return 1;
				}

				if( events[n].filter == EVFILT_READ )
				{
					int locked;
					locked = 1;

					while( !NetworkLock( event_data->pc, 1 ) ) {
						if( !( event_data->pc->dwFlags & CF_ACTIVE ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
							lprintf( "failed lock dwFlags : %8x", event_data->pc->dwFlags );
#  endif
							locked = 0;
							break;
						}
						if( event_data->pc->dwFlags & CF_AVAILABLE ) {
							locked = 0;
							break;
						}
						Relinquish();
					}
					if( !( event_data->pc->dwFlags & ( CF_ACTIVE | CF_CLOSED ) ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
						lprintf( "not active but locked? dwFlags : %8x", event_data->pc->dwFlags );
#  endif
						continue;
					}
					if( event_data->pc->dwFlags & CF_AVAILABLE )
						continue;

					if( !IsValid( event_data->pc->Socket ) ) {
						NetworkUnlock( event_data->pc, 1 );
						continue;
					}

#  ifdef LOG_NETWORK_EVENT_THREAD
					lprintf( "EPOLLIN/EVFILT_READ %x", event_data->pc->dwFlags );
#  endif
					if( event_data->pc->dwFlags & CF_CLOSED ) {
						PCLIENT pClient = event_data->pc;
						// close notice went to application; all resources for application are gone.
						// any pending reads are no longre valid.

						//lprintf( "socket is already closed... what do we need to do?");
						//WakeableSleep( 100 );
#if 0 && !DrainSupportDeprecated
						if( 0 && !pClient->bDraining )
#endif
						{
							size_t bytes_read;
							// act of reading can result in a close...
							// there are things like IE which close and send
							// adn we might get the close notice at application level indicating there might still be data...
							while( ( bytes_read = FinishPendingRead( pClient DBG_SRC) ) > 0
								&& bytes_read != (size_t)-1 ); // try and read...
							//if( pClient->dwFlags & CF_TOCLOSE )
							{
								//lprintf( "Pending read failed - reset connection. (well this is FD_CLOSE so yeah...???)" );
								//InternalRemoveClientEx( pc, TRUE, FALSE );
							}
						}
#  ifdef LOG_NOTICES
						//if( globalNetworkData.flags.bLogNotices )
							lprintf("FD_CLOSE... %p  %08x", pClient, pClient->dwFlags );
#  endif
						//if( pClient->dwFlags & CF_ACTIVE )
						{
							// might already be cleared and gone..
							//InternalRemoveClientEx( pClient, FALSE, TRUE );
							TerminateClosedClient( pClient );
							closed = 1;
						}
						// section will be blank after termination...(correction, we keep the section state now)
						pClient->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
					} else if( !(event_data->pc->dwFlags & (CF_ACTIVE) ) ) {
						lprintf( "Event on socket no longer active..." );
						// change to inactive status by the time we got here...
					} else if( event_data->pc->dwFlags & CF_LISTEN )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "accepting..." );
#endif
						AcceptClient( event_data->pc );
					}
					else if( event_data->pc->dwFlags & CF_UDP )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "UDP Read Event..." );
#endif
						//lprintf( "UDP READ" );
						FinishUDPRead( event_data->pc, event_data->broadcast );
					}
#if 0 && !DrainSupportDeprecated
					else if( event_data->pc->bDraining )
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "TCP Drain Event..." );
#endif
						TCPDrainRead( event_data->pc );
					}
#endif
					else if( event_data->pc->dwFlags & CF_READPENDING )
					{
						size_t read;
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "TCP Read Event..." );
#endif
						// packet oriented things may probably be reading only
						// partial messages at a time...
						read = FinishPendingRead( event_data->pc DBG_SRC );
						//lprintf( "Read %d", read );
						if( ( read == -1 ) && ( event_data->pc->dwFlags & CF_TOCLOSE ) )
						{
#ifdef LOG_NOTICES
							//if( globalNetworkData.flags.bLogNotices )
							lprintf( "Pending read failed - reset connection." );
#endif
							EnterCriticalSec( &globalNetworkData.csNetwork );
							InternalRemoveClientEx( event_data->pc, FALSE, FALSE );
							TerminateClosedClient( event_data->pc );
							LeaveCriticalSec( &globalNetworkData.csNetwork );
							closed = 1;
						}
						else if( !event_data->pc->RecvPending.s.bStream )
							event_data->pc->dwFlags |= CF_READREADY;
					}
					else
					{
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "TCP Set read ready..." );
#endif
						event_data->pc->dwFlags |= CF_READREADY;
					}
					if( locked )
						LeaveCriticalSec( &event_data->pc->csLockRead );
				}

				if( !closed && ( event_data->pc->dwFlags & CF_ACTIVE ) ) {
					int locked;
					locked = 1;
					if( events[n].filter == EVFILT_WRITE )
					{
#  ifdef LOG_NETWORK_EVENT_THREAD
						lprintf( "EPOLLOUT %s", ( event_data->pc->dwFlags & CF_CONNECTING ) ? "connecting"
							: ( !( event_data->pc->dwFlags & CF_ACTIVE ) ) ? "closed" : "writing" );
#  endif

						while( !NetworkLock( event_data->pc, 0 ) ) {
							locked = 0;
							break;
						}
						if( !( event_data->pc->dwFlags & ( CF_ACTIVE | CF_CLOSED ) ) ) {
#  ifdef LOG_NETWORK_EVENT_THREAD
							lprintf( "not active but locked? dwFlags : %8x", event_data->pc->dwFlags );
#  endif
							continue;
						}
						if( event_data->pc->dwFlags & CF_AVAILABLE )
							continue;

						if( !IsValid( event_data->pc->Socket ) ) {
							NetworkUnlock( event_data->pc, 0 );
							continue;
						}
						if( !( event_data->pc->dwFlags & CF_ACTIVE ) ) {
							//lprintf( "FLAGS IS NOT ACTIVE BUT: %x", event_data->pc->dwFlags );
							// change to inactive status by the time we got here...
						} else if( event_data->pc->dwFlags & CF_CONNECTING ) {
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Connected!" );
#endif
							event_data->pc->dwFlags |= CF_CONNECTED;
							event_data->pc->dwFlags &= ~CF_CONNECTING;

							{
								PCLIENT pc = event_data->pc;
#ifdef __LINUX__
								socklen_t
#else
								int
#endif
									nLen = MAGIC_SOCKADDR_LENGTH;
								if( !pc->saSource )
									pc->saSource = AllocAddr();
								if( getsockname( pc->Socket, pc->saSource, &nLen ) ) {
									lprintf( "getsockname errno = %d", errno );
								}
								if( pc->saSource->sa_family == AF_INET )
									SET_SOCKADDR_LENGTH( pc->saSource, IN_SOCKADDR_LENGTH );
								else if( pc->saSource->sa_family == AF_INET6 )
									SET_SOCKADDR_LENGTH( pc->saSource, IN6_SOCKADDR_LENGTH );
								else
									SET_SOCKADDR_LENGTH( pc->saSource, nLen );
							}

							{
								int error;
								socklen_t errlen = sizeof( error );
								getsockopt( event_data->pc->Socket, SOL_SOCKET
									, SO_ERROR
									, &error, &errlen );
								//lprintf( "Error checking for connect is: %s on %d", strerror( error ), event_data->pc->Socket );
								if( event_data->pc->pWaiting ) {
#ifdef LOG_NOTICES
									if( globalNetworkData.flags.bLogNotices )
										lprintf( "Got connect event, waking waiter.." );
#endif
									WakeThread( event_data->pc->pWaiting );
								}
								event_data->pc->dwFlags |= CF_CONNECT_ISSUED;
								if( event_data->pc->dwFlags & CF_CPPCONNECT ) {
									if( event_data->pc->connect.CPPThisConnected )
										event_data->pc->connect.CPPThisConnected( event_data->pc->psvConnect, error );
								}else {
									if( event_data->pc->connect.ThisConnected )
										event_data->pc->connect.ThisConnected( event_data->pc, error );
								}
#ifdef LOG_NOTICES
								if( globalNetworkData.flags.bLogNotices )
									lprintf( "Connect error was: %d", error );
#endif
								// if connected okay - issue first read...
								if( !error ) {
#ifdef LOG_NOTICES
									lprintf( "Read Complete" );
#endif
									if( event_data->pc->read.ReadComplete ) {
#ifdef LOG_NOTICES
										lprintf( "Initial Read Complete" );
#endif
										event_data->pc->read.ReadComplete( event_data->pc, NULL, 0 );
									}
									if( event_data->pc->lpFirstPending ) {
										//lprintf( "Data was pending on a connecting socket, try sending it now" );
										TCPWrite( event_data->pc );
									}
								} else {
									event_data->pc->dwFlags |= CF_CONNECTERROR;
								}
							}
						} else if( event_data->pc->dwFlags & CF_UDP ) {
							//lprintf( "UDP WRITE IS NEVER QUEUED." );
							// udp write event complete....
							// do we ever care? probably sometime...
						} else {
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "TCP Write Event..." );
#endif
							// if write did not get locked, a write is in progress
							// wait until it finished there?
							// did this wake up because that wrote?
							if( locked ) {
								TCPWrite( event_data->pc );
							}
							else
								event_data->pc->flags.bWriteOnUnlock = true;
						}
						if( locked )
							NetworkUnlock( event_data->pc, 0 );
					}
				} else {
					//lprintf( "Already closed? Stop looping on this event? %p %d %x", event_data->pc, event_data->pc->Socket, event_data->pc->dwFlags );
				}
			}
			// had some event  - return 1 to continue working...
		}
		return 1;
	}
#endif
	//lprintf( "Attempting to wake thread (send sighup!)" );
	//WakeThread( globalNetworkData.pThread );
	//lprintf( "Only reason should get here is if it's not this thread..." );
	return -1;
}

//----------------------------------------------------------------------------
int CPROC IdleProcessNetworkMessages( uintptr_t quick_check )
{
	struct peer_thread_info *this_thread = IsNetworkThread();
	if( this_thread )
		return ProcessNetworkMessages( this_thread, quick_check );
	return -1;
}


SACK_NETWORK_NAMESPACE_END
