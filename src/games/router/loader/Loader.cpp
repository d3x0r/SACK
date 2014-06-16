
#include <InterShell/intershell_registry.h>
#include "../board/board.hpp"
#include "../board/toolbin.hpp"

PRELOAD( LoadBoard )
{
   LoadFunction( WIDE("network_objects.netplug"), NULL );
   LoadFunction( WIDE("eltanin_tables.plugin"), NULL );
}

static PIBOARD last_board;
//static PTRSZVAL
void DoLoadBoard( void )
{
	if( last_board )
	{
		PODBC odbc = ConnectToDatabase( WIDE("route_board.db") );
		last_board->Load( odbc, WIDE("Uhmm board name") );
		CloseDatabase( odbc );
	}
}


static PTRSZVAL OnCreateControl( WIDE("Route Board/Router Board") )( PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	PSI_CONTROL board = CreateBoardControl( parent, x, y, w, h );
	PIBOARD iBoard = GetBoardFromControl( board );
	iBoard->LoadPeiceConfiguration( WIDE("router.peice.txt") );
	iBoard->SetBackground( iBoard->GetPeice( WIDE("background") ), (PTRSZVAL)iBoard );
   // when creating a new one, load the old one.
	DoLoadBoard();
	last_board = iBoard;
	return (PTRSZVAL)board;
}

static PSI_CONTROL OnGetControl( WIDE("Route Board/Router Board") )( PTRSZVAL psv )
{
	return (PSI_CONTROL)psv;
}



//static PTRSZVAL
static PTRSZVAL OnCreateControl( WIDE("Route Board/Router Tools") )( PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
   PSI_CONTROL toolbin = CreateToolbinControl( last_board, parent, x, y, w, h );
   PTOOLBIN iToolbin = GetToolbinFromControl( toolbin );
   return (PTRSZVAL)toolbin;
}

static PSI_CONTROL OnGetControl( WIDE("Route Board/Router Tools") )( PTRSZVAL psv )
{
   return (PSI_CONTROL)psv;
}


static void OnFinishInit( WIDE("Router board") )( PCanvasData pc_canvas )
{
	DoLoadBoard();
}

static void OnKeyPressEvent( WIDE("Route Board/Load Board") )( PTRSZVAL psv )
{
	if( psv )
	{
		PODBC odbc = ConnectToDatabase( WIDE("route_board.db") );
		((PIBOARD)psv)->Load( odbc, WIDE("Uhmm board name") );
		CloseDatabase( odbc );
	}
}

static PTRSZVAL OnCreateMenuButton( WIDE("Route Board/Load Board") )( PMENU_BUTTON button )
{
	return (PTRSZVAL)last_board;
}

static void OnKeyPressEvent( WIDE("Route Board/Save Board") )( PTRSZVAL psv )
{
	if( psv )
	{
		PODBC odbc = ConnectToDatabase( WIDE("route_board.db") );
		((PIBOARD)psv)->Save( odbc, WIDE("Uhmm board name") );
		CloseDatabase( odbc );
	}
}

static PTRSZVAL OnCreateMenuButton( WIDE("Route Board/Save Board") )( PMENU_BUTTON button )
{
	return (PTRSZVAL)last_board;
}


PUBLIC( void, ExportThis )( void )
{
}
