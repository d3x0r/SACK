#include <stdhdrs.h>



typedef struct holder_tag
{
   int number;
   int r;
   struct holder_tag *pLess, *pMore;
} HOLDER, *PHOLDER;

int nHolders;
HOLDER holders[75];

PHOLDER sort( PHOLDER tree, int number, int r )
{
   if( !tree )
   {
      tree = holders + (nHolders++);
      tree->number = number;
      tree->r = r;
      tree->pLess = tree->pMore = NULL;
   }
   else
   {
      if( r > tree->r )
         tree->pMore = sort( tree->pMore, number, r );
      else
         tree->pLess = sort( tree->pLess, number, r );
   }
   return tree;
}

int nNumber;
void FoldTree( int *numbers, PHOLDER tree )
{
   if( tree->pLess )
      FoldTree( numbers, tree->pLess );
   numbers[nNumber++] = tree->number;
   if( tree->pMore )
      FoldTree( numbers, tree->pMore );
}

void Shuffle( int *numbers )
{
	PHOLDER tree;
   int n;
	tree = NULL;
	nHolders = 0;
   nNumber = 0;
   for( n = 0; n < 75; n++ )
		tree = sort( tree, numbers[n], rand() );
   FoldTree( numbers, tree );
}

static int work_nums[75];
static int nNums;
static int nums[16][75]; // temporary result.

int* DrawRandomNumbers2( void )
{
	int n;
	if( work_nums[1] != 1 )
	{
		for( n = 1; n <= 75; n++ )
		{
         work_nums[n-1] = n;
		}
	}
	Shuffle( work_nums );
	
	nNums++;
	if( nNums == 16 )
      nNums = 0;
	for( n = 0; n < 75; n++ )
	{
		nums[nNums][n] = work_nums[n];
	}
   return nums[nNums];
}
