#include <stdhdrs.h>
#include <timers.h>

PTHREAD mainthread;
int done;

void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task_ended )
{
   done = 1;
   WakeThread( mainthread );
}


#ifdef WIN32
/* Utility routine for SetResolution */
static void SetWithFindMode( LPDEVMODE mode, int bRestoreOnCrash )
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
		for( n = 0; n < 3; n++ )
		{
			uint32_t flags;
         uint32_t result;
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
			if ( !best_set || (result=ChangeDisplaySettings(&best
																		  , flags // on program exit/original mode is restored.
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
				if ( (result=ChangeDisplaySettings(mode
															 , flags // on program exit/original mode is restored.
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
					if ( (result=ChangeDisplaySettings(mode
																 , flags // on program exit/original mode is restored.
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
						if ( (result=ChangeDisplaySettings(mode
														, flags // on program exit/original mode is restored.
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
		uint32_t prior_w, prior_h;

void GetDisplaySize(uint32_t * width, uint32_t * height)
{
   RECT r;
   GetWindowRect (GetDesktopWindow (), &r);
   //Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
   if (width)
      *width = r.right - r.left;
   if (height)
      *height = r.bottom - r.top;
}


void SetResolution( uint32_t w, uint32_t h )
{
#ifdef WIN32
	DEVMODE settings;

	memset(&settings, 0, sizeof(DEVMODE));
	settings.dmSize = sizeof(DEVMODE);
	settings.dmBitsPerPel = 32; //video->format->BitsPerPixel;
	settings.dmPelsWidth = w;
	settings.dmPelsHeight = h;
	settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
   SetWithFindMode( &settings, FALSE /* we do want to restore on rpogram crahs */);

#endif
}

int main( int argc, char const *const *argv )
{
	if( argc < 3 )
	{
		printf( WIDE("usage: %0 <width> <height> [<bits>] program [<arguments...>]\n"), argv[0] );
		lprintf( WIDE("usage: %0 <width> <height> [<bits>] program [<arguments...>]"), argv[0] );
      return 1;
	}
	{
      int arg;
		uint32_t width, height, bits = 32;
		const char *program;
		GetDisplaySize( &prior_w, &prior_h );
      arg = 1;
		width = atol( argv[arg++] );
		height = atol( argv[arg++] );
		if( argv[arg][0] >= '0' && argv[arg][0] <= '9' )
			bits = atol( argv[arg++] );
		program = argv[arg];
      printf( WIDE("Running %s at %d,%d (%d)\n"), program, width, height, bits );
		{
#ifdef WIN32

			SetResolution( width, height );
#endif
		}
		if( LaunchProgramEx( program, NULL, argv + (arg), TaskEnded, 0 ) )
		{
			mainthread = MakeThread();
         while( !done )
				WakeableSleep( SLEEP_FOREVER );
		}
		SetResolution( prior_w, prior_h );
	}
   return 0;
}


