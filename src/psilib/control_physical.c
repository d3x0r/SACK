#include <stdhdrs.h>
#include <fractions.h>
#include <interface.h>

#include "controlstruc.h"
#include <keybrd.h>
#include <psi.h>
#include "mouse.h"
#include "borders.h"
#include "resource.h"

//#define DEBUG_UPDAATE_DRAW 4
//#define DEBUG_CREATE

PSI_NAMESPACE

static LOGICAL CPROC FileDroppedOnFrame( uintptr_t psvControl, CTEXTSTR filename, int32_t x, int32_t y )
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
				if( ( x < current->rect.x ) ||
					( y < current->rect.y ) ||
					( SUS_GT( x, int32_t, ( current->rect.x + current->rect.width ) , uint32_t ) ) ||
					( SUS_GT( y, int32_t, ( current->rect.y + current->rect.height ), uint32_t ) ) )
				{
					continue;
				}
				found = FileDroppedOnFrame( (uintptr_t)current, filename
						, x - (current->rect.x )
						, y - (current->rect.y ) );
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

static void CPROC FrameClose( uintptr_t psv )
{
	PPHYSICAL_DEVICE device = (PPHYSICAL_DEVICE)psv;
	PSI_CONTROL common;
	device->pActImg = NULL;
	DeleteLink( &g.shown_frames, device->common );
	common = device->common;
	DestroyCommon( &common );
}

//---------------------------------------------------------------------------

void GetCurrentDisplaySurface( PPHYSICAL_DEVICE device ) {
	PSI_CONTROL pc = device->common;
	Image surface = pc->Surface;
	Image newsurface
		= device->pActImg ? GetDisplayImage( device->pActImg ) : MakeImageFile( pc->rect.width, pc->rect.height );
	if( pc->Window != newsurface ) {
		pc->flags.bDirty = 1;
		TransferSubImages( newsurface, pc->Window );
		if( pc->Window )
			UnmakeImageFile( pc->Window );
		pc->Window = newsurface;
	}
	if( pc->Window->width != pc->rect.width ) {
		pc->rect.width = pc->Window->width;
		pc->flags.bResizedDirty = 1;
		pc->flags.bDirty = 1;
	}
	if( pc->Window->height != pc->rect.height ) {
		pc->rect.height = pc->Window->height;
		pc->flags.bDirty = 1;
		pc->flags.bResizedDirty = 1;
	}
	if( pc->flags.bResizedDirty ) {
		PFRACTION ix, iy;
		GetCommonScale( pc, &ix, &iy );
		pc->original_rect.x = InverseScaleValue( ix, pc->rect.x );
		pc->original_rect.y = InverseScaleValue( iy, pc->rect.y );
		pc->original_rect.width = InverseScaleValue( ix, pc->rect.width - FrameBorderX( pc, pc->BorderType ) );
		pc->original_rect.height = InverseScaleValue( iy, pc->rect.height - FrameBorderY( pc, pc->BorderType, GetText( pc->caption.text ) ) );
	}
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void OnDisplaySizeChange( "PSI Controls" TARGETNAME ) ( uintptr_t psvFrame, int display, int32_t x, int32_t y, uint32_t width, uint32_t height )
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

static void CPROC FrameRedraw( uintptr_t psvFrame, PRENDERER psvSelf )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psvFrame;
	uint32_t update = 0;
	PSI_CONTROL pc;
	//lprintf( "frame %p", pf );
	pc = pf->common;
	if( !g.updateThread )
		g.updateThread = MakeThread();
	//ResetImageBuffers( pc->Window );
	if( !pc ) // might (and probalby isn't) attached to anything yet.
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			Log( "no frame... early return" );
#endif
		return;
	}
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( " ------------- BEGIN FRAME DRAW -----------------" );
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
			Log( "Redraw frame..." );
#endif
		AddUse( pc );

		if( !g.flags.always_draw && pc->flags.bTransparent && pc->flags.bFirstCleaning )
		{
			Image OldSurface;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "!!Saving old image... (on frame)" );
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
						lprintf( "------------ Restoring old image..." );
					if( g.flags.bLogDebugUpdate )
						lprintf( "Restoring orignal background... " );
#endif
					BlotImage( pc->Surface, pc->OriginalSurface, 0, 0 );
				}
		}
		//pc->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.

		// but then again...
		//update++; // what if we only moved, and the driver requires a refresh?

#ifdef __DISPLAY_NO_BUFFER__
		lprintf( "REDRAW?!" );
#endif
		// if using "displaylib"
		//if( update )
		if( g.flags.always_draw || pc->flags.bResizedDirty || pc->flags.bDirtyBorder )
		{
			//lprintf( "Recomputing border..." );
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
				if( pc->border && pc->border->hasFill )
					pc->border->drawFill = 1;
				pc->DrawBorder( pc );
				DrawFrameCaption( pc );
			}
			// probably should just invoke draw... but then we won't get marked
			// dirty - so redundant smudges wont be merged... and we'll do this all twice.
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Smudging the form... %p", pc );
#endif
		}
		pc->flags.bResizedDirty = 0;
		//SmudgeCommon( pc );
		//else
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "delete use should refresh rectangle. %p", pc );
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
				lprintf( "sending update for each dirty control...." );
#endif
			do
			{
				loops++;
				if( loops > 5 )
				{
					lprintf( "Infinite recursion in drawing??" );
					break;
				}
				updated = FALSE;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "scanning list of dirty controls..." );
#endif
				LIST_FORALL( pf->pending_dirty_controls, idx, PSI_CONTROL, pc )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( "updating dirty control %p", pc );
#endif
					updated = TRUE;
					UpdateCommonEx( pc, TRUE DBG_SRC );
					SetLink( &pf->pending_dirty_controls, idx, 0 );
				}
			}
			while( updated );
			//lprintf( "done with with receiving sent redraw" );
			pf->flags.sent_redraw = 0;
		}
		else
		{
			PSI_CONTROL pcChild;
			for( pcChild = pc->child; pcChild; pcChild = pcChild->next )
				UpdateCommonEx( pcChild, FALSE DBG_SRC );
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "trusting that the frame is already drawn to the stable buffer..." );
#endif
			UpdateDisplay( pf->pActImg );
		}
		pc->flags.bRestoring = 0;
	}
}

//---------------------------------------------------------------------------

static void CPROC FrameFocusProc( uintptr_t psvFrame, PRENDERER loss )
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
			lprintf( "Frame is not initial..." );
#endif

//#ifdef DEBUG_UPDAATE_DRAW
#ifdef DEBUG_FOCUS_STUFF
	if( g.flags.bLogDebugUpdate )
		Log1( "PSI Focus change called: %p", loss );
#endif
//#endif
	if( loss )
	{
		if( frame->pFocus && ( frame->pFocus != frame->common ) )
		{
#ifdef DEBUG_FOCUS_STUFF
			lprintf( "Dispatch to current focused control also?" );
#endif
			frame->pFocus->flags.bFocused = 0;
			if( frame->pFocus->ChangeFocus )
				frame->pFocus->ChangeFocus( frame->pFocus, FALSE );
		}
#ifdef DEBUG_FOCUS_STUFF
		lprintf( "Control lost focus. (the frame itself loses focus)" );
#endif
		pc->flags.bFocused = 0;
		if( pc->ChangeFocus )
			pc->ChangeFocus( pc, FALSE );
	}
	else
	{
		pc->flags.bFocused = 1;
#ifdef DEBUG_FOCUS_STUFF
		lprintf( "Control gains focus. (the frame itself gains focus)" );
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
			lprintf( "Dispatch to current focused control also?" );
#endif
			if( frame->pFocus->ChangeFocus )
				frame->pFocus->ChangeFocus( frame->pFocus, FALSE );
			DeleteUse( frame->pFocus );
		}
	}
	if( !pc->flags.bHidden )
	{
		// update just the caption portion?
		if( !pc->flags.bRestoring )
		{
#ifdef DEBUG_FOCUS_STUFF
			lprintf( "Updating the frame caption..." );
			lprintf( "Update portion %d,%d to %d,%d", 0, 0, pc->rect.width, pc->surface_rect.y );
			lprintf( "Updating just the caption portion to the display" );
#endif
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "updating display portion %d,%d"
						 , pc->rect.width
						 , pc->surface_rect.y );
#endif
			pc->flags.bDirtyBorder = 1; // the border will always always change on focus...
			SmudgeCommon( pc );
		}
		// and draw here...
	}
#ifdef DEBUG_UPDAATE_DRAW
	else
		if( g.flags.bLogDebugUpdate )
			lprintf( "Did not draw frame of hidden frame." );
#endif
	if( added_use )
		DeleteUse( pc );
}

//---------------------------------------------------------------------------

static int CPROC FrameKeyProc( uintptr_t psvFrame, uint32_t key )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psvFrame;
	//PFRAME pf = (PFRAME)psvFrame;
	PSI_CONTROL pc = pf->common;
	int result = 0;
	if( pc->flags.bDestroy )
		return 0;
	AddUse( pc );
	if( g.flags.bLogKeyEvents )
		lprintf( "Added use for a key %08" _32fx, key );
	{
		if( pf->EditState.flags.bActive && pf->EditState.pCurrent )
		{
			//if( pf->EditState.pCurrent->KeyProc )
			//	pf->EditState.pCurrent->KeyProc( pf->EditState.pCurrent->psvKey, key );
			AddUse( pf->EditState.pCurrent );
			if( g.flags.bLogKeyEvents )
				lprintf( "invoking control use... for %s", pc->pTypeName );
			InvokeResultingMethod( result, pf->EditState.pCurrent, _KeyProc, ( pf->EditState.pCurrent, key ) );
			if( g.flags.bLogKeyEvents )
				lprintf( "Result was %d", result );
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
				lprintf( "invoking control focus use...%p %p %s", pf, keep_focus, keep_focus->pTypeName );
			//	lprintf( "dispatch a key event to focused contro... " );
			InvokeResultingMethod( result, keep_focus, _KeyProc, ( keep_focus, key ) );
			if( g.flags.bLogKeyEvents )
				lprintf( "Result was %d", result );
			DeleteUse( keep_focus );
		}
	}
	// passed the key to the child window first...
   // if it did not process, then the frame can get a shot at it..
	if( !result && pc && pc->_KeyProc )
	{
		if( g.flags.bLogKeyEvents )
			lprintf( "Invoking control key method. %s", pc->pTypeName );
		InvokeResultingMethod( result, pc, _KeyProc, (pc,key));
		if( g.flags.bLogKeyEvents )
			lprintf( "Result was %d", result );
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
		touch_state.prior_x = (int)(( (touch1->x + touch2->x) / 2 ) / 100);
		touch_state.prior_y = (int)(( (touch1->y + touch2->y) / 2 ) / 100);
	}
	else if( touch1->flags.end_event || touch2->flags.end_event )
	{
	}
	else // touch is a motion, between down and up
	{
		//int tmpx;
		//int tmpy;
		/*
		SetPageOffsetRelative( canvas->current_page
			, touch_state.prior_x - (tmpx=(( (touch1->x + touch2->x) / 2 ) /100))
			, touch_state.prior_y - (tmpy=(( (touch1->y + touch2->y) / 2 ) /100)));
		touch_state.prior_x = tmpx;
		touch_state.prior_y = tmpy;
		*/
	}
}

static int CPROC HandleTouch( uintptr_t psv, PINPUT_POINT pTouches, int nTouches )
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

void CPROC FrameHide( uintptr_t psv )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psv;
	PSI_CONTROL pc = pf->common;
   InvokeControlHidden( pc );
}

//---------------------------------------------------------------------------

void CPROC FrameRestore( uintptr_t psv )
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
			lprintf( "Creating a device to show this control on ... %d,%d %d,%d"
					 , pc->rect.x
					 , pc->rect.y
					 , pc->rect.width
					 , pc->rect.height );
#endif
			//lprintf( "Original show - extending frame bounds..." );
			//pc->original_rect.width += FrameBorderX(pc, pc->BorderType);
			//pc->original_rect.height += FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) );
			// apply scale to rect from original...
			//lprintf( "Border X and Y is %d,%d", FrameBorderX(pc, pc->BorderType), FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) ) );
			pc->rect.width += FrameBorderX(pc, pc->BorderType);
			pc->rect.height += FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) );
			device->pActImg = OpenDisplayAboveUnderSizedAt( DISPLAY_ATTRIBUTE_LAYERED
																  , pc->rect.width
																  , pc->rect.height
																  , pc->rect.x
																  , pc->rect.y
																  , (over&&over->device)?over->device->pActImg:NULL
																 , (under&&under->device)?under->device->pActImg:NULL);
			if( device->pActImg )
			{
#ifdef WIN32
				WinShell_AcceptDroppedFiles( device->pActImg, FileDroppedOnFrame, (uintptr_t)pc );
#endif
				AddLink( &g.shown_frames, pc );
				SetRendererTitle( device->pActImg, GetText( pc->caption.text ) );
#ifdef DEBUG_CREATE
				lprintf( "Resulting with surface..." );
#endif
			}
		}
		else
		{
			// have to resize the frame then to this display...
#ifdef DEBUG_CREATE
			lprintf( "Using externally assigned render surface..." );
			lprintf( "Adjusting the frame to that size?!" );
#endif
			if( pc->rect.x && pc->rect.y )
				MoveDisplay( pActImg, pc->rect.x, pc->rect.y );

			if( pc->rect.width && pc->rect.height ) {
				pc->flags.bDirtyBorder = 1;
				SizeDisplay( pActImg, pc->rect.width, pc->rect.height );

			}  else
			{
				uint32_t width, height;
				GetDisplaySizeEx( 0, NULL, NULL, &width, &height );
				SizeCommon( pc, width, height );
			}
			device->pActImg = pActImg;
#ifdef WIN32
			WinShell_AcceptDroppedFiles( device->pActImg, FileDroppedOnFrame, (uintptr_t)pc );
#endif
			AddLink( &g.shown_frames, pc );

		}
		pc->BorderType |= BORDER_FRAME; // mark this as outer frame... as a popup we still have 'parent'
		GetCurrentDisplaySurface( device );

		// sets up the surface iamge...
		// computes it's offset based on border type and caption
		// characteristics...
		// readjusts surface (again) after adoption.
		//lprintf( "------------------- COMMON BORDER RE-SET on draw -----------------" );

		if( pc->border && pc->border->BorderImage )
			SetCommonTransparent( pc, TRUE );

		SetDrawBorder( pc );

		if( device->pActImg )
		{
			// this routine is in Mouse.c
			SetMouseHandler( device->pActImg, AltFrameMouse, (uintptr_t)device );
			SetHideHandler( device->pActImg, FrameHide, (uintptr_t)device );
			SetRestoreHandler( device->pActImg, FrameRestore, (uintptr_t)device );

			SetCloseHandler( device->pActImg, FrameClose, (uintptr_t)device );
			SetRedrawHandler( device->pActImg, FrameRedraw, (uintptr_t)device );
			SetKeyboardHandler( device->pActImg, FrameKeyProc, (uintptr_t)device );
			SetLoseFocusHandler( device->pActImg, FrameFocusProc, (uintptr_t)device );
#ifndef NO_TOUCH
			SetTouchHandler( device->pActImg, HandleTouch, (uintptr_t)device );
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
		//lprintf( "Closing physical frame device..." );
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
                                           , uint32_t BorderTypeFlags
                                           , PRENDERER pActImg )
{
	PSI_CONTROL pc = NULL;
	int32_t x, y;
	uint32_t width, height;
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

