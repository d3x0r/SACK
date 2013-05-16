
#include <stdhdrs.h>

int TestMirror( int vals1[16], int vals2[16] )
{
	int n;
   int m;
	// test 1, top-bottom
	for( n = 0; n < 4; n++ )
	{
		for( m = 0; m < 4; m++ )
		{
			if( vals1[(n*4)+m] != vals2[((3-n)*4)+m] )
            break;
		}
		if( m < 4 )
         break;
	}
	if( n < 4 )
	{
		// test 1, left-right
		for( n = 0; n < 4; n++ )
		{
			for( m = 0; m < 4; m++ )
			{
				if( vals1[(n*4)+m] != vals2[(n*4)+(3-m)] )
					break;
			}
			if( m < 4 )
				break;
		}
	}
	if( n < 4 )
	{
		// test 1, left-right and top-bottom
		for( n = 0; n < 4; n++ )
		{
			for( m = 0; m < 4; m++ )
			{
				if( vals1[(n*4)+m] != vals2[((3-n)*4)+(3-m)] )
					break;
			}
			if( m < 4 )
				break;
		}
	}
	if( n < 4 )
	{
		// test 1, left-top
		for( n = 0; n < 4; n++ )
		{
			for( m = 0; m < 4; m++ )
			{
				if( vals1[(n*4)+m] != vals2[(m*4)+(3-n)] )
					break;
			}
			if( m < 4 )
				break;
		}
	}
	if( n < 4 )
	{
		// test 1, right-top
		for( n = 0; n < 4; n++ )
		{
			for( m = 0; m < 4; m++ )
			{
				if( vals1[(n*4)+m] != vals2[((3-m)*4)+(n)] )
					break;
			}
			if( m < 4 )
				break;
		}
	}
	if( n < 4 )
	{
		// test 1, rotation left-bottom
		for( n = 0; n < 4; n++ )
		{
			for( m = 0; m < 4; m++ )
			{
				if( vals1[(n*4)+m] != vals2[((3-m)*4)+(3-n)] )
					break;
			}
			if( m < 4 )
				break;
		}
	}
	if( n < 4 )
	{
		// test 1, - rotation right-bottom
		for( n = 0; n < 4; n++ )
		{
			for( m = 0; m < 4; m++ )
			{
				if( vals1[(n*4)+m] != vals2[((m)*4)+(n)] )
					break;
			}
			if( m < 4 )
				break;
		}
	}
   if( n < 4 )
		return 0;
   return 1;
}


int main( void )
{
   int lines = 0;
	static int vals[2000][16];
	static TEXTCHAR line[2000][75];
	while( fgets( line[lines], sizeof( line[0] ), stdin ) )
	{
		if( line[lines][0] == '*' )
		{
			int mirror =0;
			char *sep = strchr( line[lines], '-' );
			if( !sep )
				continue;
			sep++;
			sscanf( sep, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d"
					, vals[lines] + 0, vals[lines] + 1, vals[lines] + 2, vals[lines] + 3
					, vals[lines] + 4+ 0, vals[lines] + 4+ 1, vals[lines] + 4+ 2, vals[lines] + 4+ 3
					, vals[lines] + 8+0, vals[lines] + 8+1, vals[lines] + 8+2, vals[lines] + 8+3
					, vals[lines] + 12+0, vals[lines] + 12+1, vals[lines] + 12+2, vals[lines] + 12+3
					);
			{
				int n;
				for( n = 0; n < lines; n++ )
				{
					if( TestMirror( vals[lines], vals[n] ) )
					{
                  mirror++;
                  printf( "%d mirror of %d.\n", lines, n );
					}
				}
			}
			if( !mirror )
			{
			printf( ",%d,,,%d,%d,%d,%d\n"
                ",,,,%d,%d,%d,%d\n"
                ",,,,%d,%d,%d,%d\n"
					 ",,,,%d,%d,%d,%d\n"
                , lines
                , vals[lines][0], vals[lines][1], vals[lines][2], vals[lines][3]
                , vals[lines][4+0], vals[lines][4+1], vals[lines][4+2], vals[lines][4+3]
                , vals[lines][8+0], vals[lines][8+1], vals[lines][8+2], vals[lines][8+3]
					, vals[lines][12+0], vals[lines][12+1], vals[lines][12+2], vals[lines][12+3]
					);
			printf( ",,,,,,,\n,,,,,,,\n,,,,,,,\n" );
			lines++;
			}
		}
	}
   return 0;
}

