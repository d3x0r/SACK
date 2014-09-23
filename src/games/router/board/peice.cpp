#include "interface.h"
#define PEICE_SOURCE_COMPILING
#include <stdhdrs.h>
#include <sharemem.h>
#define PEICE_DEF_DEFINED

#include "board.hpp"
#include "peice.hpp"
#include "global.h"
#ifdef __WINDOWS__
#define IMPORT __declspec(dllexport)
#else
#define IMPORT
#endif
typedef class PEICE *PPEICE;

PEICE_EXTERN(DIR_DELTA, DirDeltaMap[8]) = { { 0, -1 },
                             { 1, -1 }, 
								     { 1, 0 }, 
							        { 1, 1 }, 
								     { 0, 1 } , 
							        { -1, 1 },  
								     { -1, 0 },
								     { -1, -1 } 
									};


//class DEFAULT_METHODS:public PEICE_METHODS {
	PTRSZVAL PEICE_METHODS::Create( PTRSZVAL psvExtra, PLAYER_DATA layer )
	{
		return 0;
	}
	int PEICE_METHODS::Disconnect( PTRSZVAL psv1 /*, PIPEICE peice, PTRSZVAL psv2*/ )
	{
      return 0;
	}
	void PEICE_METHODS::Destroy( PTRSZVAL )
	{
	}
   /*
	int PEICE_METHODS::Connect( PCELL to, PCELL from )
	{
		return TRUE; // allow peices to connect to here...
		}
      */
	void PEICE_METHODS::Update( PTRSZVAL psv, _32 cycle )
	{
		return; // do nothing to update...
		// consider on failure
		// Destroy( psv );
	}
	void PEICE_METHODS::OnMove( PTRSZVAL psv )
	{
	}
	int PEICE_METHODS::ConnectBegin ( PTRSZVAL psv_to_instance, S_32 x, S_32 y
											  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
      return 0;
	}
	int PEICE_METHODS::ConnectEnd ( PTRSZVAL psv_to_instance, S_32 x, S_32 y
											  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
      return 0;
	}
	int PEICE_METHODS::OnClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		return 0;
	}
	int PEICE_METHODS::OnTap( PTRSZVAL psv, S_32 x, S_32 y )
	{
		return 0;
	}
	int PEICE_METHODS::OnBeginDrag( PTRSZVAL psv, S_32 x, S_32 y )
	{
		return 0;
	}
	int PEICE_METHODS::OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		return 0;
	}
	int PEICE_METHODS::OnDoubleClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		return 0;
	}
   /*
	void PEICE_METHODS::Draw( PTRSZVAL psvInstance, Image surface, int x, int y, int cellx, int celly )
	{
		// first 0 is current scale.
		lprintf( WIDE("Drawing peice instance %p cell: %d,%d at: %d,%d"), psvInstance, cellx, celly, x, y );
		//lprintf( WIDE("Drawing %d by %d"), rows, cols );
		BlotImage( surface
					, master->getcell(cellx, celly)
					, x, y
					);
	}
   */
	void PEICE_METHODS::Draw( PTRSZVAL psvInstance, Image surface, Image peice, S_32 x, S_32 y )
	{
		// first 0 is current scale.
		//lprintf( WIDE("Drawing peice instance %p"), psvInstance );
		//lprintf( WIDE("Drawing %d by %d"), rows, cols );

		BlotImageAlpha( surface
						  , peice
						  , x, y
						  , 1 );
	}


static class PEICE_METHODS DefaultMethods;

class DEFAULT_VIA_METHODS:public VIA_METHODS {
	int Move( void )
	{
      return 0;
	}
	int Stop( void )
	{
      return 0;
	}
} DefaultViaMethods;

class PEICE_DATA
{
public:
	struct {
		_32 block : 1;
		_32 viaset : 1;
	} flags;
	int rows, cols;
	int hotx, hoty;
	PIBOARD board;
	Image original;
	Image *scaled;// x1, x2, x4 // [3] ( [rows][cols] ) + 1
	Image *current_scale; // set by setting scale...

	TEXTSTR name;
	PEICE_DATA( PIBOARD board
				 , CTEXTSTR name
				 , Image image = NULL
				 , int rows = 1
				 , int cols = 1
				 , int hotx = 0
				 , int hoty = 0
				 , int bBlock = 0
				 , int bVia = 0
				 //, PPEICE_METHODS methods = NULL
				 );
	~PEICE_DATA()
	{
		int scale, x, y;
		for( scale = 0; scale < 3; scale++ )
		{
			for( x = 0; x < cols; x++ )
				for( y = 0; y < rows; y++ )
					UnmakeImageFile( scaled[ scale * ( (rows*cols) + 1 )
													+ ( x + (y*cols) ) + 1 ] );
			UnmakeImageFile( scaled[ scale * ( (rows*cols) + 1 ) ] );
		}
		Release( scaled );
		Release( name );
	}
};
//	void PEICE_METHODS::Draw( PTRSZVAL psvInstance, Image surface, Image peice, int x, int y )
//	{
		// first 0 is current scale.
//		lprintf( WIDE("Drawing peice instance %p"), psvInstance );
//		BlotImageAlpha( surface, peice, x, y, ALPHA_TRANSPARENT);
//	}


PEICE_DATA::PEICE_DATA( PIBOARD board
							 , CTEXTSTR name
							 , Image image// = NULL
							 , int rows// = 1
							 , int cols// = 1
							 , int hotx, int hoty
							 , int bBlock// = 0
							 , int bVia// = 0
				 //, PPEICE_METHODS methods = NULL
				 )
{
	_32 cell_width, cell_height;
	// need the native to compute correct scalings.
	board->GetCellSize( &cell_width, &cell_height, 0 );
	PEICE_DATA::rows = rows;
	PEICE_DATA::cols = cols;
	PEICE_DATA::hotx = hotx;
	PEICE_DATA::hoty = hoty;
	PEICE_DATA::board = board;
	//PEICE_DATA::image = image;
	PEICE_DATA::name = StrDup( name );
	PEICE_DATA::original = image;
	scaled = NULL;

	// this should be moved out to a thing which is
	// like re-mip-map :)
	// or recompute based on a new cell size of the main board...
	if( image )
	{
		int scale = 0;
		int x, y;
		if( scaled )
			Release( scaled );
		scaled = (Image*)Allocate( sizeof( Image ) * (1 + rows * cols)* 3 );
      lprintf( WIDE("Begin scaling..") );
		scaled[0] = MakeImageFile( cell_width * cols, cell_height * rows );
      lprintf( WIDE("scale %d scaling.. to %d %d"), scale, cell_width * cols, cell_height * rows );
		BlotScaledImage( scaled[0], original );
      lprintf( WIDE("Begin scaling..") );

		for( scale = 0; scale < 3; scale++ )
		{
			if( scale )
			{
				// scale==0 already done.
				scaled[ scale * ( (rows*cols) + 1 ) ] = MakeImageFile( (cell_width * cols)/(1<<scale)
																					  , (cell_height * rows)/(1<<scale) );
				// blot original image for least loss of detail..
				BlotScaledImage( scaled[scale * ( (rows*cols) + 1 ) ]
									, original );
			}
      lprintf( WIDE("scale %d scaling.."), scale );
			// if anything - because of blocking and clipping
			// then ANY thing may need to be cut into cells

			//if( bVia ) // blocks don't neeeed to be cut ...
			{
				Image image = scaled[scale * ((rows*cols)+1)];
				for( x = 0; x < cols; x++ )
					for( y = 0; y < rows; y++ )
					{
						Image tmp;
						tmp =
							scaled[ scale * ( (rows*cols) + 1 ) + ( x + (y*cols) ) + 1 ] =
							MakeSubImage( image
											, ( x * image->width ) / cols
											, ( y * image->height ) / rows
											, ( ((x+1) * image->width ) / cols) - (( x * image->width ) / cols)
											, ( ((y+1) * image->height ) / rows) - (( y * image->height ) / rows)
											);
					}
			}
		}
		// level 0 scaled
		current_scale = scaled;
	}
	else
		scaled = NULL;
}

void PEICE_METHODS::SaveBegin( PODBC odbc, PTRSZVAL psvInstance )
{
	lprintf( WIDE("No peice data to save... just the path... please override save to avoid this message.") );
	//return INVALID_INDEX;
}
INDEX PEICE_METHODS::Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance )
{
	lprintf( WIDE("No peice data to save... just the path... please override save to avoid this message.") );
	return INVALID_INDEX;
}
INDEX IPEICE::Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance )
{
	return INVALID_INDEX;
}
PTRSZVAL IPEICE::Load( PODBC odbc, INDEX iInstance )
{
	// by default do nothing....
	return 0;
}

PTRSZVAL PEICE_METHODS::Load( PODBC odbc, INDEX iInstance )
{
	lprintf( WIDE("Load a peice here... Bad programmer, you didn't override this.") );
	return 0;
}



class PEICE:public IPEICE, private PEICE_DATA
{
	// private method therefore safe.
	//Image GetCell( void )
	//{
   //   return scaled[0 * ((rows*cols)+1)];
	//}

	//--------------------------------------------------------------

	//inline Image getcell( S_32 x, S_32 y )
	//{
	//	if( x >=0 && x < cols && y >= 0 && y < rows )
   //      // 0 == scale ...
	//		return scaled[0 * ( (rows*cols) + 1 ) + ( x + (y*cols) ) + 1 ];
   //   return NULL;
	//}

	//--------------------------------------------------------------
	//--------------------------------------------------------------
public:
	PEICE( PIBOARD board
		  , CTEXTSTR name
		  , Image image = NULL
		  , int rows = 1
		  , int cols = 1
		  , int hotx = 0//(cols-1)/2
		  , int hoty = 0//(rows-1)/2
        , int bBlock = 0
		  , int bVia = 0
        , PPEICE_METHODS methods = NULL
		, PTRSZVAL psv = 0
		  ):PEICE_DATA(board, name,image,rows,cols,hotx,hoty,bBlock,bVia)
	{
		// from the original file deifnitino psv here is totally bogus.
		IPEICE::psvCreate = psv; /* brainboard for now?*/
		if( !methods )
			IPEICE::methods = &DefaultMethods;
		else
			IPEICE::methods = methods;
	};
	CTEXTSTR name(void)
	{
		return PEICE_DATA::name;
	}
	void Destroy( void )
	{
		delete this;   
	}
Image getimage( void )
{
   return current_scale[0];
}

Image getimage( int scale )
{
   if( scale < 3 && scale >= 0 )
		return scaled[( (rows*cols) + 1 )*scale];
   return NULL;
}

Image getcell( S_32 x, S_32 y )
{
	if( x < 0 ) x += cols;
	if( y < 0 ) y += rows;
	if( x > cols ) x %= cols;
	if( y < rows ) y %= rows;
	return current_scale[( x + (y*cols) ) + 1 ];
}


Image getcell( S_32 x, S_32 y, int scale )
{
	if( x < 0 ) x += cols;
	if( y < 0 ) y += rows;
	if( x > cols ) x %= cols;
	if( y < rows ) y %= rows;
	{
		if( scale < 3 && scale >= 0 )
			return scaled[( (rows*cols) + 1 )*scale + ( x + (y*cols) ) + 1 ];
	}
   return NULL;
}


void getsize( P_32 rows, P_32 cols )
{
	if( rows )
		*rows = PEICE_DATA::rows;
	if( cols )
		*cols = PEICE_DATA::cols;
}
void gethotspot( PS_32 x, PS_32 y )
{
	if( x ) (*x) = PEICE_DATA::hotx;
	if( y ) (*y) = PEICE_DATA::hoty;
}

	void SaveBegin( PODBC odbc, PTRSZVAL psvInstance )
	{
		return methods->SaveBegin(odbc, psvInstance );
	}
	INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance )
	{
		return methods->Save(odbc,iParent, psvInstance );
	}
	// maybe return true/false maybe the load can fail, caueing the peice to fall
   // off the board.
	PTRSZVAL Load( PODBC odbc, INDEX iInstance )
	{
		return methods->Load(odbc,iInstance );
	}
};


struct VIA_DATA: public PEICE_DATA
{
public:
	VIA_DATA( PIBOARD board, CTEXTSTR name
			  , Image image = NULL
			  ):PEICE_DATA(board, name,image,7,7,0,0,FALSE,TRUE)
	{
	}
};

typedef class VIA *PVIA;
class VIA:public IVIA, public VIA_DATA
{
	//--------------------------------------------------------------
	void Destroy( void );
	Image GetViaEnd( int direction, int scale )
	{
      // to direction...
		switch( direction )
		{
		case UP_LEFT:
			return getcell( 2, 2, scale );
		case UP:
			return getcell( 3, 2, scale );
		case UP_RIGHT:
			return getcell( 4, 2, scale );
		case RIGHT:
			return getcell( 4, 3, scale );
		case DOWN_RIGHT:
			return getcell( 4, 4, scale );
		case DOWN:
			return getcell( 3, 4, scale );
		case DOWN_LEFT:
			return getcell( 2, 4, scale );
		case LEFT:
			return getcell( 2, 3, scale );
		case NOWHERE:
			return getcell( 3, 3, scale );
		}
		return NULL;
	}

	//--------------------------------------------------------------

	Image GetViaFill1( int *xofs, int *yofs, int direction, int scale )
	{
		if( xofs ) (*xofs) = 0;
		if( yofs ) (*yofs) = 0;
		switch( direction )
		{
		case UP_LEFT:
			if( xofs ) (*xofs) = 0;
			if( yofs ) (*yofs) = -1;
			if( 0) {
		case DOWN_RIGHT:
				if( xofs ) (*xofs) = 1;
				if( yofs ) (*yofs) = 0;
			}
			return getcell( 5, 0, scale );
		case UP_RIGHT:
			if( xofs ) (*xofs) = 0;
			if( yofs ) (*yofs) = -1;
			if(0) {
		case DOWN_LEFT:
				if( xofs ) (*xofs) = -1;
				if( yofs ) (*yofs) = 0;
			}
			return getcell( 1, 0, scale );
		}
		return NULL;
	}
	// the diagonal fills are ... well position needs to
   // be accounted for ...

	//--------------------------------------------------------------

	Image GetViaFill2( int *xofs, int *yofs, int direction, int scale )
	{
		// via vills are done when placing a cell that exits in 'direction'
		// the xofs should be applied to the x,y of the last cell - the one that
		// is exiting in 'direction'
		// layers will consider fills as temporary and auto trash them when unwinding.
		// Any cell may call GetViaFill, GetViaFill2 in exit direction,
		// a direction which does not require a fill will result in NULL
		// otherwise the information from this should be saved, and somewhat attached
      // to the peice just layed.
		switch( direction )
		{
		case UP_LEFT:
			if( xofs ) (*xofs) = -1;
			if( yofs ) (*yofs) = 0;
			if( 0) {
		case DOWN_RIGHT:
				if( xofs ) (*xofs) = 0;
				if( yofs ) (*yofs) = 1;
			}
			return getcell( 4, 1, scale );
		case UP_RIGHT:
			if( xofs ) (*xofs) = 1;
			if( yofs ) (*yofs) = 0;
			if(0) {
		case DOWN_LEFT:
				if( xofs ) (*xofs) = 0;
				if( yofs ) (*yofs) = 1;
			}
			return getcell( 2, 1, scale );
		}
      return NULL;
	}

	//--------------------------------------------------------------

	Image GetViaStart( int direction, int scale )
	{
      // from direction...
		switch( direction )
		{
		case UP_LEFT:
			return getcell( 6, 6, scale );
		case UP:
			return getcell( 3, 5, scale );
		case UP_RIGHT:
			return getcell( 0, 6, scale );
		case RIGHT:
			return getcell( 1, 3, scale );
		case DOWN_RIGHT:
			return getcell( 0, 0, scale );
		case DOWN:
			return getcell( 3, 1, scale );
		case DOWN_LEFT:
			return getcell( 6, 0, scale );
		case LEFT:
			return getcell( 5, 3, scale );
		case NOWHERE:
			return getcell( 3, 3, scale );
		}
		return NULL;
	};
	//--------------------------------------------------------------

	Image GetViaFromTo( int from, int to, int scale )
	{
		if( from == NOWHERE )
		{
			return GetViaStart( to, scale );
		}
		else if( to == NOWHERE )
		{
			return GetViaEnd( from, scale );
		}
		switch( from | ( to << 4 ) )
		{
		case LEFT|(UP_RIGHT<<4):
		case UP_RIGHT|(LEFT<<4):
			return getcell( 4, 6, scale );
		case LEFT|(RIGHT<<4):
		case RIGHT|(LEFT<<4):
			return getcell( 3, 0, scale );
		case UP_LEFT|(RIGHT<<4):
		case RIGHT|(UP_LEFT<<4):
			return getcell( 2, 6, scale );
		case LEFT|(DOWN_RIGHT<<4):
		case DOWN_RIGHT|(LEFT<<4):
			return getcell( 4, 0, scale );
		case UP_LEFT|(DOWN_RIGHT<<4):
		case DOWN_RIGHT|(UP_LEFT<<4):
			return getcell( 5, 1, scale );
		case UP|(DOWN_RIGHT<<4):
		case DOWN_RIGHT|(UP<<4):
			return getcell( 0, 4, scale );
		case UP_LEFT|(DOWN<<4):
		case DOWN|(UP_LEFT<<4):
			return getcell( 6, 2, scale );
		case UP|(DOWN<<4):
		case DOWN|(UP<<4):
			return getcell( 0, 3, scale );
		case UP_RIGHT|(DOWN<<4):
		case DOWN|(UP_RIGHT<<4):
			return getcell( 0, 2, scale );
		case UP|(DOWN_LEFT<<4):
		case DOWN_LEFT|(UP<<4):
			return getcell( 6, 4, scale );
		case UP_RIGHT|(DOWN_LEFT<<4):
		case DOWN_LEFT|(UP_RIGHT<<4):
			return getcell( 1, 1, scale );
		case RIGHT|(DOWN_LEFT<<4):
		case DOWN_LEFT|(RIGHT<<4):
			return getcell( 2, 0, scale );
		}
      return NULL;
	}


public:
	CTEXTSTR name(void)
	{
		return VIA_DATA::PEICE_DATA::name;
	}

	// plus additional private methods relating to vias....
	VIA( PIBOARD board
		, CTEXTSTR name
		, Image image = NULL
		, PVIA_METHODS methods = NULL
		, PTRSZVAL psv = 0
		);
	int Move( void ) { return 0; }
	int Stop( void ) { return 0; }

	Image getimage( void )
	{
		return current_scale[0];
	}

	Image getimage( int scale )
	{
		if( scale < 3 && scale >= 0 )
			return scaled[( (rows*cols) + 1 )*scale];
		return NULL;
	}

	Image getcell( S_32 x, S_32 y )
	{
		if( x < 0 ) x += cols;
		if( y < 0 ) y += rows;
		if( x > cols ) x %= cols;
		if( y < rows ) y %= rows;
		return current_scale[( x + (y*cols) ) + 1 ];
	}


	Image getcell( S_32 x, S_32 y, int scale )
	{
		if( x < 0 ) x += cols;
		if( y < 0 ) y += rows;
		if( x > cols ) x %= cols;
		if( y < rows ) y %= rows;
		if( scale < 3 && scale >= 0 )
			return scaled[( (rows*cols) + 1 )*scale + ( x + (y*cols) ) + 1 ];
		return NULL;
	}



	void getsize( P_32 _rows, P_32 _cols )
	{
		if( _rows )
			*_rows = rows;
		if( _cols )
			*_cols = cols;
	}
	void gethotspot( PS_32 _x, PS_32 _y )
	{
		if( _x ) (*_x) = hotx;
		if( _y ) (*_y) = hoty;
	}

	void SaveBegin( PODBC odbc, PTRSZVAL psvInstance )
	{
		return methods->SaveBegin(odbc, psvInstance );
	}
	INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance )
	{
		return methods->Save(odbc,iParent, psvInstance );
	}
	// maybe return true/false maybe the load can fail, caueing the peice to fall
   // off the board.
	PTRSZVAL Load( PODBC odbc, INDEX iInstance )
	{
		return methods->Load(odbc,iInstance );
	}

};

int VIA_METHODS::OnClick( PTRSZVAL psv, S_32 x, S_32 y )
{
	//	lprintf(WIDE(" Psh we have to find segment at %d,%d again... actually we only care if it's the last..."), x, y );
	//	PLAYER_NODE_DATA pld = (PLAYER_NODE_DATA)PeekStack( &pds_path );
	//	if( pld->x == x && pld->y == y )
	{
		// mouse current layer...
		lprintf( WIDE("GENERATE DISCONNECT!") );
		((VIA*)master)->board->UnendPath( );
		//Disconnect();
	}
	return 0;
}
int VIA_METHODS::OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
{
	return 0;
}
int VIA_METHODS::OnDoubleClick( PTRSZVAL psv, S_32 x, S_32 y )
{
	return 0;
}

VIA::VIA( PIBOARD board, CTEXTSTR name
		  , Image image// = NULL
		  , PVIA_METHODS methods// = NULL
		  , PTRSZVAL psv
		  ) : VIA_DATA( board, name, image )
{
	IVIA::via_methods = methods;
	IPEICE::psvCreate = psv;
	methods->via_master = this;
	IPEICE::methods = (PPEICE_METHODS)methods;
	// anything special? yes.
}

void VIA::Destroy( void )
{
	delete this;
}

//IVIA::~IVIA()
//{
//}


PIPEICE DoCreatePeice( PIBOARD board, CTEXTSTR name //= WIDE("A Peice")
						 , Image image //= NULL
						 , int rows //= 1
						 , int cols //= 1
						 , int hotspot_x
						 , int hotspot_y
						 , PPEICE_METHODS methods //= NULL
						 , PTRSZVAL psv
						 )
{
	PPEICE peice = new PEICE( board, name, image, rows, cols, hotspot_x, hotspot_y, TRUE, FALSE, methods, psv );
	if( methods )
		methods->SetPeice( peice );
	return (PIPEICE)peice; // should be able to auto cast this...
}

PIVIA DoCreateVia( PIBOARD board, CTEXTSTR name //= WIDE("A Peice")
											 , Image image //= NULL
											 , PVIA_METHODS methods //= NULL
											 , PTRSZVAL psv
											 )
{
	PVIA via = new VIA( board, name, image, methods, psv );
	if( methods )
		((PVIA_METHODS)methods)->SetPeice( (PIVIA)via );
	return (PIVIA)via; // should be able to auto cast this...
}


