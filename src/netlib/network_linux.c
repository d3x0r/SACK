///////////////////////////////////////////////////////////////////////////
//
// Filename    -  network_linux.C
//
// Description -  Network event handling using epoll
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
#include <sys/epoll.h>
//*******************8

#endif

SACK_NETWORK_NAMESPACE



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
		int r;
#      ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "Removing event %p   %d from poll %d", pc, pc->Socket, thread->epoll_fd );
#      endif
		//r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DISABLE, pc->Socket, NULL );
		//if( r < 0 ) lprintf( "Error removing:%d", errno );
		
		r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DEL, pc->Socket, NULL );
		if( r < 0 ) lprintf( "Error removing:%d", errno );
		if( pc->SocketBroadcast ) {
			r = epoll_ctl( thread->epoll_fd, EPOLL_CTL_DEL, pc->SocketBroadcast, NULL );
			if( r < 0 ) lprintf( "Error removing:%d", errno );
		}
		pc->flags.bAddedToEvents = 0;
		pc->this_thread = NULL;
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
		int r;
		struct epoll_event ev;
		ev.data.ptr = New( struct event_data );
		((struct event_data*)ev.data.ptr)->pc = pc;
		((struct event_data*)ev.data.ptr)->broadcast = broadcast;
		if( pc->dwFlags & CF_LISTEN )
			ev.events = EPOLLIN;
		else {
			ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
		}
#    ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "peer add socket %d to %d now has 0x%x events", pc->Socket, peer->epoll_fd, ev.events );
#    endif
		r = epoll_ctl( peer->epoll_fd, EPOLL_CTL_ADD, broadcast?pc->SocketBroadcast:pc->Socket, &ev );
		if( r < 0 ) lprintf( "Error adding:%d %d", errno, broadcast?pc->SocketBroadcast:pc->Socket );
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

int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t non_blocking )
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
		struct epoll_event events[10];
#    ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "Wait on %d   %d", thread->epoll_fd, thread->nEvents );
#    endif
		if( non_blocking ) {
			cnt = epoll_wait( thread->epoll_fd, events, 10, 0 );
		}else {
			cnt = epoll_wait( thread->epoll_fd, events, 10, -1 );
		}
		if( cnt < 0 )
		{
			int err = errno;
			if( err == EINTR )
				return 1;
			if( err == EINVAL ) {

			}
			lprintf( "Sorry epoll_pwait/kevent call failed... %d %d %m", cnt, err );
			return 1;
		}
		else if( cnt > 0 )
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
				event_data = (struct event_data*)events[n].data.ptr;
#  ifdef LOG_NOTICES
				if( event_data != (struct event_data*)1 ) {
					lprintf( "Process %d %x", event_data->broadcast?event_data->pc->SocketBroadcast:event_data->pc->Socket
							 , events[n].events );
				}
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

				if( events[n].events & EPOLLIN )
				{
					int locked;
					locked = 1;
					// ------- Large complicated lock ---------------
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
					// ------- End Large complicated lock ---------------

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
#  ifdef LOG_NOTICES
						lprintf("FD_CLOSE... %p  %08x", pClient, pClient->dwFlags );
#  endif
						EnterCriticalSec( &globalNetworkData.csNetwork );
						InternalRemoveClientEx( pClient, FALSE, TRUE );
						TerminateClosedClient( pClient );
						closed = 1;
						LeaveCriticalSec( &globalNetworkData.csNetwork );

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
						if( ( read == -1 ) && ( event_data->pc->dwFlags & CF_TOCLOSE ) && !event_data->pc->flags.bInUse )
						{
							int locked;
							locked = 1;
#ifdef LOG_NOTICES
							//if( globalNetworkData.flags.bLogNotices )
							lprintf( "Pending read failed - reset connection." );
#endif
							// lock other both channels.
							while( !NetworkLock( event_data->pc, 0 ) ) {
								if( !(event_data->pc->dwFlags & CF_ACTIVE) ) {
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
								//lprintf( "Have to unlock..." );
								NetworkUnlock( event_data->pc, 1 );
								Relinquish();
								//lprintf( "Need to relock..." );
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
								if( !locked ) break;
							}
							if( locked ) {
								while( !TryNetworkGlobalLock( DBG_VOIDSRC ) ) {
									NetworkUnlock( event_data->pc, 1 );
									NetworkUnlock( event_data->pc, 0 );
									Relinquish();
									while( !NetworkLock( event_data->pc, 0 ) ) {
										if( !(event_data->pc->dwFlags & CF_ACTIVE) ) {
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
									if( !locked ) break;
									while( !NetworkLock( event_data->pc, 1 ) ) {
										if( !(event_data->pc->dwFlags & CF_ACTIVE) ) {
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
										NetworkUnlock( event_data->pc, 0 );
										Relinquish();
										while( !NetworkLock( event_data->pc, 0 ) ) {
											if( !(event_data->pc->dwFlags & CF_ACTIVE) ) {
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
										if( !locked ) break;
									}
									if( !locked ) break;
								}
								if( locked ) {
									InternalRemoveClientEx( event_data->pc, FALSE, FALSE );
									NetworkUnlock( event_data->pc, 0 );
									TerminateClosedClient( event_data->pc );
									LeaveCriticalSec( &globalNetworkData.csNetwork );
									closed = 1;
								}
							}
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
				// many times a read event can cause the socket to close before write can complete.
				if( !closed && ( event_data->pc->dwFlags & CF_ACTIVE ) ) {
					const PCLIENT pc = event_data->pc;
					int locked;
					locked = 1;
					if( events[n].events & EPOLLOUT )
					{
#  if defined( LOG_NETWORK_EVENT_THREAD ) || defined( LOG_WRITE_NOTICES )
						lprintf( "EPOLLOUT %s", ( event_data->pc->dwFlags & CF_CONNECTING ) ? "connecting"
							: ( !( event_data->pc->dwFlags & CF_ACTIVE ) ) ? "closed" : "writing" );
#  endif

						if( !NetworkLock( event_data->pc, 0 ) ) {							
							locked = 0;
						}
						if( !( event_data->pc->dwFlags & ( CF_ACTIVE | CF_CLOSED ) ) ) {
#  if defined( LOG_NETWORK_EVENT_THREAD ) || defined( LOG_WRITE_NOTICES )
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
#  if defined( LOG_NETWORK_EVENT_THREAD ) || defined( LOG_WRITE_NOTICES )
							//if( globalNetworkData.flags.bLogNotices )
								lprintf( "Connected!" );
#endif
							event_data->pc->dwFlags |= CF_CONNECTED;
							event_data->pc->dwFlags &= ~CF_CONNECTING;

							{
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
								if( event_data->pc->connect.ThisConnected )
									event_data->pc->connect.ThisConnected( event_data->pc, error );
#ifdef LOG_NOTICES
								if( globalNetworkData.flags.bLogNotices )
									lprintf( "Connect error was: %d", error );
#endif
								// if connected okay - issue first read...
								if( !error ) {
#ifdef LOG_NOTICES
									lprintf( "Read Complete" );
#endif
									if( pc->read.ReadComplete ) {
#  if defined( LOG_NETWORK_EVENT_THREAD ) || defined( LOG_WRITE_NOTICES )
										lprintf( "Initial Read Complete" );
#endif
										pc->read.ReadComplete( pc, NULL, 0 );
									} 
									else lprintf( "Initial read completed without read callback...");
									// see if initial read generated any writes...
									if( pc->lpFirstPending 
											&&( !pc->flags.bAggregateOutput 
											|| !pc->writeTimer ) ) {
										//lprintf( "Data was pending on a connecting socket, try sending it now" );
										TCPWrite( pc );
									} else {
										//lprintf( "No data pending on a connecting socket; setting write ready" );
										pc->dwFlags |= CF_WRITEREADY;
									}
									if( !pc->lpFirstPending ) {
										if( pc->dwFlags & CF_TOCLOSE )
										{
											pc->dwFlags &= ~CF_TOCLOSE;
											lprintf( "Pending write completed - and wants to close." );
											EnterCriticalSec( &globalNetworkData.csNetwork );
											InternalRemoveClientEx( pc, FALSE, TRUE );
											TerminateClosedClient( pc );
											LeaveCriticalSec( &globalNetworkData.csNetwork );
										}
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
#  if defined( LOG_NETWORK_EVENT_THREAD ) || defined( LOG_WRITE_NOTICES )
							//if( globalNetworkData.flags.bLogNotices )
								lprintf( "TCP Write Event..." );
#endif
							// if write did not get locked, a write is in progress
							// wait until it finished there?
							// did this wake up because that wrote?
							if( locked ) {
								if( pc->lpFirstPending 
										&&( !pc->flags.bAggregateOutput 
											|| !pc->writeTimer ) ) {
									//lprintf( "(2)Data was pending on a connected socket, try sending it now" );
									// this is the normal large packet auto write....
									TCPWrite( pc );
								}else {
									pc->dwFlags |= CF_WRITEREADY;
								}
							} else {
								lprintf( "Write but lock wasn't enabled?" );
								// although this is only a few instructions down, this can still be a lost event that the other 
								// lock has already ended, so this will get lost still...
								event_data->pc->flags.bWriteOnUnlock = 1;
								//lprintf( "Write event didn't get the lock... and maybe we won't get to write more?" );
							}
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
