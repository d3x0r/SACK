
#define USE_IMAGE_INTERFACE g.pImageInterface

#include <stdhdrs.h>
#include <configscript.h>
#include <intershell_registry.h>
#include <sharemem.h>
#include <timers.h>
#include <image.h>
#include <network.h>

#define g global_network_graph_data
typedef struct global_data GLOBAL;

struct global_data {
   PIMAGE_INTERFACE pImageInterface;
};
#ifndef GLOBAL_SOURCE
extern
#endif
GLOBAL g;

#define NEW(what,thing)  what *thing = New(what); MemSet( thing, 0, sizeof( what ) );
//what thing = Allocate( sizeof( *thing ) );   MemSet( thing, 0, sizeof( *thing ) );


typedef struct target_address
{
   TEXTCHAR *name;  // target name (text IP address)
   SOCKADDR *addr; // name converted to a network address
	//_32 dwIP; // dwIP used for PING result
   //_16 port; // useless, but free.
} *TARGET_ADDRESS;


typedef void (*RenderFunction)( PTRSZVAL psvRenderWhat // user data to pass to render to give render a thing to render
										, PDATALIST *points // GRAPH_LINE_SAMPLE output points to draw
										, _32 from // min tick
										, _32 to // max tick
										, _32 resolution  // width to plot
										, _32 value_resolution // height to plot
										);
// this is a string from somewhere...
// application can use it for whatever...
typedef PTRSZVAL (*InstanceFunction)( TARGET_ADDRESS string );
typedef void (CPROC *TimerProc)( PTRSZVAL psvTarget );
typedef void (*DestroyFunction)( PTRSZVAL psv);

typedef struct graph_line_struct *GRAPH_LINE;


typedef struct graph_line_sample_struct
{
	_32 offset; // sparse point support.
	_32 value;
	_32 tick;
} GRAPH_LINE_SAMPLE;


typedef struct graph_line_type_struct *GRAPH_LINE_TYPE;

typedef struct graph_struct *GRAPH;
//----------------------------------
// interface functions
//----------------------------------

GRAPH_LINE_TYPE RegisterLineType( CTEXTSTR name, InstanceFunction, DestroyFunction, TimerProc,RenderFunction f );
GRAPH_LINE_TYPE RegisterLineClassType( CTEXTSTR class_name, InstanceFunction, DestroyFunction, TimerProc );
GRAPH_LINE_TYPE RegisterLineSubType( GRAPH_LINE_TYPE parent, CTEXTSTR name, RenderFunction f );

