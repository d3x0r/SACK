///////////////////////////////////////////////////////////////////////////
//
// Filename    -  Network.C
//
// Description -  Network services for Communications Client
//
// Author      -  James Buckeyne
//
// Create Date -  Before now.
// Conversion update for Linux GLIBC 2.1 9/26/2000
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
#define MAIN_PROGRAM
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

PRELOAD( InitNetworkGlobalOptions )
{
	if( !globalNetworkData.flags.bOptionsRead ) {
#ifdef __LINUX__
		signal(SIGPIPE, SIG_IGN);
#endif
#ifndef __NO_OPTIONS__
		globalNetworkData.flags.bLogProtocols = SACK_GetProfileIntEx( "SACK", "Network/Log Protocols", 0, TRUE );
		globalNetworkData.flags.bShortLogReceivedData = SACK_GetProfileIntEx( "SACK", "Network/Log Network Received Data(64 byte max)", 0, TRUE );
		globalNetworkData.flags.bLogReceivedData = SACK_GetProfileIntEx( "SACK", "Network/Log Network Received Data", 0, TRUE );
		globalNetworkData.flags.bLogSentData = SACK_GetProfileIntEx( "SACK", "Network/Log Network Sent Data", globalNetworkData.flags.bLogReceivedData, TRUE );
#  ifdef LOG_NOTICES
		globalNetworkData.flags.bLogNotices = SACK_GetProfileIntEx( "SACK", "Network/Log Network Notifications", 0, TRUE );
#  endif
		globalNetworkData.dwReadTimeout = SACK_GetProfileIntEx( "SACK", "Network/Read wait timeout", 5000, TRUE );
		globalNetworkData.dwConnectTimeout = SACK_GetProfileIntEx( "SACK", "Network/Connect timeout", 10000, TRUE );
#else
		globalNetworkData.flags.bLogNotices = 0;
		globalNetworkData.dwReadTimeout = 5000;
		globalNetworkData.dwConnectTimeout = 10000;
#endif
		globalNetworkData.flags.bOptionsRead = 1;
	}
}

static void LowLevelNetworkInit( void )
{
	if( !global_network_data ) {
		SimpleRegisterAndCreateGlobal( global_network_data );
	}
	if( !globalNetworkData.ClientSlabs )
		InitializeCriticalSec( &globalNetworkData.csNetwork );
}

PRIORITY_PRELOAD( InitNetworkGlobal, CONFIG_SCRIPT_PRELOAD_PRIORITY - 1 )
{
	LowLevelNetworkInit();
	if( !globalNetworkData.system_name )
	{
		globalNetworkData.system_name = "no.network";
	}
}

//----------------------------------------------------------------------------

#ifdef LOG_CLIENT_LISTS
void DumpLists( void )
{
	int c = 0;
	PCLIENT pc;
	for( pc = globalNetworkData.AvailableClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( "Available %p", pc );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( "Overflow available clients." );
		//DebugBreak();
	}

	c = 0;
	for( pc = globalNetworkData.ActiveClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( WIDE( "Active %p(%d)" ), pc, pc->Socket );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( "Overflow active clients." );
		DebugBreak();
	}

	c = 0;
	for( pc = globalNetworkData.ClosedClients; c < 50 && pc; pc = pc->next )
	{
		//lprintf( "Closed %p(%d)", pc, pc->Socket );
		if( (*pc->me) != pc )
			DebugBreak();
		c++;
	}
	if( c > 50 )
	{
		lprintf( "Overflow closed clients." );
		DebugBreak();
	}
}
#endif

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

PCLIENT GrabClientEx( PCLIENT pClient DBG_PASS )
#define GrabClient(pc) GrabClientEx( pc DBG_SRC )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		_lprintf(DBG_RELAY)( "grabbed client %p(%d)", pClient, pClient->Socket );
		lprintf( "grabbed client %p Ac:%p(%p(%d)) Av:%p(%p(%d)) Cl:%p(%p(%d))"
				 , pClient->me
					 , &globalNetworkData.ActiveClients, globalNetworkData.ActiveClients, globalNetworkData.ActiveClients?globalNetworkData.ActiveClients->Socket:0
					 , &globalNetworkData.AvailableClients, globalNetworkData.AvailableClients, globalNetworkData.AvailableClients?globalNetworkData.AvailableClients->Socket:0
					 , &globalNetworkData.ClosedClients, globalNetworkData.ClosedClients, globalNetworkData.ClosedClients?globalNetworkData.ClosedClients->Socket:0
				 );
		DumpLists();
#endif
		pClient->dwFlags &= ~CF_STATEFLAGS;
		if( pClient->dwFlags & CF_AVAILABLE )
			lprintf( "Grabbed. %p  %08x", pClient, pClient->dwFlags );
		pClient->LastEvent = GetTickCount();
		if( ( (*pClient->me) = pClient->next ) )
			pClient->next->me = pClient->me;
	}
	return pClient;
}

//----------------------------------------------------------------------------

static PCLIENT AddAvailable( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( "Add Avail client %p(%d)", pClient, pClient->Socket );
		DumpLists();
#endif
		pClient->dwFlags |= CF_AVAILABLE;
		pClient->LastEvent = GetTickCount();
		pClient->me = &globalNetworkData.AvailableClients;
		if( ( pClient->next = globalNetworkData.AvailableClients ) )
			globalNetworkData.AvailableClients->me = &pClient->next;
		globalNetworkData.AvailableClients = pClient;
	}
	return pClient;
}

//----------------------------------------------------------------------------
// used externally by udp/tcp
PCLIENT AddActive( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( "Add Active client %p(%d)", pClient, pClient->Socket );
		DumpLists();
#endif
		pClient->dwFlags |= CF_ACTIVE;
		pClient->LastEvent = GetTickCount();
		pClient->me = &globalNetworkData.ActiveClients;
		if( ( pClient->next = globalNetworkData.ActiveClients ) )
			globalNetworkData.ActiveClients->me = &pClient->next;
		globalNetworkData.ActiveClients = pClient;
	}
	return pClient;
}

LOGICAL sack_network_is_active( PCLIENT pc ) {
	if( pc && ( pc->dwFlags & ( CF_ACTIVE )) && !( pc->dwFlags & (CF_CLOSED)) ) return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------------

static PCLIENT AddClosed( PCLIENT pClient )
{
	if( pClient )
	{
#ifdef LOG_CLIENT_LISTS
		lprintf( "Add Closed client %p(%d)", pClient, pClient->Socket );
		DumpLists();
#endif
		pClient->dwFlags |= CF_CLOSED;
		pClient->LastEvent = GetTickCount();
		pClient->me = &globalNetworkData.ClosedClients;
		if( ( pClient->next = globalNetworkData.ClosedClients ) )
			globalNetworkData.ClosedClients->me = &pClient->next;
		globalNetworkData.ClosedClients = pClient;
	}
	return pClient;
}

//----------------------------------------------------------------------------

static void ClearClient( PCLIENT pc DBG_PASS )
{
	uintptr_t* pbtemp;
	PCLIENT next;
	PCLIENT *me;
	CRITICALSECTION csr;
	CRITICALSECTION csw;
	// keep the closing flag until it's really been closed. (getfreeclient will try to nab it)
	enum NetworkConnectionFlags  dwFlags = pc->dwFlags & (CF_STATEFLAGS | CF_CLOSING | CF_CONNECT_WAITING | CF_CONNECT_CLOSED);
#ifdef VERBOSE_DEBUG
	lprintf( "CLEAR CLIENT!" );
#endif
	me = pc->me;
	next = pc->next;
	pbtemp = pc->lpUserData;
	csr = pc->csLockRead;
	csw = pc->csLockWrite;

	ReleaseAddress( pc->saClient );
	ReleaseAddress( pc->saSource );
#if _WIN32
	if( pc->event ) {
		if( globalNetworkData.flags.bLogNotices )
			_lprintf(DBG_RELAY)( "Closing network event:%p  %p", pc, pc->event );
		WSACloseEvent( pc->event );
	}
#endif
	// sets socket to 0 - so it's not quite == INVALID_SOCKET
#ifdef LOG_NETWORK_EVENT_THREAD
	if( globalNetworkData.flags.bLogNotices )
		_lprintf(DBG_RELAY)( "Clear Client %p  %08x   %08x", pc, pc->dwFlags, dwFlags );
#endif
	MemSet( pc, 0, sizeof( CLIENT ) ); // clear all information...

	// Socket is now 0; which for linux is a valid handle... which is what I get for events...
	pc->csLockRead = csr;
	pc->csLockWrite = csw;
	pc->lpUserData = pbtemp;
	if( pc->lpUserData )
		MemSet( pc->lpUserData, 0, globalNetworkData.nUserData * sizeof( uintptr_t ) );
	pc->next = next;
	pc->me = me;
	pc->dwFlags = dwFlags;
}

//----------------------------------------------------------------------------
#if 0
static void NetworkGlobalLock( DBG_VOIDPASS ) {
	LOGICAL locked = FALSE;
	do {
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( TryEnterCriticalSection( &globalNetworkData.csNetwork ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
#ifdef LOG_NETWORK_LOCKING
			_lprintf( DBG_RELAY )("Failed enter global? %lld", globalNetworkData.csNetwork.dwThreadID );
#endif
			Relinquish();
		}
		else
			locked = TRUE;
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )("Got global lock");
#endif
	} while( !locked );
}
#endif
LOGICAL TryNetworkGlobalLock( DBG_VOIDPASS ) {
	LOGICAL locked = FALSE;
#ifdef USE_NATIVE_CRITICAL_SECTION
	if( TryEnterCriticalSection( &globalNetworkData.csNetwork ) < 1 )
#else
	if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
	{
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )("Failed enter global? %lld", globalNetworkData.csNetwork.dwThreadID);
#endif
		return FALSE;
	}
	else
		locked = TRUE;
#ifdef LOG_NETWORK_LOCKING
	_lprintf( DBG_RELAY )("Got global lock");
#endif
	return TRUE;
}


void NetworkGloalUnlock( DBG_VOIDPASS ) {
#ifdef USE_NATIVE_CRITICAL_SECTION
	LeaveCriticalSection( &globalNetworkData.csNetwork );
#else
	LeaveCriticalSecEx( &globalNetworkData.csNetwork DBG_RELAY );
#endif
}

//----------------------------------------------------------------------------

void TerminateClosedClientEx( PCLIENT pc DBG_PASS )
{
#ifdef VERBOSE_DEBUG
	_lprintf(DBG_RELAY)( "terminate client %p ", pc );
#endif
	if( !pc )
		return;
	if( pc->dwFlags & CF_TOCLOSE ) {
#ifdef VERBOSE_DEBUG
		lprintf( "WAIT FOR CLOSE LATER... %x", pc->dwFlags );
#endif
		//return;
	}
	if( pc->dwFlags & CF_CLOSED )
	{
		PendingBuffer * lpNext;
		EnterCriticalSec( &globalNetworkData.csNetwork );
		RemoveThreadEvent( pc );
#ifdef VERBOSE_DEBUG
		lprintf( "REMOVED EVENT...." );
#endif

		//lprintf( "Terminating closed client..." );
		if( IsValid( pc->Socket ) )
		{
#ifdef VERBOSE_DEBUG
			lprintf( "close soekcet." );
#endif
#if !defined( SHUT_WR ) && defined( _WIN32 )
#  define SHUT_WR SD_SEND
#endif
			shutdown( pc->Socket, SHUT_WR );
#if defined( _WIN32 )
#undef SHUT_WR
#endif
			//lprintf( "Win32:ShutdownWR+closesocket %p", pc );
			closesocket( pc->Socket );
			while( pc->lpFirstPending )
			{
				lpNext = pc->lpFirstPending -> lpNext;

				if( pc->lpFirstPending->s.bDynBuffer )
					Deallocate( POINTER, pc->lpFirstPending->buffer.p );

				if( pc->lpFirstPending != &pc->FirstWritePending )
				{
#ifdef LOG_PENDING
					lprintf( "Data queued...Deleting in remove." );
#endif
					Deallocate( PendingBuffer*, pc->lpFirstPending);
				}
				else
				{
#ifdef LOG_PENDING
					lprintf( "Normal send queued...Deleting in remove." );
#endif
				}
				if (!lpNext)
					pc->lpLastPending = NULL;
				pc->lpFirstPending = lpNext;
			}
		}
		ClearClient( pc DBG_RELAY );
		// this should move from globalNetworkData.close to globalNetworkData.available.
		AddAvailable( GrabClient( pc ) );
		pc->dwFlags &= ~CF_CLOSING; // it's no longer closing.  (was set during the course of closure)
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		//NetworkUnlock( pc );
	}
#ifdef LOG_PENDING
	else
		lprintf( "Client's state was not CLOSED..." );
#endif
}

//----------------------------------------------------------------------------

void SetNetworkWriteComplete( PCLIENT pClient
                            , cWriteComplete WriteComplete )
{
	if( pClient && IsValid( pClient->Socket ) )
	{
		pClient->write.WriteComplete = WriteComplete;
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkWriteComplete( PCLIENT pClient
                               , cppWriteComplete WriteComplete
                               , uintptr_t psv)
{
	if( pClient && IsValid( pClient->Socket ) )
	{
		pClient->write.CPPWriteComplete = WriteComplete;
		pClient->psvWrite = psv;
		pClient->dwFlags |= CF_CPPWRITE;
	}
}

//----------------------------------------------------------------------------

void SetNetworkCloseCallback( PCLIENT pClient
                            , cCloseCallback CloseCallback )
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->close.CloseCallback = CloseCallback;
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkCloseCallback( PCLIENT pClient
                               , cppCloseCallback CloseCallback
                               , uintptr_t psv)
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->close.CPPCloseCallback = CloseCallback;
		pClient->psvClose = psv;
		pClient->dwFlags |= CF_CPPCLOSE;
	}
}

//----------------------------------------------------------------------------

void SetNetworkReadComplete( PCLIENT pClient
                           , cReadComplete pReadComplete )
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.ReadComplete = pReadComplete;
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkReadComplete( PCLIENT pClient
                              , cppReadComplete pReadComplete
                              , uintptr_t psv)
{
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.CPPReadComplete = pReadComplete;
		pClient->psvRead = psv;
		pClient->dwFlags |= CF_CPPREAD;
	}
}

//----------------------------------------------------------------------------

void SetNetworkErrorCallback( PCLIENT pc, cErrorCallback callback, uintptr_t psvUser ) {
	if( pc ) {
		pc->errorCallback = callback;
		pc->psvErrorCallback = psvUser;
	}
}

//----------------------------------------------------------------------------

void TriggerNetworkErrorCallback( PCLIENT pc, enum SackNetworkErrorIdentifier error ) {
	if( pc && pc->errorCallback )
		pc->errorCallback( pc->psvErrorCallback, pc, error );
}

//----------------------------------------------------------------------------

uintptr_t CPROC NetworkThreadProc( PTHREAD thread )
{
	struct peer_thread_info *peer_thread = (struct peer_thread_info*)GetThreadParam( thread );
	struct peer_thread_info this_thread;
	// and when unloading should remove these timers.
	if( !peer_thread )
	{
#ifdef _WIN32
		extern void CPROC NetworkPauseTimer( uintptr_t psv );
		globalNetworkData.uNetworkPauseTimer = AddTimerEx( 1, 1000, NetworkPauseTimer, 0 );
		if( !globalNetworkData.client_schedule )
			globalNetworkData.client_schedule = CreateLinkQueue();
#endif
#ifdef __LINUX__
		globalNetworkData.flags.bNetworkReady = TRUE;
		globalNetworkData.flags.bThreadInitOkay = TRUE;
#endif
	}
	memset( &this_thread, 0, sizeof( this_thread ) );

	this_thread.monitor_list = NULL;
#ifdef _WIN32
	this_thread.event_list = CreateDataList( sizeof( WSAEVENT ) );
	this_thread.hThread = WSACreateEvent();
	// setup this as if it was cleared already.
	this_thread.nEvents = 1;
	SetLink( &this_thread.monitor_list, 0, (POINTER)1 ); // has to be a non zero value.  monitor is not referenced for wait event 0
	SetDataItem( &this_thread.event_list, 0, &this_thread.hThread );
#else
	// have to fall back to poll() for __MAC__ builds. (probably client only)
	//this_thread.event_list = CreateDataList( sizeof( struct pollfd ) );

#ifdef __EMSCRIPTEN__
#else
#  ifdef __LINUX__
#    ifdef __MAC__
	this_thread.kqueue = kqueue();
#    else
#      ifdef __ANDROID__
	this_thread.epoll_fd = epoll_create( 128 ); // close on exec (no inherit)
#      else
	this_thread.epoll_fd = epoll_create1( EPOLL_CLOEXEC ); // close on exec (no inherit)
#      endif
#    endif
	{
#    ifdef __MAC__
#      ifdef __64__
		struct kevent64_s ev;
		this_thread.kevents = CreateDataList( sizeof( ev ) );
#        ifdef USE_PIPE_SEMS
		EV_SET64( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uint64_t)1, NULL, NULL );
#        endif
		kevent64( this_thread.kqueue, &ev, 1, 0, 0, 0, 0 );
#      else
		struct kevent ev;
		this_thread.kevents = CreateDataList( sizeof( ev ) );
#        ifdef USE_PIPE_SEMS
		EV_SET( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uintptr_t)1 );
#        endif
		kevent( this_thread.kqueue, &ev, 1, 0, 0, 0 );
#      endif
#    else
		//struct epoll_event ev;
		//ev.data.ptr = (void*)1;
		//ev.events = EPOLLIN;
		//epoll_ctl( this_thread.epoll_fd, EPOLL_CTL_ADD, GetThreadSleeper( thread ), &ev );
#    endif
	}
#  endif
#endif

#endif
	this_thread.parent_peer = peer_thread;
	this_thread.child_peer = NULL;
	this_thread.thread = thread;

	if( peer_thread )
		peer_thread->child_peer = &this_thread;
	else {
		globalNetworkData.root_thread = &this_thread;
#ifdef _WIN32
		globalNetworkData.hMonitorThreadControlEvent = this_thread.hThread;
#endif
	}

	while( !globalNetworkData.pThreads ) // creator won't pass until bThreadInitComplete is set.
		Relinquish();

	globalNetworkData.flags.bThreadInitOkay = TRUE;
	globalNetworkData.flags.bThreadInitComplete = TRUE;
	while( !globalNetworkData.bQuit )
	{
		ProcessNetworkMessages( &this_thread, 0 );
	}

	xlprintf( 2100 )("Enter global network on shutdown... (thread exiting)");

	EnterCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( "NetworkThread(exit) in global" );
#  endif
	if( !this_thread.parent_peer )
	{
		if( ( globalNetworkData.root_thread = this_thread.child_peer ) )
			this_thread.child_peer->parent_peer = NULL;
	}
	else
	{
		if( ( this_thread.parent_peer->child_peer = this_thread.child_peer ) )
			this_thread.child_peer->parent_peer = this_thread.parent_peer;
	}
	// this used to be done in the WM_DESTROY
	DeleteLink( (PLIST*)&globalNetworkData.pThreads, thread );

	globalNetworkData.flags.bThreadExit = TRUE;

	xlprintf( 2100 )("Shut down network thread.");

	globalNetworkData.flags.bThreadInitComplete = FALSE;
	globalNetworkData.flags.bNetworkReady = FALSE;
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#  ifdef LOG_NETWORK_LOCKING
	lprintf( "NetworkThread(exit) left global" );
#  endif
	//DeleteCriticalSec( &globalNetworkData.csNetwork );	 //spv:980303
	return 0;
}


//----------------------------------------------------------------------------

struct peer_thread_info *IsNetworkThread( void )
{
	struct peer_thread_info *thread;
	PTHREAD this_thread = MakeThread();
	for( thread = globalNetworkData.root_thread; thread; thread = thread->child_peer )
	{
		if( thread->thread == this_thread )
			return thread;
	}
	return NULL;
}

//----------------------------------------------------------------------------

int NetworkQuit(void)
{
	if( !global_network_data )
		return 0;

#if 0
	if( globalNetworkData.uPendingTimer )
	{
		RemoveTimer( globalNetworkData.uPendingTimer );
		globalNetworkData.uPendingTimer = 0;
	}
#endif
	//while( globalNetworkData.ActiveClients )
	{
#ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( "NetworkQuit - Remove active client %p", globalNetworkData.ActiveClients );
#endif
		//InternalRemoveClientEx( globalNetworkData.ActiveClients, TRUE, FALSE );
	}
	globalNetworkData.bQuit = TRUE;
	{
		PTHREAD thread;
		INDEX idx;
#ifdef USE_WSA_EVENTS
		PLIST wakeEvents = NULL;
		struct peer_thread_info *peer_thread;
		WSAEVENT hThread;
		peer_thread = globalNetworkData.root_thread;
		globalNetworkData.root_thread = NULL;
      MakeThread();
		for( ; peer_thread; peer_thread = peer_thread->child_peer ) {
			AddLink( &wakeEvents, peer_thread->hThread );
		}
		LIST_FORALL( wakeEvents, idx, WSAEVENT, hThread )
			WSASetEvent( hThread );
#endif
		LIST_FORALL( globalNetworkData.pThreads, idx, PTHREAD, thread ) {
			WakeThread( thread );
		}
	}
#ifdef _WIN32
#  ifdef LOG_NOTICES
	if( globalNetworkData.flags.bLogNotices )
		lprintf( "SET GLOBAL EVENT (trigger quit)" );
#  endif
	WSASetEvent( globalNetworkData.hMonitorThreadControlEvent );
#else
#  ifndef __LINUX__
	if( IsWindow( globalNetworkData.ghWndNetwork ) )
	{
		// okay forget this... at exit, cannot guarantee that
		// any other thread other than myself has any rights to do anything.
#    ifdef LOG_NOTICES
		if( globalNetworkData.flags.bLogNotices )
			lprintf( "Post SOCKMSG_CLOSE" );
#    endif
		PostMessage( globalNetworkData.ghWndNetwork, SOCKMSG_CLOSE, 0, 0 );
		// also remove PCLIENT clients, and all client->pUserData allocated...
	}
#  else
	//while( globalNetworkData.pThread )
	//	Sleep(0);
	// should kill Our thread.... and close any active ockets...
#  endif
#endif
	globalNetworkData.flags.bThreadInitComplete = FALSE;
	//RemoveIdleProc( ProcessNetworkMessages );
	if( globalNetworkData.pThreads )
	{
		uint32_t started = timeGetTime() + 500;
		Relinquish(); // allow network thread to gracefully exit
		while( globalNetworkData.flags.bNetworkReady && timeGetTime() < started )
			IdleFor( 20 );
		if( globalNetworkData.flags.bNetworkReady )
		{
#ifdef LOG_STARTUP_SHUTDOWN
			lprintf( "Network was locked up?  Failed to allow network to exit in half a second (500ms)" );
#endif
		}
	}
	globalNetworkData.root_thread = NULL;
	return -1;
}

ATEXIT( NetworkShutdown )
{
	NetworkQuit();
}

//----------------------------------------------------------------------------

LOGICAL NetworkAlive( void )
{
	return !globalNetworkData.flags.bThreadExit;
}

//----------------------------------------------------------------------------

static void AddClients( void ) {
	PCLIENT_SLAB pClientSlab;

	// protect all structures.
	EnterCriticalSec( &globalNetworkData.csNetwork );

	{
		size_t n;
		//Log1( "Creating %d Client Resources", MAX_NETCLIENTS );
		pClientSlab = NewPlus( CLIENT_SLAB, (MAX_NETCLIENTS - 1 )* sizeof( CLIENT ) );
		pClientSlab->pUserData = NewArray( uintptr_t, MAX_NETCLIENTS * globalNetworkData.nUserData );
		MemSet( pClientSlab->client, 0, (MAX_NETCLIENTS) * sizeof( CLIENT ) ); // can't clear the lpUserData Address!!!
		MemSet( pClientSlab->pUserData, 0, (MAX_NETCLIENTS) * globalNetworkData.nUserData * sizeof( uintptr_t ) );
		pClientSlab->count = MAX_NETCLIENTS;

		for( n = 0; n < pClientSlab->count; n++ )
		{
			pClientSlab->client[n].Socket = INVALID_SOCKET; // unused sockets on all clients.
			pClientSlab->client[n].lpUserData = pClientSlab->pUserData + (n * globalNetworkData.nUserData);
			InitializeCriticalSec( &pClientSlab->client[n].csLockRead );
			InitializeCriticalSec( &pClientSlab->client[n].csLockWrite );
			AddAvailable( pClientSlab->client + n );
		}
		AddLink( &globalNetworkData.ClientSlabs, pClientSlab );
	}

	LeaveCriticalSec( &globalNetworkData.csNetwork );
}

//----------------------------------------------------------------------------


static void ReallocClients( uint32_t wClients, int nUserData )
{

	// protect all structures.
	EnterCriticalSec( &globalNetworkData.csNetwork );
	if( !wClients )
		wClients = 32;  // default 32 clients per slab...
	if( !nUserData )
		nUserData = 4;  // defualt to 4 pointer words per socket; most applications only use 1.

	// keep the max of specified data..
	if( nUserData < globalNetworkData.nUserData )
		nUserData = globalNetworkData.nUserData;

	// keep the max of specified connections..
	if( wClients < MAX_NETCLIENTS )
		wClients = MAX_NETCLIENTS;

	// if the client slab size increases, new slabs will be the new size; old slabs will still be the old size.

	if( nUserData > globalNetworkData.nUserData ) // have to reallocate the user data for all sockets
	{
		INDEX idx;
		PCLIENT_SLAB slab;
		uint32_t n;
		// for all existing client slabs...
		LIST_FORALL( globalNetworkData.ClientSlabs, idx, PCLIENT_SLAB, slab )
		{
			uintptr_t* pUserData;
			pUserData = NewArray( uintptr_t, nUserData * sizeof( uintptr_t ) * slab->count );// slab->pUserData;
			for( n = 0; n < slab->count; n++ )
			{
				if( slab->client[n].lpUserData )
					MemCpy( (char*)pUserData + (n * (nUserData * sizeof( uintptr_t )))
					      , slab->client[n].lpUserData
					      , globalNetworkData.nUserData * sizeof( uintptr_t ) );
				slab->client[n].lpUserData = pUserData + (n * nUserData);
			}
			Deallocate( uintptr_t*, slab->pUserData );
			slab->pUserData = pUserData;
		}
	}
	MAX_NETCLIENTS = wClients;
	globalNetworkData.nUserData = nUserData;
	LeaveCriticalSec( &globalNetworkData.csNetwork );
}

#ifdef __LINUX__
NETWORK_PROC( LOGICAL, NetworkWait )(POINTER unused,uint32_t wClients,int wUserData)
#else
NETWORK_PROC( LOGICAL, NetworkWait )(HWND hWndNotify,uint32_t wClients,int wUserData)
#endif
{
	// want to start the thead; clear quit.
	if( !global_network_data )
		LowLevelNetworkInit();

	// allow network to restart with new NetworkWait after NetworkQuit
	globalNetworkData.bQuit = FALSE;

	ReallocClients( wClients, wUserData );

	//-------------------------
	// please be mindful of the following data declared immediate...
	if( GetLinkCount( globalNetworkData.pThreads ) )
	{
		//xlprintf(200)( "Threads already active..." );
		// might do something... might not...
		return TRUE; // network thread active, do not realloc
	}

	AddLink( (PLIST*)&globalNetworkData.pThreads, ThreadTo( NetworkThreadProc, (uintptr_t)/*peer_thread==*/NULL ) );
	globalNetworkData.nPeers++;
	AddIdleProc( IdleProcessNetworkMessages, 1 );
	//lprintf( "Network Initialize...");
	//lprintf( "Create network thread." );
	while( !globalNetworkData.flags.bThreadInitComplete )
	{
		Relinquish();
	}
	if( !globalNetworkData.flags.bThreadInitOkay )
	{
		lprintf( "Abort network, init is NOT ok." );
		return FALSE;
	}
	while( !globalNetworkData.flags.bNetworkReady )
		Relinquish(); // wait for actual network...
	{
		char buffer[256];
		if( gethostname( buffer, sizeof( buffer ) ) == 0)
			globalNetworkData.system_name = DupCStr( buffer );
	}
	LoadNetworkAddresses();
	return globalNetworkData.flags.bThreadInitOkay;  // return status of thread initialization
}

//----------------------------------------------------------------------------

PCLIENT GetFreeNetworkClientEx( DBG_VOIDPASS )
{
	PCLIENT pClient = NULL;
get_client:
	EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( "GetFreeNetworkClient in global" );
#endif
	if( !globalNetworkData.AvailableClients ) // if there's none available, add some with current config
		AddClients();
	for( pClient = globalNetworkData.AvailableClients; pClient; pClient = pClient->next )
		if( !( pClient->dwFlags & CF_CLOSING ) )
			break;
	if( pClient )
	{
		int d;
		// oterhwise we'll deadlock the closing client...
		// an opening condition has global lock (above)
		// and a closing socket will want the global lock before it's done.
		pClient = GrabClient( pClient );

#ifdef USE_NATIVE_CRITICAL_SECTION
		d = EnterCriticalSecNoWait( &pClient->csLockRead, NULL );
#else
		d = EnterCriticalSecNoWaitEx( &pClient->csLockRead, NULL DBG_RELAY );
#endif
		if( d < 1 ) {
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			goto get_client;
		}

#ifdef USE_NATIVE_CRITICAL_SECTION
		d = EnterCriticalSecNoWait( &pClient->csLockWrite, NULL );
#else
		d = EnterCriticalSecNoWaitEx( &pClient->csLockWrite, NULL DBG_RELAY );
#endif
		if( d < 1 ) {
			LeaveCriticalSec( &pClient->csLockRead );
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			goto get_client;
		}

		if( pClient->dwFlags & ( CF_STATEFLAGS & (~CF_AVAILABLE)) )
			DebugBreak();
		ClearClient( pClient DBG_SRC ); // clear client is redundant here... but saves the critical section now
		//Log1( "New network client %p", client );
	}
	else
	{
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
		lprintf( "GetFreeNetworkClient left global" );
#endif
		Relinquish();
		if( globalNetworkData.AvailableClients )
		{
			lprintf( "there were clients available... just in a closing state..." );
			goto get_client;
		}
		lprintf( "No unused network clients are available." );
		return NULL;
	}
	LeaveCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_NETWORK_LOCKING
	lprintf( "GetFreeNetworkClient left global" );
#endif
	return pClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, SetNetworkLong )(PCLIENT lpClient, int nLong, uintptr_t dwValue)
{
	if( lpClient && ( nLong < globalNetworkData.nUserData ) )
	{
		lpClient->lpUserData[nLong] = dwValue;
	}
	return;
}

//----------------------------------------------------------------------------

NETWORK_PROC( uintptr_t, GetNetworkLong )(PCLIENT lpClient,int nLong)
{
	if( !lpClient )
	{
		return (uintptr_t)-1;
	}
	if( nLong < 0 )
	{
		switch( nLong )
		{
		case GNL_IP:  // IP of destination
			if( lpClient->saClient )
				return *(uint32_t*)(lpClient->saClient->sa_data+2);
			break;
		case GNL_REMOTE_ADDRESS:  // IP of destination
  			return (uintptr_t)lpClient->saClient;
			break;
		case GNL_LOCAL_ADDRESS:  // IP of local side
  			return (uintptr_t)lpClient->saSource;
			break;
		case GNL_PORT:  // port of server...  STUPID PATCH?!  maybe...
			if( lpClient->saClient )
				return ntohs( *(uint16_t*)(lpClient->saClient->sa_data) );
			break;
		case GNL_MYPORT:  // port of server...  STUPID PATCH?!  maybe...
			if( lpClient->saSource )
				return ntohs( *(uint16_t*)(lpClient->saSource->sa_data) );
			break;
		case GNL_MYIP: // IP of myself (after connect?)
			if( lpClient->saSource )
				return *(uint32_t*)(lpClient->saSource->sa_data+2);
			break;

			//TODO if less than zero return a (high/low)portion of the  hardware address (MAC).
		}
	}
	else if( nLong < globalNetworkData.nUserData )
	{
		return lpClient->lpUserData[nLong];
	}

	return (uintptr_t)-1;   //spv:980303
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, NetworkLockEx)( PCLIENT lpClient, int readWrite DBG_PASS )
{
	if( lpClient )
	{
		//if( lpClient->flags.bWriteOnUnlock ) {
		//	lprintf( "Still need to do that write..." );
		//}
		//lpClient->dwFlags |= CF_WANTS_GLOBAL_LOCK;
		//_lprintf(DBG_RELAY)( "Lock %p", lpClient );
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( EnterCriticalSecNoWait( &globalNetworkData.csNetwork, NULL ) < 1 )
#else
		if( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
			//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
#ifdef LOG_NETWORK_LOCKING
			_lprintf(DBG_RELAY)( "Failed enter global? %llx", globalNetworkData.csNetwork.dwThreadID  );
#endif
			Relinquish();
			return NULL;
			//DebugBreak();
		}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Got global lock %p %d", lpClient, readWrite );
#endif

		//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
#ifdef USE_NATIVE_CRITICAL_SECTION
		if( !EnterCriticalSecNoWait( (readWrite? &lpClient->csLockRead:&lpClient->csLockWrite), NULL ) )
#else
		if( EnterCriticalSecNoWaitEx( ( readWrite ?&lpClient->csLockRead : &lpClient->csLockWrite ), NULL DBG_RELAY ) < 1 )
#endif
		{
			// unlock the global section for a moment..
			// client may be requiring both local and global locks (already has local lock)

#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( &globalNetworkData.csNetwork);
#else
			LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
			//lprintf( "Idle... socket lock failed, had global though..." );
			Relinquish();
			return NULL;
			//goto start_lock;
		}
		//EnterCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#else
		LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
		if( !(lpClient->dwFlags & (CF_ACTIVE|CF_CLOSED) ) )
		{
			// change to inactive status by the time we got here...
#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#else
			LeaveCriticalSecEx( readWrite?&lpClient->csLockRead:&lpClient->csLockWrite DBG_RELAY );
#endif
#ifdef LOG_NETWORK_LOCKING
			_lprintf( DBG_RELAY )( "Failed lock: %p  %08x %08x inactive, cannot lock.", lpClient, lpClient->dwFlags, CF_ACTIVE );
#endif
			// this client is not available for client use!
			return NULL;
		}
	}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Got private lock %p %d", lpClient, readWrite );
#endif
	return lpClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, NetworkUnlockEx)( PCLIENT lpClient, int readWrite DBG_PASS )
{
	//_lprintf(DBG_RELAY)( "Unlock %p", lpClient );
	// simple unlock.
	if( lpClient )
	{
		if( !readWrite ) // is write and not read
		{
			if( lpClient->flags.bWriteOnUnlock ) {
				lpClient->flags.bWriteOnUnlock = 0;
				//lprintf( "Caught unlock..." );
				TCPWrite( lpClient );
			}
		}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Leave private lock %p %d", lpClient, readWrite );
#endif
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#else
		LeaveCriticalSecEx( readWrite?&lpClient->csLockRead:&lpClient->csLockWrite DBG_RELAY );
#endif
	}
}

//----------------------------------------------------------------------------

#undef NetworkLock
#undef NetworkUnlock
NETWORK_PROC( PCLIENT, NetworkLock )(PCLIENT lpClient, int readWrite)
{
	return NetworkLockEx( lpClient, readWrite DBG_SRC );
}

NETWORK_PROC( void, NetworkUnlock )(PCLIENT lpClient, int readWrite)
{
	NetworkUnlockEx( lpClient, readWrite DBG_SRC );
}

//----------------------------------------------------------------------------

void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
#ifdef LOG_SOCKET_CREATION
	_lprintf( DBG_RELAY )( "InternalRemoveClient Removing this client %p (%d)", lpClient, lpClient->Socket );
#endif
	if( lpClient && IsValid(lpClient->Socket) )
	{
		if( !bLinger )
		{
#ifdef LOG_DEBUG_CLOSING
			lprintf( "Setting quick close?!" );
#endif
			if( 0 )
			{
				int nAllowReuse = 1;
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_REUSEADDR,
									(char*)&nAllowReuse, sizeof(nAllowReuse)) <0 )
				{
					//cerr << "NFMSim:setHost:ERROR: could not set socket to reuse addr." << endl;
				}

				/*
				// missing symbol in windows?
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_REUSEPORT,
									(char*)&nAllowReuse, sizeof(nAllowReuse)) <0 )
				{
					//cerr << "NFMSim:setHost:ERROR: could not set socket to reuse port." << endl;
					}
				*/
			}
			if( 1 )
			{
				// www.serverframework.com/asynchronousevents/2011/01/time-wait-and-its-design-implications-for-protocols-and-scalable-servers.html
				//  the idea is to NEVER do this; but I had to do this for lots of parallel connections that were short lived...
				// windows registry http://technet.microsoft.com/en-us/library/cc938217.aspx 240 seconds time_wait timeout
				struct linger lingerSet;
				lingerSet.l_onoff = 1; // on , with no time = off.
				lingerSet.l_linger = 0; // 0 timeout sends reset.
										 // set server to allow reuse of socket port
            //lprintf( "Set no linger" );
				if (setsockopt(lpClient->Socket, SOL_SOCKET, SO_LINGER,
									(char*)&lingerSet, sizeof(lingerSet)) <0 )
				{
					lprintf( "error setting no linger in close." );
					//cerr << "NFMSim:setHost:ERROR: could not set socket to linger." << endl;
				}
			}
		}
		else {
			struct linger lingerSet;
			// linger ON causes delay on close... otherwise close returns immediately
			lingerSet.l_onoff = 1; // on , with no time = off.
			lingerSet.l_linger = 2;
			// set server to allow reuse of socket port
			if( setsockopt( lpClient->Socket, SOL_SOCKET, SO_LINGER,
				(char*)&lingerSet, sizeof( lingerSet ) ) <0 )
			{
				lprintf( "error setting 2 second linger in close." );
			}
		}

		if( !(lpClient->dwFlags & CF_ACTIVE) )
		{
			if( lpClient->dwFlags & CF_AVAILABLE )
			{
				// already available somehow.
			}
			// this is probably true, we've definatly already moved it from
			// active list to clsoed list.
			else if( !(lpClient->dwFlags & CF_CLOSED) )
			{
#ifdef LOG_DEBUG_CLOSING
				lprintf( "Client was NOT already closed?!?!" );
#endif
				AddClosed( GrabClient( lpClient ) );
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( "Client's state is CLOSED" );
#endif
			return;
		}
		if( lpClient->lpFirstPending || ( lpClient->dwFlags & CF_WRITEPENDING ) ) {
#ifdef LOG_DEBUG_CLOSING
			lprintf( "CLOSE WHILE WAITING FOR WRITE TO FINISH..." );
#endif
			//lpClient->dwFlags |= CF_TOCLOSE;
			//return;
			// continue on; otherwise the close event gets lost...
		}
		{
			int notLocked = TRUE;
			do {
				if( !NetworkLockEx( lpClient, 0 DBG_SRC ) )
				{
					if( !(lpClient->dwFlags & CF_ACTIVE ) ) // if it's already been closed
					{
						return;
					}
					Relinquish();
					continue;
				}
				if( !NetworkLockEx( lpClient, 1 DBG_SRC ) )
				{
					NetworkUnlock( lpClient, 0 );
					if( !(lpClient->dwFlags & CF_ACTIVE) )  // if it's already been closed
					{
						return;
					}
					Relinquish();
					continue;
				}
				LeaveCriticalSec( &globalNetworkData.csNetwork );
				notLocked = FALSE;
				EnterCriticalSec( &globalNetworkData.csNetwork );
			} while( notLocked );
		}

		// allow application a chance to clean it's references
		// to this structure before closing and cleaning it.

		if( !bBlockNotify )
		{
			lpClient->dwFlags |= CF_CONNECT_CLOSED;
			if( lpClient->pWaiting )
			{
				WakeThread( lpClient->pWaiting );
				while( lpClient->dwFlags & CF_CONNECT_WAITING )
					Relinquish();
			}
			lpClient->dwFlags &= ~CF_CONNECT_CLOSED;
			if( !(lpClient->dwFlags & CF_CLOSING) ) // prevent multiple notifications...
			{
#ifdef LOG_DEBUG_CLOSING
				lprintf( "Marked closing first, and dispatching callback?" );
#endif
				lpClient->dwFlags |= CF_CLOSING;
				if( lpClient->close.CloseCallback )
				{
					// during thisi if it wants a lock... and the application
					// is dispatching like
					if( lpClient->dwFlags & CF_CPPCLOSE )
						lpClient->close.CPPCloseCallback( lpClient->psvClose );
					else
						lpClient->close.CloseCallback( lpClient );

					lpClient->close.CloseCallback = NULL;
				}
#ifdef LOG_DEBUG_CLOSING
				else
					lprintf( "no close callback!? (or duplicate close?)" );
#endif
				// leave the flag closing set... we'll use that later
				// to avoid the double-lock;
				//lpClient->dwFlags &= ~CF_CLOSING;
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( "socket was already ispatched callback?" );
#endif
		}
		else
		{
#ifdef LOG_DEBUG_CLOSING
			lprintf( "blocknotify on close..." );
#endif
		}
		EnterCriticalSec( &globalNetworkData.csNetwork );
#ifdef LOG_DEBUG_CLOSING
		lprintf( "Adding current client to closed clients." );
#endif
		AddClosed( GrabClient( lpClient ) );
#ifdef LOG_DEBUG_CLOSING
		lprintf( "Leaving client critical section" );
#endif
		//lprintf( "Leaving network critical section" );
		LeaveCriticalSec( &globalNetworkData.csNetwork );
		NetworkUnlockEx( lpClient, 0 DBG_SRC );
		NetworkUnlockEx( lpClient, 1 DBG_SRC );
	}
#ifdef LOG_DEBUG_CLOSING
	else
	{
		lprintf( "No Client, or socket already closed?" );
	}
#endif
}

void RemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
#ifdef _WIN32
#  define SHUT_WR SD_SEND
#endif
	if( !lpClient ) return;

	if( !( lpClient->dwFlags & CF_UDP ) 
		&& ( lpClient->dwFlags & ( CF_CONNECTED ) ) ) {
		shutdown( lpClient->Socket, SHUT_WR );
	} else

	{
		int n = 0;
		// UDP still needs to be done this way...
				//
		//lprintf( "This will end up resetting the socket?" );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientExx( lpClient, bBlockNotify, bLinger DBG_RELAY );
		if( NetworkLock( lpClient, 0 ) && ((n=1),NetworkLock( lpClient, 1 )) ) {
			TerminateClosedClient( lpClient );
			NetworkUnlock( lpClient, 0 );
			NetworkUnlock( lpClient, 1 );
		}
		else if( n ) {
			NetworkUnlock( lpClient, 0 );
			lpClient->dwFlags |= CF_TOCLOSE;
		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
	}
}

SACK_NETWORK_NAMESPACE_END
