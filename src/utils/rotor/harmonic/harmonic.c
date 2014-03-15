#include <stdhdrs.h>

#define r1 16
#define r2 15

static struct {
	_32 _8rotor[r1];
	_32 _9rotor[r2];

   _32 degrees_offset;
} l;


void Matches( void )
{
	int n,m;
	for( n = 0; n < r1; n++ )
		for( m = 0; m < r2; m++ )
		{
			if( l._8rotor[n] == ((l._9rotor[m]+l.degrees_offset)%360) )
			{
            lprintf( "stator %d and rotor %d align at offset %d", n, m, l.degrees_offset );
			}
		}
}

int main( void )
{
	int n;
	for( n = 0; n < r1; n++ )
	{
		l._8rotor[n] = (n*360)/r1;
      lprintf( "rotor %d = %d", n, l._8rotor[n] );
	}
	for( n = 0; n < r2; n++ )
	{
      l._9rotor[n] = (n*360)/r2;
      lprintf( "stator %d = %d", n, l._9rotor[n] );
	}
	for( n = 0; n < 360; n ++ )
	{
		l.degrees_offset = n;
      Matches();
	}
   return 0;
}

