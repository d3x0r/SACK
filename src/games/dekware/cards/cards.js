

function CARD_NUMBER(id) {return ( (id) % 13 ); }
function CARD_SUIT(id)  {return  ( (id) / 13 ) ; }
function CARD_ID(suit,number) {return ( ((suit)*13) + (number)); }


//typedef struct card_tag * 
//    ϕ(HandIterator)( struct hand_tag *pHand
//                     , int level, int bStart );

const  MAX_ITERATORS = 5;
const  ITERATE_START  = ( 1 );
const ITERATE_FROM_BASE = ( ITERATE_START + 1 );
// 2 - 2+n
function ITERATE_FROM(n) { return ( n + ITERATE_FROM_BASE ) }
// 7 - 7+n
const ITERATE_AT_BASE = ( ITERATE_START +(1*MAX_ITERATORS) + 1 )
function ITERATE_AT(n)  { return } ( n + ITERATE_AT_BASE )
const ITERATE_NEXT   = ( 0 )


function Card( ID ) {
	if(! (this instanceof Card ) ) return new Card(ID);
	Object.assign( this, {
		 id : ID,			// 0-51 numeric ID of card....
		 fake_id : 0,   // 0-51 possible ID of the card... avoid duplicates.
		flags: {
			 bDiscard:0, // set for discard scanner to remove from hand...
			 bFaceDown:0 // a face down card id kinda wild....
		},
		pPlayedBy : null,  // when card is on table, this is what hand it's from...
		next : null,
		me : null,
		longName : CardLongName,
		name : CardName,
		grab : GrabCard,
	});
	this.nextRef = { o: this, f: 'next' };
}


function  update_callback(cb,psv)
{	
	return { cb:cb, psv:psv };
}


function CardStack(n)
{
	if(! (this instanceof CardStack)) return new CardStack( n );
	Object.assign( this, { name: n,
		cards : null,
		myCards : {o:this, f:'cards'},
		 update_callbacks : null,
		 addUpdate: AddCardStackUpdateCallback,
		 removeUpdate: RemoveCardStackUpdateCallback,
		 update : InvokeUpdates,
		 draw () { return this.cards.grab() },
	}  // struct update_callback(s)
	);
};


function Deck( name ){
	if (!(this instanceof Deck) ) return new Deck(name);
	Object.assign( {
		name : name,
		flags : {
			bHold : false, // cards in hand are not played
			bLowball : false // Lowest hand counts, aces are 1's.
		},
		faces: 0,
		suits: 0,
		card : [], // original array of cards, all cards always originate here.
		Hands : null,  // list of entities which have HANDS
		card_stacks:null, // named regions that have cards...
		_HandIterator : null,
		getCardStack : getCardStack,
		dealTo : DealTo,
		pick : PickACard,
		Hand : CreateHand,
		discard : Discard,
		gather : GatherCards,
		shuffle : Shuffle,
	} )
}

function Hand()
{
	if (!(this instanceof Hand) ) return new Hand();
	Object.assign(this, {
	 card_stacks : [], // named regions that have cards...
	 Deck:null,    // deck this hand belongs to...
	 Tricks:0,  // number of cards swept in tricks...
	// some tracking info to make looping states...
	// 5 levels of card iteratation
	 iStage:[], // counter for which cards's we're iterating through...
	card:[], // current card for looping states...
	getCardStack : GetCardStackFromHand
	} );
}


//typedef PCARD ϕ(IterateHand)( PHAND hand, int level, int bStart );

//CARDS_PROC( PTEXT, CardName )( PCARD card );


/*
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
*/

const poker_hand_class = {
	POKER_HAND_SOMETHING_HIGH : 0,
	POKER_HAND_PAIR : 0x100000,
	POKER_HAND_2PAIR : 0x200000,
	POKER_HAND_3KIND: 0x300000,
	POKER_HAND_STRAIGHT : 0x400000,
	POKER_HAND_FLUSH : 0x500000,
	POKER_HAND_FULLHOUSE : 0x600000,
	POKER_HAND_4KIND : 0x700000,
	POKER_HAND_STRAIGHT_FLUSH : 0x800000,
	POKER_HAND_ROYAL_FLUSH : 0x900000,
	POKER_HAND_5KIND : 0xa00000,
   POKER_HAND_MASK : 0xF00000
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

/*
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

*/

const SUITS = 4

//#define StrDup(s) SaveNameConcatN( s, NULL ); /*if( strcmp( s, ("Table") ) == 0 )DebugBreak();*/


const suits = [ "H", "C", "D", "S" ];
const suits2 = [ ("h"), ("c"), ("d"), ("s") ];
const suitlongnames = [ "Hearts", ("Clubs"), ("Diamonds"), ("Spades") ];

// include A for name of high card ace
// and 1 for name of low card ace
const cardnames = [ ("1"), ("2"), ("3")
                      , ("4"), ("5"), ("6")
                      , ("7"), ("8"), ("9")
							 , ("10"), ("11"), ("12"), ("13"), ("A") ];

const cardnames_char = [ ("1"), ("2"), ("3")
                      , ("4"), ("5"), ("6")
                      , ("7"), ("8"), ("9")
							 , ("T"), ("J"), ("Q"), ("K"), ("A") ];

const cardlongnames = [ ("1"), ("2"), ("3")
                          , ("4"), ("5"), ("6")
                          , ("7"), ("8"), ("9")
                          , ("Ten"), ("Jack"), ("Queen"), ("King"), ("Ace") ];

var l =  {
	flags: {
		ha : false
	},
   decks : []
}

//---------------------------------------------------------------------

function getCardStack(  name )
{
	var stack = this.card_stacks.find( cs=>cs.name===name );
	if( !stack )
	{
		stack = CardStack( name );
      	this.card_stacks.push( stack );
	}
   	return stack;
}

//---------------------------------------------------------------------

function GetCardStackFromHand( name )
{
	var stack = this.card_stacks.find( s=>s.name===name );
	if( !stack )
	{
		stack = CardStack( name );
		this.card_stacks.push( stack );
	}
   	return stack;

}

//---------------------------------------------------------------------

function AddCardStackUpdateCallback( f, psv )
{
	var callback = update_callback( f, psv );
	this.update_callbacks.push( callback );
}

//---------------------------------------------------------------------

function RemoveCardStackUpdateCallback( psv )
{
	var callback = this.update_callbacks.findIndex( (cb)=>cb.psv===psv );
	if( callback >= 0 )
		this.update_callbacks.splice( callback, 1 );
}

//---------------------------------------------------------------------


function InvokeUpdates( )
{
	this.update_callbacks.forEach( cb=>{
		cb.f(cb.psv);
	})
}

//---------------------------------------------------------------------

function PickACard( ID )
{
	// no matter where the card is, this can grab it.
   //console.log( ("Uhmm %p %p %d"), deck, deck.card, ID );
	this.card.grab( ID );
   	return this.card + ID;
}

//---------------------------------------------------------------------

function CreateDeck(  card_iter_name, iter )
{
	var deck = l.decks.find( deck=>deck.name === card_iter_name );

	//console.log( ("Creating a deck ? %s"), card_iter_name );
	if( !deck )
	{
		deck =  Deck( card_iter_name );
		deck.flags.bHold = FALSE;
		deck.card_stacks = null;
		deck.faces = 13;
		deck.suits = 4;
		deck.Hands = [];
		deck._HandIterator = iter;

		// allocate all cards on the draw pile.
		deck.card = [];//NewArray( CARD, deck.faces * deck.suits );
		{
			var draw = deck.getCardStack( "Draw" );
			for( var i = 0; i < deck.faces * deck.suits; i++ )
			{
				var card;
				deck.card.push( card = Card( i ) );
				// create cards, put all in the draw deck.
				if( card.next = draw.cards )
					card.next.me = card.nextRef;
				card.me = draw.myCards;
				draw.cards = card;
            	draw.cards.flags.bFaceDown = TRUE;
			}
			draw.update();
		}
		l.decks.push( deck );
	}
	return deck;
}

//---------------------------------------------------------------------

function CreateWildCard( )
{
	return Card(52);
}

//---------------------------------------------------------------------

function CreateHand()
{
	var ph = Hand();
	this.Hands.push( ph )
	ph.Deck = this;
	return ph;
}

//---------------------------------------------------------------------

function Discard(  pCardRef )
{
	var pLast;
	var discard = this.getCardStack(  ("Discard") );
	if( !pCardRef ) // don't blow up
		return;
	if( !discard.cards ) // nothing to discard....
		return;
	if( pCardRef.o == discard ) // might be able to discard from discard to discard... so, don't.
		return;
	
	pLast = pCardRef.o[pCardRef.f];
	while( pLast.next )
		pLast = pLast.next;

	discard.cards.me = pLast.nextRef;
	pLast.next = discard.cards;
	(discard.cards = (pCardRef.o[pCardRef.f])).me = discard.myCards;

	pCardRef.o[pCardRef.f] = null;
}

//---------------------------------------------------------------------

function GrabCard( )
{
 	// isolates this card from the hand...
	if( this.me )
	{
		if( this.me.o[this.me.f] = this.next )
			this.next.me = this.me;
		this.me = NULL;
		this.next = NULL;
	}
	return this;
}

//---------------------------------------------------------------------

// discard this card only
function DeckDiscard( pCard )
{
	if( !pCard )
		return;
	{
		var discard = this.getCardStack( ("Discard") );
		// wth am I doing?
		pCard.grab();
		if( ( pCard.next = discard.cards ) )
			discard.cards.me = pCard.nextRef;

		pCard.me = discard.myCards;
		discard.cards = pCard;
		discard.update();
	}
}

//---------------------------------------------------------------------

const ϕ=(ϕ)=>ϕ.o[ϕ.f]; 
const θ=(ϕ,θ)=>ϕ.o[ϕ.f]=θ;

//---------------------------------------------------------------------

function MoveCards( ppStackTo, ppStackFrom )
{
	var pLast;
	if( ppStackTo == ppStackFrom ) // don't move from a stack into the same stack.
		return 0;
	if( !ppStackTo || !ppStackFrom )
		return 0;
	pLast = ppStackFrom.o[ppStackFrom.f];
	if( !pLast )
		return 0; // nothing to discard
	while( pLast.next )
		pLast = pLast.next;
	if( pLast.next = ϕ(ppStackTo) )
		ϕ(ppStackTo).me = { o:pLast, f:'next'};
	(θ(ppStackTo, ϕ(ppStackFrom))).me = ppStackTo;
	θ(ppStackFrom, null);
	return 1;
}

//---------------------------------------------------------------------

function MoveCard( ppStackTo, ppCard )
{
	var next;
	if( !ppStackTo || !ppCard || !ϕ(ppCard) )
		return;
	if( ϕ(ppStackTo) )
		ϕ(ppStackTo).me = {o:ϕ(ppCard),f:'next'};
	next = ϕ(ppCard).next;
	ϕ(ppCard).next = ϕ(ppStackTo);
	(θ(ppStackTo, ϕ(ppCard))).me = ppStackTo;
	θ(ppCard, next );
}

//---------------------------------------------------------------------

function GetNthCard( stack, nCard )
{
	var card = stack.cards;
	var n;
	for( n = 0; n < nCard && card; n++ )
	{
		card = card.next;
	}
	return card;
}

function TransferCards(  stack_from,  stack_to,  nCards )
{
	var n;
	var cards = null;
	var card;
	for( n = 0; n < nCards; n++ )
	{
		card = stack_from.draw();
		if( card )
		{
			card.next = cards;
			//card.me = { cards;
			cards = card;
		}
	}
	while( card = cards )
	{
		cards = card.next;
		if( card.next = stack_to.cards )
			stack_to.cards.me = card.nextRef;
		card.me = stack_to.myCards;
		stack_to.cards = card;
	}
	return NULL;
}

//---------------------------------------------------------------------

function TurnTopCard( stack )
{
	if( stack && stack.cards )
		stack.cards.flags.bFaceDown = FALSE;
	stack.update();
}

//---------------------------------------------------------------------

void DeckPlayCard(  pDeck, pCard )
{
	var stack = pDeck.getCardStack( ("Table") );
	if( stack )
	{
		MoveCard( stack.myCards, pCard );
		stack.update();
	}
}

//---------------------------------------------------------------------

function HandAdd(  pCard )
{
	var stack = pHand.getCardStack( "Cards" );
	if( pCard == stack.cards )
	{
		console.log( ("FATALITY!") );
	}
	if( pCard.next = stack.cards )
		pCard.next.me = pCard.nextRef;

	pCard.me = stack.myCards;
	stack.cards = pCard;
}

//---------------------------------------------------------------------

function DeleteHand( hand )
{
	var i;
	var hand = ϕ(ppHand );
	{
		hand.card_stacks.forEach( stack=>{
			hand.Deck.discard( stack.myCards );
		})
		hand.card_stacks = null;
	}
	hand.Deck.getCardStack( "Discard").update();

	i = hand.Deck.Hands.findIndex( hand2=>hand2===hand );
	//&hand.Deck.Hands, hand );
	hand.Deck.Hands.splice( i, 1 );
}

function DeleteDeck( deck )
{
	deck.Hands.forEach( pHand =>{
		DeleteHand( pHand );
	})
	deck.Hands = null;
	deck.card = null;

	// already have all cards from all hands...
	//GatherCards( deck ); // puts them all in the draw stack for easy deletion
	//for( pc = GetCardStack( deck, ("Draw") ).cards; pc; pc= pnext )
	//{
	//	pnext = pc.next;
	//	Release( pc );
	//}
}
//---------------------------------------------------------------------

function GatherCards(  )
{
	var Draw = GetCardStack( this, ("Draw") );

	this.Hands.forEach( pHand=>
	{
		pHand.card_stacks.forEach( stack=>
		{
			if( MoveCards( Draw.myCards, stack.myCards ) )
				InvokeUpdates( stack );
		});
	});


	{
		this.card_stacks.forEach( stack=>
		{
			if( stack == Draw )
				continue;
			// returns false on failure of no actual update done.
			if( MoveCards( Draw.myCards, stack.myCards ) )
				stack.update();
		});
		//InvokeUpdates( Draw );
	}
	{
		var n;
		for( n = 0; n < this.faces*this.suits; n++ )
			this.card[n].flags.bFaceDown = TRUE;
	}
}

//---------------------------------------------------------------------

function CountCards( ph )
{
	var pc;
	var count;
	if( !ph )
		return 0;
	for( count = 0, pc = ph.getCardStack( ("Cards") ).cards; pc; pc = pc.next, count++ );
	return count;
}

//---------------------------------------------------------------------

function ListCards( ph )
{
	var pc;
	var pvt;
	var count;
	var pDeck = ph.Deck;
	if( !ph )
		return null;
	pvt = "";
	for( count = 0, pc = ph.getCardStack( "Cards" ).cards; pc; pc = pc.next, count++ );
	if( count )
	{
		if( !pDeck.flags.bHold )
		{
			for( count = 1, pc = ph.getCardStack( "Cards" ).cards; pc; pc = pc.next, count++ )
			{
				var cardval = CARD_NUMBER( pc.id );
				if( !pDeck.flags.bLowball && !cardval )
					cardval = 13;
		      	pvt += count>1?(","):("") + cardnames[cardval] + suits2[pc.id/13];
			}
		}
		else for( count = 1, pc = GetCardStackFromHand( ph, ("Cards") ).cards; pc; pc = pc.next, count++ )
		{
			var cardval = CARD_NUMBER( pc.id );
			if( !pDeck.flags.bLowball && !cardval )
				cardval = 13;
			pvt += count>1?(","):("") +  count +":" +  cardnames[cardval] + suits2[pc.id/13];
		}
	}
	else
	{
		pvt = "No cards.";
	}
	return pvt;
}


//---------------------------------------------------------------------

// uses the current game iterator to get the cards
// in this list...
function ListGameCards(  ph )
{
	var pc;
	var pvt;
	var count;
	var pDeck = ph.Deck;
	if( !ph )
		return null;
	pvt = "";
	for( count = 0, pc = ph.getCardStack( "Cards" ).cards; pc; pc = pc.next, count++ );
	if( count )
	{
		for( count = 1, pc = pDeck._HandIterator( ph, 0, true );
			 pc;
			  pc = pDeck._HandIterator( ph, 0, false ), count++ )
		{
			var cardval = CARD_NUMBER( pc.id );
			if( !pDeck.flags.bLowball && !cardval )
				cardval = 13;
			pvt += (count>1?(", "):(""))+
					   cardnames[cardval]
					  + suits2[pc.id/13];
		}
	}
	else
	{
		pvt = "No cards.";
	}
	return pvt;
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
      tree.card = card;
      tree.r = r;
      tree.pLess = tree.pMore = NULL;
   }
   else
   {
      if( r > tree.r )
         tree.pMore = sort( tree.pMore, card, r );
      else
         tree.pLess = sort( tree.pLess, card, r );
   }
   return tree;
}

void FoldTree( PCARD *pStack, PHOLDER tree )
{
	if( tree.pLess )
		FoldTree( pStack, tree.pLess );
	if( tree.card.next = *pStack )
		ϕ(pStack).me = &tree.card.next;
	tree.card.me = pStack;

	*pStack = tree.card;
	if( tree.pMore )
		FoldTree( pStack, tree.pMore );
	Release( tree );
}

function Shuffle(  )
{
	var tree;
	var card;
	this.gather();

	tree = NULL;

	while( card = GetCardStack( pDeck, ("Draw") ).cards )
	{
   		if( card.me.o[card.me.m] = card.next )
   			card.next.me = card.me.o[card.me.m];
   		card.next = null;
		card.me = null;
		tree = sort( tree, card, rand() );
	}

	FoldTree( { o:this.getCardStack( "Draw" ), m:"cards"}, tree );
	InvokeUpdates( pDeck.getCardStack( "Draw" ) );

}


//---------------------------------------------------------------------

function CardName( ) {
	return cardnames[this.id%13] + suits2[this.id/13];
}

//---------------------------------------------------------------------

function CardLongName(  )
{
	return cardlongnames[(this.id%13)?(this.id%13):13] + " of " +
			   suitlongnames[this.id/13];
}


void DealTo(  ph, bPlayTo )
{
	var pc = this.getCardStack( ("Draw") ).cards;
	if( pc )
	{
		var ppDest;
		var stack = this.getCardStack ("Draw" );
		// unlink from the draw list
		if( stack.cards = pc.next )
			pc.next.me = stack.myCards;

		if( !ph )
		{
			if( bPlayTo )
				ppDest = this.getCardStack("Table").myCards;
			else
				ppDest = this.getCardStack( ("Discard") ).myCards;
		}
		else
		{
			// determine destination to link card to
			if( bPlayTo )
				ppDest = ph.getCardStack( "Showing" ).myCards;
			else
				ppDest = ph.getCardStack( "Cards" ).myCards;
		}
		// link card into new list.
		if( pc.next = ϕ(ppDest) )
			pc.next.me = pc.nextRef;
		pc.me = ppDest;
		θ(ppDest, pc);
	}
}

