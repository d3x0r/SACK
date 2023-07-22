#include <stdhdrs.h>


#include "ansi.h"

int colormap[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static int myTypeID;

struct mydatapath_tag {
	//DATAPATH common;

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
		//				  DBG_PASS
						 ) // special handling at newline...
//#define EnqueSegments( pvt, pmdp, pRet, bRet ) EnqueSegments( pvt, pmdp, pRet, bRet DBG_SRC )
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
			pNew->format = pmdp->attribute;
			pmdp->attribute.flags.prior_background = 1;
			pmdp->attribute.flags.prior_foreground = 1;
			pmdp->attribute.position.offset.spaces = 0;
			pmdp->flags.bAttribute = 0;
		}
		if( !pmdp->flags.bNewLine )
		{
			pNew->flags |= TF_NORETURN;
		}
		if( pmdp->flags.bPosition )
		{
			//Log2( "Set abs position %d %d"
			//	 , pNew->format.position.coords.x
			//	 , pNew->format.position.coords.y
			//	 );
			pNew->flags |= TF_FORMATABS|TF_NORETURN;
		}
		if( pmdp->flags.bPositionRel )
		{
			//Log( "Set rel position" );
			pNew->flags |= TF_FORMATREL|TF_NORETURN;
		}
		if( pmdp->flags.bExtended )
			pNew->flags |= TF_FORMATEX|TF_NORETURN;
		//Log1( DBG_FILELINEFMT "Enquing segments %s" DBG_RELAY , GetText( pNew ) );
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
		//Log1( "Returning %p", pReturn );
		return pReturn;
	}
	// didn't have any data collected... see if there
	// was perhaps a collected attribute in any way.

	pTemp = NULL;
	
	// have to make a NULL segment...
	if( pmdp->flags.bPosition )
	{
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATABS|TF_NORETURN;
		pmdp->flags.bPosition = 0;
	}
	if( pmdp->flags.bExtended )
	{
		//	Log( "Set abs position" );
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATEX|TF_NORETURN;
		pmdp->flags.bExtended = 0;
	}
	if( pmdp->flags.bPositionRel )
	{
		//	Log( "Set rel position" );
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATREL|TF_NORETURN;
		pmdp->flags.bPositionRel = 0;
	}
	if( pTemp )
	{  // any attribute which was collected must be issued (all alone)
		// as there was no more input to attach it to...
		//Log1( DBG_FILELINEFMT "Enquing format only segment" DBG_RELAY , 0 );
		pTemp->format = pmdp->attribute;
		// and reset the prior attributes
		pmdp->attribute.flags.prior_background = 1;
		pmdp->attribute.flags.prior_foreground = 1;
		pmdp->attribute.position.offset.spaces = 0;
		pmdp->flags.bNewLine = 0;
		//Log1( "Enque a blank temp thing %p ", pReturn );
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
		Log( "enquing a blank." );
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
	if( pmdp->flags.wait_for_cursor )
		return;// (PTEXT)DequeLink( &pmdp->Pending );
	pmdp->flags.bNewLine = !( pBuffer->flags & TF_NORETURN );
	// also may be called with no input.
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
					if( !pmdp->nState )
					{
						//Log( "Escape - gather prior to attach attributes to next." );
						pReturn = EnqueSegments( pmdp, pReturn, FALSE );
						pmdp->nState = 1;
					}
					else
					{
						VarTextAddCharacter( pmdp->vt, ptext[idx] );
						Log( "Incomplete sequence detected...\n" );
					}
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
				}
				else if( ptext[idx] == '7' )
				{
					// save cursor and attributes(next step)

				} 
				if( (ptext[idx] == '7') || 
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
							if( pmdp->ParamSet[0] == 2 )
								pmdp->attribute.flags.format_op = FORMAT_OP_CLEAR_LINE;							

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
								if( pmdp->ParamSet[n] == 0 )
								{
									pmdp->attribute.flags.default_background = 1; // default attribute
									pmdp->attribute.flags.default_foreground = 1; // default attribute
								}
								if( pmdp->ParamSet[n] == 1 )
								{
									pmdp->attribute.flags.highlight = 1; // default attribute
								}

								if( pmdp->ParamSet[n] >= 30 &&
									 pmdp->ParamSet[n] <= 37 )
								{
									pmdp->attribute.flags.prior_foreground = 0;
									pmdp->attribute.flags.foreground = 
										colormap[pmdp->ParamSet[n]-30];
								}
								if( pmdp->ParamSet[n] >= 40 &&
									 pmdp->ParamSet[n] <= 47 )
								{
									pmdp->attribute.flags.prior_background = 0;
									pmdp->attribute.flags.background = 
										colormap[pmdp->ParamSet[n]-40];
								}
							}
							pmdp->flags.bAttribute = 1;
						}
						break;
					}
					// reset param/ansi code collection
					pmdp->nState = 0;
					pmdp->nParams = 0;
					pmdp->ParamSet[pmdp->nParams] = 0;
				}
				else if( ( ( ptext[idx] >= '0' ) && ( ptext[idx] <= '9' ) ) )
				{
					pmdp->flags.bCollectingParam = 1;
					pmdp->ParamSet[pmdp->nParams] *= 10;
					pmdp->ParamSet[pmdp->nParams] += (ptext[idx] - '0');
				}
				else if( ptext[idx] == ';' )
				{
					if( pmdp->flags.bCollectingParam )
					{
						pmdp->flags.bCollectingParam = 0;
						pmdp->nParams++;
					}	
					pmdp->ParamSet[pmdp->nParams] = 0;
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


PMYDATAPATH OpenAnsi( void )
{
	PMYDATAPATH pdp = New( MYDATAPATH );
	int lastin = 1;
	MemSet( pdp, 0, sizeof( pdp[0] ) );
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

/*
	while( ( option = GetParam( ps, &parameters ) ) )
	{
		if( OptionLike( option, "inbound" ) )
		{
			pdp->flags.inbound = 1;
			lastin = 1;
		}
		else if( OptionLike( option, "outbound" ) )
		{
			pdp->flags.outbound = 1;
			lastin = 0;
		}
		else if( OptionLike( option, "newline" ) )
		{
			if( lastin )
				pdp->flags.newline_only_in = 1;
			else
				pdp->flags.newline_only_out = 1;
		}
		else if( OptionLike( option, "encode" ) )
		{
			// if not encode - is decode.
			if( lastin )
				pdp->flags.encode_in = 1;
			else
				pdp->flags.encode_out = 1;
		}
		else if( OptionLike( option, "__keepnewline" ) )
		{
			pdp->flags.keep_newline = 1;
		}
		else
		{
			DECLTEXT( msg1, "Ansi filter options include: Inbound/Outbound, Encode/Decode" );
			DECLTEXT( msg2, "Least character match is allowed (I/O) (E/D)" );
			EnqueLink( &ps->Command->Output, &msg1 );
			EnqueLink( &ps->Command->Output, &msg2 );
			DestroyDataPath( (PDATAPATH)pdp );
			return NULL;
		}				
	}
*/
	//pdp->common.Type = myTypeID;
	//pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
	//pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
	//pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
	pdp->attribute.flags.prior_foreground = 1;
	pdp->attribute.flags.prior_background = 1;
	pdp->attribute.position.offset.spaces = 0;
	return pdp;
}

PTEXT GetPendingWrite( PMYDATAPATH pmdp ) {
	PTEXT send = (PTEXT)DequeLink( &pmdp->Pending );
	return send;
}

void SetWriteCallback( PMYDATAPATH pmdp, void ( *write )( uintptr_t, PTEXT ), uintptr_t psv ) {
	pmdp->psvWriteCallback = psv;
	pmdp->writeCallback = write;
}
