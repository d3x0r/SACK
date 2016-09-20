#include <stdhdrs.h>
#include <sqlgetoption.h>
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include "../intershell_export.h"
#include "../intershell_registry.h"
#include "../pages.h"
#include "page_cycle.h"

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
	PPAGE_DATA page = ShellGetCurrentPage( (PSI_CONTROL)psv );
	if( l.flags.allow_cycle )
	{
		CTEXTSTR name;
		PCLASSROOT data = NULL;
		for( name = GetFirstRegisteredName( TASK_PREFIX WIDE( "/page_cycle/query_page_cycle" ), &data );
			 name;
			  name = GetNextRegisteredName( &data ) )
		{
			LOGICAL (CPROC*f)(void);
			//snprintf( rootname, sizeof( rootname ), TASK_PREFIX WIDE( "/common/save common/%s" ), name );
			f = GetRegisteredProcedure2( (CTEXTSTR)data, LOGICAL, name, (void) );
			if( f )
				if( !f() )
					return;
		}
		if( !l.flags.bDisableChange )
		{
			int found = FALSE;
			INDEX idx;
			struct page_delay *delay;
			LIST_FORALL( l.delays, idx, struct page_delay *, delay )
			{
				if( !found && page->title && delay->page_name && StrCaseCmp( page->title, delay->page_name ) == 0 )
				{
					found = TRUE;
					continue;
				}
				else if( !found && !page->title && !delay->page_name )
				{
					found = TRUE;
					continue;
				}
				if( found && delay->delay )
				{
					ShellSetCurrentPage( (PSI_CONTROL)psv, delay->page_name );
					return;
				}
			}
			ShellSetCurrentPage( (PSI_CONTROL)psv, WIDE("first") );
		}
	}
}

static void DefineRegistryMethod(TASK_PREFIX,DoPageCycle,WIDE( "page_cycle" ),WIDE("commands"), WIDE( "cycle_now" ),void,(PSI_CONTROL pc_canvas))(PSI_CONTROL pc_canvas)
{
	if( !l.flags.bDisableChange )
		ShellSetCurrentPage( pc_canvas, WIDE("next") );
}

static void OnFinishInit( WIDE("Page Cycler") )( PSI_CONTROL pc_canvas )
{
   l.timer = AddTimer( l.delay, change, (uintptr_t)pc_canvas );
}

static void OnEditModeBegin( WIDE("Page Cycler") )( void )
{
   l.flags.bDisableChange = 1;
}

static void OnEditModeEnd( WIDE("Page Cycler") )( void )
{
   l.flags.bDisableChange = 0;
}

static int OnChangePage( WIDE("Page Cycler") )( PSI_CONTROL pc_canvas )
{
	PPAGE_DATA page = ShellGetCurrentPage( pc_canvas );
	INDEX idx;
	struct page_delay *delay;

	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
		if( page->title )
		{
			if( StrCaseCmp( page->title, delay->page_name ) == 0 )
			{
				//lprintf( WIDE("Compare ok - schedule %d"), delay->delay );
				if( delay->delay )
					RescheduleTimerEx( l.timer, delay->delay );
				break;
			}
		}
		else
			if( !delay->page_name )
			{
				//lprintf( WIDE("Compare ok - schedule %d"), delay->delay );
				RescheduleTimerEx( l.timer, delay->delay );
				break;
			}
	}
	if( !delay )
	{
		delay = New( struct page_delay );
		delay->delay = l.delay;
		if( page->title )
		{
			delay->page_name = StrDup( page->title );
			delay->delay = SACK_GetPrivateProfileInt( WIDE("Page Cycler/pages"), delay->page_name, l.delay, WIDE("page_changer.ini") );
		}
		else
		{
			delay->delay = SACK_GetPrivateProfileInt( WIDE("Page Cycler/pages"), "First", l.delay, WIDE("page_changer.ini") );
			delay->page_name = NULL;
		}
		AddLink( &l.delays, delay );
		//lprintf( WIDE("Compare ok - schedule %d"), l.delay );
		RescheduleTimerEx( l.timer, l.delay );
	}
   return 1;
}

static void OnGlobalPropertyEdit( WIDE( "Page Cycle" ) )( PSI_CONTROL parent )
{
	PSI_CONTROL frame;
	if( !l.flags.resources_registered )
	{
		l.flags.resources_registered = 1;
		EasyRegisterResource( WIDE("InterShell/Page Cycle"), CHECKBOX_ENABLE          , RADIO_BUTTON_NAME );
	}
	frame = LoadXMLFrameOver( parent, WIDE( "PageCycle.isFrame" ) );

	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		SetCheckState( GetControl( frame, CHECKBOX_ENABLE ), l.flags.allow_cycle );
		//lprintf( WIDE( "show frame over parent." ) );
		DisplayFrameOver( frame, parent );
		//lprintf( WIDE( "Begin waiting..." ) );
		CommonWait( frame );
		if( okay )
		{
			l.flags.allow_cycle = GetCheckState( GetControl( frame, CHECKBOX_ENABLE ) );
			SACK_WritePrivateProfileInt( WIDE("Page Cycler"), WIDE("Enabled"), l.flags.allow_cycle, WIDE("page_changer.ini") );

			//UpdateFromControls( frame );
		}
		//ClearControls();
		DestroyFrame( &frame );
	}
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

static void OnLoadCommon( WIDE("Page Cycler") )( PCONFIG_HANDLER pch )
{
	l.delay = SACK_GetPrivateProfileInt( WIDE("Page Cycler"), WIDE("default delay between pages"), 15000, WIDE("page_changer.ini") );
	l.flags.allow_cycle = SACK_GetPrivateProfileInt( WIDE("Page Cycler"), WIDE("Enabled"), 1, WIDE("page_changer.ini") );
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif

