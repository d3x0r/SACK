
#include <stdhdrs.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;

typedef struct mydatapath_tag {
	DATAPATH common;
   COMMAND_INFO CommandInfo;
} MYDATAPATH, *PMYDATAPATH;


//---------------------------------------------------------------------------

static PTEXT CPROC DoGather( PDATAPATH pdp, PTEXT Buffer )
{
   PMYDATAPATH pmdp = (PMYDATAPATH)pdp;
	return GatherLine( &pmdp->CommandInfo.CollectionBuffer
							 , &pmdp->CommandInfo.CollectionIndex
							 , pmdp->CommandInfo.CollectionInsert
							 , FALSE
							 , Buffer );
}

static int CPROC Read( PMYDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, DoGather );
}
	
		//---------------------------------------------------------------------------
		
static int CPROC Write( PMYDATAPATH pdp )
{
	return RelayOutput( (PDATAPATH)pdp, NULL );
}

//---------------------------------------------------------------------------

static int CPROC Close( PMYDATAPATH pdp )
{
   pdp->common.Type = 0;
   return 0;
}

//---------------------------------------------------------------------------

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   //PTEXT option;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC*)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC*)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC*)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("gather"), WIDE("Gathers lines command collection w/history..."), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("gather") );
}
// $Log: gather.c,v $
// Revision 1.1  2005/04/15 17:08:28  d3x0r
// Added gather project - which takes a TEXTCHAR-by-TEXTCHAR stream and prodcues lines.
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
