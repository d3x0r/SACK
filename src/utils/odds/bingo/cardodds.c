
#include <stdhdrs.h>

#define MAX_NUMS 80
int nMaxNums;
int tick;
int column_groups;

#include "shuffle.c"

int states_10 [][5] = /* 10 */
{ { 5, 5, 0, 0, 0 } // only for 80% of the cards

, { 5, 4, 1, 0, 0 }, { 4, 5, 1, 0, 0 }, { 4, 1, 5, 0, 0 }
, { 5, 1, 4, 0, 0 }, { 1, 5, 4, 0, 0 }, { 1, 4, 5, 0, 0 }

, { 5, 3, 2, 0, 0 }, { 3, 5, 2, 0, 0 }
, { 5, 2, 3, 0, 0 }, { 2, 5, 3, 0, 0 }

, { 4, 4, 2, 0, 0 }, { 4, 2, 4, 0, 0 }

, { 5, 3, 1, 1, 0 }, { 3, 5, 1, 1, 0 }, { 3, 1, 5, 1, 0 }, { 3, 1, 1, 5, 0 }
, { 5, 1, 3, 1, 0 }, { 1, 5, 3, 1, 0 }, { 1, 3, 5, 1, 0 }, { 1, 3, 1, 5, 0 }
, { 5, 1, 1, 3, 0 }, { 1, 5, 1, 3, 0 }, { 1, 1, 5, 3, 0 }, { 1, 1, 3, 5, 0 }

, { 5, 1, 2, 2, 0 }, { 1, 5, 2, 2, 0 }, { 1, 2, 5, 2, 0 }, { 1, 2, 2, 5, 0 }
, { 5, 2, 1, 2, 0 }, { 2, 5, 1, 2, 0 }, { 2, 1, 5, 2, 0 }, { 2, 1, 2, 5, 0 }
, { 5, 2, 2, 1, 0 }, { 2, 5, 2, 1, 0 }, { 2, 2, 5, 1, 0 }, { 2, 2, 1, 5, 0 }

, { 4, 4, 1, 1, 0 }, { 1, 4, 4, 1, 0 }
, { 4, 1, 4, 1, 0 }, { 1, 4, 1, 4, 0 }
, { 4, 1, 1, 4, 0 }, { 1, 1, 4, 4, 0 }

, { 4, 3, 2, 1, 0 }, { 3, 4, 2, 1, 0 }, { 3, 2, 4, 1, 0 }, { 3, 2, 1, 4, 0 }
, { 4, 2, 3, 1, 0 }, { 2, 4, 3, 1, 0 }, { 2, 3, 4, 1, 0 }, { 2, 3, 1, 4, 0 }
, { 4, 1, 3, 2, 0 }, { 1, 4, 3, 2, 0 }, { 1, 3, 4, 2, 0 }, { 1, 3, 2, 4, 0 }
, { 4, 1, 2, 3, 0 }, { 1, 4, 2, 3, 0 }, { 1, 2, 4, 3, 0 }, { 1, 2, 3, 4, 0 }
, { 4, 3, 1, 2, 0 }, { 3, 4, 1, 2, 0 }, { 3, 1, 4, 2, 0 }, { 3, 1, 2, 4, 0 }
, { 4, 2, 1, 3, 0 }, { 2, 4, 1, 3, 0 }, { 2, 1, 4, 3, 0 }, { 2, 1, 3, 4, 0 }

, { 4, 2, 2, 2, 0 }, { 2, 4, 2, 2, 0 }, { 2, 2, 4, 2, 0 }, { 2, 2, 2, 4, 0 }

, { 3, 3, 3, 1, 0 }, { 3, 3, 1, 3, 0 }, { 3, 1, 3, 3, 0 }, { 1, 3, 3, 3, 0 }
, { 3, 3, 2, 2, 0 }, { 3, 2, 3, 2, 0 }, { 3, 2, 2, 3, 0 }
, { 2, 3, 2, 3, 0 }, { 2, 2, 3, 3, 0 }, { 2, 3, 2, 3, 0 }


, { 5, 2, 1, 1, 1 }, { 2, 5, 1, 1, 1 }, { 2, 1, 1, 5, 1 }, { 2, 1, 1, 1, 5 }
, { 5, 1, 2, 1, 1 }, { 1, 5, 2, 1, 1 }, { 1, 2, 1, 5, 1 }, { 1, 2, 1, 1, 5 }
, { 5, 1, 1, 2, 1 }, { 1, 5, 1, 2, 1 }, { 1, 1, 2, 5, 1 }, { 1, 1, 2, 1, 5 }
, { 5, 1, 1, 1, 2 }, { 1, 5, 1, 1, 2 }, { 1, 1, 1, 5, 2 }, { 1, 1, 1, 2, 5 }


								 , { 4, 3, 1, 1, 1 }, { 3, 4, 1, 1, 1 }, { 3, 1, 4, 1, 1 }, { 3, 1, 1, 4, 1 }, { 3, 1, 1, 1, 4 }
								 , { 4, 1, 3, 1, 1 }, { 1, 4, 3, 1, 1 }, { 1, 3, 4, 1, 1 }, { 1, 3, 1, 4, 1 }, { 1, 3, 1, 1, 4 }
								 , { 4, 1, 1, 3, 1 }, { 1, 4, 1, 3, 1 }, { 1, 1, 4, 3, 1 }, { 1, 1, 3, 4, 1 }, { 1, 1, 3, 1, 4 }
								 , { 4, 1, 1, 1, 3 }, { 1, 4, 1, 1, 3 }, { 1, 1, 4, 1, 3 }, { 1, 1, 1, 4, 3 }, { 1, 1, 1, 3, 4 }

								 , { 4, 2, 2, 1, 1 }, { 2, 4, 2, 1, 1 }, { 2, 2, 4, 1, 1 }, { 2, 2, 1, 4, 1 }, { 2, 2, 1, 1, 4 }
								 , { 4, 2, 1, 2, 1 }, { 2, 4, 1, 2, 1 }, { 2, 1, 4, 2, 1 }, { 2, 1, 2, 4, 1 }, { 2, 1, 2, 1, 4 }
								 , { 4, 1, 2, 2, 1 }, { 1, 4, 2, 2, 1 }, { 1, 2, 4, 2, 1 }, { 1, 2, 2, 4, 1 }, { 1, 2, 2, 1, 4 }
								 , { 4, 1, 1, 2, 2 }, { 1, 4, 1, 2, 2 }, { 1, 1, 4, 2, 2 }, { 1, 1, 2, 4, 2 }, { 1, 1, 2, 2, 4 }
								 , { 4, 2, 1, 1, 2 }, { 2, 4, 1, 1, 2 }, { 2, 1, 4, 1, 2 }, { 2, 1, 1, 4, 2 }, { 2, 1, 1, 2, 4 }
								 , { 4, 1, 2, 1, 2 }, { 1, 4, 2, 1, 2 }, { 1, 2, 4, 1, 2 }, { 1, 2, 1, 4, 2 }, { 1, 2, 1, 2, 4 }

								 , { 2, 3, 3, 1, 1 }, { 3, 2, 3, 1, 1 }, { 3, 3, 2, 1, 1 }, { 3, 3, 1, 2, 1 }, { 3, 3, 1, 1, 2 }
								 , { 2, 3, 1, 1, 3 }, { 3, 2, 1, 1, 3 }, { 3, 1, 2, 1, 3 }, { 3, 1, 1, 2, 3 }, { 3, 1, 1, 3, 2 }
								 , { 2, 3, 1, 3, 1 }, { 3, 2, 1, 3, 1 }, { 3, 1, 2, 3, 1 }, { 3, 1, 3, 2, 1 }, { 3, 1, 3, 1, 2 }
								 , { 2, 1, 3, 1, 3 }, { 1, 2, 3, 1, 3 }, { 1, 3, 2, 1, 3 }, { 1, 3, 1, 2, 3 }, { 1, 3, 1, 3, 2 }
								 , { 2, 1, 3, 3, 1 }, { 1, 2, 3, 3, 1 }, { 1, 3, 2, 3, 1 }, { 1, 3, 3, 2, 1 }, { 1, 3, 3, 1, 2 }
								 , { 2, 1, 1, 3, 3 }, { 1, 2, 1, 3, 3 }, { 1, 1, 2, 3, 3 }, { 1, 1, 3, 2, 3 }, { 1, 1, 3, 3, 2 }


								 , { 3, 2, 2, 2, 1 }, { 2, 3, 2, 2, 1 }, { 2, 2, 3, 2, 1 }, { 2, 2, 2, 3, 1 }, { 2, 2, 2, 1, 3 }
								 , { 3, 2, 2, 1, 2 }, { 2, 3, 2, 1, 2 }, { 2, 2, 3, 1, 2 }, { 2, 2, 1, 3, 1 }, { 2, 2, 1, 2, 3 }
								 , { 3, 2, 1, 2, 2 }, { 2, 3, 1, 2, 2 }, { 2, 1, 3, 2, 2 }, { 2, 1, 2, 3, 1 }, { 2, 1, 2, 2, 3 }
								 , { 3, 1, 2, 2, 2 }, { 1, 3, 2, 2, 2 }, { 1, 2, 3, 2, 2 }, { 1, 2, 2, 3, 1 }, { 1, 2, 2, 2, 3 }


									, { 2, 2, 2, 2, 2 } };



int states[][20][5] = {
	{ { 1, 0, 0, 0, 0 } }
							 , { { 2, 0, 0, 0, 0 }, { 1, 1, 0, 0, 0 } }

							 , { { 3, 0, 0, 0, 0 }, { 2, 1, 0, 0, 0 }, { 1, 2, 0, 0, 0 }, { 1, 1, 1, 0, 0 } }


/*4*/  					 , { { 4, 0, 0, 0, 0 }
								, { 3, 1, 0, 0, 0 }, { 1, 3, 0, 0, 0 }
								, { 2, 2, 0, 0, 0 }, { 2, 1, 1, 0, 0 }, { 1, 2, 1, 0, 0 }, { 1, 1, 2, 0, 0 }
								, { 1, 1, 1, 1, 0 } }


/*5*/							 , { { 5, 0, 0, 0, 0 }, { 4, 1, 0, 0, 0 }, { 1, 4, 0, 0, 0 }, { 3, 2, 0, 0, 0 }
								, { 2, 3, 0, 0, 0 }, { 3, 1, 1, 0, 0 }, { 1, 3, 1, 0, 0 }, { 1, 1, 3, 0, 0 }
								, { 2, 1, 1, 1, 0 }, { 1, 2, 1, 1, 0 }, { 1, 1, 2, 1, 0 }, { 1, 1, 1, 2, 0 }
								, { 1, 1, 1, 1, 1 } }
/*6*/						  , { { 5, 1, 0, 0, 0 }, { 1, 5, 0, 0, 0 }
							 , { 4, 2, 0, 0, 0 }, { 2, 4, 0, 0, 0 }, { 4, 1, 1, 0, 0 }
								 , { 1, 4, 1, 0, 0 }, { 1, 1, 4, 0, 0 }, { 3, 2, 1, 0, 0 }, { 2, 3, 1, 0, 0 }
								 , { 2, 1, 3, 0, 0 }, { 3, 1, 2, 0, 0 }, { 3, 1, 1, 1, 0 }
								 , { 1, 3, 1, 1, 0 }, { 1, 1, 3, 1, 0 }, { 1, 1, 1, 3, 0 }
								 , { 2, 1, 1, 1, 1 }, { 1, 2, 1, 1, 1 }, { 1, 1, 2, 1, 1 }, { 1, 1, 1, 2, 1 }
								 , { 1, 1, 1, 1, 2 }
							  }

/* 7 incomplete*/						  , { { 5, 2, 0, 0, 0 }, { 2, 5, 0, 0, 0 }, { 5, 1, 1, 0, 0 }
							 , { 4, 3, 0, 0, 0 }, { 3, 4, 0, 0, 0 }
							 , { 4, 2, 1, 0, 0 }, { 2, 4, 1, 0, 0 }, { 2, 1, 4, 0, 0 }
							 , { 4, 1, 2, 0, 0 }, { 1, 4, 2, 0, 0 }, { 1, 2, 4, 0, 0 }
							 , { 4, 1, 1, 1, 0 }, { 1, 4, 1, 1, 0 }, { 1, 1, 4, 1, 0 }, { 1, 1, 1, 4, 0 }
							 , { 0, 0, 0, 0, 0 } }


/*23*/								, { { 4, 5, 4, 5, 5 }, { 5, 4, 4, 5, 5 }, { 5, 5, 3, 5, 5 }, { 5, 5, 4, 4, 5 }, { 5, 5, 4, 5, 4 } }
	/*24*/								, { { 5, 5, 4, 5, 5 } }
							  };
int stats[75][24];
int n_s[5];
int _n_s[5];

int stats_marks[MAX_NUMS];
int _stats_marks[MAX_NUMS];

#define CARDS 3000
unsigned int cards[CARDS][5][5];
int called_nums;
int chosen_nums;
//unsigned int cards[CARDS][5][5];


void DrawRandomNumbers2( int *nums )
{
	int n;
	static int *work_nums;
	if( !work_nums )
	{
		work_nums = NewArray( int, nMaxNums );
		for( n = 0; n < nMaxNums; n++ )
		{
			work_nums[n] = n;
		}
	}

	Shuffle( work_nums );

	for( n = 0; n < nMaxNums; n++ )
	{
		nums[n] = work_nums[n];
	}
}

//
// result should refer to an array [sets][toget]
//  result should refer to an array [sets][cols][rows]
void GetNofMExx( int sets, int toget, int fromset, int *counts, int *result )
{
	int n;
	int found[5];
	int base[5];
	static int *nums;
	//int set;
   if( !nums )
		nums = NewArray( int, nMaxNums );
	DrawRandomNumbers2( nums ); // shuffle all numbers
	//for( set = 0; set < sets; set++ )
	{
		for( n = 0; n < 5; n++ )
		{
			base[n] = counts[n];
			found[n] = 0;
		}
		for( n = 0; n < nMaxNums; n++ )
		{
			int col;
         int row_offset;
			if( fromset == 75 )
			{
            if( column_groups == 5 )
					col = ( nums[n] ) / 15;
				else
					col = 0;
			}
			else
			{
				if( column_groups == 5 )
					col = nums[n] / 16;
				else
					col = 0;
			}
			//lprintf( "%d", col );

			row_offset = found[col]/base[col];
			if( row_offset > (sets-1) )// N's can overflow...
				continue;

			if( col > 4 )
			{
				DebugBreak();
				continue; // out of range of bingo balls for some reason
			}

         //lprintf( "Store at %d + %d + %d - %d = %d", (row_offset*toget)
			//			 , (5*col)
			//			 , (found[col])
			//		 , (row_offset*5), nums[n] );

			if( fromset == 80 )
			{
				if( column_groups == 5 )
				{
               //lprintf( "... %p %d", result, col );
					result[(row_offset*toget)
							 + (5*col)
							 + (found[col]++)
							 - (row_offset*5)] = nums[n];
               //lprintf( "..." );
				}
				else
					result[(row_offset*toget)
							 + (5*col)
							 + (found[col]++)
							 - (row_offset*20)] = nums[n];
			}
			else
			{
				if( column_groups == 5 )
				{
					result[(row_offset*toget)
							 + (5*col)
							 + (found[col]++)
							 - (row_offset*5)] = nums[n];
				}
				else
				{
					result[(row_offset*toget)
							 + (5*col)
							 + (found[col]++)
							 - (row_offset*24)] = nums[n];
				}
			}
         //printf( "%d ", nums[n] );
		}
		//printf( "\n " );
	}
}

void Get24of75( int *result )
{
	int counts[5];
	if( column_groups == 5 )
	{
		counts[0] = 5;
		counts[1] = 5;
		counts[2] = 5;
		counts[3] = 5;
		counts[4] = 5;
	}
	else
      counts[0] = 24;
   GetNofMExx( 3, 25, 75, counts, result );
}

void Get25of80( int *result )
{
	int counts[5];
	if( column_groups == 5 )
	{
   counts[0] = 4;
   counts[1] = 4;
   counts[2] = 4;
   counts[3] = 4;
	counts[4] = 4;
	}
	else
      counts[0] = 20;
   GetNofMExx( 3, 25, 80, counts, result );
}



// column... first column will be 1 of 5
//   second column will be the same column at a lesser odd than the others and the others at a chance
//   third column has 3 states
//   fourth column has 4 states

static int win_marks;

void calc_card_odds( void )
{
	int n;
   int total = 0;
	int prior_total = 0;
   int prior_n;
	for( n = 0; n < 1000000000; n++ )
	{
		Shuffle( work_nums );
		{
			int accum[5];
         int columns[5];
			int i;
         int used;
			int m;
         int marks;
			int win = 1;
			static int crd;
            int n_marked = 0;

			crd++;
			if( crd >= CARDS )
            crd = 0;

			marks = 0;

			used = 0;

			for( m = 0; m < called_nums; m++ )
			{
				int r;
            int num = work_nums[m];
				int col;
				if( nMaxNums == 75 )
				{
					if( column_groups == 5 )
					{
						col = work_nums[m] / 15;
						if( col == 2  )
							for( r = 0; r < 4; r++ )
							{
								//lprintf( "Is %d %d", cards[crd][col][r],num );
								if( cards[crd][col][r] == num )
								{
									n_marked++;
									marks++;
									break;
								}
							}
						else
							for( r = 0; r < 5; r++ )
							{
								//lprintf( "Is %d %d", cards[crd][col][r],num );
								if( cards[crd][col][r] == num )
								{
									marks++;
									break;
								}
							}
					}
					else
					{
						col = 0;
						for( r = 0; r < chosen_nums; r++ )
						{
							//lprintf( "Is %d %d %d", r, cards[crd][col][r],num );
							if( cards[crd][col][r] == num )
							{
								marks++;
								break;
							}
						}
					}
				}
				else
				{
					col = 0;
					if( column_groups == 5 )
					{
						col = num / 16;
						for( r = 0; r < 4; r++ )
						{
							//lprintf( "Is %d %d %d", r, cards[crd][col][r],num );
							if( cards[crd][col][r] == num )
							{
								marks++;
								break;
							}
						}
					}
					else
					{
						for( r = 0; r < chosen_nums; r++ )
						{
							//lprintf( "Is %d %d %d", r, cards[crd][col][r],num );
							if( cards[crd][col][r] == num )
							{
								marks++;
								break;
							}
						}
					}
			   }
				if(0)
				{
					for( i = 0; i < used; i++ )
					{
						if( columns[i] == col )
							break;
					}
					if( i == used )
					{
						columns[i] = col;
						used++;
					}
					if( ( ( col == 2 ) && ( accum[columns[i]] < 4 ) )||
						( accum[columns[i]] < 5 ) )
					{
						accum[columns[i]]++;
						//win = 0;
						//break;
					}
				}
			}
#if 0
			for( m = 0; m < called_nums; m++ )
			{
            printf( "%d,", work_nums[m] );
			}
         printf( "\n" );
			for( m = 0; m < chosen_nums; m++ )
			{
				printf( "%d,", cards[crd][0][m] );
			}
         printf( "\n" );
#endif
			//printf( "Card marked %d\n", marks );
			//lprintf( "Card marked %d", marks );

         n_s[n_marked]++;
			stats_marks[marks]++;

			if( marks < win_marks )
				continue;

         // valid 10 marks.
         total++;

			if( !tick )
				tick = GetTickCount();
			else
			{
				if( tick + 2000 < GetTickCount() )//( total % 100000 ) == 0 )
				{
               fprintf( stderr, "%d of %d of %d %g\n", total, n, 1000000000, (float)n/1000000000.0 );
               tick = GetTickCount();
					if( prior_total )
					{
						printf( "%d,%d,%g, ", total - prior_total, n - prior_n, (double)(n-prior_n)/(float)(total-prior_total) );
					}
					printf( "%d,%d,%g", total, n+1, (double)(n+1)/(float)total );
               //lprintf( "%d", chosen_nums );
					{
						int nm;
						for( nm = 0; nm <= chosen_nums; nm++ )
						{
							printf( ",%d,", stats_marks[nm] );
						}
						for( nm = 0; nm <= chosen_nums; nm++ )
						{
							printf( ",%d,", stats_marks[nm] - _stats_marks[nm] );
                     _stats_marks[nm] = stats_marks[nm];
						}
                  printf( "," );
						for( nm = 0; nm < 5; nm++ )
							printf( ",,%d", n_s[nm] );
						for( nm = 0; nm < 5; nm++ )
						{
							printf( ",,%d", n_s[nm]-_n_s[nm] );
                     _n_s[nm] = n_s[nm];
						}
					}
					printf( "\n" );
					prior_total = total;
					prior_n = n;
				}
			}
		}
	}

}

void usage( char **argv )
{
   printf( "%s [called_nums] [chosen_nums] [max nums] [column_groups 0/5]\n", argv[0] );
}

int main( int argc, char **argv )
{
	int n;
	if( argc > 1 )
	{
      win_marks = 0;
		//win_marks = atoi( argv[1] );
		called_nums = atoi( argv[1] );
		chosen_nums = atoi( argv[2] );
		nMaxNums = atoi( argv[3] );
      column_groups = atoi( argv[4] );
	}

	if( nMaxNums == 0 )
	{
      usage( argv );
		return 0;
	}

	for( n = 0; n < 1000; n++ )
	{
      if( nMaxNums == 75 )
			Get24of75( (int*)(cards + (n * 3)) );
      else
			Get25of80( (int*)(cards + (n * 3)) );
	}

	for( n = 0; n < nMaxNums; n++ )
		work_nums[n] = n;
	printf( "last winners, last count, last_percent, total winners, total calls, total percent\n" );
	printf( "0,0,0," );

   calc_card_odds();

   return 0;
}
