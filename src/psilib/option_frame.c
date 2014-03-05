#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <controls.h>
#include <idle.h>

#ifndef WIN32
#define BTN_OKAY   1
#endif

#define DIA_X(x) x * 2
#define DIA_Y(y) y * 2
#define DIA_W(w) w * 2
#define DIA_H(h) h * 2

#ifndef __NO_GUI__
//#ifdef GRAPHIC_PROMPT
static void CPROC SetIntTRUE( PTRSZVAL psvInt, PSI_CONTROL unused )
{
	struct done_tag{
      PTHREAD me;
		int done;
	} *done = (struct done_tag*)psvInt;
	if( done )
	{
		done->done = TRUE;
		WakeThread( done->me );
	}
}
#endif

size_t _SQLPromptINIValue(			 CTEXTSTR lpszSection,
					 CTEXTSTR lpszEntry,
					 CTEXTSTR lpszDefault,
					 TEXTSTR lpszReturnBuffer,
					 size_t cbReturnBuffer,
					 CTEXTSTR filename
		)
{
#ifndef __NO_GUI__
	PSI_CONTROL frame;
	struct {
		PTHREAD me;
		int done;
	} done;
	TEXTCHAR text[256];
	done.me = MakeThread();
	done.done = 0;
	//if( blog )
	//	lprintf( WIDE("Dialog prompt for [%s] %s=%s in %s"), lpszSection, lpszEntry, lpszDefault, filename );
	frame = CreateFrame( WIDE("INI Entry Error"), DIA_X(59), DIA_Y(34), DIA_W(256) + 36, DIA_H(78), BORDER_NORMAL, NULL );
	if( frame )
	{
	MakeTextControl( frame, DIA_X(1), DIA_Y(2), DIA_W(254), DIA_H(16), -1, WIDE("The value below has not been found.  Please enter the correct value."), 0 );
	snprintf( text, sizeof( text ), WIDE("%s\n   [%s]\n      %s ="), filename, lpszSection, lpszEntry );
	MakeTextControl( frame, DIA_X(4), DIA_Y(18), DIA_W(248), DIA_H(28), 123, text, EDIT_READONLY );
	MakeEditControl( frame, DIA_X(4), DIA_Y(46), DIA_W(248), DIA_H(12), 124, lpszDefault, 0 );
	MakeButton( frame, DIA_X(4), DIA_Y(63), DIA_W(248), DIA_H(14), IDOK, WIDE("Ok"), 0, SetIntTRUE, (PTRSZVAL)&done );
	DisplayFrame( frame );
	MakeTopmost( GetFrameRenderer( frame ) );
	while( !done.done )
	{
      if( !Idle() )  // otherwise dispatched an idle, and we're dependant on someone else...
			WakeableSleep( 5000 );
	}
	GetControlText( GetControl( frame, 124 ), lpszReturnBuffer, cbReturnBuffer );
	//CryptoWritePrivateProfileString( lpszSection, lpszEntry, lpszReturnBuffer, hg_file[file].file_name );
    DestroyFrame( &frame );
	return strlen( lpszReturnBuffer );
	}
	else
#endif
	{
		StrCpyEx( lpszReturnBuffer, lpszDefault, cbReturnBuffer );
	}
	return strlen( lpszReturnBuffer );
}

