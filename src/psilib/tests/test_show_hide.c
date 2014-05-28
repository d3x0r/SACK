#include <psi.h>


#ifdef _MSC_VER
int APIENTRY WinMain( HINSTANCE x, HINSTANCE y, LPSTR a, int n )
#else
int main( void )
#endif
{
	PSI_CONTROL frame = CreateFrame( "Test Some Controls", 0, 0, 0, 0, BORDER_RESIZABLE, NULL );
	DisplayFrame( frame );
	{
		int n = 0;
		PSI_CONTROL controls[10][10];
		int x, y;
		for( x = 0; x < 10; x++ )
			for( y = 0; y < 10; y++ )
			{
				controls[x][y] = MakeNamedCaptionedControl( frame, "Button", x*15 + 5, y*15+5, 15, 15, -1, "B" );
				//SmudgeCommon( controls[x][y] );
			}
		Sleep( 5000 );
		//DisplayFrame( frame );
		//Relinquish();
		//WakeableSleep( 10000 );
		do
		{
			n++;
			for( x = 0; x < 10; x++ )
				for( y = 0; y < 10; y++ )
					HideCommon( controls[x][y] );

			Sleep( 200 );

			for( x = 0; x < 10; x++ )
				for( y = 0; y < 10; y++ )
					RevealCommon( controls[x][y] );
			Sleep( 200 );
		} while( 100 > n );
	}
	DestroyFrame( &frame );
	return 0;
}


