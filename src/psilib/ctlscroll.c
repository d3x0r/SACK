#include <stdhdrs.h>
#include <string.h>
#include <sharemem.h>

#include "controlstruc.h"
#include <keybrd.h>
#include <psi.h>
#define BTN_LESS 100
#define BTN_MORE 101

//---------------------------------------------------------------------------
PSI_SCROLLBAR_NAMESPACE

typedef struct scrollbar_tag
{
   uint32_t attr;
	Image   surface; // this is the surface of the 'bar' itself
	int min     // smallest value of bar
	  , current // first line shown?
	  , max     // highest value of var
	  , range;  // amount of distance the 'display' shows
	int x, y, b;
	int grabbed_x, grabbed_y;

	int top, bottom, width, height;

	struct {
		unsigned int bDragging : 1;
		unsigned int bHorizontal : 1;
	}scrollflags;
	PSI_CONTROL pcTopButton, pcBottomButton;
	void (CPROC*UpdatedPos)( uintptr_t psv, int type, int current);
	uintptr_t psvUpdate;
} SCROLLBAR, *PSCROLLBAR;


//---------------------------------------------------------------------------
//	if( pc->nType == SCROLLBAR_CONTROL )
//	{
//		PSCROLLBAR psb = (PSCROLLBAR)pc;
//	}

static int CPROC RenderScrollBar( PSI_CONTROL pc )
{
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
   //lprintf( "Drawing %p", pc );
	if( psb )
	{
		Image surface = psb->surface;
		int top, bottom;
		// render top button...

		if( psb->scrollflags.bHorizontal )
		{
			psb->width = pc->rect.width - 2*pc->rect.height;
			psb->height = pc->rect.height;
		}
		else
		{
			psb->width = pc->rect.width;
			psb->height = pc->rect.height - 2*pc->rect.width;
		}


		BlatColorAlpha( surface, 0, 0, surface->width, surface->height, basecolor(pc)[SCROLLBAR_BACK] );
		if( psb->range == 0 )
         return 1;

		top = psb->current - psb->min;
		bottom = top + psb->range;

		//Log3( WIDE("top: %d bottom: %d max: %d"), top, bottom, psb->max );
	
	   if( psb->scrollflags.bHorizontal )
	   {
	   		if( psb->max )
	   		{
				top = ( psb->width * top ) / psb->max;
				bottom = ( (psb->width - 1) * bottom ) / psb->max;
			}
			else
			{
				top = 0;
				bottom = psb->width - 1;
			}
			BlatColorAlpha( surface, top, 0
								, bottom-top
								, psb->height, basecolor(pc)[NORMAL] );
	   }
	   else
	   {
	   		if( psb->max )
	   		{
				top = ( psb->height * top ) / psb->max;
				bottom = ( (psb->height - 1) * bottom ) / psb->max;
			}	
			else
			{
				top = 0;
				bottom = psb->height - 1;
			}
			BlatColorAlpha( surface, 0, top
								, psb->width
								, bottom-top, basecolor(pc)[NORMAL] );
		}
		//Log2( WIDE("top: %d bottom: %d"), top, bottom );
		psb->top = top;
		psb->bottom = bottom;

		DrawThinFrameInverted( pc );

		if( psb->scrollflags.bHorizontal )
		{
			do_vlineAlpha( surface, top, 1, psb->height-2, basecolor(pc)[HIGHLIGHT] );
			do_hlineAlpha( surface, 1, top, bottom, basecolor(pc)[HIGHLIGHT] );
			do_hlineAlpha( surface, psb->height-2, top, bottom, basecolor(pc)[SHADOW] );
			do_vlineAlpha( surface, bottom, 1, psb->height-2, basecolor(pc)[SHADOW] );
		}
		else
		{
			do_hlineAlpha( surface, top, 1, psb->width-2, basecolor(pc)[HIGHLIGHT] );
			do_vlineAlpha( surface, 1, top, bottom, basecolor(pc)[HIGHLIGHT] );
			do_vlineAlpha( surface, psb->width-2, top, bottom, basecolor(pc)[SHADOW] );
			do_hlineAlpha( surface, bottom, 1, psb->width-2, basecolor(pc)[SHADOW] );
		}

	}
	return 1;
}

//---------------------------------------------------------------------------

void MoveScrollBar( PSI_CONTROL pc, int type )
{
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
   //lprintf( "move scrollbar... %p", pc );
	if( psb )
	{
		switch( type )
		{
		case UPD_1UP:
			if( psb->current )
				psb->current--;
			break;
		case UPD_1DOWN:
			if( ( psb->current + psb->range ) < psb->max )
				psb->current++;
			break;
		case UPD_RANGEUP:
			if( psb->current > psb->range )
				psb->current -= psb->range;
			else
				psb->current = 0;
			break;
		case UPD_RANGEDOWN:
			psb->current += psb->range;
			if( ( psb->current + psb->range ) > psb->max )
            psb->current = psb->max - psb->range;
			break;
			//case UPD_THUMBTO:
		}
		if( psb->UpdatedPos )
			psb->UpdatedPos( psb->psvUpdate, type, psb->current );
		SmudgeCommon(pc);
	}
}


//---------------------------------------------------------------------------

static int CPROC ScrollBarMouse( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
	if( psb )
	{
		if( b & MK_SCROLL_DOWN )
		{
			MoveScrollBar( pc, UPD_1DOWN );
		}
		if( b & MK_SCROLL_UP )
		{
			MoveScrollBar( pc, UPD_1UP );
		}
		if( psb->scrollflags.bDragging )
		{
			if( !(b & MK_LBUTTON ) )
				psb->scrollflags.bDragging = FALSE;
			else
			{
				int desired_top;
				if( psb->scrollflags.bHorizontal )
				{
					// this is effectively -1 * the delta - so the scroll
					// goes 'increasing' to the right
					x -= psb->height;
					desired_top = ( psb->grabbed_x - x );
					desired_top = ( desired_top * psb->max ) / psb->width;
					if( desired_top > ( psb->max - psb->range ) )
						desired_top = psb->max - psb->range;
					else if( desired_top < psb->min )
						desired_top = psb->min;
					if( psb->current != desired_top )
					{
						psb->current = desired_top;
						MoveScrollBar( pc, UPD_THUMBTO );
					}
				}
				else
				{
					y -= psb->width;
					desired_top = ( y - psb->grabbed_y );
					desired_top = ( desired_top * psb->max ) / psb->height;
					if( desired_top > ( psb->max - psb->range ) )
						desired_top = psb->max - psb->range;
					else if( desired_top < psb->min )
						desired_top = psb->min;
					if( psb->current != desired_top )
					{
						psb->current = desired_top;
						MoveScrollBar( pc, UPD_THUMBTO );
					}
			   }
				// do something here...
			}
		}
		else if( ( b & MK_LBUTTON ) && !(psb->b & MK_LBUTTON ) )
		{
			if( psb->scrollflags.bHorizontal ) 
			{
				x -= psb->height;
				if( x < psb->top )
				{
					MoveScrollBar( pc, UPD_RANGEUP );
				} else if( x > psb->bottom )
				{
					MoveScrollBar( pc, UPD_RANGEDOWN );
				}
				else // was on the thumb...
				{
					psb->grabbed_x = x - psb->top; // top/left *shrug*
					psb->grabbed_y = y - psb->top;
					psb->scrollflags.bDragging = TRUE;
				}
			}
			else
			{
				y -= psb->width;
				if( y < psb->top )
				{
					MoveScrollBar( pc, UPD_RANGEUP );
				} else if( y > psb->bottom )
				{
					MoveScrollBar( pc, UPD_RANGEDOWN );
				}
				else // was on the thumb...
				{
					psb->grabbed_x = x - psb->top; // top/left *shrug*
	   				psb->grabbed_y = y - psb->top;
					psb->scrollflags.bDragging = TRUE;
				}
			}
		}
		psb->x = x;
		psb->y = y;
		psb->b = b;
	}	
	return 1;

}

//---------------------------------------------------------------------------

static void CPROC BottomPushed( uintptr_t psvBar, PSI_CONTROL pc )
{
   MoveScrollBar( (PSI_CONTROL)psvBar, UPD_1DOWN );
}

//---------------------------------------------------------------------------

static void CPROC DrawBottomButton( uintptr_t psv, PSI_CONTROL pc )
{
	Image surface = pc->Surface;
	if( surface )
	{
		int mx = pc->surface_rect.width/2;
		int cx = pc->surface_rect.width/4
		  , cy = pc->surface_rect.height/3;
		BlatColorAlpha( surface, 0, 0, surface->width, surface->height, basecolor(pc)[NORMAL] );
		if( IsButtonPressed( pc ) )
		{
			cx++;
			cy++;
		}
#define fline(s,x1,y1,x2,y2,c,a) do_lineAlpha(s,x1,y1,x2,y2,SetAlpha(c,a))
		fline( surface, mx, 2*cy+1, cx, cy+1, basecolor(pc)[SHADOW], 255 );
		fline( surface, mx, 2*cy+0, cx, cy+0, basecolor(pc)[SHADE], 128 );
		fline( surface, mx, 2*cy+2, cx, cy+2, basecolor(pc)[SHADE], 128 );
		fline( surface, mx, 2*cy+2, mx+(mx-cx)  , cy+2, basecolor(pc)[HIGHLIGHT], 255 );
		fline( surface, mx, 2*cy+0, mx+(mx-cx)  , cy+0, basecolor(pc)[SHADOW], 128 );
		fline( surface, mx, 2*cy+1, mx+(mx-cx)  , cy+1, basecolor(pc)[SHADE], 128 );
	}
}

//---------------------------------------------------------------------------

static void CPROC DrawRightButton( uintptr_t psv, PSI_CONTROL pc )
{
	Image surface = pc->Surface;
	if( surface )
	{
		int cx = pc->surface_rect.width/2
		  , cy = pc->surface_rect.height/2;
		BlatColorAlpha( surface, 0, 0, surface->width, surface->height, basecolor(pc)[NORMAL] );
		if( IsButtonPressed( pc ) )
			cx++;
		do_line( surface, cy - 2, cx - 3, cy + 1, cx  , basecolor(pc)[SHADE] );
		do_line( surface, cy - 2, cx - 4, cy + 2, cx  , basecolor(pc)[SHADOW] );
		do_line( surface, cy - 1, cx - 4, cy + 3, cx  , basecolor(pc)[SHADE] );
		do_line( surface, cy + 3, cx    , cy - 1, cx+4, basecolor(pc)[HIGHLIGHT] );
		do_line( surface, cy + 2, cx    , cy - 2, cx+4, basecolor(pc)[SHADE] );
		do_line( surface, cy + 1, cx    , cy - 2, cx+3, basecolor(pc)[SHADOW] );
	}
}

//---------------------------------------------------------------------------

static void CPROC TopPushed( uintptr_t psvBar, PSI_CONTROL pc )
{
	MoveScrollBar( (PSI_CONTROL)psvBar, UPD_1UP );
}

//---------------------------------------------------------------------------

static void CPROC DrawTopButton( uintptr_t psv, PSI_CONTROL pc )
{
	Image surface = pc->Surface;
	if( surface )
	{
		CDATA c;
		int mx = pc->surface_rect.width/2;
		int cx = pc->surface_rect.width/4
		  , cy = pc->surface_rect.height/3;
		BlatColorAlpha( surface, 0, 0, surface->width, surface->height, basecolor(pc)[NORMAL] );
		if( IsButtonPressed( pc ) )
			cx++;
		c = basecolor(pc)[SHADE];
		//c = SetAlpha( c, 128 );
		fline( surface, mx, cy+1, mx+(mx-cx), 2*cy+1, basecolor(pc)[SHADOW], 255 );
		fline( surface, mx, cy, mx+(mx-cx), 2*cy, basecolor(pc)[SHADE], 128 );
		fline( surface, mx, cy+2, mx+(mx-cx), 2*cy+2, basecolor(pc)[SHADE], 128 );
		fline( surface, mx, cy+2, cx, 2*cy+2, basecolor(pc)[HIGHLIGHT], 255 );
		fline( surface, mx, cy+0, cx, 2*cy+0, basecolor(pc)[SHADOW], 128 );
		fline( surface, mx, cy+1, cx, 2*cy+1, basecolor(pc)[SHADE], 128 );
	}
}

//---------------------------------------------------------------------------

static void CPROC DrawLeftButton( uintptr_t psv, PSI_CONTROL pc )
{
	Image surface = pc->Surface;
	if( surface )
	{
		int cx = pc->surface_rect.width/2
			, cy = pc->surface_rect.height/2;
      // hmm hope clearimage uses a blatalpha..
      BlatColorAlpha( surface, 0, 0, surface->width, surface->height, basecolor(pc)[NORMAL] );
		if( IsButtonPressed( pc ) )
			cx++;
		do_lineAlpha( surface, cy - 3, cx  , cy+1, cx+4, basecolor(pc)[SHADE] );
		do_lineAlpha( surface, cy - 2, cx  , cy+2, cx+4, basecolor(pc)[SHADOW] );
		do_lineAlpha( surface, cy - 1, cx  , cy+2, cx+3, basecolor(pc)[SHADE] );
		do_lineAlpha( surface, cy + 2, cx-3, cy-1, cx  , basecolor(pc)[HIGHLIGHT] );
		do_lineAlpha( surface, cy + 2, cx-4, cy-2, cx  , basecolor(pc)[SHADE] );
		do_lineAlpha( surface, cy + 1, cx-4, cy-3, cx  , basecolor(pc)[SHADOW] );
		//plot( surface, cx + 4, cy + 2, basecolor(pc)[HIGHLIGHT] );
	}
}

//---------------------------------------------------------------------------

void SetScrollParams( PSI_CONTROL pc, int min, int cur, int range, int max )
{
	int diffs = 0;
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
	if( psb )
	{
		int full_range = 0;
		if( max < min )
		{
			int tmp;
			tmp = min;
			min = max;
			max = tmp;
		}
		if( psb->range == ( psb->max - psb->min ) )
			full_range = 1;
		if( psb->min != min )
		{
			diffs++;
			psb->min = min;
		}
		if( psb->max != max )
		{
			diffs++;
			psb->max = max;
		}
		if( psb->range != range )
		{
			diffs++;
			psb->range = range;
		}
		if( diffs && full_range)
		{
			if( psb->range == ( psb->max - psb->min ) )
			{
				full_range = 1;
				diffs = 0;
			}
		}
		if( psb->current != cur)
		{
			if( !full_range )
				diffs++;
			psb->current = cur;
		}
		//Log( WIDE("Set scroll params - therefore render") );
		if( diffs )
			SmudgeCommon(pc);
	}
}

//---------------------------------------------------------------------------

PSI_CONTROL SetScrollBarAttributes( PSI_CONTROL pc, int attr )
{
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
	//lprintf( "Set scroll attributes..." );
	if( psb )
	{
		psb->attr = attr;
		if( attr & SCROLL_HORIZONTAL )
		{
			psb->scrollflags.bHorizontal = 1;
			MoveImage( psb->surface, pc->rect.height, 0 );
			ResizeImage( psb->surface
						  , psb->width = pc->rect.width - 2*pc->rect.height
						  , psb->height = pc->rect.height );
			MoveSizeCommon( psb->pcTopButton
							  , 0, 0
							  , pc->rect.height, pc->rect.height
							  );
			MoveSizeCommon( psb->pcBottomButton
							  , pc->rect.width - pc->rect.height, 0
							  , pc->rect.height, pc->rect.height
                       );
			SetButtonDrawMethod( psb->pcTopButton, DrawLeftButton, (uintptr_t)psb );
			SetButtonDrawMethod( psb->pcBottomButton, DrawRightButton, (uintptr_t)psb );
		}
		else
		{
			psb->scrollflags.bHorizontal = 0;
			MoveImage( psb->surface, 0, pc->rect.width );
			ResizeImage( psb->surface
						  , psb->width = pc->rect.width
						  , psb->height = pc->rect.height - 2*pc->rect.width );
			MoveSizeCommon( psb->pcTopButton
							  , 0, 0
							  , pc->rect.width, pc->rect.width
							  );
			MoveSizeCommon( psb->pcBottomButton
							  , 0, pc->rect.height-pc->rect.width
							  , pc->rect.width, pc->rect.width
                       );

			SetButtonDrawMethod( psb->pcBottomButton, DrawBottomButton, (uintptr_t)psb );
			SetButtonDrawMethod( psb->pcTopButton, DrawTopButton, (uintptr_t)psb );
		}
	}
   return pc;
}


//CONTROL_PROC_DEF( SCROLLBAR_CONTROL, SCROLLBAR, ScrollBar, ()  )
static int OnCreateCommon( SCROLLBAR_CONTROL_NAME )( PSI_CONTROL pc )
{
   //ARG( uint32_t, attr );
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
	if( psb )
	{
      //lprintf( "Creating %p", pc );
		psb->min = 0;
		psb->max = 1;
		psb->current = 0;
		psb->range = 1;
		{
			psb->surface = MakeSubImage( pc->Surface
												, 0, pc->rect.width
												, psb->width = pc->rect.width
												, psb->height = pc->rect.height - 2*pc->rect.width );
			psb->pcTopButton = MakePrivateControl( pc, CUSTOM_BUTTON
															 , 0, 0
															 , pc->rect.width, pc->rect.width
															 , BTN_LESS );
         SetButtonPushMethod( psb->pcTopButton, TopPushed, (uintptr_t)pc );
			psb->pcTopButton->flags.bNoFocus = TRUE;

			psb->pcBottomButton = MakePrivateControl( pc, CUSTOM_BUTTON
																 , 0, pc->rect.height-pc->rect.width
																 , pc->rect.width, pc->rect.width
																 , BTN_MORE );
         SetButtonPushMethod( psb->pcBottomButton, BottomPushed, (uintptr_t)pc );
			psb->pcBottomButton->flags.bNoFocus = TRUE;

         if( psb->width > psb->height )
				SetScrollBarAttributes( pc, SCROLL_HORIZONTAL );
         else
				SetScrollBarAttributes( pc, 0 );
		}
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
// flags may indicate - horizontal not vertical...

void SetScrollUpdateMethod( PSI_CONTROL pc
					, void (CPROC*UpdateProc)(uintptr_t psv, int type, int current)
					, uintptr_t data )
{
   ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
	if( psb )
	{
		psb->UpdatedPos = UpdateProc;
		psb->psvUpdate = data;
	}
}

static void OnSizeCommon( SCROLLBAR_CONTROL_NAME )( PSI_CONTROL pc, LOGICAL begin_sizing )
//static void CPROC ResizeScrollbar( PSI_CONTROL pc )
{
	ValidatedControlData( PSCROLLBAR, SCROLLBAR_CONTROL, psb, pc );
   //lprintf( "Resizing %p", pc );
	if( psb )
	{
		int32_t width = 15;
      //lprintf( "Resize called." );
		ScaleCoords( (PSI_CONTROL)pc, &width, NULL );
		// resize the scrollbar accordingly...
		//lprintf( WIDE( "Getting a resize on the scrollbar..." ) );
		if( psb->attr & SCROLL_HORIZONTAL )
		{
			MoveImage( psb->surface, pc->rect.height, 0 );
			ResizeImage( psb->surface
						  , psb->width = pc->rect.width - 2*pc->rect.height
						  , psb->height = pc->rect.height );
			MoveSizeCommon( psb->pcTopButton, 0, 0
							  , pc->rect.height, pc->rect.height );
			MoveSizeCommon( psb->pcBottomButton, pc->rect.width - pc->rect.height, 0
							  , pc->rect.height, pc->rect.height );
		}
		else if( psb->surface )
		{
			MoveImage( psb->surface, 0, pc->rect.width );
			ResizeImage( psb->surface
						  , psb->width = pc->rect.width
						  , psb->height = pc->rect.height - 2*pc->rect.width );
			MoveSizeCommon( psb->pcTopButton, 0, 0
							  , pc->rect.width, pc->rect.width );
			MoveSizeCommon( psb->pcBottomButton, 0, pc->rect.height-pc->rect.width
							  , pc->rect.width, pc->rect.width );
		}
 	}
}


CONTROL_REGISTRATION
scroll_bar = { SCROLLBAR_CONTROL_NAME
				 , { { 18, 18 }, sizeof( SCROLLBAR ), BORDER_NONE }
				 , NULL //ConfigureScrollBar
				 , NULL
				 , RenderScrollBar
				 , ScrollBarMouse
};
PRIORITY_PRELOAD( RegisterScrollBar,PSI_PRELOAD_PRIORITY )
{
   DoRegisterControl( &scroll_bar );
	//SimpleRegisterMethod( PSI_ROOT_REGISTRY WIDE("/control/") SCROLLBAR_CONTROL_NAME WIDE("/rtti")
	//						  , ResizeScrollbar
	//						  , WIDE("void"), WIDE("resize"), WIDE("(PSI_CONTROL)") );
}

PSI_SCROLLBAR_NAMESPACE_END

//---------------------------------------------------------------------------
