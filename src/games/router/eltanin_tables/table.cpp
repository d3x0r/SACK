

#define DEFINE_DEFAULT_IMAGE_INTERFACE

#include <stdhdrs.h>
#include <procreg.h>
//#include "
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include <intershell/intershell_registry.h>
#include <intershell/intershell_export.h>
#include "../board/methods.hpp"



struct background_peice{
	PIBOARD board;
   PLAYER_DATA layer;
};


struct local_tag {
	struct {
		BIT_FIELD allow_edit : 1;
	} flags;
	PLIST tables;

}l;

PRELOAD( ConfigureTables )
{
	//l.flags.allow_edit = 1;
}



static int OnPeiceTap( WIDE("Eltanin Table background") )( PTRSZVAL psv, S_32 x, S_32 y )
{
	if( l.flags.allow_edit )
	{
		struct background_peice* peice = ( struct background_peice *)psv;
		PLAYER layer = peice->board->CreatePeice( WIDE("table"), x, y, (PTRSZVAL)peice->board );
		if( !layer )
			lprintf( WIDE("Failed to create a table?") );
		AddLink( &l.tables, layer );
	}
   return 1;
   // if return true, allows dragging the board.
   //return TRUE;
}


static PTRSZVAL OnPeiceCreate( WIDE("Eltanin Table background") )( PTRSZVAL psvCreate, PLAYER_DATA layer )
{
	struct background_peice* peice = New( struct background_peice );
	peice->board = (PIBOARD)psvCreate;
   return (PTRSZVAL)peice;
}


// -----------------  an table peice
struct table_peice{
	PIBOARD board;
	PLAYER_DATA layer;
   PLIST units;
};




static PTRSZVAL OnPeiceCreate( WIDE("table") )( PTRSZVAL psvCreate, PLAYER_DATA layer )
{
	struct table_peice* peice = New( struct table_peice );
	peice->board = (PIBOARD)psvCreate;
   peice->units = NULL;
   peice->layer = layer;
   layer->atext = WIDE("Table");
   return (PTRSZVAL)peice;
}

static int OnPeiceTap( WIDE("table") )( PTRSZVAL psv, S_32 x, S_32 y )
{
	if( l.flags.allow_edit )
	{
		struct table_peice* peice = ( struct table_peice *)psv;
		PLAYER layer = peice->board->CreatePeice( WIDE("stationary"),  x, y, (PTRSZVAL)peice->board );
		if( !layer )
		{
			lprintf( WIDE("Failed to create a stationary") );
		}
		else
			AddLink( &peice->units, layer );
	}
   return 1;
}

static int OnPeiceBeginDrag( WIDE("table") )( PTRSZVAL psv, S_32 x, S_32 y )
{
	if( l.flags.allow_edit )
	{
	struct table_peice* peice = ( struct table_peice *)psv;
	S_32 size_x, size_y;
   S_32 hotx, hoty;
	peice->board->GetCurrentPeiceSize( &size_x, &size_y );
	peice->board->GetCurrentPeiceHotSpot( &hotx, &hoty );
	lprintf( WIDE("Peice is %d,%d? click is %d,%d"), size_x, size_y, x, y );

	//if( x == 0 && y == 0 )
	{
		// this is implied to be the current peice that
		// has been clicked on...
		// will receive further OnMove events...
		//return TRUE;
	}
	//else
	{
		if( x == (size_x-1-hotx) &&
			y == (size_y-1-hoty) )
		{
         peice->board->BeginSize( DOWN_RIGHT );
         lprintf( WIDE("lower right "));
		}
		else if( x == (size_x-1-hotx) &&
				  y == -hoty )
		{
         peice->board->BeginSize( UP_RIGHT );
         lprintf( WIDE("upper right") );
		}
		else if( x == -hotx &&
				  y == -hoty )
		{
         peice->board->BeginSize( UP_LEFT );
         lprintf( WIDE("upper left") );
		}
		else if( x == -hotx &&
				  y == (size_y-1-hoty) )
		{
         peice->board->BeginSize( DOWN_LEFT );
         lprintf( WIDE("lower left"));
		}
		else
			peice->board->LockPeiceDrag();
	}
	}
   return 1;
}




// -----------------  a stationary peice
struct stationary_peice{
	PIBOARD board;
   PLAYER_DATA layer;
	_32 stationary_number;
	CTEXTSTR stationary_name;
	struct {
		BIT_FIELD failed : 1;
	} flags;
};

static PTRSZVAL OnPeiceCreate( WIDE("stationary") )( PTRSZVAL psvCreate, PLAYER_DATA layer )
{
	struct stationary_peice* peice = New( struct stationary_peice );
	peice->board = (PIBOARD)psvCreate;
   peice->layer = layer;
   return (PTRSZVAL)peice;
}



static int OnPeiceClick( WIDE("stationary") )( PTRSZVAL psv, S_32 x, S_32 y )
{
	if( l.flags.allow_edit )
	{
		struct stationary_peice* peice = ( struct stationary_peice *)psv;
		{
			// this is implied to be the current peice that
			// has been clicked on...
			// will receive further OnMove events...
			peice->board->LockPeiceDrag();
		}
	}
	else
	{
		struct stationary_peice* peice = ( struct stationary_peice *)psv;
		peice->flags.failed = !peice->flags.failed;
	}
   return 1;
}


static void OnPeiceExtraDraw( WIDE("Stationary") )( PTRSZVAL psv, Image surface, S_32 x, S_32 y, _32 w, _32 h )
{
   // I dunno this is pretty close to ... raw *shrug*
}

static void OnPeiceDraw( WIDE("Stationary") )( PTRSZVAL psv, Image surface, Image peice_image, S_32 x, S_32 y )
{
	struct stationary_peice* peice = ( struct stationary_peice *)psv;
	// I dunno this is pretty close to ... raw *shrug*
	BlotImageMultiShaded( surface, peice_image, x, y
							  , peice->flags.failed?BASE_COLOR_RED:BASE_COLOR_GREEN // screen (status)
                       , 0xFF998744 // tan sorta
							  , 0xFF614421  // brown
                        );
}


OnKeyPressEvent( WIDE("Route Board/Eltanin Table/Enable Edit") )( PTRSZVAL psv )
{
	l.flags.allow_edit = !l.flags.allow_edit;
   UpdateButton( (PMENU_BUTTON)psv );
}

OnShowControl( WIDE("Route Board/Eltanin Table/Enable Edit") )( PTRSZVAL psvUnused )
{
   PMENU_BUTTON button = (PMENU_BUTTON)psvUnused;
	InterShell_SetButtonHighlight( button, l.flags.allow_edit );
}


OnCreateMenuButton( WIDE("Route Board/Eltanin Table/Enable Edit") )( PMENU_BUTTON button )
{
	return (PTRSZVAL)button;
}

PUBLIC( void, ExportThis )( void )
{
}


