/****************
 * Some performance concerns regarding high numbers of layered windows...
 * every 1000 windows causes 2 - 12 millisecond delays...
 *    the first delay is between CreateWindow and WM_NCCREATE
 *    the second delay is after ShowWindow after WM_POSCHANGING and before POSCHANGED (window manager?)
 * this means each window is +24 milliseconds at least at 1000, +48 at 2000, etc...
 * it does seem to be a linear progression, and the lost time is
 * somewhere within the windows system, and not user code.
 ************************************************/

#define X11_VIDLIB
#ifdef __LINUX__
// this was originally implemented when porting to CLI/CLR, but should work good for linux
#define __NO_WIN32API__
#endif

//#define _OPENGL_ENABLED
/* this must have been done for some other collision in some other bit of code...
 * probably the update queue? the mosue queue ?
 */
//#define USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
//#define USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS

#ifdef UNDER_CE
#define NO_MOUSE_TRANSPARENCY
#define NO_ENUM_DISPLAY
#define NO_DRAG_DROP
#define NO_TRANSPARENCY
#undef _OPENGL_ENABLED
#else
#define USE_KEYHOOK
#endif

#ifdef _MSC_VER
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#endif

#define NEED_VECTLIB_COMPARE
#define NEED_REAL_IMAGE_STRUCTURE

#include <stdhdrs.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */
#ifdef _WIN32
#include "../glext.h"
#endif

#include <sqlgetoption.h>
#include <deadstart.h>
#include <stdio.h>
#include <string.h>
#include <timers.h>
#include <sharemem.h>
//#define NO_LOGGING
#include <logging.h>
#include <msgclient.h>
#include <idle.h>
//#include <imglib/imagestruct.h>
#undef StrDup

#ifdef _WIN32
#include <shlwapi.h> // have to include this if shellapi.h is included (for mingw)
#include <shellapi.h> // very last though - this is DragAndDrop definitions...
#endif

// this is safe to leave on.
#define LOG_ORDERING_REFOCUS

//#define LOG_MOUSE_EVENTS
//#define LOG_RECT_UPDATE
//#define LOG_DESTRUCTION
//#define LOG_STARTUP
//#define LOG_FOCUSEVENTS
//#define OTHER_EVENTS_HERE
//#define LOG_SHOW_HIDE
//#define LOG_DISPLAY_RESIZE
//#define NOISY_LOGGING
// related symbol needs to be defined in KEYDEFS.C
//#define LOG_KEY_EVENTS
#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_OPENGL_CONTEXT
#include <vidlib/vidstruc.h>
#include <render3d.h>
#define NEED_VECTLIB_COMPARE
#include <image.h>
#include <vectlib.h>
//#include "vidlib.H"

#include <keybrd.h>

static int stop;
//HWND     hWndLastFocus;

// commands to the video thread for non-windows native ....
#define CREATE_VIEW 1


#define WM_RUALIVE 5000 // lparam = pointer to alive variable expected to set true
#define WD_HVIDEO   0   // WindowData_HVIDEO


IMAGE_NAMESPACE

struct saved_location
{
	S_32 x, y;
	_32 w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
	Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h );
	PTRSZVAL psv;
};
IMAGE_NAMESPACE_END

RENDER_NAMESPACE

// move local into render namespace.
#define VIDLIB_MAIN
#include "local.h"

extern KEYDEFINE KeyDefs[];

// forward declaration - staticness will probably cause compiler errors.
static int CPROC OldProcessDisplayMessages(void );
static int CPROC ProcessDisplayMessages( struct display_camera *camera );

//----------------------------------------------------------------------------


#ifdef __LINUX__
/*
 * this code started as a seed from nehe opengl lessons
 *
 * will be heavily modified to fit library requirements
 *
 */

/*
 * This code was created by Jeff Molofee '99 
 * (ported to Linux/GLX by Mihael Vrbanec '00)
 *
 * If you've found this code useful, please let me know.
 *
 * Visit Jeff at http://nehe.gamedev.net/
 * 
 * or for port-specific comments, questions, bugreports etc. 
 * email to Mihael.Vrbanec@stud.uni-karlsruhe.de
 */
 

/* attributes for a single buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
static int attrListSgl[] = {GLX_RGBA, GLX_RED_SIZE, 4, 
    GLX_GREEN_SIZE, 4, 
    GLX_BLUE_SIZE, 4, 
    GLX_DEPTH_SIZE, 16,
    None};

/* attributes for a double buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
static int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 
    GLX_RED_SIZE, 4, 
    GLX_GREEN_SIZE, 4, 
    GLX_BLUE_SIZE, 4, 
    GLX_DEPTH_SIZE, 16,
    None };

//GLWindow GLWin;

/* function called when our window is resized (should only happen in window mode) */
void resizeGLScene(unsigned int width, unsigned int height)
{
    if (height == 0)    /* Prevent A Divide By Zero If The Window Is Too Small */
        height = 1;
    glViewport(0, 0, width, height);    /* Reset The Current Viewport And Perspective Transformation */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

/* general OpenGL initialization function */
int initGL(GLWindow *GLWin )
{
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    /* we use resizeGLScene once to set up our initial perspective */
    resizeGLScene(GLWin->width, GLWin->height);
    glFlush();
    return True;
}

void RenderGL( struct display_camera *camera );

/* Here goes our drawing code */
int drawGLScene( struct display_camera *camera, GLWindow *GLWin)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    Move( l.origin );
    RenderGL( camera );
    
    if (GLWin->doubleBuffered)
    {
        glXSwapBuffers(GLWin->dpy, GLWin->win);
    }
    return True;    
}

/* function to release/destroy our resources and restoring the old desktop */
GLvoid killGLWindow(GLWindow *GLWin )
{
    if (GLWin->ctx)
    {
        if (!glXMakeCurrent(GLWin->dpy, None, NULL))
        {
            printf("Could not release drawing context.\n");
        }
        glXDestroyContext(GLWin->dpy, GLWin->ctx);
        GLWin->ctx = NULL;
    }
    /* switch back to original desktop resolution if we were in fs */
    if (GLWin->fs)
    {
        XF86VidModeSwitchToMode(GLWin->dpy, GLWin->screen, &GLWin->deskMode);
        XF86VidModeSetViewPort(GLWin->dpy, GLWin->screen, 0, 0);
    }
    XCloseDisplay(GLWin->dpy);
}

/* this function creates our window and sets it up properly */
/* FIXME: bits is currently unused */
GLWindow * createGLWindow(struct display_camera *camera)
{
   GLWindow *x11_gl_window = New( GLWindow );
    XVisualInfo *vi;
    Colormap cmap;
    int dpyWidth, dpyHeight;
    int i;
    int glxMajorVersion, glxMinorVersion;
    int vidModeMajorVersion, vidModeMinorVersion;
    XF86VidModeModeInfo **modes;
    int modeNum;
    int bestMode;
    Atom wmDelete;
    Window winDummy;
    unsigned int borderDummy;
    
    x11_gl_window->fs = 0;//fullscreenflag;
    /* set best mode to current */
    bestMode = 0;
    /* get a connection */
		x11_gl_window->dpy = XOpenDisplay(0);
		if( x11_gl_window->dpy )
		{
			x11_gl_window->atom_create = XInternAtom( x11_gl_window->dpy, "UserCreateWindow", 0 );

			x11_gl_window->screen = DefaultScreen(x11_gl_window->dpy);
			XF86VidModeQueryVersion(x11_gl_window->dpy, &vidModeMajorVersion,
		        &vidModeMinorVersion);
		    printf("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion,
		        vidModeMinorVersion);
		    XF86VidModeGetAllModeLines(x11_gl_window->dpy, x11_gl_window->screen, &modeNum, &modes);
		    /* save desktop-resolution before switching modes */
		    x11_gl_window->deskMode = *modes[0];
    /* look for mode with requested resolution */
    for (i = 0; i < modeNum; i++)
    {
        if ((modes[i]->hdisplay == camera->w) && (modes[i]->vdisplay == camera->h))
        {
            bestMode = i;
        }
    }
    /* get an appropriate visual */
    vi = glXChooseVisual(x11_gl_window->dpy, x11_gl_window->screen, attrListDbl);
    if (vi == NULL)
    {
        vi = glXChooseVisual(x11_gl_window->dpy, x11_gl_window->screen, attrListSgl);
        x11_gl_window->doubleBuffered = False;
        printf("Only Singlebuffered Visual!\n");
    }
    else
    {
        x11_gl_window->doubleBuffered = True;
        printf("Got Doublebuffered Visual!\n");
    }
    glXQueryVersion(x11_gl_window->dpy, &glxMajorVersion, &glxMinorVersion);
    printf("glX-Version %d.%d\n", glxMajorVersion, glxMinorVersion);
    /* create a GLX context */
    x11_gl_window->ctx = glXCreateContext(x11_gl_window->dpy, vi, 0, GL_TRUE);
    /* create a color map */
    cmap = XCreateColormap(x11_gl_window->dpy, RootWindow(x11_gl_window->dpy, vi->screen),
        vi->visual, AllocNone);
    x11_gl_window->attr.colormap = cmap;
    x11_gl_window->attr.border_pixel = 0;

    x11_gl_window->attr.event_mask = ExposureMask
        | KeyPressMask
        | KeyReleaseMask
        | ButtonPressMask
        | ButtonReleaseMask
        | ButtonMotionMask
        | PointerMotionMask
        | StructureNotifyMask;

    if (x11_gl_window->fs)
    {
        XF86VidModeSwitchToMode(x11_gl_window->dpy, x11_gl_window->screen, modes[bestMode]);
        XF86VidModeSetViewPort(x11_gl_window->dpy, x11_gl_window->screen, 0, 0);
        dpyWidth = modes[bestMode]->hdisplay;
        dpyHeight = modes[bestMode]->vdisplay;
        printf("Resolution %dx%d\n", dpyWidth, dpyHeight);
        XFree(modes);
    
        /* create a fullscreen window */
        x11_gl_window->attr.override_redirect = True;
        x11_gl_window->win = XCreateWindow(x11_gl_window->dpy, RootWindow(x11_gl_window->dpy, vi->screen),
            0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
            &x11_gl_window->attr);
        XWarpPointer(x11_gl_window->dpy, None, x11_gl_window->win, 0, 0, 0, 0, 0, 0);
		XMapRaised(x11_gl_window->dpy, x11_gl_window->win);
        XGrabKeyboard(x11_gl_window->dpy, x11_gl_window->win, True, GrabModeAsync,
            GrabModeAsync, CurrentTime);
        XGrabPointer(x11_gl_window->dpy, x11_gl_window->win, True, ButtonPressMask,
            GrabModeAsync, GrabModeAsync, x11_gl_window->win, None, CurrentTime);
        
    }
    else
    {
        /* create a window in window mode*/
        x11_gl_window->win = XCreateWindow(x11_gl_window->dpy, RootWindow(x11_gl_window->dpy, vi->screen),
            0, 0, camera->w, camera->h, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask, &x11_gl_window->attr);
        /* only set window title and handle wm_delete_events if in windowed mode */
        wmDelete = XInternAtom(x11_gl_window->dpy, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(x11_gl_window->dpy, x11_gl_window->win, &wmDelete, 1);
        XSetStandardProperties(x11_gl_window->dpy, x11_gl_window->win, "No Title",
            "No Title", None, NULL, 0, NULL);
        XMapRaised(x11_gl_window->dpy, x11_gl_window->win);
    }       
    /* connect the glx-context to the window */
    glXMakeCurrent(x11_gl_window->dpy, x11_gl_window->win, x11_gl_window->ctx);
    XGetGeometry(x11_gl_window->dpy, x11_gl_window->win, &winDummy, &x11_gl_window->x, &x11_gl_window->y,
        &x11_gl_window->width, &x11_gl_window->height, &borderDummy, &x11_gl_window->depth);
    printf("Depth %d\n", x11_gl_window->depth);
    if (glXIsDirect(x11_gl_window->dpy, x11_gl_window->ctx)) 
        printf("Congrats, you have Direct Rendering!\n");
    else
        printf("Sorry, no Direct Rendering possible!\n");
	 initGL( x11_gl_window );
            }
            else
            {
                lprintf( "Failed to open display" );
                return NULL;
            }
    return x11_gl_window;
}

#endif





LOGICAL InMyChain( PRENDERER hVideo, HWND hWnd )
{
	PRENDERER base;
	int count = 0 ;
	base = hVideo;
	while( base->pBelow ) 
	{
		count++;
		if( count > 100 )
			DebugBreak();
		base = base->pBelow;
	}
	while( base )
	{
	#ifdef WIN32
		if( base->hWndOutput== hWnd )
			return 1;
	#endif
      base = base->pAbove;
	}
   return 0;
}
void DumpMyChain( PRENDERER hVideo DBG_PASS )
#define DumpMyChain(h) DumpMyChain( h DBG_SRC )
{
#ifdef _WIN32
#ifndef UNDER_CE
	PRENDERER base;
	base = hVideo;
	// follow to the lowest, and chain upwards...
	while( base->pAbove ) base = base->pAbove;
	_lprintf(DBG_RELAY)( WIDE( "bottommost" ) );
	while( base )
	{
		char title[256];
		char classname[256];
		//GetClassName( base->hWndOutput, classname, sizeof( classname ) );
		//GetWindowText( base->hWndOutput, title, sizeof( title ) );
		//if( base == hVideo )
		// _lprintf(DBG_RELAY)( WIDE( "--> %p[%p] %s" ), base, base->hWndOutput, title );
		//else
		//	_lprintf(DBG_RELAY)( WIDE( "    %p[%p] %s" ), base, base->hWndOutput, title );
      base = base->pBelow;
	}
	lprintf( WIDE( "topmost" ) );
#endif
#endif
}

void DumpChainAbove( PRENDERER chain, HWND hWnd )
{
#ifdef _WIN32
#ifndef UNDER_CE
	int not_mine = 0;
	char title[256];
   GetWindowText( hWnd, title, sizeof( title ) );
	lprintf( WIDE( "Dumping chain of windows above %p %s" ), hWnd, title );
	while( hWnd = GetNextWindow( hWnd, GW_HWNDPREV ) )
	{
	}
#endif
#endif
}

void DumpChainBelow( PRENDERER chain, HWND hWnd DBG_PASS )
#define DumpChainBelow(c,h) DumpChainBelow(c,h DBG_SRC)
{
#ifdef _WIN32
#ifndef UNDER_CE
	int not_mine = 0;
	char title[256];
	GetWindowText( hWnd, title, sizeof( title ) );
	_lprintf(DBG_RELAY)( WIDE( "Dumping chain of windows below %p" ), hWnd );
	while( hWnd = GetNextWindow( hWnd, GW_HWNDNEXT ) )
	{
		int ischain;
		char classname[256];
		GetClassName( hWnd, classname, sizeof( classname ) );
		GetWindowText( hWnd, title, sizeof( title ) );
		if( ischain = InMyChain( chain, hWnd ) )
		{
			lprintf( WIDE( "%s %p %s %s" ), ischain==2?WIDE( ">>>" ):WIDE( "^^^" ), hWnd, title, classname );
			not_mine = 0;
		}
		else
		{
			not_mine++;
			if( not_mine < 10 )
				lprintf( WIDE( "... %p %s %s" ), hWnd, title, classname );
		}
	}
#endif
#endif
}

//----------------------------------------------------------------------------

void SafeSetFocus( HWND hWndSetTo )
{
#ifdef _WIN32
	DWORD dwProc1;
	DWORD dwThread2;
	DWORD dwThreadMe = 0;
	if( hWndSetTo != GetForegroundWindow() )
	{
		dwThread2 = GetWindowThreadProcessId(
														 hWndSetTo
														,NULL);
		dwProc1 = GetWindowThreadProcessId(
													  GetForegroundWindow()
													 ,NULL);
		dwThreadMe = GetCurrentThreadId();
#ifndef UNDER_CE
		if( dwThreadMe != dwProc1 )
			AttachThreadInput(dwProc1, dwThreadMe
								  ,TRUE);
		if( dwThreadMe != dwThread2 && dwThread2 != dwProc1 )
			AttachThreadInput( dwThread2, dwThreadMe
								  ,TRUE);
#endif
		//Detach the attached thread
	}
   //lprintf( WIDE( "Safe Set Focus %p" ), hWndSetTo );
	SetFocus( hWndSetTo );
	SetActiveWindow( hWndSetTo );
	SetForegroundWindow( hWndSetTo );

	if( dwThreadMe )
	{
#ifndef UNDER_CE
		if( dwThreadMe != dwProc1 )
			AttachThreadInput(dwProc1, dwThreadMe
								  ,FALSE);
		if( dwThreadMe != dwThread2 && dwThread2 != dwProc1 )
			AttachThreadInput( dwThread2, dwThreadMe
								  ,FALSE);
#endif
	}
#endif
}


//----------------------------------------------------------------------------
void  EnableLoggingOutput( LOGICAL bEnable )
{
   l.flags.bLogWrites = bEnable;
}

void  UpdateDisplayPortionEx( PRENDERER hVideo
                                          , S_32 x, S_32 y
                                          , _32 w, _32 h DBG_PASS)
{

   if( hVideo )
		hVideo->flags.bShown = 1;
	l.flags.bUpdateWanted = 1;
}

//----------------------------------------------------------------------------

void
UnlinkVideo (PRENDERER hVideo)
{
	// yes this logging is correct, to say what I am below, is to know what IS above me
	// and to say what I am above means I nkow what IS below me
//#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( " -- UNLINK Video %p from which is below %p and above %p" ), hVideo, hVideo->pAbove, hVideo->pBelow );
//#endif
	//if( hVideo->pBelow || hVideo->pAbove )
	//   DebugBreak();
	if (hVideo->pBelow)
	{
		hVideo->pBelow->pAbove = hVideo->pAbove;
	}
	else
		l.top = hVideo->pAbove;
	if (hVideo->pAbove)
	{
		hVideo->pAbove->pBelow = hVideo->pBelow;
	}
	else
		l.bottom = hVideo->pBelow;


	hVideo->pPrior = hVideo->pNext = hVideo->pAbove = hVideo->pBelow = NULL;
}

//----------------------------------------------------------------------------

void
FocusInLevel (PRENDERER hVideo)
{
   lprintf( WIDE( "Focus IN level" ) );
   if (hVideo->pPrior)
   {
      hVideo->pPrior->pNext = hVideo->pNext;
      if (hVideo->pNext)
         hVideo->pNext->pPrior = hVideo->pPrior;

      hVideo->pPrior = NULL;

      if (hVideo->pAbove)
      {
         hVideo->pNext = hVideo->pAbove->pBelow;
         hVideo->pAbove->pBelow->pPrior = hVideo;
         hVideo->pAbove->pBelow = hVideo;
      }
      else        // nothing points to this - therefore we must find the start
      {
         PRENDERER pCur = hVideo->pPrior;
         while (pCur->pPrior)
            pCur = pCur->pPrior;
         pCur->pPrior = hVideo;
         hVideo->pNext = pCur;
       }
		hVideo->pPrior = NULL;
   }
   // else we were the first in this level's chain...
}

//----------------------------------------------------------------------------

void  PutDisplayAbove (PRENDERER hVideo, PRENDERER hAbove)
{
	//  this above that...
	//  this->below is now that // points at things below
	// that->above = this // points at things above
#ifdef LOG_ORDERING_REFOCUS
	PRENDERER original_below = hVideo->pBelow;
	PRENDERER original_above = hVideo->pAbove;
#endif
	PRENDERER topmost = hAbove;

	if( !l.bottom )
	{
		if( hAbove )
			lprintf( WIDE( "Failure, no bottom, but somehow a second display is already known?" ) );
		l.bottom = hVideo;
		l.top = hVideo;
		return;
	}

#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( "Begin Put Display Above..." ) );
#endif
	if( hVideo->pAbove == hAbove )
		return;
	if( hVideo == hAbove )
		DebugBreak();

	// unlink the video from the stack first.
	if( hVideo->pBelow )
		hVideo->pBelow->pAbove = hVideo->pAbove;
	hVideo->pBelow = NULL;
	if( hVideo->pAbove )
		hVideo->pAbove->pBelow = hVideo->pBelow;
	hVideo->pAbove = NULL;

	// if this is already in a list (like it has pBelow things)
	// I want to insert hAbove between hVideo and pBelow...
	// if if above already has things above it, I want to put those above hvideo
	if( hVideo && hAbove )
	{
		if( hVideo->pBelow = hAbove->pBelow )
		{
			hAbove->pBelow->pAbove = hVideo;
		}
		else
			l.top = hVideo;

		if( hVideo->pAbove )
		{
			lprintf( WIDE( "Windwo was over somethign else and now we die." ) );
			DebugBreak();
		}

		hAbove->pBelow = hVideo;
		hVideo->pAbove = hAbove;

		LeaveCriticalSec( &l.csList );
		return;
	}
	{

		EnterCriticalSec( &l.csList );
		UnlinkVideo (hVideo);      // make sure it's isolated...

		if( ( hVideo->pAbove = topmost ) )
		{
			//HWND hWndOver = GetNextWindow( topmost->hWndOutput, GW_HWNDPREV );
			if( hVideo->pBelow = topmost->pBelow )
			{
				hVideo->pBelow->pAbove = hVideo;
			}
			topmost->pBelow = hVideo;
#ifdef WIN32
#ifdef LOG_ORDERING_REFOCUS
			if( l.flags.bLogFocus )
				lprintf( WIDE( "Only thing here is... %p after %p or %p so use %p means...below?" )
						 , 0//hVideo->hWndOutput
						 , topmost->hWndOutput
						 , topmost->pWindowPos.hwndInsertAfter
						 , GetNextWindow( topmost->hWndOutput, GW_HWNDPREV ) );
#endif
         /*
			SetWindowPos ( hVideo->hWndOutput
							 , topmost->pWindowPos.hwndInsertAfter
							 , 0, 0, 0, 0
							 , SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE
							 );
			*/
#endif
#ifdef LOG_ORDERING_REFOCUS
			if( l.flags.bLogFocus )
				lprintf( WIDE( "Finished ordering..." ) );
			//DumpChainBelow( hVideo, hVideo->hWndOutput );
			//DumpChainAbove( hVideo, hVideo->hWndOutput );
#endif
		}
#ifdef LOG_ORDERING_REFOCUS
		//lprintf(WIDE( "Put Display Above (this)%p above (below)%p and before %p" ), hVideo->hWndOutput, hAbove->hWndOutput, hAbove->pWindowPos.hwndInsertAfter );
#endif
		LeaveCriticalSec( &l.csList );
	}
#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( "End Put Display Above..." ) );
#endif
}

void  PutDisplayIn (PRENDERER hVideo, PRENDERER hIn)
{
   lprintf( WIDE( "Relate hVideo as a child of hIn..." ) );
}

//----------------------------------------------------------------------------

LOGICAL CreateDrawingSurface (PRENDERER hVideo)
{
	if (!hVideo)
		return FALSE;

#if WIN32
	hVideo->pImage =
		RemakeImage( hVideo->pImage, NULL, hVideo->pWindowPos.cx,
						hVideo->pWindowPos.cy );
#else
	hVideo->pImage =
		RemakeImage( hVideo->pImage, NULL, hVideo->WindowPos.cx,
						hVideo->WindowPos.cy );
#endif
	if( !hVideo->transform )
	{
		TEXTCHAR name[64];
		snprintf( name, sizeof( name ), WIDE( "render.display.%p" ), hVideo );
		lprintf( WIDE( "making initial transform" ) );
		hVideo->transform = hVideo->pImage->transform = CreateTransformMotion( CreateNamedTransform( name ) );
	}

#if WIN32
	lprintf( WIDE( "Set transform at %d,%d" ), hVideo->pWindowPos.x, hVideo->pWindowPos.y );
	Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );
#else
	lprintf( WIDE( "Set transform at %d,%d" ), hVideo->WindowPos.x, hVideo->WindowPos.y );
	Translate( hVideo->transform, hVideo->WindowPos.x, hVideo->WindowPos.y, 0 );
#endif
	// additionally indicate that this is a GL render point
	hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER;
	return TRUE;
}

void DoDestroy (PRENDERER hVideo)
{
   if (hVideo)
   {
#ifdef _WIN32
      hVideo->hWndOutput = NULL; // release window... (disallows FreeVideo in user call)
      SetWindowLong (hVideo->hWndOutput, WD_HVIDEO, 0);
#endif
      if (hVideo->pWindowClose)
      {
         hVideo->pWindowClose (hVideo->dwCloseData);
		}

		if( hVideo->over )
			hVideo->over->under = NULL;
		if( hVideo->under )
			hVideo->under->over = NULL;
#ifdef _WIN32
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
#endif
		Release (hVideo->pTitle);
		DestroyKeyBinder( hVideo->KeyDefs );
		// Image library tracks now that someone else gave it memory
		// and it does not deallocate something it didn't allocate...
		UnmakeImageFile (hVideo->pImage);

#ifdef LOG_DESTRUCTION
		lprintf( WIDE( "In DoDestroy, destroyed a good bit already..." ) );
#endif

      // this will be cleared at the next statement....
      // which indicates we will be ready to be released anyhow...
		//hVideo->flags.bReady = FALSE;
      // unlink from the stack of windows...
		UnlinkVideo (hVideo);
		if( l.hCaptured == hVideo )
			l.hCaptured = NULL;
		//Log (WIDE( "Cleared hVideo - is NOW !bReady" ));
		if( !hVideo->flags.event_dispatched )
		{
			int bInDestroy = hVideo->flags.bInDestroy;
			MemSet (hVideo, 0, sizeof (VIDEO));
			// restore this flag... need to keep this so
			// we don't release the structure cuasing a
			// infinite hang while the bit is checked through
			// a released memory pointer.
			hVideo->flags.bInDestroy = bInDestroy;
		}
		else
			hVideo->flags.bReady = 0; // leave as much as we can if in a key...
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

HWND MoveWindowStack( PRENDERER hInChain, HWND hwndInsertAfter, int use_under )
{
   // defered window pos stacking
}

#ifdef _MSC_VER

static int EvalExcept( int n )
{
	switch( n )
	{
	case 		STATUS_ACCESS_VIOLATION:
		if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "Access violation - OpenGL layer at this moment.." ) );
	return EXCEPTION_EXECUTE_HANDLER;
	default:
		if( l.flags.bLogKeyEvent )
			lprintf( WIDE( "Filter unknown : %08X" ), n );

		return EXCEPTION_CONTINUE_SEARCH;
	}
   // unreachable code.
	//return EXCEPTION_CONTINUE_EXECUTION;
}
#endif
//----------------------------------------------------------------------------

 void SendApplicationDraw( PRENDERER hVideo )
{
	// if asked to paint we have definatly been shown.
#ifdef LOG_OPEN_TIMING
	lprintf( WIDE( "Application should redraw... %p" ), hVideo );
#endif
	if( hVideo && hVideo->pRedrawCallback )
	{
		if( !hVideo->flags.bShown || hVideo->flags.bHidden )
		{
#ifdef LOG_SHOW_HIDE
			lprintf(WIDE( " hidden." ) );
#endif
         // oh - opps, it's not allowed to draw.
			return;
		}
		if( hVideo->flags.bOpenGL )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( WIDE( "Auto-enable window GL." ) );
#endif
			//if( hVideo->flags.event_dispatched )
			{
				//lprintf( WIDE( "Fatality..." ) );
				//Return 0;
			}
			//lprintf( WIDE( "Allowed to draw..." ) );
#ifdef _OPENGL_ENABLED
			if( !SetActiveGLDisplay( hVideo ) )
			{
				// if the opengl failed, dont' let the application draw.
				return;
			}
#endif
		}
		hVideo->flags.event_dispatched = 1;
		//					lprintf( WIDE( "Disaptched..." ) );
#ifdef _MSC_VER
		__try
		{
			//try
#elif defined( __WATCOMC__ )
#ifndef __cplusplus
			_try
			{
#endif
#endif
				//if( !hVideo->flags.bShown || !hVideo->flags.bLayeredWindow )
				{
					//HDWP hDeferWindowPos = BeginDeferWindowPos( 1 );
#ifdef NOISY_LOGGING
					lprintf( WIDE( "redraw... WM_PAINT (sendapplicationdraw)" ) );
					lprintf( WIDE( "%p %p %p"), hVideo->pRedrawCallback, hVideo->dwRedrawData, (PRENDERER) hVideo );
#endif
					hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
				}
				//catch(...)
				{
					//lprintf( WIDE( "Unknown exception during Redraw Callback" ) );
				}
#ifdef _MSC_VER
			}
			__except( EvalExcept( GetExceptionCode() ) )
			{
				lprintf( WIDE( "Caught exception in video output window" ) );
				;
			}
#elif defined( __WATCOMC__ )
#ifndef __cplusplus
		}
		_except( EXCEPTION_EXECUTE_HANDLER )
		{
			lprintf( WIDE( "Caught exception in video output window" ) );
			;
		}
#endif
#endif
		if( hVideo->flags.bOpenGL )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( WIDE( "Auto disable (swap) window GL" ) );
#endif
#ifdef _OPENGL_ENABLED
			SetActiveGLDisplay( NULL );
			if( hVideo->flags.bLayeredWindow )
			{
				UpdateDisplay( hVideo );
			}
#endif
		}
		// might have 'controls' over the open...
		// these would need to be updated seperately?
		hVideo->flags.event_dispatched = 0;
		if( hVideo->flags.bShown )
		{
#ifdef NOISY_LOGGING
			lprintf( WIDE( "painting... shown... %p" ), hVideo );
#endif
         // application should issue update display as appropriate.
			//UpdateDisplayPortion(hVideo, 0, 0, 0, 0);
		}
		//else
		//	lprintf( "Not painting... not shown yet..." );
	}
	else if( hVideo )
	{
		// default update
		UpdateDisplay( hVideo );
	}
#ifdef LOG_OPEN_TIMING
	//lprintf( WIDE( "Application should have redrawn..." ) );
#endif
}


int IsVidThread( void )
{
   // used by opengl to allow selecting context.
	if( IsThisThread( l.actual_thread ) )\
		return TRUE;
	return FALSE;
}

void Redraw( PRENDERER hVideo )
{
	if( IsThisThread( hVideo->pThreadWnd ) )
		//if( IsVidThread() )
	{
#ifdef LOG_RECT_UPDATE
		lprintf( WIDE( "..." ) );
#endif
		SendApplicationDraw( hVideo );
	}
	else
	{
		if( l.flags.bLogWrites )
			lprintf( WIDE( "Posting invalidate rect..." ) );
#ifdef _WIN32
		InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
#endif
	}
}


#define MODE_UNKNOWN 0
#define MODE_PERSP 1
#define MODE_ORTHO 2
static int mode = MODE_UNKNOWN;

 void BeginVisPersp( struct display_camera *camera )
{
	//if( mode != MODE_PERSP )
	{
		mode = MODE_PERSP;
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glLoadIdentity();									// Reset The Projection Matrix
		gluPerspective(90.0f,camera->aspect,1.0f,30000.0f);
		glGetDoublev( GL_PROJECTION_MATRIX, l.fProjection );

		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	}
}


 int InitGL( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	if( !camera->flags.init )
	{
		glShadeModel(GL_SMOOTH);							// Enable Smooth Shading

		glEnable( GL_ALPHA_TEST );
		glEnable( GL_BLEND );
 		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		glEnable( GL_TEXTURE_2D );
 		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
 		glEnable(GL_NORMALIZE); // glNormal is normalized automatically....
		glEnable( GL_POLYGON_SMOOTH );
 		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
 
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 
		BeginVisPersp( camera );
		lprintf( "First GL Init Done." );
		camera->flags.init = 1;
		camera->hVidCore->flags.bReady = TRUE;
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer

	return TRUE;										// Initialization Went OK
}


 void InvokeExtraInit( struct display_camera *camera, PTRANSFORM view_camera, RCOORD identity_depth )
{
	PTRSZVAL (CPROC *Init3d)(PTRANSFORM,RCOORD);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( "sack/render/puregl/init3d", &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		Init3d = GetRegisteredProcedureExx( data,(CTEXTSTR)name,PTRSZVAL,"ExtraInit3d",(PTRANSFORM,RCOORD));

		if( Init3d )
		{
			struct plugin_reference *reference;
			PTRSZVAL psvInit = Init3d( view_camera, identity_depth );
			if( psvInit )
			{
				reference = New( struct plugin_reference );
				reference->psv = psvInit;
				reference->name = name;
				{
					static PCLASSROOT draw3d;
					void (CPROC *Draw3d)(PTRSZVAL);
					if( !draw3d )
						draw3d = GetClassRoot( "sack/render/puregl/draw3d" );

					reference->Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"ExtraDraw3d",(PTRSZVAL));
				}
				AddLink( &camera->plugins, reference );
			}
		}
	}
}

void InvokeExtraBeginDraw( CTEXTSTR name, PTRSZVAL psvInit )
{
	static PCLASSROOT draw3d;
	void (CPROC *Draw3d)(PTRSZVAL);
	if( !draw3d )
		draw3d = GetClassRoot( "sack/render/puregl/draw3d" );
	Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"ExtraBeginDraw3d",(PTRSZVAL));
	if( Draw3d )
	{
		Draw3d( psvInit );
	}
}

void InvokeExtraDraw( CTEXTSTR name, PTRSZVAL psvInit )
{
	static PCLASSROOT draw3d;
	void (CPROC *Draw3d)(PTRSZVAL);
	if( !draw3d )
		draw3d = GetClassRoot( "sack/render/puregl/draw3d" );
	Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,"ExtraDraw3d",(PTRSZVAL));
	if( Draw3d )
	{
		Draw3d( psvInit );
	}
}

LOGICAL InvokeExtraMouse( CTEXTSTR name, PTRSZVAL psvInit, PRAY mouse_ray, _32 b )
{
	static PCLASSROOT mouse3d;
	LOGICAL (CPROC *Mouse3d)(PTRSZVAL,PRAY,_32);
	LOGICAL used;
	if( !mouse3d )
		mouse3d = GetClassRoot( "sack/render/puregl/mouse3d" );
	Mouse3d = GetRegisteredProcedureExx( mouse3d, (CTEXTSTR)name,LOGICAL,"ExtraMouse3d",(PTRSZVAL,PRAY,_32));
	if( Mouse3d )
	{
		used = Mouse3d( psvInit, mouse_ray, b );
		if( used )
			return used;
	}
   return FALSE;
}

#if !defined( __WATCOMC__ ) && !defined( __LINUX__ )
static int CPROC Handle3DTouches( PRENDERER hVideo, PTOUCHINPUT touches, int nTouches )
{
	static struct touch_event_state
	{
		struct {
			struct {
				BIT_FIELD bDrag : 1;
			} flags;
			int x;
         int y;
		} one;
		struct {
			int x;
         int y;
		} two;
	} touch_info;
	if( l.flags.bRotateLock )
	{
		int t;
		for( t = 0; t < nTouches; t++ )
		{
			lprintf( "%d %5d %5d ", t, touches[t].x, touches[t].y );
		}
		lprintf( "touch event" );
		if( nTouches == 2 )
		{
         // begin rotate lock
			if( touches[1].dwFlags & TOUCHEVENTF_DOWN )
			{
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
			else if( touches[1].dwFlags & TOUCHEVENTF_UP )
			{
			}
			else
			{
				// drag
				int delx, dely;
				int delx2, dely2;
				int delxt, delyt;
				int delx2t, dely2t;
				lprintf( "drag" );
            delxt = touches[1].x - touches[0].x;
				delyt = touches[1].y - touches[0].y;
            delx2t = touch_info.two.x - touch_info.one.x;
            dely2t = touch_info.two.y - touch_info.one.y;
				delx = -touch_info.one.x + touches[0].x;
            dely = -touch_info.one.y + touches[0].y;
				delx2 = -touch_info.two.x + touches[1].x;
            dely2 = -touch_info.two.y + touches[1].y;
				{
               VECTOR v1,v2,vr;
					RCOORD delta_x = delx / 40.0;
					RCOORD delta_y = dely / 40.0;
					static int toggle;
					v1[vUp] = delyt;
					v1[vRight] = delxt;
               v1[vForward] = 0;
					v2[vUp] = dely2t;
					v2[vRight] = delx2t;
					v2[vForward] = 0;
               normalize( v1 );
               normalize( v2 );
					lprintf( "angle %g", atan2( v2[vUp], v2[vRight] ) - atan2( v1[vUp], v1[vRight] ) );
					RotateRel( l.origin, 0, 0, - atan2( v2[vUp], v2[vRight] ) + atan2( v1[vUp], v1[vRight] ) );
					delta_x /= hVideo->pWindowPos.cx;
					delta_y /= hVideo->pWindowPos.cy;
					if( toggle )
					{
						RotateRel( l.origin, delta_y, 0, 0 );
						RotateRel( l.origin, 0, delta_x, 0 );
					}
					else
					{
						RotateRel( l.origin, 0, delta_x, 0 );
						RotateRel( l.origin, delta_y, 0, 0 );
					}
					toggle = 1-toggle;
				}

            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
		}
		else if( nTouches == 1 )
		{
			if( touches[0].dwFlags & TOUCHEVENTF_DOWN )
			{
            lprintf( "begin" );
				// begin touch
            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
			}
			else if( touches[0].dwFlags & TOUCHEVENTF_UP )
			{
				// release
            lprintf( "done" );
			}
			else
			{
				// drag
				int delx, dely;
				lprintf( "drag" );
				delx = -touch_info.one.x + touches[0].x;
            dely = -touch_info.one.y + touches[0].y;
				{
					RCOORD delta_x = -delx / 40.0;
					RCOORD delta_y = -dely / 40.0;
					static int toggle;
					delta_x /= hVideo->pWindowPos.cx;
					delta_y /= hVideo->pWindowPos.cy;
					if( toggle )
					{
						RotateRel( l.origin, delta_y, 0, 0 );
						RotateRel( l.origin, 0, delta_x, 0 );
					}
					else
					{
						RotateRel( l.origin, 0, delta_x, 0 );
						RotateRel( l.origin, delta_y, 0, 0 );
					}
					toggle = 1-toggle;
				}


				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
			}
		}
		return 1;
	}
	return 0;
}
#endif

void RenderGL( struct display_camera *camera )
{

	// if there are plugins, then we want to render always.
	if( !camera->plugins )
	{
		PRENDERER other = NULL;
		PRENDERER hVideo = camera->hVidCore;
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( other == hVideo )
				DebugBreak();
			other = hVideo;
			if( l.flags.bLogMessageDispatch )
				lprintf( "Have a video in stack..." );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogMessageDispatch )
					lprintf( "But it's nto exposed..." );
				continue;
			}
			if( hVideo->flags.bUpdated )
				break;
		}
		if( !hVideo )
			return;
	}

	// do OpenGL Frame
	SetActiveGLDisplay( camera->hVidCore );
	InitGL( camera );

	{
		PRENDERER hVideo = camera->hVidCore;

		ApplyTranslationT( VectorConst_I, camera->origin_camera, l.origin );
		switch( camera->type )
		{
		case 0:
			RotateRight( camera->origin_camera, vRight, vForward );
			break;
		case 1:
			RotateRight( camera->origin_camera, vForward, vUp );
			break;
		case 2:
			break;
		case 3:
			RotateRight( camera->origin_camera, vUp, vForward );
			break;
		case 4:
			RotateRight( camera->origin_camera, vForward, vRight );
			break;
		case 5:
			RotateRight( camera->origin_camera, -1, -1 );
			break;
		}

		GetGLMatrix( camera->origin_camera, camera->hVidCore->fModelView );
		glLoadMatrixd( (RCOORD*)camera->hVidCore->fModelView );

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				InvokeExtraBeginDraw( ref->name, ref->psv );
			}
		}

		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( l.flags.bLogMessageDispatch )
				lprintf( "Have a video in stack..." );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogMessageDispatch )
					lprintf( "But it's nto exposed..." );
				continue;
			}

			hVideo->flags.bUpdated = 0;

			if( l.flags.bLogWrites )
				lprintf( "------ BEGIN A REAL DRAW -----------" );

			glEnable( GL_DEPTH_TEST );
			// put out a black rectangle
			// should clear stensil buffer here so we can do remaining drawing only on polygon that's visible.
			ClearImageTo( hVideo->pImage, 0 );
			glDisable(GL_DEPTH_TEST);							// Enables Depth Testing

			if( hVideo->pRedrawCallback )
				hVideo->pRedrawCallback( hVideo->dwRedrawData, (PRENDERER)hVideo );

			// allow draw3d code to assume depth testing 
			glEnable( GL_DEPTH_TEST );
		}

		{
#if 1
			// render a ray that we use for mouse..
			{
				VECTOR target;
				VECTOR origin;
				addscaled( target, camera->mouse_ray.o, camera->mouse_ray.n, 1.0 );
				SetPoint( origin, camera->mouse_ray.o );
				origin[0] += 0.001;
				origin[1] += 0.001;
				//mouse_ray_origin[2] += 0.01;
				glBegin( GL_LINES );
				glColor4ub( 255,0,255,128 );
				glVertex3dv(origin);	// Bottom Left Of The Texture and Quad
				glColor4ub( 255,255,0,128 );
 				glVertex3dv(target);	// Bottom Left Of The Texture and Quad
				glEnd();
			}
			// render a ray that we use for mouse..
			{
				VECTOR target;
				VECTOR origin;
				addscaled( target, l.mouse_ray.o, l.mouse_ray.n, 1.0 );
				SetPoint( origin, l.mouse_ray.o );
				origin[0] += 0.001;
				origin[1] += 0.001;
				//mouse_ray_origin[2] += 0.01;
				glBegin( GL_LINES );
				glColor4ub( 255,255,255,128 );
				glVertex3dv(origin);	// Bottom Left Of The Texture and Quad
				glColor4ub( 255,56,255,128 );
 				glVertex3dv(target);	// Bottom Left Of The Texture and Quad
				glEnd();
			}
#endif
		}

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				InvokeExtraDraw( ref->name, ref->psv );
			}
		}
	}
	SetActiveGLDisplay( NULL );
}

//----------------------------------------------------------------------------

void LoadOptions( void )
{
	_32 average_width, average_height;
	//int some_width;
	//int some_height;
#ifndef __NO_OPTIONS__
	if( !l.cameras )
	{
		int nDisplays = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/Number of Displays", 1, TRUE );
		int n;
		_32 screen_w = 1024, screen_h = 768;
		//GetDisplaySizeEx( 0, NULL, NULL, &screen_w, &screen_h );
		switch( nDisplays )
		{
		default:
		case 0:
			average_width = screen_w;
			average_height = screen_h;
			break;
		case 6:
			average_width = screen_w/4;
			average_height = screen_h/3;
			break;
		}
		for( n = 0; n < nDisplays; n++ )
		{
			TEXTCHAR tmp[128];
			int custom_pos;
			struct display_camera *camera = New( struct display_camera );
			MemSet( camera, 0, sizeof( *camera ) );
			snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Use Custom Position", n+1 );
			custom_pos = SACK_GetProfileIntEx( GetProgramName(), tmp, 0, TRUE );
			if( custom_pos )
			{
				camera->display = -1;
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/x", n+1 );
				camera->x = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?400:n==2?0:n==3?400:0
					:nDisplays==6
						?n==0?0:n==1?300:n==2?300:n==3?300:n==4?600:n==5?900:0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/y", n+1 );
				camera->y = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?0:n==2?300:n==3?300:0
					:nDisplays==6
						?n==0?240:n==1?0:n==2?240:n==3?480:n==4?240:n==5?240:0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/width", n+1 );
				camera->w = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?400
					:nDisplays==6
						?300
					:nDisplays==1
						?800
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/height", n+1 );
				camera->h = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?300
					:nDisplays==6
						?240
					:nDisplays==1
						?600
					:0), TRUE );
				/*
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/Direction", n+1 );
				camera->direction = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?1:n==2?2:n==3?3:0
					:nDisplays==6
						?n // this is natural 0=left, 2=forward, 1=up, 3=down, 4=right, 5=back
					:nDisplays==1
						?0
					:0), TRUE );
					*/
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Use Display", n+1 );
				camera->display = SACK_GetProfileIntEx( GetProgramName(), tmp, nDisplays>1?n+1:0, TRUE );
            GetDisplaySizeEx( camera->display, &camera->x, &camera->y, &camera->w, &camera->h );
			}

			InvokeExtraInit( camera, camera->T_camera, camera->w/2 );			

			AddLink( &l.cameras, camera );
		}
	}
	l.flags.bView360 = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/360 view", 0, TRUE );
	l.flags.bLogFocus = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/log focus event", 0, TRUE );
	l.flags.bLogKeyEvent = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/log key event", 0, TRUE );
	l.flags.bLogMouseEvent = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/log mouse event", 0, TRUE );
	l.flags.bOptimizeHide = SACK_GetProfileIntEx( "SACK", "Video Render/Optimize Hide with SWP_NOCOPYBITS", 0, TRUE );
	l.flags.bLogWrites = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/Log Video Output", 0, TRUE );
	l.flags.bUseLLKeyhook = SACK_GetProfileIntEx( GetProgramName(), "SACK/Video Render/Use Low Level Keyhook", 0, TRUE );
#endif
	if( !l.origin )
	{
		static MATRIX m;
		l.origin = CreateNamedTransform( "render.camera" );
		Translate( l.origin, average_width/2, average_height/2, average_height/2 );
		RotateAbs( l.origin, M_PI, 0, 0 );
		l.origin_surface_camera = CreateTransform( );


		//Translate( l.origin, 200, 150, 150 );
		//Translate( l.origin, some_width/2, some_height/2, some_height/2 );
		//ShowTransform( l.origin );
		// flip over
		//RotateAbs( l.origin, M_PI, 0, 0 );
		//ShowTransform( l.origin );
		CreateTransformMotion( l.origin ); // some things like rotate rel
		{
			VECTOR v;
			v[0] = 0;
			v[1] = 0; //3.14/2;
			v[2] = 3.14/4;
			//SetRotation( l.origin, v );
		}
	}
}


// intersection of lines - assuming lines are 
// relative on the same plane....

//int FindIntersectionTime( RCOORD *pT1, LINESEG pL1, RCOORD *pT2, PLINE pL2 )

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 )
{
   VECTOR R1, R2, denoms;
   RCOORD t1, t2, denom;

#define a (o1[0])
#define b (o1[1])
#define c (o1[2])

#define d (o2[0])
#define e (o2[1])
#define f (o2[2])

#define na (s1[0])
#define nb (s1[1])
#define nc (s1[2])

#define nd (s2[0])
#define ne (s2[1])
#define nf (s2[2])

   crossproduct(denoms, s1, s2 ); // - result...
   denom = denoms[2];
//   denom = ( nd * nb ) - ( ne * na );
   if( NearZero( denom ) )
   {
      denom = denoms[1];
//      denom = ( nd * nc ) - (nf * na );
      if( NearZero( denom ) )
      {
         denom = denoms[0];
//         denom = ( ne * nc ) - ( nb * nf );
         if( NearZero( denom ) )
         {
#ifdef FULL_DEBUG
            lprintf("Bad!-------------------------------------------\n");
#endif
            return FALSE;
         }
         else
         {
            DebugBreak();
            t1 = ( ne * ( c - f ) + nf * ( b - e ) ) / denom;
            t2 = ( nb * ( c - f ) + nc * ( b - e ) ) / denom;
         }
      }
      else
      {
         DebugBreak();
         t1 = ( nd * ( c - f ) + nf * ( d - a ) ) / denom;
         t2 = ( na * ( c - f ) + nc * ( d - a ) ) / denom;
      }
   }
   else
   {
      // this one has been tested.......
      t1 = ( nd * ( b - e ) + ne * ( d - a ) ) / denom;
      t2 = ( na * ( b - e ) + nb * ( d - a ) ) / denom;
   }

   R1[0] = a + na * t1;
   R1[1] = b + nb * t1;
   R1[2] = c + nc * t1;

   R2[0] = d + nd * t2;
   R2[1] = e + ne * t2;
   R2[2] = f + nf * t2;

   if( ( !COMPARE(R1[0],R2[0]) ) ||
       ( !COMPARE(R1[1],R2[1]) ) ||
       ( !COMPARE(R1[2],R2[2]) ) )
   {
      return FALSE;
   }
   *pT2 = t2;
   *pT1 = t1;
   return TRUE;
#undef a
#undef b
#undef c
#undef d
#undef e
#undef f
#undef na
#undef nb
#undef nc
#undef nd
#undef ne
#undef nf
}


int Parallel( PVECTOR pv1, PVECTOR pv2 )
{
   RCOORD a,b,c,cosTheta; // time of intersection

   // intersect a line with a plane.

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

   a = dotproduct( pv1, pv2 );

   if( a < 0.0001 &&
       a > -0.0001 )  // near zero is sufficient...
	{
#ifdef DEBUG_PLANE_INTERSECTION
		Log( "Planes are not parallel" );
#endif
      return FALSE; // not parallel..
   }

   b = Length( pv1 );
   c = Length( pv2 );

   if( !b || !c )
      return TRUE;  // parallel ..... assumption...

   cosTheta = a / ( b * c );
#ifdef FULL_DEBUG
   lprintf( " a: %g b: %g c: %g cos: %g \n", a, b, c, cosTheta );
#endif
   if( cosTheta > 0.99999 ||
       cosTheta < -0.999999 ) // not near 0degrees or 180degrees (aligned or opposed)
   {
      return TRUE;  // near 1 is 0 or 180... so IS parallel...
   }
   return FALSE;
}

// slope and origin of line, 
// normal of plane, origin of plane, result time from origin along slope...
RCOORD IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,  // line m, b
                            PCVECTOR n, PCVECTOR o,  // plane n, o
										RCOORD *time DBG_PASS )
#define IntersectLineWithPlane( s,o,n,o2,t ) IntersectLineWithPlane(s,o,n,o2,t DBG_SRC )
{
   RCOORD a,b,c,cosPhi, t; // time of intersection

   // intersect a line with a plane.

//   v € w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v € w)/(|v| |w|) = cos ß     

	//cosPhi = CosAngle( Slope, n );

   a = ( Slope[0] * n[0] +
         Slope[1] * n[1] +
         Slope[2] * n[2] );

   if( !a )
	{
		//Log1( DBG_FILELINEFMT "Bad choice - slope vs normal is 0" DBG_RELAY, 0 );
		//PrintVector( Slope );
      //PrintVector( n );
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
	if( !b || !c )
	{
      Log( "Slope and or n are near 0" );
		return FALSE; // bad vector choice - if near zero length...
	}

   cosPhi = a / ( b * c );

   t = ( n[0] * ( o[0] - Origin[0] ) +
         n[1] * ( o[1] - Origin[1] ) +
         n[2] * ( o[2] - Origin[2] ) ) / a;

//   lprintf( " a: %g b: %g c: %g t: %g cos: %g pldF: %g pldT: %g \n", a, b, c, t, cosTheta,
//                  pl->dFrom, pl->dTo );

//   if( cosTheta > e1 ) //global epsilon... probably something custom

//#define 

   if( cosPhi > 0 ||
       cosPhi < 0 ) // at least some degree of insident angle
	{
		*time = t;
		return cosPhi;
	}
	else
	{
		Log1( "Parallel... %g\n", cosPhi );
		PrintVector( Slope );
		PrintVector( n );
      // plane and line are parallel if slope and normal are perpendicular
//      lprintf("Parallel...\n");
		return 0;
	}
}

void UpdateMouseRay( struct display_camera * camera )
{
#define BEGIN_SCALE 1
#define COMMON_SCALE ( 2*camera->aspect)
#define END_SCALE 1000
#define tmp_param1 (END_SCALE*COMMON_SCALE)
	if( camera->T_camera )
        {
            GLWindow *x11_gl_window = camera->hVidCore->x11_gl_window;
		VECTOR tmp1;
		static PTRANSFORM t;
		if( !t )
			t = CreateTransform();

		// camera origin is already relative to l.origin 
		//   as l.origin rotates, the effetive camera origin also rotates.

		ApplyT( camera->T_camera, t, l.origin );
		//GetOriginV( camera->T_camera, tmp1 );
		//InvertVector( tmp1 );
		//ApplyRotation( l.origin, tmp1, GetOrigin( camera->T_camera ) );
		//add( (P_POINT)GetOrigin( t ), tmp1, GetOrigin( l.origin  ) );
		//ApplyT( camera->T_camera, t, l.origin );
		/*
		addscaled( l.mouse_ray_origin, GetOrigin( t ), GetAxis( t, vForward ), BEGIN_SCALE );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, GetAxis( t, vRight ), -((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )*COMMON_SCALE/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, GetAxis( t, vUp ), -((float)l.mouse_y-((float)camera->viewport[3]/2.0f) )*(COMMON_SCALE/camera->aspect)/(float)camera->viewport[3] );
		PrintVector( l.mouse_ray_origin );
		addscaled( l.mouse_ray_target, GetOrigin( t ), GetAxis( t, vForward ), END_SCALE );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, GetAxis( t, vRight ), -tmp_param1*((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, GetAxis( t, vUp ), -(tmp_param1/camera->aspect)*((float)l.mouse_y-((float)camera->viewport[3]/2.0f))/(float)camera->viewport[3] );
		PrintVector( l.mouse_ray_target );
		*/
	  // ShowTransform( l.origin );
		//ShowTransform( camera->T_camera );
		///ShowTransform( t );
		addscaled( l.mouse_ray_origin, _0, _Z, BEGIN_SCALE );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, _X, ((float)x11_gl_window->mouse_x-((float)camera->viewport[2]/2.0f) )*COMMON_SCALE/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, _Y, -((float)x11_gl_window->mouse_y-((float)camera->viewport[3]/2.0f) )*(COMMON_SCALE/camera->aspect)/(float)camera->viewport[3] );
		addscaled( l.mouse_ray_target, _0, _Z, END_SCALE );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, _X, tmp_param1*((float)x11_gl_window->mouse_x-((float)camera->viewport[2]/2.0f) )/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, _Y, -(tmp_param1/camera->aspect)*((float)x11_gl_window->mouse_y-((float)camera->viewport[3]/2.0f))/(float)camera->viewport[3] );

		// this is definaly the correct rotation
		ApplyInverseRotation( t, tmp1, l.mouse_ray_origin );
		ApplyTranslation( t, l.mouse_ray_origin, tmp1 );
		ApplyInverseRotation( t, tmp1, l.mouse_ray_target );
		ApplyTranslation( t, l.mouse_ray_target, tmp1 );

		//ApplyInverseRotation( t, tmp1, l.mouse_ray_origin );
		//ApplyTranslation( t, l.mouse_ray_origin, tmp1 );
		//ApplyInverseRotation( t, tmp1, l.mouse_ray_target );
		//ApplyTranslation( t, l.mouse_ray_target, tmp1 );
		sub( l.mouse_ray_slope, l.mouse_ray_target, l.mouse_ray_origin );
		//PrintVector( l.mouse_ray_origin );
		//PrintVector( l.mouse_ray_target );
		
		SetPoint( camera->mouse_ray.n, l.mouse_ray_slope );
		SetPoint( camera->mouse_ray.o, l.mouse_ray_origin );
		SetRay( &l.mouse_ray, &camera->mouse_ray );
	}
}

void UpdateMouseRays( void )
{
	struct display_camera *camera;
	INDEX idx;
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		UpdateMouseRay( camera );
	}
}

// h
int CPROC InverseOpenGLMouse( struct display_camera *camera, PRENDERER hVideo, RCOORD x, RCOORD y, int *result_x, int *result_y )
{
	if( camera->T_camera )
	{
		VECTOR v1, v2;
		int v = 0;

		v2[0] = x;
		v2[1] = y;
		v2[2] = 1.0;
		//ApplyInverse( l.origin, v
		if( hVideo )
			Apply( hVideo->transform, v1, v2 );
		else
			SetPoint( v1, v2 );
		ApplyInverse( camera->T_camera, v2, v1 );
		ApplyInverse( l.origin, v1, v2 );

		//lprintf( "%g,%g,%g  from %g,%g,%g ", v1[0], v1[1], v1[2], v2[0], v2[1] , v2[2] );

		// so this puts the point back in world space
		{
			RCOORD t;
			RCOORD cosphi;
			VECTOR v4;
			SetPoint( v4, _0 );
			v4[2] = 1.0;

			cosphi = IntersectLineWithPlane( v2, _0, _Z, v4, &t );
			//lprintf( "t is %g  cosph = %g", t, cosphi );
			if( cosphi != 0 )
				addscaled( v2, _0, v1, t );

			//lprintf( "%g,%g,%g  ", v1[0], v1[1],v1[2] );

			//surface_x * l.viewport[2] +l.viewport[2] =   ((float)l.mouse_x-((float)l.viewport[2]/2.0f) )*0.25f/(float)l.viewport[2]
		}

		//lprintf( "%g,%g became like %g,%g,%g or %g,%g", x, y
   		//		 , v1[0], v1[1], v1[2]
		//		 , (v1[0]/2.5 * l.viewport[2]) + (l.viewport[2]/2)
		//		 , (l.viewport[3]/2) - (v1[1]/(2.5/l.aspect) * l.viewport[3])
		//   	 );
		if( result_x )
			(*result_x) = (int)((v2[0]/COMMON_SCALE * camera->viewport[2]) + (camera->viewport[2]/2));
		if( result_y )
			(*result_y) = (camera->viewport[3]/2) - (v2[1]/(COMMON_SCALE/camera->aspect) * camera->viewport[3]);
	}
	return 1;
}



 int CPROC OpenGLMouse( PTRSZVAL psvMouse, S_32 x, S_32 y, _32 b )
{
    int used = 0;
    PRENDERER check;
    struct display_camera *camera = (struct display_camera *)psvMouse;
    GLWindow *x11_gl_window = camera->hVidCore->x11_gl_window;
    lprintf( "Good, a mouse event..." );
    if( camera->T_camera )
    {
        UpdateMouseRay( camera );

        {
            INDEX idx;
            struct plugin_reference *ref;
            LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
            {
                used = InvokeExtraMouse( ref->name, ref->psv, &camera->mouse_ray, b );
                if( used )
                    break;
            }
        }

        if( !used )
            for( check = l.top; check ;check = check->pAbove)
            {
                VECTOR target_point;
                if( l.hCaptured )
                    if( check != l.hCaptured )
                        continue;
                if( check->flags.bHidden || (!check->flags.bShown) )
                    continue;
                {
                    RCOORD t;
                    RCOORD cosphi;

                    //PrintVector( GetOrigin( check->transform ) );
                    cosphi = IntersectLineWithPlane( camera->mouse_ray.n, camera->mouse_ray.o
                                                    , GetAxis( check->transform, vForward )
                                                    , GetOrigin( check->transform ), &t );
                    if( cosphi != 0 )
                        addscaled( target_point, l.mouse_ray_origin, l.mouse_ray_slope, t );
                    //PrintVector( target_point );
                }
                // okay so here's the theory now
                //   there's an origin where the camera is, I know where that is
                //   I don't nkow how the model projection is used...
                //   can reverse enginner it I guess... but then we need to break it into
                //    resolution spots,
                {
                    VECTOR target_surface_point;
                    int newx;
                    int newy;

                    x11_gl_window->real_mouse_x = target_point[0];
                    x11_gl_window->real_mouse_y = target_point[1];

                    ApplyInverse( check->transform, target_surface_point, target_point );
                    //PrintVector( target_surface_point );
                    newx = (int)target_surface_point[0];
                    newy = (int)target_surface_point[1];
                    //lprintf( "Is %d,%d in %d,%d %dx%d"
                    //  	 ,newx, newy
                    //  	 ,check->pWindowPos.x, check->pWindowPos.y
                    //  	 , check->pWindowPos.x+ check->pWindowPos.cx
                    //  	 , check->pWindowPos.y+ check->pWindowPos.cy );
#if WIN32
                    if( check == l.hCaptured ||
                       ( ( newx >= 0 && newx < (check->pWindowPos.cx ) )
                        && ( newy >= 0 && newy < (check->pWindowPos.cy ) ) ) )
#endif
                    {
                        if( check && check->pMouseCallback)
                        {
                            if( l.flags.bLogMouseEvent )
                                lprintf( "Sent Mouse Proper. %d,%d %08x", newx, newy, b );
                            //InverseOpenGLMouse( camera, check, newx, newy, NULL, NULL );
                            used = check->pMouseCallback( check->dwMouseData
                                                         , newx
                                                         , newy
                                                         , b );
                            if( used )
                                break;
                        }
                    }
                }
            }
    }
    return used;
}

static PTRSZVAL CPROC RenderCameraThread( PTHREAD thread )
{
	struct display_camera *camera = (struct display_camera *)GetThreadParam( thread );
	AddLink( &l.threads, thread );
	camera->hVidCore->x11_gl_window = createGLWindow( camera );
	while( !(l.bExitThread) && camera->hVidCore->x11_gl_window )
	{
		ProcessDisplayMessages( camera );
	}
}



BOOL  CreateCameras ( void )
{
#  ifdef __LINUX__
	struct display_camera *camera;
   INDEX idx;
	LoadOptions();

	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		if( !camera->hVidCore )
		{
			int UseCoords = camera->display == -1;
			int DefaultScreen = camera->display;
			S_32 x, y;
			_32 w, h;
			// if we don't do it this way, size_t overflows in camera definition.
			if( !UseCoords )
			{
				GetDisplaySizeEx( DefaultScreen, &camera->x, &camera->y, &w, &h );
				camera->w = w;
				camera->h = h;
			}
			else
			{
				w = (_32)camera->w;
				h = (_32)camera->h;
			}


			x = camera->x;
			y = camera->y;

			camera->aspect = ((float)w/(float)h);
			lprintf( "aspect is %g", camera->aspect );

			lprintf(WIDE( "Creating container window named: %s" ),
					(l.gpTitle && l.gpTitle[0]) ? l.gpTitle :"No Name");

	#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.. %d,%d %dx%d" ),x,y,w,h );
	#endif
			camera->hVidCore = New( VIDEO );
			MemSet (camera->hVidCore, 0, sizeof (VIDEO));
			InitializeCriticalSec( &camera->hVidCore->cs );
			camera->hVidCore->camera = camera;
			camera->hVidCore->pMouseCallback = OpenGLMouse;
			camera->hVidCore->dwMouseData = (PTRSZVAL)camera;
			camera->viewport[0] = x;
			camera->viewport[1] = y;
			camera->viewport[2] = (int)w;
			camera->viewport[3] = (int)h;

         ThreadTo( RenderCameraThread, (PTRSZVAL)camera );
		}
	}
#  else
	// need a .NET window here...
	return FALSE;
#  endif
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

void HandleDestroyMessage( PRENDERER hVidDestroy )
{
	{
#ifdef LOG_DESTRUCTION
		//lprintf( WIDE("To destroy! %p %d"), 0 /*Msg.lParam*/, hVidDestroy->hWndOutput );
#endif
#ifdef _WIN32
		// hide the window! then it can't be focused or active or anything!
		//if( hVidDestroy->flags.key_dispatched ) // wait... we can't go away yet...
      //   return;
		//if( hVidDestroy->flags.event_dispatched ) // wait... we can't go away yet...
      //   return;
		//EnableWindow( hVidDestroy->hWndOutput, FALSE );
		SafeSetFocus( (HWND)GetDesktopWindow() );
#  ifdef asdfasdfsdfa
		if( GetActiveWindow() == hVidDestroy->hWndOutput)
		{
#    ifdef LOG_DESTRUCTION
			lprintf( WIDE("Set ourselves inactive...") );
#    endif
			//SetActiveWindow( l.hWndInstance );
#    ifdef LOG_DESTRUCTION
			lprintf( WIDE("Set foreground to instance...") );
#    endif
		}
		//if( GetFocus() == hVidDestroy->hWndOutput)
		{
#    ifdef LOG_DESTRUCTION
			lprintf( WIDE("Fixed focus away from ourselves before destroy.") );
#    endif
//Detach the attached thread

			//SetFocus( GetDesktopWindow() );

		}
#  endif
#endif
		//ShowWindow( hVidDestroy->hWndOutput, SW_HIDE );
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("------------ DESTROY! -----------") );
#endif
#ifdef __LINUX__
      killGLWindow( hVidDestroy->x11_gl_window );
#else
      //DestroyWindow (hVidDestroy->hWndOutput);
#endif
		//UnlinkVideo (hVidDestroy);
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("From destroy") );
#endif
	}
}

//----------------------------------------------------------------------------
void InvokeMouseEvent( PRENDERER hVideo, GLWindow *x11_gl_window  )
{
    if( l.flags.bRotateLock  )
    {
        RCOORD delta_x = x11_gl_window->mouse_x - (hVideo->WindowPos.cx/2);
        RCOORD delta_y = x11_gl_window->mouse_y - (hVideo->WindowPos.cy/2);
        lprintf( "mouse came in we're at %d,%d %g,%g", x11_gl_window->mouse_x, x11_gl_window->mouse_y, delta_x, delta_y );
        if( delta_y && delta_y )
        {
            static int toggle;
            delta_x /= hVideo->WindowPos.cx;
            delta_y /= hVideo->WindowPos.cy;
            if( toggle )
            {
                RotateRel( l.origin, delta_y, 0, 0 );
                RotateRel( l.origin, 0, delta_x, 0 );
            }
            else
            {
                RotateRel( l.origin, 0, delta_x, 0 );
                RotateRel( l.origin, delta_y, 0, 0 );
            }
            toggle = 1-toggle;
            x11_gl_window->mouse_x = hVideo->WindowPos.cx/2;
            x11_gl_window->mouse_y = hVideo->WindowPos.cy/2;
            lprintf( "Set curorpos.." );
            XWarpPointer( x11_gl_window->dpy, None
                         , XRootWindow( x11_gl_window->dpy, 0)
                         , 0, 0, 0, 0
                         , x11_gl_window->mouse_x, x11_gl_window->mouse_y);
            //SetCursorPos( hVideo->pWindowPos.cx/2, hVideo->pWindowPos.cy / 2 );
            lprintf( "Set curorpos Done.." );
        }
    }
    else if (hVideo->pMouseCallback)
    {
        hVideo->pMouseCallback (hVideo->dwMouseData
                                , x11_gl_window->mouse_x
                                , x11_gl_window->mouse_y
                                , x11_gl_window->mouse_b);
    }
}

//----------------------------------------------------------------------------

void HandleMessage (PRENDERER gl_camera, GLWindow *x11_gl_window, XEvent *event)
{
    switch( event->type )
    {
    case ButtonPress:
        {
            // event->xbutton.state - prior state before this event
            // event->xbutton.button = which button is changed.
            switch( event->xbutton.button )
            {
            case Button1:
                x11_gl_window->mouse_b |= MK_LBUTTON;
                break;
            case Button2:
                x11_gl_window->mouse_b |= MK_RBUTTON;
                break;
            case Button3:
                x11_gl_window->mouse_b |= MK_MBUTTON;
                break;
            }
        }
        InvokeMouseEvent( gl_camera, x11_gl_window );
        break;
    case ButtonRelease:
        {
            // event->xbutton.state - prior state before this event
            // event->xbutton.button = which button is changed.
            switch( event->xbutton.button )
            {
            case Button1:
                x11_gl_window->mouse_b &= ~MK_LBUTTON;
                break;
            case Button2:
                x11_gl_window->mouse_b &= ~MK_RBUTTON;
                break;
            case Button3:
                x11_gl_window->mouse_b &= ~MK_MBUTTON;
                break;
            }
        }
        InvokeMouseEvent( gl_camera, x11_gl_window );
        break;
    case MotionNotify:
        x11_gl_window->mouse_x = event->xmotion.x;
        x11_gl_window->mouse_y = event->xmotion.y;
        InvokeMouseEvent( gl_camera, x11_gl_window );
        break;
    case Expose:
        if (event->xexpose.count != 0)
            break;
        lprintf( "X Expose Event" );
		//drawGLScene( camera, x11_gl_window );
		break;
	case ConfigureNotify:
		/* call resizeGLScene only if our window-size changed */
		if ((event->xconfigure.width != x11_gl_window->width) ||
			 (event->xconfigure.height != x11_gl_window->height))
		{
			x11_gl_window->width = event->xconfigure.width;
			x11_gl_window->height = event->xconfigure.height;
			printf("Resize event\n");
			resizeGLScene(event->xconfigure.width,
							  event->xconfigure.height);
		}
		break;
		/* exit in case of a mouse button press */
	case KeyPress:

            {
                char buffer[32];
                KeySym keysym;
                XLookupString( &event->xkey, buffer, 32, &keysym, NULL );

		if (XLookupKeysym(&event->xkey, 0) == XK_Escape)
		{
			//done = True;
		}
		if (XLookupKeysym(&event->xkey,0) == XK_F1)
		{
                    //killGLWindow();
						  //x11_gl_window->fs = !x11_gl_window->fs;
						  //createGLWindow("NeHe's OpenGL Framework", 640, 480, 24, x11_gl_window->fs);
		}
		}
		break;
	case KeyRelease:
		break;
	case ClientMessage:
		{
			if( event->xclient.message_type == x11_gl_window->atom_create )
			{
			}

			char *msg = XGetAtomName( event->xany.display
											, event->xclient.message_type );
			if( !msg )
			{
            lprintf( "Received an any message with bad atom." );
				break;
			}


         XFree( msg );
		}
		if (*XGetAtomName(x11_gl_window->dpy, event->xclient.message_type) ==
			 *"WM_PROTOCOLS")
		{
			printf("Exiting sanely...\n");
			//done = True;
		}
		break;
	default:
		break;
	}
}

//----------------------------------------------------------------------------
int internal;
int CPROC OldProcessDisplayMessages(void )
{
    return -1;
}
int CPROC ProcessDisplayMessages( struct display_camera *camera )
{
	XEvent event;
	//lprintf( "Checking Thread %Lx", GetThreadID( MakeThread() ) );
	GLWindow *x11_gl_window = camera->hVidCore->x11_gl_window;
	//lprintf( "is it %Lx?", GetThreadID( thread ) );
	{
		while( XPending( x11_gl_window->dpy ) > 0 )
		{
			XNextEvent(x11_gl_window->dpy, &event);
			//if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("(E)Got message:%d"), event.type );
			HandleMessage( camera->hVidCore, x11_gl_window, &event );
			//if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("(X)Got message:%d"), event.type );
		}
		drawGLScene( camera, x11_gl_window );
	}
	return 1;
}


//----------------------------------------------------------------------------

int  InitDisplay (void)
{
	{
		GLboolean params = 0;
		if( glGetBooleanv(GL_STEREO, &params), params )
		{
			lprintf( "yes stereo" );
		}
		else
			lprintf( "no stereo" );
	}


	return TRUE;
}

//----------------------------------------------------------------------------

PRIORITY_ATEXIT( RemoveKeyHook, 100 )
{
	// might get the release of the alt while wiating for networks to graceful exit.

	l.bExitThread = TRUE;
   xlprintf(LOG_ALWAYS)( "Send events to threads appropriately to shut down here please!" );
}

TEXTCHAR  GetKeyText (int key)
{
   int c;
	char ch[5];
	if( key & KEY_MOD_DOWN )
      return 0;
	key ^= 0x80000000;
#ifndef __LINUX__
   c =  
#  ifndef UNDER_CE
      ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
               l.kbd.key, (unsigned short *) ch, 0);
#  else
	   key;
#  endif
#else  // linux
#endif //linux
   if (!c)
   {
      // check prior key bindings...
      //printf( WIDE("no translation\n") );
      return 0;
   }
   else if (c == 2)
   {
      //printf( WIDE("Key Translated: %d %d\n"), ch[0], ch[1] );
      return 0;
   }
   else if (c < 0)
   {
      //printf( WIDE("Key Translation less than 0\n") );
      return 0;
   }
   //printf( WIDE("Key Translated: %d(%c)\n"), ch[0], ch[0] );
   return ch[0];
}

//----------------------------------------------------------------------------

LOGICAL DoOpenDisplay( PRENDERER hNextVideo )
{
	InitDisplay();

   // there may not be any cameras... but each time we open a window, open cameras.
	CreateCameras();

	PutDisplayAbove( hNextVideo, l.top );

	AddLink( &l.pActiveList, hNextVideo );

	hNextVideo->KeyDefs = CreateKeyBinder();
	CreateDrawingSurface( hNextVideo );
	hNextVideo->flags.bReady = 1;
#ifdef LOG_OPEN_TIMING
	lprintf( WIDE( "Doing open of a display..." ) );
#endif
		{
			_32 timeout = timeGetTime() + 500000;
			//hNextVideo->thread = GetMyThreadID();
			while (!hNextVideo->flags.bReady && timeout > timeGetTime())
			{
				// need to do this on the possibility that
				// thIS thread did create this window...
				if( !Idle() )
				{
#ifdef NOISY_LOGGING
					lprintf( WIDE("Sleeping until the window is created.") );
#endif
					WakeableSleep( SLEEP_FOREVER );
					//Relinquish();
				}
			}
		}
#ifdef LOG_STARTUP
	//lprintf( WIDE("Resulting new window %p %d"), hNextVideo, hNextVideo->hWndOutput );
#endif
	return TRUE;
}


PRENDERER  MakeDisplayFrom ( int hWnd)
{
	
	PRENDERER hNextVideo;
	hNextVideo = New( VIDEO );
	MemSet (hNextVideo, 0, sizeof (VIDEO));
   InitializeCriticalSec( &hNextVideo->cs );
#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif

#if 0
	hNextVideo->hWndContainer = hWnd;
	hNextVideo->flags.bFull = TRUE;
   // should check styles and set rendered according to hWnd
   hNextVideo->flags.bLayeredWindow = l.flags.bLayeredWindowDefault;
	if( DoOpenDisplay( hNextVideo ) )
	{
		lprintf( "New bottom is %p", l.bottom );
		return hNextVideo;
	}
	Release( hNextVideo );
#endif
	return NULL;
}


PRENDERER  OpenDisplaySizedAt (_32 attr, _32 wx, _32 wy, S_32 x, S_32 y) // if native - we can return and let the messages dispatch...
{
	PRENDERER hNextVideo;
	//lprintf( "open display..." );
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
	InitializeCriticalSec( &hNextVideo->cs );
#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif
	if (wx == -1)
		wx = 1280;
	if (wy == -1)
		wy = 800;
	if (x == -1)
		x = 25;
	if (y == -1)
		y = 25;
#ifdef UNDER_CE
	l.WindowBorder_X = 0;
	l.WindowBorder_Y = 0;
#else
	l.WindowBorder_X = 0;//GetSystemMetrics (SM_CXFRAME);
	l.WindowBorder_Y = 0;//GetSystemMetrics (SM_CYFRAME)
		//+ GetSystemMetrics (SM_CYCAPTION) + GetSystemMetrics (SM_CYBORDER);
#endif
	// NOT MULTI-THREAD SAFE ANYMORE!
	//lprintf( WIDE("Hardcoded right here for FULL window surfaces, no native borders.") );
	hNextVideo->flags.bFull = TRUE;
	hNextVideo->flags.bNoAutoFocus = (attr & DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS)?TRUE:FALSE;
	hNextVideo->flags.bChildWindow = (attr & DISPLAY_ATTRIBUTE_CHILD)?TRUE:FALSE;
	hNextVideo->flags.bNoMouse = (attr & DISPLAY_ATTRIBUTE_NO_MOUSE)?TRUE:FALSE;
	hNextVideo->WindowPos.x = x;
	hNextVideo->WindowPos.y = y;
	hNextVideo->WindowPos.cx = wx;
	hNextVideo->WindowPos.cy = wy;

	if( DoOpenDisplay( hNextVideo ) )
	{
      lprintf( "New bottom is %p", l.bottom );
      return hNextVideo;
	}
	Release( hNextVideo );
	return NULL;
}

 void  SetDisplayNoMouse ( PRENDERER hVideo, int bNoMouse )
{
	if( hVideo ) 
	{
		if( hVideo->flags.bNoMouse != bNoMouse )
		{
			hVideo->flags.bNoMouse = bNoMouse;
#ifndef NO_MOUSE_TRANSPARENCY
			if( bNoMouse )
			{
				//SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
            ; //SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) & ~WS_EX_TRANSPARENT );
#endif
		}

	}
}

//----------------------------------------------------------------------------

PRENDERER  OpenDisplayAboveSizedAt (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PRENDERER parent)
{
   PRENDERER newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

PRENDERER  OpenDisplayAboveUnderSizedAt (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PRENDERER parent, PRENDERER barrier)
{
   PRENDERER newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if( barrier )
   {
	   // use initial SW_RESTORE instead of SW_NORMAL
		newvid->flags.bOpenedBehind = 1;
		newvid->under = barrier;
		//if( barrier->over )
		{
         //if( barrier->over != parent )
			//	DebugBreak();
		}
		barrier->over = newvid;
		if( barrier )
		{
			if( l.bottom == barrier )
			{
				l.bottom = newvid;
			}

			if( newvid->pBelow )
				newvid->pBelow->pAbove = newvid->pAbove;
			if( newvid->pAbove )
				newvid->pAbove->pBelow = newvid->pBelow;

			newvid->pBelow = barrier;
			if( newvid->pAbove = barrier->pAbove )
				barrier->pAbove->pBelow = newvid;
			barrier->pAbove = newvid;
		}
		//lprintf( "Opening window behind another." );
		//lprintf( "--- before SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );

		//SetWindowPos( newvid->hWndOutput, barrier->hWndOutput, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	   //lprintf( "--- after SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );
   }
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

//----------------------------------------------------------------------------

void  CloseDisplay (PRENDERER hVideo)
{
	lprintf( "close display %p", hVideo );
	// just kills this video handle....
	if (!hVideo)         // must already be destroyed, eh?
		return;
#ifdef LOG_DESTRUCTION
	Log (WIDE( "Unlinking destroyed window..." ));
#endif
	// take this out of the list of active windows...
	DeleteLink( &l.pActiveList, hVideo );
	UnlinkVideo( hVideo );
	lprintf( "and we should be ok?" );
	hVideo->flags.bDestroy = 1;

	// the scan of inactive windows releases the hVideo...
	AddLink( &l.pInactiveList, hVideo );
	// generate an event to dispatch pending...
	// there is a good chance that a window event caused a window
	// and it will be sleeping until the next event...
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
	SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
#endif
   return;
}

//----------------------------------------------------------------------------

void  SizeDisplay (PRENDERER hVideo, _32 w, _32 h)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Size Display..." ) );
#endif
	if( w == hVideo->WindowPos.cx && h == hVideo->WindowPos.cy )
		return;
	if( hVideo->flags.bLayeredWindow )
	{
		// need to remake image surface too...
		hVideo->WindowPos.cx = w;
		hVideo->WindowPos.cy = h;
      /*
		SetWindowPos (hVideo->hWndOutput, hVideo->WindowPos.hwndInsertAfter
					, 0, 0
					, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
					, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
					, SWP_NOMOVE|SWP_NOACTIVATE);
      */
		CreateDrawingSurface (hVideo);
		if( hVideo->flags.bShown )
			UpdateDisplay( hVideo );
	}
	else
	{
      /*
		SetWindowPos (hVideo->hWndOutput, hVideo->WindowPos.hwndInsertAfter
					, 0, 0
					, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
					, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
					, SWP_NOMOVE|SWP_NOACTIVATE);
      */
	}
}


//----------------------------------------------------------------------------

void  SizeDisplayRel (PRENDERER hVideo, S_32 delw, S_32 delh)
{
	if (delw || delh)
	{
		S_32 cx, cy;
		cx = hVideo->WindowPos.cx + delw;
		cy = hVideo->WindowPos.cy + delh;
		if (hVideo->WindowPos.cx < 50)
			hVideo->WindowPos.cx = 50;
		if (hVideo->WindowPos.cy < 20)
			hVideo->WindowPos.cy = 20;
#ifdef LOG_RESIZE
		Log2 (WIDE( "Resized display to %d,%d" ), hVideo->WindowPos.cx,
            hVideo->WindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "size display relative" ) );
#endif
      /*
		SetWindowPos (hVideo->hWndOutput, NULL, 0, 0, cx, cy,
		SWP_NOZORDER | SWP_NOMOVE);
      */
   }
}

//----------------------------------------------------------------------------

void  MoveDisplay (PRENDERER hVideo, S_32 x, S_32 y)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Move display %d,%d" ), x, y );
#endif
   if( hVideo )
	{
		if( ( hVideo->WindowPos.x != x ) || ( hVideo->WindowPos.y != y ) )
		{
			hVideo->WindowPos.x = x;
			hVideo->WindowPos.y = y;
			Translate( hVideo->transform, x, y, 0.0 );
			if( hVideo->flags.bShown )
			{
				// layered window requires layered output to be called to move the display.
				UpdateDisplay( hVideo );
			}
		}
	}
}

//----------------------------------------------------------------------------

void  MoveDisplayRel (PRENDERER hVideo, S_32 x, S_32 y)
{
   if (x || y)
   {
		lprintf( "Moving display %d,%d", x, y );
		hVideo->WindowPos.x += x;
		hVideo->WindowPos.y += y;
		Translate( hVideo->transform, hVideo->WindowPos.x, hVideo->WindowPos.y, 0 );
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "Move display relative" ) );
#endif
   }
}

//----------------------------------------------------------------------------

void  MoveSizeDisplay (PRENDERER hVideo, S_32 x, S_32 y, S_32 w,
                                     S_32 h)
{
   S_32 cx, cy;
   hVideo->WindowPos.x = x;
	hVideo->WindowPos.y = y;
   cx = w;
   cy = h;
   if (cx < 50)
      cx = 50;
   if (cy < 20)
		cy = 20;
   hVideo->WindowPos.cx = cx;
   hVideo->WindowPos.cy = cy;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( WIDE( "move and size display." ) );
#endif
   // updates window translation
   CreateDrawingSurface( hVideo );
}

//----------------------------------------------------------------------------

void  MoveSizeDisplayRel (PRENDERER hVideo, S_32 delx, S_32 dely,
                                        S_32 delw, S_32 delh)
{
	S_32 cx, cy;
   hVideo->WindowPos.x += delx;
   hVideo->WindowPos.y += dely;
   cx = hVideo->WindowPos.cx + delw;
   cy = hVideo->WindowPos.cy + delh;
   if (cx < 50)
      cx = 50;
   if (cy < 20)
		cy = 20;
   hVideo->WindowPos.cx = cx;
   hVideo->WindowPos.cy = cy;
//fdef LOG_DISPLAY_RESIZE
	lprintf( WIDE( "move and size relative %d,%d %d,%d" ), delx, dely, delw, delh );
//ndif
   CreateDrawingSurface( hVideo );
}

//----------------------------------------------------------------------------

void  UpdateDisplayEx (PRENDERER hVideo DBG_PASS )
{
   // copy hVideo->lpBuffer to hVideo->hDCOutput
   if (hVideo )
   {
      UpdateDisplayPortionEx (hVideo, 0, 0, 0, 0 DBG_RELAY);
   }
   return;
}

//----------------------------------------------------------------------------

void  ClearDisplay (PRENDERER hVideo)
{
   lprintf( "Who's calling clear display? it's assumed clear" );
   //ClearImage( hVideo->pImage );
}

//----------------------------------------------------------------------------

void  SetMousePosition (PRENDERER hVid, S_32 x, S_32 y)
{
	if( !hVid )
	{
		int newx, newy;
		lprintf( "TAGHERE" );
		InverseOpenGLMouse( hVid->camera, hVid, (RCOORD)x, (RCOORD)y, &newx, &newy );
		lprintf( "%d,%d became %d,%d", x, y, newx, newy );
		XWarpPointer( hVid->camera->hVidCore->x11_gl_window->dpy, None
						, XRootWindow( hVid->camera->hVidCore->x11_gl_window->dpy, 0)
						, 0, 0, 0, 0
						, newx, newy );
      XFlush( hVid->camera->hVidCore->x11_gl_window->dpy );
	}
	else
	{
		if( hVid->camera && hVid->flags.bFull)
		{
			int newx, newy;
			//lprintf( "TAGHERE" );
			InverseOpenGLMouse( hVid->camera, hVid, x+ hVid->cursor_bias.x, y, &newx, &newy );
			//lprintf( "%d,%d became %d,%d", x, y, newx, newy );
			XWarpPointer( hVid->camera->hVidCore->x11_gl_window->dpy, None
							, XRootWindow( hVid->camera->hVidCore->x11_gl_window->dpy, 0)
							, 0, 0, 0, 0
							, newx, newy );
			XFlush( hVid->camera->hVidCore->x11_gl_window->dpy );
		}
		else
		{
			XWarpPointer( hVid->camera->hVidCore->x11_gl_window->dpy, None
							, XRootWindow( hVid->camera->hVidCore->x11_gl_window->dpy, 0)
							, 0, 0, 0, 0
							, x + l.WindowBorder_X + hVid->cursor_bias.x,
							  y + l.WindowBorder_Y + hVid->cursor_bias.y );
			XFlush( hVid->camera->hVidCore->x11_gl_window->dpy );
		}
	}
}

//----------------------------------------------------------------------------

void  GetMousePosition (S_32 * x, S_32 * y)
{
	lprintf( "This is really relative to what is looking at it " );
        //DebugBreak();
#if 0
	if (x)
		(*x) = l.real_mouse_x;
	if (y)
            (*y) = l.real_mouse_y;
#endif
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(S_32 * x, S_32 * y, _32 *b)
{
    lprintf( "Cannot GetMouseState." );
#if 0
    GetMousePosition( x, y );
    if( b )
        (*b) = l.mouse_b;
#endif
}

//----------------------------------------------------------------------------

void  SetCloseHandler (PRENDERER hVideo,
                                     CloseCallback pWindowClose,
                                     PTRSZVAL dwUser)
{
	if( hVideo )
	{
		hVideo->dwCloseData = dwUser;
		hVideo->pWindowClose = pWindowClose;
	}
}

//----------------------------------------------------------------------------

void  SetMouseHandler (PRENDERER hVideo,
                                     MouseCallback pMouseCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwMouseData = dwUser;
   hVideo->pMouseCallback = pMouseCallback;
}

//----------------------------------------------------------------------------
#if !defined( __WATCOMC__ ) && !defined( __LINUX__ )
RENDER_PROC (void, SetTouchHandler) (PRENDERER hVideo,
                                     TouchCallback pTouchCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwTouchData = dwUser;
   hVideo->pTouchCallback = pTouchCallback;
}
#endif
//----------------------------------------------------------------------------


void  SetRedrawHandler (PRENDERER hVideo,
                                      RedrawCallback pRedrawCallback,
                                      PTRSZVAL dwUser)
{
	hVideo->dwRedrawData = dwUser;
	if( (hVideo->pRedrawCallback = pRedrawCallback ) )
	{
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
		if( hVideo->flags.bShown )
		{
			Redraw( hVideo );
			//lprintf( WIDE( "Invalida.." ) );
			//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
			//SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
		}
		//hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
	}

}

//----------------------------------------------------------------------------

void  SetKeyboardHandler (PRENDERER hVideo, KeyProc pKeyProc,
                                        PTRSZVAL dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

void  SetLoseFocusHandler (PRENDERER hVideo,
                                         LoseFocusCallback pLoseFocus,
                                         PTRSZVAL dwUser)
{
	hVideo->dwLoseFocus = dwUser;
	hVideo->pLoseFocus = pLoseFocus;
}

//----------------------------------------------------------------------------

void  SetApplicationTitle (const TEXTCHAR *pTitle)
{
	l.gpTitle = pTitle;
	if (l.cameras)
	{
      //DebugBreak();
		//SetWindowText((((struct display_camera *)GetLink( &l.cameras, 0 ))->hWndInstance), l.gpTitle);
	}
}

//----------------------------------------------------------------------------

void  SetRendererTitle (PRENDERER hVideo, const TEXTCHAR *pTitle)
{
	//l.gpTitle = pTitle;
	//if (l.hWndInstance)
	{
		if( hVideo->pTitle )
         Release( hVideo->pTitle );
		hVideo->pTitle = StrDupEx( pTitle DBG_SRC );
		//SetWindowText( hVideo->hWndOutput, pTitle );
	}
}

//----------------------------------------------------------------------------

void  SetApplicationIcon (ImageFile * hIcon)
{
#ifdef _WIN32
   //HICON hIcon = CreateIcon();
#endif
}

//----------------------------------------------------------------------------

void  MakeTopmost (PRENDERER hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		if( hVideo->flags.bShown )
		{
			//lprintf( WIDE( "Forcing topmost" ) );
         /*
			SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE);
         */
		}
		else
		{
		}
	}
}

//----------------------------------------------------------------------------

void  MakeAbsoluteTopmost (PRENDERER hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		hVideo->flags.bAbsoluteTopmost = 1;
		if( hVideo->flags.bShown )
		{
			lprintf( WIDE( "Forcing topmost" ) );
         /*
			SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE);
         */
		}
	}
}

//----------------------------------------------------------------------------

 int  IsTopmost ( PRENDERER hVideo )
{
   return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
void  HideDisplay (PRENDERER hVideo)
{
//#ifdef LOG_SHOW_HIDE
	//lprintf(WIDE( "Hiding the window! %p %p %p %p" ), hVideo, hVideo->hWndOutput, hVideo->pAbove, hVideo->pBelow );
//#endif
	if( hVideo )
	{
		if( l.hCaptured == hVideo )
			l.hCaptured = NULL;
		hVideo->flags.bHidden = 1;
		/* handle lose focus */
	}
}

//----------------------------------------------------------------------------
void  RestoreDisplayEx (PRENDERER hVideo DBG_PASS )
{
//#ifdef LOG_SHOW_HIDE
	//lprintf( WIDE( "Restore display. %p %p" ), hVideo, hVideo->hWndOutput );
//#endif
	if( hVideo )
	{
		hVideo->flags.bHidden = 0;
		hVideo->flags.bShown = 1;
	}
}

//----------------------------------------------------------------------------

void  GetDisplaySizeEx ( int nDisplay
												 , S_32 *x, S_32 *y
												 , _32 *width, _32 *height)
{
   switch( nDisplay )
   {
   case 0: 
	(*x) = 0;
	(*y) = 0;
	(*width) = 800;
	(*height) = 600;
	break;
   case 1: 
	(*x) = 0;
	(*y) = 0;
	(*width) = 800;
	(*height) = 600;
	break;
   case 2: 
	(*x) = 800;
	(*y) = 0;
	(*width) = 800;
	(*height) = 600;
	break;
   case 3: 
	(*x) = -800;
	(*y) = 0;
	(*width) = 800;
	(*height) = 600;
	break;
   }
}

void  GetDisplaySize (_32 * width, _32 * height)
{
	if( width )
		(*width) = 65535;
	if( height )
		(*height) = 65535;
#if 0
   RECT r;
   GetWindowRect (GetDesktopWindow (), &r);
   //Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
   if (width)
      *width = r.right - r.left;
   if (height)
		*height = r.bottom - r.top;
#endif
}

//----------------------------------------------------------------------------

void  GetDisplayPosition (PRENDERER hVid, S_32 * x, S_32 * y,
                                        _32 * width, _32 * height)
{
	if (!hVid)
		return;
	if (width)
		*width = hVid->WindowPos.cx;
	if (height)
		*height = hVid->WindowPos.cy;
#ifndef NO_ENUM_DISPLAY
	{
		int posx = 0;
		int posy = 0;
		{
			//WINDOWINFO wi;
			//wi.cbSize = sizeof( wi);
			
			//GetWindowInfo( hVid->hWndOutput, &wi );
			//posx += wi.rcClient.left;
			//posy += wi.rcClient.top;
		}
		if (x)
			*x = posx;
		if (y)
			*y = posy;
	}
#endif
}

//----------------------------------------------------------------------------
LOGICAL  DisplayIsValid (PRENDERER hVid)
{
   return hVid->flags.bReady;
}

//----------------------------------------------------------------------------

void  SetDisplaySize (_32 width, _32 height)
{
   SizeDisplay (l.hVideoPool, width, height);
}

//----------------------------------------------------------------------------

ImageFile * GetDisplayImage (PRENDERER hVideo)
{
   return hVideo->pImage;
}

//----------------------------------------------------------------------------

PKEYBOARD  GetDisplayKeyboard (PRENDERER hVideo)
{
   return &hVideo->kbd;
}

//----------------------------------------------------------------------------

LOGICAL  HasFocus (PRENDERER hVideo)
{
   return hVideo->flags.bFocused;
}

//----------------------------------------------------------------------------

#if ACTIVE_MESSAGE_IMPLEMENTED
int  SendActiveMessage (PRENDERER dest, PACTIVEMESSAGE msg)
{
   return 0;
}

PACTIVEMESSAGE  CreateActiveMessage (int ID, int size,...)
{
   return NULL;
}

void  SetDefaultHandler (PRENDERER hVideo,
                                       GeneralCallback general, PTRSZVAL psv)
{
}
#endif
//----------------------------------------------------------------------------

void  OwnMouseEx (PRENDERER hVideo, _32 own DBG_PASS)
{
	if (own)
	{
		lprintf( WIDE("Capture is set on %p"),hVideo );
		if( !l.hCaptured )
		{
			l.hCaptured = hVideo;
			hVideo->flags.bCaptured = 1;
			//SetCapture (hVideo->hWndOutput);
		}
		else
		{
			if( l.hCaptured != hVideo )
			{
				lprintf( WIDE("Another window now wants to capture the mouse... the prior window will ahve the capture stolen.") );
				l.hCaptured = hVideo;
				hVideo->flags.bCaptured = 1;
				//SetCapture (hVideo->hWndOutput);
			}
			else
			{
				if( !hVideo->flags.bCaptured )
				{
					lprintf( WIDE("This should NEVER happen!") );
               *(int*)0 = 0;
				}
				// should already have the capture...
			}
		}
	}
	else
	{
		if( l.hCaptured == hVideo )
		{
			lprintf( WIDE("No more capture.") );
			//ReleaseCapture ();
			hVideo->flags.bCaptured = 0;
			l.hCaptured = NULL;
		}
	}
}

//----------------------------------------------------------------------------
void
NoProc (void)
{
   // empty do nothing prodecudure for unimplemented features
}

//----------------------------------------------------------------------------
#undef GetNativeHandle
	HWND  GetNativeHandle (PRENDERER hVideo)
	{
		return (HWND)hVideo; //hVideo->hWndOutput;
	}

int  BeginCalibration (_32 nPoints)
{
   return 1;
}

//----------------------------------------------------------------------------
void  SyncRender( PRENDERER hVideo )
{
   // sync has no consequence...
   return;
}

//----------------------------------------------------------------------------

 void  ForceDisplayFocus ( PRENDERER pRender )
{
	//SetActiveWindow( GetParent( pRender->hWndOutput ) );
	//SetForegroundWindow( GetParent( pRender->hWndOutput ) );
	//SetFocus( GetParent( pRender->hWndOutput ) );
	//lprintf( WIDE( "... 3 step?" ) );
	//SetActiveWindow( pRender->hWndOutput );
	//SetForegroundWindow( pRender->hWndOutput );
	//if( pRender )
	//	SafeSetFocus( pRender->hWndOutput );
}

//----------------------------------------------------------------------------

 void  ForceDisplayFront ( PRENDERER pRender )
{
	if( pRender != l.top )
		PutDisplayAbove( pRender, l.top );

}

//----------------------------------------------------------------------------

 void  ForceDisplayBack ( PRENDERER pRender )
{
	// uhmm...
   lprintf( WIDE( "Force display backward." ) );
   //SetWindowPos( pRender->hWndOutput, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
void  UpdateDisplay (PRENDERER hVideo )
{
   //DebugBreak();
   UpdateDisplayEx( hVideo DBG_SRC );
}

void  DisableMouseOnIdle (PRENDERER hVideo, LOGICAL bEnable )
{
	if( hVideo->flags.bIdleMouse != bEnable )
	{
		if( bEnable )
		{
			//l.redraw_timer_id = SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)1, 33, NULL );
			//l.mouse_timer_id = SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)2, 100, NULL );
			//hVideo->idle_timer_id = SetTimer( hVideo->hWndOutput, (UINT_PTR)3, 100, NULL );
			//l.last_mouse_update = timeGetTime(); // prime the hider.
			//hVideo->flags.bIdleMouse = bEnable;
		}
		else // disabling...
		{
			//hVideo->flags.bIdleMouse = bEnable;
			//if( !l.flags.mouse_on )
			{
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "Mouse was off... want it on..." ) );
#endif
			}
			//if( hVideo->idle_timer_id )
			//{
			//	KillTimer( hVideo->hWndOutput, hVideo->idle_timer_id );
         //   hVideo->idle_timer_id = 0;
			//}
		}
	}
}


 void  SetDisplayFade ( PRENDERER hVideo, int level )
{
	if( hVideo )
	{
		if( level < 0 )
         level = 0;
		if( level > 254 )
			level = 254;
		hVideo->fade_alpha = 255 - level;
		//if( l.flags.bLogWrites )
		//	lprintf( WIDE( "Output fade %d %p" ), hVideo->fade_alpha, hVideo->hWndOutput );
#ifndef UNDER_CE
		//IssueUpdateLayeredEx( hVideo, FALSE, 0, 0, 0, 0 DBG_SRC );
#endif
	}
}

#undef GetRenderTransform
PTRANSFORM CPROC GetRenderTransform       ( PRENDERER r )
{
	return r->transform;
}

LOGICAL RequiresDrawAll ( void )
{
	return TRUE;
}

void MarkDisplayUpdated( PRENDERER r )
{
   l.flags.bUpdateWanted = 1;
	if( r )
      r->flags.bUpdated = 1;
}

#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay
                                       , SetApplicationTitle
                                       , (void (CPROC*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , OldProcessDisplayMessages
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32)) OpenDisplaySizedAt
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void (CPROC*)(PRENDERER)) CloseDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32, _32, _32 DBG_PASS)) UpdateDisplayPortionEx
                                       , (void (CPROC*)(PRENDERER DBG_PASS)) UpdateDisplayEx
                                       , (void (CPROC*)(PRENDERER)) ClearDisplay
                                       , GetDisplayPosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplayRel
                                       , (void (CPROC*)(PRENDERER, _32, _32)) SizeDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image (CPROC*)(PRENDERER)) GetDisplayImage
                                       , (void (CPROC*)(PRENDERER, CloseCallback, PTRSZVAL)) SetCloseHandler
                                       , (void (CPROC*)(PRENDERER, MouseCallback, PTRSZVAL)) SetMouseHandler
                                       , (void (CPROC*)(PRENDERER, RedrawCallback, PTRSZVAL)) SetRedrawHandler
                                       , (void (CPROC*)(PRENDERER, KeyProc, PTRSZVAL)) SetKeyboardHandler
													,  SetLoseFocusHandler
                                          , NULL
                                       , (void (CPROC*)(S_32 *, S_32 *)) GetMousePosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SetMousePosition
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
#ifdef _OPENGL_ENABLED
													, NULL //EnableOpenGL
                                       , SetActiveGLDisplay
#else
                                       , NULL
                                       , NULL
#endif
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
													, NULL // WinShell_AcceptDroppedFiles
													, PutDisplayIn
#ifdef WIN32
                                       , MakeDisplayFrom
#endif
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
													, NULL //IssueUpdateLayeredEx
                                       , RequiresDrawAll
#if !defined( __WATCOMC__ ) && !defined( __LINUX__ )
													, SetTouchHandler
#endif
                                       , MarkDisplayUpdated
};

RENDER3D_INTERFACE Render3d = {
	GetRenderTransform

};

#undef GetDisplayInterface
#undef DropDisplayInterface

POINTER  CPROC GetDisplayInterface (void)
{
	InitDisplay();
   return (POINTER)&VidInterface;
}

void  CPROC DropDisplayInterface (POINTER p)
{
}

#undef GetDisplay3dInterface
POINTER CPROC GetDisplay3dInterface (void)
{
	InitDisplay();
	return (POINTER)&Render3d;
}

void  CPROC DropDisplay3dInterface (POINTER p)
{
}

static LOGICAL CPROCDefaultExit( PTRSZVAL psv, _32 keycode )
{
   lprintf( WIDE( "Default Exit..." ) );
   BAG_Exit(0);
}

static LOGICAL CPROCEnableRotation( PTRSZVAL psv, _32 keycode )
{
	lprintf( "Enable Rotation..." );
	if( IsKeyPressed( keycode ) )
	{
		l.flags.bRotateLock = 1 - l.flags.bRotateLock;
		if( l.flags.bRotateLock )
		{
			struct display_camera *default_camera = (struct display_camera *)GetLink( &l.cameras, 0 );
			//x11_gl_window->mouse_x = default_camera->hVidCore->WindowPos.cx/2;
			//x11_gl_window->mouse_y = default_camera->hVidCore->WindowPos.cy/2;
			//XWarpPointer( hVid->camera->hVidCore->x11_gl_window->dpy, None
			//				, XRootWindow( hVid->camera->hVidCore->x11_gl_window->dpy, 0)
			//				, 0, 0, 0, 0
			//				, default_camera->hVidCore->WindowPos.cx/2, default_camera->hVidCore->WindowPos.cy / 2 );
			//XFlush( hVid->camera->hVidCore->x11_gl_window->dpy );
		}
		lprintf( "ALLOW ROTATE" );
	}
	else
		lprintf( "DISABLE ROTATE" );
	if( l.flags.bRotateLock )
		lprintf( "lock rotate" );
	else
		lprintf( "unlock rotate" );
}

static LOGICAL CPROCCameraForward( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
#define SPEED_CONSTANT 250
		if( IsKeyPressed( keycode ) )
		{
			if( keycode & KEY_SHIFT_DOWN )
				Forward( l.origin, -SPEED_CONSTANT );
			else
				Forward( l.origin, SPEED_CONSTANT );
		}
      else
			Forward( l.origin, 0.0 );
      UpdateMouseRays( );
//      return 1;
	}
//   return 0;
}

static LOGICAL CPROCCameraLeft( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			if( keycode & KEY_SHIFT_DOWN )
			{
				Right( l.origin, SPEED_CONSTANT );
			}
			else
			{
				Right( l.origin, -SPEED_CONSTANT );
			}
		}
      else
			Right( l.origin, 0.0 );
      UpdateMouseRays( );
//      return 1;
	}
//   return 0;
}

static LOGICAL CPROCCameraRight( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
      if( IsKeyPressed( keycode ) )
			Right( l.origin, SPEED_CONSTANT );
      else
			Right( l.origin, 0.0 );
//      return 1;
      UpdateMouseRays( );
	}
//   return 0;
}

static LOGICAL CPROCCameraRollRight( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			VECTOR tmp;
			scale( tmp, _Z, -1.0 );
			SetRotation( l.origin, tmp );
		}
      else
			SetRotation( l.origin, _0 );
//      return 1;
      UpdateMouseRays( );
	}
//   return 0;
}

static LOGICAL CPROCCameraRollLeft( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			//VECTOR tmp;
         //scale(tmp,_Z,1.0 );
			SetRotation( l.origin, _Z );
		}
      else
			SetRotation( l.origin, _0 );
//      return 1;
      UpdateMouseRays( );
	}
//   return 0;
}

static LOGICAL CPROCCameraDown( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
 			if( keycode & KEY_SHIFT_DOWN )
				Up( l.origin, SPEED_CONSTANT );
			else
				Up( l.origin, -SPEED_CONSTANT );
		}
		else
			Up( l.origin, 0.0 );
      UpdateMouseRays( );
//      return 1;
	}
//   return 0;
}

int IsTouchDisplay( void )
{
   return 0;
}

LOGICAL IsDisplayHidden( PRENDERER video )
{
   if( video )
		return video->flags.bHidden;
   return 0;
}

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 46
#endif
PRIORITY_PRELOAD( VideoRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	if( l.flags.bLogRegister )
		lprintf( WIDE("Regstering video interface...") );


	RegisterInterface( 
	   WIDE("puregl.render")
	   , GetDisplayInterface, DropDisplayInterface );
	RegisterInterface( 
	   WIDE("puregl.render.3d")
	   , GetDisplay3dInterface, DropDisplay3dInterface );
	BindEventToKey( NULL, KEY_F4, KEY_MOD_RELEASE|KEY_MOD_ALT, DefaultExit, 0 );
	BindEventToKey( NULL, KEY_SCROLL_LOCK, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_F12, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_A, KEY_MOD_ALL_CHANGES, CameraLeft, 0 );
	BindEventToKey( NULL, KEY_S, KEY_MOD_ALL_CHANGES, CameraDown, 0 );
	BindEventToKey( NULL, KEY_D, KEY_MOD_ALL_CHANGES, CameraRight, 0 );
	BindEventToKey( NULL, KEY_W, KEY_MOD_ALL_CHANGES, CameraForward, 0 );
	BindEventToKey( NULL, KEY_Q, KEY_MOD_ALL_CHANGES, CameraRollLeft, 0 );
	BindEventToKey( NULL, KEY_E, KEY_MOD_ALL_CHANGES, CameraRollRight, 0 );
	//EnableLoggingOutput( TRUE );
}

//typedef struct sprite_method_tag *PSPRITE_METHOD;

PSPRITE_METHOD  EnableSpriteMethod (PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	// add a sprite callback to the image.
	// enable copy image, and restore image
	PSPRITE_METHOD psm = New( struct sprite_method_tag );
	psm->renderer = render;
	psm->saved_spots = CreateDataQueue( sizeof( struct saved_location ) );
	psm->RenderSprites = RenderSprites;
	psm->psv = psv;
	AddLink( &render->sprites, psm );
   return psm; // the sprite should assign this...
}

// this is a magic routine, and should only be called by sprite itself
// and therefore this is handed to the image library via an export into image library
// this is done this way, because the image library MUST exist before this library
// therefore relying on the linker to handle this export is not possible.
static void CPROC SavePortion( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h )
{
	struct saved_location location;
   location.x = x;
   location.y = y;
   location.w = w;
	location.h = h;
	//lprintf( "Save Portion %d,%d %d,%d", x, y, w, h );
	EnqueData( &psm->saved_spots, &location );
	//lprintf( "Save Portion %d,%d %d,%d", x, y, w, h );
}

PRELOAD( InitSetSavePortion )
{
   //SetSavePortion( SavePortion );
}

void LockRenderer( PRENDERER render )
{
	EnterCriticalSec( &render->cs );
}

void UnlockRenderer( PRENDERER render )
{
	LeaveCriticalSec( &render->cs );
}







RENDER_NAMESPACE_END

