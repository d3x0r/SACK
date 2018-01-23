/* 
 * Crafted by Jim Buckeyne
 * Resembles function of SYSV IPC Message Queueus, and handle event based, inter-process, shared
 * queue, message transport.
 *
 * (c)1999-2006++ Freedom Collective
 *
 */

#include <stdhdrs.h> // Sleep

#include <stddef.h> // offsetof

#include <sharemem.h>

#include <sack_types.h>
#include <timers.h>
#include <msgprotocol.h>
#ifdef __LINUX__
#define SetLastError(n)  errno = n
#else
#define SetLastError(n)  SetLastError(n);errno = n
#endif

#ifdef __cplusplus
namespace sack {
	namespace containers {
	namespace message {
		using namespace sack::memory;
		using namespace sack::timers; 	
		using namespace sack::logging;
#endif

//#define DISABLE_MSGQUE_LOGBINARY
//#define DISABLE_MSGQUE_LOGGING
#define DISABLE_MSGQUE_LOGGING_DETAILED
static INDEX _tmp, __tmp;

void _UpdatePos( INDEX *tmp, INDEX inc DBG_PASS )
{
	if( inc == 0 )
		DebugBreak();
	__tmp = _tmp;
	_tmp = (*tmp);
	(*tmp) += inc;
	if( (*tmp) > 0x8000 )
		DebugBreak();
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	_xlprintf(5 DBG_RELAY )( WIDE("updating position from %d,%d,%d by %d"), __tmp, _tmp, (*tmp), inc );
#endif
}

void _SetPos( INDEX *tmp, INDEX inc, INDEX initial DBG_PASS )
{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	if( initial )
		_xlprintf(5 DBG_RELAY )( WIDE("Setting position to %d"), inc );
	else
		_xlprintf(5 DBG_RELAY )( WIDE("Setting position from %d to %d"), (*tmp), inc );
#endif
	if( !initial )
	{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
		__tmp = _tmp;
		_tmp = (*tmp);
#endif
		(*tmp) = inc;
	}
	else
	{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
		__tmp = _tmp =
#endif
			(*tmp) = inc;
	}
}
#define UpdatePos(t,i) _UpdatePos( &(t), (i) DBG_SRC )
#define SetPos(t,i) _SetPos( &(t), (i), FALSE DBG_SRC )
#define SetPosI(t,i) _SetPos( &(t), (i), TRUE DBG_SRC )

//--------------------------------------------------------------------------
//  MSG Queue functions.
//--------------------------------------------------------------------------
typedef PREFIX_PACKED struct MsgInternalData {
	// length  & 0x80000000 == after msg, return to head of queue
	// length == 0x80000000 == next is actually first in queue...
	// length & 0x40000000 message has already been received...
	// length & 0x20000000 message tag for request for specific message
	//   then length(low) points at next waiting.
	//   MsgID is the ID being waited for
	//   datalength is sizeof( THREAD_ID ) and data is MyThreadID()
	uint32_t length;
	uint32_t real_length; // size resulting in read...
	uint32_t ttl;  // this is tick count + time to live, if tick count is greater than this, message is dropped.

	// space which we added ourselves...
} PACKED MSGCORE;

typedef PREFIX_PACKED struct MsgData
{
  	MSGCORE msg;
	long MsgID;
	// ... end of this structure is
	// defined by length & 0x0FFFFFFC
} PACKED MSGDATA, *PMSGDATA;

typedef PREFIX_PACKED struct ThreadWaitMsgData
{
	MSGDATA msgdata;
	THREAD_ID thread;
	// ... end of this structure is
	// defined by length & 0x0FFFFFFC
} PACKED THREADMSGDATA, *PTHREADMSGDATA;

typedef struct MsgDataQueue
{
	TEXTCHAR name[128];
	INDEX   Top;
	INDEX   Bottom;
	INDEX   Cnt;   // number of times this is open, huh?
	uint32_t     waiter_top; // reference of first element in queue waiting for specific ID
	uint32_t     waiter_bottom; // reference of first element in queue waiting for specific ID
	//THREAD_ID waiting;  // a thread waiting for any message...
	CRITICALSECTION cs;
	INDEX   Size;
	// this is a lot of people using this queue....
	// 1000 unique people waiting for a message....
	// this is fairly vast..
	struct {
		long msg; // and this is the message I wait for.
		THREAD_ID me;  // my ID - who I am that I am waiting...
	} waiters[1024];
	uint8_t      data[1];
} MSGQUEUE, *PMSGQUEUE;

typedef struct MsgDataHandle
{
	PMSGQUEUE pmq;
	uint32_t default_ttl;
	MsgQueueReadCallback Read;
	uintptr_t psvRead;
	DeclareLink( struct MsgDataHandle );
} MSGHANDLE;

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------

// in this case size is the size of the queue, there
// is no expansion possible...
// this should be created such that it is at least 3 * largest message
 PMSGHANDLE  SackCreateMsgQueue ( CTEXTSTR name, size_t size
                                , MsgQueueReadCallback Read
                                , uintptr_t psvRead )
{
	PMSGHANDLE pmh;
	PMSGQUEUE pmq;
	uintptr_t dwSize = size + sizeof( MSGQUEUE );
	uint32_t bCreated;
#ifdef __LINUX__
	TEXTCHAR tmpname[128];
	sprintf( tmpname, WIDE("/tmp/%s"), name );
#endif
	pmq = (PMSGQUEUE)OpenSpaceExx(
#ifdef __LINUX__
							 NULL
						  , tmpname
#else
							 name
						  , NULL
#endif
						  , 0
						  , &dwSize
							, &bCreated );
	if( !pmq )
		return NULL;
	InitializeCriticalSec( &pmq->cs );

	pmh          = (PMSGHANDLE)Allocate( sizeof( MSGHANDLE ) );
	pmh->default_ttl = 250;
	pmh->Read    = Read;
	pmh->psvRead = psvRead;
	pmh->pmq     = pmq;
	// now - how to see if result is new...
	// space is 0'd on create.
	// so if the second open results before the create
	// always increment this - otherwise the create open will
	// obliterate the second opener's presense.
	StrCpyEx( pmq->name, name?name:WIDE("Anonymous"),127 );
	pmq->name[127] = 0;
	pmq->Cnt++;
	if( bCreated )
	{
		pmq->Size = size;
	}
	return pmh;
}

//--------------------------------------------------------------------------

// in this case size is the size of the queue, there
// is no expansion possible...
// this should be created such that it is at least 3 * largest message
 PMSGHANDLE  SackOpenMsgQueue ( CTEXTSTR name
													 , MsgQueueReadCallback Read
													 , uintptr_t psvRead )
{
	PMSGHANDLE pmh;
	PMSGQUEUE pmq;
	uintptr_t dwSize = 0;
	uint32_t bCreated;
#ifdef __LINUX__
	char tmpname[128];
	sprintf( tmpname, WIDE("/tmp/%s"), name );
#endif
	pmq = (PMSGQUEUE)OpenSpaceExx(
#ifdef __LINUX__
							 NULL
						  , tmpname
#else
							 name
						  , NULL
#endif
						  , 0
						  , &dwSize
							, &bCreated );
	if( !pmq )
		return NULL;
	pmh          = (PMSGHANDLE)Allocate( sizeof( MSGHANDLE ) );
	pmh->default_ttl = 250;
	pmh->Read    = Read;
	pmh->psvRead = psvRead;
	pmh->pmq     = pmq;
	// now - how to see if result is new...
	// space is 0'd on create.
	// so if the second open results before the create
	// always increment this - otherwise the create open will
	// obliterate the second opener's presense.
	StrCpyEx( pmq->name, name?name:WIDE("Anonymous"), sizeof( pmq->name ) );
	pmq->Cnt++;
	if( bCreated )
	{
		lprintf( WIDE("SackOpenMsgQueue should never result with a created queue!") );
		DebugBreak();
		//pmq->Size = size;
	}
	return pmh;
}

//--------------------------------------------------------------------------

 void  DeleteMsgQueueEx ( PMSGHANDLE *ppmh DBG_PASS )
{

	if( ppmh )
	{
		EnterCriticalSec( &(*ppmh)->pmq->cs );
		if( (*ppmh)->pmq )
		{
			INDEX owners;
			owners = --(*ppmh)->pmq->Cnt;
			lprintf( WIDE("Remaining owners of queue: %") _size_f, owners );
			CloseSpaceEx( (*ppmh)->pmq, (!owners) );
		}
		Release( *ppmh );
		*ppmh = NULL;
	}
}

#define DBG_SOURCE DBG_SRC
//--------------------------------------------------------------------------

#define ACTUAL_LEN_MASK           0x000FFFFF
#define MARK_FLAGS                0xF0000000
#define MARK_END_OF_QUE           0x80000000
#define MARK_MESSAGE_ALREADY_READ 0x40000000
#define MARK_THREAD_WAITING       0x20000000
#define MESSAGE_SKIPABLE          0xc0000000

//--------------------------------------------------------------------------

#ifndef DISABLE_MSGQUE_LOGGING_DETAILED

 void DumpMessageQueue( PMSGQUEUE pmq )
 {
	 int tmp;
	 for( tmp = pmq->Bottom; tmp != pmq->Top; tmp = tmp + (((uint32_t*)(pmq->data+tmp))[0] & ACTUAL_LEN_MASK ) )
	 {
		 lprintf( "Message In Queue... %d", (((uint32_t*)(pmq->data+tmp))[0] & ACTUAL_LEN_MASK ) );
		 LogBinary( pmq->data + tmp, (((uint32_t*)(pmq->data+tmp))[0] & ACTUAL_LEN_MASK ) );
	 }
 }

void DumpWaiterQueue( PMSGQUEUE pmq )
{
	INDEX tmp;
	tmp = pmq->waiter_bottom;
	while( tmp != pmq->waiter_top )
	{
		lprintf( WIDE("[%d] waiter sleeping is %016") _64fX WIDE(" for %") _32f WIDE("")
				  , tmp
				 , pmq->waiters[tmp].me
				 , pmq->waiters[tmp].msg
				 );
		tmp++;
		if( tmp >= 1024 )
			tmp = 0;
	}
}
#endif
//--------------------------------------------------------------------------

static void CollapseWaiting( PMSGQUEUE pmq, long msg )
{
	//int nWoken = 0;

	int32_t tmp = pmq->waiter_bottom;
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	lprintf( WIDE("before moving the waiters forward on %s... msg %ld (or me? %s)"), pmq->name, msg, msg?"no":"yes" );
	DumpWaiterQueue( pmq );
#endif
	// now walk tmp backwards...
	// and move entried before the threads woken forward...
	// end up with a new bottom always (if having awoken something)
	if( pmq->waiter_top != pmq->waiter_bottom )
	{
		//uint32_t last = 0;
		uint32_t found = 0;
		uint32_t marked = 0;
		int32_t next = pmq->waiter_top - 1;
		INDEX tmp_bottom = pmq->waiter_bottom;
		if( next < 0 )
			next = 1023;
		// start at the last used queue spot (tmp)
		tmp = next;
		tmp_bottom = pmq->waiter_bottom;
		while( 1 )
		{
			if( !pmq->waiters[next].me ||
				( msg &&
				 ( msg == pmq->waiters[next].msg ) ) )
			{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				lprintf( WIDE("Skipping a next %d... "), next );
#endif
				if( !marked )
				{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
					lprintf( WIDE("marking tmp to copy into...") );
#endif
					tmp = next;
					marked = 1;
				}
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				else
					lprintf( WIDE("Already marked... and moving every element..") );
#endif
				if( (++tmp_bottom) >= 1024 )
				{
					tmp_bottom = 0;
				}
				// no reason to move next if it's still NULL...
				//continue;
			}
			else
			{
				found = 1;
				if( marked )
				{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
					lprintf( WIDE("Marked something... and now we move next %d into %d"), next, tmp );
#endif
					pmq->waiters[tmp--] = pmq->waiters[next];
					if( tmp < 0 )
						tmp = 1023;
					//continue;
				}
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				lprintf( WIDE("Next queue element is kept... set temp here.") );
#endif
			}
			// update next
			if( next == pmq->waiter_bottom )
					break;
			next--;
			if( next < 0 )
				next = 1023;
		}
		if( !found )
		{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
			lprintf( WIDE("Found nothing of interest.  Empty queue.") );
#endif
			pmq->waiter_bottom = pmq->waiter_top;
		}
		else if( marked )
		{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
			lprintf( WIDE("Moving final into last position, and updating bottom to %d"), tmp_bottom );
#endif
			pmq->waiters[tmp] = pmq->waiters[next];
			pmq->waiter_bottom = (uint32_t)tmp_bottom;
		}
	}

#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	lprintf( WIDE("And then after moving waiters forward....") );
	DumpWaiterQueue( pmq );
#endif
}

//--------------------------------------------------------------------------

static int ScanForWaiting( PMSGQUEUE pmq, long msg )
{
	int nWoken = 0;

	INDEX tmp = pmq->waiter_bottom;
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	lprintf( WIDE("before Scanning waiting on %s"), pmq->name );
	DumpWaiterQueue( pmq );
#endif
	while( tmp != pmq->waiter_top )
	{
		if( pmq->waiters[tmp].me )
		{
			// if waiting for any message....
			// or waiting for the exact message... and it is that message
			// or waiting for any other message... and it's not the message...
			if( !pmq->waiters[tmp].msg ||
				( (msg & 0x80000000)
				 ? (pmq->waiters[tmp].msg != (msg & 0x7FFFFFFF))
				 : (pmq->waiters[tmp].msg == msg) ) )
			{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				lprintf( WIDE("Wake thread %016Lx"), pmq->waiters[tmp].me );
#endif
				WakeThreadID( pmq->waiters[tmp].me );
				// reset the waiter ID... it's been
				// awoken, and is no longer waiting....
				pmq->waiters[tmp].me = 0;
				nWoken++;
				// go through all possible people who might wake up
				// because of this message... it's typically bad form
				// to have two or more processes watiing on the same ID...
			}
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
			else
			{
				lprintf( WIDE("Not waking thread %016Lx %08lx %08lx"), pmq->waiters[tmp].me
						 , msg, pmq->waiters[tmp].msg );
			}
#endif
		}
		tmp++;
		if( tmp >= 1024 )
			tmp = 0;
	}

	// now walk tmp backwards...
	// and move entried before the threads woken forward...
	// end up with a new bottom always (if having awoken something)
	if( nWoken )
	{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
		lprintf( WIDE("Scanning to delete messages that have been awoken.") );
#endif
		CollapseWaiting( pmq, 0 );
	}

	if( nWoken > 1 )
	{
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
		lprintf( WIDE("Woke %d threads as a result of message id %08lx"), nWoken, msg );
#endif
	}
	return 0;
}

//--------------------------------------------------------------------------

 int  EnqueMsgEx ( PMSGHANDLE pmh,  POINTER msg, size_t size, uint32_t opts DBG_PASS )
{
	if( pmh )
	{
		PMSGQUEUE pmq = pmh->pmq;
		INDEX tmp;
#ifndef DISABLE_MSGQUE_LOGGING
		int bNoSpace = 0;
#endif
		uint32_t realsize = (( size + (sizeof( MSGDATA ) ) ) + 3 ) & 0x7FFFFFFC;
		if( !pmq )
		{
			// errno = ENOSPACE; // or something....
			return -1; // cannot create this - no idea how big.
		}
		if( ( size > ( pmq->Size >> 2 ) )
			||( realsize > ( pmq->Size >> 2 ) ) )
		{
			//errno = E2BIG;
			return -1; // message is too big for this queue...
		}
		// probably someday need an error variable of
		// some sort or another...
#ifndef DISABLE_MSGQUE_LOGGING
		_xlprintf(3 DBG_RELAY)( WIDE("Enque space left...raw: %") _size_f WIDE(" %") _size_f WIDE(" Avail: %") _size_f WIDE(" %") _size_f WIDE(" used: %") _size_f WIDE(" %") _size_f WIDE(" %d")
				 , pmq->Top, pmq->Bottom
				 , pmq->Bottom-pmq->Top, pmq->Size-pmq->Top + pmq->Bottom
				 , pmq->Top-pmq->Bottom, pmq->Size-pmq->Bottom + pmq->Top
				 , realsize );
		_xlprintf(3 DBG_RELAY)( WIDE("[%s] ENqueMessage [%p] %ld len %") _MsgID_f WIDE(" %08") _32fx WIDE(""), pmq->name, pmq, *(MSGIDTYPE*)msg, size, *(uint32_t*)(pmq->data + pmq->Bottom) );
		//LogBinary( pmq->data + pmq->Bottom, 32 );
#endif
		while( msg )
		{
			int nWaiting;
			PMSGDATA pStoreMsg;
			EnterCriticalSec( &pmq->cs  );
			pStoreMsg = (PMSGDATA)(pmq->data + pmq->Top);
			SetPosI( tmp, pmq->Top + realsize );
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
			lprintf( "pStoreMsg = %p, %d %d", pStoreMsg, pmq->Top, tmp );
#endif
			if( tmp == (pmq->Size) )
			{
				// space is exactly what we need.
				pStoreMsg->msg.ttl = timeGetTime() + pmh->default_ttl;
				pStoreMsg->msg.real_length = (uint32_t)size;
				pStoreMsg->msg.length = realsize | MARK_END_OF_QUE | (( opts & MSGQUE_WAIT_ID )?MARK_THREAD_WAITING:0 );
#ifndef DISABLE_MSGQUE_LOGGING
				lprintf( WIDE("New tmp will be 0.") );
#endif
				SetPos( tmp, 0 );
			}
			else
			{
				if( tmp >= ( pmq->Size - sizeof( MSGDATA ) ) )
				{
					// okay - this message is too big to fit here...
					// going to have to store at start, or I suppose whenever the
					// queue has enough space...
#ifndef DISABLE_MSGQUE_LOGGING
					lprintf( WIDE("space left is not big enough for the message... %") _size_f WIDE(" %") _size_f WIDE(" %") _size_f WIDE(" %") _size_f WIDE("")
							 , pmq->Top, pmq->Bottom, pmq->Bottom-pmq->Top, pmq->Size-pmq->Top + pmq->Bottom );
#endif
					if( ( pmq->Bottom == 0 ) ||
						( pmq->Bottom <= size ) )
					{
						// Need to wait for some space...
						LeaveCriticalSec( &pmq->cs );
						if( opts & MSGQUE_NOWAIT )
						{
							//errno = EAGAIN;
							return -1;
						}
#ifndef DISABLE_MSGQUE_LOGGING
						lprintf( WIDE("bottom isn't far enough away either. Waiting for space") );
#endif
						Relinquish(); // someone's gotta run and take their message.
						continue;
					}
#ifndef DISABLE_MSGQUE_LOGGING
					lprintf( WIDE("Setting step to origin in length, going to origin, setting data %p"), pStoreMsg );
#endif
					// 0 data length, marked end, just junk...
					// okay there, and it's deleted, so noone can read it
					// even if they want a zero byte message :)
					pStoreMsg->msg.length = MARK_END_OF_QUE|MARK_MESSAGE_ALREADY_READ;
					// tmp needs to point to the next top.
					SetPos( tmp, realsize );
					pStoreMsg = (PMSGDATA)pmq->data;
					lprintf( WIDE("pStoreMsg = %p, %") _size_f, pStoreMsg, pmq->Top );
				}
				else
				{
					// this is the size of this msg... we can store
					// and there IS room for another message header of
					// at least 0 bytes at the end.
					if( tmp > pmq->Bottom && pmq->Top < pmq->Bottom )
					{
						lprintf( WIDE("No room left in queue...") );
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
						DumpMessageQueue( pmq );
#endif
						LeaveCriticalSec( &pmq->cs );
						Relinquish();
						continue; // try again from the top...
					}
				}
				pStoreMsg->msg.ttl = timeGetTime() + pmh->default_ttl;
				lprintf( WIDE( "Send Message TTL Expired in queue... %d %d %d" ), pStoreMsg->msg.ttl, pmh->default_ttl );
				pStoreMsg->msg.real_length = (uint32_t)size;
				pStoreMsg->msg.length = realsize | (( opts & MSGQUE_WAIT_ID )?MARK_THREAD_WAITING:0 );
			}
			if( tmp == pmq->Bottom )
			{
#ifndef DISABLE_MSGQUE_LOGGING
				bNoSpace = 1;
				lprintf( WIDE("Head would collide with tail...") );
#endif
				LeaveCriticalSec( &pmq->cs );
				if( opts & MSGQUE_NOWAIT )
				{
					//errno = EAGAIN;
					return -1;
				}
#ifndef DISABLE_MSGQUE_LOGGING
				lprintf( WIDE("Waiting for space") );
#endif
				Relinquish(); // someone's gotta run and take their message.
				continue;
			}
			else
			{
#ifndef DISABLE_MSGQUE_LOGGING
				if( bNoSpace )
					lprintf( WIDE("Okay there's space now...") );
#endif
			}

			MemCpy( &pStoreMsg->MsgID, msg, size + sizeof( pStoreMsg->MsgID ) );
#ifndef DISABLE_MSGQUE_LOGGING
			lprintf( WIDE("[%s] Stored message data..... at %") _size_f WIDE(" %") _size_f WIDE(""), pmq->name, pmq->Top ,size );
#  ifndef DISABLE_MSGQUE_LOGBINARY
			LogBinary( (uint8_t*)pStoreMsg, size + sizeof( pStoreMsg->MsgID ) + offsetof( MSGDATA, MsgID ) );
#  endif
#endif
			msg = NULL;
#ifndef DISABLE_MSGQUE_LOGGING
			lprintf( WIDE("Update top to %") _size_f,tmp );
#endif
			pmq->Top = tmp;
			if( !(opts & MSGQUE_WAIT_ID) )
			{
				// look for, and wake anyone waiting for this
				// type of message... or anyone waiting on any message
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				lprintf( WIDE("not sending a wait, therefore scan for messages...") );
#endif
				nWaiting = ScanForWaiting( pmq, pStoreMsg->MsgID );
				LeaveCriticalSec( &pmq->cs );
				if( nWaiting )
					Relinquish();
			}
			else
			{
				//lprintf( WIDE("Okay then we leave here?") );
				LeaveCriticalSec( &pmq->cs );
			}
		}
		// return success
		return 0;
	}
	// errno = EINVAL;
	return -1; // fail if no pmh
}

	//--------------------------------------------------------------------------

int IsMsgQueueEmpty ( PMSGHANDLE pmh )
{
	PMSGQUEUE pmq = pmh->pmq;
	if( !pmq || ( pmq->Bottom == pmq->Top ) )
		return TRUE;
	return FALSE;
}

//--------------------------------------------------------------------------

// if this thread id known, you may change the MsgID
// being waited for, which will result in this waking up
// and reading for the new ID...
int DequeMsgEx ( PMSGHANDLE pmh, long *MsgID, POINTER result, size_t size, uint32_t options DBG_PASS )
{
	PMSGQUEUE pmq = pmh->pmq;
	int p;
	int slept = 0;
	INDEX tmp
		, _tmp
	;
	uint32_t now = timeGetTime();
	INDEX _Bottom, _Top;

	//uint64_t tick, tick2;
	if( !pmq )
		return 0;
	// if there's a read routine, this should not be called.
	// instead the routine to handle
	p = 0;
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	_xlprintf(3 DBG_RELAY)( WIDE("[%s] Enter dequeue... for %") _32f WIDE(""), pmq->name, MsgID?*MsgID:0 );
#endif
	EnterCriticalSec( &pmq->cs );
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
	_xlprintf(3 DBG_RELAY)( WIDE("Deque space left... Top:%d Bottom:%d Avail: %d %d used: %d %d")
			 , pmq->Top, pmq->Bottom
			 , pmq->Bottom-pmq->Top, pmq->Size-pmq->Top + pmq->Bottom
			 , pmq->Top-pmq->Bottom, pmq->Size-pmq->Bottom + pmq->Top );
#endif
	_Bottom = INVALID_INDEX;
	_Top = INVALID_INDEX;
	while( !p && !slept )
	{
		PTHREADMSGDATA pThreadMsg = NULL;
		PMSGDATA pReadMsg;
		PMSGDATA pLastReadMsg;
		_tmp = tmp = pmq->Bottom;
		//lprintf( WIDE("tmp = %d"), tmp );
		if( !(options & MSGQUE_NOWAIT) )
		{
			long LastMsgID = *MsgID;
			// then here we must wait...
			// if the queue is empty, or we've already
			// checked the queue, go to sleep.
			while( ( ((_tmp = tmp),(tmp=pmq->Bottom)) == pmq->Top ||
					(pmq->Bottom == _Bottom &&
					pmq->Top == _Top )) && !slept )
			{
				//lprintf( WIDE("no message, waiting...") );
				{
					uint32_t tmp_top = pmq->waiter_top + 1;
					if( tmp_top >= 1024 )
						tmp_top = 0;

#if 0
					// do a scan to see if already waiting...
					// but since we're single process... there may be multipel threads
					// interacting here?  one on the server side reading, one on the client
					// but still only one ID for that queue should be waiting for
					// any specific message....
					// not sure why this doesn't work to avoid redundant wakeups
					tmp_top = pmq->waiter_bottom;
					while( tmp_top != pmq->waiter_top )
					{
						//lprintf( WIDE("Checking %d msg:%d"), tmp_top, pmq->waiters[tmp_top].msg );
						if( pmq->waiters[tmp_top].msg == *MsgID )
						{
							//lprintf( WIDE("waiting... leave...") );
							break;
						}
						tmp_top++;
						if( tmp_top >= 1024 )
							tmp_top = 0;
					}
#else
					tmp_top = pmq->waiter_top;
#endif
					// if waiter for message is already registered...
					// do not mark him.
					if( tmp_top == pmq->waiter_top )
					{
						tmp_top++;
						if( tmp_top >= 1024 )
							tmp_top = 0;
						if( tmp_top != pmq->waiter_bottom )
						{
							pmq->waiters[pmq->waiter_top].me = GetMyThreadID();
							pmq->waiters[pmq->waiter_top].msg = *MsgID;
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
							lprintf( WIDE("New waiter - waiting for %016") _64fX WIDE(" %") _32f WIDE("")
									 , pmq->waiters[pmq->waiter_top].me
									 , pmq->waiters[pmq->waiter_top].msg );
#endif
							pmq->waiter_top = tmp_top;
						}
						else
						{
							lprintf( WIDE("CRITICAL ERROR - No space to mark this process to wait.") );
							Relinquish();
							continue;
						}
					}
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
					else
						lprintf( WIDE("Already waiting...") );
					DumpWaiterQueue( pmq );
#endif
				}
				LeaveCriticalSec( &pmq->cs );
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				lprintf( WIDE("(left section) sleeping until message (%016Lx)")
						 , GetMyThreadID() );
#endif
				// if someone wakes wakeable sleep - either a> there's a new message
				// or b> someone wants to wakeup a process from Idle()... and we need to return
				slept = 1;
				WakeableSleep( SLEEP_FOREVER );
				// remove wait message...
#ifndef DISABLE_MSGQUE_LOGGING_DETAILED
				lprintf( WIDE("Re-enter critical section here...(%016Lx)")
						 , GetMyThreadID() );
#endif
				EnterCriticalSec( &pmq->cs );
				CollapseWaiting( pmq, LastMsgID );
				if( (*MsgID) == 0xFFFFFFFF )
				{
					lprintf( WIDE( "Aborting waiting read..." ) );
					SetLastError( MSGQUE_ERROR_EABORT );
					break;
				}
				//pmq->waiting = prior;
			}
#ifndef DISABLE_MSGQUE_LOGGING
			//lprintf( WIDE("Fetching a message...") );
#endif
			now = timeGetTime();
		}
		else
		{
			// if the queue is empty, then result now with no message.
			if( tmp == pmq->Top )
			{
#ifndef DISABLE_MSGQUE_LOGGING
				lprintf( WIDE("[%s] NOWAIT option selected... resulting NOMSG."), pmq->name );
#endif
				SetLastError( MSGQUE_ERROR_NOMSG );
				LeaveCriticalSec( &pmq->cs );
				return -1;
			}
		}
#ifndef DISABLE_MSGQUE_LOGGING
		_xlprintf( 1 DBG_RELAY )( WIDE("------- tmp = %") _size_f WIDE(" bottom=%") _size_f WIDE(" top = %") _size_f WIDE(" ------"), tmp, pmq->Bottom, pmq->Top );
#endif
		pLastReadMsg = NULL;

		while( tmp != pmq->Top )
		{
			// after returning a message, the next should be checked
			// 2 conditions - if the length == MARK_END_OF_QUE or
			// length & 0x40000000 then the next message needs to
			// be consumed until said condition is not set.
			pReadMsg = (PMSGDATA)(pmq->data + tmp);
#ifndef DISABLE_MSGQUE_LOGGING
			//lprintf( WIDE("Check for a message at %d (%08lx)"), tmp, pReadMsg->msg.length );
			//LogBinary( (POINTER)pReadMsg, pReadMsg->msg.real_length + sizeof( pReadMsg->MsgID ) + sizeof( MSGDATA ));
#endif
			if( pReadMsg->msg.ttl < now )
			{
				lprintf( WIDE("Message TTL Expired in queue... %d %d %d"), pReadMsg->msg.ttl, now, now - pReadMsg->msg.ttl );
				LogBinary( (uint8_t*)pReadMsg, pReadMsg->msg.length & ACTUAL_LEN_MASK );
				pReadMsg->msg.length |= MARK_MESSAGE_ALREADY_READ;
			}
			if( pReadMsg->msg.length & MARK_MESSAGE_ALREADY_READ )
			{
#ifndef DISABLE_MSGQUE_LOGGING
				//lprintf( WIDE("Message has already been read...") );
#endif
				if( tmp == pmq->Bottom )
				{
					if( pReadMsg->msg.length & MARK_END_OF_QUE )
					{
						SetPos( tmp, 0 );
						pmq->Bottom = tmp;
					}
					else
					{
						UpdatePos( tmp, pReadMsg->msg.length & ACTUAL_LEN_MASK );
						pmq->Bottom = tmp;
					}
				}
				else
				{
					// next is start.
					if( pReadMsg->msg.length & MARK_END_OF_QUE )
					{
#ifndef DISABLE_MSGQUE_LOGGING
						lprintf( WIDE("Looking for a message %") _size_f WIDE("...at %") _size_f WIDE(" haven't found one yet. (%") _size_f WIDE(")"), *MsgID, tmp, (pReadMsg->msg.length + sizeof( MSGCORE )) & ACTUAL_LEN_MASK );
#endif
						SetPos( tmp, 0 );
					}
					else
					{
#ifndef DISABLE_MSGQUE_LOGGING
						lprintf( WIDE("Looking for a message %") _size_f WIDE("...at %") _size_f WIDE(" haven't found one yet. (%") _size_f WIDE(")"), *MsgID, tmp, (pReadMsg->msg.length + sizeof( MSGCORE )) & ACTUAL_LEN_MASK );
#  ifndef DISABLE_MSGQUE_LOGBINARY
						LogBinary( (uint8_t*)pReadMsg, (pReadMsg->msg.length + sizeof( MSGCORE )) & ACTUAL_LEN_MASK );
#  endif 
#endif
						UpdatePos( tmp, pReadMsg->msg.length & ACTUAL_LEN_MASK );
					}
				}
				// skip messages already read. (and/or throw them out
				// by updating bottom, we remove them from the queue
				// with no further consideration.
				if( pLastReadMsg && ( pLastReadMsg->msg.length & MARK_MESSAGE_ALREADY_READ ) )
				{
					lprintf( WIDE("Collapsing two already read messages prior length was %") _32f WIDE("(%") _size_f WIDE(") and %")_32f WIDE("(%") _size_f WIDE(") (%s)")
							 , pLastReadMsg->msg.length& ACTUAL_LEN_MASK
							 , (uintptr_t)pLastReadMsg-(uintptr_t)pmq->data
							 , pReadMsg->msg.length& ACTUAL_LEN_MASK
							 , (uintptr_t)pReadMsg-(uintptr_t)pmq->data
							 , (pReadMsg->msg.length&MARK_END_OF_QUE)?WIDE("end"):WIDE("") );
						// prior message was read; collapse this one into it.
					pLastReadMsg->msg.length |= (pReadMsg->msg.length & MARK_END_OF_QUE);
					pLastReadMsg->msg.length += (pReadMsg->msg.length & ACTUAL_LEN_MASK);
					lprintf( WIDE("Result in %")_32f WIDE(" %p   (tmp is %") _size_f WIDE(",t:%") _size_f WIDE(",b:%") _size_f WIDE(")"), pLastReadMsg->msg.length & ACTUAL_LEN_MASK
						, (POINTER)((uintptr_t)pmq->data + (pLastReadMsg->msg.length & ACTUAL_LEN_MASK))
						, tmp
						, pmq->Top
						, pmq->Bottom
						);
				}
				else
					pLastReadMsg = pReadMsg;
				continue;
			}
			// just looped around to kill deleted messages.
			if( p )
				break;

			if( pReadMsg->msg.length & MARK_THREAD_WAITING )
			{
#ifndef DISABLE_MSGQUE_LOGGING
				lprintf( WIDE("A thread is waiting message...") );
#endif
				// skip these... don't care on read?
				// well maybe we care if this is the
				// wait message of me, in which case I can
				// clean it up. It's likely the first message
				// in the queue when I get awoke, it may be
				// early - but all other near messages will
				// likely also be thread wakes...
				if( ((PTHREADMSGDATA)pReadMsg)->thread == GetMyThreadID() )
				{
					// retest this current message as already read.
#ifndef DISABLE_MSGQUE_LOGGING
					lprintf( WIDE("And it's the message that denoted *I* was waiting... delete please") );
#endif
					if( tmp != pmq->Bottom )
					{
						PTHREADMSGDATA pTmpMsg = (PTHREADMSGDATA)(pmq->data + pmq->Bottom);
						if( pTmpMsg->msgdata.msg.length & MARK_THREAD_WAITING )
						{
							((PTHREADMSGDATA)pReadMsg)->thread = pTmpMsg->thread;
							((PTHREADMSGDATA)pReadMsg)->msgdata.MsgID = pTmpMsg->msgdata.MsgID;
							lprintf( WIDE("Mark message as having been read( should be a temporary wait message...") );
							pTmpMsg->msgdata.msg.length |= MARK_MESSAGE_ALREADY_READ;
						}
						else
						{
							lprintf( WIDE("First message in queue is not a thread wait?!") );
							pReadMsg->msg.length |= MARK_MESSAGE_ALREADY_READ;
						}
					}
					else
					{
						lprintf( WIDE("Mark message as having been read( should be a temporary wait message...") );
						pReadMsg->msg.length |= MARK_MESSAGE_ALREADY_READ;
					}
					// and now move forward still...
					UpdatePos( tmp, pReadMsg->msg.length & ACTUAL_LEN_MASK );
					pLastReadMsg = pReadMsg;
					continue;
				}
				if( !pThreadMsg )
				{
					pThreadMsg = (PTHREADMSGDATA)pReadMsg;
				}
				else
				{
					// concatentate this new one in the old one.
					// assuming there's space...
				}
				UpdatePos( tmp, pReadMsg->msg.length & ACTUAL_LEN_MASK );
				pLastReadMsg = pReadMsg;
				continue;
			}
			if( !(*MsgID) || ( pReadMsg->MsgID == (*MsgID) ) )
			{
				if( size > ( pReadMsg->msg.length & ACTUAL_LEN_MASK ) )
				{
					MemCpy( result
							, &pReadMsg->MsgID
							, pReadMsg->msg.real_length + sizeof( pReadMsg->MsgID ) );
					p = pReadMsg->msg.real_length;
#ifndef DISABLE_MSGQUE_LOGGING
					lprintf( WIDE("DequeMessage [%p] %")_MsgID_f WIDE(" len %") _size_f , result, *(MSGIDTYPE*)result, p+sizeof( pReadMsg->MsgID ) );
#  ifndef DISABLE_MSGQUE_LOGBINARY
					LogBinary( (uint8_t*)result, p+sizeof( pReadMsg->MsgID ) );
#  endif
#endif
					pReadMsg->msg.length |= MARK_MESSAGE_ALREADY_READ;
				}
				else
				{
#ifdef __LINUX__
					errno = E2BIG;
#endif
					return -1;
				}
				//lprintf( WIDE("...") );
				continue; // reprocess this mesage...
			}
			else if( *MsgID )
			{
#ifndef DISABLE_MSGQUE_LOGGING
				lprintf( WIDE("Looking for a message %")_MsgID_f WIDE("...at %") _size_f WIDE(" haven't found one yet."), *MsgID, tmp );
#  ifndef DISABLE_MSGQUE_LOGBINARY
				LogBinary( (uint8_t*)pReadMsg, (pReadMsg->msg.length + sizeof( MSGCORE )) & ACTUAL_LEN_MASK );
#  endif
#endif
				pLastReadMsg = pReadMsg;
				if( pReadMsg->msg.length & MARK_END_OF_QUE )
				{
					SetPos( tmp, 0 );
					pLastReadMsg = NULL;
				}
				else
					UpdatePos( tmp, ( pReadMsg->msg.length & ACTUAL_LEN_MASK ) );
				continue;
			}
		}

		if( !p )
		{
#ifndef DISABLE_MSGQUE_LOGGING
			lprintf( WIDE("No message found... looping...") );
#endif
			if( options & MSGQUE_NOWAIT )
			{
				lprintf( WIDE("Retunign - not looping...err uhh...") );
				SetLastError( MSGQUE_ERROR_NOMSG );
				LeaveCriticalSec( &pmq->cs );
				return -1;
			}
			_Bottom = pmq->Bottom;
			_Top = pmq->Top;
		}
	}
	LeaveCriticalSec( &pmq->cs );
	if( !p )
	{
		SetLastError( MSGQUE_ERROR_NOMSG );
		return -1;
	}
	return p;
}

#ifdef __cplusplus
}; //	namespace message {
}; // namespace containers {
}; //namespace sack {
#endif

