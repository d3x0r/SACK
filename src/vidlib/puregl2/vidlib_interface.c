#define NO_UNICODE_C
#define VIDLIB_INTERFACE_DEFINED
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include "local.h"

RENDER_NAMESPACE


static RENDER_INTERFACE VidInterface = { NULL //InitDisplay
                                       , ogl_SetApplicationTitle
                                       , (void (CPROC*)(Image)) ogl_SetApplicationIcon
                                       , ogl_GetDisplaySize
                                       , ogl_SetDisplaySize
                                       , (PRENDERER (CPROC*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t)) ogl_OpenDisplaySizedAt
                                       , (PRENDERER (CPROC*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t, PRENDERER)) ogl_OpenDisplayAboveSizedAt
                                       , (void (CPROC*)(PRENDERER)) ogl_CloseDisplay
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t, uint32_t, uint32_t DBG_PASS)) ogl_UpdateDisplayPortionEx
                                       , (void (CPROC*)(PRENDERER DBG_PASS)) ogl_UpdateDisplayEx
                                       , ogl_GetDisplayPosition
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) ogl_MoveDisplay
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) ogl_MoveDisplayRel
                                       , (void (CPROC*)(PRENDERER, uint32_t, uint32_t)) ogl_SizeDisplay
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) ogl_SizeDisplayRel
                                       , ogl_MoveSizeDisplayRel
                                       , (void (CPROC*)(PRENDERER, PRENDERER)) ogl_PutDisplayAbove
                                       , (Image (CPROC*)(PRENDERER)) ogl_GetDisplayImage
                                       , (void (CPROC*)(PRENDERER, CloseCallback, uintptr_t)) ogl_SetCloseHandler
                                       , (void (CPROC*)(PRENDERER, MouseCallback, uintptr_t)) ogl_SetMouseHandler
                                       , (void (CPROC*)(PRENDERER, RedrawCallback, uintptr_t)) ogl_SetRedrawHandler
                                       , (void (CPROC*)(PRENDERER, KeyProc, uintptr_t)) ogl_SetKeyboardHandler
													,  ogl_SetLoseFocusHandler
                                          , NULL
                                       , (void (CPROC*)(int32_t *, int32_t *)) ogl_GetMousePosition
                                       , (void (CPROC*)(PRENDERER, int32_t, int32_t)) ogl_SetMousePosition
                                       , ogl_HasFocus  // has focus
                                       , ogl_GetKeyText
                                       , ogl_IsKeyDown
                                       , ogl_KeyDown
                                       , ogl_DisplayIsValid
                                       , ogl_OwnMouseEx
                                       , ogl_BeginCalibration
													, ogl_SyncRender   // sync
                                       , ogl_MoveSizeDisplay
                                       , ogl_MakeTopmost
                                       , ogl_HideDisplay
                                       , ogl_RestoreDisplay
                                       , ogl_ForceDisplayFocus
                                       , ogl_ForceDisplayFront
                                       , ogl_ForceDisplayBack
                                       , ogl_BindEventToKey
													, ogl_UnbindKey
													, ogl_IsTopmost
													, NULL // OkaySyncRender is internal.
													, ogl_IsTouchDisplay
													, ogl_GetMouseState
													, ogl_EnableSpriteMethod
#if defined( __ANDROID__ ) || defined( __LINUX__ )
													, NULL// WinShell_AcceptDroppedFiles
#else
													, ogl_WinShell_AcceptDroppedFiles
#endif
													, ogl_PutDisplayIn
                                       , NULL //MakeDisplayFrom
													, ogl_SetRendererTitle
													, ogl_DisableMouseOnIdle
													, ogl_OpenDisplayAboveUnderSizedAt
													, ogl_SetDisplayNoMouse
													, ogl_Redraw
													, ogl_MakeAbsoluteTopmost
													, ogl_SetDisplayFade
													, ogl_IsDisplayHidden
#ifdef WIN32
													, ogl_GetNativeHandle
#endif
                                       , ogl_GetDisplaySizeEx
													, ogl_LockRenderer
													, ogl_UnlockRenderer
													, NULL  
                                       , ogl_RequiresDrawAll
#ifndef NO_TOUCH
													, ogl_SetTouchHandler
#endif
                                       , ogl_MarkDisplayUpdated
									   , ogl_SetHideHandler
									   , ogl_SetRestoreHandler
													, ogl_RestoreDisplayEx
#if defined( __ANDROID__ )
                                       , ogl_SACK_Vidlib_ShowInputDevice
													, ogl_SACK_Vidlib_HideInputDevice
#else
													, NULL   //SACK_Vidlib_ShowInputDevice
                                       , NULL   //SACK_Vidlib_HideInputDevice
#endif
									   , ogl_PureGL2_Vidlib_AllowsAnyThreadToUpdate
									   , NULL // SetDisplayFullScreen
									   , ogl_PureGL2_Vidlib_SuspendSystemSleep
									   , NULL //PureGL2_Vidlib_RenderIsInstanced
									   , NULL // PureGL2_Vidlib_VidlibRenderAllowsCopy
									   , ogl_PureGL2_Vidlib_SetDisplayCursor
};

RENDER3D_INTERFACE Render3d = {
	ogl_GetRenderTransform
										, NULL
                              , ogl_GetViewVolume
};

#undef GetDisplayInterface
#undef DropDisplayInterface

POINTER  CPROC ogl_GetDisplayInterface (void)
{
   return (POINTER)&VidInterface;
}

void  CPROC ogl_DropDisplayInterface (POINTER p)
{
}

#undef GetDisplay3dInterface
POINTER CPROC ogl_GetDisplay3dInterface (void)
{
	return (POINTER)&Render3d;
}

void  CPROC ogl_DropDisplay3dInterface (POINTER p)
{
}


RENDER_NAMESPACE_END

