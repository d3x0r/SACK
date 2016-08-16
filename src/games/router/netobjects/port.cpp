
#include <stdhdrs.h>
#include <procreg.h>
//#include "
#include "../board/methods.hpp"


struct background_peice{
	PIBOARD board;
   PLAYER_DATA layer;
};


struct local_tag {
	PLIST interfaces;

}l;


//------------- The background Peice -------------------
// needs to at least LockDrag so we can move the board around...

//static LOGICAL OnPeiceBeginDrag( WIDE("background") )( uintptr_t psv, int32_t x, int32_t y )
//{
   // if return true, allows dragging the board.
//   return TRUE;
//}
static int OnPeiceTap( WIDE("Router background") )( uintptr_t psv, int32_t x, int32_t y )
{
   struct background_peice* peice = ( struct background_peice *)psv;
	peice->board->CreateActivePeice( x, y, (uintptr_t)peice->board );
   return 1;
   // if return true, allows dragging the board.
   //return TRUE;
}
static uintptr_t OnPeiceCreate( WIDE("Router background") )( uintptr_t psvCreate, PLAYER_DATA layer )
{
	struct background_peice* peice = New( struct background_peice );
	peice->board = (PIBOARD)psvCreate;
   return (uintptr_t)peice;
}

//-------------------------------------------------------




// -----------------  a port peice
struct port_peice{
	PIBOARD board;
   PLAYER_DATA layer;
	uint32_t port_number;
   CTEXTSTR port_name;
};

static uintptr_t OnPeiceCreate( WIDE("port") )( uintptr_t psvCreate, PLAYER_DATA layer )
{
	struct port_peice* peice = New( struct port_peice );
	peice->board = (PIBOARD)psvCreate;
   peice->layer = layer;
   return (uintptr_t)peice;
}

static void OnPeiceProperty( WIDE("port") )( uintptr_t psv, PSI_CONTROL parent )
{
}

static int OnPeiceClick( WIDE("port") )( uintptr_t psv, int32_t x, int32_t y )
{
	struct port_peice* peice = ( struct port_peice *)psv;
		if( x == 0 && y == 0 )
		{
			// this is implied to be the current peice that
			// has been clicked on...
			// will receive further OnMove events...
			peice->board->LockPeiceDrag();
         return TRUE;
			//return TRUE;
		}
		else
		{
			if( !peice->board->BeginPath( (PIVIA)peice->board->GetPeice( WIDE("port_forward") ), (uintptr_t)peice->board ) )
			{
				// attempt to grab existing path...
				// current position, and current layer
				// will already be known by the grab path method
				//brainboard->board->GrabPath();
			}
		}
      return 1;
}

static int OnPeiceEndConnect( WIDE("port") )( uintptr_t psv_to_instance, int32_t x, int32_t y
												  , PIPEICE peice_from, uintptr_t psv_from_instance )
{
   lprintf( WIDE("Connecting to %d,%d"), x, y );
   // can return FALSE to abort connection
   return TRUE;
}


static int OnPeiceBeginConnect( WIDE("port") )( uintptr_t psv_to_instance, int32_t x, int32_t y
									  , PIPEICE peice_from, uintptr_t psv_from_instance )
{
   // can return false to fault connection...
   return TRUE;
}


static void EndConnectFrom( WIDE("port") )( uintptr_t psv, uintptr_t psv_connector )
{
}

static void EndConnectTo( WIDE("port") )( uintptr_t psv, uintptr_t psv_connector )
{
}


// -----------------  a port forwarding

//static LOGICAL Begin



// -----------------  an interface peice
struct interface_peice{
	TEXTSTR name;
   TEXTSTR IP;
	PIBOARD board;
	PLAYER_DATA layer;
   PLIST ports;
};



static uintptr_t OnPeiceCreate( WIDE("interface") )( uintptr_t psvCreate, PLAYER_DATA layer )
{
	struct interface_peice* peice = New( struct interface_peice );
	peice->board = (PIBOARD)psvCreate;
   peice->layer = layer;
   layer->atext = WIDE("Interface");
   return (uintptr_t)peice;
}

static int OnPeiceTap( WIDE("interface") )( uintptr_t psv, int32_t x, int32_t y )
{
	struct interface_peice* peice = ( struct interface_peice *)psv;
	peice->board->CreatePeice(  WIDE("port"),  x, y, (uintptr_t)peice->board );

   return 1;
}


static void OnPeiceProperty( WIDE("interface") )( uintptr_t psv, PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, WIDE("router_interface.frame") );
	if( frame )
	{
		DisplayFrameOver( frame, parent );
		CommonWait( frame );



      DestroyFrame( &frame );

	}
}

static int OnPeiceBeginDrag( WIDE("interface") )( uintptr_t psv, int32_t x, int32_t y )
{
	struct interface_peice* peice = ( struct interface_peice *)psv;
	int32_t size_x, size_y;
   int32_t hot_x, hot_y;
	peice->board->GetCurrentPeiceSize( &size_x, &size_y );
	//peice->board->GetCurrentPeiceHotSpot( &hot_x, &hot_y );
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
		if( x == (size_x-2) &&
			y == (size_y-2) )
		{
         peice->board->BeginSize( DOWN_RIGHT );
         lprintf( WIDE("lower right "));
		}
		else if( x == (size_x-2) &&
				  y == -1 )
		{
         peice->board->BeginSize( UP_RIGHT );
         lprintf( WIDE("upper right") );
		}
		else if( x == -1 &&
				  y == -1 )
		{
         peice->board->BeginSize( UP_LEFT );
         lprintf( WIDE("upper left") );
		}
		else if( x == -1 &&
				  y == (size_y-2) )
		{
         peice->board->BeginSize( DOWN_LEFT );
         lprintf( WIDE("lower left"));
		}
		else
			peice->board->LockPeiceDrag();
	}
   return 1;
}

PUBLIC( void, ExportThis )( void )
{
}

