///////////////////////////////////////////////////////////////////////////
//
// Filename    -  Network.C
//
// Description -  Network event handler for windows winsock2 events
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

#include "netstruc.h"
#include <network.h>

#include <logging.h>
#include <procreg.h>

#include <idle.h>

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


//----------------------------------------------------------------------------

static int NetworkStartup( void )
{
	static int attempt = 0;
	static int nStep = 0,
	          nError;
	static SOCKET sockMaster;
	static SOCKADDR remSin;
	switch( nStep )
	{
	case 0 :
		SystemCheck();
		nStep++;
		attempt = 0;
	case 1 :
		// Sit around, waiting for the network to start...
		//--------------------
		// sorry this is really really ugly to read!
		sockMaster = OpenSocket( TRUE, FALSE, FALSE, 0 );
		if( sockMaster == INVALID_SOCKET )
		{
			lprintf( "Clever OpenSocket failed... fallback... and survey sez..." );
			//--------------------
			sockMaster = socket( AF_INET, SOCK_DGRAM, 0);
			//--------------------
		}
		//--------------------

		if( sockMaster == INVALID_SOCKET )
		{
			nError = WSAGetLastError();

			lprintf( "Failed to create a socket - error is %ld", WSAGetLastError() );
			if( nError == 10106 ) // provvider init fail )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				return -2;
			}
			if( nError == WSAENETDOWN )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				//else return NetworkPause("Socket is delaying...");
			}
			 nStep = 0; // reset...
			if( ++attempt >= 30 ) return NetworkQuit();
			return 1;//NetworkQuit();
		}

		// Retrieve my IP address and UDP Port number
		remSin.sa_family=AF_INET;
		remSin.sa_data[0]=0;
		remSin.sa_data[1]=0;
		((SOCKADDR_IN*)&remSin)->sin_addr.s_addr=INADDR_ANY;

		nStep++;
		attempt = 0;
		// Fall into next state..............
	case 2 :
		// Associate an address with a socket. (bind)
		if( bind( sockMaster, (PSOCKADDR)&remSin, sizeof(remSin))
			 == SOCKET_ERROR )
		{
			if( WSAGetLastError() == WSAENETDOWN )
			{
				if( ++attempt >= 30 ) return NetworkQuit();
				//else return NetworkPause("Bind is Delaying");
			}
			nStep = 0; // reset...
			return NetworkQuit();
		}
		nStep++;
		attempt = 0;
		// Fall into next state..............
	case 3 :
		closesocket(sockMaster);
		sockMaster = INVALID_SOCKET;
		nStep = 0; // reset...
		break;
	}
	return 0;
}

//----------------------------------------------------------------------------

void CPROC NetworkPauseTimer( uintptr_t psv )
{
	int nResult;
	nResult = NetworkStartup();
	if( nResult == 0 )
	{
		while( !globalNetworkData.uNetworkPauseTimer )
			Relinquish();
		RemoveTimer( globalNetworkData.uNetworkPauseTimer );
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = TRUE;
	}
	else if( nResult == -1 )
	{
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = FALSE;
		// exiting ... bad stuff happens
	}
	else if( nResult == -2 )
	{
		// delaying... okay....
	}
}

//----------------------------------------------------------------------------

static void HandleEvent( PCLIENT pClient )
{
	WSANETWORKEVENTS networkEvents;
	if( globalNetworkData.bQuit )
	{
		lprintf( "Task is shutting down network... don't handle event." );
		return;
	}
	if( !pClient )
	{
		lprintf( "How did a NULL client get here?!" );
		return;
	}

	pClient->dwFlags |= CF_PROCESSING;
#ifdef LOG_NETWORK_EVENT_THREAD
	//if( globalNetworkData.flags.bLogNotices )
	//	lprintf( "Client event on %p", pClient );
#endif
	if( WSAEnumNetworkEvents( pClient->Socket, pClient->event, &networkEvents ) == ERROR_SUCCESS )
	{
		if( networkEvents.lNetworkEvents == 0 ) {
#ifdef LOG_NETWORK_EVENT_THREAD
			if( globalNetworkData.flags.bLogNotices )
				lprintf( "zero events...%p %p %p", pClient, pClient->Socket, pClient->event );
#endif
			return;
		}
		{
			if( pClient->dwFlags & CF_UDP )
			{
				if( networkEvents.lNetworkEvents & FD_READ )
				{
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "Got UDP FD_READ" );
#endif
					FinishUDPRead( pClient, 0 );
				}
			}
			else
			{
				THREAD_ID prior = 0;
#ifdef LOG_CLIENT_LISTS
				lprintf( "client lists Ac:%p(%p(%d)) Av:%p(%p(%d)) Cl:%p(%p(%d))"
						 , &globalNetworkData.ActiveClients, globalNetworkData.ActiveClients, globalNetworkData.ActiveClients?globalNetworkData.ActiveClients->Socket:0
						 , &globalNetworkData.AvailableClients, globalNetworkData.AvailableClients, globalNetworkData.AvailableClients?globalNetworkData.AvailableClients->Socket:0
						 , &globalNetworkData.ClosedClients, globalNetworkData.ClosedClients, globalNetworkData.ClosedClients?globalNetworkData.ClosedClients->Socket:0
						 );
#endif

#ifdef LOG_NETWORK_LOCKING
				lprintf( "Handle Event left global" );
#endif
				// if an unknown socket issued a
				// notification - close it - unknown handling of unknown socket.
#ifdef LOG_NETWORK_EVENT_THREAD
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "events : %08x on %p", networkEvents.lNetworkEvents, pClient );
#endif
				if( networkEvents.lNetworkEvents & FD_CONNECT )
				{
					{
						uint16_t wError = networkEvents.iErrorCode[FD_CONNECT_BIT];
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "FD_CONNECT on %p", pClient );
#endif
						if( !wError )
							pClient->dwFlags |= CF_CONNECTED;
						else
						{
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Connect error: %d", wError );
#endif
							pClient->dwFlags |= CF_CONNECTERROR;
						}
						if( !( pClient->dwFlags & CF_CONNECTERROR ) )
						{
							// since this is done before connecting is clear, tcpwrite
							// may make notice of previously queued data to
							// connection opening...
							//lprintf( "Sending any previously queued data." );

							// with events, we get a FD_WRITE also... which calls tcpwrite.
							//TCPWrite( pClient );
						}
						pClient->dwFlags &= ~CF_CONNECTING;
						if( pClient->connect.ThisConnected )
						{
							if( !wError && !pClient->saSource ) {
#ifdef __LINUX__
								socklen_t
#else
								int
#endif
									nLen = MAGIC_SOCKADDR_LENGTH;
								if( !pClient->saSource )
									pClient->saSource = AllocAddr();
								if( getsockname( pClient->Socket, pClient->saSource, &nLen ) )
								{
									lprintf( "getsockname errno = %d", errno );
								}
								SET_SOCKADDR_LENGTH( pClient->saSource
										, pClient->saSource->sa_family == AF_INET 
												? IN_SOCKADDR_LENGTH 
												: IN6_SOCKADDR_LENGTH );
							}
#ifdef LOG_NOTICES
							if( globalNetworkData.flags.bLogNotices )
								lprintf( "Post connect to application %p  error:%d", pClient, wError );
#endif
							if( pClient->dwFlags & CF_CPPCONNECT )
								pClient->connect.CPPThisConnected( pClient->psvConnect, wError );
							else
								pClient->connect.ThisConnected( pClient, wError );
						}
						//lprintf( "Returned from application connected callback" );
						// check to see if the read was queued before the connect
						// actually completed...
						if( (pClient->dwFlags & ( CF_ACTIVE | CF_CONNECTED )) ==
							( CF_ACTIVE | CF_CONNECTED ) )
						{
							if( pClient->read.ReadComplete )
								if( pClient->dwFlags & CF_CPPREAD )
									pClient->read.CPPReadComplete( pClient->psvRead, NULL, 0 );
								else
									pClient->read.ReadComplete( pClient, NULL, 0 );
						}
						if( pClient->pWaiting )
							WakeThread( pClient->pWaiting );
#ifdef LOG_NOTICES
						if( globalNetworkData.flags.bLogNotices )
							lprintf( "FD_CONNECT Completed" );
#endif
						//lprintf( "Returned from application inital read complete." );
					}
				}
				if( networkEvents.lNetworkEvents & FD_READ )
				{
					PCLIENT pcLock;
					while( !( pcLock = NetworkLockEx( pClient, 1 DBG_SRC ) ) ) {
						// done with events; inactive sockets can't have events
						if( !( pClient->dwFlags & CF_ACTIVE ) ) {
							pcLock = NULL;
							break;
						}
						Relinquish();
					}
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( "FD_READ" );
#endif
  					if( ( pClient->dwFlags & CF_ACTIVE ) ) {
#if 0 && !DrainSupportDeprecated
						if( pClient->bDraining )
						{
							TCPDrainRead( pClient );
						}
						else
#endif
						{
							// got a network event, and won't get another until recv is called.
							// mark that the socket has data, then the pend_read code will trigger the finishpendingread.
							//lprintf( "FD_READ on %p (finishpendingread)", pClient );
							if( FinishPendingRead( pClient DBG_SRC ) == 0 )
							{
								pClient->dwFlags |= CF_READREADY;
							}
							if( pClient->dwFlags & CF_TOCLOSE )
							{
								//lprintf( "Pending read failed - and wants to close." );
								//InternalRemoveClientEx( pc, TRUE, FALSE );
							}
						}
						NetworkUnlock( pClient, 1 );
					}
				}
				if( networkEvents.lNetworkEvents & FD_WRITE )
				{
					PCLIENT pcLock;
					while( !( pcLock = NetworkLockEx( pClient, 0 DBG_SRC ) ) ) {
						// done with events; inactive sockets can't have events
						if( !( pClient->dwFlags & CF_ACTIVE ) ) {
							pcLock = NULL;
							break;
						}
						Relinquish();
					}
					if( pClient->dwFlags & CF_ACTIVE ) {
#ifdef LOG_NOTICES
						//if( globalNetworkData.flags.bLogNotices )
						//	lprintf( "FD_Write" );
#endif
						// returns true while it wrote or there is data to write
						if( pClient->lpFirstPending && !pClient->flags.bAggregateOutput && !pClient->writeTimer ) {
							lprintf( "TCPWrite on %p (no timer)", pClient );
							TCPWrite(pClient);
						} else {
							pClient->dwFlags |= CF_WRITEREADY;
						}
						if( !pClient->lpFirstPending ) {
							if( pClient->dwFlags & CF_TOCLOSE )
							{
								pClient->dwFlags &= ~CF_TOCLOSE;
								//lprintf( "Pending read failed - and wants to close." );
								EnterCriticalSec( &globalNetworkData.csNetwork );
								InternalRemoveClientEx( pClient, FALSE, TRUE );
								TerminateClosedClient( pClient );
								LeaveCriticalSec( &globalNetworkData.csNetwork );
							}
						}
						NetworkUnlock( pClient, 0 );
					}
				}

				if( networkEvents.lNetworkEvents & FD_CLOSE )
				{
					//lprintf( "FD_CLOSE %p", pClient );
#if 0 && !DrainSupportDeprecated
					if( !pClient->bDraining )
#endif
					{
						size_t bytes_read;
						// act of reading can result in a close...
						// there are things like IE which close and send
						// adn we might get the close notice at application level indicating there might still be data...
						//lprintf( "closed, try pending read %p", pClient );
						while( ( bytes_read = FinishPendingRead( pClient DBG_SRC) ) > 0
							&& bytes_read != (size_t)-1 ) // try and read...
						//if( pClient->dwFlags & CF_TOCLOSE )
						{
							//lprintf( "Pending read failed - reset connection. (well this is FD_CLOSE so yeah...???)" );
							//InternalRemoveClientEx( pc, TRUE, FALSE );
						}
					}
#ifdef LOG_NOTICES
					if( globalNetworkData.flags.bLogNotices )
						lprintf("FD_CLOSE... %p  %08x", pClient, pClient->dwFlags );
#endif
					if( pClient->dwFlags & CF_ACTIVE && !pClient->flags.bInUse )
					{
						// might already be cleared and gone..
						EnterCriticalSec( &globalNetworkData.csNetwork );
						InternalRemoveClientEx( pClient, FALSE, TRUE );
						TerminateClosedClient( pClient );
						LeaveCriticalSec( &globalNetworkData.csNetwork );
						pClient->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
					}
					// section will be blank after termination...(correction, we keep the section state now)
				}
				if( networkEvents.lNetworkEvents & FD_ACCEPT )
				{
#ifdef LOG_NOTICES
					//if( globalNetworkData.flags.bLogNotices )
					//	lprintf( "FD_ACCEPT on %p", pClient );
#endif
					AcceptClient(pClient);
					//NetworkUnlock( pClient, 1 );
				}
				//lprintf( "leaveing event handler..." );
				//lprintf( "Left event handler CS." );
			}
		}
	}
	else
	{
		DWORD dwError = WSAGetLastError();
		if( dwError == 10038 )
		{
			// no longer a socket, probably in a closed or closing state.
		}
		else
			lprintf( "Event enum failed... do what? close socket? %p %" _32f, pClient, dwError );
	}
	pClient->dwFlags &= ~CF_PROCESSING;
}

//----------------------------------------------------------------------------


void RemoveThreadEvent( PCLIENT pc ) {
	struct peer_thread_info *thread = pc->this_thread;
	if( !thread ) return; // could be closed (accept, initial read, protocol causes close before ever completing getting scheduled)

	// reduce peer wait count to 1.
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "Remove client %p from %p thread events...  proc:%d  ev:%d  wait:%d", pc, thread, thread->flags.bProcessing, thread->nEvents, thread->nWaitEvents );
#endif
	thread->counting = TRUE;
	thread->nEvents = 1;
	if( thread->thread != MakeThread() && !thread->flags.bProcessing )
		while( thread->nEvents != thread->nWaitEvents ) {
			if( !thread->flags.bProcessing )
			{
				// have to make sure threads reset to the new list.
				//lprintf( "have to wait for thread to be in wait state..." );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				while( (thread->nWaitEvents > 1) || thread->flags.bProcessing ) {
					WSASetEvent( thread->hThread );
					Relinquish();
				}
				EnterCriticalSec( &globalNetworkData.csNetwork );
				EnterCriticalSec( &globalNetworkData.csNetwork );
			}
			else
				break;  // if it's processing, race it; build new list, count it.
			Relinquish();
		}
	{
		INDEX idx;
		INDEX c = 0;
		PCLIENT previous;
		PLIST newList = CreateList();
		SetLink( &newList, 65, 0 );
		//EmptyDataList( &thread->event_list );
		LIST_FORALL( thread->monitor_list, idx, PCLIENT, previous ) {
			if( previous != pc )
			{
				POINTER p = GetDataItem( &thread->event_list, idx );
				if( p ) {
					AddLink( &newList, previous );
					SetDataItem( &thread->event_list, c++, p );
				} else {
					lprintf( "Item %d is not found in events.", idx );
				}
			}
		}
		thread->event_list->Cnt = c;
		thread->nEvents = (int)c;
		thread->counting = FALSE;

		DeleteListEx( &thread->monitor_list DBG_SRC );
		thread->monitor_list = newList;
		pc->this_thread = NULL;
#ifdef LOG_NETWORK_EVENT_THREAD
		lprintf( "peer %p now has %d events", thread, thread->nEvents );
#endif
	}
	if( thread->parent_peer )  // don't bubble sort root thread
		while( ( thread->nEvents < thread->parent_peer->nEvents ) && thread->parent_peer->parent_peer ) {
#ifdef LOG_NETWORK_EVENT_THREAD
			//lprintf( "swapping this with parent peer ... this events %d  parent %d", thread->nEvents, thread->parent_peer->nEvents );
#endif
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

// unused parameter broadcsat on windows; not needed.
void AddThreadEvent( PCLIENT pc, int broadcsat )
{
	struct peer_thread_info *peer = globalNetworkData.root_thread;
	LOGICAL addPeer = FALSE;
#ifdef LOG_NOTICES
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "Add thread event %p %p %08x", pc, pc->event, pc->dwFlags );
#endif
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
			if( peer->nEvents >= 60 ) {
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( "this has max events already....", globalNetworkData.nPeers, peer->nEvents );
#endif
				addPeer = TRUE;
				break;
			}
		}
		if( peer->nEvents < 60 ) {
			if( !peer->child_peer )// last thread.
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

	// make sure to only add this handle when the first peer will also be added.
	// this means the list can be 61 and at this time no more.
	AddLink( &peer->monitor_list, pc );
	AddDataItem( &peer->event_list, &pc->event );
	pc->this_thread = peer;
	pc->flags.bAddedToEvents = 1;
	peer->nEvents++;
#ifdef LOG_NETWORK_EVENT_THREAD
	lprintf( "peer %p now has %d events", peer, peer->nEvents );
#endif
	if( !peer->flags.bProcessing && peer->parent_peer ) // scheduler thread already awake do not wake him.
		WSASetEvent( peer->hThread );
}

// this is passed '0' when it is called internally
// this is passed '1' when it is called by idleproc
int CPROC ProcessNetworkMessages( struct peer_thread_info *thread, uintptr_t quick_check )
{
	//lprintf( "Check messages." );
	if( globalNetworkData.bQuit )
		return -1;
	// disallow changing of the lists.
	if( !thread->parent_peer )
	{
		EnterCriticalSec( &globalNetworkData.csNetwork );
		{
			PCLIENT pc;
			PCLIENT next;
			for( pc = globalNetworkData.ClosedClients; pc; pc = next )
			{
#    ifdef LOG_NOTICES
				lprintf( "Have a closed client to check..." );
#    endif
				next = pc->next;
				if( +GetTickCount() > (pc->LastEvent + 1000) )
				{
					//lprintf( "Remove thread event on closed thread (should be terminate here..)" );
					// also does the remove.
					TerminateClosedClient( pc );
					//RemoveThreadEvent( pc ); // also does the close.
				}
			}
		}
		{
			PCLIENT pc;
			while( pc = (PCLIENT)DequeLink( &globalNetworkData.client_schedule ) )
			{
				// use this for "added to schedule".  Closing removes from schedule.
				if( !pc->flags.bAddedToEvents )
				{
					if( pc->dwFlags & CF_CLOSED || (!(pc->dwFlags & CF_ACTIVE)) )
					{
						lprintf( " Found closed? %p", pc );
						continue;
					}
#ifdef LOG_NETWORK_EVENT_THREAD
					if( globalNetworkData.flags.bLogNotices )
						lprintf( "Added to schedule : %p %08x", pc, pc->dwFlags );
#endif
					AddThreadEvent( pc, 0 );
				}
				else
					lprintf( "Client in schedule queue, but it is already schedule?! %p", pc );
			}
		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
		lprintf( "Process Network left global" );
#endif
	}
	else
	{
		// wait for master thread to set up the proper wait
		while( thread->nEvents == 0 )
			Relinquish();
	}
	if( thread->flags.bProcessing )
		return 0;
	while( 1 )
	{
		int32_t result;
		// want to wait here anyhow...
#ifdef LOG_NETWORK_EVENT_THREAD
		//if( globalNetworkData.flags.bLogNotices )
		//	lprintf( "%p Waiting on %d events", thread, thread->nEvents );
#endif
		thread->nWaitEvents = thread->nEvents;
		thread->flags.bProcessing = 0;
		while( thread->counting ) { thread->nWaitEvents = thread->nEvents; Relinquish(); }
		result = WSAWaitForMultipleEvents( thread->nEvents
													, (const HANDLE *)thread->event_list->data
													, FALSE
													, (quick_check)?0:WSA_INFINITE
													, FALSE
													);
		if( globalNetworkData.bQuit )
			return -1;
#ifdef LOG_NETWORK_EVENT_THREAD
		//if( globalNetworkData.flags.bLogNotices )
		//	lprintf( "Event Wait Result was %d", result );
#endif
		// this should never be 0, but we're awake, not sleeping, and should say we're in a place
		// where we probably do want to be woken on a 0 event.
		if( result != WSA_WAIT_EVENT_0 )
		{
#ifdef LOG_NETWORK_EVENT_THREAD
			//if( globalNetworkData.flags.bLogNotices )
			//	lprintf( "Begin - thread processing %d", result );
#endif
			thread->flags.bProcessing = 1;
		}
		thread->nWaitEvents = 0;
		if( result == WSA_WAIT_FAILED )
		{
			DWORD dwError = WSAGetLastError();
			if( dwError == WSA_INVALID_HANDLE )
			{
				lprintf( "Rebuild list, have a bad event handle somehow." );
				break;
			}
			lprintf( "error of wait is %d   %p", dwError, thread );
			LogBinary( (const uint8_t*)thread->event_list->data, 64 );
			break;
		}
#ifndef UNDER_CE
		else if( result == WSA_WAIT_IO_COMPLETION )
		{
			// reselect... not sure where io completion fits for network...
			continue;
		}
#endif
		else if( result == WSA_WAIT_TIMEOUT )
		{
			if( quick_check )
				return 1;
		}
		else if( result >= WSA_WAIT_EVENT_0 )
		{
			// if result is _0, then it's the global event, and we just return.
			if( result > WSA_WAIT_EVENT_0 )
			{
				PCLIENT pc = (PCLIENT)GetLink( &thread->monitor_list, result - (WSA_WAIT_EVENT_0) );
				if( !pc || ( pc->dwFlags & CF_AVAILABLE ) ) {
					//lprintf( "thread event happened on a now available client." );
				}
				else
					HandleEvent( pc );
				if( !quick_check )
					continue;
			}
			//else
			{
#ifdef LOG_NOTICES
				if( globalNetworkData.flags.bLogNotices )
					lprintf( thread->parent_peer?"RESET THREAD EVENT":"RESET GLOBAL EVENT" );
#endif
				WSAResetEvent( thread->hThread );
			}
			return 1;
		}
	}
	// result 0... we had nothing to do
	// but we are this thread.
	return 0;
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
