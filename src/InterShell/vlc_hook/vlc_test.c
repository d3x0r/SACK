#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <idle.h>
#include <controls.h>
#include <sqlgetoption.h>
#include "vlcint.h"

static struct vlc_test_local {

	uint8_t  display; // What display do you want to use Ex. 1, 2
	uint32_t time;    // Time for vlc to live

	TEXTCHAR **wargv;          // Stores all the arguments
	TEXTCHAR input[4096];      // Stores extra vlc input and options
	TEXTCHAR extra_opts[4096]; // Stores extra vlc options

	struct {
			
		BIT_FIELD in_control : 1;
		BIT_FIELD make_top : 1;
		BIT_FIELD is_transparent : 1;
		BIT_FIELD is_stream : 1;
		BIT_FIELD rec_input_type : 1;		
		BIT_FIELD nodisplay : 1;
	} flags;

} l;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Set Options 

int setOptions( int argc )
{
	int n, ofs, ofs2;	
	TEXTCHAR tmp1[400], tmp2[400];

	for( n = 1, ofs = 0, ofs2 = 0; n < argc; n++ )
	{		
		if( l.wargv[n][0] == '-' )
		{
			// with subcanvas support, this cannot function, sorry
			// we get confused about which menu belongs to which frame
			// some thought will have to be done to figure this one out.
			if( StrCaseCmp( l.wargv[n]+1, "display" ) == 0 )
			{
				l.display = atoi( l.wargv[n+1] );
				n++;
			}
			else if( StrCaseCmp( l.wargv[n]+1, "nodisplay" ) == 0 )
			{
				l.flags.nodisplay = 1;
				n++;
			}
			else if( StrCaseCmp( l.wargv[n]+1, "transparent" ) == 0 )
			{
				l.flags.is_transparent = 1;                                          
			}

			else if( StrCaseCmp( l.wargv[n]+1, "stream" ) == 0 )
			{
				l.flags.is_stream = 1;
			}

			else if( StrCaseCmp( l.wargv[n]+1, "control" ) == 0 )
			{
				l.flags.in_control = 1;
			}

			else if( StrCaseCmp( l.wargv[n]+1, "log_stdout" ) == 0 )
			{
				SetSystemLog( SYSLOG_FILE, stdout );
			}

			else if( StrCaseCmp( l.wargv[n]+1, "top" ) == 0 )
			{
				l.flags.make_top = 1;
			}

			else if( StrCaseCmp( l.wargv[n]+1, "time" ) == 0 )
			{
				l.time = atoi( l.wargv[n+1] );
				n++;;
			}

			else if( ( StrCaseCmp( l.wargv[n]+1, "input" ) == 0 ) && ( !l.flags.rec_input_type )  )
			{
				l.flags.rec_input_type = 1;
				SACK_GetPrivateProfileString( "vlc/config", "vlc input", "dshow://", tmp1, sizeof( tmp1 ), "video.ini" );
				ofs2 += snprintf( l.input + ofs2, sizeof( l.input ) - ofs2, "%s", tmp1 );
			}
			else if( ( StrCaseCmp( l.wargv[n]+1, "input" ) == 0 ) && ( l.flags.rec_input_type )  )
			{
				lprintf(" An input has already been specified!!!" );
				lprintf(" Ignoring -input" );
			}

			else if( StrCaseCmp( l.wargv[n]+1, "options" ) == 0 )
			{
				SACK_GetPrivateProfileString( "vlc/config", "vlc options", "-vvv", tmp2, sizeof( tmp2 ), "video.ini" );
				ofs += snprintf( l.extra_opts + ofs, sizeof( l.extra_opts ) - ofs, "%s%s", ofs?" ":"", tmp2 );
			}			

			else 
			{
				ofs += snprintf( l.extra_opts + ofs, sizeof( l.extra_opts ) - ofs, "%s%s", ofs?" ":"", l.wargv[n] );
			}
		}

		else
		{
         /*
			if( ( !l.flags.rec_input_type ) && ( l.wargv[n][0] != ':' ) )
			{
				l.flags.rec_input_type = 1;

				if( ofs2 == 0 )
					ofs2 += snprintf( l.input + ofs2, sizeof( l.input ) - ofs2, "%s", l.wargv[n] );								
			}

			else if ( ( l.flags.rec_input_type ) && ( l.wargv[n][0] == ':' ) )
			{
				if( ofs2 != 0 )
					ofs2 += snprintf( l.input + ofs2, sizeof( l.input ) - ofs2, " %s", l.wargv[n] );
			}

			else if( ( !l.flags.rec_input_type ) && ( l.wargv[n][0] == ':' ) )
			{
				#ifdef _WIN32
					MessageBox( NULL, " Must specify input type before input options!!!\n", "Error", MB_OK );
				#endif

				lprintf( " Must specify input type before input options\n" );
				lprintf( " Terminating Application..." );
				return 0;
			}
			else
            */
			{
				ofs += snprintf( l.extra_opts + ofs, sizeof( l.extra_opts ) - ofs, "%s%s", ofs?" ":"", l.wargv[n] );
			}			
		}
	}
	
    return 1;
}



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// vlc_test main 

int main( int argc, char ** argv )
{	
	uint32_t w, h;                 // w: how wide this display is, h: how tall this display is
	int32_t x, y;                // x: left screen coordinate of this display, y: top screen coordinate of this display
	PRENDERER transparent;
	PSI_CONTROL surface;
	struct my_vlc_interface *pmyi;
	
	// Store arguments
	l.wargv = NewArray( TEXTCHAR *, argc + 1 );
	for( x = 0; x < argc; x++ )
		l.wargv[x] = DupCharToText( argv[x] );

	// If no args, message box and return
	if( argc == 1 )
	{
#ifdef _WIN32
		MessageBox( NULL, "Command Line Arguments for VLC Test:\n"
					  " -display # : show on display number\n"
					  " -time # : time for vlc to live\n"
					  " -top : make topmost\n"
					  " -control : use a control to host video\n"
					  " -log_stdout : send logging to standard out instead of log file\n"
					  " -transparent : use transparent display\n"
					  " -stream : output is stream\n"
					  " -input : Read input info from editoptions\n"
					  " -nodisplay : probably VLC will create its own window; so don't create one\n"
					  " -options : Read extra vlc options from editoptions\n"
					  "\n All VLC Options Should Be Valid\n"
					 , "Must Enter a Command Line Argument", MB_OK );
#endif
		lprintf( " Terminating Application..." );
		return 0;
	}

	// Set Options from the command line ( returns 1 if successful ) 
	if ( !( setOptions( argc ) ) )
		return 0;			
	
	lprintf( " Input = %s", l.input );
	lprintf( " VLC Options = %s", l.extra_opts );

	//if( l.input[0] )
	{
		// Get Display size
		GetDisplaySizeEx( l.display, &x, &y, &w, &h );


		// If streaming do not open window
		if( !l.flags.is_stream && !l.flags.nodisplay)
		{
			transparent = OpenDisplaySizedAt( l.flags.is_transparent?DISPLAY_ATTRIBUTE_LAYERED:0, w, h, x, y );
			surface = l.flags.in_control?CreateFrameFromRenderer( "Video Display"
																		, BORDER_NONE|BORDER_NOCAPTION
																		, transparent ):0;
			DisableMouseOnIdle( transparent, TRUE );
			if( surface )
				DisplayFrame( surface );
			else
				UpdateDisplay( transparent );
			if( l.flags.make_top )
				MakeTopmost( transparent );
		}
		else
		{
			transparent = NULL;
			surface = NULL;
		}
		
		if( l.flags.is_stream )
			pmyi = PlayItemAtEx( l.input, l.extra_opts );

		else if( l.flags.in_control )
			pmyi = PlayItemInEx( surface, l.input, l.extra_opts );

		else
			pmyi = PlayItemOnExx( transparent, l.input, l.extra_opts, l.flags.is_transparent );

		lprintf ( " Playing..." );
		PlayItem( pmyi );
	}	
   /*
	else
	{
		#ifdef _WIN32
			MessageBox( NULL, " No input to open was specified...\n", "Error", MB_OK );
		#endif
		
		lprintf( " No input to open was specified...\n" );
		lprintf( " Terminating Application..." );
		return 0;
	}
   */
	lprintf( " Going to sleep..." );
	

	while( 1 )
	{		

		if( l.time )
		{
			IdleFor( l.time );
			break;
		}

		else
			IdleFor( 250000 );

		lprintf( " Woke up..." );


		if( !l.flags.is_stream && transparent )
			if( !DisplayIsValid( transparent ) )
				break;

		lprintf( " Going back to sleep..." );
	}

	lprintf( " Terminating Application..." );
  	StopItem( pmyi );		

	return 0;
}


