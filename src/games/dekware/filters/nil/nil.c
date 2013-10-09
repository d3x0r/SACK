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

static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, NULL );
}

//---------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( (PDATAPATH)pdp, NULL );
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdp )
{
   PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
   pmdp->common.Type = 0; // allow delete
   return 0;
}

//---------------------------------------------------------------------------

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   //PTEXT option;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->common.Type = myTypeID;
   pdp->common.Read = Read;
   pdp->common.Write = Write;
   pdp->common.Close = Close;
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("nil"), WIDE("Performs no translation..."), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("nil") );
}
// $Log: nil.c,v $
// Revision 1.9  2005/02/21 12:08:34  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.8  2005/01/17 08:45:48  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.7  2004/04/05 22:58:54  d3x0r
// Take advantage of extended 16 bit col pos
//
// Revision 1.6  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.5  2003/03/31 01:19:08  panther
// Remove unused option
//
// Revision 1.4  2003/03/25 08:59:02  panther
// Added CVS logging
//
