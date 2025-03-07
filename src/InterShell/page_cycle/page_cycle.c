#include <stdhdrs.h>
#include <sqlgetoption.h>
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include "../intershell_export.h"
#include "../intershell_registry.h"
#include "../pages.h"

struct page_delay
{
	CTEXTSTR page_name;
	uint32_t delay;
};
enum {
	CHECKBOX_ENABLE = 2000
};
static struct page_cycle_local {
	struct {
		BIT_FIELD bDisableChange : 1;
		BIT_FIELD resources_registered : 1;
		BIT_FIELD allow_cycle : 1;
	} flags;
	int nPage;
	PLIST delays;
	uint32_t timer;
	uint32_t delay;
} l;

static void CPROC change( uintptr_t psv )
{
	if( l.flags.allow_cycle )
		if( !l.flags.bDisableChange )
			ShellSetCurrentPage( GetCanvas( (PSI_CONTROL)psv ), "next", PAGE_TRANSITION_NONE, 0 );
}

static void OnFinishInit( "Page Cycler" )( PCanvasData pc_canvas )
{
   l.timer = AddTimer( l.delay, change, (uintptr_t)pc_canvas );
}

static void OnEditModeBegin( "Page Cycler" )( void )
{
   l.flags.bDisableChange = 1;
}

static void OnEditModeEnd( "Page Cycler" )( void )
{
   l.flags.bDisableChange = 0;
}

static int OnChangePage( "Page Cycler" )( PCanvasData pc_canvas )
{
	PPAGE_DATA page = ShellGetCurrentPage( pc_canvas );
	INDEX idx;
	struct page_delay *delay;
   //lprintf( "new page = %s", page->title );
	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
		if( page->title )
		{
			if( StrCaseCmp( page->title, delay->page_name ) == 0 )
			{
            //lprintf( "Compare ok - schedule %d", delay->delay );
				RescheduleTimerEx( l.timer, delay->delay );
				break;
			}
		}
		else
			if( !delay->page_name )
			{
            //lprintf( "Compare ok - schedule %d", delay->delay );
				RescheduleTimerEx( l.timer, delay->delay );
				break;
			}
	}
	if( !delay )
	{
		delay = New( struct page_delay );
		delay->delay = l.delay;
		if( page->title )
			delay->page_name = StrDup( page->title );
		else
			delay->page_name = NULL;
      AddLink( &l.delays, delay );
		//lprintf( "Compare ok - schedule %d", l.delay );
		RescheduleTimerEx( l.timer, l.delay );
	}
   return 1;
}

static void OnGlobalPropertyEdit( "Page Cycle" )( PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "PageCycle.isFrame" );
	if( l.flags.resources_registered )
	{
      l.flags.resources_registered = 1;
		EasyRegisterResource( "InterShell/Page Cycle", CHECKBOX_ENABLE          , RADIO_BUTTON_NAME );
	}

	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
      SetCheckState( GetControl( frame, CHECKBOX_ENABLE ), l.flags.allow_cycle );
		//lprintf( "show frame over parent." );
		DisplayFrameOver( frame, parent );
		//lprintf( "Begin waiting..." );
		CommonWait( frame );
		if( okay )
		{
         l.flags.allow_cycle = GetCheckState( GetControl( frame, CHECKBOX_ENABLE ) );
			//UpdateFromControls( frame );
		}
		//ClearControls();
		DestroyFrame( &frame );
	}
}


static void OnSaveCommon( "Page Cycler" )( FILE *file )
{
	INDEX idx;
	struct page_delay *delay;
	fprintf( file, "Page Cycling is enabled=%s\n", l.flags.allow_cycle?"yes":"no" );
	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
		if( delay->page_name )
			fprintf( file, "page delay for '%s'=%d\n", delay->page_name, delay->delay );
	}
}

static uintptr_t CPROC LoadAllowCycle( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, enable );
	l.flags.allow_cycle = enable;
	return psv;
}
static uintptr_t CPROC LoadDelay( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PARAM( args, int64_t, delay_time );
	INDEX idx;
	struct page_delay *delay;
	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
		if( StrCaseCmp( name, delay->page_name ) == 0 )
			break;
	}
	if( delay )
	{
		delay->delay = (uint32_t)delay_time;
	}
	else
	{
		delay = New( struct page_delay );
		delay->page_name = StrDup( name );
		delay->delay = (uint32_t)delay_time;
		AddLink( &l.delays, delay );
	}
   return psv;
}

static void OnLoadCommon( "Page Cycler" )( PCONFIG_HANDLER pch )
{
	l.delay = SACK_GetPrivateProfileInt( "Page Cycler", "default delay between pages", 15000, "page_changer.ini" );
	AddConfigurationMethod( pch, "Page Cycling is enabled=%b", LoadAllowCycle );
	AddConfigurationMethod( pch, "page delay for '%m'=%i", LoadDelay );
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif

