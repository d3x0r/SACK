#include <stdhdrs.h>
#include <psi.h>

#include "consolestruc.h"

PSI_CONSOLE_NAMESPACE

static int colormap[8] = { 0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 14, 13, 15 };
extern CONTROL_REGISTRATION ConsoleClass;


struct mydatapath_tag {
	//DATAPATH common;
	PCONSOLE_INFO console;
	// ANSI burst state variables...
	int nState;
	int nParams;
	int ParamSet[12];
	TEXTCHAR repeat_character; // set at '\x1b' so that \x1b[#b can repeat it.
	FORMAT attribute; // keep this for continuous attributes across buffers
	PLINKQUEUE Pending;
	int savex, savey;
	PVARTEXT vt;
	int cursorX, cursorY;
	void (*writeCallback)( uintptr_t psv, PTEXT seg );
	uintptr_t psvWriteCallback;
	void (*setTitle)( uintptr_t psv, PTEXT title );
	uintptr_t psvTitleCallback;
	PVARTEXT OC_code;
	PVARTEXT OC;
	int OC_code_length;
	int OC_code_gathered;
	// default is inbound, decode
	struct mydatapath_flags{
		uint32_t outbound	: 1;
		uint32_t inbound	: 1;
		uint32_t encode_in	: 1;
		uint32_t encode_out	: 1;
		uint32_t bNewLine	: 1;
		uint32_t bPosition	: 1; // set TF_FORMATABS
		uint32_t bPositionRel	: 1; // set TF_FORMATREL
		uint32_t bExtended	: 1; // set TF_FORAMTEX
		uint32_t bCollectingParam : 1;
		uint32_t bAttribute      : 1;
		uint32_t keep_newline    : 1;
		uint32_t wait_for_cursor : 1;
		uint32_t newline_only_in : 1;
		uint32_t newline_only_out : 1;
		uint32_t ST_escape : 1;
		uint32_t OC_LongPrompt : 1;
		uint32_t OC_Link : 1;
		uint32_t OC_get_length : 1;
		uint32_t OC_LongPromptCollect : 1;
		uint32_t extended_cursor : 1;
	} flags;
};

typedef struct mydatapath_tag  MYDATAPATH, * PMYDATAPATH;

//---------------------------------------------------------------------------

//void IncomingAnsi( PMYDATAPATH pdp, PTEXT pBuffer )
//{
//   INDEX idx, store = 0;
//   TEXTCHAR *p;
//   int state = 0;
//   p = pBuffer->data.data;
//   // filter out escape parameters from windows telnet...
//   // this should be like <ESC>[A <ESC>[B, etc...
//
//   for( idx = 0; idx < pBuffer->data.size; idx++ )
//   {
//      switch( state )
//      {
//      case 0:
//         if( p[idx] == 27 )
//         {
//
//            state = 1;
//         }
//         else
//            p[store++] = p[idx];
//         break;
//      case 1:
//         if( p[idx] == 'O' )
//         {
//            state = 2;
//         }
//         else
//         {
//            /*
//            ClearLastCommandOutput();
//            if( pdp->nHistory != -1 )
//            {
//               pdp->nHistory = -1;
//               pdp->bRecalled = FALSE;
//            }
//            */
//            p[store++] = p[idx-1];
//            p[store++] = p[idx];
//         }
//         break;
//      case 2:
//         if( p[idx] == 'A' )  // up arrow
//         {
//            ClearLastCommandOutput();
//            RecallCommand( &pdp->CommandInfo, TRUE );   
//            SendTCP( pdp->handle
//                   , GetText( pdp->CommandInfo.CollectionBuffer )
//                   , GetTextSize( pdp->CommandInfo.CollectionBuffer ) );
//         }
//         else if( p[idx] == 'B' ) // down arrow
//         {
//            ClearLastCommandOutput();
//            RecallCommand( &pdp->CommandInfo, FALSE );   
//            SendTCP( pdp->handle
//                   , GetText( pdp->CommandInfo.CollectionBuffer )
//                   , GetTextSize( pdp->CommandInfo.CollectionBuffer ) );
//			}
//			else if( p[idx] == 'C' ) // right arrow...
//			{
//			}
//			else if( p[idx] == 'D' ) // left arrow....
//			{
//			}
//			else
//			{ // otherwise data will be shifted down...
//				/*
//				ClearLastCommandOutput();
//				if( pdp->nHistory != -1 )
//				{
//					pdp->nHistory = -1;
//					pdp->bRecalled = FALSE;
//				}
//				*/
//				p[store++] = p[idx-2];
//				p[store++] = p[idx-1];
//				p[store++] = p[idx];
//			}
//			state = 0;
//			break;
//		}
//	}
//	pBuffer->data.size = store;
//}

static PTEXT CPROC AnsiEncode( PMYDATAPATH pmdp, PTEXT pBuffer )
{
	// this would handle things like keyboard and mouse motion into ansi codes...
	// null operation for now... incoming doesn't even work :(
	return pBuffer;
}

// all in all there is but one circum stance that any data is returned from this
// new data, without extended attributes (positioning, screen controls), and there
// was old data (or not) then all is well.
// an attribute alone can enque the new segment with existing data
// for example a full line will result from 'blackmud.com' s score command.
static PTEXT EnqueSegments( PMYDATAPATH pmdp
						  , PTEXT pReturn
						 , LOGICAL bReturn
						  DBG_PASS
						 ) // special handling at newline...
#define EnqueSegments( pmdp, pRet, bRet ) EnqueSegments( pmdp, pRet, bRet DBG_SRC )
{
	PTEXT pNew;
	PTEXT pTemp;
	if( pNew  = VarTextGet( pmdp->vt ) )
	{
		if( pmdp->flags.bAttribute 
			 || pmdp->flags.bPosition 
			 || pmdp->flags.bPositionRel 
			 || pmdp->flags.bExtended 
		  )
		{
			_lprintf( DBG_RELAY )( "Attribute ansi parser" );
			pNew->format                           = pmdp->attribute;
			pmdp->attribute.flags.prior_background = 1;
			pmdp->attribute.flags.prior_foreground = 1;
			pmdp->attribute.position.offset.spaces = 0;
			pmdp->flags.bAttribute = 0;
		}
		if( !pmdp->flags.bNewLine )
		{
			pNew->flags |= TF_NORETURN;
			_lprintf( DBG_RELAY )( "Not a newline, clear newline on empty segment" );
		}
		if( pmdp->flags.bPosition )
		{
			_lprintf( DBG_RELAY )( "Set abs position %d %d"
				 , pNew->format.position.coords.x
				 , pNew->format.position.coords.y
				 );
			pNew->flags |= TF_FORMATABS|TF_NORETURN;
		}
		if( pmdp->flags.bPositionRel )
		{
			_lprintf( DBG_RELAY )( "Set rel position" );
			pNew->flags |= TF_FORMATREL|TF_NORETURN;
		}
		if( pmdp->flags.bExtended )
			pNew->flags |= TF_FORMATEX|TF_NORETURN;
		_lprintf( DBG_RELAY )( "Enquing segments %s", GetText( pNew ) );
		pmdp->flags.bPosition = 0;
		pmdp->flags.bPositionRel = 0;
		pmdp->flags.bExtended = 0;
		pmdp->flags.bNewLine = 0;
		// new segment has attributes attached
		// cannot be part of an existing line.
		if( pNew->flags & (TF_FORMATEX|TF_FORMATREL|TF_FORMATABS) )
		{
		  // Log( "making a new return... enquing return" );
			EnqueLink( &pmdp->Pending, pReturn );
			pReturn = pNew;
		}
		else // otherwise attach it to any prior input....
		{
			//Log( "appending pReturn with pNew" );
			pReturn = SegAppend( pReturn, pNew );
		}
		// must now terminate any data if was a return.
		if( bReturn )
		{
			//Log( "Enquing previous data... " );
			EnqueLink( &pmdp->Pending, pReturn );
			pReturn = NULL;
		}
		//EnqueLink( &pmdp->Pending, pReturn );
		_lprintf( DBG_RELAY )( "Returning %p", pReturn );
		return pReturn;
	}
	// didn't have any data collected... see if there
	// was perhaps a collected attribute in any way.

	pTemp = NULL;
	
	// have to make a NULL segment...
	if( pmdp->flags.bExtended ) {
		if( !pTemp )
			pTemp = SegCreate( 0 );
		pTemp->flags |= TF_FORMATEX | ( pmdp->flags.bNewLine ? 0 : TF_NORETURN );
		pmdp->flags.bExtended = 0;
		_lprintf( DBG_RELAY )( "extended set... made temp" );
	}
	if( pmdp->flags.bPosition )
	{
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATABS|TF_NORETURN;
		pmdp->flags.bPosition = 0;
		_lprintf( DBG_RELAY )( "position set... made temp" );
	}
	if( pmdp->flags.bPositionRel )
	{
		//	Log( "Set rel position" );
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATREL|TF_NORETURN;
		pmdp->flags.bPositionRel = 0;
		_lprintf( DBG_RELAY )( "position relative set... made temp" );
	}
	if( pTemp )
	{  // any attribute which was collected must be issued (all alone)
		// as there was no more input to attach it to...
		//Log1( DBG_FILELINEFMT "Enquing format only segment" DBG_RELAY , 0 );
		pTemp->format = pmdp->attribute;
		if( pTemp->flags & TF_FORMATABS ) {
			lprintf( "Absolute position set: %d, %d", pTemp->format.position.coords.x, pTemp->format.position.coords.y );
		}
		if( pTemp->flags & TF_FORMATREL ) {
			lprintf( "Relative position set: %d, %d", pTemp->format.position.offset.tabs,
					 pTemp->format.position.offset.spaces );
		}
		// and reset the prior attributes
		pmdp->attribute.flags.prior_background = 1;
		pmdp->attribute.flags.prior_foreground = 1;
		pmdp->attribute.position.offset.spaces = 0;
		pmdp->flags.bNewLine = 0;
		_lprintf( DBG_RELAY )( "Enque a blank temp thing %p ", pReturn );
		EnqueLink( &pmdp->Pending, pReturn );
		EnqueLink( &pmdp->Pending, pTemp );
		return NULL;
	}
	// no new data... no attributed data...
	// must return - and already was a newline
	// force a total blank newline.
	//Log( "Test for return..." );
	if( bReturn && pmdp->flags.bNewLine )
	{  // but - we have a newline...
		PTEXT blank = SegCreate(0);
		//blank->data.data[0] = ' ';
		//blank->data.data[1] = 0;
		_lprintf( DBG_RELAY )( "enquing a blank." );
		EnqueLink( &pmdp->Pending, blank );
		// there will be a newline (pReturn) so don't bother clearing it.
		//pmdp->flags.bNewLine = 0;
	}
	else
	{
		// Log( "Enquing pReturn" );
		EnqueLink( &pmdp->Pending, pReturn );
	}
	//Log( "Returning NULL" );
	return NULL;
}


void AnsiBurst( PMYDATAPATH pmdp, PTEXT pBuffer )
{
	// this function is expecting a single buffer segment....
	INDEX idx;
	TEXTCHAR *ptext;
	PTEXT pReturn = NULL, pDelete = pBuffer;
	//lprintf( "ansiburst... %d", pmdp->flags.wait_for_cursor);
	if( pmdp->flags.wait_for_cursor )
		return;// (PTEXT)DequeLink( &pmdp->Pending );
	pmdp->flags.bNewLine = !( pBuffer->flags & TF_NORETURN );
	// also may be called with no input.
	LogBinary( GetText( pBuffer ), GetTextSize( pBuffer ) );
	while( pBuffer ) //may have been restrung into multiple buffers...
	{
		if( pBuffer->flags & TF_BINARY )
		{
			pBuffer = NEXTLINE( pBuffer );
			continue;
		}
		ptext = GetText( pBuffer );
		for( idx = 0; idx < pBuffer->data.size; idx++ )
		{
			if( !pmdp->nState )
			{
				switch( ptext[idx] )
				{
				default:
					pmdp->repeat_character = ptext[idx];
					VarTextAddCharacter( pmdp->vt, ptext[idx] );
					break;
				case 0:
					// ignore nulls?
					//ptext[idx] = ' '; // space fill nulls...
					lprintf( "Ignoring nulls..." );
					break;
				case '\r':
					pReturn = EnqueSegments( pmdp, pReturn, FALSE );
					pmdp->attribute.position.coords.y = -16384; // ignore position
					pmdp->attribute.position.coords.x = 0; // return left column.
					pmdp->flags.bPosition = 1;
					break;
				case '\b':
					pReturn = EnqueSegments( pmdp, pReturn, FALSE );
					pmdp->attribute.position.coords.y = 0;
					pmdp->attribute.position.coords.x = -1;
					pmdp->flags.bPositionRel = 1;
					break;
				case '\n':
					{
						// try getting any data we've already collected...
						//Log( "Newline character - flush exisiting" );
					// flushes any segments also...
						if( pmdp->flags.newline_only_in )
						{
							pReturn = EnqueSegments( pmdp, pReturn, FALSE );
							pmdp->attribute.position.coords.y = -16384; // ignore position
							pmdp->attribute.position.coords.x = 0; // return left column.
							pmdp->flags.bPosition = 1;
							pReturn = EnqueSegments( pmdp, pReturn, TRUE );
							pmdp->attribute.position.coords.y = 1;
							pmdp->attribute.position.coords.x = 0;
							pmdp->flags.bPositionRel = 1;
						}
						else
						{
							pReturn = EnqueSegments( pmdp, pReturn, TRUE );

							pmdp->attribute.position.coords.y = 1;
							pmdp->attribute.position.coords.x = 0;
							pmdp->flags.bPositionRel = 1;
						}
																 // next line will start with a newline.
						pmdp->flags.bNewLine = TRUE;
					}
					break;
				case 27:
					//Log( "Escape - gather prior to attach attributes to next." );
					pReturn = EnqueSegments( pmdp, pReturn, FALSE );
					pmdp->nState = 1;
					break;
				}
			}
			else if( pmdp->nState == 1 )
			{
				if( ptext[idx] == '[' )
				{
					if( pmdp->nParams )
						Log( "Invalid states... nparams beginning sequence." );
					pmdp->nState = 2;
					continue;
				} else if( ptext[idx] == ']' ) {
					// begin system command, terminate wwith \x1b\\ or ^G
					//lprintf( "Begin gathering command");
					pmdp->flags.OC_get_length = 1;
					pmdp->OC_code_length = pmdp->OC_code_gathered = 0;
					pmdp->nState = 3;
					if( !pmdp->OC ) pmdp->OC = VarTextCreate();
					else VarTextEmpty( pmdp->OC );
					if( !pmdp->OC_code ) pmdp->OC_code = VarTextCreate();
					else VarTextEmpty( pmdp->OC_code );

				} else if( ptext[idx] == '\\' ) {
					// end system command
				} else if( ptext[idx] == '7' ) {
					// save cursor and attributes(next step)
				} else if( (ptext[idx] == '7') || 
					 (ptext[idx]=='s') )
				{
					pmdp->attribute.flags.format_op = FORMAT_OP_GET_CURSOR;
					pmdp->flags.bExtended = 1;
					pmdp->flags.wait_for_cursor = 1;
					//pmdp->savex = GetVolatileVariable( pmdp->common.Owner->Current
					//												, "cursorx" );
					//pmdp->savey = GetVolatileVariable( pmdp->common.Owner->Current
					//												, "cursory" );

					// save cursor position
					pmdp->nState = 0; // reset state.
					pReturn = EnqueSegments( pmdp, pReturn, FALSE );
					EnqueLink( &pmdp->Pending, pReturn );
					break;
				}
				else if( ptext[idx] == '8' )
				{
					// restore cursor position
					pmdp->attribute.position.coords.x = pmdp->savex;
					pmdp->attribute.position.coords.y = pmdp->savey;
					pmdp->flags.bPosition = 1;
					pmdp->nState = 0; // reset state.
					continue;
				}
				else
				{
					VarTextAddCharacter( pmdp->vt, ptext[idx] );
					pmdp->nState = 0;
				}
			}
			else if( pmdp->nState == 2 )
			{
				if( ( ( ptext[idx] >= 'A' ) && ( ptext[idx] <= 'Z' ) ) ||
					 ( ( ptext[idx] >= 'a' ) && ( ptext[idx] <= 'z' ) ) )
				{
					if( pmdp->flags.bCollectingParam )
					{
						pmdp->flags.bCollectingParam = 0;
						pmdp->nParams++;
					}	
					lprintf( "Final char of escape sequence: %c", ptext[ idx ] );
					switch( ptext[idx] )
					{
					default:
						Log6( "Unknown escape code : (%d) [%d;%d;%d;%d%c"
										, pmdp->nParams
											, pmdp->ParamSet[0], pmdp->ParamSet[1] 
											, pmdp->ParamSet[2], pmdp->ParamSet[3] 
										, ptext[idx]
											);
						break;
					case 'h': // enable code \x1b ? # h
						if( pmdp->ParamSet[0] == 2004 ) {
							// enable bracketed paste mode
							// sends "\x1b [ 200 ~" and "\x1b [ 201 ~"  around pasted text - VIM doesnt treat as commands
							// https://en.wikipedia.org/wiki/ANSI_escape_code#CSIsection
						} else if( pmdp->ParamSet[ 0 ] == 9001 ) {
							// SetModeWin32Input

						} else if( pmdp->ParamSet[ 0 ] == 1004 ) {
							// enable reporting focus 
							// send \x1b [ I and \x1b [ O  on focus and on lose focus.
						} else if( pmdp->ParamSet[0] == 1049 ) {
							// aternate screen buffer (xterm )
						} else if( pmdp->ParamSet[0] == 25 ) {
							// show cursor
						}
						pmdp->flags.extended_cursor = 0;
						break;
					case 'l': // disable code \x1b ? # l 
						if( pmdp->ParamSet[0] == 2004 ) {
							// disable bracketed paste mode
						} else if( pmdp->ParamSet[ 0 ] == 9001 ) {
							// no idea?
						} else if( pmdp->ParamSet[ 0 ] == 1004 ) {
							// disable reporting focus
						} else if( pmdp->ParamSet[0] == 1049 ) {
							// disable alt screen buffer
						} else if( pmdp->ParamSet[0] == 25 ) {
							// hide cursor
						}
						pmdp->flags.extended_cursor = 0;
						break;
					case 'b': // repeat character n times
						if( !pmdp->nParams )
						{
							pmdp->nParams = 1;
							pmdp->ParamSet[0] = 1;
						}
						{
							int n;
							for( n = 0; n < pmdp->ParamSet[0]; n++ )
								VarTextAddCharacter( pmdp->vt, pmdp->repeat_character );
						}
						// okay - now what was the last valid character?
						// it will have already been commited at the opening escape.
						break;
					//case 'R':
					// cursor report position... suppose inbound we don't care
					//	break;
					case 'r':
						pmdp->flags.bExtended = 1;
						pmdp->attribute.flags.format_op = FORMAT_OP_SET_SCROLL_REGION;
						if( !pmdp )
						{
							pmdp->attribute.position.coords.x = -1;
							pmdp->attribute.position.coords.y = -1;
						}
						else
						{
							pmdp->attribute.position.coords.x = pmdp->ParamSet[0];
							pmdp->attribute.position.coords.y = pmdp->ParamSet[1];
						}
						break;
					case 'n': // device queries - 6n == get cursor pos.
						if( pmdp->nParams )
						{
							switch( pmdp->ParamSet[0] )
							{
							case 6:

								{
									DECLTEXTSZ( out, 32 );
									//PTEXT x, y;
									//x = GetVolatileVariable( pmdp->common.Owner->Current, "cursorx" );
									//y = GetVolatileVariable( pmdp->common.Owner->Current, "cursory" );
									out.data.size = snprintf( out.data.data, 32, "\x1b[%d;%dR"
											, pmdp->cursorY   // row
											, pmdp->cursorX );// col
									if( pmdp->writeCallback )
										pmdp->writeCallback( pmdp->psvWriteCallback, SegDuplicate( (PTEXT)&out ) );
								}
								break;
							}
						}
						break;
					case 'H': // set cursor position;
					case 'f':
						{
							if( pmdp->nParams == 2 )
							{
								pmdp->attribute.position.coords.x = pmdp->ParamSet[1] - 1;
								if( pmdp->attribute.position.coords.x < 0 )
									pmdp->attribute.position.coords.x = 0;
								pmdp->attribute.position.coords.y = pmdp->ParamSet[0] - 1;
								if( pmdp->attribute.position.coords.y < 0 )
									pmdp->attribute.position.coords.y = 0;
							}
							else if( !pmdp->nParams )
							{
								pmdp->attribute.position.coords.x = 0;
								pmdp->attribute.position.coords.y = 0;
							}
							else
								Log( "Invalid escape[*H - only one parameter?" );
							pmdp->flags.bPosition = 1;
						}
						break;
					case 'A':
						{
							if( !pmdp->nParams )
								pmdp->attribute.position.coords.y = -1;
							else
								pmdp->attribute.position.coords.y = -pmdp->ParamSet[0];
							pmdp->attribute.position.coords.x = 0;
							pmdp->flags.bPositionRel = 1;
						}
						break;
					case 'B':
						{
							if( !pmdp->nParams )
								pmdp->attribute.position.coords.y = 1;
							else
								pmdp->attribute.position.coords.y = pmdp->ParamSet[0];
							pmdp->attribute.position.coords.x = 0;
							pmdp->flags.bPositionRel = 1;
						}
						break;
					case 'C':
						{
							if( !pmdp->nParams )
								pmdp->attribute.position.coords.x = 1;
							else
								pmdp->attribute.position.coords.x = pmdp->ParamSet[0];
							pmdp->attribute.position.coords.y = 0;
							pmdp->flags.bPositionRel = 1;
						}
						break;
					case 'D':
						{
							if( !pmdp->nParams )
								pmdp->attribute.position.coords.x = -1;
							else
								pmdp->attribute.position.coords.x = -pmdp->ParamSet[0];
							pmdp->attribute.position.coords.y = 0;
							pmdp->flags.bPositionRel = 1;
						}
						break;
					case 'd': // this would be line position.
						if( pmdp->nParams )
							pmdp->attribute.position.coords.y = pmdp->ParamSet[0];
						else
							pmdp->attribute.position.coords.y = 0;
						pmdp->attribute.position.coords.x = 0xFF; // special ignore case.
						pmdp->flags.bPosition = 1;
						break;
					case 'G': // this would be column position absolute.
						if( pmdp->nParams )
							pmdp->attribute.position.coords.x = pmdp->ParamSet[0];
						else
							pmdp->attribute.position.coords.x = 0;
						pmdp->attribute.position.coords.y = 0xFF; // special ignore case.
						pmdp->flags.bPosition = 1;
						break;
					case 'J':
						pmdp->flags.bExtended = 1;
						if( !pmdp->nParams )
						{
							pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_END_OF_PAGE;
						}
						else
						{
							if( pmdp->ParamSet[0] == 1 )
								pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_START_OF_PAGE;
							if( pmdp->ParamSet[0] == 2 )
								pmdp->attribute.flags.format_op = FORMAT_OP_PAGE_BREAK;
						}
						break;
					case 'P':
						pmdp->flags.bExtended = 1;
						pmdp->attribute.flags.format_op = FORMAT_OP_DELETE_CHARS;
						if( !pmdp->nParams )
							pmdp->attribute.position.offset.spaces = 1;
						else
							pmdp->attribute.position.offset.spaces = pmdp->ParamSet[0];
						break;
					case 'K':
						pmdp->flags.bExtended = 1;
						if( !pmdp->nParams )
						{
							pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_END_OF_LINE;
						}
						else
						{
							if( pmdp->ParamSet[0] == 1 )
								pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_START_OF_LINE;
							else if( pmdp->ParamSet[0] == 2 )
								pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_LINE;							
							else
								pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_END_OF_LINE;
						}
						break;
					case 'm': // Color attribute goes here.....
						{
							int n;
							pmdp->attribute.flags.highlight = 0; // default attribute
							pmdp->attribute.flags.default_background = 0;
							pmdp->attribute.flags.default_foreground = 0;
							pmdp->attribute.flags.prior_background = 1;
							pmdp->attribute.flags.prior_foreground = 1;
							//pmdp->attribute.position.x = 0;
							//pmdp->attribute.position.y = 0;
							for( n = 0; n < pmdp->nParams; n++ )
							{
								if( pmdp->ParamSet[n] == 0 ) {
									pmdp->attribute.flags.default_background = 1; // default attribute
									pmdp->attribute.flags.default_foreground = 1; // default attribute
								   lprintf( "color attribute: default" );
								} else if( pmdp->ParamSet[n] == 1 ) {
									pmdp->attribute.flags.highlight = 1; // default attribute
									lprintf( "highlight attribute: bright" );
								} else if( pmdp->ParamSet[n] == 4 ) {
									pmdp->attribute.flags.underline = 1; // default attribute
									lprintf( "highlight attribute: underline" );
								} else if( pmdp->ParamSet[n] == 5 ) {
									pmdp->attribute.flags.blink = 1; // default attribute
									lprintf( "highlight attribute: blink" );
								} else if( pmdp->ParamSet[n] == 6 ) {
									pmdp->attribute.flags.blinkFast = 1; // default attribute
									lprintf( "highlight attribute: blinkfast" );
								} else if( pmdp->ParamSet[ n ] == 7 ) {
									pmdp->attribute.flags.reverse = 1; // default attribute
									lprintf( "highlight attribute: reverse" );
								} else if( pmdp->ParamSet[n] >= 30
								         && pmdp->ParamSet[n] <= 37 ) {
									pmdp->attribute.flags.prior_foreground = 0;
									pmdp->attribute.flags.foreground = 
										colormap[pmdp->ParamSet[n]-30];
									lprintf( "Set fore color: %d", pmdp->ParamSet[ n ] - 30 );
								} else if( pmdp->ParamSet[n] >= 90
								         && pmdp->ParamSet[n] <= 97 ) {
									pmdp->attribute.flags.prior_foreground = 0;
									pmdp->attribute.flags.foreground = 
										colormap[8+pmdp->ParamSet[n]-90];
									lprintf( "Set bright fore color: %d", pmdp->ParamSet[ n ] - 90 );
								} else if( pmdp->ParamSet[n] >= 38 ) {
									if( pmdp->ParamSet[ n+1 ] == 5 ) {
										pmdp->attribute.flags.prior_foreground = 0;
										if( pmdp->ParamSet[n+2] < 16 )
											pmdp->attribute.flags.foreground       = colormap[ pmdp->ParamSet[ n + 2 ] ];
										else if( pmdp->ParamSet[n+2] < 232 ) {
											const int r = ((pmdp->ParamSet[n + 2] - 16) / 36) % 6;
											const int g = ((pmdp->ParamSet[n + 2] - 16) / 6) % 6;
											const int b = (pmdp->ParamSet[n + 2] - 16 ) % 6;
											pmdp->attribute.flags.rgb_foreground = Color( r, g, b);
										} else {
											const int gray = 8 + (pmdp->ParamSet[n + 2] - 232) * 10;
											pmdp->attribute.flags.rgb_foreground = Color(gray, gray, gray);
										}
										lprintf( "Set a fancy foreground color here: %d", pmdp->ParamSet[ n + 2 ] );
									} 
									else if( pmdp->ParamSet[ n + 1 ] == 2 ) {
										lprintf( "Would set a RGB foreground color here: %d", pmdp->ParamSet[ n + 2 ],
									            pmdp->ParamSet[ n + 3 ], pmdp->ParamSet[ n + 4 ] );
									} else {
										lprintf( "Bad sequence \\x1b [ 38; %d ... m", pmdp->ParamSet[ n + 2 ] );
									}
								} else if( pmdp->ParamSet[ n ] >= 40
								         && pmdp->ParamSet[n] <= 47 ) {
									pmdp->attribute.flags.prior_background = 0;
									pmdp->attribute.flags.background = 
										colormap[pmdp->ParamSet[n]-40];
									lprintf( "Set back color: %d", pmdp->ParamSet[ n ] - 30 );
								} else if( pmdp->ParamSet[n] >= 100
								         && pmdp->ParamSet[n] <= 107 ) {
									pmdp->attribute.flags.prior_foreground = 0;
									pmdp->attribute.flags.foreground = 
										colormap[8+pmdp->ParamSet[n]-100];
									lprintf( "Set bright fore color: %d", pmdp->ParamSet[ n ] - 100 );
								} else if( pmdp->ParamSet[n] >= 48 ) {
									if( pmdp->ParamSet[ n+1 ] == 5 ) {
										pmdp->attribute.flags.prior_background = 0;
										if (pmdp->ParamSet[n + 2] < 16)
										pmdp->attribute.flags.background       = colormap[ pmdp->ParamSet[ n + 2 ] ];
										else if( pmdp->ParamSet[n+2] < 232 ) {
											const int r = ((pmdp->ParamSet[n + 2] - 16) / 36) % 6;
											const int g = ((pmdp->ParamSet[n + 2] - 16) / 6) % 6;
											const int b = (pmdp->ParamSet[n + 2] - 16 ) % 6;
											pmdp->attribute.flags.rgb_background = Color( r, g, b);
										} else {
											const int gray = 8 + (pmdp->ParamSet[n + 2] - 232) * 10;
											pmdp->attribute.flags.rgb_background = Color(gray, gray, gray);
										}
										lprintf( "Would set a fancy background color here: %d", pmdp->ParamSet[ n + 2 ] );
									} 
									else if( pmdp->ParamSet[ n + 1 ] == 2 ) {
										lprintf( "Would set a RGB background color here: %d %d %d", pmdp->ParamSet[ n + 2 ],
									            pmdp->ParamSet[ n + 3 ], pmdp->ParamSet[ n + 4 ] );
									} else {
										lprintf( "Bad sequence \\x1b [ 38; %d ... m", pmdp->ParamSet[ n + 2 ] );
									}
								} else {
									lprintf( "Unhandled color attribute: %d", pmdp->ParamSet[n] );
								}
							}
							pmdp->flags.bAttribute = 1;
						}
						break;
					case 'X':
						   if( pmdp->nParams == 4 ) {
							   /* Erase characters */
						   }
						   else if( pmdp->nParams == 1 ) {
							   /* Erase characters */
						   } else {
							   /* Erase characters */
						   }

						break;
					}
					// reset param/ansi code collection
					pmdp->nState = 0;
					pmdp->nParams = 0;
					pmdp->ParamSet[pmdp->nParams] = 0;
				} else if( ( ( ptext[idx] >= '0' ) && ( ptext[idx] <= '9' ) ) ) {
					//lprintf( "still cursor?");
					if( pmdp->flags.extended_cursor ) {
						pmdp->ParamSet[pmdp->nParams] *= 10 ;
						pmdp->ParamSet[pmdp->nParams] += (ptext[idx] - '0');
					} else {
						pmdp->flags.bCollectingParam = 1;
						pmdp->ParamSet[pmdp->nParams] *= 10;
						pmdp->ParamSet[pmdp->nParams] += (ptext[idx] - '0');
					}
				} else if( ptext[idx] == '?' ) {
					pmdp->flags.extended_cursor = 1;
				} else if( ptext[idx] == ';' ) {
					if( pmdp->flags.bCollectingParam )
					{
						pmdp->flags.bCollectingParam = 0;
						pmdp->nParams++;
					}	
					pmdp->ParamSet[pmdp->nParams] = 0;
				}
			} else if( pmdp->nState == 3 ) {
				// collecting system command...
				//lprintf( "check char: %d", ptext[idx] );
				if( ptext[idx] != '\x7' && pmdp->flags.OC_LongPromptCollect){
					VarTextAddCharacter( pmdp->OC, ptext[idx] );
					continue;
				}

				if( ptext[idx] == '\x7' ) {  // ^G 
					PTEXT cmd = VarTextPeek( pmdp->OC );
					if( pmdp->setTitle )
						pmdp->setTitle( pmdp->psvTitleCallback, cmd );
					//lprintf( "------- SET TITLE UPDATE -------- %p ", pmdp->console->UpdateSize);
					if( pmdp->console->UpdateSize ) {
						pmdp->console->UpdateSize( pmdp->console->psvUpdateSize
							, pmdp->console->nColumns
							, pmdp->console->nLines
							, pmdp->console->nWidth
							, pmdp->console->nHeight );
					}
/*
					lprintf( "Got end of command...%d %d %d:%s"
						, pmdp->flags.ST_escape
						, pmdp->flags.OC_LongPrompt
						, pmdp->flags.OC_LongPromptCollect
						, GetText( cmd ) );
			*/						
					pmdp->nState = 0;
					pmdp->flags.OC_LongPrompt = 0;
					pmdp->flags.OC_LongPromptCollect = 0;
				} else if( ptext[idx] >= '0' && ptext[idx] <= '9' ) {
					if( pmdp->flags.OC_get_length ){
						pmdp->OC_code_length*= 10;
						pmdp->OC_code_length += ptext[idx] - '0';
					}
					else
						pmdp->flags.OC_LongPrompt = 1;
				} else if( ptext[idx] == ';' ) {
					//lprintf( "getlength:%d", pmdp->OC_code_length );
					if( pmdp->flags.OC_get_length ){
						pmdp->flags.OC_get_length = 0;
						if( pmdp->OC_code_length == 0 ){
							// set title...
							pmdp->flags.OC_LongPrompt = 0;
							pmdp->flags.OC_LongPromptCollect = 1;
						}
					}
					if( pmdp->flags.OC_LongPrompt ) {
					}
				} else if( ptext[idx] == '\x1b' ) {
					pmdp->flags.ST_escape = 1;
				} else if( ptext[idx] == '\\' ) {
					if( pmdp->flags.ST_escape ) {
						pmdp->flags.ST_escape = 0;
						pmdp->nState = 0;
					} else {
						// still collecting command?
					}
				}
			}
		}
		pBuffer = NEXTLINE( pBuffer );
	}
	//Log1( "Finished with buffer... %p", pReturn );
	pReturn = EnqueSegments( pmdp, pReturn, FALSE );
	if( pReturn )
		EnqueLink( &pmdp->Pending, pReturn );
	if( pDelete )
		LineRelease( pDelete );
	//return (PTEXT)DequeLink( &pmdp->Pending );
}


static PTEXT CPROC FilterCursor( PMYDATAPATH pdp, PTEXT segment )
{
	if( segment && 
		 segment->flags & TF_FORMATEX )
		if( segment->format.flags.format_op == FORMAT_OP_SET_CURSOR )
		{
			pdp->savex = segment->format.position.coords.x;
			pdp->savey = segment->format.position.coords.y;
			pdp->flags.wait_for_cursor = 0; 
			LineRelease( segment );
			return NULL;
		}
	return segment;
}


void CloseAnsi( PMYDATAPATH pdp )
{
	PTEXT text;
	//pdp->common.Type = 0;
	VarTextDestroy( &pdp->vt );
	while( text = (PTEXT)DequeLink( &pdp->Pending ) )
		LineRelease( text );
	DeleteLinkQueue( &pdp->Pending );
}

//#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )


PMYDATAPATH OpenAnsi( PCONSOLE_INFO console )
{
	PMYDATAPATH pdp = New( MYDATAPATH );
	int lastin = 1;
	MemSet( pdp, 0, sizeof( pdp[0] ) );
	pdp->console = console;
	// parameters
	//	 encode, decode
	//	 inbound, outbound
	//	 else required parameters...
	//pdp = CreateDataPath( pChannel, MYDATAPATH );
	pdp->vt = VarTextCreate();
	// default options.
	pdp->flags.inbound=1;
	pdp->flags.encode_in = 0; // decode in
	pdp->flags.outbound=1;
	pdp->flags.encode_out = 1; // encode out.

	pdp->attribute.flags.prior_foreground = 1;
	pdp->attribute.flags.prior_background = 1;
	pdp->attribute.position.offset.spaces = 0;
	return pdp;
}

PTEXT GetPendingWrite( PMYDATAPATH pmdp ) {
	PTEXT send = (PTEXT)DequeLink( &pmdp->Pending );
	return send;
}

void PSI_Console_SetWriteCallback( PSI_CONTROL pc, void ( *write )( uintptr_t, PTEXT ), uintptr_t psv ) {
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	PANSI_DATAPATH ansi = console->ansi;
	ansi->psvWriteCallback = psv;
	ansi->writeCallback = write;
}

void PSI_Console_SetTitleCallback( PSI_CONTROL pc, void ( *title )( uintptr_t, PTEXT ), uintptr_t psv ) {
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	PANSI_DATAPATH ansi = console->ansi;
	ansi->psvTitleCallback = psv;
	ansi->setTitle = title;
}

PSI_CONSOLE_NAMESPACE_END
