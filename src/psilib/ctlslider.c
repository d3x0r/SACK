#include "controlstruc.h"

#include <psi.h>

PSI_SLIDER_NAMESPACE
//------------------------------------------------------------------------------
//---------------------------------------------------------------------------

typedef struct slider_tag
{
	struct {
		int bHorizontal:1; // vertical if not horizontal
		int bDragging:1; // clicked and held...
	}flags;
	int min, max, current;	
	int _x, _y, _b; // prior button state;
	void (CPROC*SliderUpdated)( PTRSZVAL psvUser, PSI_CONTROL pc, int val );
   PTRSZVAL psvUser;
} SLIDER, *PSLIDER;


//---------------------------------------------------------------------------

static int OnDrawCommon( SLIDER_CONTROL_NAME )( PSI_CONTROL pc )
//int CPROC _DrawSlider( PSI_CONTROL pc )
{
	ValidatedControlData( PSLIDER, SLIDER_CONTROL, ps, pc );
	if( ps )
	{
      //BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolor(pc)[NORMAL] );
		//ClearImageTo( pc->Surface, basecolor(pc)[NORMAL] );
		if( ps->flags.bHorizontal )
		{
			int midy;
			int fromx, tox;
			int caretx;
			int caretw, careth;
			careth = pc->surface_rect.height - 2;
			caretw = careth / 2;
			midy = pc->surface_rect.height / 2;
			fromx = 2+(caretw/2);
			tox = pc->surface_rect.width - (2+(caretw/2));
			do_line( pc->Surface, fromx+1, midy-1, tox-1, midy-1, basecolor(pc)[SHADOW] );
			do_line( pc->Surface, fromx, midy, tox, midy, basecolor(pc)[SHADE] );
			do_line( pc->Surface, fromx+1, midy+1, tox-1, midy+1, basecolor(pc)[HIGHLIGHT] );
			{
				int x;
				caretx = fromx + ( ps->current - ps->min ) * ( tox - fromx ) / ( ps->max - ps->min );
				for( x = -caretw/2; x <= caretw/2; x++ )
				{
					int y, c;
					if( x < 0 )
						y = -x;
					else
						y = x;
					if( x == -caretw/2 )
						c = basecolor(pc)[HIGHLIGHT];
					else if( x == caretw/2 )
						c = basecolor(pc)[SHADOW];
					else if( x == caretw/2 - 1 )
						c = basecolor(pc)[SHADE];
					else
						c = basecolor(pc)[NORMAL];
					do_line( pc->Surface, x + caretx, 1 + y, caretx + x, ( careth - 1 ) - y, c );
				}
				do_line( pc->Surface, caretx, 1,
						  caretx + caretw/2 - 1,  caretw/2, basecolor(pc)[SHADE] );
				do_line( pc->Surface, caretx - caretw/2, 1 + caretw/2
						 , caretx           , 1, basecolor(pc)[HIGHLIGHT] );
				do_line( pc->Surface, caretx - caretw/2, careth - 1 - caretw/2
								     , caretx, careth - 1, basecolor(pc)[SHADE] );
				do_line( pc->Surface, caretx, careth - 1
						 , caretx + caretw/2, careth - 1 - caretw/2, basecolor(pc)[SHADOW] );
				do_line( pc->Surface, caretx, careth - 2
						 , caretx + caretw/2 - 1, careth - 1 - caretw/2, basecolor(pc)[SHADE] );
			}
		}
		else
		{
			int midx;
			int fromy, toy, carety, careth, caretw;
			careth = pc->surface_rect.width - 2;
			caretw = careth / 2;
			midx = pc->surface_rect.width /2;
			fromy = 2 + (caretw/2);
			toy = pc->surface_rect.height - (2 + (caretw/2));
			do_line( pc->Surface, midx-1, fromy+1, midx-1, toy-1, basecolor(pc)[SHADOW] );
			do_line( pc->Surface, midx, fromy, midx, toy, basecolor(pc)[SHADE] );
			do_line( pc->Surface, midx+1, fromy+1, midx+1, toy-1, basecolor(pc)[HIGHLIGHT] );
			{
				int x;
				carety = fromy + ( ps->current - ps->min ) * ( toy - fromy ) / ( ps->max - ps->min );
				for( x = -caretw/2; x <= caretw/2; x++ )
				{
					int y, c;
					if( x < 0 )
						y = -x;
					else
						y = x;
					if( x == -caretw/2 )
						c = basecolor(pc)[HIGHLIGHT];
					else if( x == caretw/2 )
						c = basecolor(pc)[SHADOW];
					else if( x == caretw/2 - 1 )
						c = basecolor(pc)[SHADE];
					else
						c = basecolor(pc)[NORMAL];
					do_inv_line( pc->Surface, x + carety, 1 + y, carety + x, ( careth - 1 ) - y, c );
				}
				do_inv_line( pc->Surface, carety, 1,
								carety + caretw/2 - 1,  caretw/2, basecolor(pc)[SHADE] );
				do_inv_line( pc->Surface, carety - caretw/2, 1 + caretw/2
							  , carety           , 1, basecolor(pc)[HIGHLIGHT] );
				do_inv_line( pc->Surface, carety - caretw/2, careth - 1 - caretw/2
							  , carety, careth - 1, basecolor(pc)[SHADE] );
				do_inv_line( pc->Surface, carety, careth - 1
							  , carety + caretw/2, careth - 1 - caretw/2, basecolor(pc)[SHADOW] );
				do_inv_line( pc->Surface, carety, careth - 2
							  , carety + caretw/2 - 1, careth - 1 - caretw/2, basecolor(pc)[SHADE] );
			}
		}
	}
   return 1;
}

//---------------------------------------------------------------------------

static int OnMouseCommon( SLIDER_CONTROL_NAME )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
//int CPROC _SliderMouse( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	PSLIDER ps = ControlData( PSLIDER, pc );
	int pos, span;
	// see if the x/y is on the slider tab...
	if( b & MK_LBUTTON )
	{
		if( ps->flags.bDragging ) // already selected, update to this pos...
		{
			int from, to, newcur;
			if( ps->flags.bHorizontal )
			{
				span = ( pc->surface_rect.height - 2 ) / 4;
				from = 2 + span;
				to = pc->surface_rect.width - (2 + span);
				pos = from + ( ps->current - ps->min ) * ( to - from ) / ( ps->max - ps->min );
				newcur = ( ( x - from ) * (ps->max-ps->min)/(to-from) ) + ps->min;
			}
			else
			{
				span = ( pc->surface_rect.width - 2 ) / 4;
				from = 2 + span;
				to = pc->surface_rect.height - (2 + span);
				pos = from + ( ps->current - ps->min ) * ( to - from ) / ( ps->max - ps->min );
				newcur = ( ( y - from ) * (ps->max-ps->min)/(to-from) ) + ps->min;
				
         }
			if( newcur != ps->current )
			{
				ps->current = newcur;
				if( ps->current < ps->min )
					ps->current = ps->min;
				if( ps->current > ps->max )
					ps->current = ps->max;
				if( ps->SliderUpdated )
					ps->SliderUpdated( ps->psvUser, pc, ps->current );
				SmudgeCommon( pc );
			}
		}
		else if( !(ps->_b & MK_LBUTTON ) )
		{
			int from, to, c;
			if( ps->flags.bHorizontal )
			{
				span = ( pc->surface_rect.height - 2 ) / 4;
				from = 2 + span;
				to = pc->surface_rect.width - (2 + span);
				pos = from + ( ps->current - ps->min ) * ( to - from ) / ( ps->max - ps->min );
				c = x;
			}
			else
			{
				int from, to;
				span = ( pc->surface_rect.width - 2 ) / 4;
				from = 2 + span;
				to = pc->surface_rect.height - (2 + span);
				pos = from + ( ps->current - ps->min ) * ( to - from ) / ( ps->max - ps->min );
				c = y;
			}
			if( c > ( pos + span ) )
			{
				int step;
				step = ( ps->max - ps->min ) / 10;
				ps->current += step;
				if( ps->current > ps->max )
					ps->current = ps->max;
				if( ps->SliderUpdated )
					ps->SliderUpdated( ps->psvUser, pc, ps->current );
            SmudgeCommon( pc );
			}
			else if( c < (pos - span ) )
			{
				int step;
				step = ( ps->max - ps->min ) / 10;
				ps->current -= step;
				if( ps->current < ps->min )
					ps->current = ps->min;
            //UpdateCommon(pc);
				if( ps->SliderUpdated )
					ps->SliderUpdated( ps->psvUser, pc, ps->current );
            SmudgeCommon( pc );
			}
			else // on span of control...
			{
				ps->flags.bDragging = TRUE;
				ps->_x = x;
				ps->_y = y;
			}
			if( 1 )
			{
			}
		}
	}
	else
	{
		ps->flags.bDragging = FALSE;
	}

	ps->_b = b;
	return 1;
}

//---------------------------------------------------------------------------

PSI_CONTROL SetSliderOptions( PSI_CONTROL pc, int attr )
{
	ValidatedControlData( PSLIDER, SLIDER_CONTROL, ps, pc );
	if( ps )
	{
		if( attr & SLIDER_HORIZ )
			ps->flags.bHorizontal = 1;
		else
			ps->flags.bHorizontal = 0;
      SmudgeCommon( pc );
	}
    return pc;
}

PSI_CONTROL SetSliderUpdateHandler( PSI_CONTROL pc, SliderUpdateProc SliderUpdated, PTRSZVAL psvUser )
{
	ValidatedControlData( PSLIDER, SLIDER_CONTROL, ps, pc );
	if( ps )
	{
		ps->SliderUpdated = SliderUpdated;
		ps->psvUser = psvUser;
	}
   return pc;
}



#undef MakeSlider
static int OnCreateCommon( SLIDER_CONTROL_NAME )( PSI_CONTROL pc )
//CONTROL_PROC_DEF( SLIDER_CONTROL, SLIDER, Slider, (void)/* (CPROC*SliderUpdated)(PTRSZVAL psv, PSI_CONTROL pc, int val), PTRSZVAL psv)*/ )
{
	ValidatedControlData( PSLIDER, SLIDER_CONTROL, ps, pc );
	if( ps )
	{
		SetCommonTransparent( pc, TRUE );
		ps->min = 0;
		ps->max = 100;
		ps->current = 50;
		//if( args )
		{
			//FP_ARG( void CPROC, SliderUpdated,(PTRSZVAL psv, PSI_CONTROL pc, int val));
			//ARG( PTRSZVAL, psv );
         //ARG( _32, attr );
			//ps->SliderUpdated = SliderUpdated;
			//ps->psvUser = psv;
			//if( attr & SLIDER_HORIZ )
         //   ps->flags.bHorizontal = 1;
		}
      return TRUE;
	}
   return FALSE;
}

//---------------------------------------------------------------------------

void SetSliderValues( PSI_CONTROL pc, int min, int current, int max )
{	
	if( pc )
	{
		ValidatedControlData( PSLIDER, SLIDER_CONTROL, ps, pc );
		if( ps )
		{
			ps->min = min;
			if( current > max )
				current = max;
			if( current < min ) 
				current = min;
			ps->current = current;
			ps->max = max;
			if( ps->SliderUpdated )
				ps->SliderUpdated( ps->psvUser, pc, ps->current );
			SmudgeCommon(pc);
		}
	}
}

CONTROL_REGISTRATION
slider_control = { SLIDER_CONTROL_NAME
					  , { { 140, 20 }, sizeof( SLIDER ), BORDER_NONE }
					  , NULL //InitSlider
					  , NULL
					  , NULL //_DrawSlider
					  , NULL //_SliderMouse
};

PRIORITY_PRELOAD( RegisterSlider, PSI_PRELOAD_PRIORITY )
{
   DoRegisterControl( &slider_control );
}

PSI_SLIDER_NAMESPACE_END

// $Log: ctlslider.c,v $
// Revision 1.22  2005/03/30 11:36:38  panther
// Remove a lot of debugging messages...
//
// Revision 1.21  2005/03/22 12:41:58  panther
// Wow this transparency thing is going to rock! :) It was much closer than I had originally thought.  Need a new class of controls though to support click-masks.... oh yeah and buttons which have roundable scaleable edged based off of a dot/circle
//
// Revision 1.20  2004/12/14 14:40:23  panther
// Fix slider control to handle the attr flags again for direction.  Fixed font selector dialog to close coorectly, update between controls correctly, etc
//
// Revision 1.19  2004/12/13 11:26:53  panther
// Minor protection on update common, also fix registration of image to not have a destroy function... hmm maybe should though in order to release the image attached.
//
// Revision 1.18  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.5  2004/10/08 01:21:17  d3x0r
// checkpoint
//
// Revision 1.4  2004/10/07 04:37:16  d3x0r
// Okay palette and listbox seem to nearly work... controls draw, now about that mouse... looks like my prior way of cheating is harder to step away from than I thought.
//
// Revision 1.3  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.2  2004/09/27 20:44:28  d3x0r
// Sweeping changes only a couple modules left...
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.17  2004/09/07 07:05:46  d3x0r
// Stablized up to palette dialog, which is internal... may require recompile binary upgrade may not work.
//
// Revision 1.16  2004/09/03 14:43:48  d3x0r
// flexible frame reactions to font changes...
//
// Revision 1.15  2004/08/25 08:44:52  d3x0r
// Portability changes for MSVC... Updated projects, all projects build up to PSI, no display...
//
// Revision 1.14  2003/09/13 17:06:29  panther
// Okay - and now we use stdargs... ugly kinda but okay...
//
// Revision 1.13  2003/09/11 13:09:25  panther
// Looks like we maintained integrety while overhauling the Make/Create/Init/Config interface for controls
//
// Revision 1.12  2003/07/24 23:47:22  panther
// 3rd pass visit of CPROC(cdecl) updates for callbacks/interfaces
//
// Revision 1.11  2003/05/01 21:31:57  panther
// Cleaned up from having moved several methods into frame/control common space
//
// Revision 1.10  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
