#include <stdhdrs.h>


char *file;
int start;
int count;
int pack_size;
int p; // used counter for which pack we're on globally.

// per-pack count numbers...
uint8_t shared_nums[75];
int multi_nums[75]; // 75 is overkill/ really this is pack size.
LOGICAL used_n_row[15];
int same_spot[25];
//
uint32_t current_card;


PLIST pack;

void CARDLOOP( uint8_t* card, void(*f)(int,int,int,int) ) {
	int n = 0;
	int c, r;
	for( c = 0; c < 5; c++ ) {
		int base = c * 15;
		for( r = 0; r < 5; r++ ) {
			int spot;
			if( r == 2 && c == 2 )
				continue;
			if( n & 1 )
				spot = card[n/2] & 0xF;
			else
				spot = card[n/2] >> 4;
			f( c, r, base, spot );
			//shared_nums[spot + base]++;
			n++;
		}
	}
}


void matchSpots( uint8_t* card, uint8_t* card2 )
{
	int n = 0;
	int c, r;
	for( c = 0; c < 5; c++ ) {
		//int base = c * 15;
		for( r = 0; r < 5; r++ ) {
			int spot;
			int spot2;
			if( r == 2 && c == 2 )
				continue;
			if( n & 1 )
				spot = card[n/2] & 0xF;
			else
				spot = card[n/2] >> 4;
			if( n & 1 )
				spot2 = card2[n/2] & 0xF;
			else
				spot2 = card2[n/2] >> 4;
			if( spot == spot2 )
				same_spot[n]++;
			n++;
		}
	}
}

void packReset( void )
{
	int n;
	EmptyList( &pack );
	for( n = 0; n < 15; n++ ) {
		used_n_row[n] = FALSE;
	}
	for( n = 0; n < 75; n++ ) {
		shared_nums[n] = 0;
		multi_nums[n] = 0;
	}
	for( n = 0; n < 25; n++ )
	{
		same_spot[n] = 0;
	}
	//lprintf( "pack reset -----------" );
}


void NCounter( int c, int r, int base, int spot )
{
	if( c == 2 ) {
		//lprintf( "used %d", spot - 1 );
		used_n_row[spot - 1]++;
	}
}

void countNs( uint8_t* card )
{
   CARDLOOP( card, NCounter );
}

void NumCounter( int c, int r, int base, int spot )
{
   shared_nums[spot+base]++;
}

void countNums( uint8_t* card )
{
	CARDLOOP( card, NumCounter );
}

void countPack( void )
{
	uint8_t* card;
	INDEX idx;


	LIST_FORALL( pack, idx, uint8_t*, card ) {
		countNums( card );
		countNs( card );
	}
	LIST_FORALL( pack, idx, uint8_t*, card ) {
		INDEX idx2 = idx + 1;
		uint8_t* card2;
		LIST_NEXTALL( pack, idx2, uint8_t*, card2 ) {
			matchSpots( card, card2 );
		}
	}

	{
		int total;
		int n;
		if( pack_size == 3 ) {
			int missed = 0;
			for( n = 0; n < 15; n++ )
				if( !used_n_row[n] )
					missed++;
			if( missed != 3 ) {
				for( n = 0; n < 15; n++ )
					if( !used_n_row[n] )
						lprintf( "missing N : %d    card:%d", n, current_card );
			}

		}
		else
		{
			for( n = 0; n < 15; n++ )
				if( !used_n_row[n] )
					lprintf( "missing N : %d    card:%d", n, current_card );
		}
		for( n = 0; n < 75; n++ ) {
			if( pack_size == 3 )
				if( shared_nums[n] > 1  )
					lprintf( "3on Overflow.   %2d:%d ", n, shared_nums[n] );
			if( pack_size == 6 )
				if( shared_nums[n] > 4 )
					lprintf( "6on Overflow.   %2d:%d ", n, shared_nums[n] );
			if( pack_size == 9 )
				if( shared_nums[n] > 6 )
					lprintf( "9on Overflow.   %2d:%d ", n, shared_nums[n] );

			multi_nums[shared_nums[n]]++;
		}
		//	lprintf( "%3d : %d", n, multi_nums[n] );
		{
			int z;
			int zmin;
			if( pack_size == 3 )
				zmin = 2;
			if( pack_size == 6 )
				zmin = 4;
			if( pack_size == 9 )
				zmin = 5;
			for( z = zmin; z < 12; z++ )
				if( multi_nums[z] )
					lprintf( "multinum overlap:  %3d : %d", z, multi_nums[z] );

		}

		for( n = 0; n < 75; n++ ) {
		}
		total = 0;
		for( n = 0; n < 25; n++ )
		{
			if( same_spot[n] )
				total++;
		}
			if( pack_size == 6 )
				if( total > 14 )
					lprintf( "too many overlaps (6) %d of 14 %d", total, p );
			if( pack_size == 9 )
				if( total > 31 )
					lprintf( "too many overlaps (9) %d of 31 %d", total, p );
			if( pack_size == 3 )
				if( total > 3 )
					lprintf( "too many overlaps (3) %d", p );
	}
}


void countStats( uint8_t* cards, int pack_size, int start, int count )
{
	int n;
	int c;
	uint8_t* card;
	start = start-1;
	start -= start % pack_size;
	for( p = 0; p < count; p++ ) {
		for( c = 0; c < pack_size; c++ ) {
			current_card = (start + (p * pack_size) + c);
			card = cards + current_card * 12;
			AddLink( &pack, card );
		}
		countPack();
		packReset();
	}
}


int main( int argc, char **argv )
{
	if( argc < 4 )
	{
		printf( "usage: %s <cardfile.dat> <pack size> <start> <count>\n", argv[0] );
		return 0;
	}
	file = argv[1];
	pack_size = atoi( argv[2] );
	start = atoi( argv[3] );
	count = atoi( argv[4] );

	{
		uintptr_t size = 0;
		uint8_t* card_memory = OpenSpace( NULL, file, &size );
		if( card_memory )
		{
			countStats( card_memory, pack_size, start, count );
		}
		else
		{
			printf( "failed to open %s\n", file );
		}
	}
	lprintf( "Done." );
	return 0;
}


