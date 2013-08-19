#define PLUGIN_MODULE
#include <stdhdrs.h>
#include <plugin.h>
#define DO_LOGGING
#include <logging.h>

#include "cards.h"

//-------------------------------------------------------------------------

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

//-------------------------------------------------------------------------

int IsWild( PCARD pCard, PCARD pWild )
{
   // check to see if the card can be wild....
	if( pCard->id == 52 )
		return TRUE;
	{
		PCARD pCheck = pWild;
		for( ; pCheck; pCheck = pCheck->next )
		{
			if( pCard->id == pCheck->id )
				return TRUE;
		}
	}
   return FALSE;
}

//-------------------------------------------------------------------------

int CountWild( PHAND pHand, PCARD pWild )
{
   // check to see if the card can be wild....
   int n = 0;
   PCARD pCheck, pCard;
   HandIterator iter = pHand->Deck->_HandIterator;
   for( pCard = iter( pHand, 0, TRUE ); 
		pCard; 
		pCard = iter( pHand, 0, FALSE ) )
   {
      pCheck = pWild;
      while( pCheck )
      {
         if( pCard->id == pCheck->id )
         {
            n++;
            break;
         }
         pCheck = pCheck->next;
      }
   }
   return n;
}

//-------------------------------------------------------------------------

int HighCard( PHAND pHand, PCARD pWild )
{
   int result, max = 0, val, skipwild = 0, n;
   PCARD pCard;
	HandIterator iter = pHand->Deck->_HandIterator;
   result = 0;
   for( n = 0; n < 5; n++ )
   {
		max = 0;
		for( pCard = iter( pHand, 0, TRUE ); 
				pCard; 
				pCard = iter( pHand, 0, FALSE ) )
		{
			// if it's wild, it's the highest card.
      	if( IsWild( pCard, pWild ) )
			{
            // low card play needs to not pass wild cards...
            // whatever, we have at least a pair.
            return 0;
        }
        else // if not wild, then grab this card's value.
        {
			  val = CARD_NUMBER( pCard->id ) + 1;
           //lprintf( "Checking card %d in %08x", val, result );
            if( val == 1 )  // aces are high.
                val = 14;
        }
		   // biggest number yet
		   // but if we have a result, the low nibble is the highest card 
		   // which it may be.  
         if( val >= max  )
         {
         	if( result )
         	{
         		// only problem is when this has a pair or more....
         		// but this algorithm does not apply to a pair
         		// though it does apply to 2 pair and flushes.
         		if( val < ( (result) & 0xF ) )
         		{
         			max = val;
         		}
         	}
         	else
         		max = val; // if no result yet we want THE highest.
			} // otherwise we don't want to store this.
      }
     	result <<= 4;
		result += (max);
      //lprintf( "Result is now %08x", result );
   }
   return 0x0000000 | result;
}

//-------------------------------------------------------------------------

int IsPair( PHAND pHand, PCARD pWild )
{
   int val1, val2, max = 0;
	HandIterator iter = pHand->Deck->_HandIterator;
   PCARD pCheck, pCheck2;
   for( pCheck = iter(pHand, 0, ITERATE_START );
		  pCheck; 
		  pCheck = iter( pHand, 0, ITERATE_NEXT ) )
   {
   	// matching 3 of a kind aces (0) are high... therefore correct
   	// the value. (and below)
      val1 = CARD_NUMBER( pCheck->id );
      if( !val1 )
         val1 = 13;
      if( IsWild( pCheck, pWild ) )
      	val1 = 0; // this is okay to match with ANY
      for( pCheck2 = iter( pHand, 1, ITERATE_FROM(0) ); 
			  pCheck2; 
			  pCheck2 = iter( pHand, 1, ITERATE_NEXT ) )
      {
			val2 = CARD_NUMBER( pCheck2->id );
         //lprintf( "checking to see if %d=%d", val2, val1 );
         if( !val2 )
            val2 = 13;
	      if( IsWild( pCheck2, pWild ) )
   	   	val2 = 0; // this is okay to match with ANY
			
         if( !val1 || !val2 || ( val1 == val2 ) )
         {
	         if( val1 )
	         {
		      	if( val1 > max )
		      		max = val1;
		      } 
		      else if( val2 )
	         {
		      	if( val2 > max )
		      		max = val2;
		      } 
		      else
				{
					// all 2 of these are wild, MUST be a pair.
					// max=13;
               //lprintf( "More than a pair - failed is pair." );
					return 0x10D; // 3 of a kind ace high.
				}
         }
      }
   }
   if( max )
      return 0x100000 | max;
   return 0;
}

//-------------------------------------------------------------------------

int Is2Pair( PHAND pHand, PCARD pWild )
{
   int n1 = 0, n2 = 0, val1, val2;
   PCARD pCheck, pCheck2;
	HandIterator iter = pHand->Deck->_HandIterator;
   // 2 wild cards or more is better.
   // 1 wild card and a pair is better. (3 of a kind)
   // 2 pair is only natural.
   if( CountWild( pHand, pWild ) )
   	return 0; // it can't be this. (it could by WHY)

   for( pCheck = iter( pHand, 0, ITERATE_START );
		  pCheck; 
		  pCheck = iter( pHand, 0, ITERATE_NEXT ) )
   {
      val1 = CARD_NUMBER( pCheck->id );
      if( !val1 ) // aces are numerically 0 but need to be 13.
         val1 = 13;
      for( pCheck2 = iter( pHand, 1, ITERATE_FROM(0) );
			  pCheck2 ; 
			  pCheck2 = iter( pHand, 1, FALSE ) )
      {
         val2 = CARD_NUMBER( pCheck2->id );
         if( !val2 )
            val2 = 13;
         if( val1 == val2 )
         {
            if( n1 )
            {
               if( n1 == val1 ) // 3 of a kind trip
                  return 0; // 3 of a kind will trigger....
               if( n2 )
               {
                  if( val1 > n1 )
                  	n1 = val1;
                  else if( val1 > n2 )
                  	n2 = val1;
                  // otherwise both are higher than this one we'd call.
	            }
	            else
	               n2 = val1;
            }
            else
               n1 = val1;
            break;
         }
      }
   }
   if( n1 < n2 )
   {
      int t;
      t = n1;
      n1 = n2;
      n2 = t;
   }
   if( n1 && n2 )
      return 0x0200000 | (n1<<4) | n2 ;
   return 0;
}

//-------------------------------------------------------------------------

int Is3Kind( PHAND pHand, PCARD pWild )
{
   int val1, val2, val3, max = 0;
   PCARD pCheck, pCheck2, pCheck3;
	HandIterator iter = pHand->Deck->_HandIterator;
   for( pCheck = iter( pHand, 0, ITERATE_START ); 
		  pCheck; 
		  pCheck = iter( pHand, 0, ITERATE_NEXT ) )
   {
   	// matching 3 of a kind aces (0) are high... therefore correct
   	// the value. (and below)
      val1 = CARD_NUMBER( pCheck->id );
      if( !val1 )
         val1 = 13;
      if( IsWild( pCheck, pWild ) )
      	val1 = 0; // this is okay to match with ANY
      for( pCheck2 = iter( pHand, 1, ITERATE_FROM(0) ); 
			  pCheck2; 
			  pCheck2 = iter( pHand, 1, ITERATE_NEXT ) )
      {
         val2 = CARD_NUMBER( pCheck2->id );
         if( !val2 )
            val2 = 13;
	      if( IsWild( pCheck2, pWild ) )
   	   	val2 = 0; // this is okay to match with ANY
         if( !val1 || !val2 || ( val1 == val2 ) )
         {
	         for( pCheck3 = iter( pHand, 2, ITERATE_FROM(1) ); 
					  pCheck3; 
					  pCheck3 = iter( pHand, 2, 0 ) )
   	      {
      	      val3 = CARD_NUMBER( pCheck3->id );
         	   if( !val3 )
            	   val3 = 13;
			      if( IsWild( pCheck3, pWild ) )
   			   	val3 = 0; // this is okay to match with ANY
	            if( !val3 || (val1)?(val3==val1):(val2)?(val3==val2):0 )
	            {
	            	if( val1 )
	            	{
		            	if( val1 > max )
		            		max = val1;
		            } 
		            else if( val2 )
	            	{
		            	if( val2 > max )
		            		max = val2;
		            } 
		            else if( val3 )
		            {
		            	if( val3 > max )
		            		max = val3;
					   }
					   else
					   {
					   	// all 3 of these are wild, MUST be 3 kind.
					   	// max=13; 
					   	return 0x30000D; // 3 of a kind ace high.
					   }
	            	break;
	            }
				}
         }
      }
   }
   if( max )
      return 0x300000 | max;
   return 0;
}

//-------------------------------------------------------------------------

int IsStraight( PHAND pHand, PCARD pWild )
{
    int start = -1, length = 0, maxstart, maxlength = 0, wilds;
    PCARD card;
	HandIterator iter = pHand->Deck->_HandIterator;
    int n;
    wilds = CountWild( pHand, pWild );
    for( n = 0; n <= 13; n++ )
    {
        for( card = iter( pHand, 0, ITERATE_START );
			    card; 
				 card = iter( pHand, 0, ITERATE_NEXT ) )
        {
            int val = CARD_NUMBER(card->id);
            if( val == 0 && n > 0 )
                val = 13;
            if( val == n )
            {
                if( start < 0 )
                {
                    start = n;
                    length = 1;
                }
                else
                {
                    length++;
                }
                break;
            }
        }
        if( !card )
        {
            if( wilds )
            {
                if( start >= 0 )
                {
                    wilds--;
                    length++;
                }
            }
            else
            {
                if( length > maxlength )
                {
                    maxlength = length;
                    maxstart = start;
                }
                length = 0;
                start = -1;
                wilds = CountWild( pHand, pWild );
            }
        }
    }
    //Log4( "Length: %d start: %d  maxlen: %d  maxstart : %d", length, start, maxlength, maxstart );
    if( length >= 5 )
    {
        if( wilds )
            length += wilds;
        if( (start + length-2) >= 13 )
            return 0x40D;
        return 0x400000 + (start + length-2);
    }
    if( maxlength >= 5 )
        return 0x400000 + (maxstart + maxlength-2);
    return 0;
}

//-------------------------------------------------------------------------

int IsFlush( PHAND pHand, PCARD pWild )
{
	HandIterator iter = pHand->Deck->_HandIterator;	
	int number = 0, val, match = 0;
	PCARD card, start = GetCardStackFromHand( pHand, "Cards" )->cards;
	for( start = iter( pHand, 0, ITERATE_START );
		  start && ( match < 5 );
		  start = iter( pHand, 0, ITERATE_NEXT ) )
	{
		match = 0;
		number = 0;
		for( card = iter( pHand, 1, ITERATE_AT(0) );
			 match < 5 && card;
			  card = iter( pHand, 1, ITERATE_NEXT ) )
			if( !IsWild( card, pWild ) ) // if it's wild we don't care.
			{
				if( !number )
				{
					number = CARD_SUIT( card->id ) + 1;
					match++;
				}
				// if this number is not the number we're matching, is not 5 kind
				else if( number == ( CARD_SUIT( card->id ) + 1 ) )
					match++;
			}
			else
				match++;
	}
	if( match == 5)
	{
		val = HighCard( pHand, pWild );
		return 0x500000 + val;
	}
	return 0;
}

//-------------------------------------------------------------------------

int IsFullHouse( PHAND pHand, PCARD pWild )
{
	int val, max = 0;
	int nWilds = CountWild( pHand, pWild );
	HandIterator iter = pHand->Deck->_HandIterator;
	if( nWilds > 2 )
	{
		Log( " we're in trouble... we have more than 2 wilds in a full house?" );
		return 0;
	}

	// Due to the nature - will use all wild cards in the three of a kind
	// and never ever in a pair.
	// for if we have 4+ wild cards that's 5 kind
	// if we have 3 wild cards that's 4 kind
	// if we have 2 wild cards and a pair that's 4 kind
	// if we have 2 wild cards and no pair that's 3 kind(only)
	// if we have 1 wild card and a natural 3 kind that's 4 kind.
	// if we have 1 wild card then we need 2 pair or a natural. 
	//   AND the wild card will construct the top 3 kind
	// else we have a natural 3 kind and pair (which is not the same value
	//    as the three of a kind.
	if( val = Is3Kind( pHand, pWild ) )
	{
		// here have to do custom ispair code so that we exclude those 
		// which are already accounted for.
      int val1, val2;
      PCARD pCheck, pCheck2;
      for( pCheck = iter( pHand, 0, ITERATE_START ); 
			  pCheck; 
			  pCheck = iter( pHand, 0, ITERATE_NEXT ) )
      {
      	// matching 3 of a kind aces (0) are high... therefore correct
      	// the value. (and below)
      	if( IsWild( pCheck, pWild ) )
      		continue; // any wild is already used.
      	if( ( val1 = CARD_NUMBER( pCheck->id ) ) == ( val & 0xFF ) )
      		continue; // any card which is in the 3 of a kinda found is used.

         if( !val1 )
            val1 = 13;

         for( pCheck2 = iter( pHand, 0, ITERATE_FROM(0) ); 
				  pCheck2; 
				  pCheck2 = iter( pHand, 0, ITERATE_NEXT ) )
         {
	      	if( IsWild( pCheck2, pWild ) )
   	   		continue; // any wild is already used.
      		if( ( val2 = CARD_NUMBER( pCheck2->id ) ) == ( val & 0xFF ) )
      			continue; // any card which is in the 3 of a kinda found is used.
				if( val1 == val2 )
				{
					if( val1 > max )
						max = val1;
					break; // next outer card to match, please.
				}
         }
      }
		if( max )
		{
			return 0x600000 | ( ( val & 0xF ) << 4 ) | max;
		}
	}
	return 0;
}

//-------------------------------------------------------------------------

int Is4Kind( PHAND pHand, PCARD pWild )
{
	int number, max = 0, test;
	int matched = 0;
	HandIterator iter = pHand->Deck->_HandIterator;
	int last_kicker = 0;
	int kicker = 0;
	PCARD start = GetCardStackFromHand( pHand, "Cards" )->cards;
	PCARD card;
	for( start = iter( pHand, 0, ITERATE_START ); start; start = iter( pHand, 0, ITERATE_NEXT ) )
	{
		matched = 0;
		number = 0;
		kicker = 0;
		for( card = iter( pHand, 1, ITERATE_AT(0) );
			 matched < 4 && card;
			 card = iter( pHand, 1, ITERATE_NEXT ) )
		{
			if( !IsWild( card, pWild ) ) // if it's wild we don't care.
			{
				test = CARD_NUMBER( card->id );
				if( !test )
					test = 13;
				if( !number )
				{
					number = test;
					matched++;
				}
				else if( number == test ) 
				{
					matched++;
				}
				else
				{
					if( test > kicker )
						kicker = test;
				}
			}
			else // all wild cards match.
				matched++;
		}
		if( matched >= 4 )
		{
			if( !number )
				number = 13;			
			if( number > max )
				max = number;
			last_kicker = kicker;
		}
		start = start->next;
	}
	if( max )
		return 0x700000 + (max<<4) + last_kicker;
	return 0;
}

//-------------------------------------------------------------------------

int IsStraightFlush( PHAND pHand, PCARD pWild )
{
	int val = 0;
	if( IsFlush( pHand, pWild ) )
            if( val = IsStraight( pHand, pWild ) )
            {
                if( (val & 0xFF) == 0xD ) // special case - royal flush.
                    return 0x900000;
                return ( val & 0xFF ) | 0x800000;
            }
	return 0;
}

//-------------------------------------------------------------------------

int Is5Kind( PHAND pHand, PCARD pWild )
{
	int nWild = CountWild( pHand, pWild );
	HandIterator iter = pHand->Deck->_HandIterator;
	int count;
	// must have at least one wild to consider this hand.
	if( !nWild )
		return 0;
	{
		int number = 0;
		PCARD card;
		count = 0;
		for( card = GetCardStackFromHand( pHand, "Cards" )->cards; card; card = card->next )
			if( !IsWild( card, pWild ) ) // if it's wild we don't care.
			{
				if( !number )
				{
					number = CARD_NUMBER( card->id ) + 1;
					count++;
				}
				// if this number is not the number we're matching, is not 5 kind
				else if( number != CARD_NUMBER( card->id ) )
					return 0;
				count++;
			}
			else
				count++;
		if( !number )
			number = 13; // all wilds, therefore we assume 5 aces.
		if( count >= 5 )
			return 0xA00000 + number;
	}
	return 0; // may not be passed 5 cards
}

//-------------------------------------------------------------------------

int ValuePokerHand( PHAND pHand, PCARD pWild, int bHigh )
{
	int val;
	if( !pHand )
		return 0;
	if( bHigh )
	{
		val = Is5Kind( pHand, pWild );
		// this is a special straight flush (name only?)
		// if( !val ) val = IsRoyalFlush( pHand, pWild );
		if( !val ) val = IsStraightFlush( pHand, pWild );
		if( !val ) val = Is4Kind( pHand, pWild );
		if( !val ) val = IsFullHouse( pHand, pWild );
		if( !val ) val = IsFlush( pHand, pWild );
		if( !val ) val = IsStraight( pHand, pWild );
		if( !val ) val = Is3Kind( pHand, pWild );
		if( !val ) val = Is2Pair( pHand, pWild );
		if( !val ) val = IsPair( pHand, pWild );
		if( !val ) val = HighCard( pHand, pWild );
	}
	else
	{
		val = HighCard( pHand, pWild );
		// this is a special straight flush (name only?)
		// if( !val ) val = IsRoyalFlush( pHand, pWild );
		if( !val ) val = IsStraightFlush( pHand, pWild );
		if( !val ) val = Is4Kind( pHand, pWild );
		if( !val ) val = IsFullHouse( pHand, pWild );
		if( !val ) val = IsFlush( pHand, pWild );
		if( !val ) val = IsStraight( pHand, pWild );
		if( !val ) val = Is3Kind( pHand, pWild );
		if( !val ) val = Is2Pair( pHand, pWild );
		if( !val ) val = IsPair( pHand, pWild );
		if( !val ) val = Is5Kind( pHand, pWild );
	}
   return val;
}

//-------------------------------------------------------------------------

PTEXT GetPokerHandName( PHAND pHand, PTEXT *ppLastValue )
{
	int value;
	PVARTEXT pvt;
	if( *ppLastValue )
		LineRelease( *ppLastValue );
	pvt = VarTextCreate();
	value = ValuePokerHand( pHand, NULL, TRUE );
	if( !value )
	{
		vtprintf( pvt, "No hand" );
	}
	else switch( value >> 20 )
	{
	case 0:
		vtprintf( pvt, "High Card - %s", cardlongnames[ ((value>>16)&0xF)-1 ] );
		break;
	case 1:
		vtprintf( pvt, "A Pair of %ss", cardlongnames[ (value&0xF) ] );
		break;
	case 2:
		vtprintf( pvt, "2 Pair %ss and %ss"
						, cardlongnames[ (value&0xF0)>>4] 
						, cardlongnames[ (value&0xF) ] 
						 );
		break;
	case 3:
		vtprintf( pvt, "3 of a Kind %ss", cardlongnames[ (value&0xF) ] );
		break;
	case 4:
		vtprintf( pvt, "Straight" );
		break;
	case 5:
		vtprintf( pvt, "Flush" );
		break;
	case 6:
		vtprintf( pvt, "Full House %ss over %ss"
						, cardlongnames[ (value&0xF0)>>4] 
						, cardlongnames[ (value&0xF) ] 
						);
		break;
	case 7:
		vtprintf( pvt, "4 of a Kind %ss", cardlongnames[(value&0xF0)>>4] );
		break;
	case 8:
		vtprintf( pvt, "Straight Flush" );
		break;
	case 9:
		vtprintf( pvt, "Royal Flush" );
		break;
	case 10:
		vtprintf( pvt, "5 of a Kind %ss", cardlongnames[value&0xF] );
		break;
	}
	*ppLastValue = VarTextGet( pvt );
   VarTextDestroy( &pvt );
	return *ppLastValue;
}

//-------------------------------------------------------------------------

PCARD IterateHoldemHand( PHAND hand, int level, int bStart )
{
	//lprintf( "Iterate start: %d level: %d", bStart, level );
	if( bStart != ITERATE_NEXT )
	{
		if( bStart == ITERATE_START )
		{
			hand->card[level] = GetCardStackFromHand( hand, "Cards" )->cards;;
			hand->iStage[level] = 0;
		}
		else
		{
			if( bStart >= ITERATE_FROM_BASE )
			{
				int n = bStart - ITERATE_FROM_BASE;
				if( n >= MAX_ITERATORS )
               n -= MAX_ITERATORS;
            // this is iterate_from or iterate_at
				// this is iterate from - which starts at the next
				// beyond the iterator of the level specified...
				hand->card[level] = hand->card[n];
				hand->iStage[level] = hand->iStage[n];
            // this is what makes an Iteratefrom be an iterate from
				if( bStart < ITERATE_AT_BASE ) // this is NOT an AT so do step, else return here.
					goto Step_next_card;
			}
		}
	}
	else
	{
	Step_next_card:
		if( hand->card[level] )
		{
			hand->card[level] = hand->card[level]->next;
			while( hand->iStage[level] < 1 &&
					!hand->card[level] )
			{
				hand->card[level] = GetCardStack( hand->Deck, "Table" )->cards;
				hand->iStage[level]++;
			}
		}
	}
	//lprintf( "Result: %p", hand->card[level] );
	return hand->card[level];
}

//-------------------------------------------------------------------------

PCARD IterateStudHand( PHAND hand, int level, int bStart )
{
	if( bStart )
	{
		hand->card[level] = GetCardStackFromHand( hand, "Cards" )->cards;;
		hand->iStage[level] = 0;
	}
	else
	{
		hand->card[level] = hand->card[level]->next;
		while( hand->iStage[level] < 1 &&
			   !hand->card[level] )
		{
			hand->card[level] = GetCardStackFromHand( hand, "Showing" )->cards;
			hand->iStage[level]++;
		}
	}
	return hand->card[level];
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

/* okay - this is the plan - build a brain programatically to handle
this poker thing.... setting event triggers in and out... 

define inputs... all cards unique trigger inputs...

  okay - first - how?


  outputs as....
      5 of a kind... need to include what wild cards are....


  :trigger wild cards...
  :trigger normal cards
  :enable trigger check....

  neurons for results must be above threshold 500
  all neurons are digital 0-100

  result as ...  2 pair is reallly nasty....
  */


// $Log: poker.c,v $
// Revision 1.14  2005/08/08 10:57:07  d3x0r
// Fix calculation of highcard to include all cards so KQJT5 and KQJT4 compare different.... also include name of high card A and change name of low card ace to 1
//
// Revision 1.13  2005/08/08 09:28:43  d3x0r
// Modified default iterator to holdem.(works for draw poker too) Fixed up deleting decks and hands
//
// Revision 1.12  2004/08/13 09:27:50  d3x0r
// Extend API a bit to generate wild cards, and dump test data into a database... 30 minutes or so local
//
// Revision 1.11  2004/08/12 09:16:57  d3x0r
// Extend card API a bit... provide a plugin to demo test cards
//
// Revision 1.10  2004/08/11 10:13:50  d3x0r
// Links, stretched cards into self-library
//
// Revision 1.9  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.8  2003/08/01 23:53:13  panther
// Updates for msvc build
//
// Revision 1.7  2003/08/01 02:36:18  panther
// Updates for watcom...
//
// Revision 1.6  2003/03/25 09:44:44  panther
// Ug strange breaks due to bad lgoging additions
//
// Revision 1.5  2003/03/25 08:59:01  panther
// Added CVS logging
//
