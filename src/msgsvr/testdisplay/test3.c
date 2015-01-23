#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <controls.h>
#include <psi/shadewell.h>
#include <logging.h>
	
#ifdef UNDER_CE
int APIENTRY WinMain( HINSTANCE h, HINSTANCE p, LPCSTR cmd, int nCmdShow )
{
#else
int main( void )
	{
#endif
	CDATA result;
	//SetSystemLog( SYSLOG_FILENAME, WIDE("Test3.Log") );
	//SetAllocateLogging( TRUE );

  //SetBlotMethod( BLOT_MMX );
   //ActImage_GetMousePosition( &x, &y );
   result = Color(127,127,127);
	while( PickColorEx( &result, result,(PSI_CONTROL) 0, 256, 150 ) )
		printf( WIDE("Guess what! We got a color: %08x\n"), result );
	printf( WIDE("Color Dialog was canceled.\n") );
	return 0;
}

// $Log: test3.c,v $
// Revision 1.4  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
