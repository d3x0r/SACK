
#include <X11/Xlib.h>

typedef struct xvideo_tag
{
	struct {
		_32 bShown : 1;
	} flags;
	GC gc;
	Window win;
   int x, y, width, height;
} XPANEL, *PXPANEL;

typedef struct local_tag
{
	struct {
		_32 bInited : 1;
	} flags;
	Display* display;
	int screen_width;
	int screen_height;
   int screen_num;
/* these variables will be used to store the IDs of the black and white */
/* colors of the given screen. More on this will be explained later.    */
	unsigned long white_pixel;
	unsigned long black_pixel;

} LOCAL;
static LOCAL l;

RENDER_PROC (void, UpdateDisplayPortionEx)( PXPANEL pPanel
                                          , S_32 x, S_32 y
                                          , _32 w, _32 h DBG_PASS)
{
   ImageFile *pImage;
   if (pPanel
       && (pImage = pPanel->pImage) && pPanel->hDCBitmap && pPanel->hDCOutput)
   {
      if (!h)
         h = pImage->height;
      if (!w)
         w = pImage->width;

      //_xlprintf( 1 DBG_RELAY )( WIDE("Write to Window: %d %d %d %d"), x, y, w, h );

      if (!pPanel->flags.bShown)
		{
         lprintf( WIDE("Setting shown...") );
			pPanel->flags.bShown = TRUE;
#ifdef LOG_RECT_UPDATE
			_xlprintf( 1 DBG_RELAY )( WIDE("Show Window: %d %d %d %d"), x, y, w, h );
#endif
// this is done at update...
			XMapWindow(l.display, pPanel->win);
		}
		else
		{
         //XCopyPlane(display, bitmap, win, gc,
         // 0, 0,
         // bitmap_width, bitmap_height,
         // 100, 50,
         // 1);
			XCopyArea(display, bitmap, win, gc,
						 0, 0,
						 bitmap_width, bitmap_height,
                   /*
						 void glDrawPixels( GLsizei width,
												 GLsizei height,
												 GLenum format,
												 GLenum type,
												 const GLvoid *pixels )
                   */
		 }
	}


int errorHandler( Display *dpy, XErrorEvent *e )
{
    char errorText[1024];
    XGetErrorText( dpy, e->error_code, errorText, sizeof(errorText) );
    printf( WIDE("**********************************\n") );
    printf( WIDE("X Error: %s\n"), errorText );
    printf( WIDE("**********************************\n") );

    exit( 1 );
}

int InitDisplay( void )
{
	if( !l.flags.bInited )
	{
		char *display_name = getenv( WIDE("DISPLAY") );
		if( !display_name )
			display_name="127.0.0.1:0";
		l.display = XOpenDisplay( display_name );
		if( !l.display )
		{
         lprintf( WIDE("Failed to connect to X") );
			return;
		}
      l.screen_num = DefaultScreen(l.display);
		/* find the width of the default screen of our X server, in pixels. */
		l.screen_width = DisplayWidth(l.display, l.screen_num);

/* find the height of the default screen of our X server, in pixels. */
		l.screen_height = DisplayHeight(l.display, l.screen_num);

/* find the ID of the root window of the screen. */
		l.root_window = RootWindow(l.display, l.screen_num);

/* find the value of a white pixel on this screen. */
		l.white_pixel = WhitePixel(l.display, l.screen_num);

/* find the value of a black pixel on this screen. */
		l.black_pixel = BlackPixel(l.display, l.screen_num);

	}
}

BOOL CreateDrawingSurface( PXPANEL *pPanel )
{
   /* create the window, as specified earlier. */
	pPanel->win = XCreateSimpleWindow( l.display
												, l.root_window
												, pPanel->x, pPanel->y
												, pPanel->width, pPanel->height
												, 0 /*win_border_width*/
												, l.black_pixel
												, l.white_pixel
												);

	XSelectInput( l.display, pPanel->win
					, ExposureMask
					| ButtonPressMask
					| ButtonReleaseMask
					| KeyPressMask
					| KeyReleaseMask
					| EnterWindowMask
					| LeaveWindowMask
					| PointerMotionMask
					| ButtonMotionMask
					);
	{
	/* this variable will contain the color depth of the pixmap to create. */
	/* this 'depth' specifies the number of bits used to represent a color */
	/* index in the color map. the number of colors is 2 to the power of   */
	/* this depth.                                                         */
		int depth = DefaultDepth(display, DefaultScreen(display));
		pPanel->pixmap = XCreatePixmap(l.display, l.root_win
												, width, height
												, 32);
  	 100, 50	}
   // this is done at update...

}

void ShutdownVideo( void )
{
	if( l.flags.bInited )
	{
		if( l.display )
         XCloseDisplay( l.display );
	}
}
/* these variables are used to specify various attributes for the GC. */
/* initial values for the GC. */
XGCValues values = CapButt | JoinBevel;
/* which values in 'values' to check when creating the GC. */
unsigned long valuemask = GCCapStyle | GCJoinStyle;

/* create a new graphical context. */
gc = XCreateGC(display, win, valuemask, &values);
if (gc < 0) {
    fprintf(stderr, WIDE("XCreateGC: \n"));
}


static int CPROC ProcessDisplayMessages(void )
{
/* this structure will contain the event's data, once received. */
	XEvent an_event;

/* enter an "endless" loop of handling events. */
	while (1) {
		XNextEvent(display, &an_event);
		switch (an_event.type) {
		case Expose:
									  /* handle this event type... */
			break;
		case ButtonPress:
         break;
		case ButtonRelease:
         break;
		case KeyPress:
			{
				int x = an_event.xbutton.x;
				int y = an_event.xbutton.y;
				int the_win = an_event.xbutton.window;
				int button = an_event.xbutton.button;
            Time time = an_event.xbutton.time;
			}

			break;
		case MotionNotify:
			x = an_event.xmotion.x;
			y = an_event.xmotion.y;
			the_win = an_event.xmotion.window;

    /* if the 1st mouse button was held during this event, draw a pixel */
    /* at the mouse pointer location.                                   */
    if (an_event.xmotion.state & Button1Mask) {
        /* draw a pixel at the mouse position. */
        XDrawPoint(display, the_win, gc_draw, x, y);
    }
         break;
		case KeyRelease:
         break;
		default: /* unknown event type - ignore it. */
			break;
		}
	}
}


