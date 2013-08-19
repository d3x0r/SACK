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





static void Redraw( PRENDERER hVideo )
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




RENDER_NAMESPACE_END

