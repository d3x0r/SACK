#define DEFINES_DEKWARE_INTERFACE
#include <stdhdrs.h>
#ifdef _WIN32
#include <plugin.h>
HINSTANCE hInstMe;
#ifdef __TURBOC__
int APIENTRY DllEntryPoint( HINSTANCE hDLL, DWORD dwReason, void *pReserved )
#else
int APIENTRY DllMain( HINSTANCE hDLL, DWORD dwReason, void *pReserved )
#endif
{
   hInstMe = hDLL;
   return TRUE; // success whatever the reason...
}
#endif

int myTypeID;
PDATAPATH CPROC CreateVideoCapture( PDATAPATH *pChannel, PSENTIENT ps, PTEXT params );

PUBLIC( char *, RegisterRoutines )( void )
{
   myTypeID = RegisterDevice( "capture", "Video capture device.", CreateVideoCapture );
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   UnregisterDevice( "capture" );
}

//--------------------------------------------------------------------
//
// $Log: ntlink.c,v $
// Revision 1.1  2004/04/05 15:41:51  d3x0r
// *** empty log message ***
//
//

