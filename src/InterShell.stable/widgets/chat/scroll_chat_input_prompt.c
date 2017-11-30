
#ifndef FORCE_NO_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#endif
#define CHAT_CONTROL_SOURCE


#include <stdhdrs.h>
#include <image.h>
#include "chat_control.h"

#include "chat_control_internal.h" 


static CDATA crColorTableText[16];
static CDATA crColorTableBack[16];
#define text_alpha 255
#define back_alpha 255

static void FillDefaultColors( void )
{
	if( !crColorTableText[0] )
	{
		crColorTableText[0] = AColor( 0,0,1, text_alpha );
		crColorTableText[1] = AColor( 0, 0, 128, text_alpha );
		crColorTableText[2] = AColor( 0, 128, 0, text_alpha );
		crColorTableText[3] = AColor( 0, 128, 128, text_alpha );
		crColorTableText[4] = AColor( 192, 32, 32, text_alpha );
		crColorTableText[5] = AColor( 140, 0, 140, text_alpha );
		crColorTableText[6] = AColor( 160, 160, 0, text_alpha );
		crColorTableText[7] = AColor( 192, 192, 192, text_alpha );
		crColorTableText[8] = AColor( 128, 128, 128, text_alpha );
		crColorTableText[9] = AColor( 0, 0, 255, text_alpha );
		crColorTableText[10] = AColor( 0, 255, 0, text_alpha );
		crColorTableText[11] = AColor( 0, 255, 255, text_alpha );
		crColorTableText[12] = AColor( 255, 0, 0, text_alpha );
		crColorTableText[13] = AColor( 255, 0, 255, text_alpha );
		crColorTableText[14] = AColor( 255, 255, 0, text_alpha );
		crColorTableText[15] = AColor( 255, 255, 255, text_alpha );

		crColorTableBack[0] = AColor( 0,0,1, back_alpha );
		crColorTableBack[1] = AColor( 0, 0, 128, back_alpha );
		crColorTableBack[2] = AColor( 0, 128, 0, back_alpha );
		crColorTableBack[3] = AColor( 0, 128, 128, back_alpha );
		crColorTableBack[4] = AColor( 192, 32, 32, back_alpha );
		crColorTableBack[5] = AColor( 140, 0, 140, back_alpha );
		crColorTableBack[6] = AColor( 160, 160, 0, back_alpha );
		crColorTableBack[7] = AColor( 192, 192, 192, back_alpha );
		crColorTableBack[8] = AColor( 128, 128, 128, back_alpha );
		crColorTableBack[9] = AColor( 0, 0, 255, back_alpha );
		crColorTableBack[10] = AColor( 0, 255, 0, back_alpha );
		crColorTableBack[11] = AColor( 0, 255, 255, back_alpha );
		crColorTableBack[12] = AColor( 255, 0, 0, back_alpha ) ;
		crColorTableBack[13] = AColor( 255, 0, 255, back_alpha );
		crColorTableBack[14] = AColor( 255, 255, 0, back_alpha );
		crColorTableBack[15] = AColor( 255, 255, 255, back_alpha );
	}
}


//----------------------------------------------------------------------------------------
//------------------ This bit copied from psi_console ------------------------------------

static void CPROC SetCurrentColor( PCHAT_LIST list, enum current_color_type type, PTEXT segment )
{
	switch( type )
	{
	case COLOR_COMMAND:
		( list->colors.crText = list->colors.crCommand );
		( list->colors.crBack = list->colors.crCommandBackground );
		break;
	case COLOR_MARK:
		( list->colors.crText = list->colors.crMark );
		( list->colors.crBack = list->colors.crMarkBackground );
		break;
	case COLOR_DEFAULT:
		( list->colors.crText = crColorTableText[15] );
		( list->colors.crBack = list->colors.crBackground );
		break;
	case COLOR_SEGMENT:
		if( segment )
		{
			( list->colors.crText = crColorTableText[segment->format.flags.foreground] );
			( list->colors.crBack = crColorTableBack[segment->format.flags.background] );
		}
		break;
	}
	//lprintf( WIDE( "Set Color :%p %d #%08lX #%08lX" ), console, type
	//		 , list.crText, list.crBack );
}


static uint32_t CPROC DrawString( Image window, SFTFont font, int x, int y, CDATA crText, CDATA crBack, RECT *r, CTEXTSTR s, int nShown, int nShow )
{
	uint32_t width;
	//lprintf( WIDE( "Adding string out : %p %s start:%d len:%d at %d,%d #%08lX #%08lX" ), console, s, nShown, nShow,x,y,r->left,r->top
	//		 , console->psicon.crText, console->psicon.crBack );
	if( 1 /*debug*/)
	{
		uint32_t w, h;
		width = GetStringSizeFontEx( s + nShown, nShow, &w, &h, font );
		r->right = r->left + w;
		r->bottom = r->top + h;
		//lprintf( WIDE("Output string (%d-%d)  (%d-%d) %*.*s"), (*r).left, (*r).right, (*r).top, (*r).bottom, nShow, nShow, s + nShown );
	}
	PutStringFontEx( window, x, y
						, crText, crBack
						, s + nShown
						, nShow
						, font );
	return width;
}

void RenderTextLine( 
	PCHAT_LIST list
	, Image window
	, PDISPLAYED_LINE pCurrentLine
	, RECT *r
	, int nLine
	, SFTFont font
	, int nFirstLine
	, int nMinLine
	, int left_pad
	, int mark_start
	, int mark_end
						)
{
	// left and right are relative... to the line segment only...
	// for the reason of color changes inbetween segments...
	{
		int justify = 0;
		int x, y;
		int nChar;
		PTEXT pText;
		int nShow, nShown;
		int nTotalShown = pCurrentLine->nLineStart;
#ifdef DEBUG_HISTORY_RENDER
		lprintf( WIDE("Get display line %d"), nLine );
#endif
		if( !pCurrentLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("No such line... %d"), nLine );
#endif
			return;
		}
		(*r).top = nFirstLine - list->nFontHeight * nLine;
		(*r).bottom = (*r).top + list->nFontHeight;
		if( (*r).bottom <= nMinLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("bottom < minline..") );
#endif
			return;
		}
		y = (*r).top;

		(*r).left = left_pad;
		x = (*r).right = left_pad;//l.side_pad;
		//if( list->FillConsoleRect )
		//	list->FillConsoleRect(pdp, r, FILL_DISPLAY_BACK );

		//(*r).left = x;
		nChar = 0;
		pText = pCurrentLine->start;
		if( pText && ( pText->flags & TF_FORMATEX ) )
		{
			if( pText->format.flags.format_op == FORMAT_OP_JUSTIFY_RIGHT )
			{
				justify = 1;
			}
		}

		nShown = pCurrentLine->nFirstSegOfs;
#ifdef DEBUG_HISTORY_RENDER
		if( !pText )
			lprintf( WIDE("Okay no text to show... end up filling line blank.") );
#endif
		while( pText && SUS_LT( nChar, int, pCurrentLine->nToShow, INDEX ) )
		{
			size_t nLen;
			TEXTCHAR *text = GetText( pText );
#ifdef __DEKWARE_PLUGIN__
			if( !list->flags.bDirect && ( pText->flags & TF_PROMPT ) )
			{
				lprintf( WIDE("Segment is promtp - and we need to skip it.") );
				pText = NEXTLINE( pText );
				continue;
			}
#endif

			nLen = GetTextSize( pText );
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("start: %d  len: %d"), nShown, nLen );
#endif
			while( SUS_LT( nShown, int, nLen, size_t ) )
			{
#ifdef DEBUG_HISTORY_RENDER
				lprintf( WIDE("nShown < nLen... char %d len %d toshow %d"), nChar, nLen, pCurrentLine->nToShow );
#endif
				if( ( nChar + ( nLen - nShown ) ) > pCurrentLine->nToShow )
					nShow = pCurrentLine->nToShow - nChar;
				else
				{
#ifdef DEBUG_HISTORY_RENDER
					lprintf( WIDE("nShow is what's left of now to nLen from nShown... %d,%d"), nLen, nShown );
#endif
					nShow = nLen - nShown;
				}
				if( !nShow )
				{
					//lprintf( WIDE("nothing to show...") );
					break;
				}

				//lprintf( WIDE("Some stats %d %d %d"), nChar, nShow, nShown );
				if( nChar )
				{
					// not first character on line...
					x = (*r).left = (*r).right;
				}
				else
				{
					(*r).left = (*r).right;
					x = left_pad;
				}
				if( !(*r).right )
				{
					if( justify == 1 )
					{
						x = (*r).right = ( window->width - ( left_pad + pCurrentLine->nPixelEnd ) ) + (*r).left;
					}
					else if ( justify == 2 ) 
					{
						x = (*r).right = ( ( window->width - pCurrentLine->nPixelEnd ) / 2  ) + (*r).left;
					}
					else
						(*r).right = (*r).left + left_pad;
					// we're going to do text output transparent
					//if( list->FillConsoleRect )
					//	list->FillConsoleRect(pdp, r, FILL_DISPLAY_BACK );
				}
				(*r).right = l.side_pad + pCurrentLine->nPixelEnd;
				if( (*r).bottom > nMinLine )
				{
					(*r).left = x;
					//lprintf( WIDE("putting string %s at %d,%d (left-right) %d,%d"), text, x, y, (*r).left, (*r).right );
					if( nTotalShown < mark_start )
					{
						if( nTotalShown + nShow > mark_start )
						{
							nShow = mark_start - ( nTotalShown );
							DrawString( window, font, x, y
							          , list->colors.crText
									  , 0
									  , r, text, nShown, nShow );
						}
						else
							DrawString( window, font, x, y, list->colors.crText, 0, r, text, nShown, nShow );

					}
					else if( nTotalShown < mark_end )
					{
						if( nTotalShown + nShow > mark_end )
						{
							nShow = mark_end - ( nTotalShown );
							DrawString( window, font, x, y
									, list->colors.crMark
									, list->colors.crMarkBackground
									, r, text, nShown, nShow );
						}
						else
							DrawString( window, font, x, y
									, list->colors.crMark
									, list->colors.crMarkBackground
									, r, text, nShown, nShow );

					}
					else
						DrawString( window, font, x, y, list->colors.crText, 0, r, text, nShown, nShow );

					//DrawString( text );
				}
				// fill to the end of the line...
				//nLen -= nShow;
				nTotalShown += nShow;
				nShown += nShow;
				nChar += nShow;
			}
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("nShown >= nLen...") );
#endif

			nShown = 0;
			pText = NEXTLINE( pText );
		}
	}
	//lprintf( WIDE(WIDE("(*(*r)...bottom nMin %d %d")), (*r)..bottom, nMinLine );
	if( (*r).bottom > nMinLine )
	{
		(*r).bottom = (*r).top;
		(*r).top = nMinLine;
		(*r).left = 0;
		(*r).right = window->width;
#ifndef PSI_LIB
		//lprintf( WIDE("Would be blanking the screen here, but no, there's no reason to.") );
				 //FillEmptyScreen();
#endif
	}
}


//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

