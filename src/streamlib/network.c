#include <stdhdrs.h>
#include <sharemem.h>
#include <network.h>
#include <timers.h>

#include "streamstruct.h"
#include "filters.h"

#include "global.h"
extern GLOBAL g;

#define NL_DEVICE 0

typedef struct frame_collect_tag {
	INDEX length;
   INDEX real_length;
	P_8 bitstream;
   INDEX seg_count;
	_32 *segments;
} FRAME_COLLECT, *PFRAME_COLLECT;

//#define PACKET_SIZE (1472 - (sizeof( PORTION ) ))
#define PACKET_SIZE (1316 - (sizeof( PORTION ) ))
typedef struct portion_tag {
	INDEX length;
	INDEX part;
   INDEX parts;
	INDEX pos;
   INDEX segment_length;
	INDEX frame;
	INDEX channel;
	char data[];
} PORTION, *PPORTION;

typedef struct frame_portion_tag
{
	INDEX frame;
	_32 time;
   PLIST portions;
} FRAME_PORTION, *PFRAME_PORTION;


typedef struct network_device_data_tag {
	PCLIENT socket; // listener - this is a capture device ....
	CRITICALSECTION cs_busy;
   char *buffer;
	int last_frame;
   PLIST portions;
	PLINKQUEUE frames;
   PLINKQUEUE packets;
	SIMPLE_QUEUE done, ready;
   PFRAME_COLLECT processing;
   // the current frame being collected
	PFRAME_COLLECT collecting;
   PFRAME_COLLECT condensing; // missing segments...
} NETDATA, *PNETDATA;

void InitCollection( PFRAME_COLLECT pfc, INDEX length, INDEX frags )
{
	if( pfc->real_length < length )
	{
		pfc->real_length = length;
		if( pfc->bitstream )
         Release( pfc->bitstream );
		pfc->bitstream = NewArray( _8, length );
		if( pfc->segments )
         Release( pfc->segments );
		pfc->segments = (unsigned long*)Reallocate( pfc->segments
										  , FLAGSETSIZE( pfc->segments[0]
															, frags ) );
	}
   pfc->seg_count = frags;
	MemSet( pfc->segments
			, 0
			, FLAGSETSIZE( pfc->segments[0], frags ) );
	pfc->length = length;
}

PFRAME_COLLECT NewCollection( void )
{
	PFRAME_COLLECT pfc = New( FRAME_COLLECT );
   pfc->real_length = 0;
	pfc->length = 0;
	pfc->bitstream = NULL;
	pfc->segments = NULL;
   return pfc;
}

int FinishCollection( PFRAME_COLLECT pfc )
{
	int n;
   int failed = 0;
	for( n = 0; n < pfc->seg_count; n++ )
	{
		if( !TESTFLAG( pfc->segments, n ) )
		{
			lprintf( WIDE("Failed bit %d of %d"), n, pfc->seg_count );
         failed++;
			//return 0;
		}
	}
	if( failed )
      return 0;
   return 1;
}

void CPROC ReadComplete( PCLIENT pc, POINTER buffer, int size, SOCKADDR *source )
{
   PNETDATA pnd = (PNETDATA)GetNetworkLong( pc, NL_DEVICE );
	PFRAME_COLLECT pfc;
   if( pnd )
	{
		PPORTION portion = (PPORTION)buffer;
      lprintf( WIDE("Received portion frame %ld part %ld of %ld"), portion->frame, portion->part, portion->parts );
		if( pnd->last_frame != portion->frame )
		{
         lprintf( WIDE("new frame... process the old one.") );
			if( pnd->collecting != (PFRAME_COLLECT)INVALID_INDEX )
			{
				if( FinishCollection( pnd->collecting ) )
				{
					EnqueFrame( &pnd->ready, (PTRSZVAL)pnd->collecting );
					pnd->collecting = NULL;
				}
				else
				{
               lprintf( WIDE("Lost data in frame... consider reque...") );
				}
			}
			{
				pfc = (PFRAME_COLLECT)DequeFrame( &pnd->done );
				if( pfc == (PFRAME_COLLECT)INVALID_INDEX )
				{
               lprintf( WIDE("beginning a new collection.") );
					pfc = NewCollection();
				}
            lprintf( WIDE("INitializing collection") );
				InitCollection( pfc, portion->length, portion->parts );
				pnd->collecting = pfc;
            pnd->last_frame = portion->frame;
			}
		}
      else
			pfc = pnd->collecting;
      lprintf( WIDE("Received portion frame %ld part %ld of %ld"), portion->frame, portion->part, portion->parts );
		MemCpy( pfc->bitstream + portion->pos
				, portion->data
				, portion->segment_length );
		//lprintf( WIDE("received %d %d %d"), PACKET_SIZE, portion->pos, portion->pos/PACKET_SIZE );
		SETFLAG( pfc->segments
				 , portion->pos/PACKET_SIZE );
		//lprintf( WIDE("read %d bytes"), size );

      // frame ready...
		ReadUDP( pc, pnd->buffer, 5000 );
	}
}

int CPROC GetNetworkCapturedFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
	PNETDATA pDevData = (PNETDATA)psv;
	if( pDevData->processing != (PFRAME_COLLECT)INVALID_INDEX )
		EnqueFrame( &pDevData->done, (PTRSZVAL)pDevData->processing );
   pDevData->processing = (PFRAME_COLLECT)DequeFrame( &pDevData->ready );
	if( pDevData->processing != (PFRAME_COLLECT)INVALID_INDEX )
	{
      PFRAME_COLLECT pfc = (PFRAME_COLLECT)pDevData->processing;
		SetDeviceData( pDevice
						 , pfc->bitstream
						 , pfc->length );
      return 1;
	}
   return 0;
}


PTRSZVAL OpenNetworkCapture( char *name )
{
	PNETDATA pnd = New( NETDATA );
	MemSet( pnd, 0, sizeof( NETDATA ) );
	pnd->socket = ServeUDP( NULL, 16661, ReadComplete, NULL );
	pnd->buffer = NewArray( char, 5000 );
	pnd->done.size = 5;
   pnd->done.frames = (_32*)Allocate( sizeof( PTRSZVAL) * pnd->done.size );
	pnd->ready.size = 5;
   pnd->ready.frames = (_32*)Allocate( sizeof( PTRSZVAL) * pnd->done.size );
	pnd->processing = (PFRAME_COLLECT)INVALID_INDEX;
	pnd->collecting = (PFRAME_COLLECT)INVALID_INDEX;
   pnd->frames = CreateLinkQueue();

	SetNetworkLong( pnd->socket, NL_DEVICE, (PTRSZVAL)pnd );
   ReadUDP( pnd->socket, pnd->buffer, 5000 );
   return (PTRSZVAL)pnd;
}

void CPROC RenderRequest( PCLIENT pc, POINTER buffer, int length, SOCKADDR *sa )
{
// this is a request for missing frame portions...
   lprintf( WIDE("Request for missing portions.") );
}

void CPROC ExpireFrames( PTRSZVAL psv )
{
	PNETDATA pnd = (PNETDATA)psv;
	PFRAME_PORTION pfp;
	for( pfp = (PFRAME_PORTION)PeekQueue( pnd->frames );
		 ( pfp->time + 500 < GetTickCount() );
		  pfp = (PFRAME_PORTION)PeekQueue( pnd->frames ) )
	{
		INDEX idx;
      PPORTION pp;
		DequeLink( &pnd->frames );
		LIST_FORALL( pfp->portions, idx, PPORTION, pp )
		{
			EnqueLink( &pnd->packets, pp );
		}
      DeleteList( &pfp->portions );
      Release( pfp );
	}
}

PTRSZVAL CPROC OpenNetworkRender( char *name )
{
	PNETDATA pnd = New( NETDATA );
	MemSet( pnd, 0, sizeof( NETDATA ) );
	{
		SOCKADDR *sa = CreateLocal( 16660 );
		pnd->socket = ConnectUDPAddr( sa, g.saBroadcast, RenderRequest, NULL );
	}
	pnd->processing = (PFRAME_COLLECT)INVALID_INDEX;
   pnd->collecting = (PFRAME_COLLECT)INVALID_INDEX;
	pnd->portions = CreateList();
	UDPEnableBroadcast( pnd->socket, TRUE );
   return (PTRSZVAL)pnd;
}



int CPROC RenderNetworkFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice )
{
	PNETDATA pnd = (PNETDATA)psv;
	char *data;
	static PORTION portion;
	INDEX part = 0;
	portion.frame++;
	portion.channel = 5;
	GetDeviceData( pDevice, (void**)&data, &portion.length );
   portion.parts = (portion.length + PACKET_SIZE -1 ) / PACKET_SIZE;
	for( portion.pos = 0;
		 portion.pos < portion.length;
		 portion.pos += PACKET_SIZE )
	{
		PPORTION new_portion;
		new_portion = (PPORTION)DequeLink( &pnd->packets );
		if( !new_portion )
		{
			new_portion = (PPORTION)Allocate( 2000 );
		}
		(*new_portion) = portion;
		new_portion->segment_length = PACKET_SIZE;
      new_portion->part = part++;
		if( portion.length - portion.pos < PACKET_SIZE )
			new_portion->segment_length = portion.length - portion.pos;
      //lprintf( WIDE("Send %d %d"), PACKET_SIZE, new_portion->segment_length );
		MemCpy( new_portion->data, data + portion.pos, PACKET_SIZE );
      AddLink( &pnd->portions, new_portion );
		SendUDP( pnd->socket, new_portion, PACKET_SIZE );
      //Relinquish();
	}
   //lprintf( WIDE("----------------------------------") );
	{
		PFRAME_PORTION frame_portion = New( FRAME_PORTION );
		frame_portion->portions = pnd->portions;
		frame_portion->frame = portion.frame;
      frame_portion->time = GetTickCount(); // expiration date...
		EnqueLink( &pnd->frames, frame_portion );
      pnd->portions = CreateList();
	}

   return 1;
}

