// split filter - takes the stream and redirects half of it to 
// another object.  Said object must have a merge filter applied to 
// accept the split.
#include <stdhdrs.h>
#include <logging.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;

#include "../merge/mergedatapath.h"


typedef struct mydatapath_tag {
   DATAPATH common;
   struct {
   	_32 inbound:1;
        _32 outbound:1;
        _32 intoin:1; // else in to out if inbound
        _32 outtoin:1; // else out to out if outbound
   } flags;
   PMERGEDATAPATH pMerger;  // thing to receive the split information
} MYDATAPATH, *PMYDATAPATH;


static PTEXT CPROC SplitInbound( PMYDATAPATH pdp, PTEXT line )
{
    PTEXT dup = TextDuplicate( line, FALSE );
    if( pdp->flags.intoin )
        EnqueLink( &pdp->pMerger->common.Input, dup );
    else
        EnqueLink( &pdp->pMerger->common.Output, dup );
    return line;
}


static int CPROC Read( PMYDATAPATH pdp )
{
    if( pdp->flags.inbound )
        return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))SplitInbound );
    return RelayInput( (PDATAPATH)pdp, NULL );
}

static PTEXT CPROC SplitOutbound( PMYDATAPATH pdp, PTEXT line )
{
	PTEXT dup = TextDuplicate( line, FALSE );
	static int count;
	if( pdp->flags.outtoin )
	{
		EnqueLink( &pdp->pMerger->common.Input, dup );
		if( count++ > 100 )
		{
			count = 0;
			Relinquish(); // allow other sentients to process...
		}
	}
	else
		EnqueLink( &pdp->pMerger->common.Output, dup );
	return line;
}

static int CPROC Write( PMYDATAPATH pdp )
{
	if( pdp->flags.outbound )
	{
		return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))SplitOutbound );
	}
	return RelayOutput( (PDATAPATH)pdp, NULL );
}

static int CPROC Close( PMYDATAPATH pdp )
{
    pdp->pMerger->RemoveSplitter( pdp->pMerger, (PDATAPATH)pdp );
    pdp->common.Type = 0;
    return 0;
}

#define OptionLike(text,string) ( strnicmp( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   PTEXT option;
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   // parameters
   //    object name
   //    both/inbound/outbound - which channel to split off
	//
	lprintf( WIDE("Creating split datapath...") );
   while( ( option = GetParam( ps, &parameters ) ) )
   {
       if( OptionLike( option, WIDE("both") ) )
       {
           pdp->flags.outbound = 1;
           pdp->flags.inbound = 1;
       }
       else if( OptionLike( option, WIDE("inbound") ) )
       {
			 lprintf( WIDE("Creating inbound splitter...") );
           pdp->flags.inbound = 1;
       }
       else if( OptionLike( option, WIDE("outbound") ) )
       {
           pdp->flags.outbound = 1;
       }
       else if( OptionLike( option, WIDE("outtoin") ) )
       {
           pdp->flags.outtoin = 1;
       }
       else if( OptionLike( option, WIDE("intoin") ) )
       {
           pdp->flags.intoin = 1;
       }
       else
       {
           PENTITY pe = NULL;
			  if( option == ps->Current->pName )
			  {
				  lprintf( WIDE("(pe = me)...") );
				  pe = ps->Current;
			  }
			  if( !pe )
			  {
              lprintf( WIDE("looking for entity %s"), GetText( option ) );
				  pe = (PENTITY)FindThing( ps, &option, ps->Current, FIND_VISIBLE, NULL );
              if( pe ) lprintf( WIDE("found.") ); else lprintf( WIDE("not found.") );
			  }
           if( !pe )
           {
               DECLTEXT( msg, WIDE("Unknown parameter or entity to split filter") );
					EnqueLink( &ps->Command->Output, &msg );
               lprintf( WIDE("should have logged output...") );
               pdp->flags.inbound = pdp->flags.outbound = 0;
           }
           else
           {
               DECLTEXT( merge, WIDE("merge") );
					pdp->pMerger = (PMERGEDATAPATH)FindOpenDevice( ps, (PTEXT)&merge );
               //if( merger )
					{
                  //lprintf( WIDE("foudn datapth type ID...") );
                   //pdp->pMerger = (PMERGEDATAPATH)FindDatapath( pe->pControlledBy, TypeID );
                   if( pdp->pMerger )
						 {
                      lprintf( WIDE("COnnecting splitter to merger...") );
                       pdp->pMerger->AddSplitter( pdp->pMerger, (PDATAPATH)pdp );
                   }
                   else
                   {
                       DECLTEXT( msg, WIDE("Entity does not have a merge filter available.") );
                       EnqueLink( &ps->Command->Output, &msg );
                       pdp->flags.inbound = pdp->flags.outbound = 0;
                   }
					}
               /*
               else
               {
                   DECLTEXT( msg, WIDE("No device called 'merge' available in system.") );
                   EnqueLink( &ps->Command->Output, &msg );
                   pdp->flags.inbound = pdp->flags.outbound = 0;
               }
               */
           }
       }
   }
   if( !pdp->flags.outbound && !pdp->flags.inbound )
	{
      lprintf( WIDE("splitter is not doing in or out - destroying it.") );
       DestroyDataPath( (PDATAPATH)pdp );
       return NULL;
   }
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("split"), WIDE("Splits datapath to another object..."), Open );
   return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("split") );
}
//------------------------------------------------------
// $Log: split.c,v $
// Revision 1.12  2005/04/20 06:20:24  d3x0r
// Okay a massive refit to 'FindThing' To behave like GetParam(SubstToken) to handle count.object references better.
//
// Revision 1.11  2005/02/21 12:08:39  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.10  2005/01/17 08:45:48  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.9  2004/05/12 10:04:55  d3x0r
// checkpoint
//
// Revision 1.8  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.7  2002/09/16 01:13:56  panther
// Removed the ppInput/ppOutput things - handle device closes on
// plugin unload.  Attempting to make sure that we can restore to a
// clean state on exit.... this lets us track lost memory...
//
// Revision 1.6  2002/09/09 02:58:13  panther
// Conversion to new make system - includes explicit coding of exports.
//
// Revision 1.5  2002/07/15 08:17:34  panther
// If relaying much data (from a file source perhaps) this version
// allows a relinquish to happen to allow the other object being split
// to to process its data.  Otherwise we end up with HUGE queues.
//
// 
