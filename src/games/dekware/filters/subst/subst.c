


// totally useless filter 
// other than this filter provides a shell for all other filters.

#include <logging.h>

#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;

typedef struct mydatapath_tag {
	DATAPATH common;
	struct {
		_32 bOutbound : 1;
		_32 bInbound : 1;
	} flags;
} MYDATAPATH, *PMYDATAPATH;



static PTEXT CPROC Subst( PMYDATAPATH pdp, PTEXT text )
{
	PMACRO_STATE pms;
   if( pdp->common.Owner )
		pms = PeekStack( &pdp->common.owner->MacroStack );
	else
		pms = NULL;
	return MacroDuplicateExx( /*PSENTIENT*/ pdp->common.owner
									, /*PTEXT*/ text
									, TRUE /*bKeepEOL*/
									, TRUE /*bSubst*/
									, pms?pms->pArgs:NULL /*pArgs*/
									, DBG_SRC );
}


//---------------------------------------------------------------------------

static int CPROC Read( PDATAPATH pdp )
{
   if( ((PMYDATAPATH)pdp)->flags.bInbound )
		return RelayInput( (PDATAPATH)pdp, Subst );
}

//---------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
   if( ((PMYDATAPATH)pdp)->flags.bOutbound )
		return RelayOutput( (PDATAPATH)pdp, Subst );
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdp )
{
   PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
   pmdp->common.Type = 0; // allow delete
   return 0;
}

//---------------------------------------------------------------------------

#define OptionLike(text,string) ( strnicmp( GetText(text), string, GetTextSize( text ) ) == 0 )

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
	pdp->flags.bOutbound = 1;
   pdp->flags.bInbound = 1;
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( char *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( "subst", "Performs standard param subst(uses current macro, if any for args) translation...", Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( "nil" );
}
// $Log: subst.c,v $
// Revision 1.1  2005/07/21 04:02:37  d3x0r
// Wow cheap filter for powerful substitutions.
//
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


