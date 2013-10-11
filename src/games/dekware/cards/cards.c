#include <stdhdrs.h>
#include <stdlib.h>
#include <logging.h>
#define PLUGIN_MODULE
#include "plugin.h"

#define DECLARE_DATA 
#include "cards.h"

#define SUITS 4

#include <procreg.h>
#undef StrDup
#define StrDup(s) SaveNameConcatN( s, NULL ); /*if( strcmp( s, WIDE("Table") ) == 0 )DebugBreak();*/


static TEXTCHAR *suits[SUITS] = { WIDE("H"), WIDE("C"), WIDE("D"), WIDE("S") };
static TEXTCHAR *suits2[SUITS] = { WIDE("h"), WIDE("c"), WIDE("d"), WIDE("s") };
TEXTCHAR *suitlongnames[SUITS] = { WIDE("Hearts"), WIDE("Clubs"), WIDE("Diamonds"), WIDE("Spades") };

// include A for name of high card ace
// and 1 for name of low card ace
static TEXTCHAR *cardnames[14] = { WIDE("1"), WIDE("2"), WIDE("3")
                      , WIDE("4"), WIDE("5"), WIDE("6")
                      , WIDE("7"), WIDE("8"), WIDE("9")
							 , WIDE("10"), WIDE("11"), WIDE("12"), WIDE("13"), WIDE("A") };

static TEXTCHAR *cardnames_char[14] = { WIDE("1"), WIDE("2"), WIDE("3")
                      , WIDE("4"), WIDE("5"), WIDE("6")
                      , WIDE("7"), WIDE("8"), WIDE("9")
							 , WIDE("T"), WIDE("J"), WIDE("Q"), WIDE("K"), WIDE("A") };

TEXTCHAR *cardlongnames[14] = { WIDE("1"), WIDE("2"), WIDE("3")
                          , WIDE("4"), WIDE("5"), WIDE("6")
                          , WIDE("7"), WIDE("8"), WIDE("9")
                          , WIDE("Ten"), WIDE("Jack"), WIDE("Queen"), WIDE("King"), WIDE("Ace") };

static struct {
	struct {
		BIT_FIELD ha : 1;
	} flags;
   PLIST decks;
} l;

//---------------------------------------------------------------------

PCARD_STACK GetCardStack( PDECK deck, CTEXTSTR name )
{
	INDEX idx;
	PCARD_STACK stack;
	if( !deck || !name )
	{
      if( !deck )
			lprintf( WIDE("no deck...") );
      if( !name )
			lprintf( WIDE("no name...") );
		return NULL;
	}
	LIST_FORALL( deck->card_stacks, idx, PCARD_STACK, stack )
	{
		if( StrCmp( stack->name, name ) == 0 )
         break;
	}
	if( !stack )
	{
		stack = New( CARD_STACK );
		stack->cards = NULL;
		stack->name = StrDup( name );
      stack->update_callbacks = NULL;
      AddLink( &deck->card_stacks, stack );
	}
   return stack;

}

//---------------------------------------------------------------------

PCARD_STACK GetCardStackFromHand( PHAND hand, CTEXTSTR name )
{
	INDEX idx;
	PCARD_STACK stack;
	if( !hand )
      return NULL;
	LIST_FORALL( hand->card_stacks, idx, PCARD_STACK, stack )
	{
		if( StrCmp( stack->name, name ) == 0 )
         break;
	}
	if( !stack )
	{
		stack = New( CARD_STACK );
		stack->cards = NULL;
		stack->name = StrDup( name );
      stack->update_callbacks = NULL;
      AddLink( &hand->card_stacks, stack );
	}
   return stack;

}

//---------------------------------------------------------------------

void AddCardStackUpdateCallback( PCARD_STACK stack, void (CPROC*f)(PTRSZVAL), PTRSZVAL psv )
{
	if( !stack )
		return;
	{
	struct update_callback *callback = New( struct update_callback );
	callback->f = f;
   callback->psv = psv;
	AddLink( &stack->update_callbacks, callback );
	}
}

//---------------------------------------------------------------------

void RemoveCardStackUpdateCallback( PCARD_STACK stack, PTRSZVAL psv )
{
   INDEX idx;
	struct update_callback *callback;
	if( !stack )
      return;
	LIST_FORALL( stack->update_callbacks, idx, struct update_callback *, callback )
	{
		if( callback->psv == psv )
		{
			SetLink( &stack->update_callbacks, idx, NULL );
			Release( callback );
         break;
		}
	}
}

//---------------------------------------------------------------------


void InvokeUpdates( PCARD_STACK stack )
{
	INDEX idx;
	struct update_callback *callback;
	LIST_FORALL( stack->update_callbacks, idx, struct update_callback *, callback )
	{
      callback->f( callback->psv );
	}
}

//---------------------------------------------------------------------

PCARD PickACard( PDECK deck, int ID )
{
	// no matter where the card is, this can grab it.
   //lprintf( WIDE("Uhmm %p %p %d"), deck, deck->card, ID );
	GrabCard( deck->card + ID );
   return deck->card + ID;
}

//---------------------------------------------------------------------

PDECK CreateDeck( CTEXTSTR card_iter_name, IterateHand iter )
{
	PDECK deck = NULL;
	if( iter )
	{
	}
	if( card_iter_name )
	{
		INDEX idx;
		LIST_FORALL( l.decks, idx, PDECK, deck )
		{
			if( TextLike( deck->name, card_iter_name ) )
			{
				break;
			}
		}
	}
	//lprintf( WIDE("Creating a deck ? %s"), card_iter_name );
	if( !deck )
	{
		unsigned int i;
		deck = New( DECK );
		deck->name = SegCreateFromText( card_iter_name );
		deck->flags.bHold = FALSE;
		deck->card_stacks = NULL;
		deck->faces = 13;
		deck->suits = 4;
		deck->Hands = NULL;
		deck->_HandIterator = iter;

		// allocate all cards on the draw pile.
		deck->card = NewArray( CARD, deck->faces * deck->suits );
		{
			PCARD_STACK draw = GetCardStack( deck, WIDE("Draw") );
			for( i = 0; i < deck->faces * deck->suits; i++ )
			{
				// create cards, put all in the draw deck.
				deck->card[i].id = i;
				if( deck->card[i].next = draw->cards )
					deck->card[i].next->me = &deck->card[i].next;
				deck->card[i].me = &draw->cards;
				draw->cards = &deck->card[i];
            draw->cards->flags.bFaceDown = TRUE;
			}
			InvokeUpdates( draw );
		}
		AddLink( &l.decks, deck );
	}
	return deck;
}

//---------------------------------------------------------------------

CARDS_PROC( PCARD, CreateWildCard )( void )
{
	PCARD pc = New( CARD );
	MemSet( pc, 0, sizeof( CARD ) );
	pc->id = 52; // special case wild card...
	return pc;
}

//---------------------------------------------------------------------

CARDS_PROC( PHAND, CreateHand )( PDECK deck )
{
	PHAND ph;
	ph = New( HAND );
	MemSet( ph, 0, sizeof( HAND ) );
	AddLink( &deck->Hands, ph );
	ph->Deck = deck;
	return ph;
}

//---------------------------------------------------------------------

CARDS_PROC( void, Discard )( PDECK pDeck, PCARD *pCards )
{
	PCARD pLast;
	PCARD_STACK discard = GetCardStack( pDeck, WIDE("Discard") );
	if( !pCards ) // don't blow up
		return;
	if( pCards == &discard->cards ) // might be able to discard from discard to discard... so, don't.
		return;
	pLast = (*pCards);
	if( !pLast )
		return; // nothing to discard
	while( pLast->next )
		pLast = pLast->next;
	if( discard->cards )
		discard->cards->me = &pLast->next;
	pLast->next = discard->cards;
	(discard->cards = (*pCards))->me = &discard->cards;

	(*pCards) = NULL;
}

//---------------------------------------------------------------------

CARDS_PROC( PCARD, GrabCard )( PCARD pCard )
{
 	// isolates this card from the hand...
	if( pCard && pCard->me )
	{
		if( *pCard->me = pCard->next )
			pCard->next->me = pCard->me;
		pCard->me = NULL;
		pCard->next = NULL;
	}
	return pCard;
}

//---------------------------------------------------------------------

// discard this card only
CARDS_PROC( void, DeckDiscard )( PDECK pDeck, PCARD pCard )
{
	if( !pCard || !pDeck )
		return;
	{

		PCARD_STACK discard = GetCardStack( pDeck, WIDE("Discard") );
		// wth am I doing?
		GrabCard( pCard );
		if( ( pCard->next = discard->cards ) )
			discard->cards->me = &pCard->next;

		pCard->me = &discard->cards;
		discard->cards = pCard;
		InvokeUpdates( discard );
	}
}

//---------------------------------------------------------------------

int MoveCards( PCARD *ppStackTo, PCARD *ppStackFrom )
{
	PCARD pLast;
	if( ppStackTo == ppStackFrom ) // don't move from a stack into the same stack.
		return 0;
	if( !ppStackTo || !ppStackFrom )
		return 0;
	pLast = (*ppStackFrom);
	if( !pLast )
		return 0; // nothing to discard
	while( pLast->next )
		pLast = pLast->next;
	if( (*ppStackTo) )
		(*ppStackTo)->me = &pLast->next;
	pLast->next = (*ppStackTo);
	((*ppStackTo) = (*ppStackFrom))->me = ppStackTo;
	(*ppStackFrom) = NULL;
	return 1;
}

//---------------------------------------------------------------------

void MoveCard( PCARD *ppStackTo, PCARD *ppCard )
{
	PCARD next;
	if( !ppStackTo || !ppCard || !(*ppCard) )
		return;
	if( (*ppStackTo) )
		(*ppStackTo)->me = &(*ppCard)->next;
	next = (*ppCard)->next;
	(*ppCard)->next = (*ppStackTo);
	((*ppStackTo) = (*ppCard))->me = ppStackTo;
	(*ppCard) = next;
}

//---------------------------------------------------------------------

PCARD GetNthCard( PCARD_STACK stack, int nCard )
{
	PCARD card = stack->cards;
	int n;
	for( n = 0; n < nCard && card; n++ )
	{
		card = card->next;
	}
	return card;
}

PCARD TransferCards( PCARD_STACK stack_from, PCARD_STACK stack_to, int nCards )
{
	int n;
	PCARD cards = NULL;
	PCARD card;
	for( n = 0; n < nCards; n++ )
	{
		card = GrabCard( stack_from->cards );
		if( card )
		{
			card->next = cards;
			card->me = &cards;
			cards = card;
		}
	}
	while( card = cards )
	{
		if( (*card->me) = card->next )
			card->next->me = card->me;
		if( card->next = stack_to->cards )
			stack_to->cards->me = &card->next;
		card->me = &stack_to->cards;
		stack_to->cards = card;
	}
	return NULL;
}

//---------------------------------------------------------------------

void TurnTopCard( PCARD_STACK stack )
{
	if( stack && stack->cards )
		stack->cards->flags.bFaceDown = FALSE;
	InvokeUpdates( stack );
}

//---------------------------------------------------------------------

void DeckPlayCard( PDECK pDeck, PCARD pCard )
{
	PCARD_STACK stack = GetCardStack( pDeck, WIDE("Table") );
	if( stack )
	{
		MoveCard( &stack->cards, &pCard );
		InvokeUpdates( stack );
	}
}

//---------------------------------------------------------------------

void HandAdd( PHAND pHand, PCARD pCard )
{
	if( !pHand || !pCard )
		return;
	if( pCard == GetCardStackFromHand( pHand, WIDE("Cards") )->cards )
	{
		DebugBreak();
		lprintf( WIDE("FATALITY!") );
	}
	if( pCard->next = GetCardStackFromHand( pHand, WIDE("Cards") )->cards )
		pCard->next->me = &pCard->next;
	pCard->me = &GetCardStackFromHand( pHand, WIDE("Cards") )->cards;
	GetCardStackFromHand( pHand, WIDE("Cards") )->cards = pCard;
}

//---------------------------------------------------------------------

void DeleteHand( PHAND *ppHand )
{
	INDEX i;
	if( ppHand )
	{
		PHAND hand = (*ppHand );
		{
			INDEX idx;
			PCARD_STACK stack;
			LIST_FORALL( hand->card_stacks, idx, PCARD_STACK, stack )
			{
				Discard( hand->Deck, &stack->cards );
            Release( stack );
			}
		}
		InvokeUpdates( GetCardStack( hand->Deck, WIDE("Discard") ) );

		i = FindLink( &hand->Deck->Hands, hand );
		SetLink( &hand->Deck->Hands, i, NULL );
		Release( hand );
		(*ppHand) = NULL;
	}
}

void DeleteDeck( PDECK *ppDeck )
{
	PDECK deck;
	//PENTITY peHand;
	//INDEX idx;
	//PCARD pc, pnext;
	if( ppDeck )
	{
		deck = (*ppDeck );
		if( deck )
		{
			PHAND pHand;
			INDEX hand;
			LIST_FORALL( deck->Hands, hand, PHAND, pHand )
				DeleteHand( &pHand );
			DeleteList( &deck->Hands );
			Release( deck->card );
			// already have all cards from all hands...
			//GatherCards( deck ); // puts them all in the draw stack for easy deletion
			//for( pc = GetCardStack( deck, WIDE("Draw") )->cards; pc; pc= pnext )
			//{
			//	pnext = pc->next;
			//	Release( pc );
			//}
			Release( *ppDeck );
			(*ppDeck) = NULL;
		}
	}
}
//---------------------------------------------------------------------

void GatherCards( PDECK deck )
{
	INDEX hand;
	PCARD pc;
	PHAND pHand;
	PCARD_STACK Draw = GetCardStack( deck, WIDE("Draw") );

	LIST_FORALL( deck->Hands, hand, PHAND, pHand )
	{
		INDEX idx;
		PCARD_STACK stack;
		LIST_FORALL( pHand->card_stacks, idx, PCARD_STACK, stack )
		{
			if( MoveCards( &Draw->cards, &stack->cards ) )
				InvokeUpdates( stack );
		}
	}


	{
		INDEX idx;
		PCARD_STACK stack;
		LIST_FORALL( deck->card_stacks, idx, PCARD_STACK, stack )
		{
			if( stack == Draw )
				continue;
			// returns false on failure of no actual update done.
			if( MoveCards( &Draw->cards, &stack->cards ) )
				InvokeUpdates( stack );
		}
		//InvokeUpdates( Draw );
	}
	{
		unsigned n;
		for( n = 0; n < deck->faces*deck->suits; n++ )
			deck->card[n].flags.bFaceDown = TRUE;
	}
}

//---------------------------------------------------------------------

int CountCards( PHAND ph )
{
   PCARD pc;
   INDEX count;
   if( !ph )
      return 0;
   for( count = 0, pc = GetCardStackFromHand( ph, WIDE("Cards") )->cards; pc; pc = pc->next, count++ );
   return count;
}

//---------------------------------------------------------------------

PTEXT ListCards( PHAND ph )
{
	PCARD pc;
	PVARTEXT pvt;
	PTEXT pt;
	INDEX count;
	PDECK pDeck = ph->Deck;
	if( !ph )
		return NULL;
	pvt = VarTextCreate();
	for( count = 0, pc = GetCardStackFromHand( ph, WIDE("Cards") )->cards; pc; pc = pc->next, count++ );
	if( count )
	{
		if( !pDeck->flags.bHold )
		{
			for( count = 1, pc = GetCardStackFromHand( ph, WIDE("Cards") )->cards; pc; pc = pc->next, count++ )
			{
				int cardval = CARD_NUMBER( pc->id );
				if( !pDeck->flags.bLowball && !cardval )
					cardval = 13;
		      vtprintf( pvt, WIDE("%s%s%s"), count>1?WIDE(","):WIDE("")
				                       , cardnames[cardval]
					                    , suits2[pc->id/13]);
			}
		}
		else for( count = 1, pc = GetCardStackFromHand( ph, WIDE("Cards") )->cards; pc; pc = pc->next, count++ )
		{
			int cardval = CARD_NUMBER( pc->id );
			if( !pDeck->flags.bLowball && !cardval )
				cardval = 13;
			vtprintf( pvt, WIDE("%s%ld:%s%s"), count>1?WIDE(","):WIDE("")
                                      , count
                                      , cardnames[cardval]
                                      , suits2[pc->id/13]);
		}
	}
	else
	{
		vtprintf( pvt, WIDE("No cards.") );
	}
	pt = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	return pt;
}


//---------------------------------------------------------------------

// uses the current game iterator to get the cards
// in this list...
PTEXT ListGameCards( PHAND ph )
{
	PCARD pc;
	PVARTEXT pvt;
	PTEXT pt;
	INDEX count;
	PDECK pDeck = ph->Deck;
	if( !ph )
		return NULL;
	pvt = VarTextCreate();
	for( count = 0, pc = GetCardStackFromHand( ph, WIDE("Cards") )->cards; pc; pc = pc->next, count++ );
	if( count )
	{
		for( count = 1, pc = pDeck->_HandIterator( ph, 0, TRUE );
			 pc;
			  pc = pDeck->_HandIterator( ph, 0, FALSE ), count++ )
		{
			int cardval = CARD_NUMBER( pc->id );
			if( !pDeck->flags.bLowball && !cardval )
				cardval = 13;
			vtprintf( pvt, WIDE("%s%s%s"), count>1?WIDE(", "):WIDE("")
					  , cardnames[cardval]
					  , suits2[pc->id/13]);
		}
	}
	else
	{
		vtprintf( pvt, WIDE("No cards.") );
	}
	pt = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	return pt;
}




//---------------------------------------------------------------------

typedef struct holder_tag
{
   PCARD card;
   int r;
   struct holder_tag *pLess, *pMore;
} HOLDER, *PHOLDER;

PHOLDER sort( PHOLDER tree, PCARD card, int r )
{
   if( !tree )
   {
      tree = (PHOLDER)Allocate( sizeof(HOLDER) );
      tree->card = card;
      tree->r = r;
      tree->pLess = tree->pMore = NULL;
   }
   else
   {
      if( r > tree->r )
         tree->pMore = sort( tree->pMore, card, r );
      else
         tree->pLess = sort( tree->pLess, card, r );
   }
   return tree;
}

void FoldTree( PCARD *pStack, PHOLDER tree )
{
	if( tree->pLess )
		FoldTree( pStack, tree->pLess );
	if( tree->card->next = *pStack )
		(*pStack)->me = &tree->card->next;
	tree->card->me = pStack;

	*pStack = tree->card;
	if( tree->pMore )
		FoldTree( pStack, tree->pMore );
	Release( tree );
}

void Shuffle( PDECK pDeck )
{
	PHOLDER tree;
	PCARD card;
	GatherCards( pDeck );

	tree = NULL;

	while( card = GetCardStack( pDeck, WIDE("Draw") )->cards )
	{
   		if( *card->me = card->next )
   			card->next->me = card->me;
   		card->next = NULL;
		card->me = NULL;
		tree = sort( tree, card, rand() );
	}

	FoldTree( &GetCardStack( pDeck, WIDE("Draw") )->cards, tree );
	InvokeUpdates( GetCardStack( pDeck, WIDE("Draw") ) );

}


//---------------------------------------------------------------------

PTEXT CardName( PCARD card )
{
	PVARTEXT pvt = VarTextCreate();
	PTEXT name;
	vtprintf( pvt, WIDE("%s%s")
			  , cardnames[card->id%13]
			  , suits2[card->id/13]);
	name = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	return name;
}

//---------------------------------------------------------------------

PTEXT CardLongName( PCARD card )
{
	PVARTEXT pvt = VarTextCreate();
	PTEXT name;
	vtprintf( pvt, WIDE("%s of %s")
			  , cardlongnames[(card->id%13)?(card->id%13):13]
			  , suitlongnames[card->id/13]);
	name = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	return name;
}


void DealTo( PDECK pd, PHAND ph, LOGICAL bPlayTo )
{
	PCARD pc = GetCardStack( pd, WIDE("Draw") )->cards;
	if( pc )
	{
		PCARD *ppDest;
		// unlink from the draw list
		if( GetCardStack( pd, WIDE("Draw") )->cards = pc->next )
			pc->next->me = &GetCardStack( pd, WIDE("Draw") )->cards;

		if( !ph )
		{
			if( bPlayTo )
				ppDest = &GetCardStack( pd, WIDE("Table") )->cards;
			else
				ppDest = &GetCardStack( pd, WIDE("Discard") )->cards;
		}
		else
		{
			// determine destination to link card to
			if( bPlayTo )
				ppDest = &GetCardStackFromHand( ph, WIDE("Showing") )->cards;
			else
				ppDest = &GetCardStackFromHand( ph, WIDE("Cards") )->cards;
		}
		// link card into new list.
		if( pc->next = *ppDest )
			pc->next->me = &pc->next;
		pc->me = ppDest;
		*ppDest = pc;
	}
}

//--------------------------------------------------------------------------
// $Log: cards.c,v $
// Revision 1.27  2005/08/08 10:57:07  d3x0r
// Fix calculation of highcard to include all cards so KQJT5 and KQJT4 compare different.... also include name of high card A and change name of low card ace to 1
//
// Revision 1.26  2005/08/08 10:09:43  d3x0r
// Fix handling of rules... and initialize .bHold rule flag to off.  If Hold rule applies - cards report numbered.
//
// Revision 1.25  2005/08/08 09:28:42  d3x0r
// Modified default iterator to holdem.(works for draw poker too) Fixed up deleting decks and hands
//
// Revision 1.24  2005/04/25 07:52:56  d3x0r
// Working on the cards project - wow I left lots of loose ends.
//
// Revision 1.23  2004/09/09 13:41:28  d3x0r
// checkpoint
//
// Revision 1.22  2004/08/13 09:27:50  d3x0r
// Extend API a bit to generate wild cards, and dump test data into a database... 30 minutes or so local
//
// Revision 1.21  2004/08/12 09:16:57  d3x0r
// Extend card API a bit... provide a plugin to demo test cards
//
// Revision 1.20  2004/08/11 10:13:50  d3x0r
// Links, stretched cards into self-library
//
// Revision 1.19  2003/09/01 20:10:22  panther
// facedown card option
//
// Revision 1.18  2003/08/05 10:05:49  panther
// Temp stop area - building cards engine itself as a private library.
//
// Revision 1.17  2003/08/04 07:17:03  panther
// Updates to cards engine
//
// Revision 1.16  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.15  2003/08/01 23:53:13  panther
// Updates for msvc build
//
// Revision 1.14  2003/08/01 02:36:18  panther
// Updates for watcom...
//
// Revision 1.13  2003/04/02 06:43:26  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.12  2003/01/22 11:09:50  panther
// Cleaned up warnings issued by Visual Studio
//
// Revision 1.11  2002/10/27 19:17:11  panther
// work on come cards, some makefiles...
//
// Revision 1.10  2002/10/09 13:14:57  panther
// Changes and tweaks - show volatile variables now...
// cards nearly evaluates all hands - straight is still iffy.
// Fixed support for macros to have descriptions.
//
//
