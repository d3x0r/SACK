#define TOOLBIN_SOURCE
#include <stdhdrs.h>
#include <psi.h>

#include "toolbin.hpp"

extern CONTROL_REGISTRATION toolbin_control;

typedef struct local_tag
{
} LOCAL;
static LOCAL l;

typedef class TOOLBIN *PTOOLBIN;
class TOOLBIN
{
public:
	PIBOARD board;
	PCOMMON display;
	uint32_t cell_width, cell_height;
public:
	TOOLBIN(PIBOARD boar);
	void Init( PIBOARD board );

};

int CPROC MouseToolbin( PCOMMON pc, int32_t x, int32_t y, uint32_t b )
{
   return TRUE;
}

int CPROC DrawToolbin( PCOMMON pc )
{
	ValidatedControlData( PTOOLBIN, toolbin_control.TypeID, toolbin, pc );
	if( toolbin )
	{
		int x, y;
		INDEX idx;
		PIPEICE peice;
		Image image = GetControlSurface( pc );
		x = 5;
		y = 5;

		for( peice = toolbin->board->GetFirstPeice( &idx );
			 peice;
			  peice = toolbin->board->GetNextPeice( &idx ) )
		{
			uint32_t h;
			if( peice->methods )
				peice->methods->Draw( (uintptr_t)NULL, image, peice->getimage(), x, y );
			peice->methods->getsize( &h, NULL );
			y += h * toolbin->cell_height;
		}
	}
	return TRUE;
}



void TOOLBIN::Init( PIBOARD board )
{
	TOOLBIN::board = board;
	board->GetCellSize( &cell_width, &cell_height, 0 );
	// by default show the toolbin also
	DisplayFrame( display );
}


PSI_CONTROL SetBoardControlBoard( PCOMMON pc, PIBOARD board )
{
	ValidatedControlData( PTOOLBIN, toolbin_control.TypeID, toolbin, pc );
	if( toolbin )
	{
		toolbin->display = pc;
		toolbin->Init(board);
	}
	return pc;
}

TOOLBIN::TOOLBIN( PIBOARD board )
{
	display = SetBoardControlBoard( MakeControl( NULL, toolbin_control.TypeID
															 , 0, 0, 0, 0
															 , 0 )
											, board );
}

int CPROC InitToolbinControl( PCOMMON pc )
{
   return TRUE;
}

CONTROL_REGISTRATION toolbin_control = { WIDE("Board Toolbin")
										 , { { 64, 256 }, sizeof( TOOLBIN ), BORDER_NORMAL }
										 , InitToolbinControl // Init
										 , NULL // Load
										 , DrawToolbin
										 , MouseToolbin
};

PRELOAD( RegisterToolbin )
{
   DoRegisterControl( &toolbin_control );
}

void CreateToolbin( PIBOARD board )
{
   PTOOLBIN toolbin = new TOOLBIN( board );
}

