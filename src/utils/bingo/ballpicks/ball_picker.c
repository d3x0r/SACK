#include <stdhdrs.h>

#include "ball_shuffer.h"
#include "cardset_loader.h"

struct card_tester {
	char *card;
	int marks;
};
typedef struct card_tester CARD_TESTER;
typedef struct card_tester *PCARD_TESTER;
#define MAXCARD_TESTERSPERSET 32000
DeclareSet( CARD_TESTER );
PCARD_TESTERSET test_set;


struct card_tester_link {
	struct card_tester_link *next;
	struct card_tester_link **me;
	struct card_tester *tester;
};
typedef struct card_tester_link CARD_TESTER_LINK;
typedef struct card_tester_link *PCARD_TESTER_LINK;
#define MAXCARD_TESTER_LINKSPERSET 3200
DeclareSet( CARD_TESTER_LINK );
PCARD_TESTER_LINKSET test_set_links;


PCARD_TESTER_LINK sorted_cards[76];
int marks[25];
int counter;

uintptr_t CPROC TestOk( POINTER p, uintptr_t psv )
{
	PCARD_TESTER tester = (PCARD_TESTER)p;
	//lprintf( "card has %p has %d marks", tester, tester->marks );
	marks[tester->marks]++;
	return 0;
}

uintptr_t CPROC TestOk2( POINTER p, uintptr_t psv )
{
	if( counter == 72000 )
	{
		counter = 0;
		return 1;
	}
	{
		PCARD_TESTER tester = (PCARD_TESTER)p;
		marks[tester->marks]++;
	}
	counter++;
	return 0;
}


int my_balls[45];
int column;
int max_counts[5];

int *PickBalls( int *balls )
{
   int out = 0;
	int counts[5];
	int n;
	for( n = 0; n < 5; n++ )
		counts[n] = 0;
	for( n = 0; n < 75; n++ )
	{
		int col = (balls[n]-1) / 15;
		if( counts[col] < max_counts[col] )
		{
			my_balls[out++] = balls[n];
			if( out == 45 )
				return my_balls;
			counts[col]++;
		}
	}
	return my_balls;
}

int main( void )
{
	int n;
	char tmp[256];
   int card_count = 0;
	int card;
	char *cardset;
	int cardset_start = 0;
	int cardset_end = 32000;
	snprintf( tmp, sizeof( tmp ), "cards.dat" );
	printf( "Loading cardset....\n" );
	LoadCardset( tmp, &card_count, &cardset );
	{
      int num;
      //int s =  ( n - 1 ) / 15;
		printf( "Sorting cardset...\n" );
		for( card = 0; card < card_count; card++ )
		{
			PCARD_TESTER tester = GetFromSet( CARD_TESTER, &test_set );
			tester->card = cardset + (card * 25);
			//lprintf( "card %d(%d) = %p", card, GetMemberIndex( CARD_TESTER, &test_set, tester ), tester );
			for( num = 0; num < 25; num++ )
			{
				if( num == 12 )
					continue;
				{
					PCARD_TESTER_LINK link = GetFromSet( CARD_TESTER_LINK, &test_set_links );
					PCARD_TESTER_LINK *base_tester= &sorted_cards[cardset[card*25+num]];
					link->tester = tester;
					//lprintf( "Add card %d to %d", card, cardset[card*25+num] );
					if( link->next = (*base_tester) )
						(*base_tester)->me = &link->next;
					(*base_tester) = link;
				}
			}
		}
	}
	printf( "Drawing balls...\n" );
	for( n = 0; n < 0x40000000; n++ )
	{
      int *source_balls = DrawRandomNumbers2();
		int *balls;
		int ball;

		switch( n % 6 )
		{
		case 0:
			max_counts[0] = 5;
			max_counts[1] = 15;
			max_counts[2] = 15;
			max_counts[3] = 15;
			max_counts[4] = 5;
			break;
		case 1:
			max_counts[0] = 15;
			max_counts[1] = 5;
			max_counts[2] = 15;
			max_counts[3] = 15;
			max_counts[4] = 5;
			break;
		case 2:
			max_counts[0] = 5;
			max_counts[1] = 15;
			max_counts[2] = 15;
			max_counts[3] = 5;
			max_counts[4] = 15;
			break;
		case 3:
			max_counts[0] = 15;
			max_counts[1] = 5;
			max_counts[2] = 15;
			max_counts[3] = 5;
			max_counts[4] = 15;
			break;
		case 4:
			max_counts[0] = 5;
			max_counts[1] = 5;
			max_counts[2] = 15;
			max_counts[3] = 15;
			max_counts[4] = 15;
			break;
		case 5:
			max_counts[0] = 15;
			max_counts[1] = 15;
			max_counts[2] = 15;
			max_counts[3] = 5;
			max_counts[4] = 5;
			break;
		}

      balls = PickBalls( source_balls );

		for( card = 0; card < card_count; card++ )
		{
			PCARD_TESTER tester = GetSetMember( CARD_TESTER, &test_set, card );
			tester->marks = 0;
		}
		for( ball = 0; ball <= 75; ball++ )
		{
			lprintf( "First card %p", sorted_cards[ball] );
			
		}
		for( ball = 0; ball < 45; ball++ )
		{
			int cards_marked = 0;
			int real_ball = balls[ball];
			PCARD_TESTER_LINK tester;
			PCARD_TESTER_LINK prior = NULL;
			PCARD_TESTER_LINK _prior = NULL;
			lprintf( "First card %p", sorted_cards[real_ball] );
			for( tester = sorted_cards[real_ball]; tester; tester = tester->next )
			{
				cards_marked++;
				tester->tester->marks++;
				_prior = prior;
				prior = tester;
			}
			lprintf( "Marked %d cards with %d", cards_marked, real_ball );
		}

		printf( "balls: " );
		for( ball = 0; ball < 75; ball++ )
		{
			printf( "%d,", balls[ball] );
		}
		printf( "\n" );
		fflush( stdout );
		{
			for( ball = 0; ball < 25; ball++ )
			{
				marks[ball] = 0;
			}
			ForAllInSet( CARD_TESTER, test_set, TestOk, 0 );
			printf( "cards with X marks: " );
			for( ball = 0; ball < 25; ball++ )
			{
				printf( "%d,", marks[ball] );
			}
		}
		printf( "\n" );
		fflush( stdout );
		{
			for( ball = 0; ball < 25; ball++ )
			{
				marks[ball] = 0;
			}
			ForAllInSet( CARD_TESTER, test_set, TestOk2, 0 );
			printf( "cards with X marks: " );
			for( ball = 0; ball < 25; ball++ )
			{
				printf( "%d,", marks[ball] );
			}
		}
		printf( "\n" );
		fflush( stdout );
	}
	return 0;
}
