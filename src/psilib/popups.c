#define DEBUG_MENUS
#define DEBUG_DRAW_MENU

//#if !defined( WIN32 ) && !defined( _MSC_VER)
// haha - well at least under windows these menu issues can be resolved.
#ifndef _WIN32
#define DISABLE_NATIVE_POPUPS
// custom menus, in windows, with intershell transparency, falls behind
// the applicatoin surface; so only enable this as a last resort
#endif
#define CUSTOM_MENUS
//#endif
#define MENU_DRIVER_SOURCE
#include <stdhdrs.h>
#include <idle.h>
#include <sharemem.h>
//#define NO_LOGGING
#include <logging.h>
#include <sqlgetoption.h>

#include "global.h"

#include "menustruc.h"
#include "controlstruc.h"
#include <psi.h>


PSI_MENU_NAMESPACE


#define MENU_VERTPAD 5
#define MENU_HORZPAD 5

#define PutMenuStringFont PutStringFont
#define GetMenuStringSizeFontEx GetStringSizeFontEx

	static struct {
		struct {
			BIT_FIELD bCustomMenuEnable : 1;
			BIT_FIELD bDisplayBoundless : 1;
		} flags;
	} local_popup_data;

#ifdef CUSTOM_MENUS
static uint32_t last_buttons;
#endif
extern CONTROL_REGISTRATION menu;


//----------------------------------------------------------------------
//extern void GetMyInterface(void);
//----------------------------------------------------------------------

PSI_PROC( PMENU, CreatePopup )( void )
{
#ifndef PSI_SERVICE
#ifdef USE_INTERFACES
	if( !g.MyDisplayInterface )
	{
		GetMyInterface();
	}
#endif
#endif
#if defined( DISABLE_NATIVE_POPUPS )
	local_popup_data.flags.bCustomMenuEnable = 1;
#else
	local_popup_data.flags.bCustomMenuEnable = RequiresDrawAll();
	local_popup_data.flags.bDisplayBoundless = RequiresDrawAll();
#endif
#ifndef __NO_OPTIONS__
#  if WIN32
	local_popup_data.flags.bCustomMenuEnable = SACK_GetProfileIntEx( GetProgramName()
																						, "SACK/PSI/menus/Use Custom Popups"
																						, local_popup_data.flags.bCustomMenuEnable
																						, TRUE );
#  else
	local_popup_data.flags.bCustomMenuEnable = 1;
#  endif
	local_popup_data.flags.bDisplayBoundless = 1||SACK_GetProfileIntEx( GetProgramName()
																						, "SACK/PSI/menus/Do not clip to display"
																						, local_popup_data.flags.bDisplayBoundless
																						, TRUE );
#endif

	if( local_popup_data.flags.bCustomMenuEnable )
	{
		PMENU pm;
		PSI_CONTROL pc = MakeControl( NULL, menu.TypeID
											 , 0, 0
											 , 1, 1
											 , -1 );
		//= Allocate( sizeof( MENU ) );
		pm = ControlData( PMENU, pc );
		pm->image = pc;
		//								MemSet( menu, 0, sizeof( MENU ) );
		//pm->font = GetDefaultFont();
		return pm;
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
	{
		return (PMENU)CreatePopupMenu();
	}
#endif
   return 0;
}

#ifdef CUSTOM_MENUS
//----------------------------------------------------------------------
void ShowMenu( PMENU pm, int x, int y, LOGICAL bWait, PSI_CONTROL parent );
void UnshowMenu( PMENU pm );

int CalculateMenuItems( PMENU pm )
{
	PMENUITEM pmi;
	uint32_t maxwidth, totalheight, hassubmenu = 0;
	pmi = pm->items;
	maxwidth = 0;
	 totalheight = MENU_VERTPAD;
	pm->font = GetCommonFont( pm->image );
	while( pmi )
	{
		pmi->baseline = totalheight;
		if( pmi->flags.bSubMenu )
		{
			hassubmenu |= 1;
		}
		if( pmi->flags.bSeparator )
		{
			pmi->height = ( GetFontHeight( pm->font ) ) / 2;
		}
		else if( pmi->flags.bHasText )
		{
			uint32_t width;
			GetMenuStringSizeFontEx( pmi->data.text.text
			                        , pmi->data.text.textlen
			                        , &width, &pmi->height
			                        , pm->font );
			if( width > maxwidth )
			    maxwidth = width;
		}
		else if( pmi->flags.bOwnerDraw )
		{
			  DrawPopupItemProc dmip = pmi->data.owner.DrawPopupItem;
			  DRAWPOPUPITEM dpi;
  			  dpi.psvUser = pmi->value.userdata;
			  dpi.flags.selected = pmi->flags.bSelected;
			  dpi.flags.checked = pmi->flags.bChecked;
			  if( dmip )
				  dmip( TRUE, &dpi );
			  pmi->height = dpi.measure.height;
			  pmi->width = dpi.measure.width;
			  if( dpi.measure.width > maxwidth )
				  maxwidth = dpi.measure.width;
			// call owner proc to get the item size...
			//pmi->height += GetFontHeight( pm->font ) + 2;
		}
		totalheight += pmi->height + 2;
		pmi->offset = CHECK_WIDTH;
		pmi = pmi->next;
	}
	pm->height = (int16_t)(totalheight + MENU_VERTPAD);
	pm->width = (int16_t)(maxwidth
		+ 2*MENU_HORZPAD
		+ CHECK_WIDTH
							  + (hassubmenu?SUB_WIDTH:0));
#ifdef DEBUG_MENUS
	 lprintf( "Resize menu to %d,%d", pm->width, pm->height );
#endif
	 SizeCommon( pm->image, pm->width //+ FrameBorderX( pm->image->BorderType )
				  , pm->height //+ FrameBorderY( pm->image, pm->image->BorderType, NULL )
				  );
	pm->flags.changed = 0;
	return 1;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void DestroyItem( PMENUITEM pmi )
{
	if( pmi->flags.bHasText )
	{
		if( pmi->data.text.text )
			Release( pmi->data.text.text );
	}

	if( pmi->flags.bSubMenu )
		DestroyPopup( pmi->value.menu );

	*pmi->me = pmi->next;
	if( pmi->next )
		pmi->next->me = pmi->me;
	Release( pmi );
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
static void AddItem( PMENU pm, PMENUITEM pmi )
{
	PMENUITEM pmiafter;
	pmiafter = pm->items;
	while( pmiafter && pmiafter->next )
		pmiafter = pmiafter->next;
	if( !pmiafter )
	{
		pm->items = pmi;
		pmi->me = &pm->items;
	}
	else
	{
	  pmiafter->next = pmi;
	  pmi->me = &pmiafter->next;
	}
   pm->flags.changed = 1;
	}
*/

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RenderItem( PMENU pm, PMENUITEM pmi )
{
   Image surface;
	if( !pm || !pmi || !pm->surface || !pm->image )
		 return;
	surface = GetControlSurface( pm->image );
	if( pmi->flags.bSeparator )
	{
		int height = pmi->height;
		int baseline = pmi->baseline + (height/2);
		do_hline( surface, baseline-1, 6, pm->width - 12, basecolor(pm->image)[SHADOW] );
		do_hline( surface, baseline  , 5, pm->width - 10, basecolor(pm->image)[SHADE] );
		do_hline( surface, baseline+1, 6, pm->width - 12, basecolor(pm->image)[HIGHLIGHT] );
		//pmi->baseline += height;
		//pmi->height = height;
	}
	else if( pmi->flags.bOwnerDraw )
	{
		  DrawPopupItemProc dmip = pmi->data.owner.DrawPopupItem;
		  DRAWPOPUPITEM dpi;
		  dpi.psvUser = pmi->value.userdata;
		  dpi.flags.selected = pmi->flags.bSelected;
		  dpi.flags.checked = pmi->flags.bChecked;
		  dpi.draw.height = pmi->height;
		  dpi.draw.width = pmi->width;
		  dpi.draw.image = surface;
		  dpi.draw.x = pmi->offset;
		  dpi.draw.y = pmi->baseline;
		  if( dmip )
			  dmip( FALSE, &dpi );
		//pmi->baseline += GetFontHeight( pm->font ) + 2;
	}
	else if( pmi->flags.bHasText )
	{
		BlatColor( surface
			      , 0, pmi->baseline
			      , pm->width, pmi->height + 2
			      , basecolor(pm->image)[NORMAL] );
		PutMenuStringFont( surface
			              , MENU_HORZPAD + CHECK_WIDTH, pmi->baseline + 1
			              , basecolor(pm->image)[TEXTCOLOR], 0
			              , pmi->data.text.text, pm->font );
		pmi->height = GetFontHeight( pm->font );
		if( pmi->flags.bSubMenu )
		{
			int width = pm->width - 4;
			            do_line( surface
			                    , width - 8, pmi->baseline + 1 + 2
			                    , width, pmi->baseline + 1 + (pmi->height/2)
			                    , basecolor(pm->image)[SHADOW] );
			            do_line( surface
			                    , width, pmi->baseline + 1 + (pmi->height/2)
			                    , width - 8, pmi->baseline + 1 + (pmi->height - 2)
			                    , basecolor(pm->image)[HIGHLIGHT] );
			do_line( surface
			                    , width - 8, pmi->baseline + 1 + 2
			                    , width - 8, pmi->baseline + 1 + ( pmi->height - 2 )
			                    , basecolor(pm->image)[SHADE] );
		  }
#ifdef DEBUG_DRAW_MENU
		  lprintf( "Item is %s",pmi->flags.bChecked?"checked":"unchecked" );
#endif
		if( pmi->flags.bChecked )
		{
			do_line( surface
			        , 3, pmi->baseline + 1
			        , 13, pmi->baseline + 11
			        , Color( 0, 0, 0 ) );
			do_line( surface
			        , 3, pmi->baseline + 11
			        , 13, pmi->baseline + 1
			        , Color( 0, 0, 0 ) );
		  }
		  else
		  {
			do_line( surface
			        , 3, pmi->baseline + 1
			        , 13, pmi->baseline + 11
			        , basecolor(pm->image)[NORMAL] );
			do_line( surface
			        , 3, pmi->baseline + 11
			        , 13, pmi->baseline + 1
						 , basecolor(pm->image)[NORMAL]);
		  }
	 }
	 //if( pmi->flags.bSelected )
	 //{
	 //   do_hline( pm->surface, pmi->baseline, 0, pm->width, basecolor(pm->image)[SHADOW] );
	 //   do_hline( pm->surface, pmi->baseline+pmi->height+2, 0, pm->width, basecolor(pm->image)[HIGHLIGHT] );
	 //}
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void RenderUnselect( PMENU pm, PMENUITEM pmi )
{
	do_hline( pm->surface, pmi->baseline, 0, pm->width, basecolor(pm->image)[NORMAL] );
	do_hline( pm->surface, pmi->baseline+pmi->height+2, 0, pm->width, basecolor(pm->image)[NORMAL] );
#ifdef DEBUG_DRAW_MENU
	lprintf( "Render unselect" );
#endif
	//SmudgeCommon( pm->image );
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void RenderSelect( PMENU pm, PMENUITEM pmi )
{
	do_hline( pm->surface, pmi->baseline, 0, pm->width-1, basecolor(pm->image)[SHADOW] );
	do_hline( pm->surface, pmi->baseline+pmi->height+2, 0, pm->width-1, basecolor(pm->image)[HIGHLIGHT] );
	do_vline( pm->surface, 0, pmi->baseline, pmi->baseline + pmi->height + 2, basecolor(pm->image)[SHADE] );
	do_vline( pm->surface, pm->width-1, pmi->baseline, pmi->baseline + pmi->height + 2, basecolor(pm->image)[HIGHLIGHT] );
#ifdef DEBUG_DRAW_MENU
	lprintf( "Render select" );
#endif
	//SmudgeCommon( pm->image );
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static int CPROC RenderItems( PSI_CONTROL pc )
{
	//uintptr_t psv;
	//PRENDERER display;
	ValidatedControlData( PMENU, menu.TypeID, pm, pc );
	//PMENU pm = (PMENU);
	PMENUITEM pmi;
	if( pm )
	{
		lprintf( "rendering a menu popup control thing..." );
		ClearImageTo( pc->Surface, basecolor(pm->image)[NORMAL] );
		pm->surface = pc->Surface;
		if( !pm->surface )
			return 0;
		pmi = pm->items;
		while( pmi )
		{
			RenderItem( pm, pmi );
			pmi = pmi->next;
		}

		//selection has to be drawn on top of all menu items rendered
	  // otherwise all is bad.
		pmi = pm->items;
		while( pmi )
		{
			if( pmi->flags.bSelected )
			{
				RenderSelect( pm, pmi );
				break;
			}
			pmi = pmi->next;
		}
	}
	return 1;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CPROC TimerProc( uintptr_t psv )
{
	// some timer fired ... useful when a mouse is on a popup item
	// and still for a time....

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static int MenuMouse( PMENU pm, int32_t x, int32_t y, uint32_t b )
{
	PMENUITEM pmi;
#ifdef DEBUG_DRAW_MENU
	lprintf( "Mouse on menu... %d,%d %x", x, y, b );
	lprintf( "menu is %d by %d and has %p", pm->width, pm->height, pm->child );
#endif
	pmi = pm->items;
	if( x < 0 || y < 0 ||
		x > pm->width || y > pm->height )
	{
		PMENU parent = pm->parent;
		lprintf( "on menu.");
		if( parent )
		{
			//lprintf( "Menu has a parent... check to see if it's still there." );
			MenuMouse( parent
						, pm->display.x - parent->display.x + x
						, pm->display.y - parent->display.y + y
						, b );
			last_buttons = b;
			lprintf( "passed mouse to parent" );
			return TRUE;
		}
		else
		{
#ifdef DEBUG_DRAW_MENU
			//lprintf( "%08lx %08lx", b, last_buttons );
#endif
			if( MAKE_FIRSTBUTTON( b, last_buttons ) )
			{
#ifdef DEBUG_DRAW_MENU
				lprintf( "Click outside menu- close menu." );
#endif
				UnshowMenu( pm );
			}
			last_buttons = b;
			// but also check parent's to see if the mouse is on them...
			return FALSE;
		}
	}


	while( pmi )
	{
		lprintf( "Scanning for which item this is... %p", pmi );
		if( y >= pmi->baseline &&
			 y <= (pmi->baseline + (int)pmi->height) )
		{
			lprintf( "Finding item which has %d in it... starts at %d?", y, pmi->baseline );
			if( pmi->flags.bSeparator )
			{
				if( pm->selected )
				{
					if( pm->selected->flags.bSubMenu )
					{
						UnshowMenu( pm->selected->value.menu );
						pm->flags.bSubmenuOpen = 0;
					}
					//RenderUnselect( pm, pm->selected );
					lprintf( "Smudge unselected");
					SmudgeCommon( pm->image );
					pm->selected->flags.bSelected = FALSE;
				}
				pm->selected = NULL;
				pmi = NULL;
				break;
			}
			if( pmi->flags.bSelected )
			{
				lprintf( "Selected item is already set?");
				break;
			}
			else
			{
				if( pm->selected )
				{
					if( pm->selected->flags.bSubMenu )
					{
						UnshowMenu( pm->selected->value.menu );
						pm->flags.bSubmenuOpen = 0;
					}
					//RenderUnselect( pm, pm->selected );
					pm->selected->flags.bSelected = FALSE;
				}

				//RenderSelect( pm, pmi );
				lprintf( "Smudge selected");
				SmudgeCommon( pm->image );
#ifdef DEBUG_DRAW_MENU
				lprintf( "New selected %p", pmi );
#endif
				pmi->flags.bSelected = TRUE;
				pm->selected = pmi;
				if( pmi->flags.bSubMenu && pmi->value.menu )
				{
					pm->child = pmi->value.menu;
					pmi->value.menu->parent = pm;
					// release ownership so  that show mmeenu can  grrab it
					// is okay now - since we'll be owning our parent's mouse anyhow
					pm->flags.bSubmenuOpen = 1;
					ShowMenu( pmi->value.menu
								 , pm->display.x + pm->width - MENU_HORZPAD
								 , pm->display.y + pmi->baseline, FALSE, NULL );
					last_buttons = b;
					return TRUE;
				}
			}
		}
		pmi = pmi->next;
	}
	if( !( pm->_b & MK_LBUTTON ) &&
		 ( b & MK_LBUTTON ) )
	{
		if( x < 0 || x >= pm->width ||
			y < 0 || y >= pm->height )
		{
			//Log2( "Aborting Menu %" _32fs " %" _32fs "", x, y );
			pm->flags.abort = TRUE;
			UnshowMenu( pm );
		}
	}
	if( !( b & MK_LBUTTON  ) &&
	   (pm->_b & MK_LBUTTON ) )
	{
		if( pmi && !pmi->flags.bSubMenu )
		{
			Log1( "Returning Selection: %08" _PTRSZVALfx "", pmi->value.ID );
			pm->selection = pmi->value.ID;
			UnshowMenu( pm );
			last_buttons = b;
			return  TRUE;
		}
	}
	pm->_x = x;
	pm->_y = y;
	pm->_b = b;
	last_buttons = b;
	return TRUE;
}

static int OnMouseCommon( "Popup Menu" )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PMENU, menu.TypeID, pm, pc );
	if( pm )
	{
		MenuMouse( pm, x, y, b );
		return 1;
	}
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void UnshowMenu( PMENU pm )
{
	PMENU passed_result = NULL;
#ifdef DEBUG_MENUS
	Log1( "Menu to unshow=%p", pm );
#endif
	if( !pm || !pm->flags.tracking )
	{
#ifdef DEBUG_MENUS
		if( pm )
			lprintf( "No pm or not tracking this menu why am I unshowing?" );
#endif
		return;
	}
#ifdef DEBUG_DRAW_MENU
	lprintf( "%p show menu (close children first)", pm );
#endif
	if( pm->child )
		UnshowMenu( pm->child );
#ifdef DEBUG_DRAW_MENU
	lprintf( "%p show menu (NOT tracking)", pm );
#endif
	pm->flags.tracking = FALSE;
	if( pm->selected )
		pm->selected->flags.bSelected = FALSE;
#ifdef DEBUG_MENUS
	lprintf( "Popup release capture" );
#endif
	CaptureCommonMouse( pm->image, 0 ); // release ownership...
	//pm->flags.abort = TRUE;
	if( pm->parent )
	{
		if( pm->flags.abort )
		{
#ifdef DEBUG_MENUS
			Log( "Aboring the parent." );
#endif
			pm->parent->flags.abort = TRUE;
		}
		//pm->flags.abort = TRUE; // if we weren't previously we still need this..
//#ifdef DEBUG_MENUS
		Log1( "Telling parent that selection is: %08" _32fx "", pm->selection );
//#endif
		if( ( ( pm->parent->selection = pm->selection ) != -1 )
			|| pm->flags.abort)
			passed_result = pm->parent;
		// no longer is there a child menu..
		pm->parent->child = NULL;
		// give ownership back to parent...
#ifdef DEBUG_MENUS
		lprintf( "Popup capture capture" );
#endif

		CaptureCommonMouse( pm->parent->image, 1 ); // grab ownership again...
	}
#ifdef DEBUG_MENUS
	else
	{
		Log1( "This menu doesn't have a parent? but this selection is %08" _PTRSZVALfx "", pm->selection );
	}
#endif
	HideControl( pm->image );
	pm->flags.showing = 0;
	//UnmakeImageFile( pm->surface );
	//pm->surface = NULL;
	//DestroyCommon( &pm->image );
	//pm->window = NULL;
	pm->surface = NULL;
	pm->parent = NULL;

	if( pm->callback ) {
		pm->callback( pm->psvParam, pm->selection );
	}
	if( passed_result )
		UnshowMenu( passed_result );
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int CPROC FocusChanged( PSI_CONTROL pc, LOGICAL bFocus )
{
	ValidatedControlData( PMENU, menu.TypeID, pm, pc );
#define LosingFocus  (!pc->flags.bFocused )
	Log1( "Losing focus callback? %08" _32fx "", bFocus );
	if( !pm )
	{
		// if bFocus - refuse, if losefocus, accept.
#ifdef DEBUG_MENUS
		lprintf( "focus, but not on a menu?" );
#endif
		return !bFocus;
	}
	if( !pm->flags.tracking )
	{
#ifdef DEBUG_MENUS
		lprintf( "Not tracking this menu anymore - focus is irrelavant." );
#endif
		return !bFocus;
	}
	if( bFocus )
	{
		PSI_CONTROL child;
#ifdef DEBUG_MENUS
		Log1( "Menu selection = %" _PTRSZVALfs "", pm->selection );
#endif
		if( pm->selection != -1 || pm->flags.abort )
		{
			if( pm->parent && pm->flags.abort )
				pm->parent->flags.abort = TRUE;
#ifdef DEBUG_MENUS
			Log1( "menu = %p Removing newly focused menu...", pm );
#endif
			UnshowMenu( pm );
			return TRUE;
		}
		//if( pm->image == LosingFocus )
		//{
		//	Log( "This menu is about ot get focus? but it lost it but..." );
		//	return;
		//}
 		//if( pm->parent && ( pm->parent->image == LosingFocus )
			 // hmm can't check this yet since loss is issued and then
			 // set - on a windows platform anyhow...
			 /*HasFocus( pm->parent->image )*/
		//     )
		//{
		//	return; // not really losing focus...
		//}
		child = pc;
		while( child )
		{
			Log2( "Check  to see if child is gaining %p losing=%d ",
					 child, LosingFocus );
		 // actually LosingFocus is GainingFocus...
			if( child )
				break;
			child = child->child;
		}
		if( !child )
		{
			Log( "Truly lost focus and children are not gaining." );
			pm->flags.abort = TRUE;
			UnshowMenu( pm );
		}
	}
	else
	{
		if( pm && pm->selection != -1 )
		{
			Log( "Returning updated selection..." );
			UnshowMenu( pm );
			return TRUE;
		}
		else
		{
			if( !pm->flags.bSubmenuOpen )
			{
#ifdef DEBUG_MENUS
				lprintf( "Closing menu - lost focus, not to a child." );
#endif
				pm->flags.abort = 1;
				UnshowMenu( pm );
			}
		}
	}
	return TRUE;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CONTROL_REGISTRATION menu = { "Popup Menu"
									 , { { 94, 120 }, sizeof( MENU ), BORDER_NORMAL }
									 , NULL // init menu... bare control - I guess attach a device
									 , NULL
			                , RenderItems
									 , NULL//MenuMouse
									 , NULL // key
									 , NULL // destroy
									 , NULL // prop_page
									 , NULL // apply_page
									 , NULL // save
									 , NULL // AddedControl
									 , NULL // caption changed
			                , FocusChanged // focuschange
};

PRIORITY_PRELOAD( RegisterMenuPopup, PSI_PRELOAD_PRIORITY )
{
	DoRegisterControl( &menu );
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ShowMenu( PMENU pm, int x, int y, LOGICAL bWait, PSI_CONTROL parent )
{
	int32_t cx, cy;
	int32_t dx, dy;
	PSI_CONTROL display_parent;
	if( pm->flags.showing )
	{
		lprintf( "Can only show one instance of a popup at a time." );
		return;
	}
#ifdef DEBUG_MENUS
	lprintf( "%p show menu (tracking)", pm );
#endif
	pm->flags.tracking = TRUE;
	pm->selection = -1;
	pm->flags.abort = FALSE;
	pm->selected = NULL; // nothing selected on initial draw...
#ifdef DEBUG_MENUS
	Log2( "ShowMenu %d,%d", x, y );
#endif
	if( pm->flags.changed )
	{
#ifdef DEBUG_MENUS
		lprintf( "Recalculate items" );
#endif
		CalculateMenuItems( pm );
	}
	if( !local_popup_data.flags.bDisplayBoundless )
	{
		GetDisplaySize( (uint32_t*)&cx, (uint32_t*)&cy );
		if( ( x + pm->width + FrameBorderX( pm->image, BORDER_NORMAL) ) >= cx )
		{
			if( pm->parent )
			{
				dx = pm->parent->display.x - pm->width;
			}
			else
			{
				dx = cx - ( pm->width  + FrameBorderX( pm->image, BORDER_NORMAL) );
			}
		}
		else
			dx = x;

		if( ( y + pm->height  + FrameBorderY( pm->image, BORDER_NORMAL, NULL) ) >= cy )
		{
			dy = cy - ( pm->height + FrameBorderY( pm->image, BORDER_NORMAL, NULL) );
		}
		else
			dy = y;
		pm->display.x = (uint16_t)dx;
		pm->display.y = (uint16_t)dy;
	}
	else
	{
		 pm->display.x = x;
		 pm->display.y = y;
	}
	MoveCommon( pm->image, pm->display.x, pm->display.y );
	if( pm->parent )
		display_parent = pm->parent->image;
	else
		display_parent = parent;

	pm->surface = pm->image->Surface;
#ifdef DEBUG_MENUS
	lprintf( "do the reveal to compliemnt the hide? %p %p", pm->image, display_parent );
#endif
	//RevealCommon( pm->image );
	DisplayFrameOver( pm->image, display_parent );
#ifdef DEBUG_MENUS
	lprintf( "Popup capture capture" );
#endif
	CaptureCommonMouse( pm->image, 1 ); // grab ownership again...

	//MoveCommon( pm->image, pm->display.x, pm->display.y );
	pm->flags.showing = 1;

	// well should loop and wait for responces... guess I need
	// to invent a MouseHandler
	if( bWait )
	{
		while( pm->selection == -1 && !pm->flags.abort && pm->flags.showing )
		{
			if( Idle() < 0 ) // not a thread with work...
			{
				WakeableSleep( SLEEP_FOREVER );
			}
		}
		if( pm->flags.showing )
			UnshowMenu( pm );
	}
}

//----------------------------------------------------------------------
#endif

// int DeleteMenu(...)
PSI_PROC( PMENUITEM, DeletePopupItem )( PMENU pm, uintptr_t dwID, uint32_t state )
{
	if( local_popup_data.flags.bCustomMenuEnable )
	{
	PMENUITEM pmi;
	pmi = pm->items;
	if( state & MF_BYPOSITION )
	{
		while( pmi && dwID )
		{
			pmi = pmi->next;
		 dwID--;
		}
		if( pmi )
		 DestroyItem( pmi );
	}
	else
	{
		while( pmi )
		{
			if( !pmi->flags.bSeparator )
			{
				if( pmi->value.ID == dwID )
				{
					DestroyItem( pmi );
					break;
				}
			}
			pmi = pmi->next;
		}
	}
   pm->flags.changed = 1;
	return 0;
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
		return (PMENUITEM)(uintptr_t)DeleteMenu( (HMENU)pm, dwID, state );
#else
   return 0;
#endif
}

//----------------------------------------------------------------------

PSI_PROC( void, ResetPopup)( PMENU pm )
{
	if( local_popup_data.flags.bCustomMenuEnable )
	{
	if( !pm )
		return;

	if( pm->flags.tracking )
	{
		pm->flags.abort = TRUE;
		UnshowMenu( pm );
	}
	while( pm->items )
		DestroyItem( pm->items );
	}
}

//----------------------------------------------------------------------

// void DestroyMenu( HMENU );
PSI_PROC( void, DestroyPopup )( PMENU pm )
{
	if( local_popup_data.flags.bCustomMenuEnable )
	{
		if( !pm ) return;
		ResetPopup( pm );
		Release( pm );
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
		DestroyMenu( (HMENU)pm );
#else
	return;
#endif
}

//----------------------------------------------------------------------
//HMENU GetSubMenu( HMENU, int nMenu );
PSI_PROC( void *, GetPopupData )( PMENU pm, int item )
{
	if( local_popup_data.flags.bCustomMenuEnable )
	{
		PMENUITEM pmi;
		if( !pm || item < 0 )
			return 0;
		pmi = pm->items;
		while( pmi && item--)
			pmi = pmi->next;
		if( pmi->flags.bSubMenu )
			return pmi->value.menu;
		return NULL;
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
	{
		return GetSubMenu( (HMENU)pm, item );
	}
#else
   return NULL;
#endif
}

//----------------------------------------------------------------------
//int AppendMenu( HMENU, type, dwID, pData )
PSI_PROC( PMENUITEM, AppendPopupItem )( PMENU pm, int type, uintptr_t dwID, CPOINTER pData )
{
	if( local_popup_data.flags.bCustomMenuEnable )
	{
	PMENUITEM pmi;
	pmi = (PMENUITEM)Allocate( sizeof( MENUITEM ) );
   MemSet( pmi, 0, sizeof( MENUITEM ) );
	if( type & MF_SEPARATOR )
	{
	  pmi->flags.bSeparator = TRUE;
	}
	else if( type & MF_OWNERDRAW )
	{
	  	pmi->flags.bOwnerDraw = TRUE;
		pmi->data.owner.DrawPopupItem = (DrawPopupItemProc)pData;
	  	pmi->value.userdata = dwID;
	}
	else
	{
		if( pData )
		{
		 	int len;
			pmi->flags.bHasText = TRUE;
			pmi->data.text.text = NewArray( TEXTCHAR, ( len = (int)( pmi->data.text.textlen = StrLen( (TEXTSTR)pData ) ) + 1));
			StrCpyEx( pmi->data.text.text, (CTEXTSTR)pData, len );
		}
		if( type & MF_POPUP )
		{
			pmi->flags.bSubMenu = TRUE;
			pmi->value.menu = (PMENU)dwID;
		}
		else
		{
			pmi->value.ID = dwID;
		}

	}
	if( type & MF_CHECKED )
	  pmi->flags.bChecked = TRUE;
	else
	  pmi->flags.bChecked = FALSE;

	{
		PMENUITEM last = pm->items;
		while( last && last->next )
			last = last->next;
		if( !last )
		{
			pm->items = pmi;
		 pmi->me = &pm->items;
		}
		else
		{
			last->next = pmi;
		 pmi->me = &last->next;
		}
	  //lprintf( "and mark changed..." );
		pm->flags.changed = TRUE;
	}
   //lprintf( "Added new menu item..." );
	return pmi;
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
	{
	return (PMENUITEM)(uintptr_t)AppendMenu( (HMENU)pm, type, dwID,
#ifdef __cplusplus
			                    // override type cast here...
										  (TEXTCHAR const*)
#endif
										  pData );
	}
#endif
   return 0;
}

//----------------------------------------------------------------------
//int CheckMenuItem( HMENU, dwID, state )
PSI_PROC( PMENUITEM, CheckPopupItem )( PMENU pm, uintptr_t dwID, uint32_t state )
{
	if( local_popup_data.flags.bCustomMenuEnable )
	{
		PMENUITEM pmi = pm->items;
		if( !pm )
			return NULL;
#ifdef DEBUG_MENUS
		lprintf( "updating an item %" _32f " to %" _32fx, dwID, state );
#endif
	if( state & MF_BYPOSITION )
	{
		while( dwID-- && pmi )
			pmi = pmi->next;
	}
	else
	{
		while( pmi )
		{
			if( !pmi->flags.bSeparator )
			{
				if( pmi->value.ID == dwID )
				{
#ifdef DEBUG_MENUS
					lprintf( "Found item" );
#endif
					break;
				}
			}
		 pmi = pmi->next;
		}
	}
	if( pmi )
	{
		if( state & MF_CHECKED )
		{
#ifdef DEBUG_MENUS
			lprintf( "checked" );
#endif
			pmi->flags.bChecked = 1;
		}
		else
		{
#ifdef DEBUG_MENUS
			lprintf( "unchecked" );
#endif
			pmi->flags.bChecked = 0;
		}
				 if( pm->image )
				 {
#ifdef DEBUG_MENUS
					 lprintf( "update item..." );
#endif
					 RenderItem( pm, pmi );
				 }
	}
	return pmi;
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
		return (PMENUITEM)(uintptr_t)CheckMenuItem( (HMENU)pm, dwID, state );
#else
   return 0;
#endif
}

//----------------------------------------------------------------------

struct trackerParams {
	PMENU hMenuSub; PSI_CONTROL parent; void (*callback)(uintptr_t psv, int menuStatus); uintptr_t psvParam;
};
static uintptr_t CPROC OldTrackerPopup( PTHREAD thread ) {
	uintptr_t param = GetThreadParam( thread );
	struct trackerParams* params = (struct trackerParams*)param;
	int result = TrackPopup( params->hMenuSub, params->parent );
	if( params->callback )
		params->callback( params->psvParam, result );
	Release( params );
	return 0;
}

PSI_PROC( void, TrackPopup_v2 )( PMENU hMenuSub, PSI_CONTROL parent, void (*callback)( uintptr_t psv, int menuStatus ), uintptr_t psvParam ) {
	if( local_popup_data.flags.bCustomMenuEnable )
	{

		int32_t x, y;
		if( !hMenuSub )
		{
			//Log( "Invalid menu handle." );
			callback( psvParam, -1 );
			return;
		}
		hMenuSub->callback = callback;
		hMenuSub->psvParam = psvParam;
		//GetMousePosition( &x, &y );
		GetMouseState( &x, &y, &last_buttons );
#ifdef DEBUG_MENUS
		lprintf( "Mouse position: %" _32fs ", %" _32fs " %p is the menu", x, y, hMenuSub );
#endif
		if( hMenuSub->flags.showing || hMenuSub->flags.tracking )
		{
#ifdef DEBUG_MENUS
			if( hMenuSub->flags.showing )
				Log( "Already showing menu..." );
			if( hMenuSub->flags.tracking )
				Log( "Already tracking the menu..." );
#endif
			callback( psvParam, -1 );
			return;
		}
		hMenuSub->parent = NULL;
		hMenuSub->child = NULL;
		ShowMenu( hMenuSub, x, y, FALSE, parent );
		//selection = (int)hMenuSub->selection;
#ifdef DEBUG_MENUS
		lprintf( "Track popup return selection:%p %d", hMenuSub, hMenuSub->selection );
#endif
	}
	else {
		struct trackerParams* params = New( struct trackerParams );
		params->hMenuSub = hMenuSub;
		params->parent = parent;
		params->callback = callback;
		params->psvParam = psvParam;
		ThreadTo( OldTrackerPopup, (uintptr_t)params );
	}
}


PSI_PROC( int, TrackPopup )( PMENU hMenuSub, PSI_CONTROL parent )
{
	lprintf( "Parent:%p", parent );
	//lprintf( "TRACK POPUP IS DEPRECAED, USE CALLBACK VERSION INSTAEAD" );
	if( local_popup_data.flags.bCustomMenuEnable )
	{
		int32_t x, y;
		int selection;
		if( !hMenuSub )
		{
			Log( "Invalid menu handle." );
			return -1;
		}
		//GetMousePosition( &x, &y );
		GetMouseState( &x, &y, &last_buttons );
#ifdef DEBUG_MENUS
		lprintf( "Mouse position: %" _32fs ", %" _32fs " %p is the menu", x, y, hMenuSub );
#endif
		if( hMenuSub->flags.showing || hMenuSub->flags.tracking )
		{
#ifdef DEBUG_MENUS
			if( hMenuSub->flags.showing )
				Log( "Already showing menu..." );
			if( hMenuSub->flags.tracking )
				Log( "Already tracking the menu..." );
#endif
			return -1;
		}
		hMenuSub->parent = NULL;
		hMenuSub->child = NULL;
		ShowMenu( hMenuSub, x, y, TRUE, parent );
#ifdef DEBUG_MENUS
		lprintf( "Result of selection is: %d", hMenuSub->selection );
#endif
		selection = (int)hMenuSub->selection;
#ifdef DEBUG_MENUS
		Log1( "Track popup return selection: %d", selection );
#endif
		return selection;
	}
#if !defined( DISABLE_NATIVE_POPUPS )
	else
	{
		// this member shall be external... we need
		// knowledge of the porition of control which
		// is invoking this popup....
		HWND hWndLastFocus = GetActiveWindow();
		PRENDERER r = GetFrameRenderer( parent );
		int nCmd;
		POINT p;
#ifdef WIN32
		if( r )
			hWndLastFocus = (HWND)GetNativeHandle( r );
#endif
		GetCursorPos( &p );
		/*
		 #define TPM_CENTERALIGN	4
		 #define TPM_LEFTALIGN	0
		 #define TPM_RIGHTALIGN	8
		 #define TPM_LEFTBUTTON	0
		 #define TPM_RIGHTBUTTON	2
		 #define TPM_HORIZONTAL	0
		 #define TPM_VERTICAL	64
		 #define TPM_VCENTERALIGN 16
		 #define TPM_BOTTOMALIGN 32
		 */
#ifndef TPM_TOPALIGN
#define TPM_TOPALIGN 0
#endif
#ifndef TPM_NONOTIFY
#define TPM_NONOTIFY 128
#endif
#ifndef TPM_RETURNCMD
#define TPM_RETURNCMD 256
#endif

#ifdef UNDER_CE
#define TPM_RIGHTBUTTON 0
#endif

		nCmd = TrackPopupMenu( (HMENU)hMenuSub, TPM_CENTERALIGN
									 | TPM_TOPALIGN
									 | TPM_RETURNCMD
									 | TPM_RIGHTBUTTON
									 | TPM_NONOTIFY
									,p.x
									,p.y
									,0
									, hWndLastFocus
									, NULL);
		if( !nCmd ) {
			lprintf( "Canceled or was an error: %d", GetLastError() );
		}
		// hWndController	//("This line must be handled with a good window handle...\n") )
		if( nCmd == 0 ) // In my world.... 0 means in process, -1 is selected nothing, so we have to translate no selection
			nCmd = -1;
		return nCmd;
	}
#endif
   return -1;
}
PSI_MENU_NAMESPACE_END


//--------------------------------------------
