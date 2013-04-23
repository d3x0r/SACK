// totally useless filter 
// other than this filter provides a shell for all other filters.

#include <stdhdrs.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;

typedef struct mydatapath_tag {
   DATAPATH common;
} MYDATAPATH, *PMYDATAPATH;


//---------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, NULL );
}

//---------------------------------------------------------------------------

static int CPROC Write( PMYDATAPATH pdp )
{
	PTEXT line;
	while( line = (PTEXT)DequeLink( &pdp->common.Output ) )
		LineRelease( line );
   return 0;
}

//---------------------------------------------------------------------------

static int CPROC Close( PMYDATAPATH pdp )
{
   pdp->common.Type = 0;
   return 0;
}

//---------------------------------------------------------------------------

#define OptionLike(text,string) ( strnicmp( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("sink"), WIDE("Sink eats all output, and passes any input"), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("sink") );
}
// $Log: sink.c,v $
// Revision 1.8  2005/02/21 12:08:35  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.7  2005/01/18 02:47:01  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.6  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.5  2003/03/25 08:59:02  panther
// Added CVS logging
//
