#include <stdhdrs.h>

static TEXTCHAR hex[] = WIDE("0123456789abcdef");
static TEXTCHAR HEX[] = WIDE("0123456789ABCDEF");

void ConvertFromBinary( TEXTCHAR *out, TEXTCHAR *in, int sz )
{
	int n;
	for( n = 0; n < sz; n++ )
	{
		out[n * 2] = hex[in[n] >> 4];
      out[n * 2 + 1] = hex[in[n] & 0xF];
	}
   out[n*2] = 0;
}

void ConvertToBinary( TEXTSTR out, CTEXTSTR in, int sz )
{
	int n;
	for( n = 0; in[0] && n < sz; n++ )
	{
		TEXTCHAR const *p1;
      int lo, hi;
		p1 = StrChr( hex, in[0] );
		if( !p1 )
		{
			p1 = StrChr( HEX, in[0] );
         if( p1 )
				hi = p1 - HEX;
			else
            lprintf( WIDE("fatal - character out of range!") );
		}
		else
			hi = p1 - hex;
		p1 = StrChr( hex, in[1] );
		if( !p1 )
		{
			p1 = StrChr( HEX, in[1] );
         if( p1 )
				lo = p1 - HEX;
			else
            lprintf( WIDE("fatal - character out of range!") );
		}
		else
			lo = p1 - hex;

		out[0] = ( hi << 4 ) + lo;

		out++;
      in += 2;
	}

	for( ;n < sz; n++ )
      out[n] = 0;


}

int GetProgramID( CTEXTSTR program)
{
   return 1;
}

