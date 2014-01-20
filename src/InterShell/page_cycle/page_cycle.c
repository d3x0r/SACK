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
   _32 delay;
};

static struct page_cycle_local {
	struct {
		BIT_FIELD bDisableChange : 1;
	} flags;
   int nPage;
	PLIST delays;
   _32 timer;
   _32 delay;
} l;

static void CPROC change( PTRSZVAL psv )
{
   if( !l.flags.bDisableChange )
		ShellSetCurrentPage( WIDE("next") );
}

OnFinishInit( WIDE("Page Cycler") )( void )
{
   l.timer = AddTimer( l.delay, change, 0 );
}

OnEditModeBegin( WIDE("Page Cycler") )( void )
{
   l.flags.bDisableChange = 1;
}

OnEditModeEnd( WIDE("Page Cycler") )( void )
{
   l.flags.bDisableChange = 0;
}

OnChangePage( WIDE("Page Cycler") )( void )
{
	PPAGE_DATA page = ShellGetCurrentPage();
	INDEX idx;
	struct page_delay *delay;
   //lprintf( WIDE("new page = %s"), page->title );
	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
		if( page->title )
		{
			if( StrCaseCmp( page->title, delay->page_name ) == 0 )
			{
            //lprintf( WIDE("Compare ok - schedule %d"), delay->delay );
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
			delay->page_name = StrDup( page->title );
		else
			delay->page_name = NULL;
      AddLink( &l.delays, delay );
		//lprintf( WIDE("Compare ok - schedule %d"), l.delay );
		RescheduleTimerEx( l.timer, l.delay );
	}
   return 1;
}

OnSaveCommon( WIDE("Page Cycler") )( FILE *file )
{
	INDEX idx;
	struct page_delay *delay;
	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
      if( delay->page_name )
			fprintf( file, WIDE("page delay for '%s'=%d\n"), delay->page_name, delay->delay );
	}
}

static PTRSZVAL CPROC LoadDelay( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PARAM( args, S_64, delay_time );
	INDEX idx;
	struct page_delay *delay;
	LIST_FORALL( l.delays, idx, struct page_delay *, delay )
	{
		if( StrCaseCmp( name, delay->page_name ) == 0 )
         break;
	}
	if( delay )
	{
      delay->delay = (_32)delay_time;
	}
	else
	{
		delay = New( struct page_delay );
		delay->page_name = StrDup( name );
		delay->delay = (_32)delay_time;
      AddLink( &l.delays, delay );
	}
   return psv;
}

OnLoadCommon( WIDE("Page Cycler") )( PCONFIG_HANDLER pch )
{
	l.delay = SACK_GetPrivateProfileInt( WIDE("Page Cycler"), WIDE("default delay between pages"), 15000, WIDE("page_changer.ini") );

   AddConfigurationMethod( pch, WIDE("page delay for '%m'=%i"), LoadDelay );
}

#if ( __WATCOMC__ < 1291 )
PUBLIC( void, NeedAtLeastOneExport )( void )
{

}
#endif

