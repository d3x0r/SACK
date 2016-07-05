#include <stdhdrs.h>

char *filea;
char *fileb;
FILE *dupout;
	char filename[256];
	long totalCount = 0;

struct card_relation {
	DeclareLink( struct card_relation );
	POINTER card;
	char *file;
	_32 number;
};
typedef struct card_relation RELATION;
typedef struct card_relation *PRELATION;
#define MAXRELATIONSPERSET 4096
PRELATION related[256][256];
PRELATION relatedb[256][256];

DeclareSet( RELATION );
PRELATIONSET relations;

void openDups(  void)
{
	if( dupout )
		return;

	//snprintf( filename, 256, "dups.%s", argv[1] );
	dupout = fopen( filename, "wt" );
}

void TickLog( LOGICAL force, const char *leader, int max, int n )
{
	static _32 tick;
	static _32 prior;
	_32 del;
	if( !tick )
	{
		tick = GetTickCount();
		return;
	}
	del = GetTickCount() - tick;
	if( force || ( del > 5000 ) ) {
		tick += del;
		printf( "%5d  %s   %d  %d  (%d)\n", del, leader, n, n - prior, max - n );
		prior = n;
	}
}

POINTER OpenAFile( const char *file, PTRSZVAL *size )
{
	POINTER result = OpenSpace( NULL, file, size );
	return result;
}

struct params {
	P_8 p;
	PTRSZVAL size;
};

void classify( char *file, P_8 p, PTRSZVAL size )
{
	int n;
	for( n = 0; n < size; n += 12 )
	{
		PRELATION relation = GetFromSet( RELATION, &relations );
		relation->card = p + n;
		relation->file = file;
		relation->number = ( n / 12 ) + 1;
		PRELATION *rel = related[p[n+0]] + p[n+1];
		LinkThing( rel[0], relation );
		totalCount++;
		//TickLog( "classify", n / 12 );
	}
}

PTRSZVAL CPROC read_ahead( PTHREAD thread )
{
	struct params  * p = (struct params  *)GetThreadParam( thread );
	P_8 p_tmp = p->p;
	PTRSZVAL n;
	int a;
	for( n = 0; n <p->size; n += 4096 ) {
		a = p_tmp[0];
		p_tmp += 4096;
	}
	return 0;
}

void self_compare( char *file, POINTER p, PTRSZVAL size )
{
	int n, m;
	P_64 p64a = (P_64)p;
	P_32 p32a = (P_32)(p64a+1);
	for( n = 0; n < size; n += 12 )
	{
		P_64 p64b = (P_64)(p32a + 1);
		P_32 p32b = (P_32)(p64b + 1);
		TickLog( FALSE, "self", size, n);
		for( m = n + 12; m < size; m += 12 )
		{
			if( p64a[0] == p64b[0] &&
				p32a[0] == p32b[0] ) {
						openDups();
				fprintf( dupout, "overlap on %s %d", file, ( n / 12 ) + 1 );
			}
			p64b = (P_64)(p32b + 1);
			p32b = (P_32)(p64b + 1);
		}
		p64a = (P_64)(p32a + 1);
		p32a = (P_32)(p64a + 1);
	}


}

void other_compare( struct params *pa, struct params *pb )
{
	int n, m;
	P_64 p64a = (P_64)pa->p;
	P_32 p32a = (P_32)(p64a + 1);
	for( n = 0; n < pa->size; n += 12 )
	{
		P_64 p64b = (P_64)(pb->p + (12*(n+1)));
		P_32 p32b = (P_32)(p64b + 1);
		TickLog( FALSE, "other", pa->size, n );
		for( m = n + 12; m < pb->size; m++ )
		{
			if( p64a[0] == p64b[0] &&
				p32a[0] == p32b[0] ) {
						openDups();
				fprintf( dupout, "overlap on %s %d and %s %d", filea, (n / 12) + 1, fileb, (m/12)+1 );
				return TRUE;
			}
			p64b = (P_64)(p32b + 1);
			p32b = (P_32)(p64b + 1);
		}
		p64a = (P_64)(p32a + 1);
		p32a = (P_32)(p64a + 1);
	}
}


LOGICAL classify_compare( LOGICAL have_related_b )
{
	int total = 0;
	int i, j;
	for( i = 0; i < 256; i++ ) {
		TickLog( FALSE, "class compare", totalCount, total );
		for( j = 0; j < 256; j++ )
		{
			PRELATION a = related[i][j];
			while( a && !( (have_related_b) && ( a == relatedb[i][j] ) ) ) {
				PRELATION b = have_related_b?relatedb[i][j] :a->next;
				while( a && b )
				{
					P_64 p64a = (P_64)a->card;
					P_32 p32a = (P_32)(p64a + 1);
					P_64 p64b = (P_64)b->card;
					P_32 p32b = (P_32)(p64b + 1);
					if( p64a[0] == p64b[0] &&
						p32a[0] == p32b[0] ) {
						openDups();
						fprintf( dupout, "overlap on %s %d and %s %d\n", a->file, a->number, b->file, b->number );
					}
					b = b->next;
				}
				total++;
				a = a->next;
			}
		}
	}
}


int main( int argc, char **argv )
{
	struct params pa;
	struct params pb;
	snprintf( filename, 256, "dups.%s", argv[1] );
	TickLog( FALSE, "Start", 0, 0 );
	if( argc < 3 ) {
		printf( "usage : %s <result ID> <cardfile1> <cardfile2>\n", argv[0] );
		printf( " or %s <result ID> <cardfile1>", argv[0] );
		return 0;
	}
	pa.size = 0;
	pa.p = (P_8)OpenAFile( filea = argv[2], &pa.size );
	ThreadTo( read_ahead, (PTRSZVAL)&pa );
	classify( filea, pa.p, pa.size );
	TickLog( TRUE, "A Loaded", pa.size, 0 );
	if( argc > 3 && argv[3] ) {
		pb.size = 0;
		pb.p = (P_8)OpenAFile( fileb = argv[3], &pb.size );
		ThreadTo( read_ahead, (PTRSZVAL)&pb );
		memcpy( relatedb, related, sizeof( related ) );
		classify( fileb, pb.p, pb.size );
		TickLog( TRUE, "B Loaded", pb.size, 0 );
	}
	//else self_compare( filea, pa.p, pa.size );

	//other_compare( &pa, &pb );
	classify_compare( argc > 3 );


	return 0;
}


