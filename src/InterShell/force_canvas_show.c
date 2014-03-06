
#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>

#include <sharemem.h>
#include <filesys.h>
#include <vidlib/vidstruc.h>

int main( int argc, char **argv )
{
	POINTER mem_lock;
	PTRSZVAL size = 0;
	TEXTCHAR *myname = StrDup( pathrchr( DupCharToText( argv[0] ) ) );
	TEXTCHAR *endname;
	TEXTCHAR lockname[256];
	if( myname )
		myname++;
	else
		myname = DupCharToText( argv[0] );
   // go to the .exe extension
	endname = (TEXTCHAR*)StrRChr( myname, '.' );
	if( endname )
	{
      // remove .exe extension
		endname[0] = 0;
      // go to the .stop extension
		endname = (TEXTCHAR*)StrRChr( myname, '.' );
	}
	if( endname )
	{
      // remove .stop extension
		endname[0] = 0;
	}
	else
	{
      // this would be an invalid name.
		return 0;
	}
	snprintf( lockname, sizeof( lockname ), WIDE( "%s.instance.lock" ), myname );
	lprintf( WIDE( "Checking lock %s" ), lockname );
	mem_lock = OpenSpace( lockname
		, NULL
		//, WIDE("memory.delete")
		, &size );
	if( mem_lock )
	{
#ifdef WIN32
		PRENDER_INTERFACE pri = GetDisplayInterface();
		PVIDEO video = (PVIDEO)mem_lock;
		if( video->hWndOutput )
		{
			ForceDisplayFocus( video );
			RestoreDisplay( video );

		}
		else
		{
			// region found, but no content...
		}
#endif
	}
	else
		lprintf( WIDE("lock region not found.") );
	return 0;
}
