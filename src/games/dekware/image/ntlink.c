// well do stuff here....
//#include <windows.h>
#include <stdio.h>
#define DO_LOGGING
#include <logging.h>
#include "plugin.h"

#include <render.h>
extern PRENDER_INTERFACE pActImage;
extern PIMAGE_INTERFACE pImageInterface;

// common DLL plugin interface.....

INDEX iImage, iRender;

int InitImage( PSENTIENT ps, PENTITY pe, PTEXT parameters );
int InitDisplayObject( PSENTIENT ps, PENTITY pe, PTEXT parameters );
int InitRegion( PSENTIENT ps, PENTITY pe, PTEXT parameters );


PUBLIC( char *, RegisterRoutines )( void )
{
	pActImage = GetDisplayInterface();
   pImageInterface = GetImageInterface();
	RegisterObject( "Image", "Image object, position determines role", InitImage );
	RegisterObject( "Render", "Image data Display manager", InitDisplayObject );
	iImage = RegisterExtension( "image" );
	iRender = RegisterExtension( "render" );
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterObject( "Image" );
	UnregisterObject( "Render" );

}

// $Log: ntlink.c,v $
// Revision 1.11  2004/06/12 09:46:09  d3x0r
// Linux first pass fixes to compile
//
// Revision 1.10  2004/04/01 08:59:32  d3x0r
// Updates to implement interface to image and render libraries
//
// Revision 1.9  2003/07/28 09:07:39  panther
// Fix makefiles, fix cprocs on netlib interfaces... fix a couple badly formed functions
//
// Revision 1.8  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.7  2003/03/25 08:59:02  panther
// Added CVS logging
//
