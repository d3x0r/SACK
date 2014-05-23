// Merge is the mating peice for a split.
// a merge connection must exist on an object for split to work.

#include <stdhdrs.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"
#include "mergedatapath.h"
static int myTypeID;

static void AddSplitter( PMERGEDATAPATH pMerger, PDATAPATH pSplitter )
{
	PSPLITTER newsplit = New( SPLITTER );
	newsplit->pSplitter = pSplitter;
	if( ( newsplit->next = pMerger->connections ) )
	{
      pMerger->connections->me = &newsplit->next;
	}
	newsplit->me = &pMerger->connections;
}

static void RemoveSplitter( PMERGEDATAPATH pMerger, PDATAPATH pSplitter )
{
	PSPLITTER oldsplit;
	oldsplit = pMerger->connections;
	while( oldsplit )
	{
		if( oldsplit->pSplitter == pSplitter )
			break;
      oldsplit = oldsplit->next;
	}
	if( oldsplit )
	{
		if( ( *oldsplit->me = oldsplit->next ) )
         oldsplit->next->me = oldsplit->me;

	}
}

static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, NULL );
}

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( (PDATAPATH)pdp, NULL );
}

static int CPROC Close( PMERGEDATAPATH pdp )
{
   pdp->common.Type = 0;
   return 0;
}

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMERGEDATAPATH pdp = NULL;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MERGEDATAPATH );
   pdp->AddSplitter = AddSplitter;
   pdp->RemoveSplitter = RemoveSplitter;
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}


PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("merge"), WIDE("Performs no translation..."), Open );
   return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("merge") );
}
// $Log: merge.c,v $
// Revision 1.8  2005/02/21 12:08:34  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.7  2005/01/18 02:46:59  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.6  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.5  2003/03/25 08:59:02  panther
// Added CVS logging
//
