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

//#define LOCK_GLOBAL_WHEN_LOCKING_CLIENT

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

/* PROBE: how often does RemoveClientExx have to skip TerminateClosedClient because a
 * channel was locked?  That is the case that leaves a client on ClosedClients with no
 * marker for the unlock path to find, and it is what the win32 timer sweep cleans up.
 * If this never fires under the existing tests, the sweep is uncovered by them. */
static volatile uint32_t strandedTerminates;


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
#  if defined( LOG_NOTICES ) || defined( LOG_WRITE_NOTICES )
		globalNetworkData.flags.bLogNotices = 1 || SACK_GetProfileIntEx( "SACK", "Network/Log Network Notifications", 0, TRUE );
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
	if( !globalNetworkData.ClientSlabs ) {
		InitializeCriticalSec( &globalNetworkData.csNetwork );
		InitializeCriticalSec( &globalNetworkData.csPeerChain );
	}
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

//----------------------------------------------------------------------------

static char flag_buf[16][1024];
static int flag_bufidx;

#define NETWORK_FLAG_DEF(a,b)   \
		if (pc->dwFlags&b) { if( f ) { out[0] = ' '; out++; f = 0; } strcpy( out, #b ); out += sizeof(#b)-1; f = 1; }


char *NetworkExpandFlags( PCLIENT pc ) {
	char *buf = flag_buf[flag_bufidx++];
	char *out = buf;
	uint32_t f = 0;
	if( flag_bufidx >= 16 ) flag_bufidx = 0;
	
		NETWORK_FLAG_DEF( 0, CF_UDP )
		NETWORK_FLAG_DEF( CF_UDP, CF_TCP )
		NETWORK_FLAG_DEF( CF_TCP, CF_LISTEN )
		NETWORK_FLAG_DEF( CF_LISTEN, CF_WRITEPENDING )
		NETWORK_FLAG_DEF( CF_WRITEPENDING, CF_READPENDING )
		NETWORK_FLAG_DEF( CF_READPENDING, CF_READREADY )
		NETWORK_FLAG_DEF( CF_READREADY, CF_READWAITING )
		NETWORK_FLAG_DEF( CF_READWAITING, CF_CONNECTED )
		NETWORK_FLAG_DEF( CF_CONNECTED, CF_CONNECTERROR )
		NETWORK_FLAG_DEF( CF_CONNECTERROR, CF_CONNECTING )
		NETWORK_FLAG_DEF( CF_CONNECTING, CF_CONNECT_WAITING )
		NETWORK_FLAG_DEF( CF_CONNECT_WAITING, CF_CONNECT_CLOSED )
		NETWORK_FLAG_DEF( CF_CONNECT_CLOSED, CF_TOCLOSE )
		NETWORK_FLAG_DEF( CF_TOCLOSE, CF_WANTCLOSE )
		NETWORK_FLAG_DEF( CF_WANTCLOSE, CF_CLOSING )
		NETWORK_FLAG_DEF( CF_CLOSING, CF_DRAINING )
		NETWORK_FLAG_DEF( CF_DRAINING, CF_CLOSED )
		NETWORK_FLAG_DEF( CF_CLOSED, CF_ACTIVE )
		NETWORK_FLAG_DEF( CF_ACTIVE, CF_AVAILABLE )
		NETWORK_FLAG_DEF( CF_AVAILABLE, CF_CPPCONNECT )
		NETWORK_FLAG_DEF( CF_CPPCONNECT, CF_CPPREAD )
		NETWORK_FLAG_DEF( CF_CPPREAD, CF_CPPCLOSE )
		NETWORK_FLAG_DEF( CF_CPPCLOSE, CF_CPPWRITE )
		NETWORK_FLAG_DEF( CF_CPPWRITE, CF_PROCESSING )
		NETWORK_FLAG_DEF( CF_PROCESSING, CF_WRITEREADY )

	return buf;
}

//----------------------------------------------------------------------------

PCLIENT GrabClientEx( PCLIENT pClient DBG_PASS )
#define GrabClient(pc) GrabClientEx( pc DBG_SRC )
{
	if( pClient )
	{
		ClearClientFlags( pClient, CF_STATEFLAGS );
		if( pClient->dwFlags & CF_AVAILABLE )
			lprintf( "Grabbed. %p  %08x", pClient, pClient->dwFlags );
		pClient->LastEvent = timeGetTime();
		if( ( (*pClient->me) = pClient->next ) )
			pClient->next->me = pClient->me;
	}
	return pClient;
}

//----------------------------------------------------------------------------

#ifdef DEBUG_CLIENT_LOCK_TRACE
// One atomic increment plus a few stores - cheap enough to leave the race intact.
void sack_dbg_traceClient( PCLIENT pc, int ev, int channel, uint32_t a, CTEXTSTR file, uint32_t line ) {
	struct client_lock_trace *t;
	uint32_t i;
	if( !pc ) return;
	i = (uint32_t)LockedIncrement( &pc->lockTraceIdx ) - 1;
	t = pc->lockTrace + ( i % CLIENT_LOCK_TRACE_DEPTH );
	t->file    = file;
	t->line    = line;
	t->thread  = GetThisThreadID();
	t->a       = a;
	t->ev      = (uint8_t)ev;
	t->channel = (uint8_t)( channel ? 1 : 0 );
}

// Dump one client's whole lock history, oldest first.  Capped globally so a storm
// of stuck clients cannot flood the log.
static uint32_t sack_dbg_dumps;
void sack_dbg_dumpClientLockTrace( PCLIENT pc, const char *why ) {
	uint32_t n, total, first;
	if( !pc ) return;
	if( sack_dbg_dumps++ > 12 ) return;
	total = pc->lockTraceIdx;
	first = ( total > CLIENT_LOCK_TRACE_DEPTH ) ? ( total - CLIENT_LOCK_TRACE_DEPTH ) : 0;
	fprintf( stderr, "LOCKTRACE %s pc=%p sock=%d flags=%08x ops=%u rd=%u(owner %llx) wr=%u(owner %llx)\n"
	       , why, (void*)pc, pc->Socket, pc->dwFlags, total
	       , pc->csLockRead.dwLocks, (unsigned long long)pc->csLockRead.dwThreadID
	       , pc->csLockWrite.dwLocks, (unsigned long long)pc->csLockWrite.dwThreadID );
	for( n = first; n < total; n++ ) {
		struct client_lock_trace *t = pc->lockTrace + ( n % CLIENT_LOCK_TRACE_DEPTH );
		static const char *evname[] = { "UNLOCK", "LOCK", "ADDNET", "CLEARNET", "EVIN", "EVSKIP" };
		const char *nm = ( t->ev < 6 ) ? evname[t->ev] : "?";
		if( t->ev <= CLTRACE_LOCK )
			fprintf( stderr, "   [%3u] %-8s ch%d ->%u  thr=%llx  %s(%u)\n"
			       , n, nm, t->channel, t->a, (unsigned long long)t->thread
			       , t->file ? t->file : "?", t->line );
		else
			fprintf( stderr, "   [%3u] %-8s inUseCount=%u  thr=%llx  %s(%u)\n"
			       , n, nm, t->a, (unsigned long long)t->thread
			       , t->file ? t->file : "?", t->line );
	}
}
#endif

static PCLIENT AddAvailable( PCLIENT pClient )
{
	if( pClient )
	{
		SetClientFlags( pClient, CF_AVAILABLE );
		pClient->LastEvent = timeGetTime();
		// Head insert, same as AddActive.  This used to walk to the tail so the pool
		// behaved LRU - that was a probe to expose ClearClient leakage by preventing
		// immediate reuse, not intended behaviour, and it is O(available) on every
		// recycle while holding csNetwork.  Worse at the slab pre-fill (AddClients
		// calls this 256 times in a row), which made pool growth O(n^2).
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
		SetClientFlags( pClient, CF_ACTIVE );
		pClient->LastEvent = timeGetTime();
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

uint32_t NetworkClientSerial( PCLIENT pc ) {
	if( pc ) return pc->serial;
	return 0;
}

// diagnostic accessors - where is outbound data being held?
LOGICAL NetworkClientHasPendingSend( PCLIENT pc ) {
	return pc && ( pc->lpFirstPending != NULL );
}

uint32_t NetworkClientFlags( PCLIENT pc ) {
	return pc ? (uint32_t)pc->dwFlags : 0;
}

// diagnostic: is the client's network event object signaled right now, and is
// the client attached to an event thread?  A signaled event on an attached,
// live client means the event thread is failing to dispatch it.
int NetworkClientEventState( PCLIENT pc ) {
	int state = 0;
#ifdef _WIN32
	if( !pc ) return 0;
	if( pc->this_thread ) state |= 1;
	if( pc->event && WaitForSingleObject( pc->event, 0 ) == WAIT_OBJECT_0 )
		state |= 2;
	if( pc->this_thread && pc->this_thread->flags.bProcessing ) state |= 4;
	if( pc->this_thread ) state |= ( (uint32_t)pc->this_thread->nWaitEvents << 8 ) | ( (uint32_t)pc->this_thread->nEvents << 16 );
#endif
	return state;
}

uint32_t NetworkClientWritesPended( PCLIENT pc ) {
	return pc ? pc->nWritesPended : 0;
}

LOGICAL NetworkClientValid( PCLIENT pc, uint32_t serial ) {
	// TRUE only if this is still the same connection the serial was captured
	// from; a closed or recycled client fails even though a new connection on
	// the same PCLIENT would pass the active-flags test.
	return sack_network_is_active( pc ) && pc->serial == serial;
}

//----------------------------------------------------------------------------

// guards the psvInUse lists (AddNetWork/ClearNetWork run on application
// threads while the client closes/recycles on the network thread), and
// serializes the deferred-close decision: an event thread deciding whether to
// defer a close because work is outstanding (bInUse) must not race the
// ClearNetWork that releases the work, or both sides conclude the other will
// perform the close and the socket strands in CLOSE_WAIT.
static volatile uint32_t netWorkListLock;

void lockNetWorkList( void ) {
	while( LockedExchange( &netWorkListLock, 1 ) )
		Relinquish();
}

void unlockNetWorkList( void ) {
	netWorkListLock = 0;
}

//----------------------------------------------------------------------------

static PCLIENT AddClosed( PCLIENT pClient DBG_PASS )
{
	if( pClient )
	{
#if DBG_AVAILABLE
		// blame the caller of InternalRemoveClientEx, not this line - see netstruc.h
		pClient->closedFile = pFile;
		pClient->closedLine = nLine;
#endif
		// leaving active life; invalidate handles captured against this connection.
		LockedIncrement( &pClient->serial );
		SetClientFlags( pClient, CF_CLOSED );
		pClient->LastEvent = timeGetTime();
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
#ifndef NO_SSL
	// everything past clear_offset is about to be scrubbed; a session still parked
	// here would be lost outright.  Every path reaching this should already have
	// passed ssl_finalize() in TerminateClosedClientEx.
	if( pc->ssl_session_closed ) ssl_finalize( pc );
#endif
	uintptr_t* pbtemp;
	PCLIENT next;
	PCLIENT *me;
	SOCKADDR *sa_rel;
	uint32_t serial;
	// Everything from here to the end of the struct is scrubbed; the two
	// CRITICALSECTIONs deliberately sit in front of it and are never touched.
	static const size_t clear_offset = offsetof( struct NetworkClient, csLockWrite )
	                                 + sizeof( ( (PCLIENT)0 )->csLockWrite );
	// keep the closing flag until it's really been closed. (getfreeclient will try to nab it)
	int /*enum NetworkConnectionFlags*/  dwFlags = pc->dwFlags & (CF_STATEFLAGS | CF_CLOSING | CF_CONNECT_WAITING | CF_CONNECT_CLOSED);
#ifdef VERBOSE_DEBUG
	lprintf( "CLEAR CLIENT!" );
#endif
	me = pc->me;
	next = pc->next;
	// these states are saved to be restored.
	pbtemp = pc->lpUserData;
	serial = pc->serial; // generation continues across reuse; only close bumps it
	lockNetWorkList();
	DeleteListEx( &pc->psvInUse DBG_SRC );
	unlockNetWorkList();
	// these are memset to 0 afterward... 
	sa_rel = pc->saClient;
	pc->saClient = NULL;
	ReleaseAddress( sa_rel );
	sa_rel = pc->saSource;
	pc->saSource = NULL;
	ReleaseAddress( sa_rel );
#if _WIN32
	if( pc->event ) {
		if( globalNetworkData.flags.bLogNotices )
			_lprintf(DBG_RELAY)( "Closing network event:%p  %p", pc, pc->event );
		WSACloseEvent( pc->event );
	}
#endif

	// clear all information... but start after the two CRITICALSECTIONs at the head
	// of the struct.  They must never be written here: other threads spin on them in
	// EnterCriticalSecNoWaitEx with no global lock held, so zeroing and restoring
	// them resurrects stale owners/counts (clients left permanently read-locked).
	MemSet( (uint8_t*)pc + clear_offset, 0, sizeof( CLIENT ) - clear_offset );
	pc->Socket = INVALID_SOCKET;
	pc->serial = serial;
	pc->lpUserData = pbtemp;
	if( pc->lpUserData )
		MemSet( pc->lpUserData, 0, globalNetworkData.nUserData * sizeof( uintptr_t ) );
	pc->next = next;
	pc->me = me;
	pc->dwFlags = dwFlags;
}

//----------------------------------------------------------------------------

// used in network_linux during close...
LOGICAL TryNetworkGlobalLock( DBG_VOIDPASS ) {
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
	if( pc->dwFlags & CF_CLOSED )
	{
		PendingBuffer * lpNext;
		EnterCriticalSec( &globalNetworkData.csNetwork );
		RemoveThreadEvent( pc );
#ifdef VERBOSE_DEBUG
		lprintf( "REMOVED EVENT...." );
#endif
		clearPending( pc );
		//lprintf( "Terminating closed client..." );
		if( IsValid( pc->Socket ) )
		{
#ifdef VERBOSE_DEBUG
			lprintf( "close socket: %p", pc );
#endif
#if !defined( SHUT_WR ) && defined( _WIN32 )
#  define SHUT_WR SD_SEND
#endif
#ifndef NO_SSL
			// Final TLS cleanup, here and nowhere else: both channel locks are held at
			// this point, so no read dispatch for this socket can be executing and the
			// session retired by ssl_ClosePipe / ssl_ClosePipeSession is safe to release.
			ssl_finalize( pc );
#endif
			shutdown( pc->Socket, SHUT_WR );
#if defined( _WIN32 )
#undef SHUT_WR
#endif
			//lprintf( "Win32:ShutdownWR+closesocket %p", pc );
			closesocket( pc->Socket );
			pc->Socket = INVALID_SOCKET;
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
		// DEFERRED RECYCLE.  Never hand a client back to the free pool while either
		// channel is still locked.  The EPOLLIN handler locks channel 1 for the whole
		// event and then closes from *inside* the read (a Connection: close response
		// sets CF_TOCLOSE during FinishPendingRead, which then returns -1 and takes
		// the "reset connection" branch straight to here) - so recycling here put a
		// still-locked client into AvailableClients, and GetFreeNetworkClient handed
		// it straight back out already locked.  That is the leaked +1 on csLockRead.
		// The final NetworkUnlockEx finishes the recycle once nothing holds it.
		if( pc->csLockRead.dwLocks || pc->csLockWrite.dwLocks ) {
			pc->recyclePending = 1;
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			return;
		}
		ClearClient( pc DBG_RELAY );
		// this should move from globalNetworkData.close to globalNetworkData.available.
		AddAvailable( GrabClient( pc ) );
		ClearClientFlags( pc, CF_CLOSING ); // it's no longer closing.  (was set during the course of closure)
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
		ClearClientFlags( pClient, CF_CPPWRITE );
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
		SetClientFlags( pClient, CF_CPPWRITE );
	}
}

//----------------------------------------------------------------------------

void SetNetworkCloseCallback( PCLIENT pClient
                            , cCloseCallback CloseCallback )
{
#ifndef NO_SSL
	if( pClient->ssl_session ) {
		pClient->ssl_session->user_close = CloseCallback;
		pClient->ssl_session->dwOriginalFlags &= ~CF_CPPCLOSE;
		return;
	}
#endif
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
#ifndef NO_SSL
	if( pClient->ssl_session ) {
		pClient->ssl_session->cpp_user_close = CloseCallback;
		pClient->psvClose = psv;
		pClient->ssl_session->dwOriginalFlags |= CF_CPPCLOSE;

		return;
	}
#endif
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->close.CPPCloseCallback = CloseCallback;
		pClient->psvClose = psv;
		SetClientFlags( pClient, CF_CPPCLOSE );
	}
}

//----------------------------------------------------------------------------

void SetNetworkReadComplete( PCLIENT pClient
                           , cReadComplete pReadComplete )
{
#ifndef NO_SSL
	if( pClient->ssl_session ) {
		pClient->ssl_session->user_read = pReadComplete;
		pClient->ssl_session->dwOriginalFlags &= ~CF_CPPREAD;
		return;
	}
#endif
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.ReadComplete = pReadComplete;
	}
	if( !( pClient->RecvPending.buffer.p ) ) {
		SetClientFlags( pClient, CF_READREADY ); // may be... at least we can fail sooner...
		if( pClient->read.ReadComplete )
			pClient->read.ReadComplete( pClient, NULL, 0 );
	}
}

//----------------------------------------------------------------------------

void SetCPPNetworkReadComplete( PCLIENT pClient
                              , cppReadComplete pReadComplete
                              , uintptr_t psv)
{
#ifndef NO_SSL
	if( pClient->ssl_session ) {
		//lprintf( "is an ssl connection - set new cpp_user_read %p %p", pClient->ssl_session->cpp_user_read, pClient->ssl_session->user_read);
		pClient->ssl_session->cpp_user_read = pReadComplete;
		pClient->ssl_session->psvRead = psv;
		//lprintf( "maybe psv Read needs to be reset? %p %p", pClient->psvRead, psv );
		pClient->psvRead = psv;
		pClient->ssl_session->dwOriginalFlags |= CF_CPPREAD;
		//lprintf( "Session original flags set? %x", pClient->ssl_session->dwOriginalFlags);
		return;
	}
#endif
	if( pClient && IsValid(pClient->Socket) )
	{
		pClient->read.CPPReadComplete = pReadComplete;
		pClient->psvRead = psv;
		SetClientFlags( pClient, CF_CPPREAD );
	}
	if( !( pClient->RecvPending.buffer.p ) ) {
		SetClientFlags( pClient, CF_READREADY ); // may be... at least we can fail sooner...
		if( pClient->read.ReadComplete )
			pClient->read.CPPReadComplete( pClient->psvRead, NULL, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void TriggerNetworkErrorCallback( PCLIENT pc, enum SackNetworkErrorIdentifier error ) {
	/*
	if( pc && pc->ssl_session && pc->ssl_session->errorCallback )
		pc->ssl_session->errorCallback( pc->ssl_session->psvErrorCallback, pc, error );
	else
	*/
	if( pc && pc->errorCallback )
		pc->errorCallback( pc->psvErrorCallback, pc, error );
}

//----------------------------------------------------------------------------

/* checkStuckConnects used to live here: a 1s timer that re-selected any socket
   sitting CF_CONNECTING without events.  It was a safety net for a winsock case
   where FD_CONNECT was never delivered even though the connection had been
   established - but that is handled properly at the source now, in the
   network_win32.c FD_WRITE handler, which completes the connect when the socket
   turns out to be writable while still CF_CONNECTING.  Re-selecting adds nothing
   on top of that: if the socket really is connected, FD_WRITE already heals it;
   if it is not (no service on the port, or the traffic is being dropped) there is
   no event to recover and the re-select just logged once a second forever.  That
   false positive is all it produced in practice, so it is gone. */

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
		EV_SET64( &ev, GetThreadSleeper( thread ), EVFILT_READ, EV_ADD, 0, 0, (uint64_t)1, 0, 0 );
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
		this_thread.parent_peer->child_peer = this_thread.child_peer;
		if( this_thread.child_peer )
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
		struct peer_thread_info *peer_thread;
		peer_thread = globalNetworkData.root_thread;
		globalNetworkData.root_thread = NULL;
		MakeThread();
		// set the events directly - this runs from atexit/process detach where a
		// terminated thread may hold the heap lock; allocating (AddLink) here
		// spins forever on that orphaned lock.
		for( ; peer_thread; peer_thread = peer_thread->child_peer ) {
			WSASetEvent( peer_thread->hThread );
		}
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

// The slab packs clients back to back at sizeof(CLIENT) stride, so aligning
// client[0] only helps if the stride is a whole number of cache lines - otherwise
// every client after the first drifts and its lock words share a line with the
// previous client's tail.  The alignas in netstruc.h guarantees this; assert it
// so a later struct edit cannot silently undo the fix.
SACK_STATIC_ASSERT( ( sizeof( CLIENT ) % SACK_CACHE_LINE ) == 0
                  , "sizeof(CLIENT) must be a multiple of SACK_CACHE_LINE or the slab stride breaks lock alignment" );

static void AddClients( void ) {
	PCLIENT_SLAB pClientSlab;

	// protect all structures.
	EnterCriticalSec( &globalNetworkData.csNetwork );

	{
		size_t n;
		//Log1( "Creating %d Client Resources", MAX_NETCLIENTS );
		// Must be HeapAllocateAligned, not NewPlus: struct NetworkClient declares
		// SACK_CACHE_LINE alignment for its lock words, and only the allocator can
		// honour that for a heap block.  The compiler's part (client[] at a 64
		// multiple, sizeof(CLIENT) a 64 multiple so the stride holds) comes from the
		// alignas in netstruc.h.  NewPlus is HeapAllocate(0,...) and does not zero,
		// and every CLIENT_SLAB field is initialised below, so this is a like-for-like
		// swap apart from the alignment.
		pClientSlab = (PCLIENT_SLAB)HeapAllocateAligned( 0
		                                               , sizeof( CLIENT_SLAB ) + (MAX_NETCLIENTS - 1 ) * sizeof( CLIENT )
		                                               , SACK_CACHE_LINE );
		if( ( (uintptr_t)pClientSlab->client ) & ( SACK_CACHE_LINE - 1 ) )
			lprintf( "WARNING: client slab is not cache-line aligned (%p); the false-sharing"
			         " separation of csLockRead/csLockWrite is NOT in effect."
			       , pClientSlab->client );
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
	if( !wClients )
		wClients = 32;  // default 32 clients per slab...
	if( !nUserData )
		nUserData = 4;  // default to 4 pointer words per socket; most applications only use 1.

	// keep the max of specified data..
	if( nUserData < globalNetworkData.nUserData )
		nUserData = globalNetworkData.nUserData;

	// keep the max of specified connections..
	if( wClients < MAX_NETCLIENTS )
		wClients = MAX_NETCLIENTS;

	// if the client slab size increases, new slabs will be the new size; old slabs will still be the old size.

	if( wClients > MAX_NETCLIENTS || nUserData > globalNetworkData.nUserData ) // have to reallocate the user data for all sockets
	{
		INDEX idx;
		PCLIENT_SLAB slab;
		uint32_t n;
		EnterCriticalSec( &globalNetworkData.csNetwork );
		// for all existing client slabs...
		if( nUserData > globalNetworkData.nUserData ) // have to reallocate the user data for all sockets
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
		MAX_NETCLIENTS = wClients;
		globalNetworkData.nUserData = nUserData;
		LeaveCriticalSec( &globalNetworkData.csNetwork );
	}
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
#ifdef DEBUG_CLIENT_LOCK_TRACE
		// Both channels were taken directly above, before ClearClient scrubbed the
		// ring - so seed the fresh ring with them or the lifetime starts unbalanced
		// on paper.  Order matches the acquisition order (read then write).
		sack_dbg_traceClient( pClient, CLTRACE_LOCK, 1, pClient->csLockRead.dwLocks, pFile, nLine );
		sack_dbg_traceClient( pClient, CLTRACE_LOCK, 0, pClient->csLockWrite.dwLocks, pFile, nLine );
#endif
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
	if( lpClient && ( nLong < globalNetworkData.nUserData ) ) {
		lpClient->lpUserData[nLong] = dwValue;
		if( lpClient->pcOther ) lpClient->pcOther->lpUserData[nLong] = dwValue;
	}
	return;
}

//----------------------------------------------------------------------------

NETWORK_PROC( uintptr_t, GetNetworkLong )(PCLIENT lpClient,int nLong)
{
	if( !lpClient )
		return (uintptr_t)-1;

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
	else if( nLong < globalNetworkData.nUserData ) {
		if( lpClient->lpUserData )
			return lpClient->lpUserData[nLong];
		else {
			lprintf( "User data wasn't set on socket... %d", nLong );
			// content is expected to be NULL, even though this is sort of an error
			return 0;
		}
	}

	return (uintptr_t)-1;
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

NETWORK_PROC( PCLIENT, NetworkLockEx)( PCLIENT lpClient, int readWrite DBG_PASS )
{
	int tries = 0;
	if( lpClient )
	{
		//uint64_t start = timeGetTime64ns();
		//if( lpClient->flags.bWriteOnUnlock ) {
		//	lprintf( "Still need to do that write..." );
		//}
		//lpClient->dwFlags |= CF_WANTS_GLOBAL_LOCK;
		//_lprintf(DBG_RELAY)( "Lock %p %d", lpClient, readWrite );
		//fprintf( stderr, DBG_FILELINEFMT "Lock %p %d\n" DBG_RELAY, lpClient, readWrite );
#ifdef LOCK_GLOBAL_WHEN_LOCKING_CLIENT
#ifdef USE_NATIVE_CRITICAL_SECTION
		while( EnterCriticalSecNoWait( &globalNetworkData.csNetwork, NULL ) < 1 )
#else
		while( EnterCriticalSecNoWaitEx( &globalNetworkData.csNetwork, NULL DBG_RELAY ) < 1 )
#endif
		{
			//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
			if( ++tries > 9 ) {
				//uint64_t wait = timeGetTime64ns() - start;
				//lprintf( "A wait for global lock... %lld", wait );
//#ifdef LOG_NETWORK_LOCKING
				_lprintf(DBG_RELAY)( "Failed enter global? %llx", globalNetworkData.csNetwork.dwThreadID  );
//#endif
				return NULL;
			} else {				
				Relinquish();

			}
			//DebugBreak();
		}
#ifdef LOG_NETWORK_LOCKING
		_lprintf( DBG_RELAY )( "Got global lock %p %d", lpClient, readWrite );
#endif
#endif
		//lpClient->dwFlags &= ~CF_WANTS_GLOBAL_LOCK;
		tries = 0;
#ifdef USE_NATIVE_CRITICAL_SECTION
		while( !EnterCriticalSecNoWait( (readWrite? &lpClient->csLockRead:&lpClient->csLockWrite), NULL ) )
#else
		while( EnterCriticalSecNoWaitEx( ( readWrite ?&lpClient->csLockRead : &lpClient->csLockWrite ), NULL DBG_RELAY ) < 1 )
#endif
		{
			// unlock the global section for a moment..
			// client may be requiring both local and global locks (already has local lock)
			if( ++tries < 10 ) {
				Relinquish();
				continue;
			}
			//uint64_t wait = timeGetTime64ns() - start;
			//lprintf( "A wait for client lock... %lld", wait );
			//fprintf( stderr, DBG_FILELINEFMT "Failed Lock:%p %d\n" DBG_RELAY, lpClient, readWrite );
#ifdef LOCK_GLOBAL_WHEN_LOCKING_CLIENT
#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( &globalNetworkData.csNetwork);
#else
			LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
#endif
			//lprintf( "Idle... socket lock failed, had global though..." );
			//Relinquish();
			return NULL;
			//goto start_lock;
		}
#ifdef LOCK_GLOBAL_WHEN_LOCKING_CLIENT
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSec( &globalNetworkData.csNetwork );
#else
		LeaveCriticalSecEx( &globalNetworkData.csNetwork  DBG_RELAY);
#endif
#endif
		if( !(lpClient->dwFlags & (CF_ACTIVE|CF_CLOSED) ) )
		{
			// change to inactive status by the time we got here...
#ifdef USE_NATIVE_CRITICAL_SECTION
			LeaveCriticalSec( readWrite ? &lpClient->csLockRead : &lpClient->csLockWrite );
#else
			LeaveCriticalSecEx( readWrite?&lpClient->csLockRead:&lpClient->csLockWrite DBG_RELAY );
#endif
#ifdef DEBUG_CLIENT_LOCK_TRACE
		sack_dbg_traceClient( lpClient, CLTRACE_UNLOCK, readWrite, readWrite ? lpClient->csLockRead.dwLocks : lpClient->csLockWrite.dwLocks, pFile, nLine );
#endif
//#ifdef LOG_NETWORK_LOCKING
			_lprintf( DBG_RELAY )( "Failed lock: %p  %08x %08x inactive, cannot lock.", lpClient, lpClient->dwFlags, CF_ACTIVE );
//#endif
			// this client is not available for client use!
			return NULL;
		}
	}
#ifdef LOG_NETWORK_LOCKING
	_lprintf( DBG_RELAY )( "Got private lock %p %d", lpClient, readWrite );
#endif
#ifdef DEBUG_CLIENT_LOCK_TRACE
	sack_dbg_traceClient( lpClient, CLTRACE_LOCK, readWrite, readWrite ? lpClient->csLockRead.dwLocks : lpClient->csLockWrite.dwLocks, pFile, nLine );
#endif
	return lpClient;
}

//----------------------------------------------------------------------------

NETWORK_PROC( void, NetworkUnlockEx)( PCLIENT lpClient, int readWrite DBG_PASS )
{
	//_lprintf(DBG_RELAY)( "Unlock %p %d", lpClient, readWrite );
	//fprintf( stderr, DBG_FILELINEFMT "Unlock %p %d\n" DBG_RELAY, lpClient, readWrite );
	int const inWakeOnUnlock = readWrite & 0x10;
	readWrite &= 3;
	// simple unlock.
	if( lpClient )
	{
		if( !readWrite ) // is write and not read
		{
			//lprintf( "Unlocking write... %p (WOU?)%d", lpClient, lpClient->flags.bWriteOnUnlock );
			if( lpClient->flags.bWriteOnUnlock ) {
				lpClient->flags.bWriteOnUnlock = 0;
				//lprintf( "Caught unlock..." );
				TCPWriteEx( lpClient DBG_RELAY );
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
#ifdef DEBUG_CLIENT_LOCK_TRACE
		sack_dbg_traceClient( lpClient, CLTRACE_UNLOCK, readWrite, readWrite ? lpClient->csLockRead.dwLocks : lpClient->csLockWrite.dwLocks, pFile, nLine );
#endif
		// Deferred recycle: a close that ran while this client was locked left the
		// job to whoever drops the last lock.  Doing it here, after the leave, is
		// what guarantees a client is never sitting in AvailableClients with a
		// channel still held.  No client lock is held at this point, so taking
		// csNetwork here cannot invert the established clientLock -> csNetwork order.
		if( lpClient->recyclePending
		 && !lpClient->csLockRead.dwLocks && !lpClient->csLockWrite.dwLocks ) {
			EnterCriticalSec( &globalNetworkData.csNetwork );
			if( lpClient->recyclePending
			 && !lpClient->csLockRead.dwLocks && !lpClient->csLockWrite.dwLocks ) {
				lpClient->recyclePending = 0;
				ClearClient( lpClient DBG_RELAY );
				AddAvailable( GrabClient( lpClient ) );
				ClearClientFlags( lpClient, CF_CLOSING );
			}
			LeaveCriticalSec( &globalNetworkData.csNetwork );
			return; // back in the pool - nothing below may touch it any more
		}
		if( !readWrite && !inWakeOnUnlock ) // is write and not read
		{
			PTHREAD wakeOnUnlock;
			if( ( wakeOnUnlock = lpClient->wakeOnUnlock ) ){
#ifdef LOG_PENDING_WRITES		
				_lprintf(DBG_RELAY)( "Wake on Unlock was set");
#endif				
				lpClient->wakeOnUnlock = NULL;
				WakeThread( wakeOnUnlock );
				//lprintf( "Woke writer..");
			}

		}
	}
}

//----------------------------------------------------------------------------

void InternalRemoveClientExx(PCLIENT lpClient, LOGICAL bBlockNotify, LOGICAL bLinger DBG_PASS )
{
#ifdef LOG_SOCKET_CREATION
	_lprintf( DBG_RELAY )( "InternalRemoveClient Removing this client %p (%d)", lpClient, lpClient->Socket );
#endif
	if( lpClient && IsValid(lpClient->Socket) )
	{
		// an abortive request cannot override a graceful one already latched
		if( !bLinger && !( lpClient->dwFlags & CF_LINGERCLOSE ) )
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
					// this happens(in windows) when a client didn't connect, and resulted with a error
					//lprintf( "error setting no linger in close." );
					//cerr << "NFMSim:setHost:ERROR: could not set socket to linger." << endl;
				}
			}
		}
		else {
			struct linger lingerSet;
			// SO_LINGER ON makes close() BLOCK until the peer ACKs or the timeout
			// expires - on an event thread that stalls every other socket it owns,
			// and backs the accept queue up into ECONNREFUSED.  Off is the correct
			// graceful close for a non-blocking server: close() returns at once and
			// the kernel still flushes, retransmits and FINs, which is all that was
			// needed to stop the response being discarded.
			lingerSet.l_onoff = 0;
			lingerSet.l_linger = 0;
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
				AddClosed( GrabClient( lpClient ) DBG_RELAY );
			}
#ifdef LOG_DEBUG_CLOSING
			else
				lprintf( "Client's state is CLOSED" );
#endif
			return;
		}
		// nWritesPended counts writes parked on the global pdqPendingWrites queue
		// (and the stall list).  Those set neither lpFirstPending nor
		// CF_WRITEPENDING, so without it a graceful close reads "nothing pending"
		// and tears down a socket whose response has not been written yet.
		if( bLinger && ( lpClient->lpFirstPending || ( lpClient->dwFlags & CF_WRITEPENDING )
		               || lpClient->nWritesPended ) ) {
#ifdef LOG_DEBUG_CLOSING
			lprintf( "GRACEFUL CLOSE WHILE WAITING FOR WRITE TO FINISH... %p", lpClient );
#endif
			SetClientFlags( lpClient, CF_TOCLOSE );
			return;
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
			SetClientFlags( lpClient, CF_CONNECT_CLOSED );
			if( lpClient->pWaiting )
			{
				WakeThread( lpClient->pWaiting );
				while( lpClient->dwFlags & CF_CONNECT_WAITING )
					Relinquish();
			}
			ClearClientFlags( lpClient, CF_CONNECT_CLOSED );
			if( !(lpClient->dwFlags & CF_CLOSING) ) // prevent multiple notifications...
			{
#ifdef LOG_DEBUG_CLOSING
				lprintf( "Marked closing first, and dispatching callback? %p", lpClient );
#endif
				SetClientFlags( lpClient, CF_CLOSING );
				// invalidate deferred handles BEFORE the close callback tears down
				// application state; NetworkClientValid() fails from here on, so a
				// deferred event validated after this cannot find half-torn state.
				LockedIncrement( &lpClient->serial );
				LeaveCriticalSec( &globalNetworkData.csNetwork );
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
				EnterCriticalSec( &globalNetworkData.csNetwork );
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
		AddClosed( GrabClient( lpClient ) DBG_RELAY );
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
	// Latch before anything else: the SSL path returns early via ssl_CloseSession,
	// and the terminal close for this client comes back through the event loop with
	// bLinger hard-coded FALSE.
	if( bLinger )
		SetClientFlags( lpClient, CF_LINGERCLOSE );
	//_lprintf(DBG_RELAY)( "RemoveClient: %p %d %d", lpClient, bBlockNotify, bLinger );
	if( !( lpClient->dwFlags & ( CF_UDP | CF_CLOSING ) ) 
		&& ( lpClient->dwFlags & ( CF_CONNECTED ) )
		&& !( lpClient->dwFlags & CF_CONNECTERROR ) ) {
		// not linger 
		// OR  nothing to write allow shutdown.
#ifndef NO_SSL
		if( ssl_IsClientSecure( lpClient ) ) {
			if( !ssl_IsClosed( lpClient ) ) {
				// let client notify_close actually close this...
				//lprintf( "secure client, not closed, just close session... (no shutdown?)");
				// bLinger MUST go with it: this returns before the pending-write
				// test below, and ssl_CloseSession's terminal RemoveClient comes
				// back through here to make that decision.
				ssl_CloseSession( lpClient, bLinger );
				return;
			}
		}
#endif
		// see InternalRemoveClientExx: a write deferred to pdqPendingWrites shows up
		// only in nWritesPended, and shutting down here discards it (send() then
		// fails EPIPE and the peer gets a clean FIN with no response).
		if( !bLinger || !(lpClient->lpFirstPending || ( lpClient->dwFlags & CF_WRITEPENDING )
		               || lpClient->nWritesPended ) ) {
			shutdown( lpClient->Socket, SHUT_WR );
		} else {
			//lprintf( "linger and still pending write data..." ); // normal path; noisy under load
			SetClientFlags( lpClient, CF_TOCLOSE );
		}
		SetClientFlags( lpClient, CF_WANTCLOSE );
	} else {
		int n = 0;
		if( !(lpClient->dwFlags & CF_ACTIVE )
			|| lpClient->dwFlags & ( CF_CLOSED | CF_CLOSING ) )
			return;
		// UDP still needs to be done this way...
		// socket not connected; shutdown will not work.
		//lprintf( "This will end up resetting the socket?" );
		EnterCriticalSec( &globalNetworkData.csNetwork );
		InternalRemoveClientExx( lpClient, bBlockNotify, bLinger DBG_RELAY );
		if( NetworkLockEx( lpClient, 0 DBG_RELAY ) && ((n=1),NetworkLockEx( lpClient, 1 DBG_RELAY )) ) {
			TerminateClosedClient( lpClient );
			NetworkUnlock( lpClient, 0 );
			NetworkUnlock( lpClient, 1 );
		}
		else if( n ) {
			NetworkUnlock( lpClient, 0 );
			SetClientFlags( lpClient, CF_TOCLOSE );
			fprintf( stderr, "STRANDED: RemoveClientExx skipped TerminateClosedClient (channel locked); count=%u\n",
			         (unsigned)LockedIncrement( &strandedTerminates ) );
		}
		LeaveCriticalSec( &globalNetworkData.csNetwork );
	}
}

PLIST* GetNetWork( PCLIENT lpClient ) {
	lockNetWorkList();
	return &lpClient->psvInUse;
}
// should we validate that the psv is in the list?  or just clear it?
void DropNetWork( PCLIENT lpClient ) {
	unlockNetWorkList();
}

void AddNetWork( PCLIENT lpClient, uintptr_t psv ) {
	lockNetWorkList();
	AddLink( &lpClient->psvInUse, (POINTER)psv );
	lpClient->flags.bInUse = 1;
#ifdef DEBUG_CLIENT_LOCK_TRACE
	// This is the marker that a request was handed toward JS: on the request path
	// the only caller is webSockHttpRequest (sack.vfs), which then queues a
	// WS_EVENT_REQUEST and uv_async_send()s it.  A client found hung with bInUse=1
	// and no matching CLEARNET got that far and the JS callback never ran.
	sack_dbg_traceClient( lpClient, CLTRACE_ADDNET, 0
	                    , (uint32_t)GetLinkCount( lpClient->psvInUse ) DBG_SRC );
#endif
	unlockNetWorkList();
}


void ClearNetWork( PCLIENT lpClient, uintptr_t psv ) {
	LOGICAL emptied = FALSE;
	lockNetWorkList();
	{
		INDEX id = FindLink( &lpClient->psvInUse, (POINTER)psv );
		if( id != INVALID_INDEX ) {
			SetLink( &lpClient->psvInUse, id, NULL );
		}
		if( !GetLinkCount( lpClient->psvInUse ) ) {
			lpClient->flags.bInUse = 0;
			emptied = TRUE;
		}
#ifdef DEBUG_CLIENT_LOCK_TRACE
		sack_dbg_traceClient( lpClient, CLTRACE_CLEARNET, 0
		                    , (uint32_t)GetLinkCount( lpClient->psvInUse ) DBG_SRC );
#endif
	}
	unlockNetWorkList();
	if( !emptied )
		return;
	// nWritesPended counts writes parked on the global pdqPendingWrites queue,
	// which set neither lpFirstPending nor CF_WRITEPENDING - so without it this
	// test sees "nothing pending" and closes on top of the response it was
	// deferring for.  Same counter already added to InternalRemoveClientExx
	// (~1514) and RemoveClientExx (~1660); this completion path was missed.
	// deliverPendingWrite finishes the close once the queue drains.
	if( ( lpClient->dwFlags & CF_TOCLOSE ) && lpClient->nWritesPended ) {
		// PROBE (silent counter): declining the close while a write is queued is the
		// NORMAL path - deliverPendingWrite completes it.  Pair with
		// DELIVER-HANDOFF - the same pc printed by both means neither completed
		// the close and the client is stranded (win32 has no re-notification to
		// rescue it, so this is the suspected source of the intermittent stall).
		static volatile uint32_t nDefer;
		LockedIncrement( &nDefer );  // silent: DELIVER-HANDOFF is the one that speaks
	}
	if( lpClient->dwFlags & CF_TOCLOSE && !lpClient->nWritesPended
	 && (!lpClient->lpFirstPending || !lpClient->lpFirstPending->dwAvail) ) {
		{	// PROBE: is a response parked on pdqPendingWrites when we close here?
			// nWritesPended is checked in InternalRemoveClientExx (~1514) and
			// RemoveClientExx (~1660) but NOT in this test.
			static volatile uint32_t nClose, nCloseWithWrites;
			LockedIncrement( &nClose );
			if( lpClient->nWritesPended )
				fprintf( stderr, "CLEARNET-CLOSE-DISCARDS pc=%p flags=%08x nWritesPended=%u n=%u of=%u\n"
				       , (void*)lpClient, (unsigned)lpClient->dwFlags, (unsigned)lpClient->nWritesPended
				       , (unsigned)LockedIncrement( &nCloseWithWrites ), (unsigned)nClose );
		}
		RemoveClient( lpClient );
	}
}

SACK_NETWORK_NAMESPACE_END
