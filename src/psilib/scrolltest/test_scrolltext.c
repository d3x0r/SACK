
#include <controls.h>

PSI_CONTROL t1, t2;
PSI_CONTROL t3, t4;

void CPROC DoScroll( PTRSZVAL psv )
{
	static int n;
	static int m;
	static int o;
	static int p;
	static int nmin;
	static int mmin;
	static int omin;
	static int pmin;
	static int nmax;
	static int mmax;
	static int omax;
	static int pmax;
	n++;
	m--;
	o++;
	p--;
	GetControlTextOffsetMinMax( t1, &nmin, &nmax );
	GetControlTextOffsetMinMax( t2, &mmin, &mmax );
	GetControlTextOffsetMinMax( t3, &omin, &omax );
	GetControlTextOffsetMinMax( t4, &pmin, &pmax );
   //lprintf( WIDE("setting offsets %d %d %d %d"), n, m, o, p );
	if( !SetControlTextOffset( t1, n ) )
		n = nmin;
	if( !SetControlTextOffset( t2, m ) )
      m = mmax;
	if( !SetControlTextOffset( t3, o ) )
		o = omin;
	if( !SetControlTextOffset( t4, p ) )
	{
		p = pmax;
	}
}

int main( void )
{
	PSI_CONTROL frame = CreateFrame( WIDE("test scrolling texts"), 0, 0, 1024, 768, 0, NULL );
	if( frame )
	{
		t1 = MakeNamedCaptionedControl( frame, STATIC_TEXT_NAME, 5, 5, 300, 15, -1, WIDE("Scroll This Text...") );
		t2 = MakeNamedCaptionedControl( frame, STATIC_TEXT_NAME, 5, 25, 300, 15, -1, WIDE("Scroll This Text...") );
		t3 = MakeNamedCaptionedControl( frame, STATIC_TEXT_NAME, 5, 45, 300, 15, -1, WIDE("Scroll This Text...") );
		SetControlAlignment( t3, TEXT_CENTER );
		t4 = MakeNamedCaptionedControl( frame, STATIC_TEXT_NAME, 5, 65, 300, 15, -1, WIDE("Scroll This Text...") );
		SetControlAlignment( t4, TEXT_CENTER );
		DisplayFrame( frame );
		AddTimer( 50, DoScroll, 0 );
		CommonWait( frame );
	}
}
