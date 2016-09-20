

#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <network.h>
#include <sharemem.h>

#include "linegraph.h"

struct {
	int an;
}blah;

static struct connect_stat_local {
	PCLIENT pcServer;
	PLIST clients;
}l;

void CPROC ReadClient( PCLIENT pc, POINTER buffer, size_t size )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
	}
	else
	{
		SendTCP( pc, (CPOINTER)buffer, size );
	}
	ReadTCP( pc, buffer, 4096 );
}

void CPROC ServerClosed( PCLIENT pc )
{
	lprintf( WIDE("Closed connection from someone... no tracking") );
}

void CPROC Connected( PCLIENT pcServer, PCLIENT pcListen )
{
	lprintf( WIDE("Received connection from someone...") );
	SetNetworkReadComplete( pcListen, ReadClient );
	SetTCPNoDelay( pcListen, TRUE );

}

PRELOAD( InitConnectStatModule )
{
	NetworkWait( NULL, 256, 16 );
	//l.pcServer = OpenTCPServerEx( 5832, Connected );

	// give ourselves lots of sockets, and some client data
	// network wait will reallocate data if more is requested by other
	// portions.

	// the network layer object itself has a very poor interface for
	// configuration... and that it wants to have configurations given
	// is a worse matter.
}

typedef struct tcp_sample {
	uint32_t tick;
	uint64_t cpu_tick_delta;
	uint32_t dropped;
} TCP_SAMPLE, *PTCP_SAMPLE;

#define MAXTCP_SAMPLESPERSET 512
DeclareSet( TCP_SAMPLE );

typedef struct tcp_data {
	struct {
		uint32_t bSend : 1;// allowed to send data
	} flags;
	uint64_t* buffer;
	TARGET_ADDRESS target;
	PTCP_SAMPLESET samples;
	PCLIENT client;
} TCP_DATA;
			 typedef TCP_DATA *PTCP_DATA;

uintptr_t CreateTCPTarget( TARGET_ADDRESS target )
{
	NEW(TCP_DATA, tcp_data);
	tcp_data->target = target;
	tcp_data->buffer = (uint64_t*)Allocate( 4096 );
	// enough for a days worth of samples...
	return (uintptr_t)tcp_data;
}

void DestroyTCPTarget( uintptr_t psv )
{
	PTCP_DATA tcp_data = (PTCP_DATA)psv;
	if( tcp_data->client )
		RemoveClient( tcp_data->client );
	Release( tcp_data->buffer );
	Release( tcp_data );
}

void RenderTCPClient( uintptr_t psvRenderWhat // user data to pass to render to give render a thing to render
						  , PDATALIST *points // GRAPH_LINE_SAMPLE output points to draw
						  , uint32_t from // min tick
						  , uint32_t to // max tick
						  , uint32_t resolution  // width to plot
						  , uint32_t value_resolution // height to plot
						  )
{
	PTCP_DATA tcp_data = (PTCP_DATA)psvRenderWhat;
	INDEX iSample;
	uint32_t tick = from;
	PTCP_SAMPLE sample;
	int min_index_set = 0;
	INDEX min_index;
	uint32_t first_tick;
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
	//lprintf( "check data... %d %d", tick, to );
	for( iSample = 0; tick < to && ( sample = GetUsedSetMember( TCP_SAMPLE, &tcp_data->samples, iSample ) ); iSample++ )
	{
		//GRAPH_LINE_SAMPLE point;
		//lprintf( "Sample at %ld vs %ld", sample->tick, tick );
		if( sample->tick >= tick )
		{
			if( !min_index_set )
			{
				first_tick = tick;
				min_index = iSample;
				min_index_set = 1;
			}
			//lprintf( "Sample %Ld", sample->cpu_tick_delta );
			if( sample->cpu_tick_delta < base_value )
				base_value = sample->cpu_tick_delta;
			if( sample->cpu_tick_delta > max_value )
				max_value = sample->cpu_tick_delta;
			tick = sample->tick + 1;
		}
	}
	tick = first_tick;
	//lprintf( "check data... %d %d", tick, to );
	for( iSample = min_index; tick < to && ( sample = GetUsedSetMember( TCP_SAMPLE, &tcp_data->samples, iSample ) ); iSample++ )
	{
		GRAPH_LINE_SAMPLE point;
		if( sample->tick >= tick )
		{
			point.offset =
					(
					 ( sample->tick - from ) // how much past the from that this is...
					 * resolution
					) / (to-from);
			point.value = (uint32_t) // truncate
					((
					 ( sample->cpu_tick_delta - base_value )
					 * value_resolution
					) / ( (max_value - base_value) +1 ));
			//lprintf( "Base %Ld max %Ld val %Ld out %ld", base_value, max_value, sample->cpu_tick_delta, point.value );
			point.tick = sample->tick;
			tick = sample->tick + 1;
			AddDataItem( points, &point );
		}
	}
	//lprintf( "done rendering..." );
}

void RenderTCPClientDrops( uintptr_t psvRenderWhat // user data to pass to render to give render a thing to render
								 , PDATALIST *points // GRAPH_LINE_SAMPLE output points to draw
								 , uint32_t from // min tick
								 , uint32_t to // max tick
								 , uint32_t resolution  // width to plot
								 , uint32_t value_resolution // height to plot
								 )
{
	PTCP_DATA tcp_data = (PTCP_DATA)psvRenderWhat;
	INDEX iSample;
	uint32_t tick = from;
	PTCP_SAMPLE sample;
	uint32_t base_value, max_value;
	int min_index_set = 0;
	INDEX min_index;
	uint32_t first_tick;
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
	for( iSample = 0; tick < to && ( sample = GetUsedSetMember( TCP_SAMPLE, &tcp_data->samples, iSample ) ); iSample++ )
	{
		//GRAPH_LINE_SAMPLE point;
		if( sample->tick >= tick )
		{
			if( !min_index_set )
			{
				first_tick = tick;
				min_index = iSample;
				min_index_set = 1;
			}
			//lprintf( "Sample %Ld", sample->dropped );
			if( sample->dropped < base_value )
				base_value = sample->dropped;
			if( sample->dropped > max_value )
				max_value = sample->dropped;
			tick = sample->tick + 1;
		}
	}
	tick = first_tick;
	for( iSample = min_index; tick < to && ( sample = GetUsedSetMember( TCP_SAMPLE, &tcp_data->samples, iSample ) ); iSample++ )
	{
		GRAPH_LINE_SAMPLE point;
		if( sample->tick >= tick )
		{
			point.offset =
					(
					 ( sample->tick - from ) // how much past the from that this is...
					 * resolution
					) / (to-from);
			point.value =
					(
					 ( sample->dropped - base_value )
					 * value_resolution
					) / ( (max_value - base_value) +1 );
			//lprintf( "Base %ld max %ld val %ld out %ld", base_value, max_value, sample->dropped, point.value );
			point.tick = sample->tick;
			tick = sample->tick + 1;
			AddDataItem( points, &point );
		}
	}
	//lprintf( "done rendering..." );
}

void CPROC ClientReceive( PCLIENT pc, POINTER buffer, size_t size )
{
	PTCP_DATA tcp_data = (PTCP_DATA)GetNetworkLong( pc, 0 );
	if( !buffer )
	{
		buffer = tcp_data->buffer;
	}
	else
	{
		uint64_t min_tick_in_packet;
		uint64_t tick = GetCPUTick();
		int dropped = 0;
		int first = 1;
		size_t ofs;
		for( ofs = 0; ofs < size; ofs += sizeof( tcp_data->buffer[0] ) )
		{
			if( !first )
			{
				dropped++;
			}
			else
				min_tick_in_packet = tcp_data->buffer[0];
		}
		if( ofs )
		{
			PTCP_SAMPLE sample;
			sample = GetFromSet( TCP_SAMPLE, &tcp_data->samples );
			sample->dropped = dropped;
			sample->tick = timeGetTime();
			sample->cpu_tick_delta = tick - min_tick_in_packet;
			lprintf( WIDE("hrm tick %p %Ld %Ld %Ld"), pc, tick, min_tick_in_packet, tick-min_tick_in_packet );
		}
		else
			lprintf( WIDE("zero ofs?") );
	}
	ReadTCP( pc, buffer, 4096 );

}

void CPROC ClientConnected( PCLIENT pc, int error )
{
	PTCP_DATA tcp_data;
	while( !(tcp_data = (PTCP_DATA)GetNetworkLong( pc, 0 ) ) )
	{
		//lprintf( "Waiting for tcp_data to be set..." );
		Relinquish();
	}
	// all is well.
	if( !error )
	{
		lprintf( WIDE("all is well, mark sendable...%s"), tcp_data->target->name );
		tcp_data->flags.bSend = 1;
		SetTCPNoDelay( pc, TRUE );
	}
	else
	{
		//lprintf( "Had an error..>" );
		RemoveClient( tcp_data->client ); // need to close this :)
	}
}

void CPROC ClientClosed( PCLIENT pc )
{
  PTCP_DATA tcp_data;
  tcp_data = (PTCP_DATA)GetNetworkLong( pc, 0 );
  if( tcp_data )
  {
	  //lprintf( "Client %s closed!", tcp_data->target->name );
	  tcp_data->flags.bSend = FALSE;
	  tcp_data->client = NULL;
  }
}

void CPROC TCPTick( uintptr_t psv )
{
	PTCP_DATA tcp_data = (PTCP_DATA)psv;
	if( !tcp_data->client )
	{
		//lprintf( "Queue connection to %s", tcp_data->target->name );
		tcp_data->client = OpenTCPClientAddrExx( tcp_data->target->addr, ClientReceive, ClientClosed, NULL, ClientConnected );
		SetNetworkLong( tcp_data->client, 0, (uintptr_t)tcp_data );
	}
	else if( tcp_data->flags.bSend )
	{
		uint64_t tick = GetCPUTick();
		//lprintf( "%p connection %s ready, sending current tick", tcp_data->client, tcp_data->target->name );
		SendTCP( tcp_data->client, (CPOINTER)&tick, sizeof( tick ) );
	}
}



PRELOAD( RegisterConnectStats )
{

	GRAPH_LINE_TYPE line = RegisterLineClassType( WIDE("TCP Traffic"), CreateTCPTarget, DestroyTCPTarget, TCPTick);
	RegisterLineSubType( line, WIDE("Latency"), RenderTCPClient );
	RegisterLineSubType( line, WIDE("Drops"), RenderTCPClientDrops );
	//RegisterLineType( "TCP Drops", NULL, NULL, NULL, RenderTCPDrop );
	//CreateLabelVariable( "TCP Min
}

// some render functions to build displayable sample data....


