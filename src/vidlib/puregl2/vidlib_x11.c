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


#define NEED_VECTLIB_COMPARE
#define NEED_REAL_IMAGE_STRUCTURE

#include <stdhdrs.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */
#ifdef _WIN32
#include "../glext.h"
#endif

#include <vidlib/vidstruc.h>
#include <render3d.h>
#define NEED_VECTLIB_COMPARE
#include <image.h>
#include <vectlib.h>
//#include "vidlib.H"

#include <keybrd.h>




IMAGE_NAMESPACE

RENDER_NAMESPACE

// move local into render namespace.
#include "local.h"
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


/* Here goes our drawing code */
int drawGLScene( struct display_camera *camera, GLWindow *GLWin)
{
	ProcessGLDraw( TRUE );

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    Move( l.origin );
    Render3D( camera );
    
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
		x11_gl_window->screen = DefaultScreen(x11_gl_window->dpy);
#if 0
		x11_gl_window->atom_create = XInternAtom( x11_gl_window->dpy, "UserCreateWindow", 0 );

		XF86VidModeQueryVersion(x11_gl_window->dpy, &vidModeMajorVersion,
										&vidModeMinorVersion);
		printf("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion,
				 vidModeMinorVersion);
		if( x11_gl_window->fs )
		{
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
		}
#endif
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
		lprintf( "colormap..." );
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
			lprintf( "create window... %d,%d  %d", camera->w, camera->h, vi->depth );
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
		lprintf( " glx make current" );
		glXMakeCurrent(x11_gl_window->dpy, x11_gl_window->win, x11_gl_window->ctx);
		lprintf( "getgeometry..." );
		XGetGeometry(x11_gl_window->dpy, x11_gl_window->win, &winDummy, &x11_gl_window->x, &x11_gl_window->y,
						 &x11_gl_window->width, &x11_gl_window->height, &borderDummy, &x11_gl_window->depth);
		printf("Depth %d\n", x11_gl_window->depth);
		if (glXIsDirect(x11_gl_window->dpy, x11_gl_window->ctx))
			printf("Congrats, you have Direct Rendering!\n");
		else
			printf("Sorry, no Direct Rendering possible!\n");
	}
	else
	{
		lprintf( "Failed to open display" );
		return NULL;
	}
	return x11_gl_window;
}

#endif


TEXTCHAR  GetKeyText (int key)
{
   int used = 0;
	TEXTCHAR result = SACK_Vidlib_GetKeyText( IsKeyPressed( key ), KEY_CODE( key ), &used );
	if( used )
		return result;
   return 0;
}

void  GetDisplaySizeEx ( int nDisplay
												 , S_32 *x, S_32 *y
												 , _32 *width, _32 *height)
{
		if( x )
         (*x) = 0;
		if( y )
			(*y) = 0;
		{
			int set = 0;
			if( l.cameras )
			{
				struct display_camera *camera = (struct display_camera *)GetLink( &l.cameras, 0 );
				if( camera && ( camera != (struct display_camera*)1 ) )
				{
               set = 1;
					if( width )
						(*width) = camera->w;
					if( height )
						(*height) = camera->h;
				}
			}
         if( !set )
			{
				if( width )
					(*width) = l.default_display_x;
				if( height )
					(*height) = l.default_display_y;
			}
		}
}

ATEXIT( hold_exit )
{
   DebugBreak();
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
/* function called when our window is resized (should only happen in window mode) */
void resizeGLScene(unsigned int width, unsigned int height)
{
    if (height == 0)    /* Prevent A Divide By Zero If The Window Is Too Small */
        height = 1;
    glViewport(0, 0, width, height);    /* Reset The Current Viewport And Perspective Transformation */
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //MygluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
    //glMatrixMode(GL_MODELVIEW);
}

//----------------------------------------------------------------------------

static void HandleMessage (PRENDERER gl_camera, GLWindow *x11_gl_window, XEvent *event)
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
        lprintf( "Mouse down... %d %d %d", x11_gl_window->mouse_x,x11_gl_window->mouse_y,x11_gl_window->mouse_b );
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
        lprintf( "Mouse up... %d %d %d", x11_gl_window->mouse_x,x11_gl_window->mouse_y,x11_gl_window->mouse_b );
        InvokeMouseEvent( gl_camera, x11_gl_window );
        break;
    case MotionNotify:
        x11_gl_window->mouse_x = event->xmotion.x;
		  x11_gl_window->mouse_y = event->xmotion.y;
        lprintf( "Mouse move... %d %d %d", x11_gl_window->mouse_x,x11_gl_window->mouse_y,x11_gl_window->mouse_b );
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



PTRSZVAL CPROC ProcessDisplayMessages( PTHREAD thread )
{
	XEvent event;
	struct display_camera *camera;
	INDEX idx;
   struct display_camera *did_one;
lprintf( "Load options..." );
	LoadOptions(); // loads camera config, and logging options...
lprintf( "Open Cameras..." );
	SACK_Vidlib_OpenCameras();  // create logical camera structures
	l.bThreadRunning = 1;
	while( 1 )
	{
		did_one = NULL;

		LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
		{
			//lprintf( "Checking Thread %Lx", GetThreadID( MakeThread() ) );
			GLWindow *x11_gl_window = camera->hVidCore->x11_gl_window;
			if( !x11_gl_window )
			{
				lprintf( "opening real output..." );
				x11_gl_window = camera->hVidCore->x11_gl_window = createGLWindow( camera );  // opens the physical device
			}
			//lprintf( "is it %Lx?", GetThreadID( thread ) );
			{
				while( XPending( x11_gl_window->dpy ) > 0 )
				{
					did_one = camera;
					XNextEvent(x11_gl_window->dpy, &event);
					//if( l.flags.bLogMessageDispatch )
					//	lprintf( WIDE("(E)Got message:%d"), event.type );
					HandleMessage( camera->hVidCore, x11_gl_window, &event );
					//if( l.flags.bLogMessageDispatch )
					//	lprintf( WIDE("(X)Got message:%d"), event.type );
				}
				//lprintf( "Draw GL..." );
				//drawGLScene( camera, x11_gl_window );

				//  calls Update; moves the camera if it has a motion...
				// does a global tick then draws all cameras
				// returns if draw should be done; might step and draw one message
				// loop for each camera instead
				ProcessGLDraw( TRUE );
			}
		}
		//if( !did_one )
		Relinquish();
	}
	return 1;
}

// init the thread;
// without cameras, the thread will be idle anyway, so we should be able to
// start this slightly before registering the interface
PRIORITY_PRELOAD( VideoRegisterInterface, DEFAULT_PRELOAD_PRIORITY + 3 )
{
   ThreadTo( ProcessDisplayMessages, 0 );
}

RENDER_NAMESPACE_END

