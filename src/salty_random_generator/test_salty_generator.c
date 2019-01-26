#include <stdhdrs.h>

#include <salty_generator.h>

#define MY_MASK_MASK(n,length)   (MASK_TOP_MASK(length) << ((n)&0x7) )
#define MASK_TOP_MASK_VAL(length,val) ((val)&( (0xFFFFFFFFUL) >> (32-(length)) ))
#define MY_MASK_MASK_VAL(n,length,val)   (MASK_TOP_MASK_VAL(length,val) << ((n)&0x7) )

#define SET_MASK(v,n,_mask_size,val)    (((MASKSET_READTYPE*)(((uintptr_t)(v))+(n)/CHAR_BIT))[0] =    \
( ((MASKSET_READTYPE*)(((uintptr_t)(v))+(n)/CHAR_BIT))[0]                                 \
 & (~(MY_MASK_MASK(n,_mask_size))) )                                                                           \
	| MY_MASK_MASK_VAL(n,_mask_size,val) )

static uint32_t seed = 0;
static LOGICAL useSeed = 1;

void getsalt( uintptr_t psv, POINTER *salt, size_t *salt_size )
{
	(*salt) = &seed;
	
	(*salt_size) = useSeed * sizeof( seed );
	//printf( "return seed\n" );
	//offset = 0;
	//LogBinary( buffer, 20 );
}

struct distribution
{
	uint64_t total;
	int bits;
	int units;// = 1 << bits;
	int *unit_counters;// = NewArray( int, units );
	int *follow_counters;// = NewArray( int*, units );
};

struct distribution * GetDistribution( int bits )
{
	struct distribution *d = New( struct distribution );
	int n;
	d->bits = bits;
	d->units = 1 << bits;
	d->unit_counters = NewArray( int, d->units );
	if( d->bits < 16 )
		d->follow_counters = NewArray( int, d->units * d->units );
	d->total = 0;
	MemSet( d->unit_counters, 0, sizeof( int ) * d->units );
	if( bits < 16 )
		for( n = 0; n < d->units; n++ )
		{
			MemSet( d->follow_counters + (n * d->units), 0, sizeof( int ) * d->units );
		}
	return d;
}

int AddSomeDistribution( struct distribution *d, struct random_context *ctx )
{
	int n;
	{
		int prior = 0;
		int64_t prior_value;
		for( n = 0; n < 1000000; n++ ) {
			int32_t value = SRG_GetEntropy( ctx, d->bits, 0 );
			//lprintf( "VALUE: %d  %x", value, value );
			d->unit_counters[value]++;
			if( d->bits < 16 )
				if( prior )
					d->follow_counters[prior_value * d->units + value]++;
			prior = 1;
			prior_value = value;
		}
	}
	d->total += n;
}

void ShowSome( struct distribution *d ) {
	int totUnit;
	int maxUnit;
	int n;

	for( n = 0; n < d->units; n ++ )
	{
		lprintf( "%d = %d ", n, d->unit_counters[n] );
	}
	if( d->bits < 17 )
	for( n = 0; n < d->units; n ++ )
	{
		int m;
		for( m = 0; m < d->units; m++ )
			lprintf( "%d,%d = %d ", n, m, d->follow_counters[n * d->units + m] );
	}
	return d;
}

void mapData( struct distribution *d ) {

	int *orderedCounters = NewArray( int, d->units );// = NewArray( int, units );
	int *orderedDeltas = NewArray( int, d->units );
	int minMidUnit = d->total;
	int maxMidUnit = 0;
	int minUnit = d->total;
	int maxUnit = 0;
	int midUnit;
	int overAvg = 0;
	int underAvg = 0;
	int n;
	uint64_t avg;
	int i;
	lprintf( "Average = total / units. %"PRId64, avg = d->total / d->units );

	for( n = 0; n < d->units; n++ ) {
		int m;
		for( m = n; m > 0; m-- ) {
			if( d->unit_counters[n] > orderedCounters[m - 1] )
				break;
		}
		for( i = n; i > m; i-- ) {
			orderedCounters[i] = orderedCounters[i-1];
			orderedDeltas[i] = orderedDeltas[i - 1];
		}
		orderedCounters[m] = d->unit_counters[n];
		orderedDeltas[m] = n;

		if( d->unit_counters[n] > avg )
			overAvg++;
		else
			underAvg++;

		if( minUnit > d->unit_counters[n] )
			minUnit = d->unit_counters[n];
		if( maxUnit < d->unit_counters[n] )
			maxUnit = d->unit_counters[n];
	}

	lprintf( "Above/below Averge: %d %d   %d", underAvg, overAvg, underAvg * 10000ULL / overAvg );
	midUnit = minUnit + (maxUnit - minUnit) / 2;
	// 2*minUnit/2 + (maxUnit-minUnit)/2 .... ( 2*minUnit + (maxUnit-minUnit) ) / 2
	midUnit = (minUnit + maxUnit) / 2;
	lprintf( "min/max and this median... Min %d Max %d  Mid %d Ofs %d Span  %d", minUnit, maxUnit, midUnit, (int)(avg-midUnit), maxUnit -minUnit );
	lprintf( "medians = %d %d", orderedCounters[d->units / 2], orderedCounters[d->units / 2 + 1] );

	if( d->units < 20 )
	for( i = 0; i < d->units; i++ ) {
		int n;
		int m;
		n = i;
		n = orderedDeltas[i];
		lprintf( "number %3d  : %d %4.3g   %6.3g  %d   %d", n
			, d->unit_counters[n]
			, ((double)d->unit_counters[n] - (double)minUnit) / ((double)maxUnit - (double)minUnit)
			, (d->unit_counters[n] - (double)(avg)) / (maxUnit - minUnit)
			, (d->unit_counters[n] - (maxUnit + minUnit)/2) 
			, (d->unit_counters[n] - avg)
		);
	}

	Release( orderedCounters );
	Release( orderedDeltas );

}


static void salt_generator( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	static struct tickBuffer {
		uint32_t tick;
		uint64_t cputick;
	} tick;
	(void)psv;
	tick.cputick = GetCPUTick();
	tick.tick = GetTickCount();
	salt[0] = &tick;
	salt_size[0] = sizeof( tick );
}


void f2(void)
{
	char message[] = "This is a test, This is Only a test.";
	char output[sizeof( message )];
	char output2[sizeof( message )];
	{
		struct byte_shuffle_key *key = BlockShuffle_ByteShuffler( SRG_CreateEntropy4( salt_generator, 0 ) );
		BlockShuffle_SubBytes( key, message, output, sizeof( message ) );
		BlockShuffle_BusBytes( key, output, output2, sizeof( message ) );

		printf( "Fun:%s\n", output2 );
	}

	{
		struct block_shuffle_key *key = BlockShuffle_CreateKey( SRG_CreateEntropy4( salt_generator, 0 ), sizeof( message ), 1 );

		BlockShuffle_SetData( key, output, 0, sizeof( message ), message, 0 );
		LogBinary( output, sizeof( output ) );
		printf( "SD Fun:%s %d\n", output );
		BlockShuffle_GetData( key, output2, 0, sizeof( message ), output, 0 );
		printf( "Fun:%s\n", output2 );
	}

}


SaneWinMain( argc, argv )
{
	f2();
	uint8_t buffer[512];
	int offset;
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
	if( argc > 2 ) {
		lprintf( "Disable external seed." );
		useSeed = 0;
	}
	uint32_t opts = 0;
	int i;
	int start = timeGetTime();
	int end = 0;
	seed = GetTickCount();
	SetSyslogOptions( &opts );
	SystemLogTime( 0 );
	start = timeGetTime();


	struct distribution *d = GetDistribution( 2 );
	AddSomeDistribution( d, entropy );
	end = timeGetTime();
	lprintf( "2bits 1,000,000 in %d   %d", end - start, (1000000)/(end-start) );
	mapData( d );

	start = timeGetTime();
	for( i = 0; i < 200; i++ )
		AddSomeDistribution( d, entropy );
	end = timeGetTime();
	lprintf( "2bits 200,000,000 in %d   %d", end - start, (200000000) / (end - start) );
	mapData( d );

	struct distribution *d2 = GetDistribution( 16 );
	start = timeGetTime();
	for( i = 0; i < 100; i++ )
		AddSomeDistribution( d2, entropy );
	end = timeGetTime();
	lprintf( "16bits 100,000,000 in %d   %d", end - start, (100000000) / (end - start) );
	mapData( d2 );

	start = timeGetTime();
	for( i = 0; i < 900; i++ )
		AddSomeDistribution( d2, entropy );
	end = timeGetTime();
	lprintf( "16bits 900,000,000 in %d   %d", end - start, (900000000) / (end - start) );
	mapData( d2 );

	start = timeGetTime();
	for( i = 0; i < 250000000; i++ )
		SRG_GetEntropyBuffer( entropy, buffer, 1 );
	end = timeGetTime();
	lprintf( "1 bits 250,000,000 in %d   %d", end - start, (250000000) / (end - start) );

	start = timeGetTime();
	for( i = 0; i < 250000000; i++ )
		SRG_GetEntropyBuffer( entropy, buffer, 2 );
	end = timeGetTime();
	lprintf( "2 bits 250,000,000 in %d   %d", end - start, (250000000) / (end - start) );

	start = timeGetTime();
	for( i = 0; i < 250000000; i++ )
		SRG_GetEntropyBuffer( entropy, buffer, 4 );
	end = timeGetTime();
	lprintf( "4 bits 250,000,000 in %d   %d", end - start, (250000000) / (end - start) );

	start = timeGetTime();
	for( i = 0; i < 250000000; i++ )
		SRG_GetEntropyBuffer( entropy, buffer, 16 );
	end = timeGetTime();
	lprintf( "16 bits 250,000,000 in %d   %d", end - start, (250000000) / (end - start) );

	start = timeGetTime();
	for( i = 0; i < 100000000; i++ )
		SRG_GetEntropyBuffer( entropy, buffer, 256 );
	end = timeGetTime();
	lprintf( "256 bits 100,000,000 in %d   %d", end - start, (100000000) / (end - start) );


#if 0
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
#endif

	return 0;
}
EndSaneWinMain()
