#include <sack_types.h>
#include <colordef.h>

typedef struct cell {
   unsigned int player:4; // player that controls this space 
	unsigned int count:4;  // count of peices here 
	unsigned int stable:1; // whether this spot has reached near 0 entropy
} CELL, *PCELL;

#define MAX_BOARD_X 52
#define MAX_BOARD_Y 44

#ifndef CHAIN_REACT_MAIN
extern
#endif
 CDATA Colors[10];

#define MAX_PLAYERS ( sizeof( Colors ) / sizeof( CDATA ) - 1)

typedef struct player_tag {
	int x, y;
	int color;
	int active;
	int went;
	int wins;
	int count; // maybe check this for win/loss?
	int computer;
	char name[64];
} PLAYER, *PPLAYER;


void AddPeice( int x, int y, int posx, int posy );

int ConfigurePlayers( void );
void PlayerErrorMin( void );

void ConfigureBoard( int *animate );

