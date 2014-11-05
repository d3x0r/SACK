#include "controlstruc.h"
#include <psi.h>

#include "mouse.h"
//#define DEBUG_FOCUS
//#define HOTSPOT_DEBUG
//#define DEBUG_ADD_DELETEUSE
//#define EDIT_MOUSE_DEBUG
#define DETAILED_MOUSE_DEBUG
#define SUPER_DETAILED_MOUSE_DEBUG
//#define DEBUG_MOUSE_SMUDGE
//---------------------------------------------------------------------------
PSI_NAMESPACE
	void UpdateSomeControls( PSI_CONTROL pfc, P_IMAGE_RECTANGLE pRect );
PSI_NAMESPACE_END

PSI_MOUSE_NAMESPACE

//---------------------------------------------------------------------------

	static void DrawHotSpotEx( PSI_CONTROL pc, P_IMAGE_POINT bias, P_IMAGE_POINT point, int hot DBG_PASS )
#define DrawHotSpot(pc,bias,point,hot) DrawHotSpotEx(pc,bias,point,hot DBG_SRC)
{
	Image surface = pc->Window;
	do_hline( surface, bias[1] + point[1] - SPOT_SIZE, bias[0] + point[0] - SPOT_SIZE, bias[0] + point[0] + SPOT_SIZE, Color( 0, 0, 0 ) );
	do_hline( surface, bias[1] + point[1] + SPOT_SIZE, bias[0] + point[0] - SPOT_SIZE, bias[0] + point[0] + SPOT_SIZE, Color( 0, 0, 0 ) );
	do_vline( surface, bias[0] + point[0] - SPOT_SIZE, bias[1] + point[1] - SPOT_SIZE, bias[1] + point[1] + SPOT_SIZE, Color( 0, 0, 0 ) );
	do_vline( surface, bias[0] + point[0] + SPOT_SIZE, bias[1] + point[1] - SPOT_SIZE, bias[1] + point[1] + SPOT_SIZE, Color( 0, 0, 0 ) );
	//_lprintf(DBG_RELAY)( "color %d %d %d %d %d %d", bias[0], bias[1], point[0], point[1], surface->width, surface->height );
	BlatColor( surface
				, bias[0] + point[0] - (SPOT_SIZE-1)
				, bias[1] + point[1] - (SPOT_SIZE-1)
				// one would fiture that this would be -2 (one for left, one for right)
				// but we have to add back in 1 for here cause we're based at here+ and here-
				, (SPOT_SIZE*2)-1, (SPOT_SIZE*2)-1
				, hot?Color( 255, 0, 0 ):Color(255,255,255) );
}

//---------------------------------------------------------------------------

void DrawHotSpotsEx( PSI_CONTROL pf, PEDIT_STATE pEditState DBG_PASS )
#define DrawHotSpots(pf,pe) DrawHotSpotsEx(pf,pe DBG_SRC)
{
	int n;
	if( !pEditState->flags.bHotSpotsActive )
		return;
#ifdef HOTSPOT_DEBUG
	lprintf( WIDE("Drawing hotspots. %d,%d"), pEditState->bias[0], pEditState->bias[1] );
#endif
	for( n = 0; n < 9; n++ )
	{
		DrawHotSpotEx( pf
					, pEditState->bias
					, pEditState->hotspot[n]
					, ( n+1 == pEditState->flags.fLocked )
					DBG_RELAY );
	}
	/* this function updates the surface of a control.... */
	UpdateFrame( pf, 0, 0, pf->surface_rect.width, pf->surface_rect.height );
	//UpdateFrame( pf
	//			, 0, 0, pf->pDevice->common->rect.width, pf->common->rect.height );
				//, pEditState->bound.position[0] + pEditState->bias[0]
				//, pEditState->bound.position[1] + pEditState->bias[1]
				//, pEditState->bound.size[0], pEditState->bound.size[1] );

}

//---------------------------------------------------------------------------

void SetupHotSpots( PEDIT_STATE pEditState )
{
	int bias_x = 0, bias_y = 0;
	PSI_CONTROL pCom;
	if( pCom = (PSI_CONTROL)pEditState->pCurrent )
	{
		do
		{
			bias_x += pCom->surface_rect.x;
			bias_y += pCom->surface_rect.y;
#ifdef HOTSPOT_DEBUG
			lprintf( WIDE("Bias is %d,%d"), bias_x, bias_y );
#endif
			if( pCom && pCom->parent )
			{
				bias_x += pCom->rect.x;
				bias_y += pCom->rect.y;
#ifdef HOTSPOT_DEBUG
				lprintf( WIDE("Bias IS %d,%d"), bias_x, bias_y );
#endif
			}
			pCom = pCom->parent;
		}
		while( pCom );
		pEditState->bias[0] = bias_x - (SPOT_SIZE);
		pEditState->bias[1] = bias_y - (SPOT_SIZE);
#ifdef HOTSPOT_DEBUG
		lprintf( WIDE("Setup hotspots for control %p (%d,%d)")
				, pEditState->pCurrent
				, pEditState->bias[0], pEditState->bias[1] );
#endif
		pEditState->hotspot[0][0] = 0;
		pEditState->hotspot[0][1] = 0;
		pEditState->hotspot[1][0] = 0 + (pEditState->pCurrent->rect.width) / 2;
		pEditState->hotspot[1][1] = 0;
		pEditState->hotspot[2][0] = 0 + (pEditState->pCurrent->rect.width);
		pEditState->hotspot[2][1] = 0;

		pEditState->hotspot[3][0] = 0;
		pEditState->hotspot[3][1] = 0 + (SPOT_SIZE + pEditState->pCurrent->rect.height) / 2;
		pEditState->hotspot[4][0] = 0 + (pEditState->pCurrent->rect.width) / 2;
		pEditState->hotspot[4][1] = 0 + (SPOT_SIZE + pEditState->pCurrent->rect.height) / 2;
		pEditState->hotspot[5][0] = 0 + (pEditState->pCurrent->rect.width);
		pEditState->hotspot[5][1] = 0 + (SPOT_SIZE + pEditState->pCurrent->rect.height) / 2;

		pEditState->hotspot[6][0] = 0;
		pEditState->hotspot[6][1] = 0 + (SPOT_SIZE + pEditState->pCurrent->rect.height) - 1;
		pEditState->hotspot[7][0] = 0 + (pEditState->pCurrent->rect.width) / 2;
		pEditState->hotspot[7][1] = 0 + (SPOT_SIZE + pEditState->pCurrent->rect.height) - 1;
		pEditState->hotspot[8][0] = 0 + (pEditState->pCurrent->rect.width);
		pEditState->hotspot[8][1] = 0 + (SPOT_SIZE + pEditState->pCurrent->rect.height) - 1;

		{
			int n;
			for( n = 0; n < 9; n++ )
			{
#ifdef HOTSPOT_DEBUG
				lprintf( WIDE("Hotspot %d is %d,%d")
						, n
						, pEditState->hotspot[n][0]
						, pEditState->hotspot[n][1] );
#endif
			}
		}
		pEditState->bound.position[0] = 0 - SPOT_SIZE;
		pEditState->bound.position[1] = 0 - SPOT_SIZE;
		pEditState->bound.size[0] = pEditState->pCurrent->rect.width + (SPOT_SIZE * 2);
		pEditState->bound.size[1] = pEditState->pCurrent->rect.height + (SPOT_SIZE * 2);
		pEditState->flags.bHotSpotsActive = 1;
	}
}

//---------------------------------------------------------------------------

static int MouseInHotSpot( PEDIT_STATE pEditState, int x, int y DBG_PASS )
#define MouseInHotSpot( pes, x, y ) MouseInHotSpot( pes, x, y DBG_SRC )
{
	int n;
	if( !pEditState->flags.bActive || !pEditState->flags.bHotSpotsActive )
		return 0;
#ifdef HOTSPOT_DEBUG
 	_xlprintf( 1 DBG_RELAY )( WIDE("Detecting mouse at %d,%d in spot..."), x, y );
  lprintf( WIDE("			mouse at %d,%d in spot..."), x, y );
#endif
	if( pEditState->flags.fLocked )
	{
		int delx = x - pEditState->_x;
		int dely = y - pEditState->_y;
#ifdef HOTSPOT_DEBUG
		lprintf( WIDE("x %d y %d"), x, y );
		lprintf( WIDE("pesx %d pesy %d"), pEditState->_x, pEditState->_y);
		lprintf( WIDE("delx %d dely %d"), delx, dely );
#endif
		if( delx > 0 )
			pEditState->delxaccum ++;
		else if( delx < 0 )
			pEditState->delxaccum--;
		if( dely > 0 )
			pEditState->delyaccum ++;
		else if( dely < 0 )
			pEditState->delyaccum--;
#ifdef HOTSPOT_DEBUG
		lprintf( WIDE("delxacc %d delyacc %d")
				, pEditState->delxaccum
				, pEditState->delyaccum );
#endif
#define LOCK_THRESHOLD 4
		if( pEditState->delxaccum <= -LOCK_THRESHOLD
			|| pEditState->delxaccum >= LOCK_THRESHOLD
			|| pEditState->delyaccum <= -LOCK_THRESHOLD
			|| pEditState->delyaccum >= LOCK_THRESHOLD )
		{
#ifdef HOTSPOT_DEBUG
			lprintf( WIDE("excessive threshold, unlock.") );
#endif
			pEditState->flags.fLocked = FALSE;
		}
		else
		{
#ifdef HOTSPOT_DEBUG
			lprintf( WIDE("remain in same spot") );
#endif
			return pEditState->flags.fLocked;
		}
	}
	for( n = 0; n < 9; n++ )
	{
		if( ( pEditState->hotspot[n][0] - SPOT_SIZE ) <= x &&
			( pEditState->hotspot[n][0] + SPOT_SIZE ) >= x &&
			( pEditState->hotspot[n][1] - SPOT_SIZE ) <= y &&
			( pEditState->hotspot[n][1] + SPOT_SIZE ) >= y )
		{
#ifdef HOTSPOT_DEBUG
			_xlprintf( 1 DBG_RELAY )( WIDE("Detected mouse at %d,%d in spot %d"), x, y, n );
#endif
			{
				pEditState->_x = pEditState->hotspot[n][0];
				pEditState->_y = pEditState->hotspot[n][1];
				pEditState->delxaccum = 0;
				pEditState->delyaccum = 0;
			}
			return n + 1;
		}
	}
	//_xlprintf( 1 DBG_RELAY )( WIDE("No spot at %d,%d..."), x, y );

	return 0;
}

//---------------------------------------------------------------------------
static int CPROC EditControlKeyProc( PSI_CONTROL pc, _32 key )
{
	PPHYSICAL_DEVICE pf = GetFrame(pc)->device;
#ifdef HOTSPOT_DEBUG
	lprintf( WIDE("Edit control key proc...") );
#endif
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pc );
	if( pf )
	{
		PEDIT_STATE pEditState = &pf->EditState;
		if( key & KEY_PRESSED )
		{
			switch( KEY_CODE( key ) )
			{
			case KEY_LEFT:
				if( !( key & KEY_SHIFT_DOWN ) )
					MoveControlRel( pEditState->pCurrent, -1, 0 );
				else
					SizeControlRel( pEditState->pCurrent, -1, 0 );
				break;
			case KEY_RIGHT:
				if( !( key & KEY_SHIFT_DOWN ) )
					MoveControlRel( pEditState->pCurrent, 1, 0 );
				else
					SizeControlRel( pEditState->pCurrent, 1, 0 );
				break;
			case KEY_DOWN:
				if( !( key & KEY_SHIFT_DOWN ) )
					MoveControlRel( pEditState->pCurrent, 0, 1 );
				else
					SizeControlRel( pEditState->pCurrent, 0, 1 );
				break;
			case KEY_UP:
				if( !( key & KEY_SHIFT_DOWN ) )
					MoveControlRel( pEditState->pCurrent, 0, -1 );
				else
					SizeControlRel( pEditState->pCurrent, 0, -1 );
				break;
			default:
//#define InvokeMethod(pc,name,args)
				if( (pEditState) && pEditState->_KeyProc )
				{
					int n;
					for( n = 0; n < pEditState->n_KeyProc; n++ )
						if( pEditState->_KeyProc[n] )
							if( pEditState->_KeyProc[n](pc,key) )
							{
								/*break*/;
								// then what?!  shouldn't we be calling the function here? :)
							}
				}

			}
		}
	}
	return 0;
}
//static int (CPROC*EditControlList)(PSI_CONTROL,_32) = { EditControlKeyProc };

//---------------------------------------------------------------------------
_MOUSE_NAMESPACE_END
void SetCommonFocus( PSI_CONTROL pc )
{
	PSI_CONTROL frame;
	if( pc && ( frame = GetFrame( pc ) ) )
	{
		PPHYSICAL_DEVICE pf = frame->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, GetFrame( pc ) );
		// don't allow THIS focus routine to focus the frame
		// as a control...
		if( pf )
			ForceDisplayFocus( pf->pActImg );
#ifdef DEBUG_FOCUS
		lprintf( WIDE("Set common focus... to control %p (%d)"), pc, pc->nType );
#endif
		if( pc && pf &&
			( pf->pFocus != pc ) ) // already is focus...
		{

			if( !pc->flags.bDisable && !pc->flags.bNoFocus )
			{
				PSI_CONTROL test;
				test = pc->parent;
				while( test )
				{
					if( test == pf->pFocus )
						break;
					test = test->parent;
				}
				// it's okay to set focus of a child of the
				// thing which currently has the focus...
				// the container does not need to (and should not)
				// lose focus
				if( test )
				{
#ifdef DEBUG_FOCUS
					lprintf( WIDE("Prior focused thing is the parent of control being focused.") );
#endif
				}
#ifdef DEBUG_FOCUS
				else
					lprintf( WIDE("Prior is not parent of control being focused...") );
#endif
				if( !test && pf->pFocus )
				{
					pf->pFocus->flags.bFocused = FALSE;
					InvokeSingleMethod( pf->pFocus, ChangeFocus, (pf->pFocus, FALSE) );
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("Clearing prior focus, telling control to draw itself...") );
#endif
#ifdef DEBUG_MOUSE_SMUDGE
					lprintf( "------------- SMUDGE OLD CONTROL THAT WAS FOCUSED...." );
#endif
					//SmudgeCommon( pf->pFocus );
				}
				// but we can transfer focus to an internal thingy...
				// might revisit uhmm something.
				pf->pFocus = pc;
				pc->flags.bFocused = TRUE;
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
				{
					lprintf( WIDE("Instruct control to draw itself...") );
					lprintf( WIDE("This should probably just refresh the border... well focus causes display phenomenoon...") );
				}
#endif
				InvokeSingleMethod( pf->pFocus, ChangeFocus, (pf->pFocus, TRUE) );
				//SmudgeCommon( pf->pFocus );
			}
#ifdef DETAILED_MOUSE_DEBUG
			else if( g.flags.bLogDetailedMouse )
			{
				lprintf( WIDE("Control is disabled or has no focus?") );
			}
#endif
		}
#ifdef DETAILED_MOUSE_DEBUG
		else if( g.flags.bLogDetailedMouse )
		{
			lprintf( WIDE("%p or %p is null or %p == %p"), pf, pc, pf, pc );
		}
#endif
	}
}

int IsControlFocused( PSI_CONTROL pc )
{
	if( pc && pc->flags.bFocused )
		return TRUE;
	return FALSE;
}
_MOUSE_NAMESPACE

//---------------------------------------------------------------------------

void AddUseEx( PSI_CONTROL pc DBG_PASS )
{
	pc->InUse++;
#ifdef DEBUG_ADD_DELETEUSE
	_xlprintf( 2 DBG_RELAY )( WIDE("(A)Use count is %d %p"), pc->InUse, pc );
	if( pc->flags.bDirty )
	{
		lprintf( "----==-=-=-=-=--==  was already dirt at add use!" );
	}
#endif
}

//---------------------------------------------------------------------------

void DeleteUseEx( PSI_CONTROL *pc DBG_PASS )
{
	if( pc && *pc )
	{
#ifdef DEBUG_ADD_DELETEUSE
		_xlprintf( 2 DBG_RELAY )( WIDE("(D)Use count is %d %p"), (*pc)->InUse, (*pc) );
#endif
		if( ((*pc)->InUse - (*pc)->NotInUse )== 1 )
		{
			if( !(*pc)->flags.bDestroy )
			{
				// do update with use still locked...
				PSI_CONTROL parent = *pc;
				//PSI_CONTROL update = NULL;
				if(0)
				while( parent && ( parent->parent && !parent->device ) /*&& (parent->flags.bTransparent || parent->flags.bDirty)*/ )
				{
					if( parent->flags.bDirty )
						parent->parent->flags.children_cleaned = 0;
					//lprintf( WIDE("Final use check - is %p dirty?!"), parent );
					//if( parent->flags.bDirty )
					//	update = parent;
					parent = parent->parent;
				}
#ifdef DEBUG_ADD_DELETEUSE
				lprintf( WIDE("Begin Update topmost parent from deleteUse: %p"), (parent) );
				//#endif
#endif
				//if( update )
				if( (!(*pc)->flags.bInitial)
					&& (*pc)->child
					&& (!(*pc)->parent)
				// && !((*pc)->flags.bRestoring)
				)
				{
					//IntelligentFrameUpdateAllDirtyControls( (*pc) DBG_RELAY );
					//UpdateCommonEx( (*pc), FALSE DBG_RELAY );
				}
				//UpdateCommonEx( parent, FALSE DBG_RELAY );
			}
		}
		if( !(--(*pc)->InUse) && !(*pc)->InWait)
		{
			if( (*pc)->flags.bDestroy )
				DestroyCommon( pc );
		}
	}
	else
	{
		_xlprintf( 2 DBG_RELAY )( WIDE("Use delete for invalid control!") );
	}
}

//---------------------------------------------------------------------------

void ReleaseCommonUse( PSI_CONTROL pc )
{
// set fake decrement to this ...
// we can force the render code of deleteuse to run
	pc->NotInUse = pc->InUse;
	// add one use, delete use consumes it.
	pc->InUse++;
	DeleteUseEx( &pc DBG_SRC );
	// the fake not-in-use counter is not useful anymore
	// we've already forced deleteuse to do the draw work (if not delete)
	pc->NotInUse = 0;
}


//---------------------------------------------------------------------------
#ifdef __cplusplus 
}
#endif
void OwnCommonMouse( PSI_CONTROL pc, int bOwn )
{
	PSI_CONTROL pfc = GetFrame( pc );
	PPHYSICAL_DEVICE pf = pfc->device;
//#ifdef DETAILED_MOUSE_DEBUG
//	if( g.flags.bLogDetailedMouse )
		lprintf( WIDE( "Own Common Mouse called on %p %s" ), pc, bOwn?WIDE( "OWN" ):WIDE( "release" ) );
//#endif
	if( pf )
	{
		if( ( !bOwn && ( pf->flags.bCurrentOwns ) )
			|| ( bOwn && !( pf->flags.bCurrentOwns ) ) )
		{
			lprintf( WIDE( "Own Common Mouse performed on %p %s" ), pc, bOwn?WIDE( "OWN" ):WIDE( "release" ) );
			pf->pCurrent = pc;
			pf->flags.bCurrentOwns = bOwn;
			pf->flags.bApplicationOwned = bOwn;
			OwnMouse( pf->pActImg, bOwn );
		}
		else if( bOwn && ( pf->pCurrent != pc ) )
		{
			lprintf( "overlapping mouse own" );
			OwnCommonMouse( pf->pCurrent, FALSE );
			OwnCommonMouse( pc, TRUE );
		}
	}
}

#ifdef __cplusplus 
namespace _mouse {
#endif
//---------------------------------------------------------------------------

void AddWaitEx( PSI_CONTROL pc DBG_PASS )
{
	pc->InWait++;
#ifdef DEBUG_ADD_DELETEUSE
	_xlprintf( 2 DBG_RELAY )( WIDE("Use count is %d %p"), pc->InWait, pc );
#endif
}

//---------------------------------------------------------------------------

void DeleteWaitEx( PSI_CONTROL *pc DBG_PASS )
{
	if( pc && *pc )
	{
#ifdef DEBUG_ADD_DELETEUSE
		_xlprintf( 2 DBG_RELAY )( WIDE("Use count is %d %p"), (*pc)->InWait, (*pc) );
#endif
		if( !(--(*pc)->InWait) && !(*pc)->InUse )
		{
			if( (*pc)->flags.bDestroy )
				DestroyCommon( pc );
		}
	}
	else
	{
		_xlprintf( 2 DBG_RELAY )( WIDE("Use delete for invalid control!") );
	}
}


//---------------------------------------------------------------------------

static PSI_CONTROL FindControl( PSI_CONTROL pfc, PSI_CONTROL pc, int x, int y, int b )
{
	// input x, y relative to the current frame bias on input.
	// _x, _y are the saved input values to this function
	// they are not modified. __x, __y are used as a temporary save spot
	int _x = x, __x;
	int _y = y, __y;
	struct
	{
		_32 was_in_surface : 1;
	} flags;
	IMAGE_POINT _bias;
	PPHYSICAL_DEVICE pf = pfc->device;
	flags.was_in_surface = pf->CurrentBias.flags.bias_is_surface;
	_bias[0] = pf->CurrentBias.x;
	_bias[1] = pf->CurrentBias.y;
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pfc );
//	lprintf( WIDE("So the input to FindControl x, y look like they are biased to THIS control? %d,%d"), x, y );

	if( !pc )
	{
#ifdef SUPER_DETAILED_MOUSE_DEBUG
		if( g.flags.bLogSuperDetailedMouse )
			lprintf( WIDE("No child control start found.") );
#endif
		return NULL;
	}
	for( ; pc; pc = pc->next )
	{
		if( pc->flags.bHidden )
		{
#ifdef SUPER_DETAILED_MOUSE_DEBUG
			if( g.flags.bLogSuperDetailedMouse )
				lprintf( WIDE("Skipping hidden control...") );
#endif
			continue;
		}
		x = _x - pc->rect.x;
		y = _y - pc->rect.y;
#ifdef SUPER_DETAILED_MOUSE_DEBUG
		if( g.flags.bLogSuperDetailedMouse )
			lprintf( WIDE("%d,%d (%d,%d) Control %p rect = (%d,%d)  (%d,%d)")
					, x, y
					, _x, _y
					, pc,pc->rect.x, pc->rect.y
					, pc->rect.width, pc->rect.height );
#endif

		if( (S_64)x >= 0 &&
			(S_64)x < pc->rect.width &&
			(S_64)y >= 0 &&
			(S_64)y < pc->rect.height )
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("Setting bias to input %d,%d  ... plus %d,%d")
						, _bias[0], _bias[1]
						, pc->rect.x, pc->rect.y );
#endif
			pf->CurrentBias.x = _bias[0] + pc->rect.x;
			pf->CurrentBias.y = _bias[1] + pc->rect.y;
			pf->CurrentBias.flags.bias_is_surface = 0;
			__x = x;
			__y = y;
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("Okay coordinate within a control...") );
#endif
			x -= pc->surface_rect.x;
			y -= pc->surface_rect.y;
			if( (S_64)x >= 0 &&
				(S_64)x < ( pc->surface_rect.width ) &&
				(S_64)y >= 0 &&
				(S_64)y < ( pc->surface_rect.height ) )
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Adding the surface of the thing to the bias... %d,%d"), pc->surface_rect.x, pc->surface_rect.y );
#endif
				pf->CurrentBias.flags.bias_is_surface = 1;
				pf->CurrentBias.x += pc->surface_rect.x;
				pf->CurrentBias.y += pc->surface_rect.y;
				// there may be a control contained within
				// this control which actually gets mouse....
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Coordinate within surface of control even...") );
#endif
				if( pc->child )
				{
					PSI_CONTROL child;
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("To find a child control... at %d,%d "), x, y );
#endif
					child = FindControl( pfc, pc->child
											, x, y
											, b );
					if( child )
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							lprintf( WIDE("Returning a child control...") );
#endif
						return child;
					}
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("So this control is THE control, no child.") );
#endif
				}
				// otherwise, this control is now the primary owner...
				// -or- this control has become current....
			}
			else
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Coordinate NOT within the surface...but definatly in control") );
#endif
				//continue;
			}

			// at this point, what do we know?
			// x, y are within pfc
			// pf->CurrentBias == offset of the control's surface
			// we were called since pf->flags.bCurrentOwns was not set
			// pfc may be equal to pf...
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				Log( WIDE("Now setting pCurrent on frame...") );
#endif
			//bias[0] = _bias[0];
			//bias[1] = _bias[1];
			pf->pCurrent = pc;

			if( MAKE_FIRSTBUTTON( b, pf->_b ) )
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Setting focus...") );
#endif
				SetCommonFocus( pc );
				OwnCommonMouse( pc, TRUE );
				//pf->flags.bCurrentOwns = TRUE;
			}
			// and now - what about editing controls?
			// now that we have the mouse within a current
			// then that current range actually extends somewhat beyond
			// the actual surface of the control...
			// and we do not change active controls until surpassing THAT bound
			{
				PSI_CONTROL parent_non_private = pc;
				while( parent_non_private && ( parent_non_private->BorderType & BORDER_NO_EXTRA_INIT ) )
				{
					parent_non_private = parent_non_private->parent;
				}

			if( pf->EditState.flags.bActive )
			{
				// YES do ugly thing here ... there is a 'pc' which is the common that was
				// found containing the mouse.  the 'pc' for this subsection is the top control
				// which does not have a private flag... could mass replace this code
				// to use 'parent_non_private' instead of 'pc' but I was lazy and clever tonight.
				PSI_CONTROL pc = parent_non_private;
#ifdef HOTSPOT_DEBUG
				lprintf( WIDE("edit state active... do some stuff... "));
#endif
				if( pc->nType &&  // don't set focus to the frame...
					(PSI_CONTROL)pc != pf->EditState.pCurrent )
				{
					pf->EditState.pCurrent = (PSI_CONTROL)pc;
					SetupHotSpots( &pf->EditState );
					// setup which spot we're in now.
#ifdef HOTSPOT_DEBUG
					lprintf( WIDE("New hot spot check... %d,%d"), _x, _y );
#endif
					pf->EditState.flags.fLocked = MouseInHotSpot( &pf->EditState
																			 , _x
																			 , _y );
#ifdef EDIT_MOUSE_DEBUG
					Log( WIDE("And now we draw new spots...") );
#endif
					// hotspots have to be drawn on the frame, so they have to be refreshed on the frame
					SmudgeCommon( pf->common );

					{
						// override the current key proc so arrows work...
#ifdef HOTSPOT_DEBUG
						lprintf( WIDE("overriding key method.") );
#endif
						pf->EditState.n_KeyProc = pf->EditState.pCurrent->n_KeyProc;
						pf->EditState._KeyProc = pf->EditState.pCurrent->_KeyProc;
						pf->EditState.pCurrent->n_KeyProc = 1;
						{
							static __KeyProc *tmp;
							tmp = (__KeyProc *)Allocate( sizeof( __KeyProc ) );
							tmp[0] = EditControlKeyProc;
							pf->EditState.pCurrent->_KeyProc = tmp;
						}
					}
				}
			}
			}
			// at this point x, y will be relative to the
			// surface of the thing.  Always...
			// if the coordinates are < 0 or > width
				// then they're still on the control but are on the
			// rectangle (border) and not within the control...
			// so.... given that let's see - perhaps for frames this
			/// will be allowed, but the range checking for controls
			// will fail of beyond the borders..
			// If it's a frame, then the x, y are relative
			// to the whole window thing.
			break;
		}
	}
	return pc;
}

//---------------------------------------------------------------------------

int CPROC FirstFrameMouse( PPHYSICAL_DEVICE pf, S_32 x, S_32 y, _32 b, int bCallOriginal )
{
	PSI_CONTROL pc = pf->common;
	extern void DumpFrameContents( PSI_CONTROL pc );
	int result = 0;
	int caption_height = CaptionHeight( pc, GetText( pc->caption.text ) );
	int frame_height = FrameBorderYOfs( pc, pc->BorderType, NULL );
	//DumpFrameContents( pc );
	//lprintf( WIDE("FFM") );
#if defined DETAILED_MOUSE_DEBUG //|| defined EDIT_MOUSE_DEBUG
	if( g.flags.bLogDetailedMouse )
		lprintf( WIDE("Mouse Event: %p %d %d %d"), pf, x, y, b );
#endif
	// need to handle - click to raise window in display list...
#if 0
	if( !pc->flags.bFocused )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("Frame not focused? don't process dragging, or edit state filtering...") );
#endif
		InvokeResultingMethod( result, pc, _MouseMethod, (pc
																		, x - pc->surface_rect.x
																		, y - pc->surface_rect.y
																		, b ) );
		return result;
	}
#endif
	if( ( pf->_b & MK_LBUTTON ) &&
		( b & MK_LBUTTON ) )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("Continued down on left button") );
#endif
		//lprintf( WIDE("continued mouse down...") );
		if( pc->pressed_caption_button )
		{
			if( ( x < pc->pressed_caption_button->offset )
				|| ( y >= ( frame_height + caption_height ) )
				|| ( y < frame_height ) 
				|| ( x > ( pc->pressed_caption_button->offset + ( caption_height ) ) )
				)
			{
				lprintf( WIDE("outside of button...") );
				if( pc->pressed_caption_button->is_pressed )
				{
					pc->pressed_caption_button->is_pressed = 0;
					DrawFrameCaption( pc );
					UpdateDisplayPortion( pf->pActImg
												, pc->surface_rect.x, 0
												, pc->surface_rect.width
												, pc->surface_rect.y );
				}
			}
			else
			{
				lprintf( WIDE("inside of button...") );
				if( !pc->pressed_caption_button->is_pressed )
				{
					pc->pressed_caption_button->is_pressed = 1;
					DrawFrameCaption( pc );
					UpdateDisplayPortion( pf->pActImg
												, pc->surface_rect.x, 0
												, pc->surface_rect.width
												, pc->surface_rect.y );
				}
			}
		}
		else if( pf->flags.bDragging )
		{
			S_32 winx, winy;
			int dx;
			int dy;
			GetDisplayPosition( pf->pActImg, &winx, &winy, NULL, NULL );
			dx = x - pf->drag_x + winx;
			dy = y - pf->drag_y + winy;
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
			{
				lprintf( WIDE("Still dragging frame at %d,%d"), winx, winy );
				lprintf( WIDE("moving to %d,%d(%d,%d)"), dx, dy, x - pf->drag_x, y - pf->drag_y );
			}
#endif
			MoveDisplay( pf->pActImg, dx, dy );
			if( pc->Move )
				pc->Move( pc, FALSE );
			return TRUE;
		}
		else if( pf->flags.bSizing )
		{
			int dx = x - pf->_x;
			int dy = y - pf->_y;
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("sizing by %d,%d"), dx, dy );
#endif
			pc->flags.bResizedDirty = 1;
			if( pf->flags.bSizing_left )
			{
				if( pf->flags.bSizing_top )
				{
					MoveSizeDisplayRel( pf->pActImg, dx, dy, -dx, -dy );
				}
				else if( pf->flags.bSizing_bottom )
				{
					pf->_y = y;
					MoveSizeDisplayRel( pf->pActImg, dx, 0, -dx, dy );
				}
				else
				{
					MoveSizeDisplayRel( pf->pActImg, dx, 0, -dx, 0 );
				}
			}
			else if( pf->flags.bSizing_right )
			{
				pf->_x = x; // update prior coord to 'here'
				if( pf->flags.bSizing_top )
				{
					MoveSizeDisplayRel( pf->pActImg, 0, dy, dx, -dy );
				}
				else if( pf->flags.bSizing_bottom )
				{
					pf->_y = y;
					SizeDisplayRel( pf->pActImg, dx, dy );
				}
				else
				{
					SizeDisplayRel( pf->pActImg, dx, 0 );
				}
			}
			else
			{
				if( pf->flags.bSizing_top )
				{
					MoveSizeDisplayRel( pf->pActImg, 0, dy, 0, -dy );
				}
				else if( pf->flags.bSizing_bottom )
				{
					pf->_y = y;
					SizeDisplayRel( pf->pActImg, 0, dy );
				}
				else
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						Log( WIDE("Sizing without a border!") );
#endif
					pf->flags.bSizing = 0;
				}
			}
			//DebugBreak();
			//SmudgeCommon( pc );
			return TRUE;
		}
		else
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("Not moving, not sizing, pass mouse through...") );
#endif
			InvokeResultingMethod( result, pc, _MouseMethod, (pc
														, x - pc->surface_rect.x
														, y - pc->surface_rect.y
														, b ) );
		 }
	}
	else if( !( b & MK_LBUTTON ) )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("left is not down...") );
#endif
		OwnCommonMouse( pc, 0 );
		if( ( x > pc->surface_rect.x && x < ( pc->surface_rect.x + pc->surface_rect.width ) )
			&& ( y > pc->surface_rect.y && y < ( pc->surface_rect.y + pc->surface_rect.height ) ) )
		{
			InvokeResultingMethod( result, pc, _MouseMethod, (pc
													 , x - pc->surface_rect.x
													 , y - pc->surface_rect.y
													 , b ) );
		}
		else
		{
			INDEX idx;
			PCAPTION_BUTTON button;
			if( ( x < ( pc->surface_rect.x + pc->surface_rect.width ) ) // left side edge
				&& ( y > frame_height ) // left side edge
				&& ( y < ( caption_height + frame_height ) ) )
			{
				LIST_FORALL( pc->caption_buttons, idx, PCAPTION_BUTTON, button )
				{
					if( x > button->offset )
					{
						if( pc->pressed_caption_button && pc->pressed_caption_button == button )
						{
							button->is_pressed = FALSE;
							DrawFrameCaption( pc );
							UpdateDisplayPortion( pf->pActImg
														, pc->surface_rect.x, 0
														, pc->surface_rect.width
														, pc->surface_rect.y );
							button->pressed_event( pc );
						}
					}
				}
			}
			if( pc->pressed_caption_button )
			{
				if( pc->pressed_caption_button->pressed )
				{
					pc->pressed_caption_button->is_pressed = FALSE;
					DrawFrameCaption( pc );
					UpdateDisplayPortion( pf->pActImg
												, pc->surface_rect.x, 0
												, pc->surface_rect.width
												, pc->surface_rect.y );
				}
				pc->pressed_caption_button = NULL;
			}
		}
		 pf->flags.bDragging = 0;
		 pf->flags.bSizing = 0;
		 pf->flags.bSizing_top = 0;
		 pf->flags.bSizing_left = 0;
		 pf->flags.bSizing_right = 0;
		 pf->flags.bSizing_bottom = 0;
	}
	else if( !(pf->_b & MK_LBUTTON )
				&& ( b & MK_LBUTTON ) ) // check first down on dialog to drag
	{
		int caption_height = CaptionHeight( pc, GetText( pc->caption.text ) );
		int frame_height = FrameBorderYOfs( pc, pc->BorderType, NULL );

		 //Log( WIDE("No control, and last state was not pressed.") );
		OwnCommonMouse( pc, 1 );
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("First left down (%d,%d) %08x"), x, y , b );
#endif
		if( pc->BorderType & BORDER_RESIZABLE )
		{
				pf->_x = x;
				pf->_y = y;
				if( y < pc->surface_rect.y )
				{
					if ( y < ( pc->surface_rect.y - caption_height ) )
					{  // very top edge
						 if( x < pc->surface_rect.x + 10 ) // left side edge
						 {
#ifdef DETAILED_MOUSE_DEBUG
							 if( g.flags.bLogDetailedMouse )
								 Log( WIDE("Setting size frame on top/left edge") );
#endif
								pf->flags.bSizing_top = 1;
								pf->flags.bSizing_left = 1;
								pf->flags.bSizing = TRUE;
								result = 1;
						 }
						 else if( (S_64)x > ( ( pc->surface_rect.x 
												+ pc->surface_rect.width ) - 10 ) ) // right side edge
						 {
#ifdef DETAILED_MOUSE_DEBUG
							 if( g.flags.bLogDetailedMouse )
								 Log( WIDE("Setting size frame on top/right edge") );
#endif
								pf->flags.bSizing_top = 1;
								pf->flags.bSizing_right = 1;
								pf->flags.bSizing = TRUE;
								result = 1;
						 }
						 else // center top edge
						 {
#ifdef DETAILED_MOUSE_DEBUG
							 if( g.flags.bLogDetailedMouse )
								 Log( WIDE("Setting size frame on top edge") );
#endif
							 pf->flags.bSizing_top = 1;
							 pf->flags.bSizing = TRUE;
								result = 1;
						 }
					}
					else
					{  // top within caption band
						 if( x < pc->surface_rect.x ) // left side edge
						 {
#ifdef DETAILED_MOUSE_DEBUG
							 if( g.flags.bLogDetailedMouse )
								 Log( WIDE("Setting size frame on left edge(caption)") );
#endif
								pf->flags.bSizing_left = 1;
								pf->flags.bSizing_top = 1;
								pf->flags.bSizing = TRUE;
								result = 1;
							}
							else if( (S_64)x > ( pc->surface_rect.x 
											+ pc->surface_rect.width ) )
							{
#ifdef DETAILED_MOUSE_DEBUG
								if( g.flags.bLogDetailedMouse )
									Log( WIDE("Setting size frame on right edge(caption)") );
#endif
								pf->flags.bSizing_right = 1;
								pf->flags.bSizing_top = 1;
								pf->flags.bSizing = TRUE;
								result = 1;
							}
							else // center bottom edge
							{
								int on_button = 0;
#ifdef DETAILED_MOUSE_DEBUG
								if( g.flags.bLogDetailedMouse )
									Log( WIDE("Setting drag frame on caption") );
#endif
								if( !on_button && pc->caption_buttons )
								{
									PCAPTION_BUTTON button;
									INDEX idx;
									LIST_FORALL( pc->caption_buttons, idx, PCAPTION_BUTTON, button )
									{
										if( x > button->offset )
										{
											on_button = TRUE;
											break;
										}
									}
									if( button )
									{
										if( !button->is_pressed )
										{
											pc->pressed_caption_button = button;
											button->is_pressed = TRUE;
											DrawFrameCaption( pc );
											UpdateDisplayPortion( pf->pActImg, pc->surface_rect.x, 0
																		, pc->surface_rect.width
																		, pc->surface_rect.y );
										}
									}
								}
								if( !on_button )
								{
									pf->flags.bDragging = TRUE;
									pf->drag_x = x;
									pf->drag_y = y;
								}
								result = 1;
							}
					}
				}
				else if( (S_64)y >= ( ( pc->surface_rect.y 
									 + pc->surface_rect.height ) - 10 ) ) // bottom side...
				{  // very bottom band
					if( x < ( pc->surface_rect.x + 10 ) ) // left side edge
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							Log( WIDE("Setting size frame on left/bottom edge") );
#endif
						 pf->flags.bSizing_left = 1;
						 pf->flags.bSizing_bottom = 1;
						 pf->flags.bSizing = TRUE;
						 result = 1;
					}
					else if( (S_64)x > ( ( pc->surface_rect.x 
										 + pc->surface_rect.width ) - 10 ) )
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							Log( WIDE("Setting size frame on right/bottom edge") );
#endif
						 pf->flags.bSizing_right = 1;
						 pf->flags.bSizing_bottom = 1;
						 pf->flags.bSizing = TRUE;
						 result = 1;
					}
					else // center bottom edge
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							Log( WIDE("Setting size frame on bottom edge") );
#endif
						 pf->flags.bSizing_bottom = 1;
						 pf->flags.bSizing = TRUE;
						 result = 1;
					}

				}
				else // between top and bottom border/caption
				{
					if( x < pc->surface_rect.x ) // left side edge
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							Log( WIDE("Setting size frame on left edge") );
#endif
						 pf->flags.bSizing_left = 1;
						 pf->flags.bSizing = TRUE;
						 result = 1;
					}
					else if( (S_64)x >= ( ( pc->surface_rect.x
										 + pc->surface_rect.width ) - 10 ) )// right side edge
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							Log( WIDE("Setting size frame on right edge") );
#endif
						pf->flags.bSizing_right = 1;
						pf->flags.bSizing = TRUE;
						result = 1;
					}
					else
					{
						if( bCallOriginal )
							InvokeResultingMethod( result, pc, _MouseMethod, (pc
																	 , x - pc->surface_rect.x
																	 , y - pc->surface_rect.y
																	 , b ) );
					}
				}
		}
		else
		{
			if( ( (S_64)y >= pc->surface_rect.y )
				&& ( (S_64)y < ( pc->surface_rect.y
									+ pc->surface_rect.height ) ) // bottom side...
				&& ( (S_64)x >= pc->surface_rect.x ) // left side edge
				&& ( (S_64)x < ( pc->surface_rect.x
									+ pc->surface_rect.width ) ) )// right side edge
			{
				// if within the surface, then forward to the real mouse proc...
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("dispatching mouse to frame mouse method %d,%d biased %d,%d"), x, y
							, pc->surface_rect.x
							, pc->surface_rect.y );
#endif
				if( bCallOriginal )
					InvokeResultingMethod( result, pc, _MouseMethod, (pc
															, x - pc->surface_rect.x
															, y - pc->surface_rect.y
															, b ) );
			}
			else
			{
				PCAPTION_BUTTON button = NULL;
				if( ( y > frame_height )
					&& ( y < ( frame_height + caption_height ) )
					&& ( x < ( pc->surface_rect.x + pc->surface_rect.width ) ) 
					)
				{
					INDEX idx;
					LIST_FORALL( pc->caption_buttons, idx, PCAPTION_BUTTON, button )
					{
						if( x > button->offset )
						{
							break;
						}
					}
					if( button )
					{
						pc->pressed_caption_button = button;
						button->is_pressed = TRUE;
						DrawFrameCaption( pc );
						UpdateDisplayPortion( pf->pActImg, pc->surface_rect.x, 0
													, pc->surface_rect.width
													, pc->surface_rect.y );
					}
				}
				if( !button && !( pc->BorderType & BORDER_NOMOVE ) && pc->device )
				{
					// otherwise set dragging... hmm
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("Set drag on frame to %d,%d"), x, y );
#endif
					pf->_x = x;
					pf->_y = y;
					pf->flags.bDragging = TRUE;
					pf->drag_x = x;
					pf->drag_y = y;
					result = 1;
				}
			}
		}
	}
	else // unhandled mouse button transition/state
	{
		int on_button = 0;
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			Log( WIDE("release button drag frame on caption") );
#endif
		if( !on_button && pc->caption_buttons )
		{
			PCAPTION_BUTTON button;
			INDEX idx;
			LIST_FORALL( pc->caption_buttons, idx, PCAPTION_BUTTON, button )
			{
				if( x > button->offset )
				{
					on_button = TRUE;
					break;
				}
			}
			if( button )
			{
				if( !button->is_pressed )
				{
					pc->pressed_caption_button = button;
					button->is_pressed = TRUE;
					DrawFrameCaption( pc );
					UpdateDisplayPortion( pf->pActImg, pc->surface_rect.x, 0
												, pc->surface_rect.width
												, pc->surface_rect.y );
				}
			}
		}
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("unhandled mouse state... see if there's a method") );
#endif
		if( pc->BorderType & BORDER_WANTMOUSE )
			InvokeResultingMethod( result, pc, _MouseMethod, (pc
													 , x - pc->surface_rect.x
													 , y - pc->surface_rect.y
													 , b ) );
	}
	//pf->_b = b;
	return result;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetFrameMousePosition )( PSI_CONTROL pfc, int x, int y )
{
	PPHYSICAL_DEVICE pf = pfc->device;
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pfc );
	if( !pf && pfc && pfc->parent && !pfc->device)
		SetFrameMousePosition( pfc->parent
									, x + pfc->surface_rect.x + pfc->rect.x
									, y + pfc->surface_rect.y + pfc->rect.y );
	else
	{
		if( 0 )
		{
			// know, the hotspot thing is too agressive anyway.
			SetMousePosition( pf->pActImg
								, x + pfc->surface_rect.x
								, y + pfc->surface_rect.y
								);
		}
	}
}

//---------------------------------------------------------------------------

int HandleEditStateMouse( PEDIT_STATE pEditState
								, PSI_CONTROL pfc
								, S_32 x
								, S_32 y
								, _32 b )
{
	PPHYSICAL_DEVICE pf = pfc->device;
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pfc );
	int spot;
	if( !pf )
		return 0;
	//lprintf( WIDE("handle edit mouse state at %d,%d"), x, y );
	if( pEditState->flags.bActive )
	{
		int dx = (x-(pEditState->bias[0]/*+pfc->surface_rect.x*/)) - pEditState->_x
		 , dy = (y-(pEditState->bias[1]/*+pfc->surface_rect.y*/)) - pEditState->_y;
		if( pEditState->flags.bDragging )
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				Log( WIDE("Having dragging set...") );
#endif
			if( BREAK_LASTBUTTON( b, pf->_b ) )
			{
				pEditState->flags.bDragging = 0;
				spot = 0; // no spot.
			}
			else
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					Log( WIDE("Moving control..") );
#endif
				//lprintf( WIDE("moving control by %d,%d  (%d,%d)"), dx, dy, x, y );
				MoveControlRel( pEditState->pCurrent
								 , dx, dy );
				spot = pEditState->flags.fLocked;
			}
		}
		else if( pEditState->flags.bSizing )
		{
			if( BREAK_LASTBUTTON( b, pf->_b ) )
			{
				pEditState->flags.bSizing = 0;
				pEditState->flags.bSizing_left = 0;
				pEditState->flags.bSizing_right = 0;
				pEditState->flags.bSizing_top = 0;
				pEditState->flags.bSizing_bottom = 0;
				spot = 0; // no spot selected...
			}
			else
			{
				//lprintf( WIDE("sizing control by %d,%d %d,%d %d,%d  (%d,%d)")
				//		, x, y
				//		 , pEditState->_x, pEditState->_y
				//		, dx, dy
				//		, x-pEditState->bias[0]
				//		, y-pEditState->bias[1] );
				if( pEditState->flags.bSizing_left )
				{
					if( pEditState->flags.bSizing_top )
					{
						MoveSizeControlRel( pEditState->pCurrent, dx, dy, -dx, -dy );
					}
					else if( pEditState->flags.bSizing_bottom )
					{
						//pEditState->_y = y;
						MoveSizeControlRel( pEditState->pCurrent, dx, 0, -dx, dy );
					}
					else
					{
						MoveSizeControlRel( pEditState->pCurrent, dx, 0, -dx, 0 );
					}
				}
				else if( pEditState->flags.bSizing_right )
				{
					pEditState->_x = x; // update prior coord to 'here'
					if( pEditState->flags.bSizing_top )
					{
						MoveSizeControlRel( pEditState->pCurrent, 0, dy, dx, -dy );
					}
					else if( pEditState->flags.bSizing_bottom )
					{
						//pEditState->_y = y;
						SizeControlRel( pEditState->pCurrent, dx, dy );
					}
					else
					{
						SizeControlRel( pEditState->pCurrent, dx, 0 );
					}
				}
				else
				{
					if( pEditState->flags.bSizing_top )
					{
						MoveSizeControlRel( pEditState->pCurrent, 0, dy, 0, -dy );
					}
					else if( pEditState->flags.bSizing_bottom )
					{
						//pEditState->_y = y;
						SizeControlRel( pEditState->pCurrent, 0, dy );
					}
				}
#ifdef HOTSPOT_DEBUG
				lprintf( WIDE("Set edit mouse to %d,%d"), x, y );
#endif
				SetupHotSpots( pEditState );
				pEditState->_x = pEditState->hotspot[pEditState->flags.fLocked-1][0];
				pEditState->_y = pEditState->hotspot[pEditState->flags.fLocked-1][1];
				spot = pEditState->flags.fLocked;
			}
		}
		else
		{
			// wasn't previously dragging or sizing...
			// perhaps we should be now?
#ifdef HOTSPOT_DEBUG
			lprintf("check - current bias = %d,%d", pf->CurrentBias.x, pf->CurrentBias.y );
			lprintf("check - current bias = %d,%d", pEditState->bias[0], pEditState->bias[1] );
#endif
			spot = MouseInHotSpot( pEditState
										, x - (pEditState->bias[0]//+pfc->surface_rect.x
												)
										, y - (pEditState->bias[1]//+pfc->surface_rect.y
												)
										);
										//, x - pfc->surface_rect.x
										//, y - pfc->surface_rect.y );
			if( pEditState->flags.fLocked != spot )
			{
				// spot has changed.
#ifdef HOTSPOT_DEBUG
				Log( WIDE("Mark changed spots...") );
#endif
				pEditState->flags.fLocked = spot;
			}
			if( MAKE_FIRSTBUTTON( b, pf->_b ) )
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					Log( WIDE("Checking edit state op based on current spot...") );
#endif
				if( pEditState->flags.bActive )
				{
					if( b & MK_RBUTTON )
					{
						if( spot && pEditState->pCurrent )
							return EditControlProperties( pEditState->pCurrent );
						else
							return EditFrameProperties( pfc
															 , x - (pfc->device?0:pfc->rect.x + pfc->surface_rect.x)
															 , y - (pfc->device?0:pfc->rect.y + pfc->surface_rect.y) );
					}
					else if( spot )
					{
#ifdef HOTSPOT_DEBUG
						lprintf( WIDE("Set edit mouse to %d,%d"), x, y );
#endif
						pEditState->_x = pEditState->hotspot[pEditState->flags.fLocked-1][0];
						pEditState->_y = pEditState->hotspot[pEditState->flags.fLocked-1][1];
						//Log( WIDE("Setting spot operation...") );
						pEditState->flags.bSizing_left = 0;
						pEditState->flags.bSizing_right = 0;
						pEditState->flags.bSizing_top = 0;
						pEditState->flags.bSizing_bottom = 0;
						switch( spot )
						{
						case 1:
							pEditState->flags.bSizing_left = 1;
							pEditState->flags.bSizing_top = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 2:
							pEditState->flags.bSizing_top = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 3:
							pEditState->flags.bSizing_right = 1;
							pEditState->flags.bSizing_top = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 4:
							pEditState->flags.bSizing_left = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 5:
#ifdef DETAILED_MOUSE_DEBUG
							if( g.flags.bLogDetailedMouse )
								Log( WIDE("Setting dragging...") );
#endif
							pEditState->flags.bDragging = 1;
							break;
						case 6:
							pEditState->flags.bSizing_right = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 7:
							pEditState->flags.bSizing_left = 1;
							pEditState->flags.bSizing_bottom = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 8:
							pEditState->flags.bSizing_bottom = 1;
							pEditState->flags.bSizing = 1;
							break;
						case 9:
							pEditState->flags.bSizing_right = 1;
							pEditState->flags.bSizing_bottom = 1;
							pEditState->flags.bSizing = 1;
							break;
						}
					}
				}
			}
		}
		if( pEditState->flags.fLocked )
		{
			SetFrameMousePosition( pfc
										, pEditState->bias[0]
										+ pEditState->hotspot[pEditState->flags.fLocked-1][0]
										- pfc->surface_rect.x
										, pEditState->bias[1]
										+ pEditState->hotspot[pEditState->flags.fLocked-1][1]
										- pfc->surface_rect.y
										);
		}
#ifdef HOTSPOT_DEBUG
 		else
		{
			lprintf( WIDE("not locked.") );
		}
#endif
	}
	else
		spot = 0;
	return spot;
}

//---------------------------------------------------------------------------

int InvokeMouseMethod( PSI_CONTROL pfc, S_32 x, S_32 y, _32 b )
{
	PPHYSICAL_DEVICE pf = pfc->device;
	PSI_CONTROL pCurrent;
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pfc );
	if( !pf )
		return 0;
#ifdef DETAILED_MOUSE_DEBUG
	if( g.flags.bLogDetailedMouse )
		lprintf( WIDE("mouse method received on %p"), pfc );
#endif
	pCurrent = pf->pCurrent;
	if( !HandleEditStateMouse( &pf->EditState, pfc, x, y, b ) )
	{
		int result;
		pCurrent = pf->pCurrent;
		if( !pf->flags.bCaptured && ( pCurrent == (PSI_CONTROL)pfc ) )
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("**** Calling frame's mouse... (%d,%d,%08x)"), x, y, b );
#endif
			AddUse( pfc );
			result = FirstFrameMouse( pf, x, y, b, TRUE );
			DeleteUse( pfc );
			return result;
		}
		else if( pCurrent )
		{
			if( !pCurrent->flags.bDestroy && pCurrent->_MouseMethod )
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("calling current, perhaps owned, mouseproc %p"), pCurrent );
#endif
				AddUse( pCurrent );
				pfc->NotInUse = pfc->InUse;
				InvokeResultingMethod( result, pCurrent, _MouseMethod, (pCurrent
														 , x - pf->CurrentBias.x
														 , y - pf->CurrentBias.y
																						, b ) );
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE( "Result of resulting method is %d" ), result );
#endif
				pfc->NotInUse = 0;
				DeleteUse( pCurrent );
				return result;
			}
		}
#ifdef DETAILED_MOUSE_DEBUG
		else if( g.flags.bLogDetailedMouse )
		{
			lprintf( WIDE("No current!") );
		}
#endif
	}
	return 0;
}

//---------------------------------------------------------------------------

int IsMouseInCurrent( PSI_CONTROL pfc, S_32 x, S_32 y, _32 is_surface, _32 b )
{
	PPHYSICAL_DEVICE pf = pfc->device;
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pfc );
	IMAGE_POINT tolerance;
	if( !pf || !pf->pCurrent )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("no current...") );
#endif
		return FALSE;
	}

	// cannot be in a hidden control.
	if( pf->pCurrent->flags.bHidden )
		return FALSE;

	if( pf->EditState.flags.bActive &&
		(PSI_CONTROL)pf->EditState.pCurrent == (PSI_CONTROL)pf->pCurrent )
	{
		tolerance[0] = SPOT_SIZE;
		tolerance[1] = SPOT_SIZE;
	}
	else
	{
		tolerance[0] = 0;
		tolerance[1] = 0;
	}
	while( 1 )
	{
		if( pf->CurrentBias.flags.bias_is_surface )
		{
			// use tolerance to make the border wider
			if( (S_64)x >= tolerance[0] && (S_64)y >= tolerance[0] &&
				(S_64)x < ( pf->pCurrent->surface_rect.width - 2*tolerance[0] ) &&
				(S_64)y < ( pf->pCurrent->surface_rect.height - 2*tolerance[1] )  )
			{
				PSI_CONTROL pc;
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Mouse (%d,%d) %08x is still in current control (%d,%d)-(%d,%d)... bias(%d,%d) let's see - does it have children? %p")
							, x, y, b
							, tolerance[0]
							, tolerance[0]
							, pf->pCurrent->surface_rect.width
							, pf->pCurrent->surface_rect.height
							, pf->CurrentBias.x
							, pf->CurrentBias.y
							, pf->pCurrent->child );
#endif
				//if( !pf->pCurrent->parent )
				// x, y are biased in this function to be relative to the control passed...
				pc = FindControl( pfc, pf->pCurrent->child, x, y, b );
				//else
				//	pc = FindControl( pfc, pf->pCurrent->child, x, y, b );
				if( pc )
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("Found a control %p"), pc );
#endif
				}
#ifdef DETAILED_MOUSE_DEBUG
				else
				{
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("no sub-control found...") );
				}
#endif
			}
			else
			{
				// still is surface... now check against the suface without the tolerance...
				if( (S_64)x < 0 || (S_64)y < 0 || (S_64)x > pf->pCurrent->surface_rect.width || (S_64)y > pf->pCurrent->surface_rect.height )
				{
					// okay the mouse has moved outside the surface... re-set the current bias
					// and try this whole thing again.
					pf->CurrentBias.flags.bias_is_surface = 0;
					pf->CurrentBias.x -= pf->pCurrent->surface_rect.x;
					pf->CurrentBias.y -= pf->pCurrent->surface_rect.y;
					x += pf->pCurrent->surface_rect.x;
					y += pf->pCurrent->surface_rect.y;
					continue;
				}
				// otherwise it's still in the surface, and all is well...
			}
			// still within the surface of the control...
			return TRUE;
		}
		else
		{
			if( (S_64)x < pf->pCurrent->surface_rect.x ||
				(S_64)y < pf->pCurrent->surface_rect.y ||
				(S_64)x > pf->pCurrent->surface_rect.width + pf->pCurrent->surface_rect.x ||
				(S_64)y > pf->pCurrent->surface_rect.height + pf->pCurrent->surface_rect.y )
			{
				// still in control ( full tolerance is based on full rect.)
				if( (S_64)x >= -tolerance[0] && (S_64)y >= -tolerance[0] &&
					(S_64)x < ( pf->pCurrent->rect.width + 2*tolerance[0] ) &&
					(S_64)y < ( pf->pCurrent->rect.height + 2*tolerance[1] )  )
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("Outside of control bounds... not in current.") );
#endif
					if( pf->common->BorderType & BORDER_RESIZABLE )
						return FALSE;
					return TRUE;
				}
				break; // return FALSE.  the mouse has fallen well outside of tolerant distance.
			}
			else
			{
				// mouse is now within the surface...
				pf->CurrentBias.flags.bias_is_surface = 1;
				pf->CurrentBias.x += pf->pCurrent->surface_rect.x;
				pf->CurrentBias.y += pf->pCurrent->surface_rect.y;
				x -= pf->pCurrent->surface_rect.x;
				y -= pf->pCurrent->surface_rect.y;
				continue;
				// otherwise we've fallen off of this control (outside surface, beyond window)
			}
		}
	}
	// not in the surface rect anymore...
	return FALSE;
}

//---------------------------------------------------------------------------

// this is called directly from the renderer layer...
// therefore x, y are relative to the upper left corner of the
// renderer which a frame is attached.  THe psv passed is the frame
// which this refers to.
// First, if it's not focused, ignore the mouse ( mouse would have
// been locked to another? )
// Then, If no buttons are pressed, then any prior control which
// may have been the current owner of the mouse is no longer.
// Such a control does not receive notification of the de-press. (until later)
// Then, if there's still a control which has posession of the
// logical mouse, then its method is then called with the current state.
// Calls to a control are relative to its surface.  It should never be
// drawing within the frame itself... that is actually not part of
// its property.
// Then - find a control.
//	Act of finding a control shall
//---------------------------------------------------------------------------

int CPROC AltFrameMouse( PTRSZVAL psvCommon, S_32 x, S_32 y, _32 b )
{
	PPHYSICAL_DEVICE pf = (PPHYSICAL_DEVICE)psvCommon;
	//PFRAME pf = (PFRAME)psvCommon;
	int result = 0;
	PSI_CONTROL pc = pf->common;
	PSI_CONTROL pcIn;
	extern void DumpFrameContents( PSI_CONTROL );
	if( !pc )
	{
		// maybe this was closed before it actually got the first message?
		return 0;
	}
#ifdef USE_IMAGE_INTERFACE
	if( !USE_IMAGE_INTERFACE )
	{
		// are we shutting down? did we fail to start?
		// somehow we have render events without an image.
		return 0;
	}
#endif
	if( pc->flags.bDestroy )
	{
		// somehow we're being destroyed, but not everyone knows yet
		return 0;
	}
	GetCurrentDisplaySurface(pf);
	//DumpFrameContents( pc );
#ifdef DETAILED_MOUSE_DEBUG
	if( g.flags.bLogDetailedMouse )
	{
		lprintf( WIDE("-------------------------------------------------") );
		lprintf( WIDE("Mouse event: (%d,%d) %08x bias is(%d,%d) %s")
				, x, y, b
				, pf->CurrentBias.x, pf->CurrentBias.y
				, pf->CurrentBias.flags.bias_is_surface?WIDE( "surface" ):WIDE( "frame" )
				);
	}
#endif
	AddUse( (PSI_CONTROL)pc );
	if( pf->flags.bCaptured )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("Caputred mouse dispatch!") );
#endif
		if( !pc->flags.bDestroy )
		{
			result = InvokeMouseMethod( pc, x, y, b );
		}
		pf->_b = b;
		DeleteUse( pc );
		return result;
	}
	if( pf->flags.bCurrentOwns )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE( "Current owns flag is set?" ) );
#endif
		result = InvokeMouseMethod( pc, x, y, b );
		if( pf = pc->device )
		{
			if( pf->flags.bApplicationOwned && BREAK_LASTBUTTON( b, pf->_b ) )
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Well current no longer owns mouse..") );
#endif
				OwnCommonMouse( pc, FALSE );
			}
			else
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Returning early... still owned...") );
#endif
			}
			pf->_b = b;
		}
		// no longer owned, but event was already dispatched.
		DeleteUse( pc );
		return result;
	}
	if( !pf->flags.bSizing 
		&& !pf->flags.bDragging
		&& !pc->pressed_caption_button
		&& IsMouseInCurrent( pc
							 , x - pf->CurrentBias.x
							 , y - pf->CurrentBias.y
							 , pf->CurrentBias.flags.bias_is_surface
							 , b ) )
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("Mouse is still in current (%d,%d)")
					, x - pf->CurrentBias.x
					, y - pf->CurrentBias.y );
#endif
		// although this should be part of InvokeMouseMethod
		// InvokeMethod doesn't have the pf to compare first button.
		if( MAKE_FIRSTBUTTON( b, pf->_b ) )
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
			{
				lprintf( WIDE("First left down %p (%d,%d) %08x"), pf->pCurrent, x, y , b );
				lprintf( WIDE("Setting current owns...") );
			}
#endif
			OwnCommonMouse( pf->pCurrent, TRUE );
			//pf->flags.bCurrentOwns = TRUE;
			SetCommonFocus( pf->pCurrent );
		}
	retry1:
		// mouseincurrent invokes mouse...
		if( pf->pCurrent && !( result = InvokeMouseMethod( pc, x, y, b ) ) )
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE( "!! Mouse was not used by control - try and find it's parent." ) );
#endif
			if( pf->CurrentBias.flags.bias_is_surface )
			{
				pf->CurrentBias.x -= pf->pCurrent->surface_rect.x;
				pf->CurrentBias.y -= pf->pCurrent->surface_rect.y;
			}
			pf->CurrentBias.x -= pf->pCurrent->rect.x;
			pf->CurrentBias.y -= pf->pCurrent->rect.y;
			pf->CurrentBias.flags.bias_is_surface = 1;
			pf->pCurrent = pf->pCurrent->parent;
			goto retry1;
		}
	}
	else
	{
#ifdef DETAILED_MOUSE_DEBUG
		if( g.flags.bLogDetailedMouse )
			lprintf( WIDE("Mouse is no longer in current (%d,%d) Setting new current control...")
					, x - pf->CurrentBias.x
					, y - pf->CurrentBias.y );
#endif
		pf->pCurrent = pc;
		if( (S_64)x < pc->surface_rect.x ||
			(S_64)y < pc->surface_rect.y ||
			(S_64)x > pc->surface_rect.x + pc->surface_rect.width ||
			(S_64)y > pc->surface_rect.y + pc->surface_rect.height ||
			pf->flags.bSizing || pf->flags.bDragging || pc->pressed_caption_button )
		{
			// it's on the frame of this frame (redunant eh?)
			//lprintf( WIDE("Outside the surface, on border or frame... invoke frame handler.") );
			result = FirstFrameMouse( pf, x, y, b, TRUE );
			pf->CurrentBias.x = 0;
			pf->CurrentBias.y = 0;
			pf->CurrentBias.flags.bias_is_surface = 0;
		}
		else
		{
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("Mouse on surface of frame, find a control.") );
#endif
			pf->CurrentBias.x = pc->surface_rect.x;
			pf->CurrentBias.y = pc->surface_rect.y;
			pf->CurrentBias.flags.bias_is_surface = 1;
			pcIn = FindControl( pc, pc->child
									, x - pf->CurrentBias.x
									, y - pf->CurrentBias.y, b );
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("Found control %p %p in %p with bias %d,%d"), pcIn, pcIn?pcIn->parent:NULL, pc
						, pf->CurrentBias.x, pf->CurrentBias.y );
#endif
			//ComputeFrameBias( pf, pcIn );
			if( pcIn )
			{
				// although this should be part of InvokeMouseMethod
				// InvokeMethod doesn't have the pf to compare first button.
				if( MAKE_FIRSTBUTTON( b, pf->_b ) )
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("Setting current owns...") );
#endif
					OwnCommonMouse( pc, TRUE );
					//pf->flags.bCurrentOwns = TRUE;
					SetCommonFocus( pf->pCurrent );
				}
			retry:
				if( pf->pCurrent && !( result = InvokeMouseMethod( pc, x, y, b ) ) )
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE( "!! Again - did not want the mouse... go back to parent." ) );
#endif
					if( pf->CurrentBias.flags.bias_is_surface )
					{
#ifdef DETAILED_MOUSE_DEBUG
						if( g.flags.bLogDetailedMouse )
							lprintf( WIDE( "was on surface... so remove that." ) );
#endif
						if( pf->pCurrent )
						{
							pf->CurrentBias.x -= pf->pCurrent->surface_rect.x;
							pf->CurrentBias.y -= pf->pCurrent->surface_rect.y;
						}
					}
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE( "was on a control remove (%d,%d)" ), pf->pCurrent->rect.x, pf->pCurrent->rect.y );
#endif
					if( pf->pCurrent )
					{
						pf->CurrentBias.x -= pf->pCurrent->rect.x;
						pf->CurrentBias.y -= pf->pCurrent->rect.y;
						pf->CurrentBias.flags.bias_is_surface = 1;
						pf->pCurrent = pf->pCurrent->parent;
					}
					goto retry;
				}
				// else coordinates to a frame proc are raw ... at least the first level one
				// which handles resize and drag, then to the actual surface.
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("Resulting bias to (%d,%d) %s")
							, pf->CurrentBias.x, pf->CurrentBias.y
							, pf->CurrentBias.flags.bias_is_surface?WIDE( "surface" ):WIDE( "frame" )
							);
#endif
 			}
			else
			{
#ifdef DETAILED_MOUSE_DEBUG
				if( g.flags.bLogDetailedMouse )
					lprintf( WIDE("no control found... see if there's a method....") );
#endif
				// although this should be part of InvokeMouseMethod
				// InvokeMethod doesn't have the pf to compare first button.
				if( MAKE_FIRSTBUTTON( b, pf->_b ) )
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE("Setting current owns...") );
#endif
					OwnCommonMouse( pc, TRUE );
					//pf->flags.bCurrentOwns = TRUE;
					SetCommonFocus( pf->pCurrent );
				}
			retry3:
				if( pf->pCurrent && !( result = InvokeMouseMethod( pc, x, y, b ) ) )
				{
#ifdef DETAILED_MOUSE_DEBUG
					if( g.flags.bLogDetailedMouse )
						lprintf( WIDE( "!! Again - did not want the mouse... go back to parent." ) );
#endif
					if( pf->CurrentBias.flags.bias_is_surface )
					{
						pf->CurrentBias.x -= pf->pCurrent->surface_rect.x;
						pf->CurrentBias.y -= pf->pCurrent->surface_rect.y;
					}
					pf->CurrentBias.x -= pf->pCurrent->rect.x;
					pf->CurrentBias.y -= pf->pCurrent->rect.y;
					pf->pCurrent = pf->pCurrent->parent;
					goto retry3;
				}
				if( pc->BorderType & BORDER_WANTMOUSE )
				{
					result = InvokeMouseMethod( pc, x, y, b );
					pf = pc->device;
				}
			}
		}
	}
	if( pf )
 		pf->_b = b;
	//lprintf("releasing %p", pc );
	DeleteUse( pc );
	return result;
}

//---------------------------------------------------------------------------

static int OnMouseCommon( WIDE("Frame") )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	// okay really frame mouse proc is JUST like every other
	// control proc- mouse coordinates are relative to the surface.
	PSI_CONTROL frame = GetFrame( pc );
	PPHYSICAL_DEVICE pf = frame?frame->device:NULL;
	if( !pf )
		return FALSE; //probably destroyed or something...

	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pc );
	// if frame is marked NOMOVE, don't drag it...
	if( !( pc->BorderType & BORDER_NOMOVE ) )
	{
		if( ( b & MK_LBUTTON ) &&
			!( pf->_b & MK_LBUTTON ) )
		{
#ifdef SUPER_DETAILED_MOUSE_DEBUG
			if( g.flags.bLogSuperDetailedMouse )
				lprintf( WIDE("Default frame mouse recieved mouse biased to surface: %d,%d"), x, y );
#endif
			pf->_x = x + pf->CurrentBias.x;
			pf->_y = y + pf->CurrentBias.y;
#ifdef DETAILED_MOUSE_DEBUG
			if( g.flags.bLogDetailedMouse )
				lprintf( WIDE("Setting drag at %d,%d + (%d,%d)==(%d,%d)?")
						, pf->_x
						, pf->_y
						, pc->surface_rect.x
						, pc->surface_rect.y
						, pf->CurrentBias.x, pf->CurrentBias.y );
#endif
			pf->flags.bDragging = TRUE;
			pf->drag_x = x + pf->CurrentBias.x;
			pf->drag_y = y + pf->CurrentBias.y;
		}
	}
	return TRUE;
}

//---------------------------------------------------------------------------

void CaptureCommonMouse( PSI_CONTROL pc, LOGICAL bCapture )
{
	PSI_CONTROL pf = GetFrame( pc );
	PPHYSICAL_DEVICE pd = pf?pf->device:NULL;
	if( pd )
	{
		PSI_CONTROL _pc;
		pd->flags.bCaptured = bCapture;
		pd->pCurrent = pc;
		pd->CurrentBias.x = 0;
		pd->CurrentBias.y = 0;
		for( _pc = pc; _pc; _pc = _pc->parent )
		{
			pd->CurrentBias.x += _pc->surface_rect.x;
			pd->CurrentBias.y += _pc->surface_rect.y;
			if( _pc->parent )
			{
				pd->CurrentBias.x += _pc->surface_rect.x;
				pd->CurrentBias.y += _pc->surface_rect.y;
			}
		}

		OwnMouse( pd->pActImg, bCapture );
	}
}

void GetMouseButtons( PSI_CONTROL pc, _32 *buttons )
{
	if( buttons )
	{
		PSI_CONTROL frame = GetFrame( pc );
		PPHYSICAL_DEVICE pf = frame?frame->device:NULL;
		(*buttons) = pf->_b;
	}
}

PSI_MOUSE_NAMESPACE_END

