//#define DEBUG_BORDER_FLAGS

/* this flag is defined in controlstruc.h...
 *
 * #define DEBUG_BORDER_DRAWING
 * #define QUICK_DEBUG_BORDER_FLAGS // simple set - tracks who sets border where
 *
 */

#define USE_INTERFACE_MANAGER

#include "controlstruc.h"
#include "global.h"
#include <psi.h>

PSI_NAMESPACE

//---------------------------------------------------------------------------
	
//---------------------------------------------------------------------------

void DrawNormalFrameInset( PSI_CONTROL pc, Image window, int bInvert, int align )
{
}

//---------------------------------------------------------------------------

void CPROC DrawFancyFrame( PSI_CONTROL pc )
{
	int tmp;
	Image window = pc->Window;
	PFrameBorder border = pc->border;
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

	switch( border->Border.bAnchorTop )
	{
	default:
	case 0: // none, just scale
		BlotScaledImageSizedToAlpha( window, border->BorderSegment[SEGMENT_TOP]
									, pc->surface_rect.x, 0
									, pc->surface_rect.width, border->BorderHeight
									, ALPHA_TRANSPARENT );
		break;
	case 1: // left
		tmp = border->BorderSegment[SEGMENT_TOP]->width;
		if( SUS_LT( pc->surface_rect.width, IMAGE_SIZE_COORDINATE, tmp, int ) )
			tmp = pc->surface_rect.width;
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_TOP]
									, pc->surface_rect.x, 0
									, pc->surface_rect.width, border->BorderHeight
									, 0, 0
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		
		break;
	case 2: // center
		tmp = border->BorderSegment[SEGMENT_TOP]->width;
		if( SUS_LT( pc->surface_rect.width, IMAGE_SIZE_COORDINATE, tmp, int ) )
			tmp = pc->surface_rect.width;
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_TOP]
									, pc->surface_rect.x, 0
									, pc->surface_rect.width, border->BorderHeight
									, (border->BorderSegment[SEGMENT_TOP]->width - tmp )/2, 0
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		break;
	case 3: // right
		tmp = border->BorderSegment[SEGMENT_TOP]->width;
		if( SUS_LT( pc->surface_rect.width, IMAGE_SIZE_COORDINATE, tmp, int ) )
			tmp = pc->surface_rect.width;
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_TOP]
									, pc->surface_rect.x, 0
									, pc->surface_rect.width, border->BorderHeight
									, border->BorderSegment[SEGMENT_TOP]->width - tmp, 0
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		
		break;
	}
	switch( border->Border.bAnchorBottom )
	{
	default:
	case 0: // none, just scale
		BlotScaledImageSizedToAlpha( window, border->BorderSegment[SEGMENT_BOTTOM]
									, pc->surface_rect.x, pc->surface_rect.y + pc->surface_rect.height
									, pc->surface_rect.width, border->BorderHeight
									, ALPHA_TRANSPARENT );
		break;
	case 1: // left
		tmp = border->BorderSegment[SEGMENT_BOTTOM]->width;
		if( SUS_LT( pc->surface_rect.width, IMAGE_SIZE_COORDINATE, tmp, int ) )
			tmp = pc->surface_rect.width;
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_BOTTOM]
									, pc->surface_rect.x, pc->surface_rect.y + pc->surface_rect.height
									, pc->surface_rect.width, border->BorderHeight
									, 0, 0
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		
		break;
	case 2: // center
		tmp = border->BorderSegment[SEGMENT_BOTTOM]->width;
		if( SUS_LT( pc->surface_rect.width, IMAGE_SIZE_COORDINATE, tmp, int ) )
			tmp = pc->surface_rect.width;
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_BOTTOM]
									, pc->surface_rect.x, pc->surface_rect.y + pc->surface_rect.height
									, pc->surface_rect.width, border->BorderHeight
									, (border->BorderSegment[SEGMENT_BOTTOM]->width - tmp )/2, 0
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		break;
	case 3: // right
		tmp = border->BorderSegment[SEGMENT_BOTTOM]->width;
		if( SUS_LT( pc->surface_rect.width, IMAGE_SIZE_COORDINATE, tmp, int ) )
			tmp = pc->surface_rect.width;
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_BOTTOM]
									, pc->surface_rect.x, pc->surface_rect.y + pc->surface_rect.height
									, pc->surface_rect.width, border->BorderHeight
									, border->BorderSegment[SEGMENT_BOTTOM]->width - tmp, 0
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		break;
	}
	switch( border->Border.bAnchorLeft )
	{
	default:
	case 0: // none, just scale
		BlotScaledImageSizedToAlpha( window, border->BorderSegment[SEGMENT_LEFT]
											, 0, border->BorderHeight
											, border->BorderWidth, window->height - ( border->BorderHeight * 2 )
											, ALPHA_TRANSPARENT );
		break;
	case 1: // top
		tmp = border->BorderSegment[SEGMENT_LEFT]->height;
		if( ( window->height - 2 * border->BorderHeight ) < tmp )
			tmp = ( window->height - 2 * border->BorderHeight );
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_LEFT]
									, 0, border->BorderHeight
									, border->BorderWidth, window->height - ( border->BorderHeight * 2 )
									, 0, 0
									, border->BorderWidth, tmp
									, ALPHA_TRANSPARENT, BLOT_COPY );
		
		break;
	case 2: // center
		tmp = border->BorderSegment[SEGMENT_LEFT]->height;
		if( ( window->height - 2 * border->BorderHeight ) < tmp )
			tmp = ( window->height - 2 * border->BorderHeight );
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_LEFT]
									, 0, border->BorderHeight
									, border->BorderWidth, window->height - ( border->BorderHeight * 2 )
									, 0, (border->BorderSegment[SEGMENT_LEFT]->height - tmp )/2
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		break;
	case 3: // bottom
		tmp = border->BorderSegment[SEGMENT_LEFT]->height;
		if( ( window->height - 2 * border->BorderHeight ) < tmp )
			tmp = ( window->height - 2 * border->BorderHeight );
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_LEFT]
									, 0, border->BorderHeight
									, border->BorderWidth, window->height - ( border->BorderHeight * 2 )
									, 0, border->BorderSegment[SEGMENT_LEFT]->height - tmp
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );		
		break;
	}
	switch( border->Border.bAnchorRight )
	{
	default:
	case 0: // none, just scale
		BlotScaledImageSizedToAlpha( window, border->BorderSegment[SEGMENT_RIGHT]
											, pc->surface_rect.x + pc->surface_rect.width, border->BorderHeight
											, border->BorderWidth, window->height - ( border->BorderHeight * 2 )
											, ALPHA_TRANSPARENT );
		break;
	case 1: // top
		tmp = border->BorderSegment[SEGMENT_RIGHT]->height;
		if( ( window->height - 2 * border->BorderHeight ) < tmp )
			tmp = ( window->height - 2 * border->BorderHeight );
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_RIGHT]
									, pc->surface_rect.x + pc->surface_rect.width, border->BorderHeight
									, border->BorderWidth, window->height - ( 2 * border->BorderHeight )
									, 0, 0
									, border->BorderWidth, tmp
									, ALPHA_TRANSPARENT, BLOT_COPY );
		
		break;
	case 2: // center
		tmp = border->BorderSegment[SEGMENT_RIGHT]->height;
		if( ( window->height - 2 * border->BorderHeight ) < tmp )
			tmp = ( window->height - 2 * border->BorderHeight );
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_RIGHT]
									, pc->surface_rect.x + pc->surface_rect.width, border->BorderHeight
									, border->BorderWidth, window->height - ( 2 * border->BorderHeight )
									, 0, (border->BorderSegment[SEGMENT_RIGHT]->height - tmp )/2
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );
		break;
	case 3: // bottom
		tmp = border->BorderSegment[SEGMENT_RIGHT]->height;
		if( ( window->height - 2 * border->BorderHeight ) < tmp )
			tmp = ( window->height - 2 * border->BorderHeight );
		BlotScaledImageSizedEx( window, border->BorderSegment[SEGMENT_RIGHT]
									, pc->surface_rect.x + pc->surface_rect.width, border->BorderHeight
									, border->BorderWidth, window->height - ( 2 * border->BorderHeight )
									, 0, border->BorderSegment[SEGMENT_RIGHT]->height - tmp
									, tmp, border->BorderHeight
									, ALPHA_TRANSPARENT, BLOT_COPY );		
		break;
	}
	BlotImageAlpha( window, border->BorderSegment[SEGMENT_TOP_LEFT]
					  , 0, 0
					  , ALPHA_TRANSPARENT );
	BlotImageAlpha( window, border->BorderSegment[SEGMENT_TOP_RIGHT]
					  , border->BorderWidth + pc->surface_rect.width, 0
					  , ALPHA_TRANSPARENT );
	BlotImageAlpha( window, border->BorderSegment[SEGMENT_BOTTOM_LEFT]
					  , 0, window->height - border->BorderHeight
					  , ALPHA_TRANSPARENT );
	BlotImageAlpha( window, border->BorderSegment[SEGMENT_BOTTOM_RIGHT]
					  , border->BorderWidth + pc->surface_rect.width, window->height - border->BorderHeight
					  , ALPHA_TRANSPARENT );
	//lprintf( "Finished fancy border draw." );
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
          return pc->border->BorderWidth;
		 }
		 /*
		 if( BorderType & BORDER_RESIZABLE )
		 {
			 return 8;
		 }
		 else
		 */
			 return (pc->border&&pc->border->BorderImage)?pc->border->BorderHeight:4;
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
          return pc->border->BorderWidth*2;
		 }
		 /*
        if( BorderType & BORDER_RESIZABLE )
            return 16;
        else
		*/
            return (pc->border&&pc->border->BorderImage)?pc->border->BorderWidth * 2:8;
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
        return 2*2;
    }
    // should actually compute this from facts known about pf
    return 8;
}

//---------------------------------------------------------------------------

PSI_PROC( int, CaptionHeight )( PSI_CONTROL pf, CTEXTSTR text )
{
    // should resemble something like text height + 3 (top) + 3 (1 bottom, 2 frameline)
    // pf itself may have a different font - which
   // will then switch the height...
	if( pf->DrawCaption )
	{
		return pf->nCaptionHeight;
	}
	else if( pf->caption.image  )
	{
		return pf->caption.image->height + pf->caption.pad*2;
	}
	else if( text || ( pf &&
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

PSI_PROC( int, FrameBorderYOfs )( PSI_CONTROL pc, _32 BorderType, CTEXTSTR caption )
{
	int result = 0;
	if( !(BorderType & BORDER_NOCAPTION ) && 
		( pc->DrawBorder || caption ) )
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
			return result + pc->border->BorderHeight;
		}
		/*
		if( BorderType & BORDER_RESIZABLE )
			return result + 8;
		else
		*/
			return result + ( ( ( (pc->border&&pc->border->BorderImage)?pc->border->BorderHeight:4)* 3 ) / 4 );
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

int FrameCaptionYOfs( PSI_CONTROL pc, _32 BorderType )
{
	int result = 0;

	//if( !pf )
	//  return CaptionHeight( caption, NULL ) + 4;
	switch( BorderType & BORDER_TYPE )
	{
	case BORDER_NONE:
		return 0;
	case BORDER_NORMAL:
		if( pc && pc->DrawBorder == DrawFancyFrame )
		{
			return result + pc->border->BorderHeight;
		}
		/*
		if( BorderType & BORDER_RESIZABLE )
			return result + 8;
		else
		*/
			return result + ( ( ((pc->border&&pc->border->BorderImage)?pc->border->BorderHeight:4)* 3 ) / 4 );
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

PSI_PROC( int, FrameBorderY )( PSI_CONTROL pc, _32 BorderType, CTEXTSTR caption )
{
	int result = 0;
	if( !(BorderType & BORDER_NOCAPTION ) && 
		( pc->DrawBorder || caption ) )
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
			return result + pc->border->BorderHeight*2;
		}
		/*
		if( BorderType & BORDER_RESIZABLE )
		{
			return result + 16;
		}
		else
		*/
		{
			return result + (((pc->border&&pc->border->BorderImage)?pc->border->BorderHeight:4) * 7 / 4 );
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
		return result + 4;
	}
	// should actually compute this from facts known about pf
	return result + 4;
}

//---------------------------------------------------------------------------

void CPROC DrawThickFrame( PSI_CONTROL pc )
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
void CPROC DrawThickFrameInverted( PSI_CONTROL pc )
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

void CPROC DrawNormalFrame( PSI_CONTROL pc )
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

void CPROC DrawNormalFrameInverted( PSI_CONTROL pc )
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

void CPROC DrawThinnerFrame( PSI_CONTROL pc )
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

void CPROC DrawThinnerFrameInverted( PSI_CONTROL pc )
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

void CPROC DrawThinFrame( PSI_CONTROL pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	if( /*pc->flags.bInitial ||*/ pc->flags.bHidden ) 
		return;
	//lprintf( "Draw thin frame: %08x %08x  %d %d", basecolor(pc)[SHADOW], basecolor(pc)[HIGHLIGHT], width, height );
	do_hline( window, 0, 0, width-1, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, 0, 0, height-1, basecolor(pc)[HIGHLIGHT] );
	do_vline( window, width -1, 0, height-1, basecolor(pc)[SHADOW] );
	do_hline( window, height-1, 0, width-1, basecolor(pc)[SHADOW] );
}
//---------------------------------------------------------------------------

void CPROC DrawThinFrameInverted( PSI_CONTROL pc )
{
	Image window = pc->Window;
	_32 width = window->width;
	_32 height = window->height;
	if( /*pc->flags.bInitial ||*/ pc->flags.bHidden ) return;
	//lprintf( "Draw thin frame Inverted: %08x %08x  %d %d", basecolor(pc)[SHADOW], basecolor(pc)[HIGHLIGHT], width, height );
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

void DrawFrameCaption( PSI_CONTROL pc )
{
	if( !pc ) return;
	if( /*pc->flags.bInitial ||*/ pc->flags.bHidden ) return;
	if( pc->BorderType & BORDER_NOCAPTION ) return;
	if( ( pc->BorderType & BORDER_TYPE ) == BORDER_NONE ) return;
	{
		int h, w;
		_32 button_left;
		_32 width, height;
		_32 xofs = ( ( FrameBorderXOfs(pc, pc->BorderType) ) );
		_32 yofs = ( ( FrameCaptionYOfs(pc, pc->BorderType ) ) );
		Image out = NULL;
		h = CaptionHeight( pc, pc?GetText(pc->caption.text):NULL ) - 1;
		if( h <= 0 ) // no caption to...
		{
			//lprintf( WIDE("But... there's no caption to render.") );
			return;
		}
#ifdef DEBUG_BORDER_FLAGS
		xlprintf(LOG_NOISE+1)( WIDE("Rendering a caption %d high"), h );
#endif
#define TEXT_INSET 5
		GetImageSize( pc->Window, &width, &height );
		w = width - (xofs + 2);
		if( pc->DrawCaption )
		{
			//lprintf( "Draw custom caption" );
			pc->DrawCaption( pc, pc->pCaptionImage );
		}
		else
		{
			if( pc->flags.bFocused )
			{
				if( g.FrameCaptionFocusedImage || g.FrameCaptionImage )
				{
					out = g.FrameCaptionFocusedImage;
					if( !out )
						out = g.FrameCaptionImage;
					{
						S_32 outx = 0;
						S_32 outy = 0;
						_32 outw = width - 2 *xofs;
						_32 outh = h+2;
						_32 routw = width - 2 *xofs;
						_32 routh = h+2;
						if( (h+2) < out->height )
							outh = h+2;
						else
							routh = out->height;
						if ( outw < out->width )
						{
							outx = out->width/2 - (outw)/2;
						}
						else
						{
							outw = out->width;						
						}
						BlotScaledImageSizedEx( pc->Window, out, xofs, yofs, routw, routh, outx, outy, outw, outh, ALPHA_TRANSPARENT, BLOT_COPY );
					}
				}
				else
				{
					//lprintf( WIDE("Draw focused caption on pcWindow") );
					BlatColor( pc->Window
								, xofs+1, yofs+1
								, width - 2*(xofs+1)
								, h
								, basecolor(pc)[CAPTION] );
				}
				if( pc->caption.image )
				{
					BlotImageAlpha( pc->Window, pc->caption.image, xofs + pc->caption.pad, yofs + pc->caption.pad, ALPHA_TRANSPARENT );
				}
				else
				{
					PutStringFont( pc->Window
									 , TEXT_INSET + xofs+2, (TEXT_INSET-2)+yofs+2 // Bad choice - but... works for now...
									 , basecolor(pc)[SHADOW], 0
									 , GetText( pc->caption.text )
									 , GetCommonFont(pc)
									 );
					PutStringFont( pc->Window
									 , TEXT_INSET + xofs+3, (TEXT_INSET-2)+yofs+3 // Bad choice - but... works for now...
									 , basecolor(pc)[HIGHLIGHT], 0
									 , GetText( pc->caption.text )
									 , GetCommonFont(pc)
									 );
				}
				//      PutString( pc->Window
				//                       // bias the text towards the top?
				//                  , TEXT_INSET + xofs+1, (TEXT_INSET-2)+xofs+1 // Bad choice - but... works for now...
				//                  , basecolor(pc)[CAPTIONTEXTCOLOR], 0
				//                  , GetText( pc->caption.text ) );
				//h += yofs;
				if( !out ) // out is set when drawing a image...
				{
					w = width - (yofs+1);
					do_hline( pc->Window
							  , yofs, xofs, w
							  , basecolor(pc)[SHADE] );
					do_vline( pc->Window
							  , xofs, yofs, yofs+h
							  , basecolor(pc)[SHADE] );
					do_hline( pc->Window
							  , yofs+h, xofs, w
							  , basecolor(pc)[HIGHLIGHT] );
					do_vline( pc->Window
							  , w, yofs, yofs+h
							  , basecolor(pc)[HIGHLIGHT] );
				}
			}
			else
			{
				//lprintf( WIDE("Draw unfocused caption on pcWindow") );
				if( g.FrameCaptionFocusedImage || g.FrameCaptionImage )
				{
					out = g.FrameCaptionImage;
					if( !out )
						out = g.FrameCaptionFocusedImage;
					{
						S_32 outx = 0;
						S_32 outy = 0;
						_32 outw = width - 2 *xofs;
						_32 outh = h+2;
						_32 routw = width - 2 *xofs;
						_32 routh = h+2;
						w = width - (xofs + 2);
						if( (h+2) < out->height )
							outh = h+2;
						else
							routh = out->height;
						if ( outw < out->width )
						{
							outx = out->width/2 - (outw)/2;
						}
						else
						{
							outw = out->width;						
						}
						BlotScaledImageSizedEx( pc->Window, out, xofs, yofs, routw, routh, outx, outy, outw, outh, ALPHA_TRANSPARENT, BLOT_COPY );
					}
				}

				if( pc->caption.image )
				{
					BlotImageAlpha( pc->Window, pc->caption.image, xofs + pc->caption.pad, yofs + pc->caption.pad, ALPHA_TRANSPARENT );
				}
				else
				{
					BlatColor( pc->Window
								, xofs+1, yofs+1
								, width - 2*(xofs+1)
								, h
								, basecolor(pc)[INACTIVECAPTION] );
					PutStringFont( pc->Window
										, TEXT_INSET + xofs+1, (TEXT_INSET-2)+yofs+1 // Bad choice - but... works for now...
										, basecolor(pc)[SHADOW], 0
										, GetText( pc->caption.text )
										, GetCommonFont(pc)
										);
					PutStringFont( pc->Window
										, TEXT_INSET + xofs+2, (TEXT_INSET-2)+yofs+2 // Bad choice - but... works for now...
										, basecolor(pc)[HIGHLIGHT], 0
										, GetText( pc->caption.text )
										, GetCommonFont(pc)
										);
				}
				//      PutString( pc->Window
				//                  , TEXT_INSET + xofs+1, (TEXT_INSET-2)+xofs+1 // Bad choice - but... works for now...
				//                  , basecolor(pc)[INACTIVECAPTIONTEXTCOLOR], 0
				//                  , GetText( pc->caption.text ) );
				//h += yofs;
				if( !out )
				{
					w = width - (yofs+1);
					do_hline( pc->Window
							  , yofs, xofs, w
							  , basecolor(pc)[HIGHLIGHT] );
					do_vline( pc->Window
							  , xofs, yofs, yofs+h
							  , basecolor(pc)[HIGHLIGHT] );
					do_hline( pc->Window
							  , yofs+h, xofs, w
							  , basecolor(pc)[SHADE] );
					do_vline( pc->Window
							  , w, yofs, yofs+h
							  , basecolor(pc)[SHADE] );
				}
			}
		}
		button_left = w - (h /* + 3 */ - 2) + pc->caption_button_x_ofs;
		if( pc->device )
		{
			INDEX idx;
			PCAPTION_BUTTON button;
			LIST_FORALL( pc->caption_buttons, idx, PCAPTION_BUTTON, button )
			{
				if( button->flags.hidden )
					continue;
				button->offset = button_left + button->extra_pad;
				if( button->is_pressed )
				{
					if( button->pressed )
						BlotScaledImageSizedToAlpha( pc->Window, button->pressed
								, button_left+ button->extra_pad
								, yofs + 1+ button->extra_pad + pc->caption_button_y_ofs
								, (h) - 2 - ( 2*button->extra_pad)
								, (h) - 2 - ( 2*button->extra_pad), ALPHA_TRANSPARENT );
				}
				else
				{
					if( button->normal )
						BlotScaledImageSizedToAlpha( pc->Window, button->normal
								, button_left + button->extra_pad
								, yofs + 1 + button->extra_pad + pc->caption_button_y_ofs
								, (h) - 2 - ( 2*button->extra_pad)
								, (h) - 2 - ( 2*button->extra_pad), ALPHA_TRANSPARENT );
				}
				if( button->flags.rollover && button->highlight )
				{
					BlotScaledImageSizedToAlpha( pc->Window, button->highlight
								, button_left + button->extra_pad
								, yofs + 1 + button->extra_pad + pc->caption_button_y_ofs
								, (h) - 2 - ( 2*button->extra_pad)
								, (h) - 2 - ( 2*button->extra_pad), ALPHA_TRANSPARENT );
				}
				button_left -= h - button->extra_pad*2;
			}
		}
	}
	//lprintf( WIDE("Is anything going to output this to the window?") );
}

//---------------------------------------------------------------------------

void CPROC DrawThickDent( PSI_CONTROL pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawNormalFrameInverted( pc );
	DrawNormalFrameInset( pc, pc->Window, FALSE, 4 );
}

//---------------------------------------------------------------------------

void CPROC DrawThickDentInverted( PSI_CONTROL pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawNormalFrame( pc );
	DrawNormalFrameInset( pc, pc->Window, TRUE, 4 );
}

//---------------------------------------------------------------------------

void CPROC DrawDent( PSI_CONTROL pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinnerFrameInverted( pc );
	DrawThinFrameInset( pc, pc->Window, FALSE, 2 );
}

//---------------------------------------------------------------------------

void CPROC DrawDentInverted( PSI_CONTROL pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinnerFrame( pc );
	DrawThinFrameInset( pc, pc->Window, TRUE, 2 );
}

//---------------------------------------------------------------------------
void CPROC DrawThinDent( PSI_CONTROL pc )
{
	if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinFrameInverted( pc );
	DrawThinFrameInset( pc, pc->Window, FALSE, 1 );
}

//---------------------------------------------------------------------------

void CPROC DrawThinDentInverted( PSI_CONTROL pc )
{
   if( pc->flags.bInitial || pc->flags.bHidden ) return;
	DrawThinFrame( pc );
	DrawThinFrameInset( pc, pc->Window, TRUE, 1 );
}

//---------------------------------------------------------------------------

static void CPROC DrawCustomBorder( PSI_CONTROL pc )
{
	if( pc )
	{
		if( pc->BorderDrawProc )
         pc->BorderDrawProc( pc, pc->Window );
	}
}

//---------------------------------------------------------------------------

void CPROC SetDrawBorder( PSI_CONTROL pc )
{
	// psv unused...
	UpdateSurface( pc );
	switch( pc->BorderType & BORDER_TYPE )
	{
	case BORDER_USER_PROC:
		if( pc->BorderDrawProc )
         pc->DrawBorder = DrawCustomBorder;
      break;
	case BORDER_NONE:
		pc->DrawBorder = NULL;
		break;
	case BORDER_NORMAL:
		/*
		if( pc->BorderType & BORDER_RESIZABLE )
		{
			if( pc->BorderType & BORDER_INVERT )
				pc->DrawBorder = DrawThickFrameInverted;
			else
				pc->DrawBorder = DrawThickFrame;
		}
		else
		*/
		{
			if( pc->BorderType & BORDER_INVERT )
				pc->DrawBorder = DrawNormalFrameInverted;
			else
			{
				extern void TryLoadingFrameImage( void );
				if( !g.flags.system_color_set )
				{
					//TryLoadingFrameImage();
					pc->DrawBorder = (pc->border&&pc->border->BorderImage)?DrawFancyFrame:DrawNormalFrame;
				}
				else
					pc->DrawBorder = DrawNormalFrame;

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
			ResetImageBuffers( pc->Window, TRUE );	
			pc->DrawBorder( pc );
		}
		if( pc->device )
			DrawFrameCaption( pc );
	}
}

//---------------------------------------------------------------------------

void UpdateSurface( PSI_CONTROL pc )
{
	_32 border;
	_32 width, height;
	LOGICAL size_changed = FALSE;
	LOGICAL pos_changed = FALSE;
	//xlprintf(2100)( "Update Surface... %p  %08x", pc, pc->BorderType );
	border = pc->BorderType;
	width = pc->rect.width;
	height = pc->rect.height;
	if( ( pc->BorderType == BORDER_USER_PROC ) && pc->BorderMeasureProc )
	{
		int left, top, right,bottom;
		pc->BorderMeasureProc( pc, &left, &top, &right, &bottom );
		if( pc->surface_rect.x != left || pc->surface_rect.y != top )
		{

			pc->surface_rect.x = left;
			pc->surface_rect.y = top;
			pos_changed = TRUE;
		}
		if( ( pc->surface_rect.width != (width - right) ) || ( pc->surface_rect.height != (height-bottom) ) )
		{
			pc->surface_rect.width = width - right;
			pc->surface_rect.height = height - bottom;
			size_changed = TRUE;
		}
	}
	else
	{
		int left, top, right,bottom;
		left = FrameBorderXOfs(pc, border);
		top = FrameBorderYOfs(pc, border, GetText(pc->caption.text));
		right = FrameBorderX(pc, border);
		bottom = FrameBorderY(pc, border, GetText(pc->caption.text));
		if( pc->DrawCaption )
		{
			MoveImage( pc->pCaptionImage, left, FrameCaptionYOfs( pc, border ) );
			ResizeImage( pc->pCaptionImage, ( width - right ), pc->nCaptionHeight );
			//MoveImage( pc->pCaptionImage, left - 1, FrameCaptionYOfs( pc, border ) );
			//ResizeImage( pc->pCaptionImage, ( width - right ) + 2, pc->nCaptionHeight );
		}
		if( pc->surface_rect.x != left || pc->surface_rect.y != top )
		{
			pc->surface_rect.x = left;
			pc->surface_rect.y = top;
			pos_changed = TRUE;
		}
		if( ( pc->surface_rect.width != (width - right) ) || ( pc->surface_rect.height != (height-bottom) ) )
		{
			pc->surface_rect.width = width - (right);
			pc->surface_rect.height = height - (bottom);
			size_changed = TRUE;
		}
	}
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
	if( pos_changed )
	{
		//if( pc-> )
		//pc->Move( pc, FALSE );
	}
	if( size_changed && pc->Surface )
	{
		if( pc->Resize )
			pc->Resize( pc, FALSE );
	}
}

PSI_PROC( void, SetCommonBorderEx )( PSI_CONTROL pc, _32 BorderType DBG_PASS )
{
	//_xlprintf((LOG_NOISE+2) DBG_RELAY)( WIDE("Setting border for %s to %08x(%08x,%08x) %08x %08x"), pc->pTypeName, pc, pc->parent, pc->device, pc->BorderType, BorderType );
	if( pc->BorderType != BorderType )
	{
		pc->BorderType = BorderType;
		pc->flags.bSetBorderType = 1;
		if( BorderType & BORDER_FIXED )
		{
			MoveSizeCommon( pc, pc->original_rect.x, pc->original_rect.y, pc->original_rect.width, pc->original_rect.height );
		}
	}
	// this is also called when the surface changes....
	SetDrawBorder( pc );
}


PSI_PROC( void, PSI_SetCustomBorder )( PSI_CONTROL pc, void (CPROC*proc)(PSI_CONTROL,Image)
                                      , void (CPROC*measure_proc)( PSI_CONTROL,int *x_offset, int *y_offset, int *right_inset, int *bottom_inset )
												 )
{
	if( pc )
	{
		pc->BorderDrawProc = proc;
		pc->BorderMeasureProc = measure_proc;
      pc->BorderType = BORDER_USER_PROC;
	}
}

void SetCaptionHeight( PSI_CONTROL pc, int height )
{
	if( pc )
	{
		pc->nCaptionHeight = height;
		UpdateSurface( pc );
	}
}


PSI_NAMESPACE_END

