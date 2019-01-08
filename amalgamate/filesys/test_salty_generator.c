#include "sack_ucb_filelib.h"
//#include "salty_generator.h"

#define MY_MASK_MASK(n,length)   (MASK_TOP_MASK(length) << ((n)&0x7) )
#define MASK_TOP_MASK_VAL(length,val) ((val)&( (0xFFFFFFFFUL) >> (32-(length)) ))
#define MY_MASK_MASK_VAL(n,length,val)   (MASK_TOP_MASK_VAL(length,val) << ((n)&0x7) )

#define SET_MASK(v,n,_mask_size,val)    (((MASKSET_READTYPE*)(((uintptr_t)(v))+(n)/CHAR_BIT))[0] =    \
( ((MASKSET_READTYPE*)(((uintptr_t)(v))+(n)/CHAR_BIT))[0]                                 \
 & (~(MY_MASK_MASK(n,_mask_size))) )                                                                           \
	| MY_MASK_MASK_VAL(n,_mask_size,val) )

	int offset;
	uint8_t buffer[100];
	static uint32_t seed = 0;

void getsalt( uintptr_t psv, POINTER *salt, size_t *salt_size )
{
	(*salt) = &seed;
	(*salt_size) = sizeof( seed );
	//printf( "return seed\n" );
	//offset = 0;
	//LogBinary( buffer, 20 );
}

struct distribution
{
	int bits;
	int units;// = 1 << bits;
	int *unit_counters;// = NewArray( int, units );
	int **follow_counters;// = NewArray( int*, units );
};

int CalculateDistribution( struct random_context *ctx, int bits )
{
	struct distribution *d = New( struct distribution );
	int n;
	d->bits = bits;
	d->units = 1 << bits;
	d->unit_counters = NewArray( int, d->units );
	d->follow_counters = NewArray( int*, d->units );
	MemSet( d->unit_counters, 0, sizeof( int ) * d->units );
	if( bits < 17 )
		for( n = 0; n < d->units; n++ )
		{
			d->follow_counters[n] = NewArray( int, d->units );
			MemSet( d->follow_counters[n], 0, sizeof( int ) * d->units );
		}

	{
		int prior = 0;
		int64_t prior_value;
		for( n = 0; n < 200000000; n ++ )
		{
			int32_t value = SRG_GetEntropy( ctx, bits, 0 );
			//lprintf( "VALUE: %d  %x", value, value );
			d->unit_counters[value]++;
			if( bits < 17 )
				if( prior )
					d->follow_counters[prior_value][value]++;
			prior = 1;
			prior_value = value;
		}
	}

	for( n = 0; n < d->units; n ++ )
	{
		lprintf( "%d = %d ", n, d->unit_counters[n] );
	}
	if( bits < 17 )
	for( n = 0; n < d->units; n ++ )
	{
		int m;
		for( m = 0; m < d->units; m++ )
			lprintf( "%d,%d = %d ", n, m, d->follow_counters[n][m] );
	}
	return 0;
}

SaneWinMain( argc, argv )
{
	//struct random_context *entropy = SRG_CreateEntropy2( getsalt, 0 );
	struct random_context *entropy = argc > 1 ? 
		  argv[1][0] == '1' ? SRG_CreateEntropy( getsalt, 0 )
		: argv[1][0] == '2' ? SRG_CreateEntropy2( getsalt, 0 )
		: argv[1][0] == '3' ? SRG_CreateEntropy2_256( getsalt, 0 )
		: argv[1][0] == '4' ? SRG_CreateEntropy3( getsalt, 0 )
		: argv[1][0] == '5' ? SRG_CreateEntropy4( getsalt, 0 )
		: SRG_CreateEntropy4( getsalt, 0 )
		: SRG_CreateEntropy4( getsalt, 0 )
		;
		int n;
	FLAGSETTYPE opts = 0;
	int start = timeGetTime();
	int end = 0;
	seed = GetTickCount();
	SetSyslogOptions( &opts );
	SystemLogTime( 0 );
	start = timeGetTime();
	CalculateDistribution( entropy, 2 );
	end = timeGetTime();
	lprintf( "2bits 200000000 in %d ", end - start );
	start = timeGetTime();
	CalculateDistribution( entropy, 7 );
	end = timeGetTime();
	lprintf( "7bits 200000000 in %d ", end - start );
	start = timeGetTime();
	//CalculateDistribution( entropy, 6 );
	//CalculateDistribution( entropy, 8 );
#ifdef __64__
	CalculateDistribution( entropy, 16 );
	end = timeGetTime();
	lprintf( "16bits 200000000 in %d ", end - start );
	start = timeGetTime();
	CalculateDistribution( entropy, 24 );
	end = timeGetTime();
	lprintf( "24bits 200000000 in %d ", end - start );
	start = timeGetTime();
#else
	CalculateDistribution( entropy, 10 );
	end = timeGetTime();
	lprintf( "10bits 200000000 in %d", end - start );
	start = timeGetTime();

#endif
	for( n = 0; n < 1000; n++ )
	{
		int d1 = ( SRG_GetEntropy( entropy, 3, 0 ) ) ;
		int d2 = ( SRG_GetEntropy( entropy, 3, 0 )  ) ;

		//lprintf( "%08x %08x", (~(MY_MASK_MASK(offset,3))), MY_MASK_MASK_VAL(offset,3,d1) );
		//SET_MASK( buffer, offset, 3, d1 );
		//offset += 3;
		//lprintf( "%08x %08x", (~(MY_MASK_MASK(offset,3))), MY_MASK_MASK_VAL(offset,3,d2) );
		//SET_MASK( buffer, offset, 3, d2 );
		//offset += 3;
		d1 = ( d1 % 6 ) + 1;
		d2 = ( d2 % 6 ) + 1;
		printf( "Roll Dice: %d %d %d\n", d1, d2, d1+d2 );
	}
	for( n = 0; n < 1000; n++ )
	{
		int bit = SRG_GetEntropy( entropy, 1, 0 );
		//SET_MASK( buffer, offset, 1, bit );
		//offset += 1;
		printf( "Flip a coin: %d %s\n", bit, bit?"heads":"tails" );
	}


	return 0;
}
EndSaneWinMain()
