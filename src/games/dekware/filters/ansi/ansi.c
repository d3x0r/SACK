#include <stdhdrs.h>
#include <logging.h>
#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"
//#include "termstruc.h" // mydatapath

int colormap[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static int myTypeID;

typedef struct mydatapath_tag {
   DATAPATH common;

   // ANSI burst state variables...
   int nState;
   int nParams;
   int ParamSet[12];
	TEXTCHAR repeat_character; // set at '\x1b' so that \x1b[#b can repeat it.
   FORMAT attribute; // keep this for continuous attributes across buffers
	PLINKQUEUE Pending;
	int savex, savey;
   PVARTEXT vt;

   // default is inbound, decode
   struct {
   	uint32_t outbound    : 1;
   	uint32_t inbound     : 1;
   	uint32_t encode_in   : 1;
   	uint32_t encode_out  : 1;
   	uint32_t bNewLine    : 1;
		uint32_t bPosition   : 1; // set TF_FORMATABS
		uint32_t bPositionRel: 1; // set TF_FORMATREL
		uint32_t bExtended   : 1; // set TF_FORAMTEX
		uint32_t bCollectingParam : 1;
		uint32_t bAttribute      : 1;
		uint32_t keep_newline    : 1;
		uint32_t wait_for_cursor : 1;
		uint32_t newline_only_in : 1;
		uint32_t newline_only_out : 1;
   } flags;
} MYDATAPATH, *PMYDATAPATH;


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
//         }
//         else if( p[idx] == 'C' ) // right arrow...
//         {
//         }
//         else if( p[idx] == 'D' ) // left arrow....
//         {
//         }
//         else
//         { // otherwise data will be shifted down...
//            /*
//            ClearLastCommandOutput();
//            if( pdp->nHistory != -1 )
//            {
//               pdp->nHistory = -1;
//               pdp->bRecalled = FALSE;
//            }
//            */
//            p[store++] = p[idx-2];
//            p[store++] = p[idx-1];
//            p[store++] = p[idx];
//         }
//         state = 0;
//         break;
//      }
//   }
//   pBuffer->data.size = store;
//}

static PTEXT CPROC AnsiEncode( PMYDATAPATH pmdp, PTEXT pBuffer )
{
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
      //              DBG_PASS
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
			//Log2( WIDE("Set abs position %d %d")
			//	 , pNew->format.position.coords.x
			//	 , pNew->format.position.coords.y
			//	 );
			pNew->flags |= TF_FORMATABS|TF_NORETURN;
		}
		if( pmdp->flags.bPositionRel )
		{
         //Log( WIDE("Set rel position") );
			pNew->flags |= TF_FORMATREL|TF_NORETURN;
		}
		if( pmdp->flags.bExtended )
			pNew->flags |= TF_FORMATEX|TF_NORETURN;
		//Log1( DBG_FILELINEFMT WIDE("Enquing segments %s") DBG_RELAY , GetText( pNew ) );
		pmdp->flags.bPosition = 0;
		pmdp->flags.bPositionRel = 0;
		pmdp->flags.bExtended = 0;
		pmdp->flags.bNewLine = 0;
		// new segment has attributes attached
      // cannot be part of an existing line.
		if( pNew->flags & (TF_FORMATEX|TF_FORMATREL|TF_FORMATABS) )
		{
        // Log( WIDE("making a new return... enquing return") );
			EnqueLink( &pmdp->Pending, pReturn );
			pReturn = pNew;
		}
		else // otherwise attach it to any prior input....
		{
         //Log( WIDE("appending pReturn with pNew") );
			pReturn = SegAppend( pReturn, pNew );
		}
      // must now terminate any data if was a return.
		if( bReturn )
		{
         //Log( WIDE("Enquing previous data... ") );
         EnqueLink( &pmdp->Pending, pReturn );
			pReturn = NULL;
		}
		//EnqueLink( &pmdp->Pending, pReturn );
      //Log1( WIDE("Returning %p"), pReturn );
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
      //   Log( WIDE("Set abs position") );
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATEX|TF_NORETURN;
		pmdp->flags.bExtended = 0;
	}
	if( pmdp->flags.bPositionRel )
	{
      //   Log( WIDE("Set rel position") );
		if( !pTemp )
			pTemp = SegCreate(0);
		pTemp->flags |= TF_FORMATREL|TF_NORETURN;
		pmdp->flags.bPositionRel = 0;
	}
	if( pTemp )
	{  // any attribute which was collected must be issued (all alone)
		// as there was no more input to attach it to...
		//Log1( DBG_FILELINEFMT WIDE("Enquing format only segment") DBG_RELAY , 0 );
		pTemp->format = pmdp->attribute;
      // and reset the prior attributes
		pmdp->attribute.flags.prior_background = 1;
		pmdp->attribute.flags.prior_foreground = 1;
		pmdp->attribute.position.offset.spaces = 0;
		pmdp->flags.bNewLine = 0;
		//Log1( WIDE("Enque a blank temp thing %p "), pReturn );
      EnqueLink( &pmdp->Pending, pReturn );
		EnqueLink( &pmdp->Pending, pTemp );
		return NULL;
	}
	// no new data... no attributed data...
	// must return - and already was a newline
	// force a total blank newline.
   //Log( WIDE("Test for return...") );
	if( bReturn && pmdp->flags.bNewLine )
	{  // but - we have a newline...
		PTEXT blank = SegCreate(0);
      //blank->data.data[0] = ' ';
		//blank->data.data[1] = 0;
      Log( WIDE("enquing a blank.") );
		EnqueLink( &pmdp->Pending, blank );
      // there will be a newline (pReturn) so don't bother clearing it.
		//pmdp->flags.bNewLine = 0;
	}
	else
	{
     // Log( WIDE("Enquing pReturn") );
		EnqueLink( &pmdp->Pending, pReturn );
	}
	//Log( WIDE("Returning NULL") );
	return NULL;
}


static PTEXT CPROC AnsiBurst( PMYDATAPATH pmdp, PTEXT pBuffer )
{
   // this function is expecting a single buffer segment....
   INDEX idx;
   TEXTCHAR *ptext;
   PTEXT pReturn = NULL, pDelete = pBuffer;
	if( pmdp->flags.wait_for_cursor )
	   return (PTEXT)DequeLink( &pmdp->Pending );
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
               lprintf( WIDE("Ignoring nulls...") );
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
						//Log( WIDE("Newline character - flush exisiting") );
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
                  // pmdp->flags.bNewLine = TRUE;
               }
               break;
            case 27:
					if( !pmdp->nState )
					{
                  //Log( WIDE("Escape - gather prior to attach attributes to next.") );
						pReturn = EnqueSegments( pmdp, pReturn, FALSE );
               	pmdp->nState = 1;
               }
               else
               {
	            	VarTextAddCharacter( pmdp->vt, ptext[idx] );
                  Log( WIDE("Incomplete sequence detected...\n") );
               }
               break;
            }
      	}
      	else if( pmdp->nState == 1 )
      	{
            if( ptext[idx] == '[' )
            {
					if( pmdp->nParams )
						Log( WIDE("Invalid states... nparams beginning sequence.") );
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
					//												, WIDE("cursorx") );
					//pmdp->savey = GetVolatileVariable( pmdp->common.Owner->Current
					//												, WIDE("cursory") );

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
						Log6( WIDE("Unknown escape code : (%d) [%d;%d;%d;%d%c")
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
									PTEXT x, y;
									x = GetVolatileVariable( pmdp->common.Owner->Current
																	, WIDE("cursorx") );
									y = GetVolatileVariable( pmdp->common.Owner->Current
																	, WIDE("cursory") );
									out.data.size = snprintf( out.data.data, 32, WIDE("\x1b[%s;%sR")
											, x?GetText( y ):WIDE("0")
											, y?GetText( y ):WIDE("0") );
									EnqueLink( &pmdp->common.Output, SegDuplicate( (PTEXT)&out ) );
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
								Log( WIDE("Invalid escape[*H - only one parameter?") );
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
   //Log1( WIDE("Finished with buffer... %p"), pReturn );
	pReturn = EnqueSegments( pmdp, pReturn, FALSE );
   if( pReturn )
      EnqueLink( &pmdp->Pending, pReturn );
	if( pDelete )
	   LineRelease( pDelete );
   return (PTEXT)DequeLink( &pmdp->Pending );
}

static int CPROC Read( PMYDATAPATH pdp )
{
	if( pdp->flags.inbound )
		if( pdp->flags.encode_in )
			return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))AnsiEncode );
		else
			return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))AnsiBurst );
	return RelayInput( (PDATAPATH)pdp, NULL );
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

static int CPROC Write( PMYDATAPATH pdp )
{
	if( pdp->flags.outbound )
		if( pdp->flags.encode_out )
			return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))AnsiEncode );
		else
			return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))AnsiBurst );
	else
		return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))FilterCursor );
}

static int CPROC Close( PMYDATAPATH pdp )
{
	PTEXT text;
   pdp->common.Type = 0;
	VarTextDestroy( &pdp->vt );
	while( text = (PTEXT)DequeLink( &pdp->Pending ) )
		LineRelease( text );
	DeleteLinkQueue( &pdp->Pending );
   return 0;
}

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )


static PDATAPATH CPROC OpenAnsi( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = NULL;
	PTEXT option;
	int lastin = 1;
	// parameters
	//    encode, decode
	//    inbound, outbound
	//    else required parameters...
	pdp = CreateDataPath( pChannel, MYDATAPATH );
	pdp->vt = VarTextCreate();
	// default options.
	pdp->flags.inbound=1;
	pdp->flags.encode_in = 0; // decode in
	pdp->flags.outbound=1;
	pdp->flags.encode_out = 1; // encode out.

	while( ( option = GetParam( ps, &parameters ) ) )
	{
		if( OptionLike( option, WIDE("inbound") ) )
		{
			pdp->flags.inbound = 1;
			lastin = 1;
		}
		else if( OptionLike( option, WIDE("outbound") ) )
		{
			pdp->flags.outbound = 1;
			lastin = 0;
		}
		else if( OptionLike( option, WIDE("newline") ) )
		{
			if( lastin )
				pdp->flags.newline_only_in = 1;
			else
				pdp->flags.newline_only_out = 1;
		}
		else if( OptionLike( option, WIDE("encode") ) )
		{
			// if not encode - is decode.
			if( lastin )
				pdp->flags.encode_in = 1;
			else
				pdp->flags.encode_out = 1;
		}
		else if( OptionLike( option, WIDE("__keepnewline") ) )
		{
         pdp->flags.keep_newline = 1;
		}
      else
		{
			DECLTEXT( msg1, WIDE("Ansi filter options include: Inbound/Outbound, Encode/Decode") );
			DECLTEXT( msg2, WIDE("Least character match is allowed (I/O) (E/D)") );
			EnqueLink( &ps->Command->Output, &msg1 );
			EnqueLink( &ps->Command->Output, &msg2 );
			DestroyDataPath( (PDATAPATH)pdp );
			return NULL;
		}				
	}
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
	pdp->attribute.flags.prior_foreground = 1;
	pdp->attribute.flags.prior_background = 1;
	pdp->attribute.position.offset.spaces = 0;
   return (PDATAPATH)pdp;
}


PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("ansi"), WIDE("Filter for handling ansi color code streams..."), OpenAnsi );
   return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("ansi") );
}
// $Log: ansi.c,v $
// Revision 1.33  2005/02/21 12:08:26  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.32  2005/01/28 16:02:10  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.31  2005/01/17 09:01:16  d3x0r
// checkpoint ...
//
// Revision 1.30  2005/01/17 08:24:25  d3x0r
// Make openansi static
//
// Revision 1.29  2004/09/29 09:31:17  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.28  2004/09/23 09:53:26  d3x0r
// Looks like it's in a usable state for windows... perhaps an alpha/beta to be released soon.
//
// Revision 1.27  2004/09/20 10:10:19  d3x0r
// Updates to reflect chagnes in sack to change default/prior attribute flags
//
// Revision 1.26  2004/05/04 07:30:26  d3x0r
// Checkpoint for everything else.
//
// Revision 1.25  2004/04/07 23:38:41  d3x0r
// Checkpoint...
//
// Revision 1.24  2004/01/20 07:43:48  d3x0r
// generate -16384 as exception coordinate
//
// Revision 1.23  2003/11/08 00:09:40  panther
// fixes for VarText abstraction
//
// Revision 1.22  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.21  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.20  2003/04/13 22:28:51  panther
// Seperate computation of lines and rendering... mark/copy works (wincon)
//
// Revision 1.19  2003/04/12 20:51:18  panther
// Updates for new window handling module - macro usage easer...
//
// Revision 1.18  2003/04/08 06:46:39  panther
// update dekware - fix a bit of ansi handling
//
// Revision 1.17  2003/04/06 23:26:13  panther
// Update to new SegSplit.  Handle new formatting option (delete chars) Issue current window size to launched pty program
//
// Revision 1.16  2003/04/06 10:00:43  panther
// Encode \r, \n, \b into cursor data.
//
// Revision 1.15  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.14  2003/04/01 12:30:36  panther
// Build extended text segments which support special formattings
//
// Revision 1.13  2003/03/25 08:59:01  panther
// Added CVS logging
//
