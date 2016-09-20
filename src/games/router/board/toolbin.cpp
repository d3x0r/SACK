#ifndef TOOLBIN_SOURCE
#define TOOLBIN_SOURCE
#endif
#include "interface.h"
#include <sack_types.h>
#include <psi.h>

#include "toolbin.hpp"
#include "global.h"

#define TILE_SIZE 32
#define TILE_PAD 4

typedef struct toolbin_tool *PTOOL;


extern CONTROL_REGISTRATION toolbin_control;

typedef struct local_tag
{
   PTOOLBIN creating;
} LOCAL;
static LOCAL l;

class TOOLBIN
{
public:
	PIBOARD board;
	PCOMMON display;
   PTOOL selected_tool;
   PLIST tools;
	uint32_t cell_width, cell_height;
   uint32_t b;
public:
	TOOLBIN(PIBOARD boar);
	TOOLBIN(PIBOARD boar, PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h );
   void Init( PIBOARD board );

};

struct toolbin_tool
{
	PIPEICE peice;
   Image image;
	int x, y;
	unsigned w, h;
	PLIST child_tools;

};

struct toolbin_tool *GetTool( PTOOLBIN toolbin, PIPEICE peice )
{
	struct toolbin_tool* tool;
   INDEX idx;
	LIST_FORALL( toolbin->tools, idx, struct toolbin_tool*, tool )
	{
		if( tool->peice == peice )
         return tool;
	}
	if( !tool )
	{
		tool = New( struct toolbin_tool );
      tool->image = NULL;
		tool->peice = peice;
		tool->child_tools = NULL;
      AddLink( &toolbin->tools, tool );
	}
   return tool;
}



int CPROC MouseToolbin( PCOMMON pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PTOOLBIN, toolbin_control.TypeID, toolbin, pc );
	if( toolbin )
	{
		if( ( b & MK_LBUTTON ) && ( !( toolbin->b & MK_LBUTTON ) ) )
		{
			INDEX idx;
			PTOOL tool;
			LIST_FORALL( toolbin->tools, idx, PTOOL, tool )
			{
				if( ( x >= tool->x && x < (tool->x+tool->w) )
					&& ( y >= tool->y && y < (tool->y+tool->h) ) )
				{
					toolbin->selected_tool = tool;
               toolbin->board->SetSelectedTool( tool->peice );
					SmudgeCommon( pc );

				}

			}
		}
      toolbin->b = b;
	}
   

   return TRUE;
}

static int OnDrawCommon( WIDE("Board toolbin") )( PCOMMON pc )
{
	ValidatedControlData( PTOOLBIN, toolbin_control.TypeID, toolbin, pc );
	if( toolbin )
	{
		int x, y;
		INDEX idx;
		PIPEICE peice;
		Image image = GetControlSurface( pc );
		ClearImageTo( image, BASE_COLOR_BLACK );

		x = TILE_PAD;
		y = TILE_PAD;

		{
			PTOOL tool = toolbin->selected_tool;
			if( tool )
			{
				BlatColor( image
							, tool->x-TILE_PAD, tool->y-TILE_PAD, tool->w+TILE_PAD*2, tool->h+TILE_PAD*2
							, BASE_COLOR_WHITE );
			}
		}

		for( peice = toolbin->board->GetFirstPeice( &idx );
			 peice;
			  peice = toolbin->board->GetNextPeice( &idx ) )
		{
			PTOOL tool = GetTool( toolbin, peice );
			if( !tool->image )
			{
				tool->x = x;
				tool->y = y;
				tool->w = TILE_SIZE;
				tool->h = TILE_SIZE;
				tool->image = peice->getimage();


				y += TILE_SIZE + TILE_PAD;//h * toolbin->cell_height;
				if( y > image->height )
				{
					y = TILE_PAD;
					x += TILE_SIZE + TILE_PAD;
				}
			}

			BlotScaledImageSizedTo( image, tool->image, tool->x, tool->y, tool->w, tool->h );
         PutString( image, tool->x + TILE_SIZE + TILE_PAD, tool->y, BASE_COLOR_WHITE, 0, tool->peice->name() );
			//if( peice->methods )
			//	peice->methods->Draw( (uintptr_t)NULL, image, peice->getimage(), x, y );
			//peice->methods->getsize( &h, NULL );
		}
	}
	return TRUE;
}



void TOOLBIN::Init( PIBOARD board )
{
	TOOLBIN::board = board;
	tools = NULL;
   selected_tool = NULL;
	board->GetCellSize( &cell_width, &cell_height, 0 );
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
	Init( board );
   l.creating = this;
	display = SetBoardControlBoard( MakeControl( NULL, toolbin_control.TypeID
															 , 0, 0, 0, 0
															 , 0 )
											, board );
	DisplayFrame( display );
   l.creating = NULL;

}

TOOLBIN::TOOLBIN( PIBOARD board, PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
   Init( board );
   l.creating = this;
	display = SetBoardControlBoard( MakeControl( parent, toolbin_control.TypeID
															 , x, y, w, h
															 , 0 )
											, board );
   l.creating = NULL;
}

int CPROC InitToolbinControl( PCOMMON pc )
{
	SetControlData( PTOOLBIN, pc, l.creating );
   SetCommonTransparent( pc, TRUE );
   return TRUE;
}

CONTROL_REGISTRATION toolbin_control = { WIDE("Board Toolbin")
										 , { { 64, 256 }, sizeof( TOOLBIN ), BORDER_NORMAL }
										 , InitToolbinControl // Init
										 , NULL // Load
										 , NULL //DrawToolbin
										 , MouseToolbin
};

PRELOAD( RegisterToolbin )
{
   DoRegisterControl( &toolbin_control );
}

PTOOLBIN CreateToolbin( PIBOARD board )
{
	PTOOLBIN toolbin = new TOOLBIN( board );
   return toolbin;
}

PSI_CONTROL CreateToolbinControl( PIBOARD board, PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{

	PTOOLBIN toolbin = new TOOLBIN( board, parent, x, y, w, h );
   return toolbin->display;
}


TOOLBIN_PROC( PTOOLBIN, GetToolbinFromControl )( PSI_CONTROL pc )
{
   ValidatedControlData( PTOOLBIN, toolbin_control.TypeID, toolbin, pc );
	return toolbin;
}
