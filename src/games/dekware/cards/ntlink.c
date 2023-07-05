// well do stuff here....
#include <stdhdrs.h>
#include <time.h>
#define DEFINES_DEKWARE_INTERFACE
#include "plugin.h"

#include "cards.h"
// common DLL plugin interface.....

#if defined( _WIN32 ) || defined( _MSC_VER )
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif
#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

size_t iCardDeck, iHand, iTable;
int CPROC Create(PSENTIENT ps, PENTITY pe, PTEXT parameters);
int CPROC CreateTable(PSENTIENT ps, PENTITY pe, PTEXT parameters);

#ifdef GCC
#define DebugBreak() asm( "int $3\n" );
#endif

//extern int b95;
extern size_t iCardDeck, iHand;


//---------------------------------------------------------------------

PTEXT ShowPlayed( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
	// depends on the game where 'played' comes from.

	return *ppLastValue;
}

//---------------------------------------------------------------------

PTEXT ShowTable( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
	// depends on the game where 'played' comes from.

	return *ppLastValue;
}

static int ObjectMethod( "Cards", "PlayCard", "Take a card from hand or deck and put into play." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int PlayCard( PSENTIENT ps, PTEXT pCard )
{
	PHAND pHand = (PHAND)GetLink( &ps->Current->pPlugin, iHand );
	PDECK pd = (PDECK)GetLink( &ps->Current->pPlugin, iCardDeck );
	if( pHand )
	{
		// play specified card to the table or local table
		// optioned by deck.
	}
	else if( pd )
	{
		// play top card to the table.
		DeckPlayCard( pd, GrabCard( GetCardStack( pd, "Draw" )->cards ) );
	}
	else
	{
		// not a hand or a deck.
	}
   return 0;
}


static int ObjectMethod( "Hand", "PlayCard", "Take a card from hand or deck and put into play." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int PlayCard( PSENTIENT ps, PTEXT pCard )
{
	PHAND pHand = (PHAND)GetLink( &ps->Current->pPlugin, iHand );
	PDECK pd = (PDECK)GetLink( &ps->Current->pPlugin, iCardDeck );
	if( pHand )
	{
		// play specified card to the table or local table
		// optioned by deck.
	}
	else if( pd )
	{
		// play top card to the table.
		DeckPlayCard( pd, GrabCard( GetCardStack( pd, "Draw" )->cards ) );
	}
	else
	{
		// not a hand or a deck.
	}
   return 0;
}

//---------------------------------------------------------------------

//static int ObjectMethod( "Hand", "Discard", "Take card from hand or put into discard." )( PSENTIENT ps, PTEXT params )
static int DoDiscard( PSENTIENT ps, PENTITY pe, PTEXT pCards )
{
   PTEXT pCard;
   PCARD pcCard;
   PLIST pCardList;
	INDEX nCard;
   PHAND ph = (PHAND)GetLink( &ps->Current->pPlugin, iHand );
	if( pCards && ph )
	{
		PTEXT newlist = TextDuplicate( pCards, FALSE );
      if( newlist )
			newlist = FlattenLine( newlist );
      pCards = newlist;
	   pCardList = CreateList();
      while( (pCard = GetParam( ps, &pCards ) ) )
      {
         if( !IsNumber( pCard ) )
         {
            DECLTEXT( msg, "Parameter to discard is not a number..." );
            if( !ps->CurrentMacro )
		         EnqueLink( &ps->Command->Output, &msg );
            continue;
         }
         else
         {
            PCARD pCurrent;
            pCurrent = GetCardStackFromHand( ph, "Cards" )->cards;
            nCard = (INDEX)IntNumber( pCard );
            while( nCard > 1 && pCurrent ) // one based list
            {
               nCard--;
               pCurrent = pCurrent->next;
            }
            if( pCurrent )
            {
            	if( FindLink( &pCardList, pCurrent ) == INVALID_INDEX )
		         	AddLink( &pCardList, pCurrent );
					else
					{
						DECLTEXT( msg, "Selected the same card more than once" );
						EnqueLink( &ps->Command->Output, &msg );
					}
            }
				else
				{
					// this aborts discarding anything if any one card was bad.
		         DECLTEXT( msg, "Invalid card selected..." );
      	      if( !ps->CurrentMacro )
		   	      EnqueLink( &ps->Command->Output, &msg );
					DeleteList( &pCardList );
					return 0;
		   	}
         }
      }
      LIST_FORALL( pCardList, nCard, PCARD, pcCard )
      {
      	Log2( "Discarding %d %d", nCard, pcCard->id );
      	DeckDiscard( ph->Deck, GrabCard( pcCard ) );
      }
		DeleteList( &pCardList );
	}
	else
	{
		PDECK pd = (PDECK)GetLink( &ps->Current->pPlugin, iCardDeck );
		if( pd )
		{
			// no parameters...
			// discards one card.
			DeckDiscard( pd, GrabCard( GetCardStack( pd, "Draw" )->cards ) );
		}
		else
		{
			// not a hand, not a deck...
		}
	}
   return 0;
}

static int ObjectMethod( "Hand", "Discard", "Take card from hand, put into discard." )( PSENTIENT ps, PENTITY pe, PTEXT params )
{
   return DoDiscard( ps, pe, params );
}
static int ObjectMethod( "Cards", "Discard", "Take card from deck, put into discard." )( PSENTIENT ps, PENTITY pe, PTEXT params )
{
   return DoDiscard( ps, pe, params );
}

//---------------------------------------------------------------------

static int ObjectMethod( "Hand", "draw", "Draw a card from the deck." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int DrawACard( PSENTIENT ps, PTEXT pNames )
{
	PHAND ph = (PHAND)GetLink( &ps->Current->pPlugin, iHand );
	if( ph )
	{
		HandAdd( ph, GrabCard( GetCardStack( ph->Deck, "Draw" )->cards ) );
	}
	else
   {
      DECLTEXT( msg, "Cannot draw, object not a hand" );
      EnqueLink( &ps->Command->Output, &msg );
   }
   return 0;
}

//---------------------------------------------------------------------

void EntDeleteHand( PENTITY pe )
{
	PHAND hand = (PHAND)GetLink( &pe->pPlugin, iHand );
   if( hand )
   {
		SetLink( &pe->pPlugin, iHand, NULL );
#if 0
      RemoveMethod( pe, Methods + METH_DISCARD );
		RemoveMethod( pe, Methods + METH_DRAW );
#endif
		Log( "Removing volatile variables from hand" );
#if 0
      RemoveVolatileVariable( pe, Variables );
      RemoveVolatileVariable( pe, Variables + 1 );
      RemoveVolatileVariable( pe, Variables + 2 );
		RemoveVolatileVariable( pe, Variables + 3 );
		RemoveVolatileVariable( pe, Variables + 4 );
		RemoveVolatileVariable( pe, Variables + 5 );
		RemoveVolatileVariable( pe, Variables + 6 );
#endif
		//RemoveVolatileVariable( pe, Variables + 7 ); // rules applies to deck
      DeleteHand( &hand );
   }
}

//---------------------------------------------------------------------

int DealPlayTo( PSENTIENT ps, PTEXT pNames, int bPlayTo )
{
	PTEXT pName, pNameList;
	PVARTEXT pvt;
	PDECK pd;
	pvt = VarTextCreate();
	pd = (PDECK)GetLink( &ps->Current->pPlugin, iCardDeck );
   if( !pd )
   {
      vtprintf( pvt, "%s is not a deck of cards.", GetText( GetName( ps->Current ) ) );
      EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
      VarTextEmpty( pvt );
      return 0;
   }
   while( ( pNameList = GetParam( ps, &pNames ) ) )
   {
      int bLoop = FALSE;
      if( GetIndirect( pNameList ) )
      {
         bLoop = TRUE;
         pNameList = GetIndirect( pNameList );
      }

      for( pName = pNameList; pName; pName = bLoop?NEXTLINE( pName ):NULL )
      {
		  PTEXT next_param = pName;
         PENTITY pe;
         PHAND ph;
         PCARD pc;
         pe = (PENTITY)FindThing( ps, &next_param, ps->Current, FIND_VISIBLE, NULL );
         if( pe )
         {
            if( !(ph = (PHAND)GetLink( &pe->pPlugin, iHand ) ) )
				{
					if( !pe->pControlledBy )
					{
						vtprintf( pvt, "%s is not aware....", GetText( pName ) );
						EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
						break;
					}
					pe->pControlledBy->pToldBy = ps;
					Assimilate( pe, ps, "Hand", NULL );
					ph = (PHAND)GetLink( &pe->pPlugin, iHand );
            }
            pc = GetCardStack( pd, "Draw" )->cards;
            if( pc )
			{
				PCARD *ppDest;
				// unlink from the draw list
				if( GetCardStack( pd, "Draw" )->cards = pc->next )
               		pc->next->me = &GetCardStack( pd, "Draw" )->cards;

				// determine destination to link card to
				if( bPlayTo )
					ppDest = &GetCardStackFromHand( ph, "Showing" )->cards;
				else
					ppDest = &GetCardStackFromHand( ph, "Cards" )->cards;

					// link card into new list.
					if( pc->next = *ppDest )
						pc->next->me = &pc->next;
					pc->me = ppDest;
					*ppDest = pc;

            }
            else
            {
               DECLTEXT( msg, "Deck is out of cards." );
               EnqueLink( &ps->Command->Output, &msg );
               VarTextEmpty( pvt );
               return 0;
            }
         }
         else
         {
            vtprintf( pvt, "Could not see %s to deal to.", GetText( pName ) );
            EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
         }
			pName = next_param;
		}
	}
	VarTextDestroy( &pvt );
	return 0;
}


//---------------------------------------------------------------------

static int ObjectMethod( "Cards", "DealTo", "deal a card to an object." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int DealTo( PSENTIENT ps, PTEXT pNames )
{
   return DealPlayTo( ps, params, FALSE );
}

//---------------------------------------------------------------------

static int ObjectMethod( "Cards", "PlayTo", "deal a card to an object (on table, face up)." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int PlayTo( PSENTIENT ps, PTEXT pNames )
{
   return DealPlayTo( ps, params, TRUE );
}
//---------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "hand", "pokerhand", "Shows name of the current poker hand" )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetPokerHand( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
	return GetPokerHandName( (PHAND)GetLink( &pe->pPlugin, iHand ), ppLastValue );
}

//---------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "hand", "pokervalue", "Shows current poker value of hand (numeric)" )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetPokerValue( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
    if( *ppLastValue )
        LineRelease( *ppLastValue );
    *ppLastValue = SegCreateFromInt(
                                    ValuePokerHand((PHAND)GetLink( &pe->pPlugin, iHand )
                                                   , NULL, TRUE ) );
    return *ppLastValue;
}

//---------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "hand", "cards", "count of cards in hand" )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetHandSize( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
	PHAND ph;
	if( !ppLastValue )
		return NULL;

	LineRelease( *ppLastValue );
	ph = (PHAND)GetLink( &pe->pPlugin, iHand );
	if( !ph ) // this really can't happen anymore.
	{
		DECLTEXT( msg, "Object is not a card player." );
		*ppLastValue = (PTEXT)&msg;
		return *ppLastValue;
	}
	else
	{
		*ppLastValue = SegCreateFromInt( CountCards( ph ) );
	}
	return *ppLastValue;
}

//---------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "hand", "pokercards", "Just list the cards of the hand(full player hand iterator?)" )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetPokerCards( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
   // hmm where to put the output?! so that it can be used in script....
   // Guess Set Result...
   PHAND ph;
   if( !ppLastValue )
   	return NULL;
   LineRelease( *ppLastValue );

   ph = (PHAND)GetLink( &pe->pPlugin, iHand );
   if( !ph )
   {
      DECLTEXT( msg, "Object is not a card player." );
      *ppLastValue = (PTEXT)&msg;
      return *ppLastValue;
   }
   else
   {
      *ppLastValue = ListGameCards( ph );
   }
   return *ppLastValue;
}
//---------------------------------------------------------------------


static PTEXT ObjectVolatileVariableGet( "hand", "ShowHand", "Shows the cards in the current hand" )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT ShowHand( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
   // hmm where to put the output?! so that it can be used in script....
   // Guess Set Result...
   PHAND ph;
   if( !ppLastValue )
   	return NULL;
   LineRelease( *ppLastValue );

   ph = (PHAND)GetLink( &pe->pPlugin, iHand );
   if( !ph )
   {
      DECLTEXT( msg, "Object is not a card player." );
      *ppLastValue = (PTEXT)&msg;
      return *ppLastValue;
   }
   else
   {
      *ppLastValue = ListCards( ph );
   }
   return *ppLastValue;
}

//---------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "cards", "Rules", "Shows the current rules of deck" )( PENTITY pe, PTEXT *ppLastValue )
//PTEXT GetDeckRules( uintptr_t psv, PENTITY pe, PTEXT *ppLastValue )
{
	PVARTEXT pvt = VarTextCreate();
   VarTextDestroy( &pvt );
	return NULL;
}

static PTEXT ObjectVolatileVariableSet( "cards", "Rules", "Shows the current rules of deck" )( PENTITY pe, PTEXT pNewValue )
//PTEXT SetDeckRules( uintptr_t psv, PENTITY pe, PTEXT pNewValue )
{
	// /deck/decl rules poker holdem
	// possible deck rules
	//   play (need to number cards)
	//   hold (no need to number cards)
	// possible game sets
	//   draw 5  draw 5
	//   holdem 
	//   stud
	//   omaha
	//    - more conventional games
	//   spades
	//   hearts
	//   pinnocle
	//    - single player games
	//   klondike
	//   4 corners
	//   accordian
	//   ...

	// 
	// possible variations on some games..
	//   high (default most games)
	//   low  (if alone, will be low only, high and low need to be 
	//         specified together to get that to work)
	//   wild (card)
	//  
	PDECK pd;
	if( ( pd = (PDECK)GetLink( &pe->pPlugin, iCardDeck ) ) )
	{
		PTEXT word;
		for( word = pNewValue; word; word = NEXTLINE( word ) )
		{
			if( OptionLike( word, "hold" ) )
			{	
				pd->flags.bHold = TRUE;
			}
			else if( OptionLike( word, "play" ) )
			{
				pd->flags.bHold = FALSE;
			}
		}
		LineRelease( pNewValue ); // don't actually store this...
	}
	else
	{
		// isn't a deck...
	}
   return NULL;
}



//---------------------------------------------------------------------

static int ObjectMethod( "Cards", "shuffle", "Gather and shuffle all cards." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int DoShuffle( PSENTIENT ps, PTEXT params )
{
   PDECK pd;
   pd = (PDECK)GetLink( &ps->Current->pPlugin, iCardDeck );
   if( !pd )
   {
      DECLTEXT( msg, "Object is not a deck of cards." );
      EnqueLink( &ps->Command->Output, &msg );
      return 1;
   }
   else
      Shuffle( pd );
   return 0;
}

//---------------------------------------------------------------------

void EntDeleteDeck( PENTITY pe )
{
	PDECK deck;
	deck = (PDECK)GetLink( &pe->pPlugin, iCardDeck );
	//RemoveVolatileVariable( pe, Variables + 2 ); //played
	//RemoveVolatileVariable( pe, Variables + 3 ); //table
	//RemoveVolatileVariable( pe, Variables + 7 ); // rules
	DeleteDeck( &deck );
	SetLink( &pe->pPlugin, iCardDeck, NULL );
}

//---------------------------------------------------------------------

static int ObjectMethod( "Cards", "status", "Show status of the deck." )( PSENTIENT ps, PENTITY pe, PTEXT params )
//int DoStatus( PSENTIENT ps, PTEXT pNothing )
{
   int CardsLeft = 0;
   int CardsDealt = 0;
   int HandsDealt = 0;
   int CardsPlayed = 0;
   int CardsGathered = 0;
   int CardsOut = 0;
   PDECK pd;
   //PENTITY pe;
   PHAND hand;
   PCARD pc;
   INDEX idx;
	PVARTEXT pvt;
   pd = (PDECK)GetLink( &ps->Current->pPlugin, iCardDeck );
   if( !pd )
   {
      DECLTEXT( msg, "Entity is not a card deck??" );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   for( pc = GetCardStack( pd, "Draw" )->cards; pc; pc=pc->next )
      CardsLeft++;
   for( pc = GetCardStack( pd, "Discard" )->cards; pc; pc = pc->next )
      CardsOut++;
   for( pc = GetCardStack( pd, "Table" )->cards; pc; pc=pc->next )
		CardsPlayed++;

	LIST_FORALL( pd->Hands, idx, PHAND, hand )
	{
		for( pc = GetCardStackFromHand( hand, "Cards" )->cards; pc; pc = pc->next )
			CardsDealt++;
		for( pc = GetCardStackFromHand( hand, "Gathered" )->cards; pc; pc = pc->next )
			CardsGathered++;
		HandsDealt++;
	}
	pvt = VarTextCreate();
   vtprintf( pvt, "Cards Left: %d  Played: %d  Out: %d  Dealt: %d  Gathered: %d  Hands: %d",
                     CardsLeft, CardsPlayed, CardsOut, CardsDealt, CardsGathered, HandsDealt );
	if( ps->CurrentMacro )
		ps->pLastResult = VarTextGet( pvt );
	else
	   EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
	VarTextDestroy( &pvt );
   return 0;
}

//---------------------------------------------------------------------

typedef struct table_tag
{
	// ruleset...
	// a deck of cards probably
	// carry to an extreme and use object
	// properties? or embed direct structures
	// here to alias methods... well depends on
	// if you can /table/deal ... which is inheritted
	// from

	// cards that are on the table...
   // embed a deck of cards within...
   PCARD cards;
} TABLE, *PTABLE;

DECLTEXT( TableDesc, "Game table." );

static int OnCreateObject( "Table", "A table for cards - joins players, and a deck of cards with a common play area" )( PSENTIENT ps, PENTITY pe, PTEXT parameters )
//int CPROC CreateTable( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
   //PTABLE table = New( TABLE );
	//PSENTIENT ps;
   pe->pDescription = (PTEXT)&TableDesc;
	SetLink( &pe->pPlugin, iCardDeck, CreateDeck( "Stud", /*IterateStudHand*/IterateHoldemHand ) );
   ps = CreateAwareness( pe );
	{
		PVARTEXT pvt = VarTextCreate();
      PTEXT object, temp;
		vtprintf( pvt, "deck of cards standard" );
		object = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
      // resulting object should be created within this one.
		CreateRegisteredObject( ps, object );
		//LineRelease( cmd );
		//VarTextDestroy( cmd );
	}
	UnlockAwareness( ps );	

   return 0;
}

//---------------------------------------------------------------------

static int OnCreateObject( "Cards", "A deck of cards" )( PSENTIENT ps, PENTITY pe, PTEXT parameters )
//int CPROC Create( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
    //PCARD card;
    //PDECK pd;
    //int i;
	if( GetLink( &pe->pPlugin, iCardDeck ) )
	{
      S_MSG( ps, "Entity %s is already a Cards", GetText( GetName( pe ) ) );
      return 0;
	}
    {
		 PSENTIENT psNew;
       PTEXT tmp = GetParam( ps, &parameters );
		 SetLink( &pe->pPlugin, iCardDeck, CreateDeck( tmp?GetText(tmp):"Stud", /*IterateStudHand*/IterateHoldemHand ) );
		 // this may not have to be made aware....
		 // creating the awareness on an entity already away will result NULL
		 psNew = pe->pControlledBy;
       if( !psNew )
			 psNew = CreateAwareness( pe );
        //AddMethod( pe, Methods + METH_SHUFFLE );
        //AddMethod( pe, Methods + METH_DEALTO );
        //AddMethod( pe, Methods + METH_PLAYTO );
        //AddMethod( pe, Methods + METH_PLAYCARD );
		 //AddMethod( pe, Methods + METH_STATUS );
#if 0
		  AddVolatileVariable( pe, Variables + 2, 0 ); //played
		  AddVolatileVariable( pe, Variables + 3, 0 ); //table
		  AddVolatileVariable( pe, Variables + 7, 0 ); // rules
#endif
		  SetLink( &pe->pDestroy, iCardDeck, EntDeleteDeck );
        WakeAThread( psNew );
    }
    return 0;
}

static int OnCreateObject( "Hand", "A player hand of cards" )(PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	if( !ps )
      return 1;
	if( !GetLink( &pe->pPlugin, iHand ) )
	{
		PSENTIENT ps_deck = NULL;//ps->pToldBy;
		if( !ps_deck )
			ps_deck = ps;
		if( ps_deck )
		{
			PHAND ph;
			PDECK pd = (PDECK)GetLink( &ps_deck->Current->pPlugin, iCardDeck );
			if( pd )
			{
				SetLink( &pe->pPlugin, iHand, ph = CreateHand( pd ) );
				SetLink( &pe->pDestroy, iHand, EntDeleteHand );
			}
		}
		else
		{
			DECLTEXT( msg, "Only a deck of cards can cause assimilation of a hand..." );
			EnqueLink( &ps->Command->Output, &msg );
		}
		return 0;
	}
	return 1;
}


PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
	srand( (int)time( NULL ) );
	//RegisterObject( "Cards", "Generic Card Deck... parameters determine type", Create );
	//RegisterObject( "Table",GetText( (PTEXT)&TableDesc), CreateTable );
   iTable = RegisterExtension( "card table" );
   iCardDeck = RegisterExtension( "card deck" );
   iHand = RegisterExtension( "card hand" );

	//return DekVersion;
	}
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterObject( "Table" );
	UnregisterObject( "Cards" );
}
// $Log: ntlink.c,v $
// Revision 1.20  2005/08/08 13:49:26  d3x0r
// Fix flattening parameters for discard
//
// Revision 1.19  2005/08/08 10:57:07  d3x0r
// Fix calculation of highcard to include all cards so KQJT5 and KQJT4 compare different.... also include name of high card A and change name of low card ace to 1
//
// Revision 1.18  2005/08/08 10:09:44  d3x0r
// Fix handling of rules... and initialize .bHold rule flag to off.  If Hold rule applies - cards report numbered.
//
// Revision 1.17  2005/08/08 09:28:43  d3x0r
// Modified default iterator to holdem.(works for draw poker too) Fixed up deleting decks and hands
//
// Revision 1.16  2005/04/25 07:52:57  d3x0r
// Working on the cards project - wow I left lots of loose ends.
//
// Revision 1.15  2005/04/20 06:19:59  d3x0r
// Okay a massive refit to 'FindThing' To behave like GetParam(SubstToken) to handle count.object references better.
//
// Revision 1.14  2005/02/23 20:41:32  d3x0r
// Change registred function def
//
// Revision 1.13  2004/08/11 10:13:50  d3x0r
// Links, stretched cards into self-library
//
// Revision 1.12  2004/04/05 23:00:52  d3x0r
// Progress...
//
// Revision 1.11  2003/08/20 08:05:48  panther
// Fix for watcom build, file usage, stuff...
//
// Revision 1.10  2003/08/05 10:05:49  panther
// Temp stop area - building cards engine itself as a private library.
//
// Revision 1.9  2003/03/25 08:59:01  panther
// Added CVS logging
//
