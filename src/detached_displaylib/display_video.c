#include <stdhdrs.h>
#include <final_types.h>
#include <sharemem.h>
#include <stddef.h>
#include <timers.h>
//#include <display.h>

#define DISPLAY_PROC RENDER_PROC
#include "global.h"
#include "displaystruc.h"
#include <render.h>


RENDER_NAMESPACE
//--------------------------------------------------------------------------

RENDER_PROC( void, SetApplicationTitle)( const char *blah )
{
#ifdef _WIN32
	//SetApplicationTitle( blah );
#endif
}

RENDER_PROC( void, SetRendererTitle)( PRENDERER renderer, const char *blah )
{
#ifdef _WIN32
	//SetApplicationTitle( blah );
#endif
}

//--------------------------------------------------------------------------

RENDER_PROC( void, SetApplicationIcon )( Image icon )
{
#ifdef _WIN32
	//SetApplicationIcon( icon );
#endif
}

//--------------------------------------------------------------------------

// might conflict with video library...
// but on a windows system direct linking to this
// library is unlikely - most likely to just use the public interface.
#if defined( _WIN32 ) 
//static 
#endif
RENDER_PROC( Image, GetDisplayImage )( PRENDERER img )
{
	return img?(((PPANEL)img)->common.StableImage):NULL;
}


//--------------------------------------------------------------------------

RENDER_PROC( void, SetCloseHandler )( PRENDERER img, CloseCallback method, PTRSZVAL psv )
{
	((PPANEL)img)->CloseMethod = method;
	((PPANEL)img)->psvClose = psv;
}

//--------------------------------------------------------------------------

RENDER_PROC( void, SetMouseHandler )( PRENDERER img, MouseCallback method, PTRSZVAL psv )
{
	((PPANEL)img)->MouseMethod = method;
	((PPANEL)img)->psvMouse = psv;
}

//--------------------------------------------------------------------------

RENDER_PROC( void, SetRedrawHandler )( PRENDERER img, RedrawCallback method, PTRSZVAL psv )
{
	((PPANEL)img)->RedrawMethod = method;
	((PPANEL)img)->psvRedraw = psv;
}

//--------------------------------------------------------------------------

RENDER_PROC( void, SetKeyboardHandler )( PRENDERER img, KeyProc method, PTRSZVAL psv )
{
	((PPANEL)img)->KeyMethod = method;
	((PPANEL)img)->psvKey = psv;
}

//--------------------------------------------------------------------------

RENDER_PROC( void, SetLoseFocusHandler )( PRENDERER img, LoseFocusCallback method, PTRSZVAL psv )
{
	((PPANEL)img)->FocusMethod = method;
	((PPANEL)img)->psvFocus = psv;
#if 0
   // exisiting vidlib does not inform of focus change during this stage.
	if( method )
	{
		if( g.pFocusedPanel == img )
			method( psv, NULL ); // well tell the display it's focused - it wants to know...
      else
			method( psv, img ); // well... non zero is lose focus...
			// and who better to claim loss than the requestor.
	}
#endif
}

//--------------------------------------------------------------------------

//RENDER_PROC( void, SetDefaultHandler )( PRENDERER img, GeneralCallback method, PTRSZVAL psv )
//{
//	((PPANEL)img)->DefaultMethod = method;
//	((PPANEL)img)->psvDefault = psv;
//}

//--------------------------------------------------------------------------

DISPLAY_PROC( LOGICAL, HasFocus )( PPANEL img )
{
	return ((PPANEL)img)->common.flags.active;
}

//--------------------------------------------------------------------------

DISPLAY_PROC( LOGICAL, DisplayIsValid )( PRENDERER renderer )
{
	return TRUE;
}

//--------------------------------------------------------------------------

DISPLAY_PROC( void, SyncRender )( PRENDERER renderer )
{
   // when using this directly everythingg is always sync'd
   return;
}

//--------------------------------------------------------------------------
#if 0
static char CPROC DisplayGetKeyText( int key )
{
	return 0;
}
#endif
//--------------------------------------------------------------------------

#if ACTIVE_MESSAGE_IMPLEMENTED
RENDER_PROC( int, SendActiveMessage )( PRENDERER dest, PACTIVEMESSAGE msg )
{
	if( ((PPANEL)dest)->DefaultMethod )
	{
		((PPANEL)dest)->DefaultMethod( ((PPANEL)dest)->psvDefault, dest, msg );
		return TRUE;
	}	
	return FALSE;
}
#endif
//--------------------------------------------------------------------------
RENDER_PROC( void, ForceDisplayFocus )( PRENDERER display )
{
}

// display set as topmost within it's group (normal/bottommost/topmost)
RENDER_PROC( void, ForceDisplayFront )( PRENDERER display )
{
}
// display is force back one layer... or forced to bottom?
// alt-n pushed the display to the back... alt-tab is different...
RENDER_PROC( void, ForceDisplayBack )( PRENDERER display )
{
}
//--------------------------------------------------------------------------
#if ACTIVE_MESSAGE_IMPLEMENTED

// could/should pass information related ot the message created
// next time?
RENDER_PROC( PACTIVEMESSAGE, CreateActiveMessage )( int ID, int size, ... )
{
	/*
	PACTIVEMESSAGE msg;
	if( ID >= ACTIVE_MSG_USER )
	{
		msg = Allocate( offsetof( ACTIVEMESSAGE, data ) + size );
		msg.size = size;
	}
	else
	{
		switch( ID )
		{
		case ACTIVE_MSG_PING:
		case ACTIVE_MSG_PONG:
			msg = Allocate( offsetof( ACTIVEMESSAGE, data ) + sizeof(msg.data.ping) );
			MemCpy( (POINTER)msg.data.raw, &size + 1, sizeof(msg.data.ping) );
			break;
		case ACTIVE_MSG_MOUSE:
			msg = Allocate( offsetof( ACTIVEMESSAGE, data ) + sizeof(msg.data.mouse) );
			MemCpy( (POINTER)msg.data.raw, &size + 1, sizeof(msg.data.mouse) );
			break;
		case ACTIVE_MSG_GAIN_FOCUS:
		case ACTIVE_MSG_LOSE_FOCUS:
			msg = Allocate( offsetof( ACTIVEMESSAGE, data ) + sizeof(msg.data.gain_focus) );
			MemCpy( (POINTER)msg.data.raw, &size + 1, sizeof(msg.data.gain_focus) );
			break;
		case ACTIVE_MSG_KEY:
			msg = Allocate( offsetof( ACTIVEMESSAGE, data ) + sizeof(msg.data.key) );
			MemCpy( (POINTER)msg.data.raw, &size + 1, sizeof(msg.data.key) );
			break;
		default:
			msg = Allocate( offsetof( ACTIVEMESSAGE, data ) );
			break;
		}
	}
	msg.ID = ID;
	return msg;
	*/
	return NULL;
}
#endif
//--------------------------------------------------------------------------

RENDER_PROC( void, MakeTopmost )(PRENDERER pRenderer )
{
	// uhmm... yeah. and stuff.
}
RENDER_PROC( int, IsTopmost )(PRENDERER pRenderer )
{
	return 0;
	// uhmm... yeah. and stuff.
}
//--------------------------------------------------------------------------

#ifdef __LINUX__
int EnableOpenGL       ( PRENDERER hVideo )
{
#ifndef __NO_SDL__
	extern CRITICALSECTION csThreadMessage;
// LINUX opengl, also means SDL opengl...
	EnterCriticalSec( &csThreadMessage );

	g.surface = SDL_SetVideoMode( g.width, g.height
#ifdef __ARM__
						 , 16
#else
						 , 32
#endif
						 , SDL_NOFRAME|SDL_OPENGL|SDL_OPENGLBLIT );
	g.pRootPanel->flags.bOpenGL = 1;
	g.RealSurface = BuildImageFile( (PCOLOR)g.surface->pixels, g.width, g.height );
#ifdef INTERNAL_BUFFER
	lprintf( WIDE("Internal buffer, soft image surface...") );
	//g.SoftSurface = MakeImageFile( g.width, g.height );
	//ClearImageTo( g.SoftSurface, BASE_COLOR_BLACK );
	//g.pRootPanel->common.RealImage = g.SoftSurface;
#else
	lprintf( WIDE("No internal buffer... redundant?") );
	g.pRootPanel->common.RealImage = g.RealSurface;
#endif
	LeaveCriticalSec( &csThreadMessage );
#endif
	return FALSE;
}

int EnableOpenGLView( PRENDERER hVideo, int x, int y, int w, int h )
{
	return EnableOpenGL(hVideo);
}
int SetActiveGLDisplayView( PRENDERER hDisplay, int nFracture )
{
	return 0;
}

int SetActiveGLDisplay ( PRENDERER hDisplay )
{
	return FALSE;
}
#endif

//--------------------------------------------------------------------------

void WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
{
	// nothing to do here?
}

RENDER_PROC(void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hContainer)
{
	// erm...
}

#ifdef WIN32
RENDER_PROC (PRENDERER, MakeDisplayFrom) (HWND hWnd)
{
	return NULL;
}
#endif


RENDER_DATA( RENDER_INTERFACE, VideoInterface ) =
{
	// one should never use the result of open display
	// it will be non zero (and also coincidentally the pointer to the
	// root panel - if int is large enough to store the pointer.
	(int CPROC (*)(void))InitDisplay

	,SetApplicationTitle
	,SetApplicationIcon 
	,GetDisplaySize
	,SetDisplaySize     
                   
	,(PRENDERER  CPROC (*)( _32, _32, _32, S_32, S_32 ))OpenDisplaySizedAt
	,(PRENDERER  CPROC (*)( _32, _32, _32, S_32, S_32, PRENDERER ))OpenDisplayAboveSizedAt
                   
	,(void CPROC (*)(PRENDERER))CloseDisplay
                   
	,(void CPROC (*)(PRENDERER,S_32,S_32,_32,_32 DBG_PASS))UpdateDisplayPortionEx
	,(void CPROC (*)(PRENDERER DBG_PASS))UpdateDisplayEx
                   

	,GetDisplayPosition
	,(void CPROC (*)(PRENDERER,S_32,S_32))MoveDisplay
	,(void CPROC (*)(PRENDERER,S_32,S_32))MoveDisplayRel
	,(void CPROC (*)(PRENDERER,_32,_32))SizeDisplay
	,SizeDisplayRel
	,MoveSizeDisplayRel
	,(void CPROC (*)(PRENDERER,PRENDERER))PutDisplayAbove
                   
	,GetDisplayImage           
                   
	,SetCloseHandler
	,SetMouseHandler
	,SetRedrawHandler
	,SetKeyboardHandler
	,SetLoseFocusHandler
	,NULL //SetDefaultHandler

	, GetMousePosition
	, SetMousePosition

        ,(LOGICAL CPROC (*)(PRENDERER))HasFocus

	, NULL //SendActiveMessage
        , NULL //CreateActiveMessage
	, GetKeyText
	, IsKeyDown
	, KeyDown
  	, DisplayIsValid
, OwnMouseEx
	, NULL //BeginCalibration // begincalib
, SyncRender
#ifdef __LINUX__
, EnableOpenGL // EnableOpenGL
, SetActiveGLDisplay // SetActiveGLDisplay
#else
, NULL
, NULL
#endif
, MoveSizeDisplay
, MakeTopmost // MakeTopMost
, HideDisplay // HIdeDisplay
, RestoreDisplay // RestoreDisplay
, ForceDisplayFocus // ForceDisplaYFocus
, ForceDisplayFront
, ForceDisplayBack
,BindEventToKey
, UnbindKey
, IsTopmost
, NULL // OkaySyncRender
, IsTouchDisplay
, GetMouseState
, EnableSpriteMethod
, WinShell_AcceptDroppedFiles
, PutDisplayIn
#ifdef WIN32
, MakeDisplayFrom
#endif
, SetRendererTitle
, DisableMouseOnIdle
, OpenDisplayAboveUnderSizedAt
, SetDisplayNoMouse
, Redraw
};

extern LOGICAL InitMemory( void );

DISPLAY_PROC( void, DisableMouseOnIdle)(PRENDERER hVideo, LOGICAL bEnable)
{
 return;
}

DISPLAY_PROC( void, SetDisplayNoMouse)(PRENDERER hVideo, int bNoMouse)
{
 return;
}

DISPLAY_PROC( POINTER, DisplayGetDisplayInterface )( void )
{
#ifdef USE_RENDER_INTERFACE
	if( !InitMemory() )
	{
		// this is harsh - but we MUST evaluate why this happened.
		return NULL;
	}
	//DebugBreak();
	VideoInterface._ProcessDisplayMessages = g.RenderInterface->_ProcessDisplayMessages;
#endif
	return &VideoInterface;
}

DISPLAY_PROC( void, DisplayDropDisplayInterface )( POINTER p )
{
}

RENDER_NAMESPACE_END

