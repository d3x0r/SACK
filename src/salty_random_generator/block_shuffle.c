
#include <stdhdrs.h>

#include "salty_generator.h"

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
   struct holder_tag *pLess, *pMore;
} HOLDER, *PHOLDER;

int nHolders;

PHOLDER sort( int *nHolders, PHOLDER holders, PHOLDER tree, int number, int r )
{
   if( !tree )
   {
      tree = holders + (*nHolders)++;
      tree->number = number;
      tree->r = r;
      tree->pLess = tree->pMore = NULL;
   }
   else
   {
      if( r > tree->r )
         tree->pMore = sort( nHolders, holders, tree->pMore, number, r );
      else
         tree->pLess = sort( nHolders, holders, tree->pLess, number, r );
   }
   return tree;
}

void FoldTree( int *nNumber, int *numbers, PHOLDER tree )
{
   if( tree->pLess )
      FoldTree( nNumber, numbers, tree->pLess );
   numbers[(*nNumber)++] = tree->number;
   if( tree->pMore )
      FoldTree( nNumber, numbers, tree->pMore );
}

void Shuffle( struct block_shuffle_key *key, int *numbers , int count )
{
	PHOLDER tree;
	int n;
	int nHolders = 0;
	int nNumber = 0;
   int need_bits;
	PHOLDER holders = NewArray( HOLDER, count );
	tree = NULL;
	nNumber = 0;
	for( n = 31; n > 0; n-- )
		if( count & ( 1 << n ) )
			break;
   need_bits = n + 1;
   for( n = 0; n < count; n++ )
		tree = sort( &nHolders, holders, tree, numbers[n], SRG_GetEntropy( key->ctx, need_bits, 0 ) );
   FoldTree( &nNumber, numbers, tree );
   Release( holders );
}


struct block_shuffle_key *BlockShuffle_CreateKey( size_t width, size_t height )
{
	struct block_shuffle_key *key = New( struct block_shuffle_key );
   int n;
	key->width = width;
	key->height = height;
	key->extra = 0;
	key->map = NewArray( int, width * height );
	{
		int m;
		for( n = 0; n < width; n++ )
			for( m = 0; m < height; m++ )
			{
				key->map[m*width+n] = m*width+n;
			}
      Shuffle( key, key->map, width * height );
	}
   return key;
}

void BlockShuffle_GetDataBlock( struct block_shuffle_key *key, POINTER encrypted, size_t x, size_t y, size_t w, size_t h, POINTER output, size_t ofs_x, size_t ofs_y, size_t stride )
{
	size_t ix, iy;
	for( ix = x; ix < ( x + w ); ix++ )
	{
		for( iy = y; iy < ( y + h ); iy++ )
		{
			((P_8)( ( (PTRSZVAL)output ) + (ix - ofs_x ) + stride * ( iy - ofs_y ) ))[0] =
				((P_8)( ( (PTRSZVAL)encrypted ) + key->map[ix + iy * key->width ] ))[0];
		}
	}
}


void BlockShuffle_GetData( struct block_shuffle_key *key, POINTER encrypted, size_t x, size_t y, size_t w, POINTER output, size_t ofs_x )
{
   BlockShuffle_GetDataBlock( key, encrypted, x, y, w, 1, output, ofs_x, 0, w );
}

void BlockShuffle_SetDataBlock( struct block_shuffle_key *key, POINTER encrypted, size_t x, size_t y, size_t w, size_t h, POINTER input, size_t ofs_x, size_t ofs_y, size_t stride )
{
	size_t ix, iy;
	for( ix = x; ix < ( x + w ); ix++ )
	{
		for( iy = y; iy < ( y + h ); iy++ )
		{
			((P_8)( ( (PTRSZVAL)encrypted ) + key->map[ix + iy * key->width] ))[0]
				= ((P_8)( ( (PTRSZVAL)input ) + (ix - ofs_x ) + stride * ( iy - ofs_y ) ))[0];
		}
	}
}


void BlockShuffle_SetData( struct block_shuffle_key *key, POINTER encrypted, size_t x, size_t y, size_t w, POINTER input, size_t ofs_x )
{
   BlockShuffle_SetDataBlock( key, encrypted, x, y, w, 1, input, ofs_x, 0, w );
}

