// this filter reads the input and converts it into a binary hexdump
// bytes and text display....
#include <stdhdrs.h>
#define DO_LOGGING
#include <logging.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"

static int myTypeID;

typedef struct mydatapath_tag {
   DATAPATH common;
   struct {
   	_32 bInput : 1;
		_32 bOutput : 1;
		_32 bLog : 1;
   } flags;
   PLINKQUEUE *current;
   PLINKQUEUE input;
   PLINKQUEUE output;
} MYDATAPATH, *PMYDATAPATH;

//--------------------------------------------------------------------------

static TEXTCHAR pHEX[] = WIDE("0123456789ABCDEF");
static PTEXT CPROC BinaryWrite( PMYDATAPATH pdp, PTEXT buffer )
{
   PTEXT pLine, bufstart = buffer;
   TEXTCHAR *pOut, *pIn;
	int n, ofs, sz, bLogged = FALSE;

   if( buffer )
	{
   	while( buffer )
	   {
   	   pIn = (TEXTCHAR*)GetText( buffer );
			sz = GetTextSize( buffer );
			if( !sz )
			{
            Log( WIDE("END OF LINE!") );
			}
   	   for( n = 0; n < sz; n += 16 )
      	{
				pLine = SegCreate( 16*3 + 2 + 16 );
			   //pLine = SegCreate( 16*3 + 2 + 16 );
         	pOut = (TEXTCHAR*)GetText( pLine );
	         for( ofs = 0; ofs < 16 && ofs < (sz - n); ofs++ )
   	      {
      	      pOut[0] = pHEX[pIn[n+ofs] / 16];
         	   pOut[1] = pHEX[pIn[n+ofs] & 0xF];
            	pOut[2] = ' ';
	            pOut += 3;
   	      }
      	   if( ofs < 16 )
         	{
            	for( ; ofs < 16; ofs ++ )
	            {
   	            pOut[0] = ' ';
      	         pOut[1] = ' ';
         	      pOut[2] = ' ';
            	   pOut += 3;
	            }
   	      }
      	   pOut[0] = '|';
         	pOut[1] = ' ';
	         pOut += 2;
   	      for( ofs = 0; ofs < 16 && ofs < (sz - n); ofs++ )
      	   {
         	   if( pIn[n+ofs] >= 32 )
            	   pOut[0] = pIn[n + ofs];
	            else
   	            pOut[0] = '.';
      	      pOut++;
         	}
	         if( ofs < 16 )
   	      {
      	      for( ; ofs < 16; ofs ++ )
         	   {
	               pOut[0] = ' ';
   	            pOut++;
      	      }
         	}
	         //pOut[0] = '\n';
   	      //pOut++;
      	   pOut[0] = 0;
				pLine->data.size = strlen( GetText( pLine ) );
				if( pdp->flags.bLog )
				{
               if( !bLogged )
					{
                  bLogged = TRUE;
						if( pdp->flags.bInput )
							Log1( WIDE("Inbound...:%s"), GetText( pdp->common.pName ) );
						if( pdp->flags.bOutput )
							Log1( WIDE("Outbound...:%s"), GetText( pdp->common.pName ) );
					}
					Log1( WIDE("%s"), GetText( pLine ) );
					LineRelease( pLine );
				}
				else
					EnqueLink( pdp->current, pLine );
   	   }
      	buffer = NEXTLINE( buffer );
		}
		if( pdp->flags.bLog )
			EnqueLink( pdp->current, bufstart );
      else
			LineRelease( bufstart );
	}
	return (PTEXT)DequeLink( pdp->current );
}

//--------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	if( pdp->flags.bInput )
	{
		pdp->current = &pdp->input;
		return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))BinaryWrite );
	}
	return RelayInput( (PDATAPATH)pdp, NULL );
}

//--------------------------------------------------------------------------

static int CPROC Write( PMYDATAPATH pdp )
{
	if( pdp->flags.bOutput )
	{
		pdp->current = &pdp->input;
		return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))BinaryWrite );
	}
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

static PDATAPATH CPROC CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   PTEXT option;
   // parameters
   //    none
   pdp = CreateDataPath( pChannel, MYDATAPATH );
	while( ( option = GetParam( ps, &parameters ) ) )
	{
		if( OptionLike( option, WIDE("inbound") ) )
			pdp->flags.bInput = 1;
		else if( OptionLike( option, WIDE("outbound") ) )
			pdp->flags.bOutput = 1;
		else if( OptionLike( option, WIDE("log") ) )
         pdp->flags.bLog = 1; // use Log instead of queuing input/output....
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
   myTypeID = RegisterDevice( WIDE("binary"), WIDE("Performs binary formatting..."), Open );
   return DekVersion;
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("binary") );
}
// $Log: binary.c,v $
// Revision 1.14  2005/08/08 09:32:02  d3x0r
// fix releasing the line used for temp logging... especially for logging to data path
//
// Revision 1.13  2005/02/21 12:08:31  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.12  2005/01/28 16:02:18  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.11  2005/01/17 08:45:47  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.10  2004/01/19 23:42:25  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.9  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.8  2003/03/31 14:47:04  panther
// Minor mods to filters
//
// Revision 1.7  2003/03/26 02:15:40  panther
// Log end of line characteristic
//
// Revision 1.6  2003/03/25 08:59:02  panther
// Added CVS logging
//
