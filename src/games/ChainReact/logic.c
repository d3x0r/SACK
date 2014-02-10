
#include <stdlib.h>
#include <stdio.h>
#include "chain.h"


extern int _boardx;
extern int _boardy;
#define BOARD_X _boardx
#define BOARD_Y _boardy

extern CELL Board[MAX_BOARD_X][MAX_BOARD_Y];

extern PLAYER players[MAX_PLAYERS + 1]; // players[0] is never used...
extern int playerturn; // 1...n
extern int maxplayers; // max players...

extern int all_players_went;

#define GetCell( Board, x, y ) ( Board + x + ( y * MAX_BOARD_Y ) )

void PrintLogicBoard( CELL *Board )
{
	int x, y;
	printf( "----------------------------------------------------------\n" );
	for( y = 0; y < BOARD_Y; y++ )
	{
		printf( "| " );
		for( x = 0; x < BOARD_X; x++ )
		{
			printf( "%d-%d |"
					, GetCell( Board, x, y )->count
					, GetCell( Board, x, y )->player );
		}
		printf( "\n----------------------------------------------------------\n" );
	}
}



// Before calling this the spot MUST be a valid place....
// will add a peice ALWAYS
int LogicAddPeice( CELL *Board, int x, int y, int color )
{
	int c;
	PCELL cell = GetCell( Board, x, y );
	cell->count++;
	cell->player = color;
	c = 2;
	if( x > 0 && x < (BOARD_X-1) )
		c++;
	if( y > 0 && y < (BOARD_Y-1) )
		c++;
	while( cell->count >= c )
	{
		cell->count -= c;
		if( !cell->count )
			cell->player = 0;
		if( x > 0 )
			LogicAddPeice( Board, x-1, y, color );
		if( y > 0 )
			LogicAddPeice( Board, x, y-1, color );
		if( x < ( BOARD_X-1) )
			LogicAddPeice( Board, x+1, y, color );
		if( y < (BOARD_Y-1) )
			LogicAddPeice( Board, x, y+1, color );
	}
		PrintLogicBoard( Board );
      return 1;
}


int FindMove( int level, CELL Board[MAX_BOARD_X][MAX_BOARD_Y] )
{
	
      return 0;
}

void ComputeMove( int player )
{
	int c;
// force AI players to wait for 1 round of human players.
	if( !all_players_went )
	{
		for( c = 0; c < 5; c++ )
		{
			switch( c )
			{
			case 0:
				if( !Board[0][0].count )
				{
					AddPeice( 0, 0, 0, 0 );
					return;
				}
				break;
			case 1:
				if( !Board[BOARD_X-1][0].count )
				{
					AddPeice( BOARD_X-1, 0, 0, 0 );
					return;
				}
				break;
			case 2:
				if( !Board[0][BOARD_Y-1].count )
				{
					AddPeice( 0, BOARD_Y-1, 0, 0 );
					return;
				}
				break;
			case 3:
				if( !Board[BOARD_X-1][BOARD_Y-1].count )
				{
					AddPeice( BOARD_X-1, BOARD_Y-1, 0, 0 );
					return;
				}
				break;
			case 4:
				{
					int x, y;
					do
					{
						x = ( rand() * BOARD_X ) / RAND_MAX;
						y = ( rand() * BOARD_Y ) / RAND_MAX;
					}
					while( Board[x][y].count && 
							( Board[x][y].player != player ) );
					AddPeice( x, y, 0, 0 );
				}
			}
		}
	}			
	else
	{
		int x, y;
		do
		{
			x = ( rand() * BOARD_X ) / RAND_MAX;
			y = ( rand() * BOARD_Y ) / RAND_MAX;
		}
		while( Board[x][y].count && 
				( Board[x][y].player != player ) );
		AddPeice( x, y, 0, 0 );
	}
}