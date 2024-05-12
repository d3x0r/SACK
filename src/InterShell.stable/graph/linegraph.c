

#include "linegraph.h"


#ifdef __cplusplus
extern
#endif
CONTROL_REGISTRATION graph_control_reg;

enum {
	LST_GRAPH_TYPE = 1000
	  , BTN_NEW_TARGET
	  , BTN_ADD_LINE
	  , BTN_REMOVE_LINE
	  , LST_TARGET_NAMES
	  , LST_LINES_ACTIVE // list of active lines, and their target
	  , CLR_LINE_COLOR
     , EDIT_GRAPH_TIMESPAN
};


struct graph_line_subtype_struct
{
	GRAPH_LINE_TYPE parent;
} ;


struct graph_line_type_struct
{
	PLIST children; // GRAPH_LINE_TYPE list... children types of this graph type
	struct graph_line_type_struct *parent; // parent of this type (if subtype)
	CTEXTSTR name;
	RenderFunction render;
	InstanceFunction instance; // passed the target address (string)
	TimerProc timer;
	DestroyFunction destroy;
};

struct graph_line_parent_struct
{
	uintptr_t psvInstance; // common instance shared by all lines to this target...
};

struct graph_line_struct
{
	CDATA color;
	PDATALIST points; // list of GRAPH_LINE_SAMPLEs
	//PLINKQUEUE queue; // the queue of raw data that the line renderer uses...
	uintptr_t psvInstance;
	TARGET_ADDRESS target;
	GRAPH_LINE_TYPE type;
	uint32_t timer;
	PTHREAD thread;
	LOGICAL done;
};

struct graph_struct
{
	PSI_CONTROL pc; // actual control this is // control also points back at graph
	PLIST lines;  // list of GRAPH_LINE
	GRAPH_LINE current_line; // this is for editing interface purposes...
	PLIST targets; // targets that the graph is showing?
   uint32_t timespan; // milliseconds of history to show
} ;


uintptr_t CPROC FakeTimer( PTHREAD thread )
{
	GRAPH_LINE graph_line = (GRAPH_LINE)GetThreadParam( thread );
	uint32_t freq = 250;
	while( !graph_line->done )
	{
		if( graph_line->type )
			graph_line->type->parent->timer( graph_line->psvInstance );
		WakeableSleep( freq );
	}
	graph_line->thread = NULL;
	return 0;
}

uintptr_t CPROC FakeTimer2( PTHREAD thread )
{
	GRAPH_LINE graph_line = (GRAPH_LINE)GetThreadParam( thread );
	uint32_t freq = 250;
	while( !graph_line->done )
	{
      graph_line->type->timer( graph_line->psvInstance );
      WakeableSleep( freq );
	}
	graph_line->thread = NULL;
	return 0;
}

uintptr_t Instance( GRAPH graph, GRAPH_LINE graph_line, TARGET_ADDRESS target )
{
	if( graph_line->type->parent )
	{
		INDEX idx;
		GRAPH_LINE line;
		LIST_FORALL( graph->lines, idx, GRAPH_LINE, line )
			if( line->target == target && line->type->parent == graph_line->type->parent )
			{
				graph_line->psvInstance = line->psvInstance;
				graph_line->timer = line->timer;
            break;
			}

		if( !line )
		{
			graph_line->psvInstance = graph_line->type->parent->instance(target);
			if( graph_line->type->parent->timer )
			{
				graph_line->thread = ThreadTo( FakeTimer, (uintptr_t)graph_line );
				//graph_line->timer = AddTimer( 50, graph_line->type->parent->timer, graph_line->psvInstance );
			}
		}
	}
	else if( graph_line->type->instance )
	{
		if( graph_line->psvInstance = graph_line->type->instance( target ) )
		{
			if( graph_line->type->timer )
			{
				//graph_line->timer = AddTimer( 50, graph_line->type->timer, graph_line->psvInstance );
				graph_line->thread = ThreadTo( FakeTimer2, (uintptr_t)graph_line );
				//graph_line->timer = AddTimer( 50, graph_line->type->timer, graph_line->psvInstance );
			}
		}
	}
	return graph_line->psvInstance;;
}

void Destroy( GRAPH graph, GRAPH_LINE destroy_line )
{
   if( destroy_line->type->parent )
	{
		INDEX idx;
		GRAPH_LINE line;
		LIST_FORALL( graph->lines, idx, GRAPH_LINE, line )
		{
			if( line != destroy_line && line->psvInstance == destroy_line->psvInstance )
				break;
		}
		if( !line )
		{
			destroy_line->type->parent->destroy( destroy_line->psvInstance );
		}
	}
	else
	{
		destroy_line->type->destroy( destroy_line->psvInstance );
	}
}


typedef struct ping_result {
   uint32_t tick;
	uint32_t dwIP;
	//CTEXTSTR name; // each RDNS lookup is a thread!?
	int min;
	int max;
	int avg;
	int drop;
	int hops;
} PING_RESULT, *PPING_RESULT;

#define MAXPING_RESULTSPERSET 512
DeclareSet( PING_RESULT );

static struct {
	PLIST line_types; // PLIST of GRAPH_LINE_TYPEs (types of lines to show )
	PLIST targets; // PLIST of TARGET_ADDRESSs (list of servers to ping)
	PLIST graphs; // PLIST of GRAPHs  ( controls)
} l;


typedef struct ping_data_col PING_DATA_COLLECTOR, *PPING_DATA_COLLECTOR;
struct ping_data_col{
	TARGET_ADDRESS address;
	//PLINKQUEUE queue;
	PPING_RESULTSET samples;
};

uintptr_t InstancePing( TARGET_ADDRESS address )
{
	NEW( PING_DATA_COLLECTOR,result);
	result->address = address;
	return (uintptr_t)result;
}

void DestroyPing( uintptr_t psv )
{
	PPING_DATA_COLLECTOR pdc = (PPING_DATA_COLLECTOR)psv;
	DeleteSet( (GENERICSET**)&pdc->samples );
	Release( pdc );
}

TARGET_ADDRESS CreateTarget( CTEXTSTR address )
{
	TARGET_ADDRESS target = New( struct target_address );
	target->name = StrDup( address );
	target->addr = CreateSockAddress( address, 0 );
	if( !target->addr )
		Release( target );
	else
		AddLink( &l.targets, target );
	return target;
}



void PingResult( uintptr_t psv, SOCKADDR* dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops )
{
   static int dropped;
	PPING_DATA_COLLECTOR ppdc = (PPING_DATA_COLLECTOR)psv;
   //lprintf( "max result is %d", max );
	if( max )
	{
		PPING_RESULT result = GetFromSet( PING_RESULT, &ppdc->samples );
		lprintf( "Need to converet this address for ping result..." );
		result->dwIP = (uint32_t)dwIP;
		//result->name = StrDup( name );
		result->min = min;
		result->max = max;
		result->avg = avg;
		//lprintf( "setting sample drops to %d", dropped );
		result->drop = dropped;
		dropped = 0;
		result->hops = hops;
		result->tick = timeGetTime();
		//lprintf( "Added a result. at %ld %p %ld %ld %ld %ld %ld"
		//		 , result->tick, result
		//		 , min, max, avg, drop, hops, dwIP, result->tick);
	}
	else
	{
      //lprintf( "Increasing drop count..." );
		dropped++;
	}
}

void CPROC PingTimer( uintptr_t psv )
{
	PPING_DATA_COLLECTOR pdc = (PPING_DATA_COLLECTOR)psv;
	//static char pResult[4096];
	// pass PLINKQUEUE* to pdc...
	DoPingEx( pdc->address->name, 0, 150, 1, NULL/*pResult*/, FALSE, PingResult, (uintptr_t)pdc );
	//lprintf( "result text: %s", pResult );
}

GRAPH_LINE AddGraphLine( GRAPH graph, CTEXTSTR server, CTEXTSTR line_type, CTEXTSTR subtype )
{
	INDEX idx;
	TARGET_ADDRESS target;
	LIST_FORALL( l.targets, idx, TARGET_ADDRESS, target )
	{
		if( strcmp( target->name, server ) == 0 )
			break;
	}
	if( target )
	{
		GRAPH_LINE_TYPE glt;
		LIST_FORALL( l.line_types, idx, GRAPH_LINE_TYPE, glt )
		{
			if( strcmp( glt->name, line_type ) == 0 )
				break;
		}
		if( glt && subtype )
		{
			GRAPH_LINE_TYPE line_subtype;
			LIST_FORALL( glt->children, idx, GRAPH_LINE_TYPE, line_subtype )
			{
				if( strcmp( line_subtype->name, subtype ) == 0 )
               break;
			}
			if( line_subtype )
				glt = line_subtype; // create one of these, not the parent.
		}
		if( glt && !glt->children )
		{
			NEW( struct graph_line_struct, graph_line );
			graph_line->type = glt;
			graph_line->target = target;
			Instance( graph, graph_line, target );
			if( !graph_line->type || !graph_line->psvInstance )
				Release( graph_line );
			else
			{
				AddLink( &graph->lines, graph_line );
			}
			/* build a structure for the line...
			 * setup a timer for the data for the line.
			 * the graph needs to tick independantly of sampling so
			 * sample rate of each ping must be
			 * independant.  However, it is a single
			 */
			return graph_line;
		}
      // else it's a parent class, and it's invalid.
	}
	return NULL;
}

uintptr_t CPROC ReloadGraphTimespan( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, timespan );
	GRAPH graph = (GRAPH)psv;
	graph->timespan = (uint32_t)timespan;
	return psv;
}

uintptr_t CPROC ReloadGraphTarget( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, server );
	PARAM( args, CTEXTSTR, type );
	GRAPH graph = (GRAPH)psv;
	GRAPH_LINE line = AddGraphLine( graph, server, type, NULL );
	if( line )
		line->color = BASE_COLOR_WHITE;
	return psv;
}

uintptr_t CPROC NewReloadGraphTarget( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, server );
	PARAM( args, CTEXTSTR, type );
	PARAM( args, CDATA, color );
	GRAPH graph = (GRAPH)psv;
	GRAPH_LINE line = AddGraphLine( graph, server, type, NULL );
	if( line )
		line->color = color;
	return psv;
}

uintptr_t CPROC NewReloadGraphSubTarget( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, server );
	PARAM( args, CTEXTSTR, type );
	PARAM( args, CTEXTSTR, sub_type );
	PARAM( args, CDATA, color );
	GRAPH graph = (GRAPH)psv;
	GRAPH_LINE line = AddGraphLine( graph, server, type, sub_type );
	if( line )
		line->color = color;
	return psv;
}

uintptr_t CPROC ReloadTarget( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, server );
	CreateTarget( server );
	//GetAddressParts( target->addr, &target->dwIP, &target->port );
	return psv;
}

GRAPH_LINE_TYPE RegisterLineType( CTEXTSTR name, InstanceFunction i, DestroyFunction d, TimerProc t, RenderFunction f )
{
	NEW(struct graph_line_type_struct,line);
	line->name = StrDup( name );
	line->render = f;
	if( !i )
	{
		Release( line );
		return NULL;
	}
	line->instance = i;
	line->timer = t;
	line->destroy = d;
	AddLink( &l.line_types, line );
	return line;
}

GRAPH_LINE_TYPE RegisterLineClassType( CTEXTSTR class_name, InstanceFunction i, DestroyFunction d, TimerProc t )
{
	NEW(struct graph_line_type_struct,class_line);
	class_line->name = StrDup( class_name );
	class_line->render = NULL;
	if( !i )
	{
		Release( class_line );
		return NULL;
	}
	class_line->instance = i;
	class_line->timer = t;
	class_line->destroy = d;
	AddLink( &l.line_types, class_line );
	return class_line;
}

GRAPH_LINE_TYPE RegisterLineSubType( GRAPH_LINE_TYPE parent, CTEXTSTR name, RenderFunction f )
{
	NEW( struct graph_line_type_struct,line );
	line->render = f;
	line->name = StrDup( name );
	AddLink( &parent->children, line );
	line->parent = parent;
	return line;
}

void RenderPingMax( uintptr_t psv, PDATALIST *points, uint32_t from, uint32_t to, uint32_t resolution, uint32_t value_resolution )
{
   PPING_DATA_COLLECTOR ppdc = (PPING_DATA_COLLECTOR)psv;
	uint32_t tick;
	INDEX idx_sample;
	PPING_RESULT sample;
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
	sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, idx_sample = 0 );
	if( sample )
	{
		base_value = sample->max;
		max_value = 0;
		for( tick = from; sample && tick < to; )
		{
			while( sample && ( sample->tick < tick ) )
			{
				sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, ++idx_sample );
			}
			if( sample )
			{
				if( !min_index_set )
				{
					first_tick = tick;
					min_index = idx_sample;
					min_index_set = 1;
				}

				if( USS_GT( base_value, uint32_t, sample->max, int32_t ) )
					base_value = sample->max;
				if( USS_LT( max_value, uint32_t, sample->max, int ) )
					max_value = sample->max;
				tick = sample->tick + 1;  // find next sample
			}
			else
				break; // we're done.
		}
		base_value = 1000;
		max_value = 250000;
		//lprintf( "Min %d max %d", base_value, max_value );
		if( !min_index_set )
			return;
		sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, idx_sample = min_index );
		for( tick = from; sample && tick < to; )
		{
			GRAPH_LINE_SAMPLE point;
			while( sample && ( sample->tick < tick ) )
			{
				sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, ++idx_sample );
			}
			if( sample )
			{
				//lprintf( "tickofs %ld value %ld base %ld del %ld", sample->tick - from, sample->max, base_value, max_value );
				// now we're on the graph... prior was...
				point.offset =
					(
					 ( sample->tick - from ) // how much past the from that this is...
					 * resolution
					) / (to-from);
				point.value =
					(
					 ( sample->max - base_value )
					 * value_resolution
					) / ( (max_value - base_value) +1 );
				point.tick = sample->tick;
				//lprintf( "Added point... %ld %ld", point.offset, point.value );

				AddDataItem( points, &point );
            //lprintf( "End is %ld", (*points)->Cnt );
				tick = sample->tick + 1;  // find next sample
			}
			else
				break; // we're done.
		}
	}
}

void RenderPingDropped( uintptr_t psv, PDATALIST *points, uint32_t from, uint32_t to, uint32_t resolution, uint32_t value_resolution )
{
	PPING_DATA_COLLECTOR ppdc = (PPING_DATA_COLLECTOR)psv;
	uint32_t tick;
	INDEX idx_sample;
	PPING_RESULT sample;
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
	sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, idx_sample = 0 );
	if( sample )
	{
		base_value = sample->drop;
		max_value = 0;
		for( tick = from; sample && tick < to; )
		{
			while( sample && ( sample->tick < tick ) )
			{
				sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, ++idx_sample );
			}
			if( sample )
			{
				if( USS_GT( base_value,uint32_t, sample->drop, int ) )
					base_value = sample->drop;
				if( USS_LT( max_value,uint32_t, sample->drop, int ) )
					max_value = sample->drop;
				tick = sample->tick + 1;  // find next sample
			}
			else
				break; // we're done.
		}
		sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, idx_sample = 0 );
		for( tick = from; sample && tick < to; )
		{
			GRAPH_LINE_SAMPLE point;
			while( sample && ( sample->tick < tick ) )
			{
				sample = GetUsedSetMember( PING_RESULT, &ppdc->samples, ++idx_sample );
			}
			if( sample )
			{
				//lprintf( "tickofs %ld value %ld base %ld del %ld", sample->tick - from, sample->drop, base_value, drop_value );
				// now we're on the graph... prior was...
				point.offset =
					(
					 ( sample->tick - from ) // how much past the from that this is...
					 * resolution
					) / (to-from);
				point.value =
					(
					 ( sample->drop - base_value )
					 * value_resolution
					) / ( (max_value - base_value) +1 );
				point.tick = sample->tick;
				//lprintf( "Added point... %ld %ld", point.offset, point.value );

				AddDataItem( points, &point );
				//lprintf( "End is %ld", (*points)->Cnt );
				tick = sample->tick + 1;  // find next sample
			}
			else
				break; // we're done.
		}
	}
}

static void OnLoadCommon( "Ping Graph Registry" )( PCONFIG_HANDLER pch )
{
	// just use this to init our structures that we might want to use
	//RegisterLineType( "Ping Min", NULL );
	{
		GRAPH_LINE_TYPE type = RegisterLineClassType( "Ping", InstancePing, DestroyPing, PingTimer );
		RegisterLineSubType( type, "Max", RenderPingMax );
		RegisterLineSubType( type, "Dropped", RenderPingDropped );
	}
	RegisterLineType( "Ping Max", InstancePing, DestroyPing, PingTimer, RenderPingMax );
	//RegisterLineType( "Ping Avg", NULL );
	//RegisterLineType( "Ping Max Avg(short)", NULL );
	//RegisterLineType( "Ping Min Avg(short)", NULL );
	//RegisterLineType( "Ping Avg Avg(short)", NULL );
	SimpleRegisterResource( LST_GRAPH_TYPE, LISTBOX_CONTROL_NAME );
	SimpleRegisterResource( LST_TARGET_NAMES, LISTBOX_CONTROL_NAME );
	SimpleRegisterResource( LST_LINES_ACTIVE, LISTBOX_CONTROL_NAME );
	SimpleRegisterResource( CLR_LINE_COLOR, "Color Well" );
	SimpleRegisterResource( BTN_NEW_TARGET, NORMAL_BUTTON_NAME );
	SimpleRegisterResource( BTN_ADD_LINE, NORMAL_BUTTON_NAME );
	SimpleRegisterResource( BTN_REMOVE_LINE, NORMAL_BUTTON_NAME );
	SimpleRegisterResource( EDIT_GRAPH_TIMESPAN, EDIT_FIELD_NAME );

}

static void OnLoadControl( "Ping Status Graph" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "graph line server:\'%m\' status:\'%m\'", ReloadGraphTarget );
	AddConfigurationMethod( pch, "graph line server:\'%m\' status:\'%m\' sub: \'%m\' color:%c", NewReloadGraphSubTarget );
	AddConfigurationMethod( pch, "graph line server:\'%m\' status:\'%m\' color:%c", NewReloadGraphTarget );
	AddConfigurationMethod( pch, "graph timespan:%i", ReloadGraphTimespan );
}
static void OnSaveControl( "Ping Status Graph" )( FILE *file, uintptr_t psv )
{
	INDEX idx;
	GRAPH_LINE line;
	GRAPH graph = (GRAPH)psv;
	LIST_FORALL( graph->lines, idx, GRAPH_LINE, line )
	{
		TARGET_ADDRESS target = (TARGET_ADDRESS)line->target;
		if( line->type->parent )
			sack_fprintf( file, "graph line server:\'%s\' status:\'%s\' sub: \'%s\' color:$%"_32fX "\n", target->name, line->type->parent->name, line->type->name, line->color );
		else
			sack_fprintf( file, "graph line server:\'%s\' status:\'%s\' color:$%"_32fX "\n", target->name, line->type->name, line->color );
	}
	sack_fprintf( file, "graph timespan:%ld\n", graph->timespan );
}

static void OnLoadCommon( "Ping Status Graph Targets" )( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, "ping target=%m", ReloadTarget );
}

static void OnSaveCommon( "Ping Status Graph Targets" )( FILE *file )
{
	INDEX idx;
	TARGET_ADDRESS target;
	LIST_FORALL( l.targets, idx, TARGET_ADDRESS, target )
	{
		sack_fprintf( file, "ping target=%s\n", target->name );
	}
}

static void CPROC CreateATarget( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR buffer[256];
	TARGET_ADDRESS target;
	if( SimpleUserQuery( buffer, 256, "Enter new target address", NULL ) )
	{
		target = CreateTarget( buffer );
		if( target )
		{
			PSI_CONTROL list = GetNearControl( button, LST_TARGET_NAMES );
			SetItemData( AddListItem( list, target->name ), (uintptr_t)target );
		}
	}

}

static void CPROC ButtonAddGraphLine( uintptr_t psv, PSI_CONTROL button )
{
	GRAPH graph = (GRAPH)psv;
	NEW( struct graph_line_struct, graph_line );
	graph_line->type =
		(GRAPH_LINE_TYPE)GetItemData( GetSelectedItem( GetNearControl( button, LST_GRAPH_TYPE ) ) );
	if( !graph_line->type->render )
	{
      // fail creation, can't create a parent class (no renderer)
		Release( graph_line );
      return;
	}
	graph_line->target =
		(TARGET_ADDRESS)GetItemData( GetSelectedItem( GetNearControl( button, LST_TARGET_NAMES ) ) );
	if( !graph_line->target )
	{
		SimpleMessageBox( NULL, "Bad Selection", "Must Select a target Server" );
		Deallocate( struct graph_line_struct, graph_line );
		return;
	}
	Instance( graph, graph_line, graph_line->target );

	if( !graph_line->type || !graph_line->target || !graph_line->psvInstance )
		Release( graph_line );
	else
	{
		AddLink( &graph->lines, graph_line );
	}
	{
		PSI_CONTROL list = GetNearControl( button, LST_LINES_ACTIVE );
		TEXTCHAR value[64];
		snprintf( value, 64, "%s of %s", graph_line->type->name, graph_line->target->name );
		SetItemData( AddListItem( list, value ), (uintptr_t)graph_line );

	}
}

static void CPROC ButtonDeleteGraphLine( uintptr_t psv, PSI_CONTROL button )
{
	GRAPH graph = (GRAPH)psv;
	{
		PSI_CONTROL list;
		PLISTITEM pli = GetSelectedItem( list = GetNearControl( button, LST_LINES_ACTIVE ) );
		if( pli )
		{
			GRAPH_LINE graph_line = (GRAPH_LINE)GetItemData( pli );

			RemoveTimer( graph_line->timer );
			graph_line->done = TRUE;
			while( graph_line->thread )
			{
				WakeThread( graph_line->thread );
				Relinquish();
			}

			DeleteListItem( list, pli );
			graph->current_line = NULL;

			// common cleanup for graph_line here.
			Destroy( graph, graph_line );
			DeleteLink( &graph->lines, graph_line );
			Release( graph_line );

		}
	}
}

static void CPROC SelectCurrentLine( uintptr_t psv, PSI_CONTROL list, PLISTITEM pli )
{
	GRAPH graph = (GRAPH)psv;
	GRAPH_LINE line = (GRAPH_LINE)GetItemData( pli );
	graph->current_line = line;
	SetColorWell( GetNearControl( list, CLR_LINE_COLOR ), line->color );
 }

static void CPROC SetLineColor( uintptr_t psv, CDATA color )
{
	GRAPH graph = (GRAPH)psv;
	if( graph->current_line )
	{
		graph->current_line->color = color;
 	}
}

void FillLineTypeList( PSI_CONTROL list, PLIST line_type_list, INDEX level )
{
	if( list )
	{
		INDEX idx;
		GRAPH_LINE_TYPE glt;
		LIST_FORALL( line_type_list, idx, GRAPH_LINE_TYPE, glt )
		{
			SetItemData( AddListItemEx( list, level, glt->name ), (uintptr_t)glt );
			if( glt->children )
			{
				FillLineTypeList( list, glt->children, level+1 );
			}
		}
	}
}

static uintptr_t OnEditControl( "Ping Status Graph" )( uintptr_t psv, PSI_CONTROL parent )
{
	GRAPH graph = (GRAPH)psv;
	PSI_CONTROL frame = LoadXMLFrame( "ConfigurePingStatusGraph.isframe" );
	if( frame )
	{
		PSI_CONTROL list;
		list = GetControl( frame, LST_GRAPH_TYPE );
		SetListboxIsTree( list, TRUE );
		FillLineTypeList( list, l.line_types, 0 );

		list = GetControl( frame, LST_TARGET_NAMES );
		if( list )
		{
			INDEX idx;
			TARGET_ADDRESS target;
			LIST_FORALL( l.targets, idx, TARGET_ADDRESS, target )
			{
				SetItemData( AddListItem( list, target->name ), (uintptr_t)target );
			}
		}
		list = GetControl( frame, LST_LINES_ACTIVE );
		if( list )
		{
			INDEX idx;
			GRAPH_LINE line;
			SetSelChangeHandler( list, SelectCurrentLine, (uintptr_t)graph );
			LIST_FORALL( graph->lines, idx, GRAPH_LINE, line )
			{
				TEXTCHAR value[64];
				snprintf( value, 64, "%s of %s", line->type->name, line->target->name );
				SetItemData( AddListItem( list, value ), (uintptr_t)line );
			}
		}
		
		SetButtonPushMethod( GetControl( frame, BTN_NEW_TARGET ), CreateATarget, (uintptr_t)graph );
		SetButtonPushMethod( GetControl( frame, BTN_ADD_LINE ), ButtonAddGraphLine, (uintptr_t)graph );
		SetButtonPushMethod( GetControl( frame, BTN_REMOVE_LINE ), ButtonDeleteGraphLine, (uintptr_t)graph );
		EnableColorWellPick(
								  SetOnUpdateColorWell( GetControl( frame, CLR_LINE_COLOR ), SetLineColor, (uintptr_t)graph )
                          , TRUE );
		{
			TEXTCHAR timespan[32];
			snprintf( timespan, 32, "%ld.%03ld", graph->timespan / 1000, graph->timespan % 1000 );
			SetControlText( GetControl( frame, EDIT_GRAPH_TIMESPAN ), timespan );
		}
		{
			int done = 0;
			int okay = 0;
			SetCommonButtons( frame, &done, &okay );
			DisplayFrame( frame );
			CommonWait( frame );
			if( okay )
			{
				PLISTITEM pli;
				pli = GetSelectedItem( GetControl( frame, LST_TARGET_NAMES ) );
				pli = GetSelectedItem( GetControl( frame, LST_GRAPH_TYPE ) );
				{
					TEXTCHAR buffer[32];
					int32_t sec, secpart;
					int n;
					GetControlText( GetControl( frame, EDIT_GRAPH_TIMESPAN ), buffer, 32 );

					// timee parse result...

					lprintf( "WARNING CHEAP Time parse... incomplete, and failable" );
					n = sscanf( buffer, "%"_32f ".%" _32f, &sec, &secpart );
					if( n == 2 )
						graph->timespan = sec*1000 + secpart;
					else if( n == 1 )
						graph->timespan = sec*1000;

				}
			}
			DestroyFrame( &frame );
		}

	}
	return psv;
}

static PSI_CONTROL OnGetControl( "Ping Status Graph" )( uintptr_t psv )
{
	GRAPH graph = (GRAPH)psv;
	return graph->pc;
}

void CPROC UpdateGraph( uintptr_t psv )
{
	GRAPH graph = (GRAPH)psv;
	SmudgeCommon( graph->pc );
}

static uintptr_t OnCreateControl( "Ping Status Graph" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	//NEW(struct graph_struct,graph);
   //DebugBreak();
	PSI_CONTROL pc = MakeNamedControl( parent, "Graph Control", x, y, w, h, 0 );
	{
		ValidatedControlData( GRAPH, graph_control_reg.TypeID, graph, pc );
		graph->pc = pc;
	
		graph->timespan = 30000; // 30 seconds
		AddTimer( 30, UpdateGraph, (uintptr_t)graph );
		//DebugBreak();
		//SetControlData( GRAPH, graph->pc, graph );
		return (uintptr_t)graph;
	}
}

static void OnDestroyControl( "Ping Status Graph" )( uintptr_t psv )
{
	GRAPH graph = (GRAPH)psv;
	SetControlData( GRAPH, graph->pc, NULL );
	Release( graph );
}

void DrawLine( Image image, GRAPH graph, GRAPH_LINE line )
{
	//uint32_t tick;
	uint32_t max_tick = timeGetTime();
	uint32_t min_tick = max_tick - graph->timespan;
	uint32_t tick_err, tick_del;
	uint32_t width, height;
	//uint32_t x, _x;
	INDEX idx_sample;
	//INDEX nSample;
	GRAPH_LINE_SAMPLE *sample;
	GRAPH_LINE_SAMPLE *_sample = NULL;
	GetImageSize( image, &width, &height );
	tick_del = graph->timespan;
	tick_err = -width;

#define X_BORDER 10
#define Y_BORDER 12
	//lprintf( "Please consider the amount of work a function like this may do... (rendering line)" );
	line->type->render( line->psvInstance, &line->points, max_tick - graph->timespan, max_tick, width - (2*X_BORDER), height - (2*Y_BORDER) );

	// get first sample in queue
	DATA_FORALL( line->points, idx_sample, GRAPH_LINE_SAMPLE*, sample )
	{
		//lprintf( "point at %ld %ld", sample->offset, sample->value );
		if( _sample )
			do_line( image, _sample->offset + X_BORDER, height - Y_BORDER - _sample->value
					 , sample->offset + X_BORDER, height - Y_BORDER - sample->value
					 , line->color );
		else
			do_line( image, X_BORDER, height - Y_BORDER - sample->value
					 , sample->offset + X_BORDER, height - Y_BORDER - sample->value
					 , line->color );

		_sample = sample;
		plot( image
			 , sample->offset + X_BORDER
			 , ( height - Y_BORDER ) - sample->value
			 , BASE_COLOR_WHITE );
	}
}

// make sure to wait until the control data is set later
// the creator of this control actually assigned the GRAPH pointer.
static int CPROC OnDrawCommon( "Graph Control" )( PSI_CONTROL pc )
//int CPROC DrawGraph( PSI_CONTROL pc )
{
	ValidatedControlData( GRAPH, graph_control_reg.TypeID, graph, pc );
	if( graph )
	{
		Image image = GetControlSurface( pc );
		//lprintf( "Begin Graph" );
		ClearImageTo( image, SetAlpha( BASE_COLOR_DARKGREY, 128 ) );
		{
			int n;
			uint32_t w, h;
			GetImageSize( image, &w, &h );
			for( n = 0; n <= 10; n++ )
			{
				do_vline( image, ( ( ( w - 30 ) *n )/ 10 ) + 15, 0, h, BASE_COLOR_BLUE );
			}
		}
		{
			GRAPH_LINE line;
			INDEX idx;
			LIST_FORALL( graph->lines, idx, GRAPH_LINE, line )
			{
            //lprintf( "... %d", idx );
            DrawLine( image, graph, line );
            //lprintf( "..." );
			}
		}
	}
   return 1;
}

CONTROL_REGISTRATION graph_control_reg = { "Graph Control", {{256, 256}, sizeof( struct graph_struct ), BORDER_NORMAL|BORDER_INVERT|BORDER_FIXED}
													  , NULL
													  , NULL
													  , //DrawGraph
													  // , mouse
													  // somewhere here is a destroy method, can wait until the
                                         // to clear the control data?
};

PRELOAD( RegisterGraphControl )
{
   DoRegisterControl( &graph_control_reg );
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif

