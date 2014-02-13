// totally useless filter 
// other than this filter provides a shell for all other filters.

#include <stdhdrs.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"

static int myTypeID;

typedef struct mydatapath_tag {
   DATAPATH common;
   //COMMAND_INFO CommandInfo;
   PLINKQUEUE output;
} MYDATAPATH, *PMYDATAPATH;

static PTEXT CPROC ParseCommand( PMYDATAPATH pdp, PTEXT buffer )
{
	if( buffer )
	{
		PTEXT pCommand;
     	//Log2( WIDE("buffer: %s(%d)"), GetText( buffer ), GetTextSize( buffer ) );
     	//LogBinary( buffer );
      pCommand = burst( buffer );
      LineRelease( buffer );
      if( pCommand )
      {
         PTEXT pTemp, pStart;
         int bEscaped = FALSE;
         pStart = pTemp = pCommand;
         while( pTemp )
         {
            if( TextIs( pTemp, WIDE("\\") ) )
            {
               if( !bEscaped )
               {
                  PTEXT pNext;
                  pNext = NEXTLINE( pTemp );
                  pNext->format.position.offset.spaces = pTemp->format.position.offset.spaces;
                  LineRelease( SegGrab( pTemp ) );
                  bEscaped = TRUE;
                  pTemp = pNext;
                  continue;
               }
            }
            if( !bEscaped && TextIs( pTemp, WIDE(";") ) )
            {
               PTEXT pPrior;
	           	//Log( WIDE("Splitting the line and enqueing it...") );
               pPrior = pTemp;
               SegBreak( pTemp );
               if( pStart != pTemp )
               {
               	// end of line included!
               	pStart = SegAppend( pStart, SegCreate(0) ); 
                  EnqueLink( &pdp->output, pStart );
               }
               pStart = pTemp = NEXTLINE( pTemp );
               SegBreak( pTemp );
               LineRelease( pPrior ); // remove ';'
               bEscaped = FALSE;
               continue;
            }
            bEscaped = FALSE;
            pTemp = NEXTLINE( pTemp );
         }
         if( pStart )
         {
         	PTEXT line;
         	line = BuildLine( pStart );
         	//Log1( WIDE("Enqueu: %s"), GetText( line ) );
         	LineRelease( line );
            EnqueLink( &pdp->output, pStart );
         }
      }
      else
      {
         // well what now?  I guess pLine got put into Partial...
         // next pass through this all for command recall will blow...
         Log( WIDE("No information from the burst!") );
      }
	}
	return (PTEXT)DequeLink( &pdp->output );
}
//---------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))ParseCommand );
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
   myTypeID = RegisterDevice( WIDE("command"), WIDE("Performs command processing burst, history..."), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("command") );
}
// $Log: command.c,v $
// Revision 1.9  2005/02/21 12:08:31  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.8  2005/01/28 16:02:18  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.7  2005/01/17 08:45:47  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.6  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.5  2003/03/25 08:59:02  panther
// Added CVS logging
//
