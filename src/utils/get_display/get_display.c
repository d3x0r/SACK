#include <windows.h>
#include <malloc.h>
#include <stdio.h>

void GetDisplaySizeEx( int nDisplay
												 , int *x, int *y
												 , int *width, int *height)
{
		if( nDisplay > 0 )
		{
			char* teststring = malloc( 20 );
			//int idx;
			int v_test = 0;
			int i;
			int found = 0;
			DISPLAY_DEVICE dev;
			DEVMODE dm;
			if( x )
				(*x) = 256;
			if( y )
				(*y) = 256;
			if( width )
				(*width) = 720;
			if( height )
				(*height) = 540;
			dm.dmSize = sizeof( DEVMODE );
			dev.cb = sizeof( DISPLAY_DEVICE );
			for( v_test = 0; !found && ( v_test < 2 ); v_test++ )
			{
				// go ahead and try to find V devices too... not sure what they are, but probably won't get to use them.
				_snprintf( teststring, 20, ( "\\\\.\\DISPLAY%s%d" ), (v_test==1)?("V"):(""), nDisplay );
				for( i = 0;
					 !found && EnumDisplayDevices( NULL // all devices
														  , i
														  , &dev
														  , 0 // dwFlags
														  ); i++ )
				{
					if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
					{
						//printf( ( "display %s is at %d,%d %dx%d" ), dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
					}
					//else
					//	lprintf( ( "Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					if( stricmp( teststring, dev.DeviceName ) == 0 )
					{
						//printf( ( "[%s] might be [%s]" ), teststring, dev.DeviceName );
						if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
						{
							if( x )
								(*x) = dm.dmPosition.x;
							if( y )
								(*y) = dm.dmPosition.y;
							if( width )
								(*width) = dm.dmPelsWidth;
							if( height )
								(*height) = dm.dmPelsHeight;
							found = 1;
							break;
						}
						//else
							//printf( ( "Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					}
					else
					{
						//lprintf( ( "[%s] is not [%s]" ), teststring, dev.DeviceName );
					}
				}
			}
		}
		else
		{
			if( x )
				(*x)= 0;
			if( y )
				(*y)= 0;
			{
				RECT r;
				GetWindowRect (GetDesktopWindow (), &r);
				//Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
				if (width)
					*width = r.right - r.left;
				if (height)
					*height = r.bottom - r.top;
			}
		}
}


#ifdef CONSOLE
int main( int argc, char **argv )
{
	if( argc > 1 )
	{
		int nDisplay = atoi( argv[1] );
		int x, y, w, h;
		GetDisplaySizeEx( nDisplay, &x, &y, &w, &h );
		printf( "%d,%d,%d,%d", x, y, w, h );
	}
	else
		printf( "Need an argument <number> which is display to get" );
	return 0;
}

#endif

