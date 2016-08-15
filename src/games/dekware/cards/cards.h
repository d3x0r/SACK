

#ifdef CARDS_SOURCE
#define CARDS_PROC(type,name) EXPORT_METHOD type name
#else
#define CARDS_PROC(type,name) IMPORT_METHOD type name
#endif


#define CARD_NUMBER(id) ( (id) % 13 )
#define CARD_SUIT(id)   ( (id) / 13 ) 
#define CARD_ID(suit,number) ( ((suit)*13) + (number))

struct card_tag;
struct hand_tag;

typedef struct card_tag * (*HandIterator)( struct hand_tag *pHand, int level, int bStart );

#define MAX_ITERATORS 5
#define ITERATE_START   ( 1 )
#define ITERATE_FROM_BASE  ( ITERATE_START + 1 )
// 2 - 2+n
#define ITERATE_FROM(n) ( n + ITERATE_FROM_BASE )
// 7 - 7+n
#define ITERATE_AT_BASE  ( ITERATE_START +(1*MAX_ITERATORS) + 1 )
#define ITERATE_AT(n)   ( n + ITERATE_AT_BASE )
#define ITERATE_NEXT    ( 0 )


typedef struct card_tag
{
	int id;			// 0-51 numeric ID of card....
	int fake_id;   // 0-51 possible ID of the card... avoid duplicates.
	struct {
		uint32_t bDiscard:1; // set for discard scanner to remove from hand...
		uint32_t bFaceDown:1; // a face down card id kinda wild....
	} flags;
	struct hand_tag *pPlayedBy;  // when card is on table, this is what hand it's from...
	struct card_tag *next;
	struct card_tag **me;
} CARD, *PCARD;

struct update_callback
{
	void (CPROC *f)( uintptr_t psv );
	uintptr_t psv;
};


typedef struct card_stack_tag
{
	CTEXTSTR name;
	PCARD cards;
	PLIST update_callbacks;  // struct update_callback(s)
} CARD_STACK, *PCARD_STACK;


typedef struct deck_tag
{
	PTEXT name;
	struct {
		uint32_t bHold : 1; // cards in hand are not played
		uint32_t bLowball : 1; // Lowest hand counts, aces are 1's.
	} flags;
	uint32_t faces;
	uint32_t suits;
	PCARD card; // original array of cards, all cards always originate here.
	PLIST Hands;  // list of entities which have HANDS
	PLIST card_stacks; // named regions that have cards...
	HandIterator _HandIterator;
} DECK, *PDECK;

typedef struct hand_tag
{
	PLIST card_stacks; // named regions that have cards...
	PDECK Deck;    // deck this hand belongs to...
	INDEX Tricks;  // number of cards swept in tricks...
	// some tracking info to make looping states...
	// 5 levels of card iteratation
	INDEX iStage[MAX_ITERATORS]; // counter for which cards's we're iterating through...
	PCARD card[MAX_ITERATORS]; // current card for looping states...

} HAND, *PHAND;

#ifndef DECLARE_DATA 
extern char *cardlongnames[14];
#endif

typedef PCARD (*IterateHand)( PHAND hand, int level, int bStart );

CARDS_PROC( PCARD, CreateCard )( int id );
CARDS_PROC( PTEXT, CardName )( PCARD card );
CARDS_PROC( PTEXT, CardLongName )( PCARD card );

CARDS_PROC( PCARD, CreateWildCard )( void );
// name of iterator...
CARDS_PROC( PDECK, CreateDeck )( CTEXTSTR name, IterateHand iter );
CARDS_PROC( void, DeleteDeck )( PDECK *deck );
CARDS_PROC( PHAND, CreateHand )( PDECK );
CARDS_PROC( void, DeckPlayCard )( PDECK, PCARD );
CARDS_PROC( void, DeckDiscard )( PDECK pDeck, PCARD pCard );
CARDS_PROC( void, HandAdd )( PHAND pHand, PCARD pCard );
CARDS_PROC( void, DeleteHand )( PHAND* );
CARDS_PROC( INDEX, CountCards )( PHAND ph );
CARDS_PROC( PTEXT, ListCards )( PHAND ph );
CARDS_PROC( PTEXT, ListGameCards )( PHAND ph );
CARDS_PROC( void, Shuffle )( PDECK pDeck );
CARDS_PROC( void, GatherCards )( PDECK deck );

// deal the next card on the deck to a hand
// if hand is NULL, then it is dealt to the face-up table.
CARDS_PROC( void, DealTo )( PDECK pd, PHAND ph, LOGICAL bPlayTo );


CARDS_PROC( PTEXT, GetPokerHandName )( PHAND, PTEXT* );

enum poker_hand_class {
	POKER_HAND_SOMETHING_HIGH,
	POKER_HAND_PAIR = 0x100000,
	POKER_HAND_2PAIR = 0x200000,
	POKER_HAND_3KIND= 0x300000,
	POKER_HAND_STRAIGHT = 0x400000,
	POKER_HAND_FLUSH = 0x500000,
	POKER_HAND_FULLHOUSE = 0x600000,
	POKER_HAND_4KIND = 0x700000,
	POKER_HAND_STRAIGHT_FLUSH = 0x800000,
	POKER_HAND_ROYAL_FLUSH = 0x900000,
	POKER_HAND_5KIND = 0xa00000,
   POKER_HAND_MASK = 0xF00000
};
/* Poker Hands....
  Something High  0x000000 // bytes highest to lowest ...
  Pair            0x100000
  2Pair           0x200000  // low nibble = value of card pairs 0x02HL
  3Kind           0x300000    // low nibble = value of card
  Straight        0x400000 // low nibble = high card
  Flush           0x500000 // low nibble = High Card/LowCard
  FullHouse       0x600000 // 3 kind and pair low byte
  4Kind           0x700000 // low nibble = card
  Straight Flush  0x800000 // low nibble = highCard (royal highcard = 13)
  // this value is actually a special straight flush.
  RoyalFlush      0x900000 // low nibble = nothing...
  5 Kind          0xa00000 // low nibble = card value...

  // suits are as follows Spades, Hearts, Diamonds, and Clubs
*/

CARDS_PROC( int, ValuePokerHand )( PHAND pHand, PCARD pWild, int bHigh );

CARDS_PROC( PCARD, IterateHoldemHand )( PHAND hand, int level, int bStart );
CARDS_PROC( PCARD, IterateStudHand )( PHAND hand, int level, int bStart );


CARDS_PROC( PCARD, PickACard )( PDECK deck, int ID );
CARDS_PROC( void, HandAdd )( PHAND, PCARD );
CARDS_PROC( PCARD, GrabCard )( PCARD );

CARDS_PROC( PCARD_STACK, GetCardStack )( PDECK deck, CTEXTSTR name );
CARDS_PROC( PCARD_STACK, GetCardStackFromHand )( PHAND deck, CTEXTSTR name );

CARDS_PROC( void, AddCardStackUpdateCallback )( PCARD_STACK stack, void (CPROC*f)(uintptr_t), uintptr_t psv );
CARDS_PROC( void, RemoveCardStackUpdateCallback )( PCARD_STACK stack, uintptr_t psv );
CARDS_PROC( PCARD, GetNthCard )( PCARD_STACK stack, int nCard );

CARDS_PROC( PCARD, TransferCards )( PCARD_STACK stack_from, PCARD_STACK stack_to, int nCards );
CARDS_PROC( void, TurnTopCard )( PCARD_STACK stack );



// $Log: cards.h,v $
// Revision 1.16  2005/08/08 09:28:43  d3x0r
// Modified default iterator to holdem.(works for draw poker too) Fixed up deleting decks and hands
//
// Revision 1.15  2005/04/25 07:52:57  d3x0r
// Working on the cards project - wow I left lots of loose ends.
//
// Revision 1.14  2004/09/09 13:41:28  d3x0r
// checkpoint
//
// Revision 1.13  2004/08/13 09:27:50  d3x0r
// Extend API a bit to generate wild cards, and dump test data into a database... 30 minutes or so local
//
// Revision 1.12  2004/08/12 09:16:57  d3x0r
// Extend card API a bit... provide a plugin to demo test cards
//
// Revision 1.11  2004/08/11 10:13:50  d3x0r
// Links, stretched cards into self-library
//
// Revision 1.10  2003/09/01 20:10:22  panther
// facedown card option
//
// Revision 1.9  2003/08/05 10:05:49  panther
// Temp stop area - building cards engine itself as a private library.
//
// Revision 1.8  2003/08/04 07:17:03  panther
// Updates to cards engine
//
// Revision 1.7  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.6  2003/08/01 23:53:13  panther
// Updates for msvc build
//
// Revision 1.5  2003/03/25 08:59:01  panther
// Added CVS logging
//
