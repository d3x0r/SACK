
#include <stdhdrs.h>

#include <salty_generator.h>
#include "srg_internal.h"

struct block_shuffle_key
{
	size_t width;
	size_t height;
	size_t extra;  // in case the map isn't entirely rectangular
	int *map;
	struct random_context *ctx;
};


typedef struct holder_tag
{
	int number;
	int r;
	int pLess, pMore;
} HOLDER, *PHOLDER;

static int sort( int *nHolders, PHOLDER holders, int nTree, int number, int r )
{
	PHOLDER tree = holders + nTree;
	if( nTree < 0 )
	{
		tree = holders + (*nHolders)++;
		tree->number = number;
		tree->r = r;
		tree->pLess = tree->pMore = -1;
		return (int)(tree - holders);
	}
	else
	{
		if( r > tree->r )
			tree->pMore = sort( nHolders, holders, tree->pMore, number, r );
		else
			tree->pLess = sort( nHolders, holders, tree->pLess, number, r );
	}
	return nTree;
}

static void FoldTree( int *nNumber, int *numbers, PHOLDER holders, int nTree )
{
	PHOLDER tree = holders + nTree;
	if( tree->pLess >= 0 )
		FoldTree( nNumber, numbers, holders, tree->pLess );
	numbers[(*nNumber)++] = tree->number;
	if( tree->pMore >= 0 )
		FoldTree( nNumber, numbers, holders, tree->pMore );
}

static void Shuffle( struct block_shuffle_key *key, int *numbers , int count )
{
	int tree;
	int n;
	int nHolders = 0;
	int nNumber = 0;
	int need_bits;
	PHOLDER holders = NewArray( HOLDER, count );
	tree = -1;
	nNumber = 0;
	for( n = 31; n > 0; n-- )
		if( count & ( 1 << n ) )
			break;
	need_bits = n + 1;
	for( n = 0; n < count; n++ )
		tree = sort( &nHolders, holders, tree, numbers[n], SRG_GetEntropy( key->ctx, need_bits, 0 ) );
	FoldTree( &nNumber, numbers, holders, tree );
	Release( holders );
}

static void FoldTreeBytes( int *nNumber, uint8_t *numbers, PHOLDER holders, int nTree )
{
	PHOLDER tree = holders + nTree;
	if( tree->pLess >= 0 )
		FoldTreeBytes( nNumber, numbers, holders, tree->pLess );
	numbers[(*nNumber)++] = (uint8_t)tree->number;
	if( tree->pMore >= 0 )
		FoldTreeBytes( nNumber, numbers, holders, tree->pMore );
}


static void ShuffleBytes( struct byte_shuffle_key *key, uint8_t *numbers, int count )
{
	int tree;
	int n;
	int nHolders = 0;
	int nNumber = 0;
	int need_bits;
	PHOLDER holders = NewArray( HOLDER, count );
	tree = -1;
	nNumber = 0;
	for( n = 31; n > 0; n-- )
		if( count & (1 << n) )
			break;
	need_bits = n + 1;
	for( n = 0; n < count; n++ )
		tree = sort( &nHolders, holders, tree, numbers[n], SRG_GetEntropy( key->ctx, need_bits, 0 ) );
	FoldTreeBytes( &nNumber, numbers, holders, tree );
	Release( holders );
}


struct block_shuffle_key *BlockShuffle_CreateKey( struct random_context *ctx, size_t width, size_t height )
{
	struct block_shuffle_key *key = New( struct block_shuffle_key );
	size_t n;
	key->width = width;
	key->height = height;
	key->extra = 0;
	key->map = NewArray( int, width * height );
	key->ctx = ctx;
	{
		size_t m;
		for( n = 0; n < width; n++ )
			for( m = 0; m < height; m++ )
			{
				key->map[m*width+n] = (int)(m*width+n);
			}
		Shuffle( key, key->map, (int)(width * height) );
	}
	return key;
}

void BlockShuffle_GetDataBlock( struct block_shuffle_key *key
	, uint8_t* encrypted, int x, int y, size_t w, size_t h, size_t encrypted_stride
	, uint8_t* output, int ofs_x, int ofs_y, size_t stride )
{
	size_t ix, iy;
	for( ix = 0; ix < (w); ix++ ) {
		for( iy = 0; iy < (h); iy++ ) {
			int km = key->map[ix%key->width + (iy%key->height) * key->width];
			int kmx = km % key->width;
			int kmy = (int)(km / key->width);
			((uint8_t*)( ( (uintptr_t)output ) + (ix + ofs_x ) + stride * ( iy + ofs_y ) ))[0] =
				((uint8_t*)( ( (uintptr_t)encrypted ) + (x+kmx)+(y*kmy)*encrypted_stride ))[0];
		}
	}
}


void BlockShuffle_GetData( struct block_shuffle_key *key
	, uint8_t* encrypted, size_t x, size_t w
	, uint8_t* output, size_t ofs_x )
{
	BlockShuffle_GetDataBlock( key, encrypted, (int)x, 0, w, 1, 0, output, (int)ofs_x, 0, 0 );
}

void BlockShuffle_SetDataBlock( struct block_shuffle_key *key
	, uint8_t* encrypted, int x, int y, size_t w, size_t h, size_t output_stride
	, uint8_t* input, int ofs_x, int ofs_y, size_t input_stride
)
{
	size_t ix, iy;
	for( ix = 0; ix < ( w ); ix++ )
	{
		for( iy = 0; iy < ( h ); iy++ )
		{
			int km = key->map[ix%key->width + (iy%key->height) * key->width];
			int kmx = km % key->width;
			int kmy = (int)(km / key->width);
			((uint8_t*)( ( (uintptr_t)encrypted ) + (x + kmx) + (y+kmy)*output_stride  ))[0]
				= ((uint8_t*)( ( (uintptr_t)input ) + (ix + ofs_x ) + input_stride * ( iy + ofs_y ) ))[0];
		}
	}
}


void BlockShuffle_SetData( struct block_shuffle_key *key
	, uint8_t* encrypted, int x, size_t w
	, uint8_t* input, int ofs_x )
{
	BlockShuffle_SetDataBlock( key, encrypted, x, 0, w, 1, 0
		, input, ofs_x, 0, 0 );
}

//------------------------------------------------------------------
// Byte Swap (works better than a position swap?)
//------------------------------------------------------------------

void BlockShuffle_DropByteShuffler( struct byte_shuffle_key *key ) {
	Release( key->map );
	Release( key );
}

//0, 43, 86 
//128, 171, 214

static uint8_t leftStacks[3][2] = { { 0, 43 }, {43, 43}, {86,42} };
static uint8_t rightStacks[4][2] = { { 128, 43 }, {171, 43}, {214,42} };
static uint8_t leftOrders[4][3] = { { 1, 0, 2 }, { 1, 2, 0 }, {2, 1, 0 }, {2, 0, 1 } };
static uint8_t rightOrders[4][3] = { { 0, 2, 1 }, { 2, 0, 1 }, { 1, 2, 0 }, {2, 1, 0 } };

struct halfDeck {
	int from;
	int until;
	int cut;
	uint8_t starts[3];
	uint8_t lens[3];
};

struct byte_shuffle_key *BlockShuffle_ByteShuffler( struct random_context *ctx ) {
	struct byte_shuffle_key *key = New( struct byte_shuffle_key );
	int n;
#define BLOCKSHUF_BYTE_ROUNDS 5
	uint8_t stacks[86];
	uint8_t halves[8][2];
	uint8_t lrStarts[8];
	uint8_t lrStart;
	uint8_t *readLMap;
	uint8_t *readRMap;
	uint8_t *writeMap;
	key->map = NewArray( uint8_t, 512 );
	int srcMap;

	key->dmap = key->map + 256;
	uint8_t *maps[2] = { key->dmap, key->map };
	key->ctx = ctx;

	/* 40 bits for 8 shuffles. */
	for( n = 0; n < BLOCKSHUF_BYTE_ROUNDS; n++ ) {
		halves[n][0] = SRG_GetEntropy( ctx, 2, 0 );
		halves[n][1] = SRG_GetEntropy( ctx, 2, 0 );
		lrStarts[n] = SRG_GetEntropy( ctx, 1, 0 );
	}

	int t[2] = { 0, 0 };
	SRG_GetBit_( lrStart, ctx );
	for( n = 0; (t[0] < 43 || t[1] < 43 ) && n < 86; n++ ) {
		int bit;
		int c;
		c = 1;
		while( c < (5- lrStart) && ( SRG_GetBit_( bit, ctx ), !bit ) ) {
			c++;
		}
		lrStart = !lrStart;
		stacks[n] = c;
		t[n&1] += c;
	}
	for( n = 0; n < 256; n++ )
		key->map[n] = n;
	srcMap = 1;
	for( n = 0; n < BLOCKSHUF_BYTE_ROUNDS; n++ ) {
		struct halfDeck left, right;
		int s;
		int useCards;

		left.starts[0] = leftStacks[ leftOrders[ halves[n][0] ] [0] ] [0];
		left.lens[0]   = leftStacks[ leftOrders[ halves[n][0] ] [0] ] [1];
		left.starts[1] = leftStacks[ leftOrders[ halves[n][0] ] [1] ] [0];
		left.lens[1]   = leftStacks[ leftOrders[ halves[n][0] ] [1] ] [1];
		left.starts[2] = leftStacks[ leftOrders[ halves[n][0] ] [2] ] [0];
		left.lens[2]   = leftStacks[ leftOrders[ halves[n][0] ] [2] ] [1];
		left.cut = 0;
		left.from = left.starts[left.cut];
		left.until = left.starts[left.cut] + left.lens[left.cut];

		right.starts[0] = rightStacks[ rightOrders[ halves[n][1] ] [0] ] [0];
		right.lens[0]   = rightStacks[ rightOrders[ halves[n][1] ] [0] ] [1];
		right.starts[1] = rightStacks[ rightOrders[ halves[n][1] ] [1] ] [0];
		right.lens[1]   = rightStacks[ rightOrders[ halves[n][1] ] [1] ] [1];
		right.starts[2] = rightStacks[ rightOrders[ halves[n][1] ] [2] ] [0];
		right.lens[2]   = rightStacks[ rightOrders[ halves[n][1] ] [2] ] [1];
		right.cut = 0;
		right.from = right.starts[right.cut];
		right.until = right.starts[right.cut] + right.lens[right.cut];

		lrStart = lrStarts[n];
		useCards = stacks[s=0];

		readLMap = maps[srcMap] + left.from;
		readRMap = maps[srcMap] + right.from;
		writeMap = maps[1 - srcMap];
		s = 0;
		for( int outCard = 0; outCard < 256;  ) {
			useCards = stacks[s];
			for( int c = 0; c < useCards; c++ ) {
				if( lrStart ) {
					(writeMap++)[0] = (readLMap++)[0];
					outCard++;
					left.from++;
					//maps[1 - srcMap][outCard++] = maps[srcMap][left.from++];
					if( left.from >= left.until ) {
						if( ++left.cut < 3 ) {
							s = 0;
							useCards = stacks[s];
							c = -1;

							left.from = left.starts[left.cut];
							left.until = left.starts[left.cut] + left.lens[left.cut];
							readLMap = maps[srcMap] + left.from;
						}
						while( left.cut != right.cut ) {
							(writeMap++)[0] = (readRMap++)[0];
							outCard++;
							right.from++;
							//maps[1 - srcMap][outCard++] = maps[srcMap][right.from++];
							if( right.from >= right.until ) {
								if( ++right.cut < 3 ) {
									right.from = right.starts[right.cut];
									right.until = right.starts[right.cut] + right.lens[right.cut];
									readRMap = maps[srcMap] + right.from;
								}
							}
						}
						if( s ) break;
						// L/R 2 new stacks... lrStart = same for whole stack each 3 subpart so...;
					}
				}
				else {
					(writeMap++)[0] = (readRMap++)[0];
					outCard++;
					right.from++;
					//maps[1 - srcMap][outCard++] = maps[srcMap][right.from++];
					if( right.from >= right.until ) {
						if( ++right.cut < 3 ) {
							s = 0;
							useCards = stacks[s];
							c = -1;

							right.from = right.starts[right.cut];
							right.until = right.starts[right.cut] + right.lens[right.cut];
							readRMap = maps[srcMap] + right.from;
						}
						while( left.cut != right.cut ) {
							(writeMap++)[0] = (readLMap++)[0];
							outCard++;
							left.from++;
							//maps[1 - srcMap][outCard++] = maps[srcMap][left.from++];
							if( left.from >= left.until ) {
								if( ++left.cut < 3 ) {
									left.from = left.starts[left.cut];
									left.until = left.starts[left.cut] + left.lens[left.cut];
									readLMap = maps[srcMap] + left.from;
								}
							}
						}
						if( s ) break;
						// L/R 2 new stacks... lrStart = same for whole stack each 3 subpart so...;
					}

				}
			}
			if( outCard >= 256 )
				break;
			lrStart = 1-lrStart;
			s++;
			if( s >= 86 ) {
				useCards = stacks[s=0];
			}
		}
#if 0
		// validate that each number is in the mapping only once.
		{
			uint8_t *check = maps[1 - srcMap];
			int n;
			for( n = 0; n < 256; n++ ) {
				for( int m = 0; m < 256; m++ ) {
					if( m == n ) continue;
					if( check[n] == check[m] ) {
						lprintf( "Index %d matches %d  %d", n, m, check[n] );
						DebugBreak();
					}
				}
			}
		}
#endif
		srcMap = 1 - srcMap;
	}

#if 0
	{
		for( n = 0; n < 256; n++ ) {
			int m = SRG_GetEntropy( ctx, 8, 0 );
			int t = key->map[m];
			key->map[m] = key->map[n];
			key->map[n] = t;
		}

		//ShuffleBytes( key, key->map, 256 );
	}
#endif
	for( n = 0; n < 256; n++ )
		key->dmap[key->map[n]] = n;
	return key;
}

void BlockShuffle_SubByte( struct byte_shuffle_key *key
	, uint8_t *bytes_input, uint8_t *bytes_output ) {
	bytes_output[0] = key->map[bytes_input[0]];
}

void BlockShuffle_SubBytes( struct byte_shuffle_key *key
	, uint8_t *bytes_input, uint8_t *bytes_output
	, size_t byteCount ) 
{
	size_t n;
	uint8_t *map = key->map;
	for( n = 0; n < byteCount; n++, bytes_input++, bytes_output++ ) {
		bytes_output[0] = map[bytes_input[0]];
	}
}

void BlockShuffle_BusByte( struct byte_shuffle_key *key
	, uint8_t *bytes_input, uint8_t *bytes_output ) {
	bytes_output[0] = key->dmap[bytes_input[0]];
}


void BlockShuffle_BusBytes( struct byte_shuffle_key *key
	, uint8_t *bytes_input, uint8_t *bytes_output
	, size_t byteCount ) 
{
	size_t n;
	uint8_t *map = key->dmap;
	for( n = 0; n < byteCount; n++, bytes_input++, bytes_output++ ) {
		bytes_output[0] = map[bytes_input[0]];
	}
}
