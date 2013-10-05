//#define BUILD_FLOWTEST

// the motion needs to wrap the atoms outside and back on-board
//#define SPHEREICAL_BOARD
#define CHAIN_REACT_MAIN
#include <stdhdrs.h>
#include <logging.h>

#include "global.h"

#include <idle.h>
#ifdef _WIN32
#include <conio.h>
#include <mmsystem.h>
#endif
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
	float x, y, _x, _y, __x, __y;
} ATOMPOINT;

CELL Board[MAX_BOARD_X][MAX_BOARD_Y];
ATOMPOINT BoardAtoms[MAX_BOARD_X][MAX_BOARD_Y][8]; // use Board[x][y].count 

#define QUEUESIZE MAX_BOARD_X * MAX_BOARD_Y

XYPOINT Explode[QUEUESIZE];
int ExplodeHead, ExplodeTail;


PLAYER players[MAX_PLAYERS + 1]; // players[0] is never used...
int playerturn; // 1...n
int maxplayers; // max players...

int first_draw = 1;
int animate = 0;
int unstable;
int drawing;
int all_players_went;


void CPROC MyRedraw( PTRSZVAL dwUser, PRENDERER pRenderer );
int CPROC Mouse( PTRSZVAL dwUser, S_32 x, S_32 y, _32 b );


void CPROC Close( PTRSZVAL dwUser )
{
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
		Idle();//Relinquish();
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
	ConfigureBoard( &animate );

	BoardSizeX = pGrid->width / zoom;
	BoardSizeY = pGrid->height / zoom;
	PeiceSize = PeiceBase / zoom;
	PeiceDist = ((PeiceSize*11)/16 );
#ifdef _WIN32
        SetApplicationTitle( WIDE("Application Title doesn't work!") );
#endif

	hDisplay = OpenDisplaySizedAt( 0, 200 + BOARD_X * BoardSizeX, 30 + BOARD_Y * BoardSizeY, 0, 0 );
	SetCloseHandler( hDisplay, Close, (PTRSZVAL)hDisplay );
	SetRedrawHandler( hDisplay, MyRedraw, (PTRSZVAL)hDisplay );
	SetMouseHandler( hDisplay, Mouse, (PTRSZVAL) hDisplay );
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
   //lprintf( WIDE("Rendering blank square %d,%d"), x, y );
	if( x == CursorX && y == CursorY )
	{
		BlotScaledImageSizedEx( GetDisplayImage(hDisplay), pCursor
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
   	BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pGrid
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
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
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
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
      								, dx + (CENTX - (HALFX*3/2)), dy + (CENTY-(HALFY*3/2))
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
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
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
      								, dx + (CENTX - (5*THIRDX/2) ), dy + (CENTY - (2*THIRDY))
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
      								, dx + (CENTX - (THIRDX/2)), dy + (CENTY - (2*THIRDY))
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
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
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
      								, dx + (CENTX - 2*HALFY + 3), dy + (CENTY - HALFY)
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
      								, dx + (CENTX - 3), dy + (CENTY - HALFY)
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
      								, dx + (CENTX - HALFX), dy + (CENTY - 2*HALFY + 3 )
      								, PeiceSize, PeiceSize
                              , 0, 0
      								, pAtom->width, pAtom->height
      								, ALPHA_TRANSPARENT
      								, BLOT_MULTISHADE
      								, 0, Color( 255,255,255)
      								, Colors[players[Board[x][y].player].color] );	
      BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
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
	//printf( WIDE("Head: %d Tail: %d\n"), ExplodeHead, ExplodeTail );
	for( n = ExplodeTail; n != ExplodeHead; n++ )
	{
		if( n == (QUEUESIZE-1) )
			n = 0;
		if( n == ExplodeHead )
			break;
		if( Explode[n].x == x && 
			 Explode[n].y == y )
		{
			//printf( WIDE("Already Enqueued?\n") );
		   return;
		}
	}
	//printf( WIDE("Head: %d\n"), ExplodeHead );
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
	//printf( WIDE("Tail: %d\n"), ExplodeTail );
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
#ifdef SPHEREICAL_BOARD
	c = 4;
#else
	c = 2;
	if( x>0 && x < (BOARD_X-1) )
		c++;
	if( y>0 && y < (BOARD_Y-1) )
		c++;
#endif
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
	BlatColor( GetDisplayImage( hDisplay ), BoardSizeX * BOARD_X, 5, 130, 26 + 26*maxplayers, 0 );
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
			snprintf( buf, 32, WIDE(" : %d (%d)      "), players[i].wins, players[i].count );
			PutString( GetDisplayImage( hDisplay )
				, BoardSizeX * BOARD_X + 35, 11 + (26 * i)
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
					, BoardSizeX * BOARD_X + 35, 11
					, Color( 255,255,255 ), Color(0,0,1)
					, WIDE("To move next") );
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
#ifdef SPHEREICAL_BOARD
	if( into_x < 0 )
		into_x += BOARD_X;
	if( into_y < 0 )
		into_y += BOARD_Y;
	if( into_x >= BOARD_X )
      into_x -= BOARD_X;
	if( into_y >= BOARD_Y )
		into_y -= BOARD_Y;
#endif
	players[Board[into_x][into_y].player].count -= Board[into_x][into_y].count;
	players[Board[x][y].player].count--;
	Board[x][y].count--;
	if( animate )
	{
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count] = BoardAtoms[x][y][Board[x][y].count];
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count].x += (x-into_x)*BoardSizeX;
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count].y += (y-into_y)*BoardSizeY;
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count].__x = BoardAtoms[into_x][into_y][Board[into_x][into_y].count].x;
		BoardAtoms[into_x][into_y][Board[into_x][into_y].count].__y = BoardAtoms[into_x][into_y][Board[into_x][into_y].count].y;
	}
   // if 0, 1
	if( Board[into_x][into_y].count < 2 )
	{
		//if( Board[into_x][into_y].player != Board[x][y].player )
      //   DrawSquare( into_x, into_y, FALSE );
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
            PlaySound( WIDE("44magnum.wav"), NULL, 0x00020003L );
#endif
			first = FALSE;
#ifndef SPHEREICAL_BOARD
			if( x > 0 )
#endif
			{
            ExplodeInto( x, y, x-1, y );
			}
#ifndef SPHEREICAL_BOARD
			if( x < ( BOARD_X - 1 ) )
#endif
			{
            ExplodeInto( x, y, x+1, y );
			}
#ifndef SPHEREICAL_BOARD
			if( y > 0 )
#endif
			{
            ExplodeInto( x, y, x, y-1 );
			}
#ifndef SPHEREICAL_BOARD
			if( y < ( BOARD_Y - 1 ) )
#endif
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
               //   ShowPlayer( TRUE );
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
			   BlotScaledImageSizedEx( GetDisplayImage( hDisplay ), pAtom
			      							, (dx - HALFX) + xy[n].x
         									, (dy - HALFY) + xy[n].y
            								, PeiceSize, PeiceSize
                                       , 0, 0
		      								, pAtom->width, pAtom->height
		      								, ALPHA_TRANSPARENT
            								, BLOT_MULTISHADE
            								, 0, ColorAverage( Color(0,0,0),Color( 255,255,255), cnt-n, cnt )
											 , Colors[players[Board[bx][by].player].color] );
#if 0
				if( !Board[bx][by].stable )
				{
					IMAGE_RECTANGLE r1, r2, r;
					r1.x = (dx - HALFX) + xy[n].x;
					r1.y = (dy - HALFY) + xy[n].y;
               r1.width = PeiceSize;
               r1.height = PeiceSize;
					r2.x = (dx - HALFX) + xy[n].__x;
					r2.y = (dy - HALFY) + xy[n].__y;
               r2.width = PeiceSize;
					r2.height = PeiceSize;
					xy[n].__x = xy[n].x;
					xy[n].__y = xy[n].y;
					//DebugBreak();
               //lprintf( WIDE("OUtput is at %d,%d to %d,%d (%d,%d) Total(%d,%d)"), r1.x, r1.y, r2.x, r2.y, r1.width, r1.height, r.width, r.height );
					MergeRectangle( &r, &r1, &r2 );
               /*
					UpdateDisplayPortion( hDisplay
					, r.x, r.y, r.width, r.height );
               */
				}
#endif
			}
		}
}

void CPROC AnimateAtoms( PTRSZVAL unused )
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
				//printf( WIDE("Drawing %d, %d\n"), xy[n].x, xy[n].y );
            // move towards center...
				//lprintf( WIDE("Checking %g %g"), xy[n]._x, xy[n].x );

				//v1x = 20/(xy[n].x * xy[n].x);
				//v1y = 20/(xy[n].y * xy[n].y);

				xy[n].x += -xy[n].x /20.0f;
				xy[n].y += -xy[n].y /20.0f;
            //lprintf( WIDE("Checking %g %g"), xy[n]._x, xy[n].x );
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
					//lprintf( WIDE("Checking %g %g"), xy[n]._x, xy[n].x );
				}
			}
			for( n = 0; n < Board[bx][by].count; n++ )
			{
				float x,y;
            //lprintf( WIDE("Checking %g %g"), xy[n]._x, xy[n].x );
				x = xy[n]._x - xy[n].x;
				y = xy[n]._y - xy[n].y;
            /* need to update these all at once so that the above calcualtion with [no] works */
				xy[n]._x = xy[n].x;
				xy[n]._y = xy[n].y;
				#define STABLEDEL (2.0/zoom)
				if( x < -STABLEDEL || x > STABLEDEL || y < -STABLEDEL || y > STABLEDEL )
				{
               //lprintf( WIDE("UNSTABLE!-------------") );
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
					//lprintf( WIDE("FINAL - MATCH Checking %g %g"), xy[n]._x, xy[n].x );
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


void CPROC DrawBoard( PTRSZVAL unused )
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
				//lprintf( WIDE("Drwaing...") );
            /*
				if( first_draw
					|| ( x && !Board[x-1][y].stable )
					|| ( y && !Board[x][y-1].stable )
               || ( ( x < (BOARD_X-1)) && !Board[x+1][y].stable )
					|| ( ( y < (BOARD_Y-1)) && !Board[x][y+1].stable )
					|| ( !Board[x][y].stable )
					)
               */
					DrawSquare( x, y, FALSE );
			}
      first_draw = 0;
		if( animate )
		{
         //lprintf( WIDE(" -- draw atoms...") );
			RenderCurrentAtoms();
		}
		ShowPlayer( TRUE );
		//lprintf( WIDE("--------------") );
		UpdateDisplay( hDisplay );
	}
	drawing = FALSE;
}

void CPROC DrawBoardTimer( PTRSZVAL unused )
{
   // a different thread, trigger a redraw.
   Redraw( hDisplay );
}


PTRSZVAL CPROC UpdateBoardThread( PTHREAD thread )
{
   int updated;
	while( 1 )
	{
		int bx, by;
		int updated = 0;
		unstable = FALSE;
		//cnt = 0;
		for( bx = 0; bx < BOARD_X; bx++ )
			for( by = 0; by < BOARD_Y; by++ )
			{
				if( CheckSquareEx( bx, by, TRUE ) )
               updated = 1;
			}
      if( !updated )
			WakeableSleep( 10000 );
		else
         UpdateBoard();
	}
   return 0;
}


void AddPeice( int x, int y, int posx, int posy )
{
	if( animate && ( unstable || drawing ) )
	{
#ifndef BUILD_FLOWTEST
		//lprintf( WIDE("not adding a peice cause it's unstable...") );
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
			//MessageBox( NULL, WIDE("Not animated?"), WIDE("debug"), MB_OK );
			DrawSquare( x, y, TRUE );
			CheckSquare( x, y );
			if( UpdateBoard() ) // returns true if in a win state...
			{
            lprintf( WIDE("Returning early becuase of update board...") );
				return;
			}
		}
      //lprintf( WIDE("Here we go to the next player... this is about the only place...") );
		do
		{
         //lprintf( WIDE("idling wiating for an active player...") );
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
//		PlaySound( WIDE("slwhist.Wav"), NULL, 0x00020003L | SND_NOWAIT);
            PlaySound( WIDE("slwhist.Wav"), NULL, 0x00020003L );
#endif
	}
}

int CPROC Mouse( PTRSZVAL dwUser, S_32 x, S_32 y, _32 b )
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
      lprintf( WIDE("%d Computer's turn..."), playerturn );
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

void CPROC MyRedraw( PTRSZVAL dwUser, PRENDERER pRenderer )
{
	DrawBoard( (PTRSZVAL)hDisplay );
}
SaneWinMain( argc, argv )
{
	//SetSystemLog( SYSLOG_FILENAME, WIDE("chainr.log") );
	AlignBaseToWindows();
	//hCursor = LoadCursor( IDC_ARROW);

	g.pii = GetImageInterface();
   g.pri = GetDisplayInterface();

	pAtom = LoadImageFile( WIDE("atom2.gif") );
	pGrid = LoadImageFile( WIDE("grid1.jpg") );
	pCursor = LoadImageFile( WIDE("cursor.jpg") );
	if( !pAtom || !pGrid || !pCursor )
	{
		printf( WIDE("Failed to load images...") );
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
			snprintf( players[n].name, 64, WIDE("Player %d"), n );
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
				DrawBoard( (PTRSZVAL)hDisplay );
			}
			else
			{
				UpdateDisplay( hDisplay );
				if( !gnTimer )
					gnTimer = AddTimer( 1000/30, DrawBoardTimer, (PTRSZVAL)hDisplay );
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
				lprintf( WIDE("can we cheat?") );
#endif
				ComputeMove( playerturn );
#ifdef BUILD_FLOWTEST
			}
#endif
		}
		else if( winstate )
		{
			SimpleMessageBox( NULL, WIDE("Someone Won!!!"), WIDE("WINNER") );
			NewGame = TRUE;
		}
		else
			Sleep( 25 );
	}
}
EndSaneWinMain()
