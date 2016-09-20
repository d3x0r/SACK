#include <controls.h>
#define DEFINES_DEKWARE_INTERFACE
#include <plugin.h>
#include <space.h>
#include <pssql.h>
#include "../cards.h"


typedef struct {
	PSENTIENT ps;
	PENTITY pe;
	//PRENDERER renderer;
	PCOMMON frame;
	PCARD cards[4][13];
	PDECK deck;
	PHAND hand[9];

	uint32_t _b; // last button state.

	uint32_t step_x, step_y;
	uint32_t width, height;
	Image card_image[4][13];

	int nHand;
	int nTable;
	int hand_id[2];
	PCARD hand_card[2];
	Image hand_image[2];
	int table_id[5];
	PCARD table_card[5];
	Image table_image[5];

	int nWildUsed;
	PCARD wild_cards[5]; // at most 5 wild cards
	FILE *out;
} LOCAL;

static LOCAL l;


uintptr_t CPROC PlayHands( PTHREAD thread )
{
	int hand = 0;
	int h;
	l.deck = CreateDeck( WIDE("test_4kind_Holdem") ,IterateHoldemHand );
	for( h = 0; h < 9; h++ )
	{
		l.hand[h] = CreateHand( l.deck );
	}
	l.out = fopen( WIDE("carddata.csv"), WIDE("wt") );
	fprintf( l.out, WIDE("Game Win, hand, qualified, ontable, howmany, winning player, real_poker_value, 4kind_number,table, player 1, player 2, player 3, player 4, player 5, player 6, player 7, player 8, player 9\n") );
	while( hand < 0x40000000 )
	{
		int h;
		int player_win;
		Shuffle( l.deck );
		for( h = 0; h < 9; h++ )
		{
			DealTo( l.deck, l.hand[h], FALSE );
		}
		for( h = 0; h < 9; h++ )
		{
			DealTo( l.deck, l.hand[h], FALSE );
		}
		{
			DealTo( l.deck, NULL, FALSE );
			DealTo( l.deck, NULL, TRUE );
			DealTo( l.deck, NULL, TRUE );
			DealTo( l.deck, NULL, TRUE );

			DealTo( l.deck, NULL, FALSE );
			DealTo( l.deck, NULL, TRUE );

			DealTo( l.deck, NULL, FALSE );
			DealTo( l.deck, NULL, TRUE );
		}
		{
			int player;
			int bestval = 0;
			int bestplayer;
			PHAND best_hand;
			int val;
			int multi = 0;
			for( player = 0; player < 9; player++ )
			{
				val = ValuePokerHand( l.hand[player], NULL, TRUE );
				if( val > bestval )
				{
					bestval = val;
					bestplayer = player;
					best_hand = l.hand[player];
				}
				if( ( val & POKER_HAND_MASK ) == POKER_HAND_4KIND )
				{
					multi++;
				}
			}
			if( multi > 0 )
			{
				PVARTEXT pvt = VarTextCreate();
				PCARD card;
				int qualified = 0;
				int ontable = 0;
				PCARD_STACK Draw = GetCardStack( l.deck, WIDE("Table") );
				vtprintf( pvt, WIDE("'") );
				for( card = Draw->cards; card; card = card->next )
				{
					PTEXT tmp;
					if( (card->id % 13) == ((bestval & 0xF0)>>4) )
						ontable++;
					vtprintf( pvt, WIDE("%3s "), GetText( tmp = CardName( card ) ) );
					LineRelease( tmp );
				}
				vtprintf( pvt, WIDE("'") );

				for( h = 0; h < 9; h++ )
				{
					int first = 1;
					Draw = GetCardStackFromHand( l.hand[h], WIDE("Cards") );
					vtprintf( pvt, WIDE(",'") );
					ontable = 0;
					for( card = Draw->cards; card; card = card->next )
					{
						PTEXT tmp;
						if( (card->id % 13) == ((bestval & 0xF0)>>4) )
							ontable++;
						if( ontable >= 2 )
							qualified++;
						vtprintf( pvt, WIDE("%s%3s"), first?WIDE(""):WIDE(" "), GetText( tmp = CardName( card ) ) );
						first = 0;
						LineRelease( tmp );
					}
					vtprintf( pvt, WIDE("'") );
					
				}
				fprintf( l.out, WIDE("'4kind Win',%d,%d,%d,%d,%d,%06X,%d,%s\n"), hand, qualified, ontable, multi
					, bestplayer+1
					, bestval
					, ( ( bestval & 0xF0 ) >> 4 ) + 1
					, GetText( VarTextPeek( pvt ) ) );
				VarTextDestroy( &pvt );
			}
			else
			{
				PVARTEXT pvt = VarTextCreate();
				PCARD card;
				int ontable = 0;
				PCARD_STACK Draw = GetCardStack( l.deck, WIDE("Table") );
				vtprintf( pvt, WIDE("'") );
				for( card = Draw->cards; card; card = card->next )
				{
					PTEXT tmp;
					if( card->id % 13 == (bestval & 0xF0000)>>16 )
						ontable++;
					vtprintf( pvt, WIDE("%3s "), GetText( tmp = CardName( card ) ) );
					LineRelease( tmp );
				}
				vtprintf( pvt, WIDE("'") );

				for( h = 0; h < 9; h++ )
				{
					int first = 1;
					Draw = GetCardStackFromHand( l.hand[h], WIDE("Cards") );
					vtprintf( pvt, WIDE(",'") );
					for( card = Draw->cards; card; card = card->next )
					{
						PTEXT tmp;
						if( card->id % 13 == (bestval & 0xF0000)>>16 )
							ontable++;
						vtprintf( pvt, WIDE("%s%3s"), first?WIDE(""):WIDE(" "), GetText( tmp = CardName( card ) ) );
						first = 0;
						LineRelease( tmp );
					}
					vtprintf( pvt, WIDE("'") );
					
				}
				{
					PTEXT tmp = NULL;
					fprintf( l.out, WIDE("'%s',%d,0,%d,%d,%d,%06X,%d,%s\n"), GetText( GetPokerHandName(best_hand, &tmp ) ), hand, 0, 0, bestplayer+1, bestval, 0, GetText( VarTextPeek( pvt ) ) );
					Release( tmp );
				}
				VarTextDestroy( &pvt );
			}
		}
		hand++;
	}
}

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	ThreadTo( PlayHands, 0 );
	return DekVersion;
}

