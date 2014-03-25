#include <stdhdrs.h>
#include "salty_generator.h"


#define SET_MASK(v,n,_mask_size,val)    (((MASKSET_READTYPE*)(((PTRSZVAL)(v))+((n)*(_mask_size))/MASKTYPEBITS((v)[0])))[0] =    \
( ((MASKSET_READTYPE*)(((PTRSZVAL)(v))+((n)*(_mask_size))/CHAR_BIT))[0]                                 \
 & (~(MASK_MASK(n,_mask_size))) )                                                                           \
	| MASK_MASK_VAL(n,_mask_size,val) )

   int offset;
	_8 buffer[100];

void getsalt( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
{
	static _32 seed = 0;
	(*salt) = &seed;
	(*salt_size) = sizeof( seed );
	printf( "return seed\n" );
   offset = 0;
   LogBinary( buffer, 20 );
}

SaneWinMain( argc, argv )
{
   struct random_context *entropy = SRG_CreateEntropy( getsalt, 0 );
	int n;

	for( n = 0; n < 1000; n++ )
	{
      int bit = SRG_GetEntropy( entropy, 1, 0 );
      SET_MASK( buffer, offset, 1, bit );
      offset += 1;
		printf( "Flip a coin: %d %s\n", bit, bit?"heads":"tails" );
	}
	for( n = 0; n < 1000; n++ )
	{
		int d1 = ( SRG_GetEntropy( entropy, 3, 0 ) % 6 ) + 1;
		int d2 = ( SRG_GetEntropy( entropy, 3, 0 ) % 6 ) + 1;
      SET_MASK( buffer, offset, 3, d1-1 );
      offset += 3;
      SET_MASK( buffer, offset, 3, d2-1 );
      offset += 3;
		printf( "Roll Dice: %d %d %d\n", d1, d2, d1+d2 );
	}
   return 0;
}
EndSaneWinMain()
