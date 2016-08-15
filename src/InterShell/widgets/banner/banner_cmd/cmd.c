
#include <configscript.h>
#include <psi/console.h>
#include "../../include/banner.h"

static struct {
	CDATA back_color;
	CDATA text_color;
	CTEXTSTR file;
	TEXTSTR text;
	uintptr_t text_size;
	int lines;
	int columns;
	int display;
   int timeout;
	struct flags_tag
	{
		BIT_FIELD bYesNo : 1;
		BIT_FIELD bOkayCancel : 1;
		BIT_FIELD bTop : 1;
		BIT_FIELD bDead : 1;
		BIT_FIELD bAlpha : 1;
	} flags;
} l;

int HandleArgs( int argc, TEXTCHAR **argv )
{

   PVARTEXT pvt = NULL;
	int arg = 1;
	if( argc == 1 )
	{
		MessageBox( NULL, WIDE( "Available Arguments\n" )
					  WIDE( " -back $aarrggbb  : set background color in hex, alpha, red, green, blue [default black]\n" )
					  WIDE( " -text $aarrggbb  : set text color in hex, alpha, red, green, blue [default white]\n" )
					  WIDE( " -lines N  : Set the target number of lines (font height, bigger number is smaller) [default 20]\n" )
					  WIDE( " -cols N  : Set the target number of columns (font width, bigger number is smaller) [default 30]\n" )
					  WIDE( " -file <filename>  : Use contents fo the file specified for text\n" )
					  WIDE( " -alpha : enable alpha transparency display\n" )
					  WIDE( " -dead : disable click and keyboard bypass\n" )
					  WIDE( " -display N : specify which display to show on\n" )
					  WIDE( " -timeout N : default time delay for no action (milliseconds)\n" )
					  WIDE( "\n" )
					  WIDE( " -notop : shows the banner as not topmost (default is top).\n" )
					  WIDE( " -yesno : shows the Yes/No buttons, and returns error level 0 for yes and 1 for no.\n" )
					  WIDE( " -okcancel : shows the Okay/Cancel buttons, and returns error level 0 for yes and 1 for no.\n" )
					  WIDE( " -yesno and -okaycancel : returns error level 0 for yes and 1 for no, 2 for cancel, 3 for ok.\n" )
					  WIDE( " \n" )
					  WIDE( " any other unknown argument or other word will show as text.\n" )
					  WIDE( " banner_command -back $20800010 -text $FF70A080 Show \"This Text On\" Banner\n" )
					  WIDE( "   - the prior command will show 3 lines 'show', 'this text on', 'banner'" )
					  WIDE( " banner_command Show \"\\\"This Text On\\\"\" Banner\n" )
					  WIDE( "   - the prior command shows how to include showing quotes" )
					 , WIDE( "Usage" )
					 , MB_OK );
      return 0;
	}
   // default to topmost.
	l.flags.bTop = 1;

	while( arg < argc )
	{
		if( argv[arg][0]=='-' )
		{
			if( StrCaseCmp( argv[arg]+1, WIDE( "back" ) ) == 0 )
			{
				// blah
				arg++;
				if( arg < argc )
				{
               PTEXT tmp2;
					PTEXT tmp = SegCreateFromText( argv[arg] );
               tmp2 = tmp;
					GetColorVar( &tmp, &l.back_color );
					LineRelease( tmp2 );
				}
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "text" ) ) == 0 )
			{
				arg++;
				if( arg < argc )
				{
               PTEXT tmp2;
					PTEXT tmp = SegCreateFromText( argv[arg] );
               tmp2 = tmp;
					GetColorVar( &tmp, &l.text_color );
					LineRelease( tmp2 );
				}

			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "lines" ) ) == 0 )
			{
				arg++;
				if( arg < argc )
				{
#ifdef UNICODE
					l.lines = _wtoi( argv[arg] );
#else
					l.lines = atoi( argv[arg] );
#endif
				}
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "cols" ) ) == 0 )
			{
				arg++;
				if( arg < argc )
				{
#ifdef UNICODE
               l.columns = _wtoi( argv[arg] );
#else
               l.columns = atoi( argv[arg] );
#endif
				}
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "display" ) ) == 0 )
			{
				arg++;
				if( arg < argc )
				{
#ifdef UNICODE
               l.display = _wtoi( argv[arg] );
#else
               l.display = atoi( argv[arg] );
#endif
				}
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "timeout" ) ) == 0 )
			{
				arg++;
				if( arg < argc )
				{
#ifdef UNICODE
               l.timeout = _wtoi( argv[arg] );
#else
               l.timeout = atoi( argv[arg] );
#endif
				}
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "dead" ) ) == 0 )
			{
            l.flags.bDead = 1;
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "alpha" ) ) == 0 )
			{
            l.flags.bAlpha = 1;
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "file" ) ) == 0 )
			{
				// blah
				arg++;
				if( arg < argc )
				{
               POINTER p;
					l.file = argv[arg];
					p = OpenSpace( NULL, l.file, &l.text_size );
					if( p )
					{
						l.text = NewArray( TEXTCHAR, l.text_size + 1 );
						MemCpy( l.text, p, l.text_size );
						l.text[l.text_size] = 0;
                  Release( p );
					}
				}
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "notop" ) ) == 0 )
			{
            l.flags.bTop = 0;
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "yesno" ) ) == 0 )
			{
            l.flags.bYesNo = 1;
			}
			else if( StrCaseCmp( argv[arg]+1, WIDE( "okcancel" ) ) == 0 )
			{
            l.flags.bOkayCancel = 1;
			}
			else
			{
				if( !pvt )
				{
					pvt = VarTextCreate();
					vtprintf( pvt, WIDE( "%s" ), argv[arg] );
				}
				else
					vtprintf( pvt, WIDE( "\n%s" ), argv[arg] );
			}
		}
		else
		{
			if( !pvt )
			{
				pvt = VarTextCreate();
            vtprintf( pvt, WIDE( "%s" ), argv[arg] );
			}
         else
            vtprintf( pvt, WIDE( "\n%s" ), argv[arg] );
		}
      arg++;
	}
	if( pvt )
	{
      l.text = StrDup( GetText( VarTextPeek( pvt ) ) );
	}

	if( !l.text )
	{
      l.text = WIDE( "INVALID COMMAND LINE\nARGUMENTS" ) ;
	}
	else
	{
		int n = 0;
      int o = 0;
		for( n = 0; l.text[n]; n++ )
		{
			if( l.text[n] < 32 )
			{
				if( l.text[n] == '\n' )
				{
					l.text[o++] = l.text[n];
				}
			}
			else
				l.text[o++] = l.text[n];
		}
		l.text[o++] = l.text[n];

	}
   return 1;
}

SaneWinMain( argc, argv )
{
   int result;
	PBANNER banner = NULL;
	if( !HandleArgs( argc, argv ) )
      return 0;

	if( !l.back_color )
      l.back_color = 0xFF000000;
	if( !l.text_color )
      l.text_color = 0xFFFFFFFF;

	result = CreateBanner2Extended( NULL, &banner, l.text
											, (l.flags.bYesNo?BANNER_OPTION_YESNO:0)
											 |( l.flags.bOkayCancel?BANNER_OPTION_OKAYCANCEL:0)
											 |( l.flags.bTop?BANNER_TOP:0)
											 |((l.flags.bYesNo||l.flags.bOkayCancel)?0:BANNER_CLICK)
											 |((l.timeout)?BANNER_TIMEOUT:0)
											 |((l.flags.bDead)?BANNER_DEAD:0)
											 |((l.flags.bAlpha)?BANNER_ALPHA:0)
											, l.timeout
											, l.text_color
											, l.back_color
											, l.lines
											, l.columns
											, l.display );

	{
		int result2 = WaitForBanner2( banner );
      lprintf( WIDE( "result is %d" ), result,result2 );
		return result;
	}

   return 0;
}
EndSaneWinMain()
