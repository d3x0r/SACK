#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <idle.h>
#include <controls.h>
#include <sqlgetoption.h>
#include "vlcint.h"

static struct vlc_test_local {

	_8  display; // What display do you want to use Ex. 1, 2
	_32 time;    // Time for vlc to live

	TEXTCHAR **wargv;          // Stores all the arguments
	TEXTCHAR input[4096];      // Stores extra vlc input and options
	TEXTCHAR extra_opts[4096]; // Stores extra vlc options

	struct {
			
		BIT_FIELD in_control : 1;
		BIT_FIELD make_top : 1;
		BIT_FIELD is_transparent : 1;
		BIT_FIELD is_stream : 1;
		BIT_FIELD rec_input_type : 1;		

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
			if( StrCaseCmp( l.wargv[n]+1, WIDE( "display" ) ) == 0 )
			{
				l.display = atoi( l.wargv[n+1] );
				n++;
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "transparent" ) ) == 0 )
			{
				l.flags.is_transparent = 1;                                          
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "stream" ) ) == 0 )
			{
				l.flags.is_stream = 1;
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "control" ) ) == 0 )
			{
				l.flags.in_control = 1;
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "log_stdout" ) ) == 0 )
			{
				FLAGSETTYPE flags = 0;
				SetSyslogOptions( &flags );
				SystemLogTime( 0 );
				SetSystemLog( SYSLOG_FILE, stdout );
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "top" ) ) == 0 )
			{
				l.flags.make_top = 1;
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "time" ) ) == 0 )
			{
				l.time = atoi( l.wargv[n+1] );
				n++;;
			}

			else if( ( StrCaseCmp( l.wargv[n]+1, WIDE( "input" ) ) == 0 ) && ( !l.flags.rec_input_type )  )
			{
				l.flags.rec_input_type = 1;
				SACK_GetPrivateProfileString( WIDE( "vlc/config" ), WIDE( "vlc input" ), WIDE("dshow://"), tmp1, sizeof( tmp1 ), WIDE( "video.ini" ) );
				ofs2 += snprintf( l.input + ofs2, sizeof( l.input ) - ofs2, WIDE( "%s" ), tmp1 );				
			}
			else if( ( StrCaseCmp( l.wargv[n]+1, WIDE( "input" ) ) == 0 ) && ( l.flags.rec_input_type )  )
			{
				lprintf(WIDE(" An input has already been specified!!!") );
				lprintf(WIDE(" Ignoring -input") );
			}

			else if( StrCaseCmp( l.wargv[n]+1, WIDE( "options" ) ) == 0 )
			{
				SACK_GetPrivateProfileString( WIDE( "vlc/config" ), WIDE( "vlc options" ), WIDE("-vvv"), tmp2, sizeof( tmp2 ), WIDE( "video.ini" ) );
				ofs += snprintf( l.extra_opts + ofs, sizeof( l.extra_opts ) - ofs, WIDE( "%s%s" ), ofs?" ":"", tmp2 );
			}			

			else 
			{
				ofs += snprintf( l.extra_opts + ofs, sizeof( l.extra_opts ) - ofs, WIDE( "%s%s" ), ofs?" ":"", l.wargv[n] );
			}
		}

		else
		{
         /*
			if( ( !l.flags.rec_input_type ) && ( l.wargv[n][0] != ':' ) )
			{
				l.flags.rec_input_type = 1;

				if( ofs2 == 0 )
					ofs2 += snprintf( l.input + ofs2, sizeof( l.input ) - ofs2, WIDE( "%s" ), l.wargv[n] );								
			}

			else if ( ( l.flags.rec_input_type ) && ( l.wargv[n][0] == ':' ) )
			{
				if( ofs2 != 0 )
					ofs2 += snprintf( l.input + ofs2, sizeof( l.input ) - ofs2, WIDE( " %s" ), l.wargv[n] );
			}

			else if( ( !l.flags.rec_input_type ) && ( l.wargv[n][0] == ':' ) )
			{
				#ifdef _WIN32
					MessageBox( NULL, WIDE( " Must specify input type before input options!!!\n" ), WIDE("Error"), MB_OK );
				#endif

				lprintf( WIDE(" Must specify input type before input options\n") );
				lprintf( WIDE( " Terminating Application..." ) );
				return 0;
			}
			else
            */
			{
				ofs += snprintf( l.extra_opts + ofs, sizeof( l.extra_opts ) - ofs, WIDE( "%s%s" ), ofs?" ":"", l.wargv[n] );
			}			
		}
	}
	
    return 1;
}



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// vlc_test main 

int main( int argc, char ** argv )
{	
	_32 w, h;                 // w: how wide this display is, h: how tall this display is
	S_32 x, y;                // x: left screen coordinate of this display, y: top screen coordinate of this display
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
		MessageBox( NULL, WIDE( "Command Line Arguments for VLC Test:\n" )
					  WIDE( " -display # : show on display number\n" )
					  WIDE( " -time # : time for vlc to live\n" )
					  WIDE( " -top : make topmost\n" )
					  WIDE( " -control : use a control to host video\n" )
					  WIDE( " -log_stdout : send logging to standard out instead of log file\n" )
					  WIDE( " -transparent : use transparent display\n" )
					  WIDE( " -stream : output is stream\n" )
					  WIDE( " -input : Read input info from editoptions\n" )
					  WIDE( " -options : Read extra vlc options from editoptions\n" )
					  WIDE( "\n All VLC Options Should Be Valid\n" )
					 , WIDE( "Must Enter a Command Line Argument" ), MB_OK );
#endif
		lprintf( WIDE( " Terminating Application..." ) );
		return 0;
	}

	// Set Options from the command line ( returns 1 if successful ) 
	if ( !( setOptions( argc ) ) )
		return 0;			
	
	lprintf( WIDE(" Input = %s"), l.input );
	lprintf( WIDE(" VLC Options = %s"), l.extra_opts );

	//if( l.input[0] )
	{
		// Get Display size
		GetDisplaySizeEx( l.display, &x, &y, &w, &h );


		// If streaming do not open window
		if( !l.flags.is_stream )
		{
			transparent = OpenDisplaySizedAt( l.flags.is_transparent?DISPLAY_ATTRIBUTE_LAYERED:0, w, h, x, y );
			surface = l.flags.in_control?CreateFrameFromRenderer( WIDE( "Video Display" )
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
		
		if( l.flags.is_stream )
			pmyi = PlayItemAtEx( l.input, l.extra_opts );

		else if( l.flags.in_control )
			pmyi = PlayItemInEx( surface, l.input, l.extra_opts );

		else
			pmyi = PlayItemOnExx( transparent, l.input, l.extra_opts, l.flags.is_transparent );

		lprintf ( WIDE( " Playing..." ) );
		PlayItem( pmyi );
	}	
   /*
	else
	{
		#ifdef _WIN32
			MessageBox( NULL, WIDE( " No input to open was specified...\n" ), WIDE( "Error" ), MB_OK );
		#endif
		
		lprintf( WIDE( " No input to open was specified...\n" ) );
		lprintf( WIDE( " Terminating Application..." ) );
		return 0;
	}
   */
	lprintf( WIDE( " Going to sleep..." ) );
	

	while( 1 )
	{		

		if( l.time )
		{
			IdleFor( l.time );
			break;
		}

		else
			IdleFor( 250000 );

		lprintf( WIDE( " Woke up..." ) );


		if( !l.flags.is_stream )
			if( !DisplayIsValid( transparent ) )
				break;

		lprintf( WIDE( " Going back to sleep..." ) );
	}

	lprintf( WIDE( " Terminating Application..." ) );
  	StopItem( pmyi );		

	return 0;
}


