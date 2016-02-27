#include <stdhdrs.h>
#include <fractions.h>
#include <interface.h>

#include "controlstruc.h"
#include <keybrd.h>
#include <psi.h>
#include "mouse.h"
#include "borders.h"
#include "resource.h"

#define DEBUG_UPDAATE_DRAW 4
//#define DEBUG_CREATE

PSI_NAMESPACE

static LOGICAL CPROC FileDroppedOnFrame( PTRSZVAL psvControl, CTEXTSTR filename, S_32 x, S_32 y )
{
	LOGICAL found = 0;
	PSI_CONTROL frame = (PSI_CONTROL)psvControl;
	if( frame )
	{
		x -= frame->surface_rect.x;
		y -= frame->surface_rect.y;
		{
			PSI_CONTROL current;
			for( current = frame->child; current; current = current->next )
			{
				if( current->flags.bHidden )
					continue;
				if( ( x < current->original_rect.x ) || 
					( y < current->original_rect.y ) || 
					( SUS_GT( x, S_32, ( current->original_rect.x + current->original_rect.width ) , _32 ) ) || 
					( SUS_GT( y, S_32, ( current->original_rect.y + current->original_rect.height ), _32 ) ) )
				{
					continue;
				}
				found = FileDroppedOnFrame( (PTRSZVAL)current, filename
						, x - (current->original_rect.x )
						, y - (current->original_rect.y ) );
			}
			if( !found )
			{
				InvokeResultingMethod( found, frame, AcceptDroppedFiles, (frame, filename, x, y ) );
			}
			///////////////
		}
	}
	return found;
}

//---------------------------------------------------------------------------

static void CPROC FrameClose( PTRSZVAL psv )
{
	PPHYSICAL_DEVICE device = (PPHYSICAL_DEVICE)psv;
	PSI_CONTROL common;
	device->pActImg = NULL;
	DeleteLink( &g.shown_frames, device->common );
	common = device->common;
	DestroyCommon( &common );
}

//---------------------------------------------------------------------------

void GetCurrentDisplaySurface( PPHYSICAL_DEVICE device )
{
	PSI_CONTROL pc = device->common;
	Image surface = pc->Surface;
	Image newsurface
		= device->pActImg?GetDisplayImage( device->pActImg ):MakeImageFile( pc->rect.width, pc->rect.height );
	if( pc->Window != newsurface )
	{
		pc->flags.bDirty = 1;
		TransferSubImages( newsurface, pc->Window );
		if( pc->Window )
			UnmakeImageFile( pc->Window );
		pc->Window = newsurface;
	}
	if( pc->Window->width != pc->rect.width )
	{
		pc->rect.width = pc->Window->width;
		pc->flags.bResizedDirty = 1;
		pc->flags.bDirty = 1;
	}
	if( pc->Window->height != pc->rect.height )
	{
		pc->rect.height = pc->Window->height;
		pc->flags.bDirty = 1;
		pc->flags.bResizedDirty = 1;
	}
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void OnDisplaySizeChange( WIDE("PSI Controls") _WIDE( TARGETNAME ) ) ( PTRSZVAL psvFrame, int display, S_32 x, S_32 y, _32 width, _32 height )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psvFrame;
	if( pf )
	{
		PSI_CONTROL pc = pf->common;
		//SizeCommon( pc, width, height );
		if( display )
		{
			// only applies to windows on this display... (maybe should remove this)
		}

	}
}

//---------------------------------------------------------------------------

static void CPROC FrameRedraw( PTRSZVAL psvFrame, PRENDERER psvSelf )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psvFrame;
	_32 update = 0;
	PSI_CONTROL pc;
	//lprintf( WIDE("frame %p"), pf );
	pc = pf->common;
	//ResetImageBuffers( pc->Window );
	if( !pc ) // might (and probalby isn't) attached to anything yet.
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			Log( WIDE("no frame... early return") );
#endif
		return;
	}
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( WIDE( " ------------- BEGIN FRAME DRAW -----------------" ) );
#endif

	pc->flags.bShown = 1;
	GetCurrentDisplaySurface(pf);
	if( !pc->flags.bDirty && IsDisplayRedrawForced( pf->pActImg ) )
		pc->flags.bDirty = 1;
	if( g.flags.always_draw || pc->flags.bDirty || pc->flags.bResizedDirty )
	{
		pc->flags.bDirty = 1;
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			Log( WIDE("Redraw frame...") );
#endif
		AddUse( pc );

		if( !g.flags.always_draw && pc->flags.bTransparent && pc->flags.bFirstCleaning )
		{
			Image OldSurface;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "!!Saving old image... (on frame)" ) );
#endif
			if( ( OldSurface = CopyOriginalSurface( pc, pc->OriginalSurface ) ) )
			{
				pc->OriginalSurface = OldSurface;
			}
			else
				if( pc->OriginalSurface )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "------------ Restoring old image..." ) );
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "Restoring orignal background... " ) );
#endif
					BlotImage( pc->Surface, pc->OriginalSurface, 0, 0 );
				}
		}
		//pc->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.

		// but then again...
		//update++; // what if we only moved, and the driver requires a refresh?

#ifdef __DISPLAY_NO_BUFFER__
		lprintf( WIDE("REDRAW?!") );
#endif
		// if using "displaylib"
		//if( update )
		if( g.flags.always_draw || pc->flags.bResizedDirty )
		{
			//lprintf( WIDE("Recomputing border...") );
			// fix up surface rect.
			if( pc->flags.bResizedDirty )
			{
				// recompute surface_rect
				extern void UpdateSurface( PSI_CONTROL pc );
				UpdateSurface( pc );
			}
			if( pc->DrawBorder )
			{
#ifdef DEBUG_BORDER_DRAWING
				lprintf( "Drawing border here too.." );
#endif
				DrawFrameCaption( pc );
				pc->DrawBorder( pc );
			}
			// probably should just invoke draw... but then we won't get marked
			// dirty - so redundant smudges wont be merged... and we'll do this all twice.
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "Smudging the form... %p" ), pc );
#endif
		}
		pc->flags.bResizedDirty = 0;
		//SmudgeCommon( pc );
		//else
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE("delete use should refresh rectangle. %p"), pc );
#endif

		UpdateCommonEx( pc, TRUE DBG_SRC );
		pc->flags.bRestoring = 0;
		if( !pc->flags.bDestroy )
		{
			PSI_CONTROL pc2;
			INDEX idx;
			LIST_FORALL( pf->pending_dirty_controls, idx, PSI_CONTROL, pc2 )
			{
				SetLink( &pf->pending_dirty_controls, idx, 0 );
			}
			pf->flags.sent_redraw = 0;
		}
		DeleteUse( pc );
	}
	else
	{
		if( pf->pending_dirty_controls )
		{
			PSI_CONTROL pc;
			INDEX idx;
			int loops = 0;
			LOGICAL updated;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "sending update for each dirty control...." ) );
#endif
			do
			{
				loops++;
				if( loops > 5 )
				{
					lprintf( WIDE( "Infinite recursion in drawing??" ) );
					break;
				}
				updated = FALSE;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE( "scanning list of dirty controls..." ) );
#endif
				LIST_FORALL( pf->pending_dirty_controls, idx, PSI_CONTROL, pc )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "updating dirty control %p"), pc );
#endif
					updated = TRUE;
					UpdateCommonEx( pc, TRUE DBG_SRC );
					SetLink( &pf->pending_dirty_controls, idx, 0 );
				}
			}
			while( updated );
			//lprintf( WIDE("done with with receiving sent redraw") );
			pf->flags.sent_redraw = 0;
		}
		else
		{
			PSI_CONTROL pcChild;
			for( pcChild = pc->child; pcChild; pcChild = pcChild->next )
				UpdateCommonEx( pcChild, FALSE DBG_SRC );
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "trusting that the frame is already drawn to the stable buffer..." ) );
#endif
			UpdateDisplay( pf->pActImg );
		}
		pc->flags.bRestoring = 0;
	}
}

//---------------------------------------------------------------------------

static void CPROC FrameFocusProc( PTRSZVAL psvFrame, PRENDERER loss )
{
	int added_use = 0;
	PPHYSICAL_DEVICE frame = (PPHYSICAL_DEVICE)psvFrame;
	//PFRAME frame = (PFRAME)psvFrame;
	PSI_CONTROL pc = frame->common;
	if( pc->flags.bShown )
	{
		added_use = 1;
		AddUse( pc );
	}
	GetCurrentDisplaySurface(frame);
	if( loss )
	{
		if( frame->pFocus )
			frame->pFocus->flags.bFocused = 0;
		pc->flags.bFocused = 0;
	}
	else
	{
		pc->flags.bFocused = 1;
		if( frame->pFocus && ( frame->pFocus != frame->common ) )
		{
			frame->pFocus->flags.bFocused = 1;
		}
	}
	if( pc->flags.bInitial )
	{
		// still in the middle of displaying... this is a false draw point.
		if( added_use )
			DeleteUse( pc );
		return;
	}
#ifdef DEBUG_UPDAATE_DRAW
	else
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE( "Frame is not initial..." ) );
#endif

//#ifdef DEBUG_UPDAATE_DRAW
#ifdef DEBUG_FOCUS_STUFF
	if( g.flags.bLogDebugUpdate )
		Log1( WIDE("PSI Focus change called: %p"), loss );
#endif
//#endif
	if( loss )
	{
		if( frame->pFocus && ( frame->pFocus != frame->common ) )
		{
#ifdef DEBUG_FOCUS_STUFF
			lprintf( WIDE("Dispatch to current focused control also?") );
#endif
			frame->pFocus->flags.bFocused = 0;
			if( frame->pFocus->ChangeFocus )
				frame->pFocus->ChangeFocus( frame->pFocus, FALSE );
		}
#ifdef DEBUG_FOCUS_STUFF
		lprintf( WIDE("Control lost focus. (the frame itself loses focus)") );
#endif
		pc->flags.bFocused = 0;
		if( pc->ChangeFocus )
			pc->ChangeFocus( pc, FALSE );
	}
	else
	{
		pc->flags.bFocused = 1;
#ifdef DEBUG_FOCUS_STUFF
		lprintf( WIDE("Control gains focus. (the frame itself gains focus)") );
#endif
		if( pc->ChangeFocus )
			pc->ChangeFocus( pc, TRUE );
		if( frame->pFocus && ( frame->pFocus != frame->common ) )
		{
			AddUse( frame->pFocus );
			// cause we need to unfocus it's control also
			// otherwise we get stupid cursors...
			frame->pFocus->flags.bFocused = 1;
#ifdef DEBUG_FOCUS_STUFF
			lprintf( WIDE("Dispatch to current focused control also?") );
#endif
			if( frame->pFocus->ChangeFocus )
				frame->pFocus->ChangeFocus( frame->pFocus, FALSE );
			DeleteUse( frame->pFocus );
		}
	}
	if( !pc->flags.bHidden )
	{
		if( !g.flags.always_draw && pc->DrawBorder )
		{
			pc->DrawBorder( pc );
		}
		if( !g.flags.always_draw )
			DrawFrameCaption( pc );
		else
			SmudgeCommon( pc );
		// update just the caption portion?
		if( pc->surface_rect.y && !pc->flags.bRestoring )
		{
#ifdef DEBUG_FOCUS_STUFF
			lprintf( WIDE("Updating the frame caption...") );
			lprintf( WIDE("Update portion %d,%d to %d,%d"), 0, 0, pc->rect.width, pc->surface_rect.y );
			lprintf( WIDE( "Updating just the caption portion to the display" ) );
#endif
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE("updating display portion %d,%d")
						 , pc->rect.width
						 , pc->surface_rect.y );
#endif
			UpdateDisplayPortion( frame->pActImg
									  , 0, 0
									  , pc->rect.width
									  , pc->surface_rect.y );
		}
		// and draw here...
	}
#ifdef DEBUG_UPDAATE_DRAW
	else
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE("Did not draw frame of hidden frame.") );
#endif
	if( added_use )
		DeleteUse( pc );
}

//---------------------------------------------------------------------------

static int CPROC FrameKeyProc( PTRSZVAL psvFrame, _32 key )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psvFrame;
	//PFRAME pf = (PFRAME)psvFrame;
	PSI_CONTROL pc = pf->common;
	int result = 0;
	if( pc->flags.bDestroy )
		return 0;
	AddUse( pc );
	if( g.flags.bLogKeyEvents )
		lprintf( WIDE("Added use for a key %08") _32fx, key );
	{
		if( pf->EditState.flags.bActive && pf->EditState.pCurrent )
		{
			//if( pf->EditState.pCurrent->KeyProc )
			//	pf->EditState.pCurrent->KeyProc( pf->EditState.pCurrent->psvKey, key );
			AddUse( pf->EditState.pCurrent );
			if( g.flags.bLogKeyEvents )
				lprintf( WIDE("invoking control use... for %s"), pc->pTypeName );
			InvokeResultingMethod( result, pf->EditState.pCurrent, _KeyProc, ( pf->EditState.pCurrent, key ) );
			if( g.flags.bLogKeyEvents )
				lprintf( WIDE("Result was %d"), result );
			DeleteUse( pf->EditState.pCurrent );
		}
		else if( pf->pFocus )
		{
			// pf can get deleted during the key event, the frame will be locked
			PSI_CONTROL keep_focus = pf->pFocus;
			//if( pf->pFocus->KeyProc )
			//	pf->pFocus->KeyProc( pf->pFocus->psvKey, key );
			AddUse( keep_focus );
			if( g.flags.bLogKeyEvents )
				lprintf( WIDE("invoking control focus use...%p %p %s"), pf, keep_focus, keep_focus->pTypeName );
			//	lprintf( WIDE("dispatch a key event to focused contro... ") );
			InvokeResultingMethod( result, keep_focus, _KeyProc, ( keep_focus, key ) );
			if( g.flags.bLogKeyEvents )
				lprintf( WIDE("Result was %d"), result );
			DeleteUse( keep_focus );
		}
	}
	// passed the key to the child window first...
   // if it did not process, then the frame can get a shot at it..
	if( !result && pc && pc->_KeyProc )
	{
		if( g.flags.bLogKeyEvents )
			lprintf( WIDE("Invoking control key method. %s"), pc->pTypeName );
		InvokeResultingMethod( result, pc, _KeyProc, (pc,key));
		if( g.flags.bLogKeyEvents )
			lprintf( WIDE("Result was %d"), result );
	}
	if( !result )
	{
		if( (KEY_CODE(key) == KEY_TAB) && (key & KEY_PRESSED))
		{
			//DebugBreak();
			if( !(key & (KEY_ALT_DOWN|KEY_CONTROL_DOWN)) ) // not control or alt...
			{
				if( key & KEY_SHIFT_DOWN ) // shift-tab backwards
				{
					FixFrameFocus( pf, FFF_BACKWARD );
					result = 1;
				}
				else
				{
					FixFrameFocus( pf, FFF_FORWARD );
					result = 1;
				}
			}
		}
		if( KEY_CODE(key) == KEY_ESCAPE && (key & KEY_PRESSED))
			result = InvokeDefault( pc, TRUE );
		if( KEY_CODE(key) == KEY_ENTER && (key & KEY_PRESSED))
			result = InvokeDefault( pc, FALSE );
	}
	DeleteUse( pc );
	return result;
}

//---------------------------------------------------------------------------
static int IsMeOrInMe( PSI_CONTROL isme, PSI_CONTROL pc )
{
	while( pc )
	{
		if( pc->child )
			if( IsMeOrInMe( isme, pc->child ) )
				return TRUE;
		if( isme == pc )
			return TRUE;
      pc = pc->next;
	}
   return FALSE;
}

//---------------------------------------------------------------------------

#ifndef NO_TOUCH
static struct {
	int prior_x, prior_y;
} touch_state;

static void HandleDefaultSingleTouch( PSI_CONTROL canvas, PINPUT_POINT touch1, PINPUT_POINT touch2 )
{
	if( touch1->flags.new_event || touch2->flags.new_event )
	{
		touch_state.prior_x = ( (touch1->x + touch2->x) / 2 ) / 100;
		touch_state.prior_y = ( (touch1->y + touch2->y) / 2 ) / 100;
	}
	else if( touch1->flags.end_event || touch2->flags.end_event )
	{
	}
	else // touch is a motion, between down and up
	{
		int tmpx;
		int tmpy;
		/*
		SetPageOffsetRelative( canvas->current_page
			, touch_state.prior_x - (tmpx=(( (touch1->x + touch2->x) / 2 ) /100))
			, touch_state.prior_y - (tmpy=(( (touch1->y + touch2->y) / 2 ) /100)));
		touch_state.prior_x = tmpx;
		touch_state.prior_y = tmpy;
		*/
	}
}

static int CPROC HandleTouch( PTRSZVAL psv, PINPUT_POINT pTouches, int nTouches )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psv;
	PSI_CONTROL pc = pf->common;
	if( !pc )
	{
		// maybe this was closed before it actually got the first message?
		return 0;
	}
	if( pc->flags.bDestroy )
	{
      // somehow we're being destroyed, but not everyone knows yet
		return 0;
	}
	AddUse( pc );

	if( nTouches == 2 )
	{
		HandleDefaultSingleTouch( pc, pTouches, pTouches + 1 );
		return TRUE;
	}
	return FALSE;
}
#endif

//---------------------------------------------------------------------------

void CPROC FrameHide( PTRSZVAL psv )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psv;
	PSI_CONTROL pc = pf->common;
   InvokeControlHidden( pc );
}

//---------------------------------------------------------------------------

void CPROC FrameRestore( PTRSZVAL psv )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psv;
	PSI_CONTROL pc = pf->common;
	InvokeControlRevealed( pc );
}

//---------------------------------------------------------------------------


PPHYSICAL_DEVICE OpenPhysicalDevice( PSI_CONTROL pc, PSI_CONTROL over, PRENDERER pActImg, PSI_CONTROL under )
{
	if( pc && !pc->device )
	{
		//Image surface;
		PPHYSICAL_DEVICE device = (PPHYSICAL_DEVICE)Allocate( sizeof( PHYSICAL_DEVICE ) );
		MemSet( device, 0, sizeof( PHYSICAL_DEVICE ) );
		device->common = pc;
		pc->device = device;
		device->nIDDefaultOK = BTN_OKAY;
		device->nIDDefaultCancel = BTN_CANCEL;
		//TryLoadingFrameImage();
		if( under )
			under = GetFrame( under );
		if( over )
			over = GetFrame( over );
		else
			if( pc->stack_parent )
			{
				over = pc->stack_parent;
				//DebugBreak();
				//OrphanCommon( pc );
				// some other method to save this?
				// for now I can guess...
				// pc->device->EditState.parent = parent ?
				// leave it otherwise linked into the stack of controls...
				pc->stack_parent = NULL;
			}
		if( !pActImg )
		{
#ifdef DEBUG_CREATE
			lprintf( WIDE("Creating a device to show this control on ... %d,%d %d,%d")
					 , pc->rect.x
					 , pc->rect.y
					 , pc->rect.width
					 , pc->rect.height );
#endif
			//lprintf( WIDE("Original show - extending frame bounds...") );
			pc->original_rect.width += FrameBorderX(pc, pc->BorderType);
			pc->original_rect.height += FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) );
			// apply scale to rect from original...
			//lprintf( "Border X and Y is %d,%d", FrameBorderX(pc, pc->BorderType), FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) ) );
			pc->rect.width += FrameBorderX(pc, pc->BorderType);
			pc->rect.height += FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) );
			device->pActImg = OpenDisplayAboveUnderSizedAt( 0
																  , pc->rect.width
																  , pc->rect.height
																  , pc->rect.x
																  , pc->rect.y
																  , (over&&over->device)?over->device->pActImg:NULL
																 , (under&&under->device)?under->device->pActImg:NULL);
			if( device->pActImg )
			{
#ifdef WIN32
				WinShell_AcceptDroppedFiles( device->pActImg, FileDroppedOnFrame, (PTRSZVAL)pc );
#endif
				AddLink( &g.shown_frames, pc );
				SetRendererTitle( device->pActImg, GetText( pc->caption.text ) );
#ifdef DEBUG_CREATE
				lprintf( WIDE("Resulting with surface...") );
#endif
			}
		}
		else
		{
			// have to resize the frame then to this display...
#ifdef DEBUG_CREATE
			lprintf( WIDE("Using externally assigned render surface...") );
			lprintf( WIDE("Adjusting the frame to that size?!") );
#endif
			if( pc->rect.x && pc->rect.y )
				MoveDisplay( pActImg, pc->rect.x, pc->rect.y );

			if( pc->rect.width && pc->rect.height )
				SizeDisplay( pActImg, pc->rect.width, pc->rect.height );
			else
			{
				_32 width, height;
				GetDisplaySizeEx( 0, NULL, NULL, &width, &height );
				SizeCommon( pc, width, height );
			}
			device->pActImg = pActImg;
#ifdef WIN32
			WinShell_AcceptDroppedFiles( device->pActImg, FileDroppedOnFrame, (PTRSZVAL)pc );
#endif
			AddLink( &g.shown_frames, pc );
         
		}
		pc->BorderType |= BORDER_FRAME; // mark this as outer frame... as a popup we still have 'parent'
		GetCurrentDisplaySurface( device );

		// sets up the surface iamge...
		// computes it's offset based on border type and caption
		// characteristics...
		// readjusts surface (again) after adoption.
		//lprintf( WIDE("------------------- COMMON BORDER RE-SET on draw -----------------") );

		if( pc->border && pc->border->BorderImage )
			SetCommonTransparent( pc, TRUE );

		SetDrawBorder( pc );

		if( device->pActImg )
		{
			// this routine is in Mouse.c
			SetMouseHandler( device->pActImg, AltFrameMouse, (PTRSZVAL)device );
			SetHideHandler( device->pActImg, FrameHide, (PTRSZVAL)device );
			SetRestoreHandler( device->pActImg, FrameRestore, (PTRSZVAL)device );

			SetCloseHandler( device->pActImg, FrameClose, (PTRSZVAL)device );
			SetRedrawHandler( device->pActImg, FrameRedraw, (PTRSZVAL)device );
			SetKeyboardHandler( device->pActImg, FrameKeyProc, (PTRSZVAL)device );
			SetLoseFocusHandler( device->pActImg, FrameFocusProc, (PTRSZVAL)device );
#ifndef NO_TOUCH
			SetTouchHandler( device->pActImg, HandleTouch, (PTRSZVAL)device );
#endif
			// these methods should aready be set by the creation above...
			// have to attach the mouse events to this frame...
		}
		Redraw( device->pActImg );
	}
	if( pc )
		return pc->device;
   return NULL;
}

//---------------------------------------------------------------------------

void DetachFrameFromRenderer(PSI_CONTROL pc )
{
	if( pc->device )
	{
		PPHYSICAL_DEVICE pf = pc->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pc );
		//lprintf( WIDE("Closing physical frame device...") );
		if( pf->EditState.flags.bActive )
		{
			// there may also be data in the edit state to take care of...
		}
		if( pf )
		{
			if( pf->pActImg ) SetMouseHandler( pf->pActImg, NULL, 0 );
			if( pf->pActImg ) SetRedrawHandler( pf->pActImg, NULL, 0 );
			if( pf->pActImg ) SetKeyboardHandler( pf->pActImg, NULL, 0 );
			if( pf->pActImg ) SetCloseHandler( pf->pActImg, NULL, 0 );
#ifndef NO_TOUCH
			if( pf->pActImg )
				SetTouchHandler( pf->pActImg, NULL, 0 );
#endif
			// closedevice
			OrphanSubImage( pc->Surface );
			//UnmakeImageFile( pc->Surface );
			if( pf->pActImg )
				CloseDisplay( pf->pActImg );
			// closing the main image closes all children?
			pc->Window = NULL;
			// this is a subimage of window, and as such, is invalid now.
			//pc->Surface = NULL;
			Release( pf );
			// and that's about it, eh?
		}
		pc->device = NULL;
	}
}


//---------------------------------------------------------------------------
PSI_PROC( PSI_CONTROL, AttachFrameToRenderer )( PSI_CONTROL pc, PRENDERER pActImg )
{
	OpenPhysicalDevice( pc
							, pc?pc->stack_parent:NULL
							, pActImg, NULL );
	return pc;
}

PSI_PROC( PSI_CONTROL, CreateFrameFromRenderer )( CTEXTSTR caption
                                           , _32 BorderTypeFlags
                                           , PRENDERER pActImg )
{
	PSI_CONTROL pc = NULL;
	S_32 x, y;
	_32 width, height;
#ifdef USE_INTERFACES
	GetMyInterface();
	if( !g.MyImageInterface )
		return NULL;
#endif
	GetDisplayPosition( pActImg, &x, &y, &width, &height );
	pc = MakeCaptionedControl( NULL, CONTROL_FRAME
									 , x, y, width, height
									 , 0
									 , caption
									 );
	AttachFrameToRenderer( pc, pActImg );
	SetCommonBorder( pc, BorderTypeFlags|((BorderTypeFlags & BORDER_WITHIN)?0:BORDER_FRAME) );
	return pc;
}

PSI_NAMESPACE_END

