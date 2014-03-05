
typedef struct holder_tag
{
   int number;
   int r;
   struct holder_tag *pLess, *pMore;
} HOLDER, *PHOLDER;

int nHolders;
HOLDER holders[MAX_NUMS];

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
   for( n = 0; n < nMaxNums; n++ )
		tree = sort( tree, numbers[n], rand() );
   FoldTree( numbers, tree );
}

static int work_nums[MAX_NUMS];
