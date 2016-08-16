//long long _rdtsc()
#include <stdhdrs.h>
#include <sharemem.h>
#include <msgclient.h>
#include <logging.h>
#include <vectlib.h>
#include <timers.h>


#define FLATLAND_MAIN
#include "flatland_global.h"

#include <controls.h>
#include <psi.h>
#include <render.h>
#include <keybrd.h>

#include <virtuality.h>

#include "fileopen.h"
#include "sectdlg.h"
#include "drawname.h"
#include "drawtexture.h"
#include "display.h"
#include "displaydlg.h"
//#include "undo.h"
#include "displayfile.h"



//#define CHECK_ALIVE // sends messages to the window processing thread.
//#define MessageBox( w,m,t,o ) MessageBoxA( NULL, m, t, o )

//static LOCAL l;

#define _display (*g.pDisplay)
// need extern here for C++ weakness.
extern CONTROL_REGISTRATION editor;

PMENU hMainMenu;
PMENU hOriginMenu;
PMENU hSectorMenu;
PMENU hSelectMenu;
PMENU hZoomMenu;

#define MNU_QUIT		  997
#define MNU_RESET		  998
#define MNU_NEWSECTOR	 999
#define MNU_SAVE		  1000
#define MNU_LOAD		  1001

#define MNU_MARK		  1002
#define MNU_MERGE		 1003
#define MNU_SPLIT		 1004
#define MNU_DELETELINE  1011

#define MNU_DELETE		 1005
#define MNU_SECTPROP	  1012
#define MNU_SECTSELECT	1006
#define MNU_WALLSELECT	1007
#define MNU_MERGEWALLS	1008
#define MNU_UNSELECTSECTORS 1009
#define MNU_ADDLINE		1010

#define MNU_MINZOOM		1020
#define MNU_MAXZOOM		MNU_MINZOOM + 11

#define MNU_DISPLAYPROP  1050
#define MNU_SETTOP		 1051
#define MNU_BREAK		  1052
#define MNU_UNDO			1053
#define MNU_MERGEOVERLAP 1054
//----------------------------------------------------------------------------
#ifdef CHECK_ALIVE
int global_alive;
int is_alive;
unsigned long CPROC CheckAliveThread( void *unused )
{
	while(_display.hVideo->flags.bReady)
	{
		int i;
		global_alive = 0;	 
		if( !PostMessage( _display.hVideo->hWndOutput, WM_RUALIVE, 0, (LPARAM)&global_alive ) )
			Log( WIDE("Failed to post message...") );
		else
		{
			for( i = 0; i < 5; i++ )
			{
				while( is_alive ) // force knowledge that we are alive...
				{
					//Log1( WIDE("Forced alive: %d"), i );
					Sleep(100);
				}
				if( global_alive )
					break;
				Sleep(50);
			}	  
			if( !global_alive ) // half a second - no responce to alive message...
			{
				Log( WIDE("Window thread is dead!") );
				//DebugBreak();
			}
		}
		Sleep(50);
	}
	return 0;
}
#endif
//----------------------------------------------------------------------------

void ClearCurrentSectors( PDISPLAY display )
{
	if( display->flags.bSectorList )
	{
		Release( display->CurSector.SectorList );
		display->flags.bSectorList = FALSE;
	}
	display->CurSector.SectorList = &display->CurSector.Sector;
	display->CurSector.Sector = INVALID_INDEX; // eh - just cause it's similar
	display->nSectors = 0;
}

//----------------------------------------------------------------------------

void ClearCurrentWalls( PDISPLAY display )
{
	if( display->flags.bWallList )
	{
		Release( display->CurWall.WallList );
		display->flags.bWallList = FALSE;
	}
	display->CurWall.WallList = &display->CurWall.Wall;
	display->CurWall.Wall = INVALID_INDEX; // eh - just cause it's similar
	display->nWalls = 0;
}

//----------------------------------------------------------------------------

#define MARK_SIZE 3

void DrawOrigin( PDISPLAY display )
{
	do_line( display->pImage, display->CurOrigin[0] - MARK_SIZE
						, display->CurOrigin[1] - MARK_SIZE
						, display->CurOrigin[0] + MARK_SIZE
						, display->CurOrigin[1] - MARK_SIZE
						, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurOrigin[0] + MARK_SIZE
						, display->CurOrigin[1] - MARK_SIZE
						, display->CurOrigin[0] + MARK_SIZE
						, display->CurOrigin[1] + MARK_SIZE
						, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurOrigin[0] + MARK_SIZE
						, display->CurOrigin[1] + MARK_SIZE
						, display->CurOrigin[0] - MARK_SIZE
						, display->CurOrigin[1] + MARK_SIZE
						, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurOrigin[0] - MARK_SIZE
						, display->CurOrigin[1] + MARK_SIZE
						, display->CurOrigin[0] - MARK_SIZE
						, display->CurOrigin[1] - MARK_SIZE
						, Color( 200, 200, 200 ) );
	
}

//----------------------------------------------------------------------------

void DrawSlope( PDISPLAY display )
{
	do_line( display->pImage, display->CurSlope[0],
						  display->CurSlope[1] - MARK_SIZE,
						  display->CurSlope[0] + MARK_SIZE, 
						  display->CurSlope[1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurSlope[0] + MARK_SIZE,
						  display->CurSlope[1] + MARK_SIZE,
						  display->CurSlope[0] - MARK_SIZE, 
						  display->CurSlope[1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurSlope[0] - MARK_SIZE,
						  display->CurSlope[1] + MARK_SIZE,
						  display->CurSlope[0], 
						  display->CurSlope[1] - MARK_SIZE, Color( 200, 200, 200 ) );
}

void DrawEnds( PDISPLAY display )
{
	do_line( display->pImage, display->CurEnds[0][0],
						  display->CurEnds[0][1] - MARK_SIZE,
						  display->CurEnds[0][0] + MARK_SIZE, 
						  display->CurEnds[0][1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurEnds[0][0] + MARK_SIZE,
						  display->CurEnds[0][1] + MARK_SIZE,
						  display->CurEnds[0][0] - MARK_SIZE, 
						  display->CurEnds[0][1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurEnds[0][0] - MARK_SIZE,
						  display->CurEnds[0][1] + MARK_SIZE,
						  display->CurEnds[0][0], 
						  display->CurEnds[0][1] - MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurEnds[1][0],
						  display->CurEnds[1][1] - MARK_SIZE,
						  display->CurEnds[1][0] + MARK_SIZE, 
						  display->CurEnds[1][1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurEnds[1][0] + MARK_SIZE,
						  display->CurEnds[1][1] + MARK_SIZE,
						  display->CurEnds[1][0] - MARK_SIZE, 
						  display->CurEnds[1][1] + MARK_SIZE, Color( 200, 200, 200 ) );
	do_line( display->pImage, display->CurEnds[1][0] - MARK_SIZE,
						  display->CurEnds[1][1] + MARK_SIZE,
						  display->CurEnds[1][0], 
						  display->CurEnds[1][1] - MARK_SIZE, Color( 200, 200, 200 ) );
}

//----------------------------------------------------------------------------

void DrawSectorOrigin( PDISPLAY display )
{
#define ORIGINCOLOR Color( 200, 32, 54 )
	do_line( display->pImage, display->CurSecOrigin[0] - MARK_SIZE
						, display->CurSecOrigin[1] 
						, display->CurSecOrigin[0] 
						, display->CurSecOrigin[1] - MARK_SIZE
						, ORIGINCOLOR );
	do_line( display->pImage, display->CurSecOrigin[0] 
						, display->CurSecOrigin[1] - MARK_SIZE
						, display->CurSecOrigin[0] + MARK_SIZE
						, display->CurSecOrigin[1] 
						, ORIGINCOLOR );
	do_line( display->pImage, display->CurSecOrigin[0] + MARK_SIZE
						, display->CurSecOrigin[1] 
						, display->CurSecOrigin[0] 
						, display->CurSecOrigin[1] + MARK_SIZE
						, ORIGINCOLOR );
	do_line( display->pImage, display->CurSecOrigin[0] 
						, display->CurSecOrigin[1] + MARK_SIZE
						, display->CurSecOrigin[0] - MARK_SIZE
						, display->CurSecOrigin[1] 
						, ORIGINCOLOR );
}

//----------------------------------------------------------------------------


void DrawLine( PDISPLAY display, P_POINT m, P_POINT o, RCOORD start, RCOORD end, CDATA color )
{
	VECTOR p1, p2, d;
	if( start == NEG_INFINITY || start == POS_INFINITY 
		|| ( end == NEG_INFINITY || end == POS_INFINITY ) )
		return;

	addscaled( p1, o, m, start );
	addscaled( p2, o, m, end );
	if( Length( sub( d, p1, p2 ) ) > 100000 )
		DebugBreak();
	p1[0] = DISPLAY_X(display, p1[0]);
	p2[0] = DISPLAY_X(display, p2[0]);
	p1[1] = DISPLAY_Y(display, p1[1]);
	p2[1] = DISPLAY_Y(display, p2[1]);
	do_line( display->pImage
			 , (int32_t)p1[0], (int32_t)p1[1]
			 , (int32_t)p2[0], (int32_t)p2[1]
			 , color );
}

#if 0
void DrawLineSeg( PLINESEG pl, CDATA color )
{
	DrawLine( &_display, pl->r.n, pl->r.o, pl->dFrom, pl->dTo, color );
}
#endif

//----------------------------------------------------------------------------

void DrawRect( Image pImage, int x, int y, int w, int h, CDATA c, CDATA cd )
{
	if( c )
	{
		do_lineAlpha( pImage, x, y, x + w, y, c );
		do_lineAlpha( pImage, x, y, x, y + h, c );
		do_lineAlpha( pImage, x + w, y, x + w, y + h, c );
		do_lineAlpha( pImage, x, y + h, x + w, y + h, c );
	}
	if( cd )
	{
		do_lineAlpha( pImage, x, y, x + w, y + h, cd );
	}
}

//----------------------------------------------------------------------------
#if 0
void DrawSpaceLines( Image pImage, PSPACENODE space )
{	  
/*
	if( !space )
		return;
	while( space->prior )
		space = space->prior;
	while( space )
	{
		do_hline( pImage, DISPLAY_Y( &display, space->min[1] )
							 , DISPLAY_X( &display, space->min[0] )
							 , DISPLAY_X( &display, space->max[0] )
							 , WHITE );
		do_hline( pImage, DISPLAY_Y( &display, space->max[1] )
							 , DISPLAY_X( &display, space->min[0] )
							 , DISPLAY_X( &display, space->max[0] )
							 , WHITE );
		do_vline( pImage, DISPLAY_X( &display, space->min[0] )
							 , DISPLAY_Y( &display, space->min[1] )
							 , DISPLAY_Y( &display, space->max[1] )
							 , WHITE );
		do_vline( pImage, DISPLAY_X( &display, space->max[0] )
							 , DISPLAY_Y( &display, space->min[1] )
							 , DISPLAY_Y( &display, space->max[1] )
							 , WHITE );
		space = space->next;						 
	}
*/
}
#endif

//----------------------------------------------------------------------------

uintptr_t CPROC DrawSectorLines( INDEX pSector, uintptr_t pc )
{
	INDEX iSector = (INDEX)pSector;
	PDISPLAY display = ControlData( PDISPLAY, (PSI_CONTROL)pc );
	INDEX iWorld = display->pWorld;
	INDEX pStart, pCur;
	int count=0;
	int priorend = TRUE;
	//DrawSpaceLines( display->pImage, pSector->spacenode );
	pCur = pStart = GetFirstWall( iWorld, (INDEX)pSector, &priorend );
	do
	{
		//GETWORLD( display->pWorld );
		INDEX iLine;
		PFLATLAND_MYLINESEG Line;
		//VECTOR p1, p2;
		int l = LineInCur( display->pWorld
							  , display->CurSector.SectorList, display->nSectors
							  , display->CurWall.WallList, display->nWalls
							  , iLine = GetWallLine( display->pWorld, pCur ));
		GetLineData( iWorld, iLine, &Line );
		// stop infinite looping... may be badly linked...
		count++;
		 if( count > 20 )
			break;
		if( l == 0 )
		{
			if( GetMatedWall( display->pWorld, pCur ) )
			{
				if( pCur != display->MarkedWall )
				{
					DrawLine( display, Line->r.n, Line->r.o, 
											Line->dFrom, Line->dTo, Color( 63,63,63 ) );
				}
				else
				{
					DrawLine( display, Line->r.n, Line->r.o, 
											Line->dFrom, Line->dTo, Color( 0,150,150 ) );
				}
			}
			else
			{
				if( pCur != display->MarkedWall )
				{
					DrawLine( display, Line->r.n, Line->r.o, 
											Line->dFrom, Line->dTo, Color( 240,240,240 ) );
				}
				else
				{
					DrawLine( display, Line->r.n, Line->r.o, 
											Line->dFrom, Line->dTo, Color( 0,210,210 ) );
				}
			}
		}
		else if( l == 1 )
		{
			if( pCur != display->MarkedWall )
			{
				DrawLine( display, Line->r.n, Line->r.o, 
										Line->dFrom, Line->dTo, Color( 23,180,23 ) );
			}
			else
			{
				DrawLine( display, Line->r.n, Line->r.o, 
										Line->dFrom, Line->dTo, Color( 0,210,210 ) );
			}
		}
		else
		{
			RCOORD len = (Line->dTo - Line->dFrom) / 3;
			DrawLine( display, Line->r.n, Line->r.o, 
									 Line->dFrom, Line->dTo, Color( 255,255,23 ) );

			DrawLine( display, Line->r.n, Line->r.o, 
									 Line->dTo, Line->dTo + len, Color( 255,23,23 ) );

			DrawLine( display, Line->r.n, Line->r.o, 
									 Line->dFrom - len, Line->dFrom, Color( 255,23,23 ) );

		}
		pCur = GetNextWall( display->pWorld
								, pCur, &priorend );
	}while( pCur != pStart );

	return 0; // continue drawing... non zero breaks loop...
}

//----------------------------------------------------------------------------

void AlignPointToGrid( PDISPLAY display, P_POINT o )
{
	if( display->flags.bUseGrid )
	{
#define XUnits(display) ( (display)->GridXUnits/* * (display)->scale*/ )
#define YUnits(display) ( (display)->GridYUnits/* * (display)->scale */)
		if( o[0] < 0 )
			o[0] = (RCOORD)((int)(( o[0] / XUnits(display))-0.5 ))*(RCOORD)XUnits(display);
		else
			o[0] = (RCOORD)((int)(( o[0] / XUnits(display))+0.5 ))*(RCOORD)XUnits(display);
		if( o[1] < 0 )
			o[1] = (RCOORD)((int)(( o[1] / YUnits(display))-0.5 ))*(RCOORD)YUnits(display);
		else
			o[1] = (RCOORD)((int)(( o[1] / YUnits(display))+0.5 ))*(RCOORD)YUnits(display);
	}

}

//----------------------------------------------------------------------------

void SetCurWall( PSI_CONTROL pc )
{
	ValidatedControlData( PDISPLAY, editor.TypeID, display, pc );
	INDEX line;
	PFLATLAND_MYLINESEG pLine;
	int x, y;
	_POINT o;
	display->CurWall.Wall = display->NextWall.Wall;
	display->NextWall.Wall = INVALID_INDEX;
	display->CurSector.Sector = GetWallSector( display->pWorld, display->CurWall.Wall );
	display->nSectors = 1;
	display->nWalls = 1;
	line = GetWallLine( display->pWorld, display->CurWall.Wall );

	do
	{
		GetLineData( display->pWorld, line, &pLine );
		Relinquish();
	} while( !pLine );
	AlignPointToGrid( display, pLine->r.o );
	//UpdateMatingLines( display->pWorld, display->CurWall.Wall, FALSE, TRUE );
	x = (int)DISPLAY_X(display, pLine->r.o[0] );
	y = (int)DISPLAY_Y(display, pLine->r.o[1] );
	display->x = x;
	display->y = y;
	o[0] = REAL_X( display, x );
	o[1] = REAL_Y( display, y );
	o[2] = 0;
	SetFrameMousePosition( pc, x, y );
	//SmudgeCommon( pc );
}

//----------------------------------------------------------------------------

uintptr_t CPROC DrawSectorName( INDEX p, uintptr_t psv )
{
	INDEX iSector = (INDEX)p;
	PDISPLAY display = (PDISPLAY)psv;
	INDEX iWorld = display->pWorld;
	INDEX iName = GetSectorName( display->pWorld, iSector );
	//char text[256];
	PNAME name;
	if( iName != INVALID_INDEX )
	{
		GetNameData( iWorld, iName, &name );
		if( name )
		{
			_POINT o;
			int x, y;
			GetSectorOrigin( iWorld, iSector, o );
			x = (int)DISPLAY_X( display, o[0] );
			y = (int)DISPLAY_Y( display, o[1] );
			DrawName( display->pImage, name, x, y );
		}
	}
	return 0; // continue drawing... non zero breaks loop...
}

//----------------------------------------------------------------------------

int dotiming;

void DrawDisplayGrid( PDISPLAY display )
{
	int x, y;
	//maxx, maxy, miny,, incx, incy, delx, dely, minx
	uint32_t start= GetTickCount();;

	if( display->flags.bShowGrid )
	{
		int DrawSubLines, DrawSetLines;
		// , drawn
		RCOORD units, set;
		RCOORD rescale = 1.0;
		int drawnSet = FALSE, drawnUnit = FALSE;
		do
		{
			DrawSubLines = DrawSetLines = TRUE;
			units = ((RCOORD)display->GridXUnits*rescale);// * display->scale;
			if( DISPLAY_SCALE( display, units ) < 5 )
				DrawSubLines = FALSE;
			set = ((RCOORD)( display->GridXUnits * display->GridXSets*rescale ));// * display->scale;
			if( DISPLAY_SCALE( display, set ) < 5 )
				DrawSetLines = FALSE;
			if( !DrawSubLines )
				rescale *= 2;
		} while( !DrawSubLines );
			
		for( x = 0; x < display->pImage->width; x++ )
		{
			RCOORD real = REAL_X( display, x );
			RCOORD nextreal = REAL_X( display, x+1 );
			int setdraw, unitdraw;
			if( real > 0 )
			{
				setdraw = ( (int)(nextreal/set)==((int)(real/set)));
				unitdraw = ( (int)(nextreal/units)==((int)(real/units)) );
				//Log7( WIDE("test: %d %d %d %g %g %d %d"), setdraw, unitdraw, x, nextreal, units, (int)(nextreal/units), (int)(real/units) );
			}
			else
			{
				setdraw = ( ((int)(nextreal/set))!=((int)(real/set)) );
				unitdraw = ( ((int)(nextreal/units))!=((int)(real/units)) );
				//Log7( WIDE("test: %d %d %d %g %g %d %d"), setdraw, unitdraw, x, nextreal, units, (int)(nextreal/units), (int)(real/units) );
			}

			if( !setdraw )
				drawnSet = FALSE;
			if( !unitdraw )
				drawnUnit = FALSE;

			if( (real <= 0) && (nextreal > 0) )
			{
				do_vlineAlpha( display->pImage
								, x
								, 0, display->pImage->height
								, AColor( 255, 255, 255, 64 ) );
				drawnUnit = TRUE;
				drawnSet = TRUE;
			}
			else if( DrawSetLines && setdraw && !drawnSet )
			{
				do_vlineAlpha( display->pImage
								, x
								, 0, display->pImage->height
								, SetAlpha( display->GridColor, 96 ) );
				drawnSet = TRUE;
				drawnUnit = TRUE;
			}
			else if( DrawSubLines && unitdraw && !drawnUnit)
			{
				do_vlineAlpha( display->pImage
								, x
								, 0, display->pImage->height
								, SetAlpha( display->GridColor, 48 ) );
				drawnUnit = TRUE;
			}
		}
		if( dotiming ) 
		{
			Log3( WIDE("%s(%d): %d Vertical Grid"), __FILE__, __LINE__, GetTickCount() - start );
			start = GetTickCount();
		}  

		drawnSet = drawnUnit = FALSE;
		rescale = 1.0;
		do
		{
			DrawSubLines = DrawSetLines = TRUE;

			units = ((RCOORD)display->GridYUnits*rescale);// * display->scale;
			if( DISPLAY_SCALE( display, units ) < 5 )
				DrawSubLines = FALSE;
			set = ((RCOORD)( display->GridYUnits * display->GridYSets *rescale));// * display->scale;
			if( DISPLAY_SCALE( display, set ) < 5 )
				DrawSetLines = FALSE;
			if( !DrawSubLines )
				rescale *= 2;
		} while( !DrawSubLines );

		for( y = display->pImage->height - 1; y >= 0 ; y-- )
		//for( y = 0; y < display->pImage->height ; y++ )
		{
			RCOORD real = REAL_Y( display, y );
			RCOORD nextreal = REAL_Y( display, y-1 );
			//RCOORD nextreal = REAL_Y( display, y+1 );
			int setdraw, unitdraw;
			if( real > 0 )
			{
				setdraw = ( (int)(nextreal/set)==((int)(real/set)));
				unitdraw = ( (int)(nextreal/units)==((int)(real/units)) );
			}
			else
			{
				setdraw = ( ((int)(nextreal/set))!=((int)(real/set)) );
				unitdraw = ( ((int)(nextreal/units))!=((int)(real/units)) );
			}
			if( !setdraw )
				drawnSet = FALSE;
			if( !unitdraw )
				drawnUnit = FALSE;

			if( (real <= 0) && (nextreal > 0) )
			{
				do_hlineAlpha( display->pImage
								, y
								, 0, display->pImage->width
								, AColor( 255, 255, 255, 64 ) );
				drawnUnit = TRUE;
				drawnSet = TRUE;
			}
			else if( DrawSetLines && setdraw && !drawnSet )
			{
				do_hlineAlpha( display->pImage
								, y
								, 0, display->pImage->width
								, SetAlpha( display->GridColor, 96 ) );
				drawnSet = TRUE;
				drawnUnit = TRUE;
			}
			else if( DrawSubLines && unitdraw && !drawnUnit)
			{
				do_hlineAlpha( display->pImage
								, y
								, 0, display->pImage->width
								, SetAlpha( display->GridColor, 48 ) );
				drawnUnit = TRUE;
			}
		}
		if( dotiming )
		{
			Log3( WIDE("%s(%d): %d Horizontal Grid"), __FILE__, __LINE__, GetTickCount() - start );
			start = GetTickCount();
		}
	}
	/*
	else
	{
	draw_major_only:
		do_lineAlpha( display->pImage
						, display->xbias, 0  
						, display->xbias, display->pImage->height
						, AColor(255,255,255,64) );
		do_lineAlpha( display->pImage
						, 0, display->ybias
						, display->pImage->width, display->ybias
						, AColor(255,255,255,64) );
	}
	*/
}

//----------------------------------------------------------------------------

void RedrawWorld( PCOMMON pc, int nLine );

static int OnDrawCommon( WIDE("Flatland Editor") )( PCOMMON pc )
{
	ValidatedControlData( PDISPLAY, editor.TypeID, pd, pc );
	if( pd )
	{
		pd->pImage = GetControlSurface( pc );
		RedrawWorld( pc, __LINE__ );
	}
	return 1;
}

//----------------------------------------------------------------------------

int sectorsdrawn, sectorsskipped;

int mousemove;
int updateonmove;
int lastupdate;
void RedrawWorld( PCOMMON pc, int nLine )
#define RedrawWorld( psv ) RedrawWorld( psv, __LINE__ )
{
	//PDISPLAY display = (PDISPLAY)psvUser;
	PDISPLAY display = ControlData( PDISPLAY, pc );
	uint32_t start, resizestart;
	//if( updateonmove == mousemove )
	//	Log3( WIDE("Dual Draw World %d last:%d this:%d"), mousemove, lastupdate, nLine );

   AcceptChanges(); // make sure we get all changes applied before drawing.
	updateonmove = mousemove;
	lastupdate = nLine;
#ifdef CHECK_ALIVE
	is_alive = TRUE;
#endif

	if( dotiming )
		resizestart = start = GetTickCount();

	//ValidateWorldLinks( display->pWorld );
	//Log3( WIDE("%s(%d): %d Validate"), __FILE__, __LINE__, GetTickCount() - start );
	//start = GetTickCount();

	ClearImageTo( display->pImage, display->Background );

	// origin should be in real space coordinate...
	// therefore we have to translate that to display space...
	// the image coordinates are already display space...

	// therefore xbias and ybias are display space things...
	display->xbias = display->pImage->width/2;
	display->ybias = display->pImage->height/2;
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Clear/Init"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}

	if( !display->flags.bGridTop )
		DrawDisplayGrid( display );
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Grid(top)"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}
	sectorsdrawn = 0;
	sectorsskipped = 0;

	if( display->flags.bShowSectorTexture )
	{
		//lprintf( WIDE("Draw sector texture.") );
		ForAllSectors( display->pWorld, DrawSectorTexture, (uintptr_t)pc );
	}
	if( dotiming )
	{
		Log5( WIDE("%s(%d): %d Sector Textures %d %d"), __FILE__, __LINE__, GetTickCount() - start
						, sectorsdrawn, sectorsskipped);
		start = GetTickCount();
	}
	if( display->flags.bShowLines || 
		!display->flags.bShowSectorTexture )
		ForAllSectors( display->pWorld, DrawSectorLines, (uintptr_t)pc );
	else
	{ 
		int n;
		for( n = 0; n < display->nSectors; n++ )
		{
			DrawSectorLines( display->CurSector.SectorList[n], (uintptr_t)pc );
			//DrawSpaceLines( display->pImage
			//				, display->CurSector.SectorList[n]->spacenode );
		}
		// eh - need to show current selected walls here...
		// 
		//
		//for( n = 0; n < 
	}
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d SectorLines"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}


	if( display->flags.bShowSectorText )
		ForAllSectors( display->pWorld, DrawSectorName, (uintptr_t)display );
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Sector Text"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}

	if( display->flags.bNormalMode )
	{
		// a wall list breaks this method of drawing the normal points...
		if( !display->flags.bWallList && display->CurWall.Wall != INVALID_INDEX )
		{
			PFLATLAND_MYLINESEG line;
			VECTOR v;
			GetLineData( display->pWorld, GetWallLine( display->pWorld, display->CurWall.Wall ), &line );
			addscaled( v, line->r.o, line->r.n, line->dFrom );
#ifdef OUTPUT_TO_VIRTUALITY
			add( v, v, line->pfl->normals.at_from );
			display->CurEnds[0][0] = DISPLAY_X( display, v[0] );
			display->CurEnds[0][1] = DISPLAY_Y( display, v[1] );
			addscaled( v, line->r.o, line->r.n, line->dTo );
			add( v, v, line->pfl->normals.at_to );
			display->CurEnds[1][0] = DISPLAY_X( display, v[0] );
			display->CurEnds[1][1] = DISPLAY_Y( display, v[1] );
			DrawEnds( display );
#endif
		}
	}
	else
	{
		if( display->nWalls )
		{
			ComputeWallSetOrigin( display->pWorld, display->nWalls
									  , display->CurWall.WallList
									  , display->WallOrigin );
			AlignPointToGrid( display, display->WallOrigin );
			display->CurOrigin[0] = (int)DISPLAY_X(display,display->WallOrigin[0]);
			display->CurOrigin[1] = (int)DISPLAY_Y(display,display->WallOrigin[1]);
			DrawOrigin( display );
		}

		if( !display->flags.bWallList && display->CurWall.Wall != INVALID_INDEX )
		{
			PFLATLAND_MYLINESEG line;
			GetLineData( display->pWorld, GetWallLine( display->pWorld, display->CurWall.Wall ), &line );
//#define IsKeyDown(h,key) IsKeyDown(NULL,key)
			if( IsKeyDown( display->hVideo, KEY_SHIFT ) )
			{
				display->CurSlope[0] = (int)DISPLAY_X( display
														  , line->r.o[0] 
														  + line->r.n[0] );
				display->CurSlope[1] = (int)DISPLAY_Y( display
														  , line->r.o[1]
														  + line->r.n[1] );
				DrawSlope( display );
			}
			else
			{
				display->CurEnds[0][0] = (int)DISPLAY_X( display
														  , line->r.o[0] 
														  + line->r.n[0]
														  * line->dFrom );
				display->CurEnds[0][1] = (int)DISPLAY_Y( display
														  , line->r.o[1]
														  + line->r.n[1]
														  * line->dFrom );
				display->CurEnds[1][0] = (int)DISPLAY_X( display
														  , line->r.o[0] 
														  + line->r.n[0]
														  * line->dTo );
				display->CurEnds[1][1] = (int)DISPLAY_Y( display
														  , line->r.o[1]
														  + line->r.n[1]
														  * line->dTo );
				DrawEnds( display );
			}
		}
	if( display->nSectors )
	{
		//Log1( WIDE("Sectorset is %d sectors...\n"), display->nSectors );
		ComputeSectorSetOrigin( display->pWorld, display->nSectors
									 , display->CurSector.SectorList
									 , display->SectorOrigin );
		AlignPointToGrid( display, display->SectorOrigin );
		display->CurSecOrigin[0] = (int)DISPLAY_X( display, display->SectorOrigin[0] );
		display->CurSecOrigin[1] = (int)DISPLAY_Y( display, display->SectorOrigin[1] );
		DrawSectorOrigin( display );
	}
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Line Origins"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}

	if( display->flags.bSelect  )
	{
		if( ( display->SelectRect.w > REAL_SCALE(display, 5) || 
				display->SelectRect.w < REAL_SCALE(display, -5) )
		  ||( display->SelectRect.h > REAL_SCALE(display, 5) || 
				display->SelectRect.h < REAL_SCALE(display, -5) ) )
		DrawRect( display->pImage, (int)DISPLAY_X(display, display->SelectRect.x)
										, (int)DISPLAY_Y(display, display->SelectRect.y)
										, (int)DISPLAY_SCALE( display, display->SelectRect.w)
										, (int)-DISPLAY_SCALE( display, display->SelectRect.h)
										, AColor( 170, 235, 120, 190 ) 
										, AColor( 240, 240, 240, 170 ) );
	}
	if( display->flags.bMarkingMerge )
	{
		DrawRect( display->pImage, (int)DISPLAY_X(display, display->SelectRect.x)
										, (int)DISPLAY_Y(display, display->SelectRect.y)
										, (int)DISPLAY_SCALE( display, display->SelectRect.w)
										, (int)-DISPLAY_SCALE( display, display->SelectRect.h)
										, 0 
										, AColor( 240, 240, 240, 170 ) );
	}
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Select Rect"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}

	if( display->flags.bGridTop )
		DrawDisplayGrid( display );
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Grid(top)"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}
	}

	{
		TEXTCHAR msg[256];
		uint32_t sectors, walls, lines;
		/*
		uint32_t used, free, chunks, freechunks;
       // gues this doesn't work anymore...
		GetMemStats( &free, &used, &chunks, &freechunks );
		sprintf( msg, WIDE("Used: %d Free: %d UsedChunks: %d FreeChunks: %d")
						, used, free, chunks-freechunks, freechunks );
		PutString( display->pImage, 5, 5, BASE_COLOR_WHITE, Color(0,0,1), msg );
      */
		{
			snprintf( msg, 256, WIDE("Sectors: %d(%d) Walls: %d(%d)")
					 , display->CurSector.SectorList[0]
					 , display->nSectors
					 , display->CurWall.WallList[0]
					 , display->nWalls );
			PutString( display->pImage, 5, 17, BASE_COLOR_WHITE, Color(0,0,1), msg );
		}
		{
			sectors = GetSectorCount( display->pWorld );
			walls = GetWallCount( display->pWorld );
			lines = GetLineCount( display->pWorld );
			snprintf( msg, 256, WIDE("Sector: %d Wall: %d Lines: %d")
						, sectors, walls, lines );
			PutString( display->pImage, 5, 29, BASE_COLOR_WHITE, Color(0,0,1), msg );
		}
	}

	// well this kinda lies - update control (frame) updates the whole
	// frame - otherwise I need to knwo specfic parts.
	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Write to Window"), __FILE__, __LINE__, GetTickCount() - start );
		start = GetTickCount();
	}

	if( dotiming )
	{
		Log3( WIDE("%s(%d): %d Resize World"), __FILE__, __LINE__, GetTickCount() - resizestart );
		start = GetTickCount();
	}
#ifdef CHECK_ALIVE
	is_alive = FALSE;
#endif
}

//----------------------------------------------------------------------------

void DoSave( int bAutoSave, PDISPLAY display, INDEX iWorld )
{
		if( SaveWorldToFile( iWorld ) < 0 )
		{
		}
		if( WriteDisplayInfo( iWorld, display ) < 0 )
			Log( WIDE("Error saving display info...") );
}

#define LOCK_THRESHOLD 5

uint32_t AutoSaveTimer;

static int OnMouseCommon( WIDE("Flatland Editor") )( PCOMMON pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PDISPLAY, editor.TypeID, display, pc );
	//lprintf( WIDE("mouse event %d,%d %d"), x, y, b );
	if( !display )
		return 0;
	AcceptChanges(); // make sure we get all changes applied before drawing.
	if( b & MK_SCROLL_DOWN )
	{
		display->scale *= 1.1;
					SmudgeCommon( pc );
					return 1;
	}
			if( b & MK_SCROLL_UP )
			{
		display->scale /= 1.1;
					SmudgeCommon( pc );
					return 1;
			}
	{
		int delx = x - display->x
		 	, dely = y - display->y;
		_POINT o;
		_POINT del;
		if( !AutoSaveTimer )
			AutoSaveTimer = GetTickCount() + (5*60*1000);
		if( GetTickCount() > AutoSaveTimer )
		{
			DoSave( TRUE, display, display->pWorld );
			AutoSaveTimer = GetTickCount() + (5*60*1000);
		}
		mousemove++;

		display->pImage = GetControlSurface( pc );
		o[0] = REAL_X( display, x );
		o[1] = REAL_Y( display, y );
		o[2] = 0;

		if( delx > 0 )
			display->delxaccum ++;
		else if( delx < 0 )
			display->delxaccum--;
		if( dely > 0 )
			display->delyaccum ++;
		else if( dely < 0 )
			display->delyaccum--;

		{
			TEXTCHAR msg[256], len;
			len = snprintf( msg, 256, WIDE("X: %5g Y: %5g  (%3d,%3d)")
							 , REAL_X(display, x)
							 , REAL_Y(display, y)
							 , delx, dely );

			PutString( display->pImage, 5, 29+12, BASE_COLOR_WHITE, Color(0,0,1), msg );
			UpdateDisplayPortion( display->hVideo, 5, 29, len*8, 12 );
		}
		//Log5( WIDE("Mouse Move: (%d,%d) %d del:(%d,%d)"), x, y, b, delx, dely );
		if( b & MK_OBUTTON )
		{
			if( KeyDown( display->hVideo, KEY_SHIFT ) )
			{
				if( !display->flags.bShiftMode )
				{
					SmudgeCommon( pc );
					display->flags.bShiftMode = TRUE;
				}
			}
			if( !KeyDown( display->hVideo, KEY_SHIFT ) )
			{
				if( display->flags.bShiftMode )
				{
					SmudgeCommon( pc );
					display->flags.bShiftMode = FALSE;
				}
			}

			if( KeyDown( display->hVideo, KEY_V ) )
			{
				//ValidateWorldLinks( display->pWorld );
			}

			if( KeyDown( display->hVideo, KEY_N ) )
			{
				display->flags.bNormalMode = !display->flags.bNormalMode;
				SmudgeCommon( pc );
				//ValidateWorldLinks( display->pWorld );
			}

			if( KeyDown( display->hVideo, KEY_T ) )
			{
				dotiming = !dotiming;
			}

			if( KeyDown( display->hVideo, KEY_D ) )
			{
				DebugDumpMemFile( WIDE("Memory.Dump") );
				//DumpSpaceTree( display->pWorld->spacetree );
			}

			if( KeyDown( display->hVideo, KEY_B ) )
			{
				TEXTCHAR name[32];
				static int n;
				snprintf( name, 32, WIDE("memory.dump.%d"), ++n );
				//TestSpaceBalance( &display->pWorld->spacetree );
				//DumpQuadTree( display->pWorld->quadtree );
				DebugDumpMemFile( name );
			}

			if( KeyDown( display->hVideo, KEY_A ) )
			{
				SetBlotMethod( BLOT_MMX );
				SmudgeCommon( pc );
			}
			if( KeyDown( display->hVideo, KEY_C ) )
			{
				SetBlotMethod( BLOT_C );
				SmudgeCommon( pc );
			}
		}

		if( display->flags.bMarkingMerge )
		{
			display->SelectRect.w = REAL_X(display, x) - display->SelectRect.x;
			display->SelectRect.h = REAL_Y(display, y) - display->SelectRect.y;
			if( b & (MK_LBUTTON|MK_RBUTTON) ) // clicked with line...
			{
				if( !MergeSelectedWalls( display->pWorld
											  , display->CurWall.Wall
											  , &display->SelectRect ) )
					SimpleMessageBox( pc
										 , WIDE("Selection rectangle contains too many walls; merge not done.")
										 , WIDE("Merge Error") );
				display->flags.bMarkingMerge = FALSE;
				SmudgeCommon( pc );
			}
			else
			{
				SmudgeCommon( pc );
				//			RedrawWorld( (uintptr_t) display );
				display->x = x;
				display->y = y;
				display->b = b;
				return 1;
			}
		}

		{
			AlignPointToGrid( display, o );

			if( !display->flags.bDragging &&
				display->flags.bLocked )
			{
				if( delx <= -LOCK_THRESHOLD
					|| delx >= LOCK_THRESHOLD
					|| dely <= -LOCK_THRESHOLD
					|| dely >= LOCK_THRESHOLD )
				{
					display->flags.bLocked = FALSE;
					// break the lock - clear anything that may be active...
					display->flags.bDisplay = FALSE;
					display->flags.bSectorOrigin = FALSE;
					display->flags.bOrigin = FALSE;
					display->flags.bSlope = FALSE;
					display->flags.bEndStart = FALSE;
					display->flags.bEndEnd = FALSE;
				}
				else
				{
					// still active lock... restore mouse position..
					x = display->x;
					y = display->y;
					if( delx || dely )
						SetFrameMousePosition( pc, x, y );
				}
			}
		}

		if( !(b & MK_LBUTTON ) ) // current left not down...
		{
			if( display->flags.bDragging )
			{
				// break the drag - clear anything that may be active...
				display->flags.bLocked = FALSE;
				display->flags.bDragging = FALSE;

				display->flags.bDisplay = FALSE;
				if( display->flags.bSectorOrigin )
					EndUndo( display->pWorld, UNDO_SECTORMOVE, display->SectorOrigin );
				display->flags.bSectorOrigin = FALSE;
				display->flags.bOrigin = FALSE;
				display->flags.bSlope = FALSE;
				display->flags.bEndStart = FALSE;
				display->flags.bEndEnd = FALSE;
			}
		}
		else if( !( display->b & MK_LBUTTON ) ) // last left not down...
		{
			if( display->flags.bLocked )
			{
				display->flags.bDragging = TRUE;

				if( display->flags.bEndStart )
					AddUndo( display->pWorld, UNDO_STARTMOVE, display->CurWall.Wall );
				else if( display->flags.bEndEnd )
					AddUndo( display->pWorld, UNDO_ENDMOVE, display->CurWall.Wall );
				else if( display->flags.bSlope )
					AddUndo( display->pWorld, UNDO_SLOPEMOVE, display->CurWall.Wall );
				else if( display->flags.bSectorOrigin )
					AddUndo( display->pWorld, UNDO_SECTORMOVE, display->nSectors
												, display->CurSector.SectorList
												, display->SectorOrigin );
				else if( display->flags.bOrigin )
				{
					//Log( WIDE("Dragging the Origin!") );
					if( !(display->flags.bWallList) &&
						IsKeyDown( display->hVideo, KEY_SHIFT ) )
					{
						INDEX curwall = display->CurWall.Wall;
						INDEX line = GetWallLine( display->pWorld, curwall );
						PFLATLAND_MYLINESEG pLine;
						GetLineData( display->pWorld, line, &pLine );
						ClearUndo(display->pWorld);
						ClearCurrentWalls( display );
						ClearCurrentSectors( display ); 
						display->NextWall.Wall = AddConnectedSector( display->pWorld, curwall, (25 * display->scale) );
					}
					else
						AddUndo( display->pWorld, UNDO_WALLMOVE
										, display->nWalls
										, display->CurWall.WallList );
				}  
			}
			else
		if( !( display->flags.bOrigin 
				||display->flags.bSlope 
				||display->flags.bEndStart 
				||display->flags.bEndEnd
				||display->flags.bSectorOrigin ) ) 

		{
			if( IsKeyDown( display->hVideo, KEY_CONTROL ) )
			{
				INDEX ps;
				int n;
				ps = FindSectorAroundPoint( display->pWorld, o );
				if( ps )
					Log( WIDE("Found Sector to add to list!") );
				for( n = 0; n < display->nSectors; n++ )
				{
					if( display->CurSector.SectorList[n] == ps )
					{
						if( display->flags.bSectorList )
						{
							lprintf( WIDE("Found sector in list - removeing from list.") );
							if( n < ( display->nSectors-1 ) )
								MemCpy( display->CurSector.SectorList + n
										, display->CurSector.SectorList + n + 1
										, ( display->nSectors - n - 1 ) * sizeof( PSECTOR ) );
							display->nSectors--;
							if( !display->nSectors )
								ClearCurrentSectors( display );
							SmudgeCommon( pc );
							//RedrawWorld( psvUser );
						}
						else
						{  
							display->nSectors = 0;
							goto AddToList;
						}
						n = -1;
						break;
					}
				}
				if( n == display->nSectors )
				{
					INDEX *pList;
				AddToList:
					pList = NewArray( INDEX, display->nSectors + 1 );
					MemCpy( pList
							, display->CurSector.SectorList
							, display->nSectors * sizeof( INDEX ) );
					pList[display->nSectors] = ps;
					display->nSectors++;
					if( display->flags.bSectorList )
						Release( display->CurSector.SectorList );
					display->CurSector.SectorList = pList;
					display->flags.bSectorList = TRUE;
					SmudgeCommon( pc );
					//RedrawWorld( psvUser );
				}
			}
			display->flags.bDragging = TRUE;
			display->flags.bDisplay = TRUE;
		}
	}
	else // prior down, current down (left button)
	{
		//lprintf( WIDE("Checking for nearness - still down...") );
		if( delx || dely /*!Near( display->lastpoint, o )*/ )
		 // (  delx || dely )
		{
			if( display->flags.bDragging )
			{
				if( display->flags.bDisplay )
				{
					//lprintf( WIDE("delx = %d (%d) [%g] dely = %d (%d) [%g]")
					//		 , delx, x - display->x, REAL_SCALE(display,delx )
					//		  , dely, y - display->y, REAL_SCALE( display,dely ) );
					display->origin[0] += REAL_SCALE( display, x - display->x );
					display->origin[1] += REAL_SCALE( display, y - display->y );
					SmudgeCommon( pc );
					//RedrawWorld( psvUser );
				}
				else if( display->flags.bOrigin )
				{
					sub( del, o, display->WallOrigin );
					if( Length( del ) > 0 )
					{
						//lprintf( WIDE("moving origin by %g %g %d"), del[0], del[1],display->CurWall.WallList[0] );
						if( !MoveWalls( display->pWorld, display->nWalls
										  , display->CurWall.WallList
										  , del
										  , IsKeyDown( display->hVideo, KEY_CONTROL ) ) )
						{
							x = display->x;
							y = display->y;
							SetFrameMousePosition( pc, x, y );
						}
					}
				}
				else
				if( display->flags.bSlope )
				{
					INDEX line = GetWallLine( display->pWorld, display->CurWall.Wall );
					PFLATLAND_MYLINESEG pLine;
					LINESEG ls;
					GetLineData( display->pWorld, line, &pLine );
					ls = ((PLINESEG)pLine)[0];
					sub( ls.r.n, o, ls.r.o );
					lprintf( WIDE("Send line update...") );
					SendLineChanged( display->pWorld, display->CurWall.Wall, line, &ls, FALSE, FALSE );
				}
				else if( display->flags.bSectorOrigin )
				{
					sub( del, o, display->SectorOrigin );
					if( Length(del) > 0 )
					if( MoveSectors( display->pWorld, display->nSectors, display->CurSector.SectorList, del ) )
					{
						SetPoint( display->SectorOrigin, o );
						//SmudgeCommon( pc );
						//RedrawWorld( (uintptr_t)display );
					}
					else
					{
						x = display->x;
						y = display->y;
						SetFrameMousePosition( pc, x, y );
					}
				}
				else if( display->flags.bEndStart )
				{
					lprintf( WIDE("at start end...") );
					if( display->flags.bNormalMode )
					{
						PFLATLAND_MYLINESEG pls;
						//MYLINESEG ls;
						//VECTOR v;
						INDEX line = GetWallLine( display->pWorld, display->CurWall.Wall );
						GetLineData( display->pWorld, line, &pls );
#ifdef OUTPUT_TO_VIRTUALITY
						ls = pls->pfl[0];
						addscaled( v, ls.l.r.o, ls.l.r.n,  ls.l.dFrom );
						ls.normals.at_from[0] = o[0] - v[0];
						ls.normals.at_from[1] = o[1] - v[1];
						ls.normals.at_from[2] = 10;//o[2] - v[2];
#endif
						lprintf( WIDE("Should be sending this line...") );
						// don't send this just yet...
						//SendLineNormalsChanged( display->pWorld, line, display->CurWall.Wall, &ls, TRUE );
						// resulting smudge will happen from server event.
						//SmudgeCommon( pc );
					}
					else
					{
						_POINT ptEnd; // save normal for restoration...
						PFLATLAND_MYLINESEG pls;
						LINESEG ls;
						//FLATLAND_MYLINESEG save;
						INDEX line = GetWallLine( display->pWorld, display->CurWall.Wall );
						GetLineData( display->pWorld, line, &pls );
						ls = ((LINESEG*)pls)[0];
						addscaled( ptEnd, ls.r.o, ls.r.n, ls.dTo );
						ls.dFrom = -1;
						ls.dTo = 0;
						SetPoint( ls.r.o, ptEnd );
						sub( ls.r.n, ptEnd, o );

						lprintf( WIDE("Send changed line....") );
						SendLineChanged( display->pWorld, display->CurWall.Wall, line, &ls, FALSE, IsKeyDown( display->hVideo, KEY_CONTROL ) );

						lprintf( WIDE("Should be sending line...") );
						// balance is really a local artifact...
						//BalanceALine( display->pWorld, display->CurWall.Wall, line, &ls, IsKeyDown( display->hVideo, KEY_CONTROL ) );
						//MarkLineChanged( display->pWorld, line );
						//SendLinesChanged( display->pWorld );
					/*
					if( UpdateMatingLines( display->pWorld, display->CurWall.Wall, IsKeyDown( display->hVideo, KEY_CONTROL ), FALSE ) )
					{
						SmudgeCommon( pc );
						//RedrawWorld( (uintptr_t)display );
					}
					else
					{
						*pls = save;
						x = display->x;
						y = display->y;
						SetFrameMousePosition( pc, x, y );
					}
					*/
					}
				}
				else if( display->flags.bEndEnd )
				{
					lprintf( WIDE("at ending end...") );
					if( display->flags.bNormalMode )
					{
						PFLATLAND_MYLINESEG pls;
						//VECTOR v;
						//MYLINESEG ls;
						INDEX line = GetWallLine( display->pWorld, display->CurWall.Wall );
						GetLineData( display->pWorld, line, &pls );
#ifdef OUTPUT_TO_VIRTUALITY
						addscaled( v, pls->pfl->l.r.o, pls->pfl->l.r.n,  pls->pfl->l.dTo );
						pls->pfl->normals.at_to[0] = o[0] - v[0];
						pls->pfl->normals.at_to[1] = o[1] - v[1];//o[0] - v[0];
						pls->pfl->normals.at_to[2] = 10;
#endif
						MarkLineChanged( display->pWorld, line );
						SendLinesChanged( display->pWorld );
						SmudgeCommon( pc );

					}
					else
					{
						_POINT ptStart; // save normal for restoration...
						PFLATLAND_MYLINESEG pls;
						LINESEG ls;
						//FLATLAND_MYLINESEG save;
						INDEX line = GetWallLine( display->pWorld, display->CurWall.Wall );
						GetLineData( display->pWorld, line, &pls );
						ls = ((LINESEG*)pls)[0];
						addscaled( ptStart, ls.r.o, ls.r.n, ls.dFrom );
						ls.dFrom = 0;
						ls.dTo = 1;
						SetPoint( ls.r.o, ptStart );
						sub( ls.r.n, o, ptStart );

						BalanceALine( display->pWorld, display->CurWall.Wall, line, &ls, IsKeyDown( display->hVideo, KEY_CONTROL ) );

						//MarkLineChanged( display->pWorld, line );
						//SendLinesChanged( display->pWorld );
						/*
						 if( UpdateMatingLines( display->pWorld, display->CurWall.Wall, IsKeyDown( display->hVideo, KEY_CONTROL ), FALSE ) )
						 {
						 SmudgeCommon( pc );
						 //RedrawWorld( (uintptr_t)display );
						 }
						 else
						 {
						 *pls = save;
						 x = display->x;
						 y = display->y;
						 SetFrameMousePosition( pc, x, y );
						 }
						 */
					}
				}
			}
		}
		else
		{
			lprintf( WIDE("Near last point...") );
		}
	}

	if( !(b & MK_RBUTTON ) ) // current right not pressed
	{
		if( display->b & MK_RBUTTON ) // last right was pressed...
		{
			int i;
			int flippedx=FALSE, flippedy = FALSE;
			display->flags.bSelect = FALSE;
			display->b = b; 
			
			if( display->SelectRect.w < 0 )
			{
				flippedx = TRUE;
				display->SelectRect.x += display->SelectRect.w;
				display->SelectRect.w = -display->SelectRect.w;
			}
			if( display->SelectRect.h < 0 )
			{
				flippedy = TRUE;
				display->SelectRect.y += display->SelectRect.h;
				display->SelectRect.h = -display->SelectRect.h;
			}
			Log( WIDE("Starting popup track...") );
			if( display->SelectRect.w < 5 &&
				 display->SelectRect.h < 5 )
			{
				if( display->flags.bOrigin || 
					 display->flags.bSlope ||
					 display->flags.bEndStart ||
					 display->flags.bEndEnd )
					i = TrackPopup( hOriginMenu, pc );
				else
				{
					_POINT o;
					o[0] = REAL_X(display, x);
					o[1] = REAL_Y(display, y);
					o[2] = 0;
					//Log4( WIDE("Checking (%g,%g) within %d sectors (%08x)"), o[0], o[1], display->nSectors, display->CurSector.SectorList[0] );
					if( FlatlandPointWithin( display->pWorld, display->nSectors, display->CurSector.SectorList, o ) != INVALID_INDEX )
					{
						i = TrackPopup( hSectorMenu, pc );
					}
					else
						i = TrackPopup( hMainMenu, pc );
				}
			}
			else // had a selection rectangle, and we should launch select menu
				i = TrackPopup( hSelectMenu, pc );
			Log1( WIDE("Popup returned: %d"), i );
			if( i >= MNU_MINZOOM && i <= MNU_MAXZOOM )
			{
				_POINT neworigin;
				neworigin[0] = -REAL_X( display, x );
				neworigin[1] = REAL_Y( display, y );
				neworigin[2] = 0;
				SetPoint( display->origin, neworigin );
				display->zbias = 1 << (i - MNU_MINZOOM);
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
			}
			else switch( i )
			{
			case MNU_UNDO:
				DoUndo(display->pWorld );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
				break;
			case MNU_BREAK:
				if( !display->flags.bWallList )
				{
					BreakWall( display->pWorld, display->CurWall.Wall );
					SmudgeCommon( pc );
					//RedrawWorld( (uintptr_t)display );
				}
				break;
			case MNU_SETTOP:
				if( !display->flags.bWallList )
				{
					//if( display->CurWall.Wall )
					// display->CurSector.Sector->wall = display->CurWall.Wall;
				}
				break;
			case MNU_DISPLAYPROP:
				display->x = x;
				display->y = y;
				display->b = b;
				Log( WIDE("Showing display properties") );
				{
					int32_t posx, posy;
					GetDisplayPosition( display->hVideo, &posx, &posy, NULL, NULL );
					DisplayProperties( pc
										  , x + posx
										  , y + posy );
				}
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
				break;
			case MNU_SECTPROP:
				// hmm maybe mass change... 
				display->x = x;
				display->y = y;
				display->b = b;
				{
					int32_t posx, posy;
					GetDisplayPosition( display->hVideo, &posx, &posy, NULL, NULL );
					ShowSectorProperties( pc, display->nSectors
												, display->CurSector.SectorList
												, x + posx
												, y + posy );
				}
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
				break;
			case MNU_DELETELINE:
				ClearUndo(display->pWorld);
				Log( WIDE("Removing a wall...") );
				if( !display->flags.bWallList )
					RemoveWall( display->pWorld, display->CurWall.Wall );
				Log( WIDE("Removed a wall...") );
				ClearCurrentWalls( display );
				Log( WIDE("Clear current walls...") );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
				break;
			case MNU_SECTSELECT:
				// if sectors got selected, then 
				// mark bSectorList ...
				ClearCurrentSectors( display );
				ClearCurrentWalls( display );
				if( MarkSelectedSectors( display->pWorld, &display->SelectRect, &display->CurSector.SectorList, &display->nSectors ) )
				{
					display->flags.bSectorList = TRUE;
				}
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t) display );
				break;
			case MNU_WALLSELECT:
				ClearCurrentSectors( display );
				ClearCurrentWalls( display );
				if( MarkSelectedWalls( display->pWorld, &display->SelectRect, &display->CurWall.WallList, &display->nWalls ) )
				{
					display->flags.bWallList = TRUE;
				}
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t) display );
				break;
			case MNU_UNSELECTSECTORS:
				ClearCurrentSectors( display );
				ClearCurrentWalls( display );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t) display );
				break;
			case MNU_MERGEWALLS:
				ClearUndo(display->pWorld);
				// this operation is direction sensitive...
				// the diagonal must remain in the same direction....
				if( flippedx )
				{
					display->SelectRect.x += display->SelectRect.w;
					display->SelectRect.w = -display->SelectRect.w;
				}
				if( flippedy )
				{
					display->SelectRect.y += display->SelectRect.h;
					display->SelectRect.h = -display->SelectRect.h;
				}
				if( !MergeSelectedWalls( display->pWorld, INVALID_INDEX, &display->SelectRect ) )
					SimpleMessageBox( pc
									, WIDE("Selection rectangle contains too many walls; merge not done.")
									, WIDE("Merge Error") );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t) display );
				break;
			case MNU_MERGEOVERLAP:
				MergeOverlappingWalls( display->pWorld, &display->SelectRect );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
				break;
			case MNU_RESET:
				ClearCurrentSectors( display );
				ClearCurrentWalls( display );
				ResetWorld( display->pWorld );
				CreateSquareSector( display->pWorld, VectorConst_0, 50 );
				ResetDisplay( display );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t) display );
				break;			
			case MNU_DELETE:
				ClearUndo(display->pWorld);
				{
					int n;
					for( n = 0; n < display->nSectors; n++ )
					{
						DestroySector( display->pWorld, display->CurSector.SectorList[n] );
					}
					ClearCurrentSectors(display);
				}
				ClearCurrentWalls( display );
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t) display );
				break;
			case MNU_SPLIT:	// can only be selected if wall menu 
				ClearUndo(display->pWorld);
				if( !display->flags.bWallList )
					if( GetMatedWall( display->pWorld, display->CurWall.Wall ) )
						SplitWall( display->pWorld, display->CurWall.Wall );
				break;
			case MNU_MERGE:  // can only be selected if wall menu 
				ClearUndo(display->pWorld);
				if( !display->flags.bWallList )
					if( display->MarkedWall )
					{
						if( MergeWalls( display->pWorld, display->CurWall.Wall, display->MarkedWall ) )
							ClearCurrentWalls( display );
					}
					else
					{
						// definatly has a selected wall... now...
						display->SelectRect.x = REAL_X( display, x );
						display->SelectRect.y = REAL_Y( display, y );
						display->SelectRect.w = 0;
						display->SelectRect.h = 0;
						display->flags.bMarkingMerge = TRUE;
						// shouldn't matter what the state of locked is if mergemark
						//display->flags.bLocked = FALSE;
					}
				display->MarkedWall = INVALID_INDEX;
				SmudgeCommon( pc );
				//RedrawWorld( (uintptr_t)display );
				break;
			case MNU_MARK:
				if( !display->flags.bWallList )
					display->MarkedWall = display->CurWall.Wall;
				// if current wall is marked deserves an update then....
				//RedrawWorld( display );
				break;
			case MNU_NEWSECTOR:
				{
					_POINT p;
					p[0] = REAL_X( display, x );
					p[1] = REAL_Y( display, y );
					p[2] = 0;
					CreateSquareSector( display->pWorld, p, 50 );
					SmudgeCommon( pc );
					//RedrawWorld( (uintptr_t)display );
				}
				break;
			case MNU_SAVE:
				{
#ifdef CHECK_ALIVE
					is_alive = TRUE;
#endif
#ifdef _WIN32
					DoSave( FALSE, display, display->pWorld );
#endif
#ifdef CHECK_ALIVE
					is_alive=FALSE;
#endif
				}
				AutoSaveTimer = GetTickCount() + (5*60*1000);
				break;
			case MNU_QUIT:
				{
					//PCOMMON pc = g.pDisplay->hVideo;
					// resetting the address within display could be hazardous.
					if( pc == g.pc )
						g.pc = NULL; 
					else
						DebugBreak();
					DestroyFrame( &pc ); /* probalby is g.pc also... */
				}
				break;
			case MNU_LOAD:
				{
					//char file[256];
#ifdef CHECK_ALIVE
					is_alive = TRUE;
#endif
#ifdef _WIN32
               // get world list, select oen.
						{
							if( LoadWorldFromFile( display->pWorld ) < 0 )
							{
								Log( WIDE("Error loading the world...") );
							}
							ReadDisplayInfo( display->pWorld, display );
							ClearCurrentSectors( display );
							ClearCurrentWalls( display );
							SmudgeCommon( pc );
							//RedrawWorld( (uintptr_t) display );
						}
#endif
					Log( WIDE("Done with file load?") );
#ifdef CHECK_ALIVE
					global_alive = TRUE;
					is_alive = FALSE;
#endif
				}
				AutoSaveTimer = GetTickCount() + (5*60*1000);
				break;
			}
		}
	}
	else if( !(display->b & MK_RBUTTON ) ) // last right not pressed
	{
		display->SelectRect.x = REAL_X( display, x );
		display->SelectRect.y = REAL_Y( display, y );
		display->SelectRect.w = 0;
		display->SelectRect.h = 0;
		display->flags.bSelect = TRUE;
	}
	else // current and last are both pressed...
	{
		display->SelectRect.w = REAL_X(display, x) - display->SelectRect.x;
		display->SelectRect.h = REAL_Y(display, y) - display->SelectRect.y;
		SmudgeCommon( pc );
		//RedrawWorld( (uintptr_t)display);
	}

	// if nothing pressed - check motion for line crossings....
	if( !(b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
	{
		
		if( !(  display->flags.bMarkingMerge 
			  || display->flags.bSelect
			  || display->flags.bLocked ) )
		{
#define LockTo(what,extratest, boolvar)	\
								if( (x > (display->what[0] - 4 )) && \
									 (x < (display->what[0] + 4 )) &&		 \
									 (y > (display->what[1] - 4 )) &&		 \
									 (y < (display->what[1] + 4 )) &&		 \
									 extratest )									 \
								{														\
									display->flags.boolvar = TRUE;			 \
									display->flags.bLocked = TRUE;			 \
									x = display->what[0];						 \
									y = display->what[1];						 \
									SetFrameMousePosition( pc, x, y );\
								}
			LockTo( CurSecOrigin, TRUE, bSectorOrigin )
			else
			LockTo( CurOrigin, TRUE, bOrigin )
			else
			LockTo( CurSlope, IsKeyDown( display->hVideo, KEY_SHIFT ), bSlope )
			else
			LockTo( CurEnds[0], !IsKeyDown( display->hVideo, KEY_SHIFT ),bEndStart )
			else
			LockTo( CurEnds[1], !IsKeyDown( display->hVideo, KEY_SHIFT ),bEndEnd )
			else
			if( !display->flags.bNormalMode )
			{
				INDEX pNewWall;
				_POINT del, o;
				INDEX ps = INVALID_INDEX;

				if( !display->flags.bSectorList &&
					 !display->flags.bWallList )
				{
					int draw = FALSE;

					if( display->CurSector.Sector != INVALID_INDEX )
					{
						o[0] = REAL_X( display, display->x );
						o[1] = REAL_Y( display, display->y );
						o[2] = 0;
						del[0] = REAL_SCALE( display, delx );
						del[1] = REAL_SCALE( display, dely );
						del[2] = 0;
						scale( del, del, 2 );

						del[1] = -del[1];
						DrawLine( display, del, o, 0, 1, Color( 90, 90, 90 ) );

						pNewWall = FindIntersectingWall( display->pWorld, display->CurSector.Sector, del, o );
						if( ( pNewWall != INVALID_INDEX ) && ( pNewWall != display->CurWall.Wall ) )
						{
							// this bit of code... may or may not be needed...
							// at this point a sector needs to be found
							// before a wall can be found...
							//display->nWalls = 1;
							//display->CurWall.WallList = &display->CurWall.Wall;
							display->CurWall.Wall = pNewWall;
							//BalanceALine( display->pWorld, GetWallLine( display->pWorld, pNewWall ) );
							draw = TRUE;
						}			  
					}
					//else
					//	lprintf( WIDE("no current sector...") );

					o[0] = REAL_X( display, x );
					o[1] = REAL_Y( display, y );
					o[2] = 0;
					//lprintf( WIDE("Checking point (%g,%g)"), o[0], o[1] );
					{ 
						uint32_t start = GetTickCount();
						//lprintf( WIDE("point %s within %p"), PointWithinSingle( display->pWorld, display->CurSector.Sector, o )?WIDE("is"):WIDE("is not")
						//		 , display->CurSector.Sector );
						if( ( (ps = display->CurSector.Sector) == INVALID_INDEX ) ||
							 ( FlatlandPointWithinSingle( display->pWorld, display->CurSector.Sector, o ) == INVALID_INDEX ) )
						{
							//lprintf( WIDE("Find sector around this point!") );
							ps = FindSectorAroundPoint( display->pWorld, o );
							if( ps != INVALID_INDEX )
							{
								//display->CurSector.Sector = ps;
								//lprintf( WIDE("Yes found one...") );
							}
							if( dotiming )
							{
								Log1( WIDE("Finding Sector: %d"), GetTickCount() - start );
							}
						}
					}

					if( ps != INVALID_INDEX && ( ps != display->CurSector.Sector ) )
					{
						//lprintf( WIDE("marking new current sector? ") );
						display->nSectors = 1;
						display->CurSector.SectorList = &display->CurSector.Sector;
						display->CurSector.Sector = ps;

						display->nWalls = 1;

						if( display->CurWall.Wall != INVALID_INDEX &&
							 WallInSector( display->pWorld, display->CurSector.Sector
											 , GetMatedWall( display->pWorld, display->CurWall.Wall ) ) )
							display->CurWall.Wall = GetMatedWall( display->pWorld, display->CurWall.Wall );
						else						  
							display->CurWall.Wall = GetFirstWall( display->pWorld, ps, NULL );

						display->CurWall.WallList = &display->CurWall.Wall;
						/*
						BalanceALine( display->pWorld, display->CurWall.Wall
							, GetWallLine( display->pWorld, display->CurWall.Wall ) 
							, 
							);
							*/
						draw = TRUE;
					}
					if( draw )
						SmudgeCommon( pc );
				} 
			}
		}	
		// see if we cross a wall, or into a new wall....
	}
	SetPoint( display->lastpoint, o );
	display->x = x;
	display->y = y;
	display->b = b;
	//SetCursor( );
	}
	return 1;
}


void CPROC UpdateVisual( uintptr_t psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	ValidatedControlData( PDISPLAY, editor.TypeID, display, pc );
	if( display->NextWall.Wall != INVALID_INDEX )
	{
		if( GetWallSector( display->pWorld, display->NextWall.Wall ) != INVALID_INDEX )
		{
			SetCurWall( pc );
		}
	}
	lprintf( "Smudge Display %p", psv );
	SmudgeCommon( pc );
}

//----------------------------------------------------------------------------

static int OnCreateCommon(WIDE("Flatland Editor"))( PCOMMON pc )
{
	ValidatedControlData( PDISPLAY, editor.TypeID, pd, pc );
	if( pd )
	{
		//display = pd;
		pd->pWorld = OpenWorld( WIDE("Default Flatland World") );
		//ResetWorld( pd->pWorld );
		ResetDisplay( pd );
		AddUpdateCallback( UpdateVisual, (uintptr_t)pc );
		lprintf( "Registered %p %p", UpdateVisual, (uintptr_t)pc );
		ClearCurrentSectors( pd );
		ClearCurrentWalls( pd );
		// might do some things like do the init menus here?
		return 1;
	}
	return 0;
}

static int OnKeyCommon( WIDE("Flatland Editor") )( PCOMMON pc, uint32_t key )
{
	ValidatedControlData( PDISPLAY, editor.TypeID, display, pc );
	// hmm probably have to move the keyboard handling routines from
	// mouse into here...

			if( KeyDown( display->hVideo, KEY_SHIFT ) )
			{
				if( !display->flags.bShiftMode )
				{
					SmudgeCommon( pc );
					display->flags.bShiftMode = TRUE;
				}
			}
			if( !KeyDown( display->hVideo, KEY_SHIFT ) )
			{
				if( display->flags.bShiftMode )
				{
					SmudgeCommon( pc );
					display->flags.bShiftMode = FALSE;
				}
			}

			if( KeyDown( display->hVideo, KEY_V ) )
			{
				//ValidateWorldLinks( display->pWorld );
			}

			if( KeyDown( display->hVideo, KEY_N ) )
			{
				display->flags.bNormalMode = !display->flags.bNormalMode;
				//SmudgeCommon( pc );
				//ValidateWorldLinks( display->pWorld );
			}

			if( KeyDown( display->hVideo, KEY_T ) )
			{
				dotiming = !dotiming;
			}

			if( KeyDown( display->hVideo, KEY_D ) )
			{
				DebugDumpMemFile( WIDE("Memory.Dump") );
				//DumpSpaceTree( display->pWorld->spacetree );
			}

			if( KeyDown( display->hVideo, KEY_B ) )
			{
				TEXTCHAR name[32];
				static int n;
				snprintf( name, 32, WIDE("memory.dump.%d"), ++n );
				//TestSpaceBalance( &display->pWorld->spacetree );
				//DumpQuadTree( display->pWorld->quadtree );
				DebugDumpMemFile( name );
			}

			if( KeyDown( display->hVideo, KEY_A ) )
			{
				SetBlotMethod( BLOT_MMX );
				//SmudgeCommon( pc );
			}
			if( KeyDown( display->hVideo, KEY_C ) )
			{
				SetBlotMethod( BLOT_C );
				//SmudgeCommon( pc );
			}
	SmudgeCommon( pc );
	return 0;
}

//----------------------------------------------------------------------------

CONTROL_REGISTRATION editor = { WIDE("Flatland Editor")
										, { { 640, 480 }
										, sizeof( DISPLAY )
										, BORDER_NORMAL|BORDER_RESIZABLE }
};
PRELOAD( RegisterEditor ) { DoRegisterControl( &editor ); }

PUBLIC( void, FlatlandMain )( void )
{
	AlignBaseToWindows();
	hZoomMenu = CreatePopup();
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+10, WIDE("x128") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+9, WIDE("x64") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+8, WIDE("x32") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+7, WIDE("x16") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+6, WIDE("x8") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+5, WIDE("x4") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+4, WIDE("x2") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+3, WIDE("x1") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+2, WIDE("x1/2") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+1, WIDE("x1/4") );
	AppendPopupItem( hZoomMenu, MF_STRING, MNU_MINZOOM+0, WIDE("x1/8") );

	hMainMenu = CreatePopup();
	AppendPopupItem( hMainMenu, MF_STRING, MNU_RESET, WIDE("Reset") );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_NEWSECTOR, WIDE("New Sector") );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_UNSELECTSECTORS, WIDE("Unslect All") );
	AppendPopupItem( hMainMenu, MF_POPUP|MF_STRING, (int)hZoomMenu, WIDE("Zoom") );
	AppendPopupItem( hMainMenu, MF_SEPARATOR, 0, NULL );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_DISPLAYPROP, WIDE("Display Properties") );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_UNDO, WIDE("Undo") );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_SAVE, WIDE("Save") );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_LOAD, WIDE("Load") );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_QUIT, WIDE("Quit") );

	hOriginMenu = CreatePopup();
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_MARK, WIDE("Mark Wall") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_MERGE, WIDE("Merge Walls") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_SPLIT, WIDE("Split Wall") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_BREAK, WIDE("Break Wall") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_DELETELINE, WIDE("Delete Wall") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_UNSELECTSECTORS, WIDE("Unselect All") );
	AppendPopupItem( hOriginMenu, MF_POPUP|MF_STRING, (int)hZoomMenu, WIDE("Zoom") );
	AppendPopupItem( hOriginMenu, MF_SEPARATOR, 0, NULL );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_DISPLAYPROP, WIDE("Display Properties") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_UNDO, WIDE("Undo") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_SAVE, WIDE("Save") );
	AppendPopupItem( hOriginMenu, MF_STRING, MNU_LOAD, WIDE("Load") );

	hSectorMenu = CreatePopup();

	AppendPopupItem( hSectorMenu, MF_STRING, MNU_SECTPROP, WIDE("Properties") );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_DELETE, WIDE("Delete Sector") );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_SETTOP, WIDE("Set Wall Top") );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_UNSELECTSECTORS, WIDE("Unslect All") );
	AppendPopupItem( hSectorMenu, MF_POPUP|MF_STRING, (int)hZoomMenu, WIDE("Zoom") );
	AppendPopupItem( hSectorMenu, MF_SEPARATOR, 0, NULL );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_DISPLAYPROP, WIDE("Display Properties") );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_UNDO, WIDE("Undo") );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_SAVE, WIDE("Save") );
	AppendPopupItem( hSectorMenu, MF_STRING, MNU_LOAD, WIDE("Load") );

	hSelectMenu = CreatePopup();

	AppendPopupItem( hSelectMenu, MF_STRING, MNU_MERGEWALLS, WIDE("Merge Walls") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_MERGEOVERLAP, WIDE("Merge Overlaps") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_SECTSELECT, WIDE("Select Sectors") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_WALLSELECT, WIDE("Select Walls") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_UNSELECTSECTORS, WIDE("Unslect All") );
	AppendPopupItem( hSelectMenu, MF_POPUP|MF_STRING, (int)hZoomMenu, WIDE("Zoom") );
	AppendPopupItem( hSelectMenu, MF_SEPARATOR, 0, NULL );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_DISPLAYPROP, WIDE("Display Properties") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_UNDO, WIDE("Undo") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_SAVE, WIDE("Save") );
	AppendPopupItem( hSelectMenu, MF_STRING, MNU_LOAD, WIDE("Load") );

	{
		PDISPLAY pDisplay;
		g.pc = MakeCaptionedControl( NULL, editor.TypeID, 0, 0, 0, 0, 0, WIDE("Flatland Editor") );
		pDisplay = ControlData( PDISPLAY, g.pc );
		SetCommonUserData( g.pc, (uintptr_t)pDisplay );	
		DisplayFrame( g.pc );

		// I use this to test for keys
		// SetFrameMousePosition - a quick MousePosition status flush to display
		// and for testing to see if the renderer is still being displayed
		// should attach a close callback I would assume...
		pDisplay->hVideo = GetFrameRenderer( g.pc );
	}
#ifdef CHECK_ALIVE
	{
		ThreadTo( CheckAliveThread, 0 );
	}
#endif
}

//----------------------------------------------------------------------------

SaneWinMain( argc, argv )
{
//	SetAllocateLogging( TRUE );
	//LoadFunction( WIDE("msgsvr.core.dll"), NULL );
	/* register the server within us for debugging... */
	SuspendDeadstart();
	g.pri = GetDisplayInterface();
	g.pii = GetImageInterface();

	{
		PSERVICE_ROUTE tmp;
		if( ( tmp = LoadService( WORLD_SCAPE_INTERFACE_NAME, NULL ) ) == NULL )
		{
			LoadFunction( WIDE("world_scape_msg_server.dll"), NULL );
		}
	}
	/* register the client interface manually, load interface will not work otherwise (interface.conf?) */
	LoadFunction( WIDE("world_scape_client.dll"), NULL );
	ResumeDeadstart();

	{
		int n = 10;
		while( n )
		{
#ifdef USE_WORLDSCAPE_INTERFACE
			g.wsi = (PWORLD_SCAPE_INTERFACE)GetInterface( WORLD_SCAPE_INTERFACE_NAME );
#endif
		   if( g.wsi ) 
			   break;
		   n--;
		   WakeableSleep( 100 );
		}
	}
		if( !g.wsi )
	{
		lprintf( "failed to find world scape interface. No work can be done." );
		return 0;
	}
	//CreateView(NULL, WIDE("FORWARD"));
	FlatlandMain();
	while( 1 )
		WakeableSleep( SLEEP_FOREVER );
	return 0;	
}
EndSaneWinMain()