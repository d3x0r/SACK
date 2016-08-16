
#include <InterShell/intershell_registry.h>
#include "../board/board.hpp"
#include "../board/toolbin.hpp"

PRELOAD( LoadBoard )
{
   LoadFunction( WIDE("network_objects.netplug"), NULL );
   LoadFunction( WIDE("eltanin_tables.plugin"), NULL );
}

static PIBOARD last_board;
//static uintptr_t
void DoLoadBoard( void )
{
	if( last_board )
	{
		PODBC odbc = ConnectToDatabase( WIDE("route_board.db") );
		last_board->Load( odbc, WIDE("Uhmm board name") );
		CloseDatabase( odbc );
	}
}


static uintptr_t OnCreateControl( WIDE("Route Board/Router Board") )( PSI_CONTROL parent,int32_t x,int32_t y,uint32_t w,uint32_t h)
{
	PSI_CONTROL board = CreateBoardControl( parent, x, y, w, h );
	PIBOARD iBoard = GetBoardFromControl( board );
	iBoard->LoadPeiceConfiguration( WIDE("router.peice.txt") );
	iBoard->SetBackground( iBoard->GetPeice( WIDE("background") ), (uintptr_t)iBoard );
   // when creating a new one, load the old one.
	DoLoadBoard();
	last_board = iBoard;
	return (uintptr_t)board;
}

static PSI_CONTROL OnGetControl( WIDE("Route Board/Router Board") )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}



//static uintptr_t
static uintptr_t OnCreateControl( WIDE("Route Board/Router Tools") )( PSI_CONTROL parent,int32_t x,int32_t y,uint32_t w,uint32_t h)
{
   PSI_CONTROL toolbin = CreateToolbinControl( last_board, parent, x, y, w, h );
   PTOOLBIN iToolbin = GetToolbinFromControl( toolbin );
   return (uintptr_t)toolbin;
}

static PSI_CONTROL OnGetControl( WIDE("Route Board/Router Tools") )( uintptr_t psv )
{
   return (PSI_CONTROL)psv;
}


static void OnFinishInit( WIDE("Router board") )( PSI_CONTROL pc_canvas )
{
	DoLoadBoard();
}

static void OnKeyPressEvent( WIDE("Route Board/Load Board") )( uintptr_t psv )
{
	if( psv )
	{
		PODBC odbc = ConnectToDatabase( WIDE("route_board.db") );
		((PIBOARD)psv)->Load( odbc, WIDE("Uhmm board name") );
		CloseDatabase( odbc );
	}
}

static uintptr_t OnCreateMenuButton( WIDE("Route Board/Load Board") )( PMENU_BUTTON button )
{
	return (uintptr_t)last_board;
}

static void OnKeyPressEvent( WIDE("Route Board/Save Board") )( uintptr_t psv )
{
	if( psv )
	{
		PODBC odbc = ConnectToDatabase( WIDE("route_board.db") );
		((PIBOARD)psv)->Save( odbc, WIDE("Uhmm board name") );
		CloseDatabase( odbc );
	}
}

static uintptr_t OnCreateMenuButton( WIDE("Route Board/Save Board") )( PMENU_BUTTON button )
{
	return (uintptr_t)last_board;
}


PUBLIC( void, ExportThis )( void )
{
}
