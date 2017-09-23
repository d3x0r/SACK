#define NO_UNICODE_C
#define VIDLIB_INTERFACE_DEFINED
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include "local.h"

RENDER_NAMESPACE


static RENDER_INTERFACE VidInterface = { NULL //InitDisplay
                                       , SetApplicationTitle
                                       , (void (CPROC*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , (PRENDERER (CPROC*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t)) OpenDisplaySizedAt
                                       , (PRENDERER (CPROC*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void (CPROC*)(PRENDERER)) CloseDisplay
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t, uint32_t, uint32_t DBG_PASS)) UpdateDisplayPortionEx
                                       , (void (CPROC*)(PRENDERER DBG_PASS)) UpdateDisplayEx
                                       , GetDisplayPosition
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) MoveDisplay
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) MoveDisplayRel
                                       , (void (CPROC*)(PRENDERER, uint32_t, uint32_t)) SizeDisplay
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image (CPROC*)(PRENDERER)) GetDisplayImage
                                       , (void (CPROC*)(PRENDERER, CloseCallback, uintptr_t)) SetCloseHandler
                                       , (void (CPROC*)(PRENDERER, MouseCallback, uintptr_t)) SetMouseHandler
                                       , (void (CPROC*)(PRENDERER, RedrawCallback, uintptr_t)) SetRedrawHandler
                                       , (void (CPROC*)(PRENDERER, KeyProc, uintptr_t)) SetKeyboardHandler
													,  SetLoseFocusHandler
                                          , NULL
                                       , (void (CPROC*)(int32_t *, int32_t *)) GetMousePosition
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) SetMousePosition
                                       , HasFocus  // has focus
                                       , GetKeyText
                                       , IsKeyDown
                                       , KeyDown
                                       , DisplayIsValid
                                       , OwnMouseEx
                                       , BeginCalibration
													, SyncRender   // sync
                                       , MoveSizeDisplay
                                       , MakeTopmost
                                       , HideDisplay
                                       , RestoreDisplay
                                       , ForceDisplayFocus
                                       , ForceDisplayFront
                                       , ForceDisplayBack
                                       , BindEventToKey
													, UnbindKey
													, IsTopmost
													, NULL // OkaySyncRender is internal.
													, IsTouchDisplay
													, GetMouseState
													, EnableSpriteMethod
#if defined( __ANDROID__ ) || defined( __LINUX__ )
													, NULL// WinShell_AcceptDroppedFiles
#else
													, WinShell_AcceptDroppedFiles
#endif
													, PutDisplayIn
                                       , NULL //MakeDisplayFrom
													, SetRendererTitle
													, DisableMouseOnIdle
													, OpenDisplayAboveUnderSizedAt
													, SetDisplayNoMouse
													, Redraw
													, MakeAbsoluteTopmost
													, SetDisplayFade
													, IsDisplayHidden
#ifdef WIN32
													, GetNativeHandle
#endif
                                       , GetDisplaySizeEx
													, LockRenderer
													, UnlockRenderer
													, NULL  //IssueUpdateLayeredEx
                                       , RequiresDrawAll
#ifndef NO_TOUCH
													, SetTouchHandler
#endif
                                       , MarkDisplayUpdated
									   , SetHideHandler
									   , SetRestoreHandler
													, RestoreDisplayEx
#if defined( __ANDROID__ )
                                       , SACK_Vidlib_ShowInputDevice
													, SACK_Vidlib_HideInputDevice
#else
													, NULL   //SACK_Vidlib_ShowInputDevice
                                       , NULL   //SACK_Vidlib_HideInputDevice
#endif
									   , PureGL2_Vidlib_AllowsAnyThreadToUpdate
									   , NULL // SetDisplayFullScreen
									   , PureGL2_Vidlib_SuspendSystemSleep
									   , NULL //PureGL2_Vidlib_RenderIsInstanced
									   , NULL // PureGL2_Vidlib_VidlibRenderAllowsCopy
									   , PureGL2_Vidlib_SetDisplayCursor
};

RENDER3D_INTERFACE Render3d = {
	GetRenderTransform
										, NULL
                              , GetViewVolume
};

#undef GetDisplayInterface
#undef DropDisplayInterface

POINTER  CPROC GetDisplayInterface (void)
{
   return (POINTER)&VidInterface;
}

void  CPROC DropDisplayInterface (POINTER p)
{
}

#undef GetDisplay3dInterface
POINTER CPROC GetDisplay3dInterface (void)
{
	return (POINTER)&Render3d;
}

void  CPROC DropDisplay3dInterface (POINTER p)
{
}


RENDER_NAMESPACE_END

