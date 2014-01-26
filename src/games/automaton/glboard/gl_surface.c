



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
                                       , (PRENDERER CPROC (*)(_32, _32, _32, S_32, S_32)) OpenDisplaySizedAt
                                       , (PRENDERER CPROC (*)(_32, _32, _32, S_32, S_32, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void CPROC (*)(PRENDERER)) CloseDisplay
                                       , (void CPROC (*)(PRENDERER, S_32, S_32, _32, _32 DBG_PASS)) UpdateDisplayPortionEx
                                       , (void CPROC (*)(PRENDERER)) UpdateDisplay
                                       , GetDisplayPosition
                                       , (void CPROC (*)(PRENDERER, S_32, S_32)) MoveDisplay
                                       , (void CPROC (*)(PRENDERER, S_32, S_32)) MoveDisplayRel
                                       , (void CPROC (*)(PRENDERER, _32, _32)) SizeDisplay
                                       , (void CPROC (*)(PRENDERER, S_32, S_32)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void CPROC (*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image CPROC (*)(PRENDERER)) GetDisplayImage
                                       , (void CPROC (*)(PRENDERER, CloseCallback, PTRSZVAL)) SetCloseHandler
                                       , (void CPROC (*)(PRENDERER, MouseCallback, PTRSZVAL)) SetMouseHandler
                                       , (void CPROC (*)(PRENDERER, RedrawCallback, PTRSZVAL)) SetRedrawHandler
                                       , (void CPROC (*)(PRENDERER, KeyProc, PTRSZVAL)) SetKeyboardHandler
                                       , (void CPROC (*)(PRENDERER, LoseFocusCallback, PTRSZVAL)) SetLoseFocusHandler, NULL   // default callback
                                       , (void CPROC (*)(S_32 *, S_32 *)) GetMousePosition
                                       , (void CPROC (*)(PRENDERER, S_32, S_32)) SetMousePosition
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




