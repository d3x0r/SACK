
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

//static LOGICAL OnPeiceBeginDrag( WIDE("background") )( PTRSZVAL psv, S_32 x, S_32 y )
//{
   // if return true, allows dragging the board.
//   return TRUE;
//}
static int OnPeiceTap( WIDE("Router background") )( PTRSZVAL psv, S_32 x, S_32 y )
{
   struct background_peice* peice = ( struct background_peice *)psv;
	peice->board->CreateActivePeice( x, y, (PTRSZVAL)peice->board );
   return 1;
   // if return true, allows dragging the board.
   //return TRUE;
}
static PTRSZVAL OnPeiceCreate( WIDE("Router background") )( PTRSZVAL psvCreate, PLAYER_DATA layer )
{
	struct background_peice* peice = New( struct background_peice );
	peice->board = (PIBOARD)psvCreate;
   return (PTRSZVAL)peice;
}

//-------------------------------------------------------




// -----------------  a port peice
struct port_peice{
	PIBOARD board;
   PLAYER_DATA layer;
	_32 port_number;
   CTEXTSTR port_name;
};

static PTRSZVAL OnPeiceCreate( WIDE("port") )( PTRSZVAL psvCreate, PLAYER_DATA layer )
{
	struct port_peice* peice = New( struct port_peice );
	peice->board = (PIBOARD)psvCreate;
   peice->layer = layer;
   return (PTRSZVAL)peice;
}

static void OnPeiceProperty( WIDE("port") )( PTRSZVAL psv, PSI_CONTROL parent )
{
}

static int OnPeiceClick( WIDE("port") )( PTRSZVAL psv, S_32 x, S_32 y )
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
			if( !peice->board->BeginPath( (PIVIA)peice->board->GetPeice( WIDE("port_forward") ), (PTRSZVAL)peice->board ) )
			{
				// attempt to grab existing path...
				// current position, and current layer
				// will already be known by the grab path method
				//brainboard->board->GrabPath();
			}
		}
      return 1;
}

static int OnPeiceEndConnect( WIDE("port") )( PTRSZVAL psv_to_instance, S_32 x, S_32 y
												  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
{
   lprintf( WIDE("Connecting to %d,%d"), x, y );
   // can return FALSE to abort connection
   return TRUE;
}


static int OnPeiceBeginConnect( WIDE("port") )( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
{
   // can return false to fault connection...
   return TRUE;
}


static void EndConnectFrom( WIDE("port") )( PTRSZVAL psv, PTRSZVAL psv_connector )
{
}

static void EndConnectTo( WIDE("port") )( PTRSZVAL psv, PTRSZVAL psv_connector )
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



static PTRSZVAL OnPeiceCreate( WIDE("interface") )( PTRSZVAL psvCreate, PLAYER_DATA layer )
{
	struct interface_peice* peice = New( struct interface_peice );
	peice->board = (PIBOARD)psvCreate;
   peice->layer = layer;
   layer->atext = WIDE("Interface");
   return (PTRSZVAL)peice;
}

static int OnPeiceTap( WIDE("interface") )( PTRSZVAL psv, S_32 x, S_32 y )
{
	struct interface_peice* peice = ( struct interface_peice *)psv;
	peice->board->CreatePeice(  WIDE("port"),  x, y, (PTRSZVAL)peice->board );

   return 1;
}


static void OnPeiceProperty( WIDE("interface") )( PTRSZVAL psv, PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, WIDE("router_interface.frame") );
	if( frame )
	{
		DisplayFrameOver( frame, parent );
		CommonWait( frame );



      DestroyFrame( &frame );

	}
}

static int OnPeiceBeginDrag( WIDE("interface") )( PTRSZVAL psv, S_32 x, S_32 y )
{
	struct interface_peice* peice = ( struct interface_peice *)psv;
	S_32 size_x, size_y;
   S_32 hot_x, hot_y;
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

