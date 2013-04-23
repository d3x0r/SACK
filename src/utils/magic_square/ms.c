#define DO_LOGGING
#include <stdhdrs.h>

struct ComboMaker
{
	int Size;
   int length;
   int *phases;
	long long step;
	int *numbers;  // which number's we're using to fill...
   long long *facts;
};

//int initnums[] = {1, 2,  3, 12, 16, 14,  4, 13, 10,  5,  8, 11,  9, 15,  6,  7};
int initnums[] = {1, 2,  3, 4 , 5 , 6 ,  7, 8 , 9 ,10 , 11, 12, 16, 14, 15, 13};

struct ComboMaker *MakeCombinationIterator( int Size )
{
	struct ComboMaker *maker = New( struct ComboMaker );
   int n;
	maker->Size = Size;
	maker->length = Size*Size;
	maker->phases = NewArray( int, maker->length );
	maker->step = 0;
	maker->numbers = NewArray( int, maker->length );
	maker->facts = NewArray(long long, maker->length );

	for( n = 0; n < maker->length; n++ )
	{
		maker->numbers[n] = n+1;
		maker->phases[n] = 0;
	}
	for( n = 0; n < maker->length; n++ )

   maker->facts[0] = 1;
   maker->facts[1] = 1;
 	for( n = 2; n < maker->length; n++ )
	{
      maker->facts[n] = maker->facts[n-1] * n;
	}
   return maker;
}

void IterateCombination( struct ComboMaker *maker )
{
	int level;
	int length = maker->length;
	int *nums = maker->numbers;
   int *phases = maker->phases;

	if( 0 )
	{
		for( level = 0; level < length; level++ )
			printf( "%2d %2d ", phases[level], level );
		printf( "\n" );
	}

	for( level = 1; level < length; level++ )
	{
		phases[level]++;
		if( phases[level] <= level )
			break;
		else
			phases[level] = 0;
	}

	for( level = 0; level < length; level++ )
      nums[level] = level;

	for( level = maker->length-1; level >= 1; level-- )
	{
		int m;
		int tmp;
		for( m = level-phases[level]; m < level; m++ )
		{
			tmp = nums[m];
			nums[m] = nums[m+1];
			nums[m+1] = tmp;
		}
		if(0)
		{
         int t;
			printf( "l%02d : ", level );

			for( t= 0; t < 16; t++ )
				printf( "%2d ", nums[t] );
			printf( "\n" );
		}
	}
}

int _is_valid( struct ComboMaker *maker )
{
	int n;
	int m;
	int total;
   int Size;
   int *numbers = maker->numbers;
	if( !((numbers[1] == 9) || (numbers[4] == 9) || (numbers[5] == 9) || (numbers[6] == 10) || (numbers[0] == 12) || (numbers[1] == 12) || (numbers[2] == 12) ) )
      return 0;
   Size = maker->Size;

	{
		int sum = 0;
		for( m = 0; m < maker->Size; m++ )
		{
         sum += numbers[m+m*4];
		}
      total = sum;
	}

	{
		int sum = 0;
		for( m = 0; m < Size; m++ )
		{
         sum += numbers[((Size-1)-m)+m*4];
		}
		if( sum != total )
			return 0;
	}
	for( n = 0; n < Size; n++ )
	{
		int sum = 0;
		for( m = 0; m < Size; m++ )
		{
         sum += numbers[m*4+n];
		}
		if( sum != total )
			return 0;
	}
	for( n = 0; n < Size; n++ )
	{
		int sum = 0;
		for( m = 0; m < Size; m++ )
		{
         sum += numbers[m+n*4];
		}
		if( sum != total )
			return 0;
	}
   return 1;
}



#define size 4

int numbers[size*size];  // which number's we're using to fill...


int is_valid( void )
{
	int n;
	int m;
	int total = 0;

	{
		int sum = 0;
		for( m = 0; m < size; m++ )
		{
         sum += numbers[m+m*4];
		}
      total = sum;
	}

	{
		int sum = 0;
		for( m = 0; m < size; m++ )
		{
         sum += numbers[((size-1)-m)+m*4];
		}
		if( sum != total )
			return 0;
	}
	for( n = 0; n < size; n++ )
	{
		int sum = 0;
		for( m = 0; m < size; m++ )
		{
         sum += numbers[m*4+n];
		}
		if( sum != total )
			return 0;
	}
	for( n = 0; n < size; n++ )
	{
		int sum = 0;
		for( m = 0; m < size; m++ )
		{
         sum += numbers[m+n*4];
		}
		if( sum != total )
			return 0;
	}


}


void ShiftOne( int n, int distance )
{
	int m;
   int tmp;
	for( m = n + distance; m > n; m-- )
	{
		tmp = numbers[m];
		numbers[m] = numbers[m-1];
      numbers[m-1] = tmp;
	}
}

void ShiftBack( int n, int distance )
{
	int m;
   int tmp;
	for( m = n; m < (n + distance); m++ )
	{
		tmp = numbers[m];
		numbers[m] = numbers[m+1];
      numbers[m+1] = tmp;
	}
}

long long facts[size*size];

long long factorial(int n )
{
	if( facts[0] == 0 )
	{
		int z;
		facts[0] = 1;
		facts[1] = 1;
		for( z = 2; z < size*size; z++ )
         facts[z] = facts[z-1]*z;
	}
   return facts[n];
}

int ProcessPhaseBack( int *phase, long long step, int level )
{

	if( ( step % factorial(level) ) == 0 )
	{
		ShiftBack( size*size-1-level, ( ((*phase)-1) % (level+1) ) );
      return 1;
	}
   return 0;
}

int MaxPhase( long long step, int level )
{
	if( ( step % factorial(level) ) == 0 )
		return 1;
	return 0;
}



void ProcessPhase( int *phase, long long step, int level )
{
	if( step && ( ( step % factorial(level) ) == 0 ) )
	{
      //printf( "Shift %d %d\n", size*size-1-level, ( (*phase) % (level+1) ));
		ShiftOne( size*size-1-level, ( (*phase) % (level+1) ) );
		(*phase)++;
      // keep these low to avoid integer wraps for long combinations
		if( (*phase) == (level+2) )
         (*phase) = 1;
	}
}

void iterate( void )
{
   static int phases[size*size];
	static long long step;

	int level;
	if( step && !(step & 1 ))
	{
		int tmp ;
		tmp = numbers[size*size-1];
		numbers[size*size-1] = numbers[size*size-2];
		numbers[size*size-2] = tmp;
		// undoes prior shift.
	}

	for( level = 2; level < size*size; level++ )
	{
		if( !ProcessPhaseBack( phases+level, step, level ) )
			break;
		if(0)
		{
			int t;
			printf( "L%02d : ", level );

			for( t= 0; t < 16; t++ )
				printf( "%2d ", numbers[t] );
			printf( "\n" );
		}
	}

	// every one will be divisible by all below at the same time
   //
	for( level = 2; level < (size*size-1); level++ )
		if( !MaxPhase( step, level ) )
          break;

	for( /*level = (size*size-1)*/; level >= 2; level-- )
	{
		ProcessPhase( phases+level, step, level );
		if(0)
		{
         int t;
			printf( "L%02d : ", level );

			for( t= 0; t < 16; t++ )
				printf( "%2d ", numbers[t] );
			printf( "\n" );
		}
	}

   if( step & 1 )
	{
		int tmp ;
		tmp = numbers[size*size-1];
		numbers[size*size-1] = numbers[size*size-2];
		numbers[size*size-2] = tmp;
	}
   step++;
}

int main ( void )
{
	long long n;
   int tick = 0;
	struct ComboMaker *maker = MakeCombinationIterator( size );

	for( n = 0; n < size*size; n++ )
	{
      numbers[n] = n+1;
	}

	{
      int count = 0;
		int zz = GetTickCount();
		for( n = 0; ; n++ )
		{
			int m;
			IterateCombination( maker );

			if( _is_valid(maker) )
			{
				printf( "** %17lld - ", n );
				for( m = 0; m < maker->length; m++ )
				{
					printf( "%2d ", maker->numbers[m] );
				}
				printf( "\n" );
				fflush( stdout );
			}
			else
			{
				if( count > 100000 )
				{
               count = 0;
					if( ( zz + 2000 ) < GetTickCount() )
					{
						zz += 2000;
						printf( "%17lld - ", n );
						for( m = 0; m < maker->length; m++ )
						{
							printf( "%2d ", maker->numbers[m] );
						}
						printf( "\n" );
						//if(0)
						{
							printf( "+%16lld - ", maker->step );
							for( m = 0; m < maker->length; m++ )
							{
								printf( "%2d ", maker->phases[m] );
							}
							printf( "\n" );
						}
						fflush( stdout );
					}
				}
            count++;
			}
		}
	}

   return 0;
}

