#if __WATCOMC__ == 1280
// define so we can use EnumDisplayDevices
#define WINVER 0x0500
#endif
#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <timers.h>

PTHREAD mainthread;


void CPROC TaskEnded( PTRSZVAL psv, PTASK_INFO task_ended )
{
   WakeThread( mainthread );
}


#ifdef WIN32
/* Utility routine for SetResolution */
static void SetWithFindMode( LPDEVMODE mode, int bRestoreOnCrash, char const * const name )
{
	DEVMODE current;
	DEVMODE check;
	DEVMODE best;
	INDEX idx;
	// going to code something that compilers hate
	// will have usage of an undefined thing.
	// it is defined by the time it is read. Assured.
	int best_set = 0;
	for( idx = 0;
		 EnumDisplaySettings( NULL /*EnumDisplaySettings */
								  , idx
									//ENUM_REGISTRY_SETTINGS
								  , &check );
		  idx++
		)
	{

		lprintf( "Found mode: %d %dx%d %d"
				 , idx
				 , check.dmPelsWidth
				 , check.dmPelsHeight
				 , check.dmBitsPerPel
				 );

		if( !idx )
			current = check;
		if( idx )
		{
			// current and check should both be valid
			if( ( check.dmPelsWidth == mode->dmPelsWidth )
				&& (check.dmPelsHeight == mode->dmPelsHeight ) )
			{
				if( best_set )
				{
					if( best.dmBitsPerPel < check.dmBitsPerPel )
					{
                  //lprintf( " ---- Updating best to current ---- " );
						best = check;
					}
				}
				else
				{
					//lprintf( " ---- Updating best to ccheck ---- " );
					best = check;
					best_set = 1;
				}
			}
		}
	}
	{
		int n;
		for( n = 2; n < 3; n++ )
		{
			_32 flags;
         _32 result;
			switch( n )
			{
			case 0:
				flags = bRestoreOnCrash?CDS_FULLSCREEN:0;
				break;
			case 1:
				flags = 0;
				break;
			case 2:
				flags = CDS_UPDATEREGISTRY | CDS_GLOBAL;
				break;


			}
			if ( !best_set || (result=ChangeDisplaySettingsEx(name, &best, NULL
																			 , flags // on program exit/original mode is restored.
                                                           , NULL
																		  )) != DISP_CHANGE_SUCCESSFUL ) {
				if( best_set && ( result == DISP_CHANGE_RESTART ) )
				{
#ifdef __cplusplus
					::
#endif
               system( "rebootnow.exe" );
					//SimpleMessageBox( NULL, "Restart?", "Must RESTART for Resolution change to apply :(" );
					lprintf( "Result indicates Forced Restart to change modes." );
					break;
				}
				mode->dmBitsPerPel = 32;
				lprintf( "Last failure %d %d", result, GetLastError() );
				if ( (result=ChangeDisplaySettingsEx(name, &best, NULL
																			 , flags // on program exit/original mode is restored.
                                                           , NULL
																		  )) != DISP_CHANGE_SUCCESSFUL ) {
					if( result == DISP_CHANGE_RESTART )
					{
#ifdef __cplusplus
					::
#endif
						system( "rebootnow.exe" );
						//SimpleMessageBox( NULL, "Restart?", "Must RESTART for Resolution change to apply :(" );
						lprintf( "Result indicates Forced Restart to change modes." );
                  break;
					}
					mode->dmBitsPerPel = 24;
					lprintf( "Last failure %d %d", result, GetLastError() );
					if ( (result=ChangeDisplaySettingsEx(name, &best, NULL
																			 , flags // on program exit/original mode is restored.
                                                           , NULL
																		  )) != DISP_CHANGE_SUCCESSFUL ) {
						if( result == DISP_CHANGE_RESTART )
						{
#ifdef __cplusplus
					::
#endif
							system( "rebootnow.exe" );
							//SimpleMessageBox( NULL, "Restart?", "Must RESTART for Resolution change to apply :(" );
							lprintf( "Result indicates Forced Restart to change modes." );
							break;
						}
						mode->dmBitsPerPel = 16;
						lprintf( "Last failure %d %d", result, GetLastError() );
						if ( (result=ChangeDisplaySettingsEx(name, &best, NULL
																			 , flags // on program exit/original mode is restored.
                                                           , NULL
																		  )) != DISP_CHANGE_SUCCESSFUL ) {

							if( result == DISP_CHANGE_RESTART )
							{
#ifdef __cplusplus
					::
#endif
								system( "rebootnow.exe" );
								//SimpleMessageBox( NULL, "Restart?", "Must RESTART for Resolution change to apply :(" );
								lprintf( "Result indicates Forced Restart to change modes." );
								break;
							}
							//char msg[256];
							lprintf( WIDE("Failed to change resolution to %d by %d (16,24 or 32 bit) %d %d")
									 , mode->dmPelsWidth
									 , mode->dmPelsHeight
									 , result
									 , GetLastError() );
							//MessageBox( NULL, msg
							//			 , WIDE("Resolution Failed"), MB_OK );
						}
						else
						{
							lprintf( "Success setting 16 bit %d %d"
									 , mode->dmPelsWidth
									 , mode->dmPelsHeight );
							break;
						}
					}
					else
					{
						lprintf( "Success setting 24 bit %d %d"
								 , mode->dmPelsWidth
								 , mode->dmPelsHeight );
						break;
					}
				}
				else
				{
					lprintf( "Success setting 32 bit %d %d"
							 , mode->dmPelsWidth
							 , mode->dmPelsHeight );
					break;
				}
			}
			else
			{
				lprintf( "Success setting enumerated bestfit %d %d"
						 , mode->dmPelsWidth
						 , mode->dmPelsHeight );
				break;
			}
		}
	}
}
#endif

void SetResolution(  _32 w, _32 h, char const * const name )
{
#ifdef WIN32
	DEVMODE settings;
	memset(&settings, 0, sizeof(DEVMODE));
	settings.dmSize = sizeof(DEVMODE);
	settings.dmBitsPerPel = 32; //video->format->BitsPerPixel;
	settings.dmPelsWidth = w;
	settings.dmPelsHeight = h;
	settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
   SetWithFindMode( &settings, FALSE/* we do want to restore on rpogram crahs */, name );
#endif
}


void FindDevice( void )
{
	DISPLAY_DEVICE dev;
	int i;
	BOOL success;
   dev.cb = sizeof( dev );
	for( i = 0; success =
		 EnumDisplayDevices( NULL // all devices
								 , i
								 , &dev
								 , 0 // dwFlags
								 ); i++ )
	{
		lprintf( "Got a device...[%s][%s]", dev.DeviceName, dev.DeviceString );
		LogBinary( &dev, sizeof( dev ) );
	}
}

void DumpDevices( void )
{
	{
		//int display = SACK_GetProfileInt( GetProgramName(), "Use Screen Number", 0 );
		//if( display > 0 )
		{
			TEXTSTR teststring = NewArray( TEXTCHAR, 20 );
         int xpos = 0;
			int idx;
			int i;
			DISPLAY_DEVICE dev;
			DEVMODE dm;
			dev.cb = sizeof( DISPLAY_DEVICE );
         dm.dmSize = sizeof( DEVMODE );
			//snprintf( teststring, 20, "//./DISPLAY%d", display );
			for( i = 0;
				 EnumDisplayDevices( NULL // all devices
										 , i
										 , &dev
										 , 0 // dwFlags
										 ); i++ )
			{
				//if( StrCaseCmp( teststring, dev.DeviceName ) )
            //   break;
				for( idx = 0;
					 EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm );
					  idx++ )
				{
               xpos += dm.dmPelsWidth;
				}
			}
		}
	}
}

int main( int argc, char const *const *argv )
{
   DumpDevices();
   FindDevice();
	if( argc < 3 )
	{
		printf( WIDE("usage: %0 <width> <height> [<bits>] program [<arguments...>]\n"), argv[0] );
		lprintf( WIDE("usage: %0 <width> <height> [<bits>] program [<arguments...>]"), argv[0] );
      return 1;
	}
	{
      int arg;
		_32 width, height, bits = 32;
		const char *program;
      arg = 1;
		width = atol( argv[arg++] );
		height = atol( argv[arg++] );
		if( argv[arg] && ( argv[arg][0] >= '0' && argv[arg][0] <= '9' ) )
			bits = atol( argv[arg++] );
		//program = argv[arg];
      printf( WIDE("Setting %d,%d (%d)\n"), program, width, height, bits );
#ifdef WIN32
      SetResolution( width, height, argv[arg++] );
#endif
	}
   return 0;
}


