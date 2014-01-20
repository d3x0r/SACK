//#define DEBUG_BORDER_FLAGS

/* this flag is defined in controlstruc.h...
 *
 * #define DEBUG_BORDER_DRAWING
 * #define QUICK_DEBUG_BORDER_FLAGS // simple set - tracks who sets border where
 *
 */

#define USE_INTERFACE_MANAGER

#include "controlstruc.h"
#include <psi.h>

PSI_NAMESPACE
//---------------------------------------------------------------------------

void DrawNormalFrameInset( PSI_CONTROL pc, Image window, int bInvert, int align )
{
}

//---------------------------------------------------------------------------

void CPROC DrawFancyFrame( PCOMMON pc )
{
	Image window = pc->Window;
//#undef ALPHA_TRANSPARENT
//#define ALPHA_TRANSPARENT 1
	if( pc->flags.bInitial || pc->flags.bHidden )
	{
#ifdef DEBUG_BORDER_DRAWING
		lprintf( "Hidden or initial, aborting fancy frame..." );
#endif
		return;
	}
#ifdef DEBUG_BORDER_DRAWING
	lprintf( "Drawing fancy border... no add of update region here..." );
#endif
	BlotScaledImageSizedToAlpha( window, g.BorderSegment[SEGMENT_TOP]
								, pc->surface_rect.x, 0
								, pc->surface_rect.width, g.BorderHeight
								, ALPHA_TRANSPARENT );
	BlotScaledImageSizedToAlpha( window, g.BorderSegment[SEGMENT_BOTTOM]
								, pc->surface_rect.x, pc->surface_rect.y + pc->surface_rect.height
								, pc->surface_rect.width, g.BorderHeight
								, ALPHA_TRANSPARENT );
	BlotScaledImageSizedToAlpha( window, g.BorderSegment[SEGMENT_LEFT]
										, 0, g.BorderHeight
										, g.BorderWidth
										, window->height - ( g.BorderHeight * 2 )
										, ALPHA_TRANSPARENT );
	BlotScaledImageSizedToAlpha( window, g.BorderSegment[SEGMENT_RIGHT]
										, pc->surface_rect.x + pc->surface_rect.width, g.BorderHeight
										, g.BorderWidth, window->height - ( g.BorderHeight * 2 )
										, ALPHA_TRANSPARENT );
	BlotImageAlpha( window, g.BorderSegment[SEGMENT_TOP_LEFT]
					  , 0, 0
					  , ALPHA_TRANSPARENT );
	BlotImageAlpha( window, g.BorderSegment[SEGMENT_TOP_RIGHT]
					  , g.BorderWidth + pc->surface_rect.width, 0
					  , ALPHA_TRANSPARENT );
	BlotImageAlpha( window, g.BorderSegment[SEGMENT_BOTTOM_LEFT]
					  , 0, window->height - g.BorderHeight
					  , ALPHA_TRANSPARENT );
	BlotImageAlpha( window, g.BorderSegment[SEGMENT_BOTTOM_RIGHT]
					  , g.BorderWidth + pc->surface_rect.width, window->height - g.BorderHeight
					  , ALPHA_TRANSPARENT );
}

//---------------------------------------------------------------------------

PSI_PROC( int, FrameBorderXOfs )( PSI_CONTROL pc, _32 BorderType )
{
   //lprintf( WIDE("Result border offset for %08x"), BorderType );
    switch( BorderType & BORDER_TYPE )
    {
    case BORDER_NONE:
      return 0;
    case BORDER_NORMAL:
		 if( pc && pc->DrawBorder == DrawFancyFrame )
		 {
          return g.BorderWidth;
		 }
		 if( BorderType & BORDER_RESIZABLE )
		 {
			 return 8;
		 }
		 else
			 return g.BorderImage?g.BorderHeight:4;
	 case BORDER_THINNER:
		 return 2;
	 case BORDER_THIN:
		 return 1;
	 case BORDER_THICK_DENT:
		 return 8;
	 case BORDER_DENT:
		 return 3;
		 break;
	 case BORDER_THIN_DENT:
		 return 2;
	 }
	 // should actually compute this from facts known about pf
	 return 8;
}

//---------------------------------------------------------------------------

PSI_PROC( int, FrameBorderX )( PSI_CONTROL pc, _32 BorderType )
{
   //lprintf( WIDE("Result total for %08x"), BorderType );
    switch( BorderType & BORDER_TYPE )
    {
    case BORDER_NONE:
      return 0;
    case BORDER_NORMAL:
		 if( pc && pc->DrawBorder == DrawFancyFrame )
		 {
          return g.BorderWidth*2;
		 }
        if( BorderType & BORDER_RESIZABLE )
            return 16;
        else
            return g.BorderImage?g.BorderWidth * 2:8;
    case BORDER_THINNER:
      return 4;
    case BORDER_THIN:
      return 2;
    case BORDER_THICK_DENT:
      return 16;
    case BORDER_DENT:
      return 6;
        break;
    case BORDER_THIN_DENT:
        return 2;
    }
    // should actually compute this from facts known about pf
    return 8;
}

//---------------------------------------------------------------------------

PSI_PROC( int, CaptionHeight )( PCOMMON pf, CTEXTSTR text )
{
    // should resemble something like text height + 3 (top) + 3 (1 bottom, 2 frameline)
    // pf itself may have a different font - which
   // will then switch the height...
   if( text || ( pf &&
                pf->caption.text &&
					 !(pf->BorderType & (BORDER_NOCAPTION|BORDER_WITHIN)) ) )
	{
		GetStringSizeFontEx( WIDE(" "), 1, NULL, NULL, GetCommonFont( pf ) );
		return GetFontHeight( GetCommonFont(pf) ) + 10; // 5 above 5 below?
		//return 17; // should probably be more specific...
	}
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( int, FrameBorderYOfs )( PCOMMON pc, _32 BorderType, CTEXTSTR caption )
{
	int result = 0;
	if( !(BorderType & BORDER_NOCAPTION ) && caption )
		result += CaptionHeight( pc, caption );

	//if( !pf )
	//  return CaptionHeight( caption, NULL ) + 4;
	switch( BorderType & BORDER_TYPE )
	{
	case BORDER_NONE:
		return 0;
	case BORDER_NORMAL:
		if( pc && pc->DrawBorder == DrawFancyFrame )
		{
			return result + g.BorderHeight;
		}
		if( BorderType & BORDER_RESIZABLE )
			return result + 8;
		else
			return result + ( ( (g.BorderImage?g.BorderHeight:4)* 3 ) / 4 );
	case BORDER_THINNER:
		return result + 2;
	case BORDER_THIN:
		return result + 1;
	case BORDER_THICK_DENT:
		return result + 8;
	case BORDER_DENT:
		return result + 3;
	case BORDER_THIN_DENT:
		return result + 2;
	}
	// should actually compute this from facts known about pf
	return result + 2;
}

//---------------------------------------------------------------------------

PSI_PROC( int, FrameBorderY )( PCOMMON pc, _32 BorderType, CTEXTSTR caption )
{
	int result = 0;
	if( !(BorderType & BORDER_NOCAPTION ) && caption )
		result += CaptionHeight( pc, caption );
	//if( !pf )
	//  return CaptionHeight( pf, NULL ) + 8;
	switch( BorderType & BORDER_TYPE )
	{
	case BORDER_NONE:
		return 0;
	case BORDER_NORMAL:
		if( pc && pc->DrawBorder == DrawFancyFrame )
		{
			return result + g.BorderHeight*2;
		}
		if( BorderType & BORDER_RESIZABLE )
		{
			return result + 16;
		}
		else
		{
			return result + ((g.BorderImage?g.BorderHeight:4) * 7 / 4 );
		}
	case BORDER_THINNER:
		return result + 4;
	case BORDER_THIN:
		return result + 2;
	case BORDER_THICK_DENT:
		return result + 16;
	case BORDER_DENT:
		return result + 6;
	case BORDER_THIN_DENT:
		return result + 2;
	}
	// should actually compute this from facts known about pf
	return result + 4;
}

//---------------------------------------------------------------------------

void CPROC DrawThickFrame( PCOMMON pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	int n, ofs;
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	for( ofs = 0, n = 0; n < 2; n++,ofs++ )
	{
		do_hline( window, ofs, ofs, width-(1+ofs), basecolor(pc)[NORMAL] );
		do_vline( window, ofs, ofs, height-(1+ofs), basecolor(pc)[NORMAL] );
		do_vline( window, width - (1+ofs), ofs, height-(1+ofs), basecolor(pc)[SHADOW] );
		do_hline( window, height- (1+ofs), ofs, width-(1+ofs), basecolor(pc)[SHADOW]);
	}
	for( n = 0; n < 2; n++,ofs++ )
	{
		do_hline( window, ofs, ofs, width-(1+ofs), basecolor(pc)[HIGHLIGHT] );
		do_vline( window, ofs, ofs, height-(1+ofs), basecolor(pc)[HIGHLIGHT] );
		do_vline( window, width - (1+ofs), ofs, height-(1+ofs), basecolor(pc)[SHADE] );
		do_hline( window, height- (1+ofs), ofs, width-(1+ofs), basecolor(pc)[SHADE]);
	}
	for( n = 0; n < 4; n++,ofs++ )
	{
		do_hline( window, ofs, ofs, width-(1+ofs), basecolor(pc)[NORMAL] );
		do_vline( window, ofs, ofs, height-(1+ofs), basecolor(pc)[NORMAL] );
		do_vline( window, width - (1+ofs), ofs, height-(1+ofs), basecolor(pc)[NORMAL] );
		do_hline( window, height- (1+ofs), ofs, width-(1+ofs), basecolor(pc)[NORMAL]);
	}
}
void CPROC DrawThickFrameInverted( PCOMMON pc )
{
   Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	int n, ofs;
   if( pc->flags.bInitial || pc->flags.bHidden ) return;
	for( ofs = 0, n = 0; n < 2; n++,ofs++ )
	{
		do_hline( window, ofs, ofs, width-(1+ofs), basecolor(pc)[SHADOW] );
		do_vline( window, ofs, ofs, height-(1+ofs), basecolor(pc)[SHADOW] );
		do_vline( window, width - (1+ofs), ofs, height-(1+ofs), basecolor(pc)[NORMAL] );
		do_hline( window, height- (1+ofs), ofs, width-(1+ofs), basecolor(pc)[NORMAL]);
	}
	for( n = 0; n < 2; n++,ofs++ )
	{
		do_hline( window, ofs, ofs, width-(1+ofs), basecolor(pc)[SHADE] );
		do_vline( window, ofs, ofs, height-(1+ofs), basecolor(pc)[SHADE] );
		do_vline( window, width - (1+ofs), ofs, height-(1+ofs), basecolor(pc)[HIGHLIGHT] );
		do_hline( window, height- (1+ofs), ofs, width-(1+ofs), basecolor(pc)[HIGHLIGHT]);
	}
	for( n = 0; n < 4; n++,ofs++ )
	{
		do_hline( window, ofs, ofs, width-(1+ofs), basecolor(pc)[NORMAL] );
		do_vline( window, ofs, ofs, height-(1+ofs), basecolor(pc)[NORMAL] );
		do_vline( window, width - (1+ofs), ofs, height-(1+ofs), basecolor(pc)[NORMAL] );
		do_hline( window, height- (1+ofs), ofs, width-(1+ofs), basecolor(pc)[NORMAL]);
	}
}

void CPROC DrawNormalFrame( PCOMMON pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	 do_hline( window, 0, 0, width-1, basecolor(pc)[NORMAL] );
	 do_hline( window, 1, 1, width-2, basecolor(pc)[HIGHLIGHT] );
	 do_hline( window, 2, 2, width-3, basecolor(pc)[NORMAL] );
	 do_hline( window, 3, 3, width-4, basecolor(pc)[NORMAL] );

	 do_vline( window, 0, 0, height-1, basecolor(pc)[NORMAL] );
	 do_vline( window, 1, 1, height-2, basecolor(pc)[HIGHLIGHT] );
	 do_vline( window, 2, 2, height-3, basecolor(pc)[NORMAL] );
	 do_vline( window, 3, 3, height-4, basecolor(pc)[NORMAL] );

	 do_vline( window, width -4, 3, height-4, basecolor(pc)[NORMAL] );
	 do_vline( window, width -3, 2, height-3, basecolor(pc)[NORMAL] );
	 do_vline( window, width -2, 1, height-2, basecolor(pc)[SHADE] );
	 do_vline( window, width -1, 0, height-1, basecolor(pc)[SHADOW] );

	 do_hline( window, height-4, 3, width-4, basecolor(pc)[NORMAL]);
	 do_hline( window, height-3, 2, width-3, basecolor(pc)[NORMAL] );
	 do_hline( window, height-2, 1, width-2, basecolor(pc)[SHADE] );
	 do_hline( window, height-1, 0, width-1, basecolor(pc)[SHADOW] );
}

void CPROC DrawNormalFrameInverted( PCOMMON pc )
{
   Image window = pc->Window;
   _32 width = window->width;
    _32 height = window->height;
   if( pc->flags.bInitial || pc->flags.bHidden ) return;
	do_hline( window, 0, 0, width-1, basecolor(pc)[SHADOW] );
	do_hline( window, 1, 1, width-2, basecolor(pc)[SHADE] );
	do_hline( window, 2, 2, width-3, basecolor(pc)[NORMAL] );
	do_hline( window, 3, 3, width-4, basecolor(pc)[NORMAL] );

	do_vline( window, 0, 0, height-1, basecolor(pc)[SHADOW] );
	do_vline( window, 1, 1, height-2, basecolor(pc)[SHADE] );
	do_vline( window, 2, 2, height-3, basecolor(pc)[NORMAL] );
	do_vline( window, 3, 3, height-4, basecolor(pc)[NORMAL] );

	do_vline( window, width -4, 3, height-4, basecolor(pc)[NORMAL] );
	do_vline( window, width -3, 2, height-3, basecolor(pc)[NORMAL] );
	do_vline( window, width -2, 1, height-2, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[NORMAL] );

	do_hline( window, height-4, 3, width-4, basecolor(pc)[NORMAL]);
	do_hline( window, height-3, 2, width-3, basecolor(pc)[NORMAL] );
	do_hline( window, height-2, 1, width-2, basecolor(pc)[HIGHLIGHT] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[NORMAL] );
}

//---------------------------------------------------------------------------

void CPROC DrawThinnerFrameImage( PSI_CONTROL pc, Image window )
{
   //Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	do_hline( window, 0, 0, width-1, basecolor(pc)[HIGHLIGHT] );
	do_hline( window, 1, 1, width-2, basecolor(pc)[NORMAL] );

	do_vline( window, 0, 0, height-1, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, 1, 1, height-2, basecolor(pc)[NORMAL] );

	do_vline( window, width -2, 1, height-2, basecolor(pc)[SHADE] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[SHADOW] );

	do_hline( window, height-2, 1, width-2, basecolor(pc)[SHADE] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[SHADOW] );
}

//---------------------------------------------------------------------------

void CPROC DrawThinnerFrame( PCOMMON pc )
{
   Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
   if( pc->flags.bInitial || pc->flags.bHidden ) return;
	do_hline( window, 0, 0, width-1, basecolor(pc)[HIGHLIGHT] );
	do_hline( window, 1, 1, width-2, basecolor(pc)[NORMAL] );

	do_vline( window, 0, 0, height-1, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, 1, 1, height-2, basecolor(pc)[NORMAL] );

	do_vline( window, width -2, 1, height-2, basecolor(pc)[SHADE] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[SHADOW] );

	do_hline( window, height-2, 1, width-2, basecolor(pc)[SHADE] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[SHADOW] );
}

void CPROC DrawThinnerFrameInverted( PCOMMON pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	do_hline( window, 0, 0, width-1, basecolor(pc)[SHADOW] );
	do_hline( window, 1, 1, width-2, basecolor(pc)[SHADE] );

	do_vline( window, 0, 0, height-1, basecolor(pc)[SHADOW] );
	do_vline( window, 1, 1, height-2, basecolor(pc)[SHADE] );

	do_vline( window, width -2, 1, height-2, basecolor(pc)[NORMAL] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[HIGHLIGHT] );

	do_hline( window, height-2, 1, width-2, basecolor(pc)[NORMAL] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[HIGHLIGHT] );
}

void CPROC DrawThinnerFrameInvertedImage( PSI_CONTROL pc, Image window )
{
	//Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	do_hline( window, 0, 0, width-1, basecolor(pc)[SHADOW] );
	do_hline( window, 1, 1, width-2, basecolor(pc)[SHADE] );

	do_vline( window, 0, 0, height-1, basecolor(pc)[SHADOW] );
	do_vline( window, 1, 1, height-2, basecolor(pc)[SHADE] );

	do_vline( window, width -2, 1, height-2, basecolor(pc)[NORMAL] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[HIGHLIGHT] );

	do_hline( window, height-2, 1, width-2, basecolor(pc)[NORMAL] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[HIGHLIGHT] );
}

//---------------------------------------------------------------------------

void DrawThinnerFrameInset( PSI_CONTROL pc, Image window, int bInvert, int amount )
{
	_32 width = window->width;
    _32 height = window->height;
	if( !bInvert )
	{
		do_hline( window, amount+0, amount+0, width-(amount+1), basecolor(pc)[HIGHLIGHT] );
		do_hline( window, amount+1, amount+1, width-(amount+2), basecolor(pc)[NORMAL] );

		do_vline( window, amount+0, amount+0, height-(amount+1), basecolor(pc)[HIGHLIGHT] );
		do_vline( window, amount+1, amount+1, height-2, basecolor(pc)[NORMAL] );

		do_vline( window, width -(amount+2), amount+1, height-(amount+2), basecolor(pc)[SHADE] );
		do_vline( window, width -(amount+1), amount+0, height-(amount+1), basecolor(pc)[SHADOW] );

		do_hline( window, height-(amount+2), amount+1, width-(amount+2), basecolor(pc)[SHADE] );
		do_hline( window, height-(amount+1), amount+0, width-(amount+1), basecolor(pc)[SHADOW] );
	}
	else
	{
		do_hline( window, amount+0, amount+0, width-(amount+1), basecolor(pc)[SHADOW] );
		do_hline( window, amount+1, amount+1, width-(amount+2), basecolor(pc)[SHADE] );

		do_vline( window, amount+0, amount+0, height-(amount+1), basecolor(pc)[SHADOW] );
		do_vline( window, amount+1, amount+1, height-(amount+2), basecolor(pc)[SHADE] );

		do_vline( window, width -(amount+2), amount+1, height-(amount+2), basecolor(pc)[NORMAL] );
		do_vline( window, width -(amount+1), amount+0, height-(amount+1), basecolor(pc)[HIGHLIGHT] );

		do_hline( window, height-(amount+2), amount+1, width-(amount+2), basecolor(pc)[NORMAL] );
		do_hline( window, height-(amount+1), amount+0, width-(amount+1), basecolor(pc)[HIGHLIGHT] );
	}
}

//---------------------------------------------------------------------------

void CPROC DrawThinFrame( PCOMMON pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	if( pc->flags.bInitial || pc->flags.bHidden ) 
		return;
	do_hline( window, 0, 0, width-1, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, 0, 0, height-1, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[SHADOW] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[SHADOW] );
}
//---------------------------------------------------------------------------

void CPROC DrawThinFrameInverted( PCOMMON pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	do_hline( window, 0, 0, width-1, basecolor(pc)[SHADOW] );
	do_vline( window, 0, 0, height-1, basecolor(pc)[SHADOW] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[HIGHLIGHT] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[HIGHLIGHT] );
}

//---------------------------------------------------------------------------

void CPROC DrawThinFrameInvertedImage( PSI_CONTROL pc, Image window )
{
	//Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	do_hline( window, 0, 0, width-1, basecolor(pc)[SHADOW] );
	do_vline( window, 0, 0, height-1, basecolor(pc)[SHADOW] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[HIGHLIGHT] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[HIGHLIGHT] );
}

//---------------------------------------------------------------------------

void DrawThinFrameInset( PSI_CONTROL pc, Image window, int bInvert, int amount )
{
	_32 width = window->width;
	_32 height = window->height;
	if( !bInvert )
	{
		do_hline( window, amount, amount, width-(amount+1), basecolor(pc)[HIGHLIGHT] );
		do_vline( window, amount, amount, height-(amount+1), basecolor(pc)[HIGHLIGHT] );
		do_vline( window, width -(amount+1), amount, height-(amount+1), basecolor(pc)[SHADOW] );
		do_hline( window, height-(amount+1), amount, width-(amount+1), basecolor(pc)[SHADOW] );
	}
	else
	{
		do_hline( window, amount, amount, width-(amount+1), basecolor(pc)[SHADOW] );
		do_vline( window, amount, amount, height-(amount+1), basecolor(pc)[SHADOW] );
		do_vline( window, width -(amount+1), amount, height-(amount+1), basecolor(pc)[HIGHLIGHT] );
		do_hline( window, height-(amount+1), amount, width-(amount+1), basecolor(pc)[HIGHLIGHT] );
	}
}

//---------------------------------------------------------------------------

void DrawFrameCaption( PCOMMON pc )
{
	if( !pc ) return;
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	if( pc->BorderType & BORDER_NOCAPTION ) return;
	if( ( pc->BorderType & BORDER_TYPE ) == BORDER_NONE ) return;
	{
		int h, w;
		_32 width, height;
		_32 xofs = ( ( FrameBorderXOfs(pc, pc->BorderType) ) );
		h = CaptionHeight( pc, pc?GetText(pc->caption.text):NULL ) - 1;
		if( h <= 0 ) // no caption to render...
		{
			//lprintf( WIDE("But... there's no caption to render.") );
			return;
		}
#ifdef DEBUG_BORDER_FLAGS
		xlprintf(LOG_NOISE+1)( WIDE("Rendering a caption %d high"), h );
#endif
#define TEXT_INSET 5
		GetImageSize( pc->Window, &width, &height );

		if( pc->flags.bFocused )
		{
			//lprintf( WIDE("Draw focused caption on pcWindow") );
			BlatColor( pc->Window
						, xofs+1, xofs+1
						, width - 2*(xofs+1)
						, h
						, basecolor(pc)[CAPTION] );
			PutStringFont( pc->Window
							 , TEXT_INSET + xofs+2, (TEXT_INSET-2)+xofs+2 // Bad choice - but... works for now...
							 , basecolor(pc)[SHADOW], 0
							 , GetText( pc->caption.text )
							 , GetCommonFont(pc)
							 );
			PutStringFont( pc->Window
							 , TEXT_INSET + xofs+3, (TEXT_INSET-2)+xofs+3 // Bad choice - but... works for now...
							 , basecolor(pc)[HIGHLIGHT], 0
							 , GetText( pc->caption.text )
							 , GetCommonFont(pc)
							 );
			//      PutString( pc->Window
			//                       // bias the text towards the top?
			//                  , TEXT_INSET + xofs+1, (TEXT_INSET-2)+xofs+1 // Bad choice - but... works for now...
			//                  , basecolor(pc)[CAPTIONTEXTCOLOR], 0
			//                  , GetText( pc->caption.text ) );
			h += xofs;
			w = width - (xofs+1);
			do_hline( pc->Window
					  , xofs, xofs, w
					  , basecolor(pc)[SHADE] );
			do_vline( pc->Window
					  , xofs, xofs, h
					  , basecolor(pc)[SHADE] );
			do_hline( pc->Window
					  , h, xofs, w
					  , basecolor(pc)[HIGHLIGHT] );
			do_vline( pc->Window
					  , w, xofs, h
					  , basecolor(pc)[HIGHLIGHT] );
		}
		else
		{
			//lprintf( WIDE("Draw unfocused caption on pcWindow") );
			BlatColor( pc->Window
						, xofs+1, xofs+1
						, width - 2*(xofs+1)
						, h
						, basecolor(pc)[INACTIVECAPTION] );
			PutStringFont( pc->Window
							 , TEXT_INSET + xofs+1, (TEXT_INSET-2)+xofs+1 // Bad choice - but... works for now...
							 , basecolor(pc)[SHADOW], 0
							 , GetText( pc->caption.text )
							 , GetCommonFont(pc)
							 );
			PutStringFont( pc->Window
							 , TEXT_INSET + xofs+2, (TEXT_INSET-2)+xofs+2 // Bad choice - but... works for now...
							 , basecolor(pc)[HIGHLIGHT], 0
							 , GetText( pc->caption.text )
							 , GetCommonFont(pc)
							 );
			//      PutString( pc->Window
			//                  , TEXT_INSET + xofs+1, (TEXT_INSET-2)+xofs+1 // Bad choice - but... works for now...
			//                  , basecolor(pc)[INACTIVECAPTIONTEXTCOLOR], 0
			//                  , GetText( pc->caption.text ) );
			h += xofs;
			w = width - (xofs+1);
			do_hline( pc->Window
					  , xofs, xofs, w
					  , basecolor(pc)[HIGHLIGHT] );
			do_vline( pc->Window
					  , xofs, xofs, h
					  , basecolor(pc)[HIGHLIGHT] );
			do_hline( pc->Window
					  , h, xofs, w
					  , basecolor(pc)[SHADE] );
			do_vline( pc->Window
					  , w, xofs, h
					  , basecolor(pc)[SHADE] );
		}
	}
	//lprintf( WIDE("Is anything going to output this to the window?") );
}

//---------------------------------------------------------------------------

void CPROC DrawThickDent( PCOMMON pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawNormalFrameInverted( pc );
	DrawNormalFrameInset( pc, pc->Window, FALSE, 4 );
}

//---------------------------------------------------------------------------

void CPROC DrawThickDentInverted( PCOMMON pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawNormalFrame( pc );
	DrawNormalFrameInset( pc, pc->Window, TRUE, 4 );
}

//---------------------------------------------------------------------------

void CPROC DrawDent( PCOMMON pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinnerFrameInverted( pc );
	DrawThinFrameInset( pc, pc->Window, FALSE, 2 );
}

//---------------------------------------------------------------------------

void CPROC DrawDentInverted( PCOMMON pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinnerFrame( pc );
	DrawThinFrameInset( pc, pc->Window, TRUE, 2 );
}

//---------------------------------------------------------------------------
void CPROC DrawThinDent( PCOMMON pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinFrameInverted( pc );
	DrawThinFrameInset( pc, pc->Window, FALSE, 1 );
}

//---------------------------------------------------------------------------

void CPROC DrawThinDentInverted( PCOMMON pc )
{
   if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinFrame( pc );
	DrawThinFrameInset( pc, pc->Window, TRUE, 1 );
}

//---------------------------------------------------------------------------
void UpdateSurface( PCOMMON pc );

void CPROC SetDrawBorder( PCOMMON pc )
{
	// psv unused...
	UpdateSurface( pc );
	switch( pc->BorderType & BORDER_TYPE )
	{
	case BORDER_NONE:
		pc->DrawBorder = NULL;
		break;
	case BORDER_NORMAL:
		if( pc->BorderType & BORDER_RESIZABLE )
		{
			if( pc->BorderType & BORDER_INVERT )
				pc->DrawBorder = DrawThickFrameInverted;
			else
				pc->DrawBorder = DrawThickFrame;
		}
		else
		{
			if( pc->BorderType & BORDER_INVERT )
				pc->DrawBorder = DrawNormalFrameInverted;
			else
			{
				extern void TryLoadingFrameImage( void );
				TryLoadingFrameImage();
				pc->DrawBorder = g.BorderImage?DrawFancyFrame:DrawNormalFrame;
			}
		}
	break;
	case BORDER_THINNER:
		if( pc->BorderType & BORDER_INVERT )
			pc->DrawBorder = DrawThinnerFrameInverted;
		else
			pc->DrawBorder = DrawThinnerFrame;
	break;
	case BORDER_THIN:
		if( pc->BorderType & BORDER_INVERT )
			pc->DrawBorder = DrawThinFrameInverted;
		else
			pc->DrawBorder = DrawThinFrame;
	break;
	case BORDER_THICK_DENT:
		if( pc->BorderType & BORDER_INVERT )
			pc->DrawBorder = DrawThickDentInverted;
		else
			pc->DrawBorder = DrawThickDent;
	break;
	case BORDER_DENT:
		if( pc->BorderType & BORDER_INVERT )
			pc->DrawBorder = DrawDentInverted;
		else
			pc->DrawBorder = DrawDent;
	break;
	case BORDER_THIN_DENT:
		if( pc->BorderType & BORDER_INVERT )
			pc->DrawBorder = DrawThinDentInverted;
		else
			pc->DrawBorder = DrawThinDent;
	break;
	}
	//if( !pc->nType )
	//{
	//   lprintf( WIDE("Draw frame caption....") );
	//}
	if( pc->flags.bInitial )
	{
#ifdef DEBUG_BORDER_FLAGS
		lprintf( WIDE("Initial set - return early.") );
#endif
		return;
	}
	//lprintf( WIDE("Oka so the caption will be draw...") );
	if( !g.flags.always_draw )
	{
		if( pc->DrawBorder && pc->Window )
		{
#ifdef DEBUG_BORDER_DRAWING
			lprintf( "Calling drawing of the border" );
#endif
			pc->DrawBorder( pc );
		}
		if( pc->device )
			DrawFrameCaption( pc );
	}
}

//---------------------------------------------------------------------------

void UpdateSurface( PCOMMON pc )
{
	_32 border;
	_32 width, height;
	//xlprintf(2100)( "Update Surface... %p  %08x", pc, pc->BorderType );
	border = pc->BorderType;
	width = pc->rect.width;
	height = pc->rect.height;
	pc->surface_rect.x = FrameBorderXOfs(pc, border);
	pc->surface_rect.y = FrameBorderYOfs(pc, border, GetText(pc->caption.text));
	pc->surface_rect.width = width - FrameBorderX(pc, border);
	pc->surface_rect.height = height - FrameBorderY(pc, border, GetText(pc->caption.text));
	if( pc->Surface )
	{
#ifdef DEBUG_BORDER_FLAGS
		lprintf( WIDE("- - - -- -- -  -- - -  ---- position is like %d,%d  %d,%d")
				 , pc->surface_rect.x, pc->surface_rect.y
				 , pc->surface_rect.width, pc->surface_rect.height );
#endif
		MoveImage( pc->Surface, pc->surface_rect.x, pc->surface_rect.y );
		ResizeImage( pc->Surface, pc->surface_rect.width, pc->surface_rect.height );
	}
	else
	{
#ifdef DEBUG_BORDER_FLAGS
		lprintf( WIDE("--------------------------------- "));
#endif
		pc->Surface = MakeSubImage( pc->Window
										  , pc->surface_rect.x
										  , pc->surface_rect.y
										  , pc->surface_rect.width
										  , pc->surface_rect.height );
		//lprintf( WIDE("Resulting surface is %p in %p"), pc->Surface, pc->Window );
	}
}

PSI_PROC( void, SetCommonBorderEx )( PCOMMON pc, _32 BorderType DBG_PASS )
{
	//_xlprintf((LOG_NOISE+2) DBG_RELAY)( WIDE("Setting border for %s to %08x(%08x,%08x) %08x %08x"), pc->pTypeName, pc, pc->parent, pc->device, pc->BorderType, BorderType );
	if( pc->BorderType != BorderType )
	{
		pc->BorderType = BorderType;
		pc->flags.bSetBorderType = 1;
	}
	// this is also called when the surface changes....
	SetDrawBorder( pc );
}
PSI_NAMESPACE_END

