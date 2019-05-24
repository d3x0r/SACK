
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
   	uint32_t bInput : 1;
   	uint32_t bOutput : 1;
		uint32_t bLog : 1;
   } flags;
   PLINKQUEUE input;
} MYDATAPATH, *PMYDATAPATH;

static TEXTCHAR *Ops[] = {
	"FORMAT_OP_CLEAR_END_OF_LINE",
	"FORMAT_OP_CLEAR_START_OF_LINE",
	"FORMAT_OP_CLEAR_LINE ",
	"FORMAT_OP_CLEAR_END_OF_PAGE",
	"FORMAT_OP_CLEAR_START_OF_PAGE",
	"FORMAT_OP_CLEAR_PAGE",
	"FORMAT_OP_CONCEAL" 
	  , "FORMAT_OP_DELETE_CHARS" // background is how many to delete.
	  , "FORMAT_OP_SET_SCROLL_REGION" // format.x, y are start/end of region -1,-1 clears.
	  , "FORMAT_OP_GET_CURSOR" // this works as a transaction...
	  , "FORMAT_OP_SET_CURSOR" // responce to getcursor...

	  , "FORMAT_OP_PAGE_BREAK" // clear page, home page... result in page break...
	  , "FORMAT_OP_PARAGRAPH_BREAK" // break between paragraphs - kinda same as lines...
};

//---------------------------------------------------------------------------

static void BuildTextFlags( PVARTEXT vt, PTEXT pSeg )
{
	vtprintf( vt, "Text Flags: ");
	if( pSeg->flags & TF_STATIC )
		vtprintf( vt, "static " );
	if( pSeg->flags & TF_QUOTE )
		vtprintf( vt, "\"\" " );
	if( pSeg->flags & TF_SQUOTE )
		vtprintf( vt, "\'\' " );
	if( pSeg->flags & TF_BRACKET )
		vtprintf( vt, "[] " );
	if( pSeg->flags & TF_BRACE )
		vtprintf( vt, "{} " );
	if( pSeg->flags & TF_PAREN )
		vtprintf( vt, "() " );
	if( pSeg->flags & TF_TAG )
		vtprintf( vt, "<> " );
	if( pSeg->flags & TF_INDIRECT )
		vtprintf( vt, "Indirect " );
   /*
	if( pSeg->flags & TF_SINGLE )
	vtprintf( vt, "single " );
   */
	if( pSeg->flags & TF_FORMATREL )
      vtprintf( vt, "format x,y(REL) " );
	if( pSeg->flags & TF_FORMATABS )
      vtprintf( vt, "format x,y " );
   else
		vtprintf( vt, "format spaces " );

	if( pSeg->flags & TF_COMPLETE )
		vtprintf( vt, "complete " );
	if( pSeg->flags & TF_BINARY )
		vtprintf( vt, "binary " );
	if( pSeg->flags & TF_DEEP )
		vtprintf( vt, "deep " );
	if( pSeg->flags & TF_ENTITY )
		vtprintf( vt, "entity " );
	if( pSeg->flags & TF_SENTIENT )
		vtprintf( vt, "sentient " );
	if( pSeg->flags & TF_NORETURN )
		vtprintf( vt, "NoReturn " );
	if( pSeg->flags & TF_LOWER )
		vtprintf( vt, "Lower " );
	if( pSeg->flags & TF_UPPER )
		vtprintf( vt, "Upper " );
	if( pSeg->flags & TF_EQUAL )
		vtprintf( vt, "Equal " );
	if( pSeg->flags & TF_TEMP )
		vtprintf( vt, "Temp " );
	if( pSeg->flags & TF_PROMPT )
		vtprintf( vt, "Prompt " );
	if( pSeg->flags & TF_PLUGIN )
		vtprintf( vt, "Plugin=%02x ", (uint8_t)(( pSeg->flags >> 26 ) & 0x3f ) );
	
	if( (pSeg->flags & TF_FORMATABS ) )
		vtprintf( vt, "Pos:%d,%d "
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else if( (pSeg->flags & TF_FORMATREL ) )
		vtprintf( vt, "Rel:%d,%d "
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else
		vtprintf( vt, "%d spaces "
				, pSeg->format.position.offset.spaces );
	
	if( pSeg->flags & TF_FORMATEX )
		vtprintf( vt, "format extended(%s) length:%d"
		           , Ops[ pSeg->format.flags.format_op
		                - FORMAT_OP_CLEAR_END_OF_LINE ] 
		           , GetTextSize( pSeg ) );
	else
		vtprintf( vt, "Fore:%d Back:%d length:%d"
					, pSeg->format.flags.foreground
					, pSeg->format.flags.background 
					, GetTextSize( pSeg ) );
}


static PTEXT CPROC ProcessSegments( PMYDATAPATH pdp, PTEXT text, TEXTCHAR *dir )
{
	if( pdp->flags.bLog )
		Log( "Process debug segments" );
	if( text )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT textsave = text;
		while( text )
		{
         PTEXT info;
			vtprintf( pvt, "%s Seg: ", dir );
			BuildTextFlags( pvt, text );
			vtprintf( pvt, "%s", GetText( text ) );
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
	return ProcessSegments( pdp, text, "input" );
}

//---------------------------------------------------------------------------

static PTEXT CPROC ProcessOutputSegments( PMYDATAPATH pdp, PTEXT text )
{
	return ProcessSegments( pdp, text, "output" );
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
		if( OptionLike( option, "inbound" ) )
			pdp->flags.bInput = 1;
		else if( OptionLike( option, "outbound" ) )
			pdp->flags.bOutput = 1;
		else if( OptionLike( option, "log" ) )
			pdp->flags.bLog = 1;
	}
	
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
   myTypeID = RegisterDevice( "debug", "Shows per-segment information (inbound only)...", Open );
	//   return DekVersion;
	}
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( "debug" );
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
