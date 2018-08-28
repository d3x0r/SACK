#include "interface.h"
#include <stdhdrs.h>
#include <sharemem.h>
//#define USES_INTERSHELL_INTERFACE
//#define DEFINES_INTERSHELL_INTERFACE
//#include "InterShell/intershell_export.h"

#include "board.hpp"
#include "layer.hpp"
#include "global.h"


#ifdef __WINDOWS__
#define IMPORT __declspec(dllexport)
#else
#define IMPORT
#endif

#undef Right
#undef Left

#define Opposite(n)  (((n)+4)&7)
#define Left(n)    (((n)-1)&7)
#define Right(n)   (((n)+1)&7)
#define LeftOrRight(n,i)     (((n)&1)?((i)?Left(n):Right(n)):((i)?Right(n):Left(n)))
#define NearDir(nNewDir, nDir) ( ( nNewDir == (nDir) ) ? 0 :   \
	( nNewDir == ( ( (nDir) + 1 ) & 7 ) )? -1 :    \
	( nNewDir == ( ( (nDir) - 1 ) & 7 ) )? 1 : 10 )

extern DIR_DELTA DirDeltaMap[8];

//--------------------------------------------------------------------------

void LAYER_DATA::Update( uint32_t cycle )
{
	//if( cycle != _cycle )
	//{
	//	_cycle = cycle;
   //   peice->methods->Update( psvInstance, cycle );
	//}
}

LAYER_DATA::~LAYER_DATA( void )
{
	//if( !(--ref) )
	{
		peice->methods->Destroy( psvInstance );
	}
}

void LAYER_DATA::operator delete( void *ptr )
{
	PLAYER_DATA layer = (PLAYER_DATA)ptr;
	PLAYER_DATASET *ppls = layer->pool;
	DeleteFromSet( LAYER_DATA, (*ppls), layer );
}
#ifdef _MSC_VER
void LAYER_DATA::operator delete( void *ptr, struct LAYER_DATAset_tag **frompool )
{
	DeleteFromSet( LAYER_DATA, frompool[0], ptr );
}
#endif
//--------------------------------------------------------------------------

void LAYER_DATA::Init( void )
{
	//ref = 1;
	peice = NULL;
	psvInstance = NULL;
   atext = NULL;
	//_cycle = 0;
	//rows = 0;
	//cols = 0;
//   pds_path = CreateDataStack( sizeof( LAYER_PATH_NODE ) );
   //cData[0] = 0;
   //cData[1] = 0;
   //cData[2] = 0;
}

//--------------------------------------------------------------------------

LAYER_DATA::LAYER_DATA( PIPEICE peice )
{
   //Init();
	LAYER_DATA::peice = peice;
	DebugBreak();
	//LAYER_DATA::psvInstance = peice->methods->Create( peice->psvCreate );
}

//--------------------------------------------------------------------------

void LAYER_DATA::SetPeice( PIPEICE peice )
{
	LAYER_DATA::peice = peice;
	//DebugBreak();
	if( !psvInstance )
	{
		LAYER_DATA::psvInstance = peice->methods->Create( peice->psvCreate, this );
	}
}

//--------------------------------------------------------------------------

LAYER_DATA::LAYER_DATA( void )
{
   Init();
}

//--------------------------------------------------------------------------



void DrawText( Image surface, int32_t base_x, int32_t base_y, TEXTSTR caption, CDATA color, SFTFont font )
{
   uint32_t x = 0;
	uint32_t y = 0;
	uint32_t w, h, maxw = 0;
	TEXTCHAR *start = caption;
	TEXTCHAR *end;
	//BlatColor( surface, 0, 0, surface->width, surface->height, banner->basecolor );
	//ClearImageTo( surface, GetBaseColor( NORMAL ) );
	GetStringSizeFontEx( caption, strlen( caption ), &maxw, &h, font );
	y = 0;
	while( start )
	{
		end = strchr( start, '\n' );
		if( !end )
		{
			end = start + strlen(start);
		}
		w = GetStringSizeFontEx( start, end-start, NULL, &h, font );
      /*
		PutStringFontEx( surface
							, ( BANNER_WIDTH - (left?maxw:w) ) /2+2
							, y+2
							, SetAlpha( ~color, 0x80 ), 0
							, start, end-start
							, font );
		PutStringFontEx( surface
							, ( BANNER_WIDTH - (left?maxw:w) ) /2-2
							, y-2
							, SetAlpha( ~color, 0x80 ), 0
							, start, end-start
							, font );
		*/
		PutStringFontEx( surface
							, x+base_x
							, y+base_y
							, color, 0
							, start, end-start
							, font );
		y += h;
		if( end[0] )
			start = end+1;
		else
			start = NULL;
	}

}

void LAYER::Draw( PIBOARD board, Image image, int32_t x, int32_t y )
{
	uint32_t cellx, celly;
	uint32_t boardx, boardy;
	int scale;
	if( flags.bRoot )
		return;
	//lprintf( WIDE("Drawing a cell...%p at %d,%d"), image, x, y );
	scale = board->GetScale();
	board->GetSize( &boardx, &boardy );
	board->GetCellSize( &cellx, &celly, scale );
	if( flags.bRoute )
	{
		int n;
		//DebugBreak();
		PLAYER_PATH_NODE node;
		PIVIA viaset = (PIVIA)(pLayerData->peice);
		PVIA_METHODS methods = viaset->via_methods;
		for( n = 0; node = (PLAYER_PATH_NODE)PeekDataEx( &pds_path, n ); n++ )
		{
			int xofs, yofs;
			Image fill;
			//lprintf( WIDE("Drawing route path node %p %d,%d  %d,%d")
			//		 , node, node->x, node->y
			//		 , node->flags.ForeDir, node->flags.BackDir );
			fill = viaset->GetViaFromTo( node->flags.BackDir, node->flags.ForeDir, scale );
			if( fill )
			{
				methods->Draw( pLayerData->psvInstance
								 , image
								 , fill
								 , x + (node->x) * cellx
								 , y + (node->y) * celly );

			}
			//else
         //   lprintf( WIDE("filler for %d,%d failed"), node->flags.BackDir, node->flags.ForeDir );
			fill = viaset->GetViaFill1( &xofs, &yofs, node->flags.ForeDir, scale );
			if( fill )
			{
				methods->Draw( pLayerData->psvInstance
								 , image
								 , fill
								 , x + (node->x + xofs) * cellx
								 , y + (node->y + yofs) * celly );
			}
			//else
         //   lprintf( WIDE("filler for %d,%d failed"), node->flags.BackDir, node->flags.ForeDir );
			fill = viaset->GetViaFill2( &xofs, &yofs, node->flags.ForeDir, scale );
			if( fill )
			{
				methods->Draw( pLayerData->psvInstance
								 , image
								 , fill
								 , x + (node->x + xofs) * cellx
								 , y + (node->y + yofs) * celly );

			}
			//else
         //   lprintf( WIDE("filler for %d,%d failed"), node->flags.BackDir, node->flags.ForeDir );
		}
	}
	else
	{
		// this requires knowing cellsize, and the current offset/origin of the
		// layer/board...
		uint32_t rows, cols;
		int xofs, yofs;
		// maximum number of cells on the board...
		// so we don't over draw.
		// later, when I get more picky, only draw those cells that changed
		// which may include an offset
		pLayerData->peice->getsize( &rows, &cols );
		//lprintf( WIDE("Drawing layer at %d,%d (%d,%d) origin at %d,%d"), LAYER::x, LAYER::y, hotx, hoty, x, y );
		{
		if( 1 )
		{
			if( rows == h && cols == w )
			{
				for( xofs = -hotx; xofs < ((signed)cols-hotx); xofs++ )
					for( yofs = -hoty; yofs < ((signed)rows-hoty); yofs++ )
					{
						pLayerData->peice->methods->Draw( pLayerData->psvInstance
																  , image
																  , pLayerData->peice->getcell( xofs+hotx, yofs+hoty, scale )
																  , x + xofs * cellx
																  , y + yofs * celly );
					}
			}
			else
			{
				for( xofs = -hotx; xofs < ((signed)w-hotx); xofs++ )
					for( yofs = -hoty; yofs < ((signed)h-hoty); yofs++ )
					{
						int cell_spot_x = 0;
						int cell_spot_y = 0;

						if( xofs == -hotx )
							cell_spot_x = 0;
						else if( xofs == (w-hotx-1) )
							cell_spot_x = cols-1;
						else
						{
							cell_spot_x = ((xofs)%(cols-2) ) + 1;
						}

						if( yofs == -hoty )
							cell_spot_y = 0;
						else if( yofs == (h-hoty-1) )
							cell_spot_y = rows-1;
						else
						{
							cell_spot_y = ((yofs)%(rows-2) ) + 1;
						}

						pLayerData->peice->methods->Draw( pLayerData->psvInstance
																  , image
																  , pLayerData->peice->getcell( cell_spot_x, cell_spot_y, scale )
																  , x + xofs * cellx
																  , y + yofs * celly );
					}
			}
		}
		else
			pLayerData->peice->methods->Draw( pLayerData->psvInstance
													  , image
													  , pLayerData->peice->getimage(scale)
													  , x, y );

	}




		if( pLayerData->atext )
		{
			SFTFont *ppFont = NULL;// UseACanvasFont( NULL, WIDE( "Network Board Font" ) );
			if( ppFont )
				DrawText( image, x /*+ cellx*cols*/
					  , y + celly*h
					  , pLayerData->atext
					  , BASE_COLOR_WHITE
					  , ppFont[0]
					  );
		}
	}
}

void LAYER::BeginPath( int32_t x, int32_t y, int direction )
{
	LAYER_PATH_NODE node;
	LAYER::x = x;
	LAYER::y = y;
	LAYER::min_x = x;
	LAYER::min_y = y;
	LAYER::w = 1;
	LAYER::h = 1;
	EmptyDataStack( &pds_path );
	node.x = 0; // x - LAYER::x
	node.y = 0; // y - LAYER::y
	node.flags.BackDir = NOWHERE;
	if( direction == NOWHERE )
	{
		flags.bForced = 0;
		// there is no real peice which is
		// NOWHERE to NOWHERE, therefore, do not attempt to draw one
		// and instead set the exit direction as valid.
		node.flags.bForced = 0;
		node.flags.ForeDir = NOWHERE;
	}
	else
	{
		flags.bForced = 1;
		node.flags.bForced = 1;
		node.flags.ForeDir = direction;
	}
	{
		PushData( &pds_path, &node );
	}
}

void LAYER::BeginPath( int32_t x, int32_t y )
{
   BeginPath( x, y, NOWHERE );
}

//--------------------------------------------------------------------------

void *LAYER::operator new( size_t sz )
{
	return NULL;
}

//--------------------------------------------------------------------------

void LAYER::link_top(void )
{
	PLAYER layer0;
	layer0 = GetSetMember( LAYER, (*pool), 0 );
	if( layer0 )
	{
      //DebugBreak();
		if( next = layer0->next )
			next->prior = this;
		prior = NULL; // if top, no prior
		if( !layer0->next )
         layer0->prior = this;
		layer0->next = this;
      // layer0->me ?!
      //LinkThing( next, this );
	}
}

void *LAYER::operator new( size_t sz, LAYERSET **frompool, LAYER_DATASET **fromdatapool )
{
	if( sz != sizeof( LAYER ) )
		return NULL;
	if( !(*frompool ) )
	{
		//(*frompool) = NULL; //new LAYERSET;
		PLAYER layer0 = GetSetMember( LAYER, (frompool), 0 );
		// grab the 0'th layer from the pool.
		// this layer is the root of all layers, and allows
		// the tracking of the order of under/over
		// by being a well known variable that this
		// module has access to.
		layer0->next = NULL;
		layer0->prior = NULL;
		layer0->flags.bRoot = 1;
	}
	PLAYER layer = GetFromSet( LAYER, (frompool) );
	MemSet( layer, 0, sizeof( LAYER ) );
	layer->pool = frompool;
	layer->pLayerData = new( fromdatapool ) LAYER_DATA();
	layer->link_top();
	return layer;
}

//--------------------------------------------------------------------------

void *LAYER_DATA::operator new( size_t sz, struct LAYER_DATAset_tag **frompool )
{
	if( sz != sizeof( LAYER_DATA ) )
		return NULL;
	//if( !(*frompool ) )
    //  (*frompool) = new LAYER_DATASET;
	PLAYER_DATA layer = GetFromSet( LAYER_DATA, (frompool) );
	layer->pool = frompool;
	return layer;
}


void LAYER::isolate( void )
{
	// can't isoloate root
	if( flags.bRoot )
		return ;
	if( !pool )
		return;
	PLAYER layer0 = GetSetMember( LAYER, (*pool), 0 );;
	if( next )
	{
		next->prior = prior;
	}
	else
		layer0->prior = prior;
	if( prior )
		prior->next = next;
	else
		layer0->next = next;
   //UnlinkThing(this);
}
//--------------------------------------------------------------------------

void LAYER::operator delete( void *ptr )
{
	PLAYER layer = (PLAYER)ptr;
	PLAYERSET *ppls = layer->pool;
	layer->isolate();
	if( ppls )
		DeleteFromSet( LAYER, (*ppls),( layer ) );
}

//--------------------------------------------------------------------------
#ifdef _MSC_VER
void LAYER::operator delete( void *ptr, struct LAYERset_tag **frompool, struct LAYER_DATAset_tag **fromdatapool )
{
	PLAYER layer = (PLAYER)ptr;
	PLAYERSET *ppls = layer->pool;
	layer->isolate();
	if( ppls )
		DeleteFromSet( LAYER, frompool[0],( layer ) );
}
#endif
//--------------------------------------------------------------------------

void LAYER::Init(void)
{
   //pLayerData = new LAYER_DATA(
	//pLayerData->Other.dwAny = 0;
	//pLayerData->DrawMethod = DRAW_RAW;
	//flags.BackDir = NOWHERE;
	//flags.ForeDir = NOWHERE;
	//Content = 0;  // should be FindPeice(NOT_BLANK)
	flags.bRoot = FALSE;
	x = 0;
	y = 0;
	w = 0;
	h = 0;
	pds_path = CreateDataStack( sizeof( LAYER_PATH_NODE ) );
	stacked_on = NULL;
   holding = NULL;
   //pds_path = NULL;
	//shadows = NULL;
}

//--------------------------------------------------------------------------

LAYER::LAYER()
{
   Init();
}

//--------------------------------------------------------------------------

LAYER::LAYER( PIPEICE peice, int _x, int _y, int _w, int _h )
{
	Init();
	x = _x;
	y = _y;
	min_x = _x;
   min_y = _y;
	w = _w;
	h = _h;
	peice->gethotspot( &hotx, &hoty );
   pLayerData->SetPeice( peice );
}

//--------------------------------------------------------------------------

LAYER::LAYER( PIPEICE peice, int _x, int _y, int ofsx, int ofsy, int _w, int _h )
{
	Init();
	x = _x;
	y = _y;
	min_x = _x - ofsx;
   min_y = _y - ofsy;
	w = _w;
	h = _h;
   peice->gethotspot( &hotx, &hoty );
   pLayerData->SetPeice( peice );
}

//--------------------------------------------------------------------------

LAYER::LAYER( PIVIA peice )
{
	// as a route, we have to expect some further information to
	// happen, and since we need to call create to generate the instance
	// of the peice, then do some user defined things, then
	// determine whether this even is valid to do, and if it is, where the
   // position to start is...
	Init();
	flags.bRoute = 1;
	peice->gethotspot( &hotx, &hoty );
	pLayerData->SetPeice( peice );
}

//--------------------------------------------------------------------------

LAYER::LAYER( PIPEICE peice )
{
	// as a route, we have to expect some further information to
	// happen, and since we need to call create to generate the instance
	// of the peice, then do some user defined things, then
	// determine whether this even is valid to do, and if it is, where the
   // position to start is...
	Init();
	peice->gethotspot( &hotx, &hoty );
	pLayerData->SetPeice( peice );
}

//--------------------------------------------------------------------------

LAYER::LAYER( PIPEICE peice, int _x, int _y )
{
	Init();
	x = _x;
	y = _y;
	w = 1;
	h = 1;
	peice->gethotspot( &hotx, &hoty );
	pLayerData->SetPeice( peice );
}

//--------------------------------------------------------------------------

uintptr_t CPROC CheckIsLayer( POINTER p, uintptr_t psv )
{
	PLAYER layer = (PLAYER)p;
	if( layer->iLayer == psv )
		return (uintptr_t)layer;
	return 0;
}

PLAYER LAYER::FindLayer( INDEX iLayer )
{
	return 	(PLAYER)ForAllInSet( LAYER, (*pool), CheckIsLayer, (uintptr_t)iLayer );
//(PLAYER)(*pool)->forall( CheckIsLayer, iLayer );
}

LAYER::LAYER( PODBC odbc, PLIST peices, INDEX iLoadLayer )
{
	if( !this )
		return;
	if( iLayer && iLayer != INVALID_INDEX )
	{
		lprintf( WIDE("Recovering prior layer") );
		return;
	}
	{
		CTEXTSTR *results;
		PushSQLQueryEx( odbc );
		if( SQLRecordQueryf( odbc, NULL, &results, NULL
					  , WIDE("select x,y,min_x,min_y,width,height,linked_from_id,linked_from_x,linked_from_y,linked_to_id,linked_to_x,linked_to_y,route,peice_info_id,peice_type,stacked_on_id from board_layer where board_layer_id=%lu")
					  , iLoadLayer
					  ) && results )
		{
			PIPEICE peice_type = ::GetPeice( peices, results[14] );
			
			pLayerData->psvInstance = peice_type->Load( odbc, atoi( results[13] ) );//, pLayerData->psvInstance );
			peice_type->gethotspot( &hotx, &hoty );
			pLayerData->SetPeice( peice_type );
			pds_path = CreateDataStack( sizeof( LAYER_PATH_NODE ) );
			x = atoi( results[0] );
			y = atoi( results[1] );
			min_x = atoi( results[2] );
			min_y = atoi( results[3] );
			w = (uint32_t)IntCreateFromText( results[4] );
			h = (uint32_t)IntCreateFromText( results[5] );
			flags.bRoute = atoi( results[12] );

		    iLayer = iLoadLayer;

			INDEX iStart = IntCreateFromText( results[6] );
			if( iStart != INVALID_INDEX )
			{
				int32_t x, y;
				x = atoi( results[7] );
				y = atoi( results[8] );
				PLAYER loaded_route_start_layer;
				loaded_route_start_layer = FindLayer( iStart );
				if( !loaded_route_start_layer )
					loaded_route_start_layer = GetFromSet( LAYER, pool ); //new(pool,pLayerData->pool) LAYER( odbc, peices, iStart );
				loaded_route_start_layer->pLayerData->peice->methods->ConnectBegin( loaded_route_start_layer->pLayerData->psvInstance
												, x, y // connect x, y
												, pLayerData->peice, pLayerData->psvInstance );
				loaded_route_start_layer->Link( this, LINK_VIA_START, x, y);
			}

			INDEX iEnd = IntCreateFromText( results[9] );
			if( iEnd != INVALID_INDEX )
			{
				int32_t x, y;
				x = atoi( results[10] );
				y = atoi( results[11] );
				PLAYER loaded_route_end_layer;
				loaded_route_end_layer = FindLayer( iEnd );
				if( !loaded_route_end_layer )
					loaded_route_end_layer = GetFromSet( LAYER, pool ); //new(pool,pLayerData->pool) LAYER( odbc, peices, iEnd );
				loaded_route_end_layer->pLayerData->peice->methods->ConnectEnd( loaded_route_end_layer->pLayerData->psvInstance
												, x, y // connect x, y
												, pLayerData->peice, pLayerData->psvInstance );
				loaded_route_end_layer->Link( this, LINK_VIA_END, x, y );
			}

			INDEX iStackedOn = IntCreateFromText( results[15] );
			if( iStackedOn != INVALID_INDEX )
			{
				stacked_on = FindLayer( iStackedOn );
				if( !stacked_on )
				{
               lprintf( WIDE("((((((( PROBABLY A FATAL CONDITION! )))))))))))))") );
				}
				else
				{
               AddLink( &stacked_on->holding, this );
				}
			}


			//}
		}
		for( SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select x,y,fore,back from board_layer_path where board_layer_id=%lu order by board_layer_path_id desc"), iLoadLayer );
			results;
			FetchSQLRecord( odbc, &results ) )
		{
			// add path node for a routed type peice
			LAYER_PATH_NODE node;
			node.x = atoi( results[0] );
			node.y = atoi( results[1] );
			node.flags.ForeDir = atoi( results[2] );
			node.flags.BackDir = atoi( results[3] );
			PushData( &pds_path, &node );
		}
		PopODBCEx( odbc );
	}
   //return iLayer;
}

//--------------------------------------------------------------------------

LAYER::~LAYER( )
{
	//while( shadows )
   //   delete shadows;
   delete pLayerData;
}

//--------------------------------------------------------------------------


PIPEICE LAYER::GetPeice( void )
{
   return pLayerData->peice;
}

//SHADOW_LAYER::~SHADOW_LAYER()
//{
//	UnlinkThing( this );
//   pLayerData->drop();
//}


void LAYER::Unlink( void )
{
	//if( flags.bRoute )
	{
		if( route_end_layer.layer )
		{
			flags.bEnded = FALSE;
			DeleteLink( &route_end_layer.layer->linked, this );
			route_end_layer.layer = NULL;
		}
		else if( route_start_layer.layer )
		{
			DeleteLink( &route_start_layer.layer->linked, this );
			route_start_layer.layer = NULL;
		}
	}

}

void LAYER::Link( PLAYER via, int link_type, int32_t x, int32_t y )
{
	// links via to this
   // or links via from this
	AddLink( &linked, via );
	switch( link_type )
	{
	case LINK_VIA_START:
		via->route_start_layer.layer = this;
		via->route_start_layer.x = x;
		via->route_start_layer.y = y;
		
		break;
	case LINK_VIA_END:
		via->flags.bEnded = TRUE;
		via->route_end_layer.layer = this;
		via->route_end_layer.x = x;
		via->route_end_layer.y = y;
		break;
	}
	// and we need to consider how to recover the linked
	// state of the actual peices that are linked.
}

int LAYER::GetLastBackDirection( void )
{
	PLAYER_PATH_NODE node;
	node = (PLAYER_PATH_NODE)PeekData( &pds_path );
	if( node )
	{
      return node->flags.BackDir;
	}
   return NOWHERE;
}
int LAYER::GetLastForeDirection( void )
{
	PLAYER_PATH_NODE node;
	node = (PLAYER_PATH_NODE)PeekData( &pds_path );
	if( node )
	{
      return node->flags.BackDir;
	}
   return NOWHERE;
}

int LAYER::IsLayerAt( int32_t *x, int32_t *y )
{
	PLAYER_PATH_NODE node;
	int n;
	// width of 3.. offset 1, should be -1 and +1 of the x, y... not -1 and +2 (3)
	// difference is really only 2 when comparing as a corrdinate
   lprintf( WIDE("layer test %d,%d within %d,%d-%d,%d"), *x, *y, min_x, min_y, w, h );
	if( flags.bRoot ||
		(LAYER::min_x+(signed)LAYER::w) <= (*x) ||
		(LAYER::min_y+(signed)LAYER::h) <= (*y) ||
		(LAYER::min_x > (*x) ) ||
		(LAYER::min_y > (*y) )
	  )
		return FALSE;
	if( !flags.bRoute )
	{
		(*x) -= LAYER::x;
		(*y) -= LAYER::y;
		return TRUE;
	}
	else
	{
      //DebugBreak();
	}
	// otherwise, we need to check the path to see
   // if we're actually on this.
	for( n = 0; node = (PLAYER_PATH_NODE)PeekDataEx( &pds_path, n ); n++ )
	{
		if( (node->x + LAYER::x) == (*x) && (node->y + LAYER::y) == (*y) )
		{
			(*x) = node->x;
         (*y) = node->y;
         return TRUE;
		}
	}
   return FALSE;
}

int LAYER::Overlaps( int x, int y )
{
	PLAYER_PATH_NODE node;
	int n;
	// just in case, don't check the top node
	// we may have been stupid...
	// and given current routing rules, there should never
   // be an opportunity which even the 5th segment might overlap.
	for( n = 1; node = (PLAYER_PATH_NODE)PeekDataEx( &pds_path, n ); n++ )
	{
		int xofs, yofs;
		lprintf( WIDE("checking overlap of %d,%d at %d,%d (%d)")
				 , x, y
				 , node->x, node->y, n );
		// should we test for overlap on via?
		// that's really the only way we can catch 50% of the intersections
      // of two diagonals, at shared via coordinate, instead of in-cell line overlap
		if( node->x == x && node->y == y )
		{
			return n;
		}
		PIVIA viaset = (PIVIA)(pLayerData->peice);
		//if( n > 2 )
		{
		if( viaset->GetViaFill1( &xofs, &yofs, node->flags.ForeDir ) )
			if( ( ( node->x + xofs ) == x ) && ( ( node->y + yofs ) == y ) )
			{
            lprintf(WIDE("hit via fill1 of node %d,%d"), xofs, yofs );
            return n;
			}
		if( viaset->GetViaFill2( &xofs, &yofs, node->flags.ForeDir ) )
			if( ( ( node->x + xofs ) == x ) && ( ( node->y + yofs ) == y ) )
			{
            lprintf(WIDE("hit via fill2 of node %d,%d"), xofs, yofs );
            return n;
			}
		if( viaset->GetViaFill1( &xofs, &yofs, node->flags.BackDir ) )
			if( ( ( node->x + xofs ) == x ) && ( ( node->y + yofs ) == y ) )
			{
            lprintf(WIDE("hit via fill1 of node back %d,%d"), xofs, yofs );
            return n;
			}
		if( viaset->GetViaFill2( &xofs, &yofs, node->flags.BackDir ) )
			if( ( ( node->x + xofs ) == x ) && ( ( node->y + yofs ) == y ) )
			{
            lprintf(WIDE("hit via fill2 of node back %d,%d"), xofs, yofs );
            return n;
			}
		}
	}
	return FALSE;
}

// result is the last node (if any... which is a peekstack)
PLAYER_PATH_NODE LAYER::UnlayPath( int nLayers )
{
	// unwind to, and including this current spot.
	// this is to handle when the line intersects itself.
	// other conditions of unlaying via pathways may require
	// other functionality.
	int n;
	PLAYER_PATH_NODE node;// = (PLAYER_PATH_NODE)PopData( &pds_path );
	lprintf( WIDE("overlapped self at path segment %d"), nLayers );
	for( n = nLayers; (n && (node = (PLAYER_PATH_NODE)PopData( &pds_path ))), n; n-- )
	{
		lprintf( WIDE("Popped node %d(%p)"), n, node );
		// grab the NEXT node...
		// if it has bForced set... then this node must exist.
		PLAYER_PATH_NODE next = (PLAYER_PATH_NODE)PeekData( &pds_path );
		if( next && next->flags.bForced )
		{
			DebugBreak();
			node->flags.ForeDir = NOWHERE;
			return node;
		}
		if( node->flags.bForced )
		{
			DebugBreak();
         // this is SO bad.
		}
		//if( node->x == dest_x && node->y == dest_y )
		{
			//lprintf( WIDE("And then we find the node we overlaped...") );
		}
	}
	lprintf( WIDE("Okay done popping... %d, %p"), n, node );
	if( node )
	{
		PLAYER_PATH_NODE next = (PLAYER_PATH_NODE)PeekData( &pds_path );
		// set this as nowhere, so that we can easily just step forward here..
		if( !next )
		{
			if( !node->flags.bForced )
			{
				node->flags.ForeDir = NOWHERE;
			}
			PushData( &pds_path, node );
			return node;
		}
		if( !nLayers
			&& next->flags.bForced
			&& next->flags.BackDir != NOWHERE )
		{
			// if it was forced, then this MUST be here.  There is a reason.
			// there is also a way to end this reason, and unlay 0 path.  This
			// releases the foredir to anything.  This may be used for error correction path
			// assumptions?
			DebugBreak();
			if( next->flags.bTry )
			{
				node = (PLAYER_PATH_NODE)PopData(&pds_path );
				// this is the second attempt
				if( !node->flags.bFlopped )
				{
					node->flags.bFlopped = 1;
					node->flags.ForeDir = LeftOrRight( Opposite( node->flags.BackDir ), 1 );
               return node;
				}
			}
			next->flags.bForced = 0;
		}
		else
		{
			next->flags.ForeDir = NOWHERE;
         lprintf( WIDE("this node itself is okay...") );
		}
      return next;
	}
   return NULL;
}
//------------------------------------------

int FindDirection( int _x, int _y, int wX, int wY ) // From, To
{
	int nDir;

	if( _x < wX ) 
	{
		if( _y > wY )
			nDir = UP_RIGHT;
		else if( _y < wY )
			nDir = DOWN_RIGHT;
		else
			nDir = RIGHT;
	}
	else if( _x > wX )
	{
		if( _y > wY ) 
			nDir = UP_LEFT;
		else if( _y < wY )
			nDir = DOWN_LEFT;
		else
			nDir = LEFT;
	}
	else
	{
		if( _y > wY ) 
			nDir = UP;
		else if( _y == wY ) 
			nDir = NOWHERE;
		else
			nDir = DOWN;
	}
	return nDir;
}

#if 0
int LAYER::UnlayPath( int bLoop )
{
BackTrace:   // method here to remove one peice from the trail.
		// should perhaps join this and the loop removal -
		// and do a BackTrace UNTIL _x = tx, _y = ty or we are at the start...
#ifdef DEBUG_BACKTRACE
		Log( WIDE("Start...") );
#endif
		while( bBackTrace || bLoop )
		{
#ifdef DEBUG_BACKTRACE
			Log( WIDE("BackTrace...\n") );
#endif
			if( bStarted )  // bStarted = bCompleted...
			{
				int nDir = route_current_layer->GetLastBackDirection(); // this is always correct
				// nDir will be trashed... // update position one step...
#ifdef DEBUG_BACKTRACE
				{
					BYTE byString[256];
					sprintf( (char*)byString, WIDE(" %d, %d, %d to %d, %d\n"),
							  nDir, _x, _y,
							  _x + DirDeltaMap[nDir].x,
							  _y + DirDeltaMap[nDir].y );
					Log( (char*)byString );
				}
#endif
				_x += DirDeltaMap[nDir].x;
				_y += DirDeltaMap[nDir].y;
			}
			else
			{
#ifdef DEBUG_BACKTRACE
				Log( WIDE("Not Started...") );
#endif
				if( bLoop )  // just abort cause we're sooo confused
				{
					// backup to the start....
					bStarted = TRUE;
					bFailed = TRUE;
					continue;
					//                  return;
				}
				// comment this out - and path will be predicted...
				//               if( ( (abs(wX - xStart) + abs(wY-yStart)) < 4 ) )
				//                  return;
				//    return;  // get out now... can't do a thing! - saves auto-pathing
			}
			if( _x == xStart && _y == yStart )
			{
#ifdef DEBUG_BACKTRACE
				Log( WIDE("At Beginning...") );
#endif
				bStarted = FALSE;
				bLoop = FALSE;
			}
			else if( (abs( _x - tx ) + abs( _y - ty))<2 ) // almost back at start...???
			{
#ifdef DEBUG_BACKTRACE
				Log( WIDE("Loop Removed...") );
#endif
				bLoop = FALSE;  // uhmm delayed one step...(if just now stating)
			}
			bBackTrace = FALSE;  // only one requested...
		}
		if( bFailed )
			return;
}
#endif

void LAYER::move( int32_t del_x, int32_t del_y )
{
	if( stacked_on )
	{
		if( ( x + del_x ) < ( stacked_on->x-stacked_on->hotx ) )
         return;
		if( ( y + del_y ) < ( stacked_on->y-stacked_on->hoty ) )
         return;
		if( ( x + del_x ) >= ( stacked_on->x-stacked_on->hotx+(int32_t)stacked_on->w ) )
         return;
		if( ( y + del_y ) >= ( stacked_on->y-stacked_on->hoty+(int32_t)stacked_on->h ) )
         return;
	}
	x += del_x;
	y += del_y;
	min_x += del_x;
	min_y += del_y;
	{
		INDEX idx;
		PLAYER layer;
		LIST_FORALL( holding, idx, PLAYER, layer )
		{
         layer->move( del_x, del_y );
		}
		LIST_FORALL( linked, idx, PLAYER, layer )
		{
			if( layer->flags.bRoute )
			{
				if( layer->route_end_layer.layer == this && layer->route_start_layer.layer != this )
				{
					PLAYER_PATH_NODE node = (PLAYER_PATH_NODE)PeekData( &layer->pds_path );
					layer->LayPath( layer->x + ((!node)?0:node->x) + del_x
									  , layer->y + ((!node)?0:node->y) + del_y );
					// and here node is invalid!
				}
				if( layer->route_start_layer.layer == this )
				{
					PLAYER_PATH_NODE node = (PLAYER_PATH_NODE)PeekData( &layer->pds_path );
					if( layer->route_end_layer.layer )
					{
						int32_t destx = layer->route_end_layer.layer->x + layer->route_end_layer.x;
						int32_t desty = layer->route_end_layer.layer->y + layer->route_end_layer.y;
						layer->BeginPath( layer->x + del_x, layer->y + del_y );
						layer->LayPath( destx, desty );
					}
					else
					{
						lprintf( WIDE("This via should have been deleted?!") );
					}
					//layer->UnlayPath(
					// RelayPathFrom( wX, wY );
				}
			}
		}
	}
}

int LAYER::MoveResizeLayer( int32_t x_del, int32_t y_del, int32_t width_del, int32_t height_del )
{
   // routes/vias cannot be resized, they are long strings of single cells, not a block.
	if( !flags.bRoute )
	{
		if( (w+width_del) < 2 )
         return FALSE;
		if( (h+height_del) < 2 )
         return FALSE;
		x += x_del;
		y += y_del;
		min_x += x_del;
		min_y += y_del;
		w += width_del;
		h += height_del;
      return TRUE;
	}
   return FALSE;
}


void LAYER::LayPath( int32_t wX, int32_t wY )
{
	int DeltaDir;
	LOGICAL bLoop = FALSE, bIsRetry;  // no looping....
	int tx, ty;
	int nPathLayed = 0;
	int nDir, nNewDir;
	LOGICAL bBackTrace = FALSE,
		bFailed = FALSE;

	PLAYER_PATH_NODE node;
	lprintf( WIDE("Laying path %p to %d,%d"), this, wX, wY );
	node = (PLAYER_PATH_NODE)PeekData( &pds_path );
	// sanity validations...
	// being done already, etc...
	wX -= LAYER::x;
	wY -= LAYER::y;
	if( node )
	{
		if( node->x == wX && node->y == wY )
		{
			lprintf( WIDE("Already at this end point, why are you telling me to end where I already did?") );
			return;
		}
		// should range check wX and wY to sane limits
		// but for now we'll trust the programmer...
		if( abs( node->x - wX ) > 100 || abs( node->y - wY ) > 100 )
		{
			DebugBreak();
			lprintf( WIDE("Laying a LONG path - is this okay?!") );
		}
	}

	#ifdef DEBUG_BACKTRACE
		Log( WIDE("Enter...") );
	#endif

		//------------ FORWARD DRAWING NOW .....
	bIsRetry = FALSE;
	DeltaDir = 0;
	{
		PLAYER_PATH_NODE node;
		// get the last node in the path.
		node = (PLAYER_PATH_NODE)PeekData( &pds_path );
		while( node )
		{
			nNewDir = FindDirection( node->x
										  , node->y
										  , wX, wY );
			if( nNewDir == NOWHERE )
			{
				// already have this node at the current spot...
				lprintf( WIDE("Node has ended here...") );
				break;
			}
			nDir = NOWHERE; // intialize this, in case we missed a path below...
			if( node->flags.BackDir == NOWHERE )
			{
				// if it is newdir, we're okay to go ahead with this plan.
				if( node->flags.ForeDir != nNewDir && flags.bForced )
				{
					lprintf( WIDE("Have a forced begin point, and no way to get there from here....") );
					DebugBreak();
					if( NearDir( node->flags.ForeDir, nNewDir ) == 10 )
					{
						lprintf( WIDE("MUST go %d , have to go %d from here.  Go nowhere."), node->flags.ForeDir, nNewDir );
						lprintf( WIDE("Okay - consider a arbitrary jump to go forward... until we can go backward.") );
					}
					else
					{
						lprintf( WIDE("It's just not quite right... return, a less radical assumption may be made.") );
					}
					return;
				}
				// else, just go ahead, we returned above here.
				node->flags.ForeDir = nNewDir;
			}
			else
			{
				// need to determine a valid foredir based on nNewDir desire, and nBackDir given.
				lprintf( WIDE("%d, %d = %d")
						 , Opposite( node->flags.BackDir )
						 , nNewDir
						 , NearDir(Opposite( node->flags.BackDir )
									 , nNewDir ) );
				lprintf( WIDE("newdir = %d backdir = %d"), nNewDir, node->flags.BackDir );
				//pold->TopLayer->ForeDir;
				if( NearDir( nNewDir, Opposite( node->flags.BackDir ) ) != 10 )
				{
					// this is a valid direction to go.
					node->flags.ForeDir = nNewDir;
				}
				else
				{
					lprintf( WIDE("Unlay path cause we can't get there from here.") );
					node = UnlayPath( nPathLayed + 1 );
					// at this point always unlay at least one more than we put down.
					nPathLayed = 1;
					continue;
#if 0
               int nBase = Opposite( node->flags.BackDir );
					nDir = ( node->flags.BackDir + 2 ) & 7;
					if( NearDir( nNewDir, nDir ) != 10 )
					{
						//node->flags.ForeDir = (nBase + 6) &7;
						node->flags.ForeDir = Right( nBase );
					}
					else if( NearDir( nNewDir, Opposite( nDir ) ) != 10 )
					{
						node->flags.ForeDir = Left(nBase);
					}
					else
					{

						// this should be a random chance to go left or right...
						// maybe tend to the lower x or higher x ?
						lprintf( WIDE("Choosing an arbitrary directino of 1, and only on1") );
						//node->flags.ForeDir = Right( nBase + 1 );
						node->flags.bFlopped = 0;
						node->flags.bTry = 1;
						node->flags.bForced = 1;
						node->flags.ForeDir = LeftOrRight( nBase, node->flags.bFlopped );
						// set a flag in this node for which way to go...
						// but a left/right node needs the ability
						// to remain forced for a single unlay, and move in a direction...

					}
#endif
				}
			}
			{
				int  n;
				tx = node->x + DirDeltaMap[node->flags.ForeDir].x;
				ty = node->y + DirDeltaMap[node->flags.ForeDir].y;
				lprintf( WIDE("New coordinate will be %d,%d"), tx, ty );
				if( n = Overlaps( tx, ty ) ) // aleady drew something here...
					// the distance of the overlap is n layers, including Nth layer
					// for( ; n; PopData(&pds_stack), n-- )
					// and some fixups which unlay path does.
				{
					lprintf( WIDE("Unlaying path %d steps to overlap") , n );
					node = UnlayPath( n );
					// at an unlay point of forced, unlay path should be 'smart' and 'wait'
					// otherwise we may unwind to our tail and be confused... specially when moving away
					// and coming back to reside at the center.
					// if the force direction to go from a forced node is excessive, that definatly
					// breaks force, and releases the path node.
					// there may be board conditions which also determine the pathing.
					// okay try this again from the top do {
					// startin laying path again.
					continue;
				}
				// otherwise we're good to go foreward.
				// at least we won't add this node if it would have
				// already been there, heck, other than that via's
				// don't exist, sometimes we'll even get the exact node
				// that this should be....
				{
					LAYER_PATH_NODE newnode;
					// this may be set intrinsically by being an excessive force
					// causing a large direction delta
					newnode.flags.bForced = FALSE;
					newnode.flags.ForeDir = NOWHERE;
					// this of course must start(?) exactly how the other ended(?)...
					newnode.flags.BackDir = Opposite( node->flags.ForeDir );
					newnode.x = tx;
					newnode.y = ty;
					{
						int xx = tx + x;
						int yy = ty + y;
						if( xx < min_x )
						{
							w += min_x - xx;
							min_x = xx;
						}
						if( xx >= ( min_x + (int32_t)w ) )
							w = xx - min_x + 1;
						if( yy < min_y )
						{
							h += min_y - yy;
							min_y = yy;
						}
						if( yy >= ( min_y + (int32_t)h ) )
							h = yy - min_y + 1;

					}
					lprintf( WIDE("Push path %d,%d  min=%d,%d size=%d,%d"), newnode.x, newnode.y, min_x, min_y, w, h );
					PushData( &pds_path, &newnode );
					nPathLayed++;
					node = (PLAYER_PATH_NODE)PeekData( &pds_path ); // okay this is now where we are.
				}
			}
		}
	}
}

/*

int LAYER::UndoPath( void )
{
	node = PeekData( &pds_path );
	if( node )
	{

	}
}
*/
//int LAYER::

INDEX LAYER::Save( PODBC odbc )
{
	if( !this )
		return INVALID_INDEX;
	if( iLayer && iLayer != INVALID_INDEX )
	{
		lprintf( WIDE("Recovering prior layer") );
		return iLayer;
	}

	{
		INDEX iStart = route_start_layer.layer->Save( odbc );
		INDEX iEnd = route_end_layer.layer->Save( odbc );
      INDEX iStack = stacked_on->Save( odbc );
		INDEX iPeice;

		if( pLayerData )
			iPeice = pLayerData->peice->Save( odbc, iLayer, pLayerData->psvInstance );
		else
			iPeice = INVALID_INDEX;
		SQLInsert( odbc, WIDE("board_layer")
					  ,WIDE("x") , 2, x
					  ,WIDE("y"), 2, y
					  ,WIDE("min_x"), 2, min_x
					  ,WIDE("min_y"), 2, min_y
					  ,WIDE("width"), 2, w
					  ,WIDE("height"), 2, h
					  ,WIDE("linked_from_id"), 2, iStart
					  ,WIDE("linked_from_x"), 2, route_start_layer.x
					  ,WIDE("linked_from_y"), 2, route_start_layer.y
					  ,WIDE("linked_to_id"), 2, iEnd
					  ,WIDE("linked_to_x"), 2, route_end_layer.x
					  ,WIDE("linked_to_y"),  2, route_end_layer.y
					  ,WIDE("stacked_on_id"),  2, iStack
					  ,WIDE("route"), 2, flags.bRoute
					  ,WIDE("peice_info_id"), 2, iPeice
					  ,WIDE("peice_type"), 1, pLayerData->peice->name() 
					  , NULL, 0, NULL );
		iLayer = FetchLastInsertID( odbc, NULL, NULL );
		lprintf( WIDE("Saved %lu"), iLayer );
	}

	if( flags.bRoute /*&& pds_path*/ )
	{
      INDEX idx;
      PLAYER_PATH_NODE path_node;
	  for( idx = 0; path_node = (PLAYER_PATH_NODE)PeekDataEx( &pds_path, idx ); idx++ )
		{
			SQLCommandf( odbc
				, WIDE("insert into board_layer_path(board_layer_id,x,y,fore,back)values(%lu,%ld,%ld,%d,%d)")
				, iLayer
				, path_node->x, path_node->y
				, path_node->flags.ForeDir, path_node->flags.BackDir );

		}
	}
   return iLayer;
}



//--------------------------------------------------------------------------
//
// $Log: layer.cpp,v $
// Revision 1.7  2004/12/15 18:48:44  panther
// Minor updates to latest SACK system...
//
// Revision 1.6  2004/07/11 10:24:14  panther
// progressively moving forward... this was badly designed before... and is still BAD.
//
// Revision 1.5  2004/07/06 20:44:41  panther
// checkpoint
//
// Revision 1.4  2004/07/06 02:04:49  panther
// Well it builds... it's got the peices... start pulling it together.
//
// Revision 1.3  2004/07/05 12:22:22  panther
// Well this is where this is headed... but ick.
//
// Revision 1.2  2004/03/21 14:30:15  panther
// Improvements...
//
// Revision 1.1  2004/02/19 01:11:42  panther
// Initial commit - rough coding
//
// Revision 1.2  2004/02/16 02:21:25  panther
// checkpoint
//
// Revision 1.1  2004/02/13 17:07:08  panther
// migrations to more object orientated methods
//
//
