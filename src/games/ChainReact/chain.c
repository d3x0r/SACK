//#define BUILD_FLOWTEST

// the motion needs to wrap the atoms outside and back on-board
#define CHAIN_REACT_MAIN
#include <stdhdrs.h>
#include <logging.h>

#include "global.h"

#include <idle.h>
#include <render.h>
#include <math.h>
#include <timers.h>
#include <keybrd.h>
//#include "player.h"
#include "logic.h"
#ifdef _WIN32
    HCURSOR hCursor;
#endif
PRENDERER hDisplay;
Image pGameBoard;
Image pAtom, pGrid, pCursor;

#include "chain.h"

int zoom = 1;
int _boardx =
#ifdef BUILD_FLOWTEST
	MAX_BOARD_X-1
#else
	5
#endif
	;
int _boardy =
#ifdef BUILD_FLOWTEST
	MAX_BOARD_Y-1
#else
	4
#endif
	;
#define BOARD_X _boardx
#define BOARD_Y _boardy

int PeiceSize, PeiceBase = 26;
int PeiceDist;
int BoardSizeX, BoardSizeY;
int CursorX, CursorY;
int winstate;
int NewGame;
int gnTimer, gnAnimateTimer;

PTHREAD pUpdateThread;

typedef struct {
	int x, y;
} XYPOINT;

typedef struct {
	float x, y, _x, _y;
	//float __x, __y;
	int wrap_x, wrap_y; // on shere board, need to consider that the target is from a wrap...
} ATOMPOINT;

CELL Board[MAX_BOARD_X][MAX_BOARD_Y];
ATOMPOINT BoardAtoms[MAX_BOARD_X][MAX_BOARD_Y][8]; // use Board[x][y].count 

#define QUEUESIZE MAX_BOARD_X * MAX_BOARD_Y

XYPOINT Explode[QUEUESIZE];
int ExplodeHead, ExplodeTail;
int updating_board_end; // communicate to board update thread to stop updating.
int updating_board;  // state of board update thread, if it is in an update or not.

PLAYER players[MAX_PLAYERS + 1]; // players[0] is never used...
int playerturn; // 1...n
int maxplayers; // max players...

int first_draw = 1;
int animate = 0;
int sphere = 0;
int unstable;
int drawing;
int all_players_went;


void CPROC MyRedraw( uintptr_t dwUser, PRENDERER pRenderer );
int CPROC Mouse( uintptr_t dwUser, int32_t x, int32_t y, uint32_t b );


void CPROC Close( uintptr_t dwUser )
{
	UnmakeImageFile( pGameBoard );
	pGameBoard = NULL;
	NewGame = TRUE;
}

void ClearBoard( void )
{
	int x, y;
	if( gnTimer )
	{
		RemoveTimer( gnTimer );
		gnTimer = 0;
	}
	if( gnAnimateTimer )
	{
		RemoveTimer( gnAnimateTimer );
		gnAnimateTimer = 0;
	}
	while( drawing )
		Idle();
	updating_board_end = 1;
	while( updating_board )
		Idle();
	if( hDisplay )
	{
		NewGame = FALSE;
		// ends up setting new game...
		CloseDisplay( hDisplay );
		while( !NewGame )
			Idle();
		hDisplay = NULL;
	}
	NewGame = FALSE;

	// configure board size here....
	for( x = 0; x < MAX_BOARD_X; x++ )
	{
		for( y = 0; y < MAX_BOARD_Y; y++ )
		{
			Board[x][y].stable = TRUE;
			Board[x][y].count = 0;
			Board[x][y].player = 0;
		}
	}
	CursorX = 0;
	CursorY = 0;
	ExplodeHead = ExplodeTail = 0; // clear pending explosions...
	updating_board_end = 0; // allow board to update
	winstate = FALSE;  // noone has one
	first_draw = TRUE;
	all_players_went = FALSE; // we haven't all gone yet...
	playerturn = 1;   // of course we start with player 1.

	maxplayers = ConfigurePlayers();

	{
		int p;
		for( p = 1; p <= maxplayers; p++ )
		{
			players[p].active = TRUE;
			players[p].count = 0;
			players[p].went = FALSE;
			//players[p].color = p;
		}
		for( ; p <= MAX_PLAYERS; p++ )
		{
			players[p].active = FALSE;
		}
	}
	ConfigureBoard( &animate, &sphere );

	BoardSizeX = pGrid->width / zoom;
	BoardSizeY = pGrid->height / zoom;
	PeiceSize = PeiceBase / zoom;
	PeiceDist = ((PeiceSize*11)/16 );
#ifdef _WIN32
	SetApplicationTitle( "Application Title doesn't work!" );
#endif

	hDisplay = OpenDisplaySizedAt( 0, 200 + BOARD_X * BoardSizeX, 30 + BOARD_Y * BoardSizeY, 0, 0 );
	SetCloseHandler( hDisplay, Close, (uintptr_t)hDisplay );
	SetRedrawHandler( hDisplay, MyRedraw, (uintptr_t)hDisplay );
	SetMouseHandler( hDisplay, Mouse, (uintptr_t) hDisplay );
	//SizeVideo( hDisplay, 200 + BOARD_X * BoardSizeX, 30 + BOARD_Y * BoardSizeY );
	//MoveVideo( hDisplay, 0, 0 );
}


void DrawSquare( int x, int y, int bWrite )
{
	int dx, dy;
	int wx, wy, ww, wh; // write rectangle defs...
	// consider shading this to the player color...
	dx = x * BoardSizeX;
	dy = y * BoardSizeY;
   //lprintf( "Rendering blank square %d,%d", x, y );
	if( x == CursorX && y == CursorY )
	{
		BlotScaledImageSizedEx( pGameBoard, pCursor
   									, dx, dy
                              , BoardSizeX, BoardSizeX
   									, 0, 0
   									, BoardSizeX, BoardSizeY
   									, FALSE
   									, BLOT_SHADED
   									, Colors[players[Board[x][y].player].color] );
	}
	else
	{
   	BlotScaledImageSizedEx( pGameBoard, pGrid
										, dx, dy
                              , BoardSizeX, BoardSizeX
										, 0, 0
   									, BoardSizeX, BoardSizeY
   									, FALSE
   									, BLOT_SHADED
   									, Colors[players[Board[x][y].player].color] );
	}
	if( !bWrite && animate )
		return;
	wx = dx;
	wy = dy;
	ww = BoardSizeX+1;
	wh = BoardSizeY+1;
	switch( Board[x][y].count )
	{
	case 0:
		break;
	case 1:
		#define CENTX (BoardSizeX/2)
		#define CENTY (BoardSizeY/2)
		#define HALFX (PeiceSize/2)
		#define THIRDX (PeiceSize*2/5)
		#define HALFY (PeiceSize/2)
		#define THIRDY (PeiceSize*2/5)
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX-HALFX), dy + (CENTX-HALFY)
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
		break;
	case 2:
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - (HALFX*3/2)), dy + (CENTY-(HALFY*3/2))
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - THIRDX ), dy + (CENTY - (THIRDY-2) )
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
		break;
	case 3:
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - (5*THIRDX/2) ), dy + (CENTY - (2*THIRDY))
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - (THIRDX/2)), dy + (CENTY - (2*THIRDY))
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );
      BlotScaledImageSizedEx( pGameBoard, pAtom
										, dx + (CENTX - HALFX), dy + (CENTY - (THIRDY - 2) )
										, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );
		
		break;
	default:
	case 4:
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - 2*HALFY + 3), dy + (CENTY - HALFY)
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - 3), dy + (CENTY - HALFY)
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - HALFX), dy + (CENTY - 2*HALFY + 3 )
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( pGameBoard, pAtom
      								, dx + (CENTX - HALFX), dy + (CENTY - 3 )
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
		break;

	}
	//if( bWrite )
	//	UpdateDisplayPortion( hDisplay
	//	                     , wx, wy
	//								, ww, wh );
}

void EnqueCell( int x, int y )
{
	int n;
	// see if we enqueued this beast already!
	//printf( "Head: %d Tail: %d\n", ExplodeHead, ExplodeTail );
	for( n = ExplodeTail; n != ExplodeHead; n++ )
	{
		if( n == (QUEUESIZE-1) )
			n = 0;
		if( n == ExplodeHead )
			break;
		if( Explode[n].x == x && 
			 Explode[n].y == y )
		{
			//printf( "Already Enqueued?\n" );
		   return;
		}
	}
	//printf( "Head: %d\n", ExplodeHead );
	Explode[ExplodeHead].x = x;
	Explode[ExplodeHead].y = y;
	ExplodeHead++;
	if( ExplodeHead == (QUEUESIZE-1) )
		ExplodeHead = 0;
}

int DequeCell( int *x, int *y )
{
	if( ExplodeTail == ExplodeHead )
		return FALSE;
	//printf( "Tail: %d\n", ExplodeTail );
	*x = Explode[ExplodeTail].x;
	*y = Explode[ExplodeTail].y;
	ExplodeTail++;
	if( ExplodeTail == (QUEUESIZE-1) )
		ExplodeTail = 0;
	return TRUE;
}

int CheckSquareEx( int x, int y, int explode )
#define CheckSquare( x, y ) CheckSquareEx( x, y, TRUE )
{
	int c;
	//if( !Board[x][y].stable )
	//	return FALSE;
	if( sphere )
	{
		c = 4;
	}
	else
	{
		c = 2;
		if( x>0 && x < (BOARD_X-1) )
			c++;
		if( y>0 && y < (BOARD_Y-1) )
			c++;
	}
	if( Board[x][y].count >= c )
	{
		//players[Board[x][y].color].count -= c;
		//Board[x][y].count -= c;
		if( explode )
		{
			if( animate )
				unstable = TRUE;
			EnqueCell( x, y );
		}
		return  TRUE;
	}
	return  FALSE;
}

void ShowPlayer( int bWrite )
{
	int i;
	BlatColor( pGameBoard, BoardSizeX * BOARD_X, 5, 130, 26 + 26*maxplayers, 0 );
	for( i = 1; i <= maxplayers; i++ )
	{
		TEXTCHAR buf[32];
		if( players[i].active )
		{
		   BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
   										, BoardSizeX * BOARD_X + 5, 5 + ( 26 * i)
   										, 26, 26
                                       , 0, 0
	      								, pAtom->width, pAtom->height
	      								, TRUE
   										, BLOT_MULTISHADE
   										, 0, Color( 255,255,255)
   										, Colors[players[i].color] );
			snprintf( buf, 32, " : %d (%d)      ", players[i].wins, players[i].count );
			PutString( GetDisplayImage( hDisplay )
				, BoardSizeX * BOARD_X + 35, 11 + (26 * i), 0
				, Color( 255,255,255 ), Color(0,0,1)
				, buf );
		}
	}	
	if( !animate || !unstable )
	{
	   BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
   									, BoardSizeX * BOARD_X + 5, 5
   									, 26, 26
                              , 0, 0
	     								, pAtom->width, pAtom->height
      								, TRUE
   									, BLOT_MULTISHADE
   									, 0, Color( 255,255,255)
   									, Colors[players[playerturn].color] );
		PutString( GetDisplayImage( hDisplay )
					, BoardSizeX * BOARD_X + 35, 11, 0
					, Color( 255,255,255 ), Color(0,0,1)
					, "To move next" );
	}
	//if( bWrite )
	//	UpdateDisplayPortion( hDisplay, BoardSizeX * BOARD_X, 5, 130, 26 + 26*i );
}

void DoLose( void )
{
	if( all_players_went )
	{
		int n;
		for( n = 1; n <= maxplayers; n++ )
		{
			if( players[n].active &&
			    (!players[n].count) )
			{
				players[n].active = FALSE;
			}
		}
	}
}

void ExplodeInto( int x, int y, int into_x, int into_y )
{
	int real_into_x = into_x;
	int real_into_y = into_y;
	if( sphere )
	{
		if( into_x < 0 )
		{
			BoardAtoms[x][y][Board[x][y].count-1].wrap_x--;
			into_x += BOARD_X;
		}
		if( into_y < 0 )
		{
			BoardAtoms[x][y][Board[x][y].count-1].wrap_y--;
			into_y += BOARD_Y;
		}
		if( into_x >= BOARD_X )
		{
			BoardAtoms[x][y][Board[x][y].count-1].wrap_x++;
			into_x -= BOARD_X;
		}
		if( into_y >= BOARD_Y )
		{
			into_y -= BOARD_Y;
			BoardAtoms[x][y][Board[x][y].count-1].wrap_y++;
		}
	}
	players[Board[into_x][into_y].player].count -= Board[into_x][into_y].count;
	players[Board[x][y].player].count--;
	Board[x][y].count--;
	if( animate )
	{
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count] = BoardAtoms[x][y][Board[x][y].count];
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count].x += (x-real_into_x)*BoardSizeX;
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count].y += (y-real_into_y)*BoardSizeY;
		//BoardAtoms[into_x][into_y][Board[into_x][into_y].count].__x = BoardAtoms[into_x][into_y][Board[into_x][into_y].count].x;
		//BoardAtoms[into_x][into_y][Board[into_x][into_y].count].__y = BoardAtoms[into_x][into_y][Board[into_x][into_y].count].y;
	}
	// if 0, 1
	if( Board[into_x][into_y].count < 2 )
	{
		//if( Board[into_x][into_y].player != Board[x][y].player )
		//	DrawSquare( into_x, into_y, FALSE );
		Board[into_x][into_y].player = Board[x][y].player;
	}
	Board[into_x][into_y].count++;
	if( CheckSquareEx( into_x, into_y, FALSE ) )
		Board[into_x][into_y].player = Board[x][y].player;
	if( !animate )
		Board[into_x][into_y].stable = TRUE;
	else
		Board[into_x][into_y].stable = FALSE;
	players[Board[into_x][into_y].player].count += Board[into_x][into_y].count;
	if( !animate )
	{
		CheckSquareEx( into_x, into_y, TRUE );
		DrawSquare( into_x, into_y, TRUE );
	}
}

int FindWinner( void );


int UpdateBoard( void )
{
	int exploded;
	int x, y;
	int first = TRUE;
	//improve this with a queue process...
	do
	{
		exploded = FALSE;
		while( DequeCell( &x, &y ) )
		{
			exploded = TRUE;
#ifdef _WIN32
			if( first )
				PlaySound( "44magnum.wav", NULL, 0x00020003L );
#endif
			first = FALSE;
			if( sphere || ( x > 0 ) )
			{
				ExplodeInto( x, y, x-1, y );
			}
			if( sphere || ( x < ( BOARD_X - 1 ) ) )
			{
				ExplodeInto( x, y, x+1, y );
			}
			if( sphere || ( y > 0 ) )
			{
				ExplodeInto( x, y, x, y-1 );
			}
			if( sphere || ( y < ( BOARD_Y - 1 ) ) )
			{
				ExplodeInto( x, y, x, y+1 );
			}
			if( !Board[x][y].count )
			{
				//if( animate )
				//   DrawSquare( x, y, FALSE );
				Board[x][y].stable = TRUE;
			}
			else
				Board[x][y].stable = FALSE; //remaining peices may be moving some...
			if( !Board[x][y].count )
				Board[x][y].player = 0;
			if( !animate )
				DrawSquare( x, y, TRUE );
			if( !winstate )
			{
				DoLose(); // check to see if we knocked out someone..
				if( FindWinner() )
				{
					winstate = TRUE;
					//if( !animate )
					//	ShowPlayer( TRUE );
					return TRUE;
				}
			}
		}
      //Idle();//Relinquish();
	}while( exploded );
	//if( winstate )
	//	return FALSE;
	return FALSE;
}

void RenderCurrentAtoms( void )
{
	int bx, by;
	int n, cnt;
	int dx, dy;
	ATOMPOINT *xy;
	if( !animate )
		return;
	for( bx = 0; bx < BOARD_X; bx++ )
		for( by = 0; by < BOARD_Y; by++ )
		{
			cnt = Board[bx][by].count;
         /* copy this, becuase it may be updating internally the state... */
			xy = BoardAtoms[bx][by];
			dx = bx * BoardSizeX + (BoardSizeX/2);
			dy = by * BoardSizeY + (BoardSizeY/2);
			for( n = 0; n < cnt; n++ )
			{
				int show_x = (dx - HALFX) + xy[n].x;
				int show_y = (dy - HALFY) + xy[n].y;
				while( show_x > BoardSizeX * BOARD_X )
				{
					show_x -= BoardSizeX * BOARD_X;
					//xy[n].x -= BoardSizeX * BOARD_X;
				}
				while( show_x < 0)
				{
					show_x += BoardSizeX * BOARD_X;
					//xy[n].x += BoardSizeX * BOARD_X;
				}
				while( show_y > BoardSizeY * BOARD_Y )
				{
					show_y -= BoardSizeY * BOARD_Y;
					//xy[n].y -= BoardSizeY * BOARD_Y;
				}
				while( show_y < 0)
				{
					show_y += BoardSizeY * BOARD_Y;
					//xy[n].y += BoardSizeY * BOARD_Y;
				}
				if( show_y > (BoardSizeY * BOARD_Y - PeiceSize ) )
				{
					BlotScaledImageSizedEx( pGameBoard, pAtom
			      							, show_x
         									, show_y - ( BoardSizeY * BOARD_Y )
            								, PeiceSize, PeiceSize
											, 0, 0
		      								, pAtom->width, pAtom->height
		      								, ALPHA_TRANSPARENT
            								, BLOT_MULTISHADE
            								, 0, ColorAverage( Color(0,0,0),Color( 255,255,255), cnt-n, cnt )
											 , Colors[players[Board[bx][by].player].color] );

				}
				if( ( show_y ) < 0 )
				{
					BlotScaledImageSizedEx( pGameBoard, pAtom
			      							, show_x 
         									, show_y + ( BoardSizeY * BOARD_Y )
            								, PeiceSize, PeiceSize
											, 0, 0
		      								, pAtom->width, pAtom->height
		      								, ALPHA_TRANSPARENT
            								, BLOT_MULTISHADE
            								, 0, ColorAverage( Color(0,0,0),Color( 255,255,255), cnt-n, cnt )
											 , Colors[players[Board[bx][by].player].color] );
				}
				if( ( show_x ) > (BoardSizeX * BOARD_X - PeiceSize ) )
				{
					BlotScaledImageSizedEx( pGameBoard, pAtom
			      								, show_x - ( BoardSizeX * BOARD_X )
         										, show_y
            									, PeiceSize, PeiceSize
												, 0, 0
		      									, pAtom->width, pAtom->height
		      									, ALPHA_TRANSPARENT
            									, BLOT_MULTISHADE
            									, 0, ColorAverage( Color(0,0,0),Color( 255,255,255), cnt-n, cnt )
													, Colors[players[Board[bx][by].player].color] );
				}
				if( ( show_x ) < 0 )
				{
				   BlotScaledImageSizedEx( pGameBoard, pAtom
			      								, show_x + ( BoardSizeX * BOARD_X )
         										, show_y
            									, PeiceSize, PeiceSize
												, 0, 0
		      									, pAtom->width, pAtom->height
		      									, ALPHA_TRANSPARENT
            									, BLOT_MULTISHADE
            									, 0, ColorAverage( Color(0,0,0),Color( 255,255,255), cnt-n, cnt )
												 , Colors[players[Board[bx][by].player].color] );
				}
			   BlotScaledImageSizedEx( pGameBoard, pAtom
			      							, show_x
         									, show_y
            								, PeiceSize, PeiceSize
											, 0, 0
		      								, pAtom->width, pAtom->height
		      								, ALPHA_TRANSPARENT
            								, BLOT_MULTISHADE
            								, 0, ColorAverage( Color(0,0,0),Color( 255,255,255), cnt-n, cnt )
											 , Colors[players[Board[bx][by].player].color] );
			}
		}
}

void CPROC AnimateAtoms( uintptr_t unused )
{
	int bx, by;
	int n;
	int dx, dy;
	ATOMPOINT *xy;
	if( !animate )
		return;

	unstable = FALSE;
   //cnt = 0;
	for( bx = 0; bx < BOARD_X; bx++ )
		for( by = 0; by < BOARD_Y; by++ )
		{
			if( Board[bx][by].stable )
			{
            /* if this square is stable, don't need to draw/update anything */
				continue;
			}
			xy = BoardAtoms[bx][by];
			dx = bx * BoardSizeX + (BoardSizeX/2);
			dy = by * BoardSizeY + (BoardSizeY/2);

			//Board[bx][by].stable = TRUE;
			for( n = 0; n < Board[bx][by].count; n++ )
			{
				float v1x, v1y;
				float v2x, v2y;
            //float
				//double dist;
				//float x,y, xd, yd;
				//printf( "Drawing %d, %d\n", xy[n].x, xy[n].y );
            // move towards center...
				//lprintf( "Checking %g %g", xy[n]._x, xy[n].x );

				//v1x = 20/(xy[n].x * xy[n].x);
				//v1y = 20/(xy[n].y * xy[n].y);

				xy[n].x += -xy[n].x /20.0f;
				xy[n].y += -xy[n].y /20.0f;
            //lprintf( "Checking %g %g", xy[n]._x, xy[n].x );
				// move away from others near...
            {
            	int no;
					for( no = 0; no < Board[bx][by].count; no++ )
					{
						float xd, yd, dist;
						if( no == n )
							continue;
						xd = xy[no].x - xy[n].x;
						yd = xy[no].y - xy[n].y;
						if( !xd && !yd )
						{
							xy[n].x +=1;
							xy[n].y +=1;
							xd = 1;
							yd = 1;
						}
					   dist = sqrt( xd*xd + yd*yd);
					   if( dist < PeiceDist )
					   {
							xd /= dist;
							yd /= dist;

					   	dist = PeiceDist - dist;
					   	dist /= 2.0; // 2+
					   	xd *= dist;
					   	yd *= dist;
					   	xy[n].x -= xd;
					   	xy[n].y -= yd;
					   	xy[no].x += xd;
					   	xy[no].y += yd;
					   }
					} 
					//lprintf( "Checking %g %g", xy[n]._x, xy[n].x );
				}
			}
			for( n = 0; n < Board[bx][by].count; n++ )
			{
				float x,y;
            //lprintf( "Checking %g %g", xy[n]._x, xy[n].x );
				x = xy[n]._x - xy[n].x;
				y = xy[n]._y - xy[n].y;
            /* need to update these all at once so that the above calcualtion with [no] works */
				xy[n]._x = xy[n].x;
				xy[n]._y = xy[n].y;
				#define STABLEDEL (2.0/zoom)
				if( x < -STABLEDEL || x > STABLEDEL || y < -STABLEDEL || y > STABLEDEL )
				{
					//lprintf( "UNSTABLE!-------------" );
					// also update this so we get a 0 delta.
					Board[bx][by].stable = FALSE;
					//unstable = TRUE;
				}
			}
         /*
			if( Board[bx][by].stable )
			{
				for( n = 0; n < Board[bx][by].count; n++ )
				{
					// also update this so we get a 0 delta.
 					xy[n]._x = xy[n].x;
					xy[n]._y = xy[n].y;
					//lprintf( "FINAL - MATCH Checking %g %g", xy[n]._x, xy[n].x );
				}
				}
				*/
			// is unstable... has atoms moving into or out of(?)
			if( CheckSquareEx( bx, by, FALSE ) )
				WakeThread( pUpdateThread );
		}
	/* compute what explodes next... */
	//UpdateBoard(); // for each stable, enqueued suare...
	//if( !animate )
	//ShowPlayer( TRUE );

}



int FindWinner( void )
{
	if( all_players_went )
	{
		int n;
		int nFirst = 0;
		for( n = 1; n <= maxplayers; n++ )
		{
			if( players[n].active )
				if( !nFirst )
					nFirst = n;
			if( players[n].active &&
			    players[n].count && 
			    n != nFirst )
				break;
		}
		if( n > maxplayers )
		{
			players[nFirst].wins++;
			return TRUE;
		}
	}
	return FALSE;
}


void CPROC DrawBoard( uintptr_t unused )
{
	int x, y;
	// called 15 frames a second
	if( !hDisplay )
		return;
	drawing = TRUE;
	// actually if not animated - there
	// won't be a timer, and this is okay
	// to do full draw.
	//if( animate )
	{
		for( x = 0; x < BOARD_X; x++ )
			for( y = 0; y < BOARD_Y; y++ )
			{
				DrawSquare( x, y, FALSE );
			}
		first_draw = 0;
		if( animate )
		{
			//lprintf( " -- draw atoms..." );
			RenderCurrentAtoms();
		}
		ShowPlayer( TRUE );
		//lprintf( "--------------" );
		UpdateDisplay( hDisplay );
	}
	drawing = FALSE;
}

void CPROC DrawBoardTimer( uintptr_t unused )
{
   // a different thread, trigger a redraw.
   Redraw( hDisplay );
}


uintptr_t CPROC UpdateBoardThread( PTHREAD thread )
{
	int updated;
	while( 1 )
	{
		int bx, by;
		int updated;
		updated = 0;
		if( !updating_board_end )
		{
			updating_board = 1;
			unstable = FALSE;
			//cnt = 0;
			for( bx = 0; bx < BOARD_X; bx++ )
				for( by = 0; by < BOARD_Y; by++ )
				{
					if( CheckSquareEx( bx, by, TRUE ) )
						updated = 1;
				}
		}
		if( !updated )
		{
			updating_board = 0;
			WakeableSleep( 10000 );
		}
		else
		{
			UpdateBoard();
			updating_board = 0;
			Relinquish();
		}
	}
	return 0;
}


void AddPeice( int x, int y, int posx, int posy )
{
	if( animate && ( unstable || drawing ) )
	{
#ifndef BUILD_FLOWTEST
		//lprintf( "not adding a peice cause it's unstable..." );
      Idle();
		return;
#endif
	}
	if( Board[x][y].player == playerturn ||
	    !Board[x][y].count )
	{
		Board[x][y].player = playerturn;
		if( animate )
		{
			Board[x][y].stable = FALSE;
			unstable = TRUE;
		}
		else
			Board[x][y].stable = TRUE;

		BoardAtoms[x][y][Board[x][y].count].x = posx;
		BoardAtoms[x][y][Board[x][y].count].y = posy;

		Board[x][y].count++;

		players[playerturn].count++;
		players[playerturn].went = TRUE;
		players[playerturn].x = x;
		players[playerturn].y = y;
		if( !animate )
		{
			//MessageBox( NULL, "Not animated?", "debug", MB_OK );
			DrawSquare( x, y, TRUE );
			CheckSquare( x, y );
			if( UpdateBoard() ) // returns true if in a win state...
			{
				lprintf( "Returning early becuase of update board..." );
				return;
			}
		}
      //lprintf( "Here we go to the next player... this is about the only place..." );
		do
		{
         //lprintf( "idling wiating for an active player..." );
			playerturn++;
			if( playerturn > maxplayers )
			{
				all_players_went = TRUE;
				playerturn = 1;
			}
			// wait for an active player...
			// may not be the next one...
         //Idle();
         //Relinquish(); // but eventually one player will be active...
		} while( !players[playerturn].active );

		if( !animate )
			ShowPlayer( TRUE );

		{
			int _x, _y;
			_x = CursorX;
			_y = CursorY;
			CursorX = players[playerturn].x;
			CursorY = players[playerturn].y;
			//if( !animate )
			{
				//DrawSquare( _x, _y, TRUE );
				//DrawSquare( CursorX, CursorY, TRUE );
			}
		}

		//if( playerturn > maxplayers ||
		//	!players[playerturn].active )
		{
			int found = 0;
         int i;
			// find first active player - wrapped off end.
			for( i = playerturn; i <= maxplayers; i++ )
			{
				if( players[i].active )
				{
					found = 1;
					playerturn = i;
					break;
				}
			}
			if( !found )
				for( i = 1; i <= playerturn; i++ )
				{
					if( players[i].active )
					{
						found = 1;
						playerturn = i;
						break;
					}
				}
			if( !found )
			{

			}
		}
	}
	else
	{
#ifdef _WIN32
//		PlaySound( "slwhist.Wav", NULL, 0x00020003L | SND_NOWAIT);
            PlaySound( "slwhist.Wav", NULL, 0x00020003L );
#endif
	}
}

int CPROC Mouse( uintptr_t dwUser, int32_t x, int32_t y, uint32_t b )
{
	static int left, right;
	int Boardx, Boardy;
	Boardx = x/BoardSizeX;
	Boardy = y/BoardSizeY;
#ifndef BUILD_FLOWTEST
	if( winstate )
		return 1;
#endif
	if( players[playerturn].computer )
	{
#ifndef BUILD_FLOWTEST
      lprintf( "%d Computer's turn...", playerturn );
		return 1; // don't allow user updates for computer players
#endif
	}
	if( IsKeyDown( hDisplay, KEY_SPACE ) )
	{
		AddPeice( players[playerturn].x, players[playerturn].y, 0, 0 );
	}            
	if( KeyDown( hDisplay, KEY_RIGHT ) )
	{
		if( players[playerturn].x < (BOARD_X-1) )
		{
			players[playerturn].x++;
			{
				int _x, _y;
				_x = CursorX;
				_y = CursorY;
				CursorX = players[playerturn].x;
				CursorY = players[playerturn].y;
				//if( !animate )
				{
					DrawSquare( CursorX, CursorY, TRUE );
					DrawSquare( _x, _y, TRUE );
				}
			}
		}
	}
	if( KeyDown( hDisplay, KEY_LEFT ) )
	{
		if( players[playerturn].x > 0 )
		{
			players[playerturn].x--;
			{
				int _x, _y;
				_x = CursorX;
				_y = CursorY;
				CursorX = players[playerturn].x;
				CursorY = players[playerturn].y;
				//if( !animate )
				{
					DrawSquare( CursorX, CursorY, TRUE );
					DrawSquare( _x, _y, TRUE );
				}
			}
		}
	}
	if( KeyDown( hDisplay, KEY_UP ) )
	{
		if( players[playerturn].y > 0 )
		{
			players[playerturn].y--;
			{
				int _x, _y;
				_x = CursorX;
				_y = CursorY;
				CursorX = players[playerturn].x;
				CursorY = players[playerturn].y;
				//if( !animate )
				{
					DrawSquare( CursorX, CursorY, TRUE );
					DrawSquare( _x, _y, TRUE );
				}
			}
		}
	}
	if( KeyDown( hDisplay, KEY_DOWN ) )
	{
		if( players[playerturn].y < (BOARD_Y - 1) )
		{
			players[playerturn].y++;
			{
				int _x, _y;
				_x = CursorX;
				_y = CursorY;
				CursorX = players[playerturn].x;
				CursorY = players[playerturn].y;
				//if( !animate )
				{
					DrawSquare( CursorX, CursorY, TRUE );
					DrawSquare( _x, _y, TRUE );
				}
			}
		}
	}
	if( b & MK_LBUTTON && !left)
	{
		left = TRUE;
		if( Boardx < BOARD_X &&
			 Boardy < BOARD_Y )
		{
			int posx, posy;

			posx = x - (Boardx*BoardSizeX + BoardSizeX / 2);
			posy = y - (Boardy*BoardSizeY + BoardSizeY / 2);
			if( (Boardx == (BOARD_X-1)) &&
   			 (posx + HALFX) > (BoardSizeX/2) )
				posx = (BoardSizeX/2) - HALFX;
			else
				if( (!Boardx) &&
					(posx - HALFX) < -(BoardSizeX/2) )
					posx = (BoardSizeX/2) - HALFX;

			if( (Boardy == (BOARD_Y-1)) &&
				(posy + HALFY) > (BoardSizeY/2) )
				posy = (BoardSizeY/2) - HALFY;
			else
				if( (!Boardy) &&
					(posy - HALFY) < -(BoardSizeY/2) )
					posy = (BoardSizeY/2) - HALFY;
			AddPeice( Boardx, Boardy
					  , posx 
					  , posy );			
		}
	}
	if( left )
	{
		if( !(b & MK_LBUTTON) )
			left = FALSE;
	}
   return 1;
}

void CPROC MyRedraw( uintptr_t dwUser, PRENDERER pRenderer )
{
   if( !pGameBoard )
		pGameBoard = MakeSubImage( GetDisplayImage( pRenderer ), 0, 0, BoardSizeX * BOARD_X, BoardSizeY * BOARD_Y );
   ClearImageTo( GetDisplayImage( pRenderer ), BASE_COLOR_BLACK );
	DrawBoard( (uintptr_t)hDisplay );
}
SaneWinMain( argc, argv )
{
	//SetSystemLog( SYSLOG_FILENAME, "chainr.log" );
	g.pii = GetImageInterface();
	g.pri = GetDisplayInterface();
	AlignBaseToWindows();
	//hCursor = LoadCursor( IDC_ARROW);


	pAtom = LoadImageFile( "atom2.gif" );
	pGrid = LoadImageFile( "grid1.jpg" );
	pCursor = LoadImageFile( "cursor.jpg" );
	if( !pAtom || !pGrid || !pCursor )
	{
		printf( "Failed to load images..." );
		return 1;
	}
	{
		Colors[0] = Color( 255, 255, 255);
		Colors[1] = Color( 0, 0, 192);
		Colors[2] = Color( 192, 0, 0);
		Colors[3] = Color( 0, 173, 0);
		Colors[4] = Color( 192, 0, 192);
		Colors[5] = Color( 192, 192, 0);
		Colors[6] = Color( 128, 128, 128);
		Colors[7] = Color( 186, 160, 116);
		Colors[8] = Color( 170, 105, 42);
		Colors[9] = Color( 208, 22, 136);
	}
   SetBlotMethod( BLOT_MMX );
	{
		int n;
		for( n = 1; n <= MAX_PLAYERS; n++ )
		{
#ifdef BUILD_FLOWTEST
			players[n].computer = 1;
#endif
			snprintf( players[n].name, 64, "Player %d", n );
			players[n].color = n;
		}
	}

	NewGame = TRUE;

	while(1)
	{
		if( NewGame )
		{
			NewGame = FALSE;
			ClearBoard(); // property of clear board - select players, config players...
			if( !animate )
			{
				DrawBoard( (uintptr_t)hDisplay );
			}
			else
			{
				UpdateDisplay( hDisplay );
				if( !gnTimer )
					gnTimer = AddTimer( 1000/30, DrawBoardTimer, (uintptr_t)hDisplay );
				if( !gnAnimateTimer )
					gnAnimateTimer = AddTimer( 30, AnimateAtoms, 0 );
				if( !pUpdateThread )
					pUpdateThread = ThreadTo( UpdateBoardThread, 0 );
			}
		}
		if( ( !animate || !unstable ) &&
			 players[playerturn].computer && 
			!winstate )
		{
#ifdef BUILD_FLOWTEST
			int n;
			for( n = 0; n < 50; n++ )
			{
				lprintf( "can we cheat?" );
#endif
				ComputeMove( playerturn );
#ifdef BUILD_FLOWTEST
			}
#endif
		}
		else if( winstate )
		{
			SimpleMessageBox( NULL, "Someone Won!!!", "WINNER" );
			NewGame = TRUE;
		}
		else
			Sleep( 25 );
	}
}
EndSaneWinMain()
