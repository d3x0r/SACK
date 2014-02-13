// provides typical token burst on a data stream.

#include <stdhdrs.h>
#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;

typedef struct mydatapath_tag {
   DATAPATH common;

   struct {
		_32 outbound : 1;
   } flags;
   PTEXT tokens;
} MYDATAPATH, *PMYDATAPATH;

//--------------------------------------------------------------------------

static PTEXT CPROC Build( PMYDATAPATH pdp, PTEXT text )
{
	PTEXT result;
	if( text )
	{
		result = BuildLine( text );
	   result->flags |= text->flags & TF_NORETURN;
		LineRelease( text );
		return result;
	}
	return NULL;
}

//--------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	if( pdp->flags.outbound )
		return RelayInput( (PDATAPATH)pdp, NULL );
   return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))Build );
}

//--------------------------------------------------------------------------

static int CPROC Write( PMYDATAPATH pdp )
{
	if( pdp->flags.outbound )
		return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))Build );
   return RelayOutput( (PDATAPATH)pdp, NULL );
}

//--------------------------------------------------------------------------

static int CPROC Close( PMYDATAPATH pdp )
{
   pdp->common.Type = 0;
   return 0;
}

//--------------------------------------------------------------------------

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   PTEXT option;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   while( option = GetParam( ps, &parameters ) )
   {
	   if( OptionLike( option, WIDE("inbound") ) )
	   {
	   	pdp->flags.outbound = 0;
	   }
	   else if( OptionLike( option, WIDE("outbound") ) )
	   {
	   	pdp->flags.outbound = 1;
	   }
	   else
	   {
	   	//DECLTEXT( msg, WIDE("Unknown option for token filter. Allowed are 'inbound' and 'outbound'.") );
	   }
	}
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}

//--------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("splice"), WIDE("Splices Tokens into a line..."), Open );
   return DekVersion;
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("splice") );
}
// $Log: splicer.c,v $
// Revision 1.7  2005/02/21 12:08:37  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.6  2005/01/17 08:45:48  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.5  2003/10/29 02:56:16  panther
// Misc fixes with base filters.  Data throughpaths...
//
// Revision 1.4  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.3  2003/03/25 08:59:02  panther
// Added CVS logging
//
