#include <stdhdrs.h>
#include <network.h>
#include <sqlgetoption.h>
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include "../intershell_export.h"
#include "../intershell_registry.h"

struct sendbutton {
	PMENU_BUTTON button;
	CTEXTSTR page;
};

static struct page_cycle_local {
	PCLIENT udp_server;
	SOCKADDR *saBroadcast;
	uint32_t delay;
	uint32_t last_msg;
} l;

static void CPROC NetEvent( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *saSource )
{
	if( buffer )
	{
		uint32_t tmp;
		sscanf( (CTEXTSTR)buffer, WIDE("%d"), &tmp );
		if( tmp != l.last_msg )
		{
			CTEXTSTR end = strrchr( (CTEXTSTR)buffer, ':' );
			// someday pay attention to command between buffer and end...
			if( end )
			{
				PSI_CONTROL pc_canvas = (PSI_CONTROL)GetNetworkLong( pc, 1 );
				end++;
				// might not have finished init...
				if( pc_canvas )
					ShellSetCurrentPage( pc_canvas, end );
			}
			l.last_msg = tmp;
		}
	}
	else
	{
		buffer = Allocate( 256 );
	}
	ReadUDP( pc, buffer, 256 );
}

static void OnFinishInit( WIDE("Net Page Changer") )( PSI_CONTROL pc_canvas )
{
	TEXTCHAR tmp[256];
	SACK_GetPrivateProfileString( WIDE("Network Page Changer"), WIDE("listen on interface"), WIDE("0.0.0.0:7636"), tmp, sizeof( tmp ), WIDE("page_changer.ini") );
	if( tmp[0] )
	{
		l.udp_server = ServeUDP( tmp, 7636, NetEvent, NULL );
		SetNetworkLong( l.udp_server, 1, (uintptr_t)pc_canvas );
		SACK_GetPrivateProfileString( WIDE("Network Page Changer"), WIDE("send change to"), WIDE("255.255.255.255:7636"), tmp, sizeof( tmp ), WIDE("page_changer.ini") );
	}
 }

static void OnKeyPressEvent(  WIDE("page/send page change") )( uintptr_t psv )
{
	struct sendbutton *button = (struct sendbutton *)psv;
	PVARTEXT pvt = VarTextCreate();
	PTEXT out;
	vtprintf( pvt, WIDE("%d:change:%s"), timeGetTime(), button->page );
	out = VarTextGet( pvt );
	SendUDPEx( l.udp_server, GetText( out ), GetTextSize( out )+1, l.saBroadcast );
	SendUDPEx( l.udp_server, GetText( out ), GetTextSize( out )+1, l.saBroadcast );
	SendUDPEx( l.udp_server, GetText( out ), GetTextSize( out )+1, l.saBroadcast );
	LineRelease( out );
	VarTextDestroy( &pvt );
}

static uintptr_t OnCreateMenuButton( WIDE("page/send page change") )( PMENU_BUTTON button )
{
	struct sendbutton *newbutton = New( struct sendbutton );
	newbutton->button = button;
	newbutton->page = StrDup( WIDE("next") );
	return (uintptr_t)newbutton;
}

static uintptr_t CPROC SetSendPage( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, page );
	struct sendbutton *button = (struct sendbutton *)psv;
	Release( (TEXTSTR)button->page );
	button->page = StrDup( page );
	return psv;
}

static void OnLoadControl( WIDE("page/send page change") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	struct sendbutton *button = (struct sendbutton *)psv;
	AddConfigurationMethod( pch, WIDE("send page='%m'"), SetSendPage );
}

static void OnSaveControl( WIDE("page/send page change") )( FILE *file, uintptr_t psv )
{
	struct sendbutton *button = (struct sendbutton *)psv;
	sack_fprintf( file, WIDE("send page='%s'\n"), button->page );
}

PUBLIC( void, ExportedSymbolToMakeWatcomHappy )( void )
{
}

