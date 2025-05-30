
/* a copy of connect_stat, mangled to handle UDP instead of TCP */


#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <network.h>
#include <sharemem.h>

#include "linegraph.h"


static struct packet_stat_local {
	PCLIENT pcServer; // server echo port
	uint64_t server_buffer[4096];
	PCLIENT pcClient; // generate commands to server
	uint64_t client_buffer[4096];
	PLIST line_data; // UDP_DATA list
	PLIST clients;
}l;

static void CPROC ReadClient( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *source )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
	}
	else
	{
		uint32_t IP;
		uint16_t port;
		lprintf( "Sending buffer received back to source..." );
		GetAddressParts( source, &IP, &port );
		lprintf( "From %08lx %d", IP, port );
		SendUDPEx( pc, buffer, size, source );
	}
	ReadUDP( pc, l.server_buffer, 4096 );
}


typedef struct udp_sample {
	uint32_t tick;
	uint64_t cpu_tick_delta;
	uint32_t dropped;
} UDP_SAMPLE;

typedef struct udp_data {
	//uint64_t* buffer;
	TARGET_ADDRESS target;
	PDATAQUEUE queue; // list of application specific structures which it may wish to use to render
	PCLIENT client;
} UDP_DATA;
typedef struct udp_data *PUDP_DATA;

uintptr_t CreateUDPTarget( TARGET_ADDRESS target )
{
	NEW(UDP_DATA, udp_data);
	udp_data->target = target;
	//udp_data->buffer = (uint64_t*)Allocate( 4096 );
	// enough for a days worth of samples...
	udp_data->queue = CreateLargeDataQueue( sizeof( UDP_SAMPLE ), 200000 );
	lprintf( "result udp thing %p", udp_data );
	AddLink( &l.line_data, udp_data );
	return (uintptr_t)udp_data;
}

void DestroyUDPTarget( uintptr_t psv )
{
	PUDP_DATA udp_data = (PUDP_DATA)psv;
	if( udp_data->client )
		RemoveClient( udp_data->client );
	DeleteLink( &l.line_data, udp_data );
	//Release( udp_data->buffer );
	Release( udp_data );
}

void RenderUDPClient( uintptr_t psvRenderWhat // user data to pass to render to give render a thing to render
						  , PDATALIST *points // GRAPH_LINE_SAMPLE output points to draw
						  , uint32_t from // min tick
						  , uint32_t to // max tick
						  , uint32_t resolution  // width to plot
						  , uint32_t value_resolution // height to plot
						  )
{
	PUDP_DATA udp_data = (PUDP_DATA)psvRenderWhat;
	INDEX iSample;
	uint32_t tick = from;
	UDP_SAMPLE sample;
	uint64_t base_value, max_value;
	if( !points ) // nowhere to render to.
		return;
	if( !(*points) )
		(*points) = CreateDataList( sizeof( GRAPH_LINE_SAMPLE ) );
	{
		// no such thing
		EmptyDataList( points );
		//(*points)->Cnt = 0;
	}
	//lprintf( "emptied data list of points... checking samples" );
	base_value = (uint64_t)~0;
	max_value = 0;
	for( iSample = 0; tick < to && PeekDataQueueEx( &udp_data->queue, UDP_SAMPLE, &sample, iSample ); iSample++ )
	{
		if( sample.tick >= tick )
		{
			//lprintf( "Sample %Ld", sample.cpu_tick_delta );
			if( sample.cpu_tick_delta < base_value )
				base_value = sample.cpu_tick_delta;
			if( sample.cpu_tick_delta > max_value )
				max_value = sample.cpu_tick_delta;
			tick = sample.tick + 1;
		}
	}
	tick = from;
	for( iSample = 0; tick < to && PeekDataQueueEx( &udp_data->queue, UDP_SAMPLE, &sample, iSample ); iSample++ )
	{
		GRAPH_LINE_SAMPLE point;
		if( sample.tick >= tick )
		{
			point.offset =
					(
					 ( sample.tick - from ) // how much past the from that this is...
					 * resolution
					) / (to-from);
			point.value = (uint32_t)(
					(
					 ( sample.cpu_tick_delta - base_value )
					 * value_resolution
					) / ( (max_value - base_value) +1 ) );
			lprintf( "Base %Ld max %Ld val %Ld out %ld", base_value, max_value, sample.cpu_tick_delta, point.value );
			point.tick = sample.tick;
			tick = sample.tick + 1;
			AddDataItem( points, &point );
		}
	}
	lprintf( "done rendering..." );
}

void RenderUDPClientDrops( uintptr_t psvRenderWhat // user data to pass to render to give render a thing to render
								 , PDATALIST *points // GRAPH_LINE_SAMPLE output points to draw
								 , uint32_t from // min tick
								 , uint32_t to // max tick
								 , uint32_t resolution  // width to plot
								 , uint32_t value_resolution // height to plot
								 )
{
	PUDP_DATA udp_data = (PUDP_DATA)psvRenderWhat;
	INDEX iSample;
	uint32_t tick = from;
	UDP_SAMPLE sample;
	uint32_t base_value, max_value;
	if( !points ) // nowhere to render to.
		return;
	if( !(*points) )
		(*points) = CreateDataList( sizeof( GRAPH_LINE_SAMPLE ) );
	{
		// no such thing
		EmptyDataList( points );
		//(*points)->Cnt = 0;
	}
	//lprintf( "emptied data list of points... checking samples" );
	base_value = (uint64_t)~0;
	max_value = 0;
	DebugBreak();
	for( iSample = 0; tick < to && PeekDataQueueEx( &udp_data->queue, UDP_SAMPLE, &sample, iSample ); iSample++ )
	{
		if( sample.tick >= tick )
		{
			//lprintf( "Sample %Ld", sample.dropped );
			if( sample.dropped < base_value )
				base_value = sample.dropped;
			if( sample.dropped > max_value )
				max_value = sample.dropped;
			tick = sample.tick + 1;
		}
	}
	tick = from;
	for( iSample = 0; tick < to && PeekDataQueueEx( &udp_data->queue, UDP_SAMPLE, &sample, iSample ); iSample++ )
	{
		GRAPH_LINE_SAMPLE point;
		if( sample.tick >= tick )
		{
			point.offset =
					(
					 ( sample.tick - from ) // how much past the from that this is...
					 * resolution
					) / (to-from);
			point.value =
					(
					 ( sample.dropped - base_value )
					 * value_resolution
					) / ( (max_value - base_value) +1 );
			//lprintf( "Base %ld max %ld val %ld out %ld", base_value, max_value, sample.dropped, point.value );
			point.tick = sample.tick;
			tick = sample.tick + 1;
			AddDataItem( points, &point );
		}
	}
	//lprintf( "done rendering..." );
}

static void CPROC ClientReceive( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *addr )
{
	if( buffer )
	{
		uint64_t min_tick_in_packet = 0;
		uint64_t tick = GetCPUTick();
		int dropped = 0;
		INDEX idx;
		PUDP_DATA udp_data;
		LIST_FORALL( l.line_data, idx, PUDP_DATA, udp_data )
		{
			if( CompareAddress( udp_data->target->addr, addr ) )
			{
				break;
			}
		}
		{
			UDP_SAMPLE sample;
			sample.dropped = dropped;
			sample.tick = timeGetTime();
			sample.cpu_tick_delta = tick - l.client_buffer[0];
			lprintf( "hrm tick %Ld %Ld %Ld", tick, min_tick_in_packet, tick-min_tick_in_packet );
			EnqueData( &udp_data->queue, &sample );
		}
	}
	ReadUDP( pc, l.client_buffer, 4096 );

}

static void CPROC UDPTick( uintptr_t psv )
{
	PUDP_DATA udp_data = (PUDP_DATA)psv;
	{
		uint64_t tick = GetCPUTick();
		SendUDPEx( l.pcClient, &tick, sizeof( tick ), udp_data->target->addr );
	}
}



PRELOAD( RegisterConnectStats )
{

	GRAPH_LINE_TYPE line = RegisterLineClassType( "UDP Traffic", CreateUDPTarget, DestroyUDPTarget, UDPTick);
	RegisterLineSubType( line, "Latency", RenderUDPClient );
	RegisterLineSubType( line, "Drops", RenderUDPClientDrops );
	//RegisterLineType( "UDP Drops", NULL, NULL, NULL, RenderUDPDrop );
	//CreateLabelVariable( "UDP Min
}

// some render functions to build displayable sample data....

PRELOAD( InitConnectStatModule )
{
	// give ourselves lots of sockets, and some client data
	// network wait will reallocate data if more is requested by other
	// portions.

	// the network layer object itself has a very poor interface for
	// configuration... and that it wants to have configurations given
	// is a worse matter.
	NetworkWait( NULL, 256, 16 );
	l.pcServer = ServeUDP( NULL, 5632, ReadClient, NULL );
	l.pcClient = ServeUDP( NULL, 5631, ClientReceive, NULL );
}

