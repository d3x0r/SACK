
#define DO_LOGGING
#include <stdhdrs.h>
// totally useless filter 
// other than this filter provides a shell for all other filters.

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
   PLINKQUEUE input;
} MYDATAPATH, *PMYDATAPATH;

static TEXTCHAR *Ops[] = {
	WIDE("FORMAT_OP_CLEAR_END_OF_LINE"),
	WIDE("FORMAT_OP_CLEAR_START_OF_LINE"),
	WIDE("FORMAT_OP_CLEAR_LINE "),
	WIDE("FORMAT_OP_CLEAR_END_OF_PAGE"),
	WIDE("FORMAT_OP_CLEAR_START_OF_PAGE"),
	WIDE("FORMAT_OP_CLEAR_PAGE"),
	WIDE("FORMAT_OP_CONCEAL") 
	  , WIDE("FORMAT_OP_DELETE_CHARS") // background is how many to delete.
	  , WIDE("FORMAT_OP_SET_SCROLL_REGION") // format.x, y are start/end of region -1,-1 clears.
	  , WIDE("FORMAT_OP_GET_CURSOR") // this works as a transaction...
	  , WIDE("FORMAT_OP_SET_CURSOR") // responce to getcursor...

	  , WIDE("FORMAT_OP_PAGE_BREAK") // clear page, home page... result in page break...
	  , WIDE("FORMAT_OP_PARAGRAPH_BREAK") // break between paragraphs - kinda same as lines...
};

//---------------------------------------------------------------------------

static void BuildTextFlags( PVARTEXT vt, PTEXT pSeg )
{
	vtprintf( vt, WIDE("Text Flags: "));
	if( pSeg->flags & TF_STATIC )
		vtprintf( vt, WIDE("static ") );
	if( pSeg->flags & TF_QUOTE )
		vtprintf( vt, WIDE("\"\" ") );
	if( pSeg->flags & TF_SQUOTE )
		vtprintf( vt, WIDE("\'\' ") );
	if( pSeg->flags & TF_BRACKET )
		vtprintf( vt, WIDE("[] ") );
	if( pSeg->flags & TF_BRACE )
		vtprintf( vt, WIDE("{} ") );
	if( pSeg->flags & TF_PAREN )
		vtprintf( vt, WIDE("() ") );
	if( pSeg->flags & TF_TAG )
		vtprintf( vt, WIDE("<> ") );
	if( pSeg->flags & TF_INDIRECT )
		vtprintf( vt, WIDE("Indirect ") );
   /*
	if( pSeg->flags & TF_SINGLE )
	vtprintf( vt, WIDE("single ") );
   */
	if( pSeg->flags & TF_FORMATREL )
      vtprintf( vt, WIDE("format x,y(REL) ") );
	if( pSeg->flags & TF_FORMATABS )
      vtprintf( vt, WIDE("format x,y ") );
   else
		vtprintf( vt, WIDE("format spaces ") );

	if( pSeg->flags & TF_COMPLETE )
		vtprintf( vt, WIDE("complete ") );
	if( pSeg->flags & TF_BINARY )
		vtprintf( vt, WIDE("binary ") );
	if( pSeg->flags & TF_DEEP )
		vtprintf( vt, WIDE("deep ") );
	if( pSeg->flags & TF_ENTITY )
		vtprintf( vt, WIDE("entity ") );
	if( pSeg->flags & TF_SENTIENT )
		vtprintf( vt, WIDE("sentient ") );
	if( pSeg->flags & TF_NORETURN )
		vtprintf( vt, WIDE("NoReturn ") );
	if( pSeg->flags & TF_LOWER )
		vtprintf( vt, WIDE("Lower ") );
	if( pSeg->flags & TF_UPPER )
		vtprintf( vt, WIDE("Upper ") );
	if( pSeg->flags & TF_EQUAL )
		vtprintf( vt, WIDE("Equal ") );
	if( pSeg->flags & TF_TEMP )
		vtprintf( vt, WIDE("Temp ") );
	if( pSeg->flags & TF_PROMPT )
		vtprintf( vt, WIDE("Prompt ") );
	if( pSeg->flags & TF_PLUGIN )
		vtprintf( vt, WIDE("Plugin=%02x "), (_8)(( pSeg->flags >> 26 ) & 0x3f ) );
	
	if( (pSeg->flags & TF_FORMATABS ) )
		vtprintf( vt, WIDE("Pos:%d,%d ")
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else if( (pSeg->flags & TF_FORMATREL ) )
		vtprintf( vt, WIDE("Rel:%d,%d ")
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else
		vtprintf( vt, WIDE("%d spaces ")
				, pSeg->format.position.offset.spaces );
	
	if( pSeg->flags & TF_FORMATEX )
		vtprintf( vt, WIDE("format extended(%s) length:%d")
		           , Ops[ pSeg->format.flags.format_op
		                - FORMAT_OP_CLEAR_END_OF_LINE ] 
		           , GetTextSize( pSeg ) );
	else
		vtprintf( vt, WIDE("Fore:%d Back:%d length:%d")
					, pSeg->format.flags.foreground
					, pSeg->format.flags.background 
					, GetTextSize( pSeg ) );
}


static PTEXT CPROC ProcessSegments( PMYDATAPATH pdp, PTEXT text, TEXTCHAR *dir )
{
	if( pdp->flags.bLog )
		Log( WIDE("Process debug segments") );
	if( text )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT textsave = text;
		while( text )
		{
         PTEXT info;
			vtprintf( pvt, WIDE("%s Seg: "), dir );
			BuildTextFlags( pvt, text );
			vtprintf( pvt, WIDE("%s"), GetText( text ) );
         info = VarTextGet( pvt );
			if( pdp->flags.bLog )
			{
				Log( GetText( info ) );
            LineRelease( info );
			}
			else
			{
				EnqueLink( &pdp->common.Owner->Command->Output, info );
			}
			text = NEXTLINE( text );
		}
      VarTextDestroy( &pvt );
		return textsave;
	}
	return text;
}

//---------------------------------------------------------------------------
static PTEXT CPROC ProcessInputSegments( PMYDATAPATH pdp, PTEXT text )
{
	return ProcessSegments( pdp, text, WIDE("input") );
}

//---------------------------------------------------------------------------

static PTEXT CPROC ProcessOutputSegments( PMYDATAPATH pdp, PTEXT text )
{
	return ProcessSegments( pdp, text, WIDE("output") );
}

//---------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	if( pdp->flags.bInput )
		return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))ProcessInputSegments );
   return RelayInput( (PDATAPATH)pdp, NULL );
}

//---------------------------------------------------------------------------

static int CPROC Write( PMYDATAPATH pdp )
{
	if( pdp->flags.bOutput )
		return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))ProcessOutputSegments );
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
			pdp->flags.bLog = 1;
	}
	
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("debug"), WIDE("Shows per-segment information (inbound only)..."), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("debug") );
}
// $Log: filtdbg.c,v $
// Revision 1.7  2005/02/21 12:08:33  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.6  2005/01/17 08:45:47  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.5  2004/09/29 09:31:31  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.4  2004/04/07 23:38:00  d3x0r
// Revise building of logging string a bit..>
//
// Revision 1.3  2004/01/19 23:42:25  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.3  2004/01/18 21:18:26  panther
// misc fixes... too bad sf is down
//
// Revision 1.2  2003/11/08 00:09:40  panther
// fixes for VarText abstraction
//
// Revision 1.1  2003/11/01 23:21:48  panther
// Renamed source so it's not erased on distclean
//
// Revision 1.13  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.12  2003/08/15 13:13:43  panther
// Add { } around log
//
// Revision 1.11  2003/04/13 22:28:51  panther
// Seperate computation of lines and rendering... mark/copy works (wincon)
//
// Revision 1.10  2003/04/08 06:46:39  panther
// update dekware - fix a bit of ansi handling
//
// Revision 1.9  2003/04/06 23:26:13  panther
// Update to new SegSplit.  Handle new formatting option (delete chars) Issue current window size to launched pty program
//
// Revision 1.8  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.7  2003/03/25 08:59:02  panther
// Added CVS logging
//
