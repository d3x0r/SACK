#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <controls.h>
//#include <render.h>
//#include <image.h>
#define DEFINES_DEKWARE_INTERFACE
#include <plugin.h>
#include <space.h>
#include <pssql.h>
#include "../cards.h"

#if 0
//# Host: 192.168.1.1
//# Table: 'poker_odds'
//#
#endif

TEXTCHAR * create_table=WIDE("CREATE TABLE if not exists `poker_odds` (")
WIDE("  `ID` int(11) NOT NULL auto_increment,")
WIDE("  `card1` int(11) NOT NULL default '0',")
WIDE("  `card2` int(11) NOT NULL default '0',")
WIDE("  `card3` int(11) NOT NULL default '0',")
WIDE("  `card4` int(11) NOT NULL default '0',")
WIDE("  `card5` int(11) NOT NULL default '0',")
WIDE("  `value` int(11) NOT NULL default '0',")
WIDE("  `class` int(11) NOT NULL default '0',")
WIDE("  `name` varchar(32) NOT NULL default '',")
WIDE("  `split` int(11) NOT NULL default '0',")
WIDE("  `win` int(11) NOT NULL default '0',")
WIDE("  `lose` int(11) NOT NULL default '0',")
WIDE("  `best` int(11) NOT NULL default '0',")
WIDE("  `better` int(11) NOT NULL default '0',")
WIDE("  PRIMARY KEY  (`ID`),")
WIDE("  UNIQUE KEY `hand` (`card1`,`card2`,`card3`,`card4`,`card5`),")
WIDE("  KEY `valkey` (`value`)")
WIDE("								  ) TYPE=MyISAM COMMENT='massive lookup for referencing odds on poker hands...';");

PRELOAD( CreateCardOddsTable )
{
   DoSQLCommand( create_table );
//	PTABLE table = GetFieldsInSQL( create_table, 0);
//   CheckODBCTable( table, 0 );
}
#if 0

create table poker_split select value,count(*) as split from poker_odds group by value

create table poker_lose select a.value as value,sum(a.count) as count, sum(b.count) as lose from poker_split as a
 join poker_split as b on a.value<b.value
group by a.value

create table poker_win 
select a.value as value,sum(a.count) as count, sum(b.count) as win from poker_split as a 
 join poker_split as b on a.value>b.value
group by a.value

#endif

typedef struct {
	PSENTIENT ps;
	PENTITY pe;
	//PRENDERER renderer;
	PCOMMON frame;
	PCARD cards[4][13];
	PDECK deck;
   PHAND hand;

	_32 _b; // last button state.

   _32 step_x, step_y;
   _32 width, height;
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
} LOCAL;

static LOCAL l;


void ResetTable( void )
{
   int s, f;
	l.nHand = 0;
	l.nTable = 0;
	for( s = 0; s < 4; s++ )
		for( f = 0; f < 13; f++ )
		{
         //lprintf( WIDE("picking cards... uhh...") );
			l.cards[s][f] = PickACard( l.deck, CARD_ID( s, f ) );
		}
}

int CPROC TableDraw( PCOMMON pf )
{
	Image surface = GetFrameSurface( pf );
	// and then I can draw like cards or something here?
	{
		int s, f;
		for( s = 0; s < 4; s++ )
			for( f = 0; f < 13; f++ )
			{
				if( l.card_image[s][f] )
					BlotImage( surface, l.card_image[s][f], f * l.step_x, s * l.step_y );
			}
			BlatColor( surface, 300, 10, l.width + l.step_x * 1, l.height, GetBaseColor( NORMAL ) );
			BlatColor( surface, 300, 90, l.width + l.step_x * 4, l.height, GetBaseColor( NORMAL ) );
		for( s = 0; s < l.nHand; s++ )
		{
			BlotImage( surface, l.hand_image[s], 300 + s * l.step_x, 10 );
		}
		for( s = 0; s < l.nTable; s++ )
		{
			BlotImage( surface, l.table_image[s], 300 + s * l.step_x, 90 );
		}
		{
			PTEXT hand_string = NULL;
			TEXTCHAR output[128];
			GetPokerHandName( l.hand, &hand_string );
			if( hand_string )
			{
            snprintf( output, sizeof( output ), WIDE("%s                             "), GetText( hand_string ) );
				PutString( surface, 50, 200, GetBaseColor( TEXTCOLOR ), GetBaseColor( NORMAL ), output );
			}
			else
				PutString( surface, 50, 200, GetBaseColor( TEXTCOLOR ), GetBaseColor( NORMAL ), WIDE("Uhh no hand?!") );

			LineRelease( hand_string );
		}
		//UpdateFrame( pf, 0, 0, 0, 0 );
	}
	return 1;
}

int CPROC TableMouse( PCOMMON psv, S_32 x, S_32 y, _32 b )
{
	// what can I do with a mouse?
	// I can drraw on a frame... I don't need to be a control...
	//lprintf( WIDE("mouse: %d %d %x"), x, y, b);
	if( ( b & MK_LBUTTON ) && !( l._b & MK_LBUTTON ) )
	{
		int s = y / l.step_y;
		int f = x / l.step_x;
		if( s > 3 )
			if( SUS_LT(y, S_32, ((l.step_y * 3) + l.height), _32 ) )
				s = 3;
		if( f > 12 )
			if( SUS_LT(x, S_32, ((l.step_x * 13 ) + l.width ), _32 ) )
            f = 12;
		lprintf( WIDE("s:%d f:%d %p"), s, f, l.cards[s][f] );
		if( ( s < 4 && s >= 0 )
			&&( f >= 0 && f < 13 ) )
		{
			lprintf( WIDE("s:%d f:%d %p"), s, f, l.cards[s][f] );
			if( l.cards[s][f] )
			{
				if( l.nHand < 2 )
				{
               l.hand_id[l.nHand] = s * 13 + f;
               l.hand_card[l.nHand] = l.cards[s][f];
					HandAdd( l.hand, l.cards[s][f] );
					l.cards[s][f] = NULL;
					l.hand_image[l.nHand++] = l.card_image[s][f];
               SmudgeCommon( l.frame );
				}
				else if( l.nTable < 5 )
				{
               l.table_id[l.nTable] = s * 13 + f;
               l.table_card[l.nTable] = l.cards[s][f];
					DeckPlayCard( l.deck, l.cards[s][f] );
					l.cards[s][f] = NULL;
					l.table_image[l.nTable++] = l.card_image[s][f];
               SmudgeCommon( l.frame );
				}
			}
		}
		else if( x > 300 && x < ( 300 + 1 * l.step_x + l.width ) &&
				  y > 10 && y < 10 + l.height )
		{
			if( l.nHand )
			{
            l.nHand--;
				l.cards[0][l.hand_id[l.nHand]] = GrabCard( l.hand_card[l.nHand] );
				SmudgeCommon( l.frame );
			}
		}
      else if( x > 300 && x < ( 300 + 5 * l.step_x ) &&
				  y > 90 && y < 90 + l.height )
		{
			if( l.nTable )
			{
				l.nTable--;
				l.cards[0][l.table_id[l.nTable]] = GrabCard( l.table_card[l.nTable] );
				SmudgeCommon( l.frame );
			}
		}
		// else check if on hand
		// else check if on table
      // else check if on player select
	}
	l._b = b;
   return 1;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
_32 hands;
_32 start_tick;
int nCards;
PCARD hand[5];
int handcount;
int no_output;
PVARTEXT pvt_sql;
int first;
void ProcessHands( int level, int start )
{
	int n;
	if( level == 0 )
	{
      start_tick = GetTickCount();
		if( !no_output )
		{
			pvt_sql = VarTextCreate();
			vtprintf( pvt_sql, WIDE("insert into poker_odds (ID,card1,card2,card3,card4,card5,value,class,name) values ") );
			first = 1;
		}
	}
	handcount++;
	if( ( handcount & 0xFFF ) == 0 )
	{
      _32 tick = GetTickCount();
      lprintf( WIDE("hands %ld in %ld = %ld"), handcount, tick - start_tick, (handcount)/(tick-start_tick));
			start_tick = tick;
         handcount = 0;
	}
	if( level == 5 )
	{
		int value;
      static PTEXT hand_string;
		handcount++;
		value = ValuePokerHand( l.hand, NULL, TRUE );
		//if( ( value = ValuePokerHand( l.hand, NULL, TRUE ) ) >= 0x100000 )
		GetPokerHandName( l.hand, &hand_string );
      /*
		lprintf( WIDE("hand %d,%d,%d,%d,%d,%d,%d,%d,\'%s\'")
				 , handcount
				 , hand[0]->id
				 , hand[1]->id
				 , hand[2]->id
				 , hand[3]->id
				 , hand[4]->id
				 , value
				 , value >> 20
				 , hand_string?GetText( hand_string ):WIDE("unknown")
				 );
      */
		if( !no_output )
		{
			vtprintf( pvt_sql, WIDE("%s(%d,%d,%d,%d,%d,%d,%d,%d,\'%s\')")
					 , first?WIDE(""):WIDE(",")
					 , handcount
					 , hand[0]->id
					 , hand[1]->id
					 , hand[2]->id
					 , hand[3]->id
					 , hand[4]->id
					 , value
					 , value >> 20
					 , hand_string?GetText( hand_string ):WIDE("unknown")
					  );
         first = 0;
		}
		return;
	}
	for( n = start; n < 53; n++ )
	{
		if( n == 52 )
		{
			HandAdd( l.hand, hand[nCards++] = l.wild_cards[l.nWildUsed++] );
		}
      else
			HandAdd( l.hand, hand[nCards++] = l.cards[0][n] );
		if( n == 52 )
			ProcessHands( level+1, n );
      else
			ProcessHands( level+1, n+1 );
		if( !no_output )
			if( level == 2 )
			{
				PTEXT cmd = VarTextGet( pvt_sql );
				DoSQLCommand( GetText( cmd ) );
				LineRelease( cmd );
				vtprintf( pvt_sql, WIDE("insert into poker_odds (ID,card1,card2,card3,card4,card5,value,class,name) values ") );
				first = 1;
			}

      nCards--;
		// take it back out of the hand...
		// when we're done, all cards have been put in
      // and have been taken out of a hand.
		if( n == 52 )
		{
         GrabCard( l.wild_cards[--l.nWildUsed] );
		}
      else
			GrabCard( l.cards[0][n] );
	}
	if( level == 0 )
	{
		{
			_32 tick = GetTickCount();
			lprintf( WIDE("hands %ld in %ld = %ld"), handcount, tick - start_tick, (handcount)/(tick-start_tick));
			start_tick = tick;
         handcount = 0;
		}
		if( !no_output )
		{
			PTEXT cmd = VarTextGet( pvt_sql );
			if( cmd )
			{
				DoSQLCommand( GetText( cmd ) );
				LineRelease( cmd );
				vtprintf( pvt_sql, WIDE("insert into poker_odds (ID,card1,card2,card3,card4,card5,value,class,name) values ") );
				first = 1;
			}
			VarTextDestroy( &pvt_sql );
		}
	}
}

//--------------------------------------------------------------------------


void CreateNames( void )
{
   int n;
	DoSQLCommand( WIDE("Create table if not exists `poker_cards`( card_id int auto_increment, name varchar(32),nicename varchar(32), primary key (card_id) )") );
	for( n = 0; n < 52; n++ )
	{
		PTEXT name;
      PTEXT nicename = CardLongName( l.cards[0][n] );
      TEXTCHAR cmd[256];
		name = CardName( l.cards[0][n] );

		snprintf( cmd, sizeof( cmd ), WIDE("insert into poker_cards (card_id,name,nicename) values (%d,\'%s\',\'%s\')")
				  , n
				  , GetText( name )
              , GetText( nicename )
				  );
		if( !DoSQLCommand( cmd ) )
		{
			snprintf( cmd, sizeof( cmd ), WIDE("update poker_cards set name=\'%s\' where card_id=%d")
					  , GetText( name ), n );
         DoSQLCommand( cmd );
		}
		LineRelease( name );
      LineRelease( nicename );
	}
}

//--------------------------------------------------------------------------

void BuildAlt( void )
{
   TEXTCHAR cmd[512];
	int a,b,c,d,e;
	for( a = 0; a < 52; a++ )
	{
		for( b = a+1; b < 52; b++ )
		{
			snprintf( cmd, sizeof( cmd ), WIDE("insert into poker_odds_2card(hand_id,pair_id,card1,card2) select id,%d,%d,%d ")
					  WIDE("from `poker_odds`")
					  WIDE("where")
						WIDE("( (card1=%d or card2=%d or card3=%d or card4=%d ) and")
						WIDE("  ( card2=%d or card3=%d or card4=%d or card5=%d ) and")
					  WIDE("   ( card1<52 and card2<52 and card3<52 and card4<52 and card5<52) )")
					  , ((a+1) * 100) + (b+1)
                  , a, b
					 , a, a, a, a
					 , b, b, b, b );
			DoSQLCommand( cmd );
		}
	}
}

//--------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	DECLTEXT( name, WIDE("table") );
	//return NULL;
	l.pe = CreateEntityIn( NULL, (PTEXT)&name );
	l.ps = CreateAwareness( l.pe );
	l.deck = CreateDeck( WIDE("Holdem") ,IterateHoldemHand );
	l.hand = CreateHand( l.deck );
	ResetTable();
	{
      int s;
		for( s = 0; s < 5; s++ )
		{
         // get some wild cards for filler
         l.wild_cards[s] = CreateWildCard();
		}
		//CreateNames();
      //no_output = 1;
		//ProcessHands( 0, 0 );
      //BuildAlt();
	}
	// and now what do I do with it?
	l.frame = CreateFrame( WIDE("Cards and stuff"), 0, 0, 800, 600
								, BORDER_WANTMOUSE|BORDER_NORMAL|BORDER_RESIZABLE
								, NULL );
	SetCommonMouse( l.frame, TableMouse );
	AddCommonDraw( l.frame, TableDraw );
	{
		int s, f;
		for( s = 0; s < 4; s++ )
			for( f = 0; f < 13; f++ )
			{
				TEXTCHAR filename[64];
				snprintf( filename, sizeof( filename ), WIDE("images/cards/card%d-%02d.bmp"), s+1, ( 12 + f ) % 13 + 2 );
				l.card_image[s][f] = LoadImageFile( filename );
			}

      GetImageSize( l.card_image[0][0], &l.width, &l.height );
      // arbitrary... could be based off card size...
		l.step_x = 20;
      l.step_y = 40;
	}

   DisplayFrame( l.frame );
   return DekVersion;
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	DestroyFrame( &l.frame );
	DestroyAwareness( l.ps );
	DestroyEntity( l.pe );
	{
		int s, f;
		for( s = 0; s < 4; s++ )
			for( f = 0; f < 13; f++ )
			{
            UnmakeImageFile( l.card_image[s][f] );
			}
	}
   MemSet( &l, 0, sizeof( l ) );
}


/*
 +----------+
| count(*) |
+----------+
|     2240 |
|     1280 |
|     1728 |
|     1792 |
|      448 |
|     1792 |
|      512 |
|      576 |
|       44 |
|      240 |
|      240 |
|      240 |
|      240 |
|      240 |
|      240 |
|      240 |
|     2640 |
|      240 |
|     3088 |
|      368 |
|      384 |
|      400 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|      396 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       18 |
|       10 |
|       10 |
|       10 |
|       10 |
|       10 |
|       10 |
|       10 |
|      264 |
|       10 |
|      265 |
|       10 |
|       10 |
|       10 |
|       64 |
|       64 |
|       64 |
|        9 |
|       18 |
|        8 |
|        6 |
+----------+

 */