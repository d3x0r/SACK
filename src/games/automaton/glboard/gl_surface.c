



typedef struct panel_surface_tag {
	Image surface;
	// no frames, this should be able to be layered
	// under psi as it's display interface...

  // which means this needs to evolve from render_interface_tag
} PANEL_SURFACE, *PPANEL_SURFACE;





//----------------------------------------------------------------------------

#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay
                                       , SetApplicationTitle
                                       , (void CPROC (*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , (PRENDERER CPROC (*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t)) OpenDisplaySizedAt
                                       , (PRENDERER CPROC (*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void CPROC (*)(PRENDERER)) CloseDisplay
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t, uint32_t, uint32_t DBG_PASS)) UpdateDisplayPortionEx
                                       , (void CPROC (*)(PRENDERER)) UpdateDisplay
                                       , GetDisplayPosition
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) MoveDisplay
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) MoveDisplayRel
                                       , (void CPROC (*)(PRENDERER, uint32_t, uint32_t)) SizeDisplay
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void CPROC (*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image CPROC (*)(PRENDERER)) GetDisplayImage
                                       , (void CPROC (*)(PRENDERER, CloseCallback, uintptr_t)) SetCloseHandler
                                       , (void CPROC (*)(PRENDERER, MouseCallback, uintptr_t)) SetMouseHandler
                                       , (void CPROC (*)(PRENDERER, RedrawCallback, uintptr_t)) SetRedrawHandler
                                       , (void CPROC (*)(PRENDERER, KeyProc, uintptr_t)) SetKeyboardHandler
                                       , (void CPROC (*)(PRENDERER, LoseFocusCallback, uintptr_t)) SetLoseFocusHandler, NULL   // default callback
                                       , (void CPROC (*)(int32_t *, int32_t *)) GetMousePosition
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) SetMousePosition
                                       , HasFocus  // has focus
                                       , NULL         // SendMessage
                                       , NULL         // CrateMessage
                                       , GetKeyText
                                       , IsKeyDown
                                       , KeyDown
                                       , DisplayIsValid
                                       , OwnMouseEx
                                       , BeginCalibration
                                       , SyncRender   // sync
                                       , EnableOpenGL
                                       , SetActiveGLDisplay
                                       , MoveSizeDisplay
                                       , MakeTopmost
                                       , HideDisplay
                                       , RestoreDisplay
                                       , ForceDisplayFocus
                                       , ForceDisplayForward
                                       , ForceDisplayBack
                                       , BindEventToKey
                                       , UnbindKey
};

#undef GetDisplayInterface
#undef DropDisplayInterface

RENDER_PROC (PRENDER_INTERFACE, GetDisplayInterface) (void)
{
   InitDisplay();
   return &VidInterface;
}

RENDER_PROC (void, DropDisplayInterface) (void)
{
}




