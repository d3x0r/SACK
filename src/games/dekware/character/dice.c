#include <stdio.h>
#include <stdhdrs.h>
#include <stdlib.h>
#include <logging.h>

#define DEFINES_DEKWARE_INTERFACE
#include "plugin.h"

#ifdef __CYGWIN__
#ifndef RAND_MAX
#include <sys/config.h>
#define RAND_MAX __RAND_MAX
#endif
#endif

//--------------------------------------------------------------------------

int GetDice( TEXTCHAR *desc, int *count, int *sides )
{
   int accum;
   int ok;
   accum = 0;
   ok = FALSE;
   while( desc[0] <= '9' && desc[0] >= '0' )
   {
      accum *=10;
      accum += desc[0] - '0';
      desc++;
      ok = TRUE;
   }
   if( !ok )
      return 0;
   *count = accum;
   if( desc[0] != 'd' &&
       desc[0] != 'D' )
     return 0;
   desc++;
   accum = 0;
   ok = FALSE;
   while( desc[0] <= '9' && desc[0] >= '0' )
   {
      accum *=10;
      accum += desc[0] - '0';
      desc++;
      ok = TRUE;
   }
   if( !ok )
      return FALSE;
   *sides = accum;
   return TRUE;
}

//--------------------------------------------------------------------------

#define RAND(n) (((rand()>>3)*(n))/(RAND_MAX>>3))

static int RollDice( int count, int sides )
{
	int accum;
   int n;
   accum = 0;
   while( count-- )
	{
		n = RAND(sides) + 1;
      Log1( WIDE("Rolled %d..."), n );
      accum += n;
	}
	Log1( WIDE("Total: %d..."), accum );
   return accum;
}

//--------------------------------------------------------------------------

static int HandleCommand( WIDE("Roll"), WIDE("Roll dice stored in macro result") )( PSENTIENT ps, PTEXT parameters )
{
   TEXTCHAR *p;
	PTEXT pDice;
   int count, sides;
   pDice = GetParam( ps, &parameters );
   p = GetText( pDice );
   if( !pDice || !GetDice( p, &count, &sides ) )
   {
      DECLTEXT( msg, WIDE("parameter is not a dice description... #d#") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
	else
	{
		if( ps->pLastResult )
         LineRelease( ps->pLastResult );
		ps->pLastResult = MakeNumberText( RollDice( count, sides ) );
      //ps->pLastResult->flags &= TF_TEMP;
	}
   return 0;
}

//--------------------------------------
// $Log: dice.c,v $
// Revision 1.6  2003/03/25 08:59:01  panther
// Added CVS logging
//
