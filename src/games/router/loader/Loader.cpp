
#include <InterShell/intershell_registry.h>
#include "../board/board.hpp"
#include "../board/toolbin.hpp"

PRELOAD( LoadBoard )
{
   LoadFunction( "network_objects.netplug", NULL );
   LoadFunction( "tables.plugin", NULL );
}

static PIBOARD last_board;
//static uintptr_t
void DoLoadBoard( void )
{
	if( last_board )
	{
		PODBC odbc = ConnectToDatabase( "route_board.db" );
		last_board->Load( odbc, "Uhmm board name" );
		CloseDatabase( odbc );
	}
}


static uintptr_t OnCreateControl( "Route Board/Router Board" )( PSI_CONTROL parent,int32_t x,int32_t y,uint32_t w,uint32_t h)
{
	PSI_CONTROL board = CreateBoardControl( parent, x, y, w, h );
	PIBOARD iBoard = GetBoardFromControl( board );
	iBoard->LoadPeiceConfiguration( "router.peice.txt" );
	iBoard->SetBackground( iBoard->GetPeice( "background" ), (uintptr_t)iBoard );
   // when creating a new one, load the old one.
	DoLoadBoard();
	last_board = iBoard;
	return (uintptr_t)board;
}

static PSI_CONTROL OnGetControl( "Route Board/Router Board" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}



//static uintptr_t
static uintptr_t OnCreateControl( "Route Board/Router Tools" )( PSI_CONTROL parent,int32_t x,int32_t y,uint32_t w,uint32_t h)
{
   PSI_CONTROL toolbin = CreateToolbinControl( last_board, parent, x, y, w, h );
   PTOOLBIN iToolbin = GetToolbinFromControl( toolbin );
   return (uintptr_t)toolbin;
}

static PSI_CONTROL OnGetControl( "Route Board/Router Tools" )( uintptr_t psv )
{
   return (PSI_CONTROL)psv;
}


static void OnFinishInit( "Router board" )( PSI_CONTROL pc_canvas )
{
	DoLoadBoard();
}

static void OnKeyPressEvent( "Route Board/Load Board" )( uintptr_t psv )
{
	if( psv )
	{
		PODBC odbc = ConnectToDatabase( "route_board.db" );
		((PIBOARD)psv)->Load( odbc, "Uhmm board name" );
		CloseDatabase( odbc );
	}
}

static uintptr_t OnCreateMenuButton( "Route Board/Load Board" )( PMENU_BUTTON button )
{
	return (uintptr_t)last_board;
}

static void OnKeyPressEvent( "Route Board/Save Board" )( uintptr_t psv )
{
	if( psv )
	{
		PODBC odbc = ConnectToDatabase( "route_board.db" );
		((PIBOARD)psv)->Save( odbc, "Uhmm board name" );
		CloseDatabase( odbc );
	}
}

static uintptr_t OnCreateMenuButton( "Route Board/Save Board" )( PMENU_BUTTON button )
{
	return (uintptr_t)last_board;
}


PUBLIC( void, ExportThis )( void )
{
}
