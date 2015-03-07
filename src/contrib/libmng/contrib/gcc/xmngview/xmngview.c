/* Built with libmng-1.0.5
 * <szukw000 :at: students.uni-mainz.de>
 *    This program my be redistributed under the terms of the
 *    GNU General Public Licence, version 2, or at your preference,
 *    any later version.
 *           
 * For more information about libmng please visit:
 * 
 * The official libmng web-site:
 *   http://www.libmng.com
 * 
 * Libmng's community on SourceForge:
 *   https://sourceforge.net/project/?group_id=5635
 * 
 * The official MNG homepage:
 *   http://www.libpng.org/pub/mng
 * 
 * The official PNG homepage:
 *   http://www.libpng.org/pub/png
 * 
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <libmng.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <X11/Xutil.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/FileSB.h>
#include <Xm/Label.h>
#include <X11/extensions/XShm.h>

#include <assert.h>

#include "xmng.h"

/* #define DEBUG */
/* #define DEBUG_DRAW */
/* #define DEBUG_DELAY */
/* #define DEBUG_X */
/* #define TEST_X */
/* #define DEBUG_READ */

static void run_viewer(FILE *reader, char *read_idf);

static mng_handle user_handle;
static image_data user_data;
static struct timeval start_tv, now_tv;
static XtIntervalId timeout_ID;
static char *prg_idf;

static XtAppContext app_context;
static  Widget toplevel, main_form, canvas, file_label;
static XmFontList file_font;
static Dimension start_width;

static void fsb_cancel_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtUnmanageChild(w);
}

void create_file_dialog(Widget w, char *button_text, char *title_text,
    void(*fsb_select_cb)(Widget,XtPointer,XtPointer))
{
    Arg args[4];
    int cnt;
    Widget dialog;
    XmString button_str, title_str, filter;
    Widget child;

    cnt = 0;
    dialog = XmCreateFileSelectionDialog(w, "Files", args, cnt);

	XtUnmanageChild(XmFileSelectionBoxGetChild(dialog,XmDIALOG_HELP_BUTTON));
    XtAddCallback(dialog, XmNcancelCallback, fsb_cancel_cb, NULL);
    XtAddCallback(dialog, XmNokCallback, fsb_select_cb, NULL);
    button_str = XmStringCreateLocalized(button_text);
    title_str = XmStringCreateLocalized(title_text);
	filter = XmStringCreateLocalized("*.[jmp]ng");
    XtVaSetValues(dialog,
        XmNokLabelString, button_str,
        XmNdialogTitle,   title_str,
		XmNpattern, filter,
		XmNfileFilterStyle, XmFILTER_NONE,
        NULL);
    XmStringFree(button_str);
    XmStringFree(title_str);
	XmStringFree(filter);
    child = XmFileSelectionBoxGetChild(dialog, XmDIALOG_FILTER_TEXT);
    XtVaSetValues(child, XmNfontList, file_font, NULL);
    child = XmFileSelectionBoxGetChild(dialog, XmDIALOG_DIR_LIST);
    XtVaSetValues(child, XmNfontList, file_font, NULL);
    child = XmFileSelectionBoxGetChild(dialog, XmDIALOG_LIST);
    XtVaSetValues(child, XmNfontList, file_font, NULL);
    child = XmFileSelectionBoxGetChild(dialog, XmDIALOG_TEXT);
    XtVaSetValues(child, XmNfontList, file_font, NULL);

    XtManageChild(dialog);
    XMapRaised(XtDisplay (dialog), XtWindow (XtParent (dialog)));
}

void run_mng_file_cb(Widget w, XtPointer client, XtPointer call)
{
    XmFileSelectionBoxCallbackStruct *fsb;
    char *read_idf;
	FILE *reader;

    XtUnmanageChild(w);
    fsb = (XmFileSelectionBoxCallbackStruct *)call;
    XmStringGetLtoR(fsb->value, XmSTRING_DEFAULT_CHARSET, &read_idf);

    if(read_idf == NULL || *read_idf == 0) return;

	reader = fopen(read_idf, "r");
    if(reader == NULL)
   {
    perror(read_idf);
    fprintf(stderr, "\n\n%s: cannot open file '%s'\n\n", prg_idf, read_idf);
    return;
   }

	run_viewer(reader, read_idf);

    free(read_idf);
}

static void user_reset_data(void)
{
	if(timeout_ID) XtRemoveTimeOut(timeout_ID);
	timeout_ID = 0;
	mng_cleanup(&user_data.user_handle);
	user_data.read_pos = 0;
	free(user_data.read_buf);
	user_data.read_buf = NULL;
	user_data.read_len = 0;
	user_data.img_width = 0;
	user_data.img_height = 0;
	user_data.mng_bytes_per_line = 0;
	user_data.bpl = 0;
	user_data.read_idf = NULL;
	user_data.frozen = 0;

	XClearWindow(user_data.dpy, user_data.win);

/* mng_buf and ximage are freed in user_init_data() or Viewer_postlude()
*/
}

static void player_stop_cb(Widget w, XtPointer client, XtPointer call)
{
	if(user_data.stopped) return;
	if(user_data.user_handle)
   {
	user_reset_data();
	user_data.stopped = 1;
   }
}

void browse_file_cb(Widget w, XtPointer client, XtPointer call)
{
	if(user_data.user_handle)
   {
	user_reset_data();
   }
	user_data.stopped = 0;
	user_data.frozen = 0;
	user_data.restarted = 0;
    create_file_dialog(w, "Select", "Select MNG file", run_mng_file_cb);
}

void Viewer_postlude(void)
{
	if(timeout_ID) XtRemoveTimeOut(timeout_ID);
	mng_cleanup(&user_data.user_handle);
	if(user_data.reader) fclose(user_data.reader);
	if(user_data.ximage) XDestroyImage(user_data.ximage);
	if(user_data.read_buf) free(user_data.read_buf);
	if(user_data.mng_buf) free(user_data.mng_buf);
	if(user_data.dither_line) free(user_data.dither_line);
	if(!user_data.user_win && user_data.dpy) XtCloseDisplay(user_data.dpy);
	fputc('\n', stderr);
}

#ifdef TEST_X
static void test_x(image_data *data)
{
    XWindowAttributes wattr;
    XPixmapFormatValues *pf;
    Visual *visual;
    int n;
    long             bpp;   /* Bytes per pixel */
    long             pad;   /* Scanline pad in byte */

    Window           target;
    Display         *dpy;
	Window root;
	int x, y;
	unsigned int width, height, border_width, depth;
	Status status;

    dpy = data->dpy;
    target = data->win;

	status = XGetGeometry(dpy, target, &root, &x, &y, &width, &height,
	  &border_width, &depth);

    XGetWindowAttributes(dpy, target, &wattr);


	fprintf(stderr,"\n\nUSERWIN %lu DATAWIN %lu and ROOT %lu\n",
	  data->user_win, target, root);
	fprintf(stderr,"X %d Y %d W %u H %u BORDER_W %u DEPTH %u \n\n",
	  x,y,width,height,border_width,depth);

    visual = wattr.visual;

    fprintf(stderr,"VisualId: 0x%lx\n", visual->visualid);
    fprintf(stderr,"BitmapPad  = %d\n", BitmapPad(dpy));
    fprintf(stderr,"BitmapUnit = %d\n", BitmapUnit(dpy));
    fprintf(stderr,"Depth      = %d\n", DefaultDepth(dpy,DefaultScreen(dpy)));
    fprintf(stderr,"RedMask    = 0x%lx\n", visual->red_mask);
    fprintf(stderr,"GreenMask  = 0x%lx\n", visual->green_mask);
    fprintf(stderr,"BlueMask   = 0x%lx\n", visual->blue_mask);
    fprintf(stderr,"Bits/RGB   = %d\n", visual->bits_per_rgb);
    bpp = 0;


    for(pf=XListPixmapFormats(dpy, &n); n--; pf++) {
        if (pf->depth == DefaultDepth(dpy, DefaultScreen(dpy))) {
            bpp = pf->bits_per_pixel/8;
            pad = pf->scanline_pad/8;
        }
        fprintf(stderr,"----------------\n");
        fprintf(stderr,"Depth          = %d\n", pf->depth);
        fprintf(stderr,"Bits Per Pixel = %d\n", pf->bits_per_pixel);
        fprintf(stderr,"Scanline Pad   = %d\n", pf->scanline_pad);
    }


}
#endif

static void user_init_data(image_data *data)
{
	unsigned int depth;
	int screen, row, col;
	Display *dpy;
	Pixel bg;
	XColor xcolor;

#ifdef TEST_X
	test_x(data);
#endif

	dpy = data->dpy;
    screen = DefaultScreen(dpy);
    depth = DefaultDepth(dpy, screen);
	data->depth = depth;

	if(!data->visual)
   {
	data->visual = DefaultVisual(dpy, screen);
	data->gc = DefaultGC(dpy, DefaultScreen(dpy));
   }else
   {
	free(data->mng_buf);
	x11_destroy_ximage(data);
   }
	if(data->canvas)
   {

	XtVaGetValues(data->canvas, XmNbackground, &bg, NULL);
	xcolor.pixel = bg;
	XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &xcolor);
   }else
   {
	if(data->user_bg)
  {
	xcolor.pixel = data->bg_pixel;
	XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &xcolor);
  }
	else
	XLookupColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
	  "grey80", &xcolor, &xcolor);
   }
	
	mng_set_bgcolor(data->user_handle, xcolor.red, xcolor.green,
	  xcolor.blue);

	data->mng_bytes_per_line = data->img_width * data->mng_rgb_size;
	data->mng_buf = (unsigned char*)
	  calloc(1, data->mng_bytes_per_line * data->img_height);
	data->dither_line = (unsigned char*)
	  calloc(1, data->mng_bytes_per_line);
	x11_init_color(data);
	data->ximage = x11_create_ximage(data);
	
	if(data->ximage == NULL)
   {
	Viewer_postlude();
	exit(0);
   }
	if(data->type == MNG_TYPE)
	  return;

	row = -1;
	while(++row < data->img_height)
   {
	col = -1;
	while(++col < data->img_width)
  {
	XPutPixel(data->ximage, col, row, xcolor.pixel);
  }
   }
}

static void player_exit_cb(Widget w, XtPointer client, XtPointer call)
{
    Viewer_postlude();
    exit(0);
}

static void player_pause_cb(Widget w, XtPointer client, XtPointer call)
{
	if(user_data.stopped) return;
	if(user_data.frozen) return;
	if(timeout_ID) XtRemoveTimeOut(timeout_ID);
	timeout_ID = 0;
	mng_display_freeze(user_data.user_handle);
	user_data.frozen = 1;
}

static void player_resume_cb(Widget w, XtPointer client, XtPointer call)
{
	if(!user_data.frozen) return;
	user_data.frozen = 0;
	mng_display_resume(user_data.user_handle);
}

static void player_restart_cb(Widget w, XtPointer client, XtPointer call)
{
	if(user_data.stopped) return;
	if(timeout_ID) XtRemoveTimeOut(timeout_ID);
	timeout_ID = 0;
	
	user_data.frozen = 0;
	user_data.read_pos = 0;
	gettimeofday(&start_tv, NULL);

	mng_reset(user_data.user_handle);
	user_data.restarted = 1;
	mng_readdisplay(user_data.user_handle);
}

static void release_event_cb(Widget w, XtPointer client, XEvent *event,
	 Boolean *cont)
{
    Viewer_postlude();
    exit(0);
}

static void redraw(int type)
{
	if((type == Expose || type == GraphicsExpose)
	&& user_data.ximage)
   {
	  XPutImage(user_data.dpy, user_data.win, user_data.gc, user_data.ximage,
	    0, 0, 0, 0, user_data.img_width, user_data.img_height);
   }
}

static void exposures_cb(Widget w, XtPointer client,
    XmDrawingAreaCallbackStruct *cbs)
{

	redraw(cbs->event->xany.type);
}

static mng_ptr user_alloc(mng_size_t len)
{
    return calloc(1, len + 2);
}

static void user_free(mng_ptr buf, mng_size_t len)
{
    free(buf);
}

static mng_bool user_read(mng_handle user_handle, mng_ptr out_buf, 
	mng_uint32  req_len, mng_uint32 *out_len)
{
    mng_uint32 more;
    image_data *data;
#ifdef DEBUG_READ
fprintf(stderr,"\n\tuser_read req %d ",req_len);
#endif
    data = (image_data *)mng_get_userdata(user_handle);

	more = data->read_len - data->read_pos;

	if(more > 0
	&& data->read_buf != NULL)
   {
	if(req_len < more) more = req_len;
	memcpy(out_buf, data->read_buf + data->read_pos, more);
	data->read_pos += more;
	*out_len = more;
#ifdef DEBUG_READ
fprintf(stderr,"sent %d",req_len);
#endif
    return MNG_TRUE;
   }
#ifdef DEBUG
fprintf(stderr,"\n%s:%5d:user_read\n\tnothing to read\n",__FILE__,__LINE__);
#endif
	return MNG_FALSE;
}

static mng_bool user_open_stream(mng_handle user_handle)
{
/* stream already open */
#ifdef DEBUG
fprintf(stderr,"\nuser_open_stream\n");
#endif
    return MNG_TRUE;
}

static mng_bool user_close_stream(mng_handle user_handle)
{
	image_data *data;

	data = (image_data*)mng_get_userdata(user_handle);

	if(data->type == MNG_TYPE)
	  return MNG_TRUE;
   {
	int have_shmem, row, src_len;
	unsigned char *src;	

	have_shmem = data->have_shmem;
	src = data->mng_buf;
	src_len = data->mng_bytes_per_line;

	row = -1;
	while(++row < data->img_height)
  {
	viewer_renderline(data, src, row, 0, data->img_width);
	
	 src += src_len;
  }
	XPUTIMAGE(data->dpy, data->win, data->gc, data->ximage,
	  0, 0, 0, 0, data->img_width, data->img_height);
	XSync(data->dpy, False);
	return MNG_TRUE;
   }
}

static void create_widgets(mng_uint32 width, mng_uint32 height)
{
	Widget but_rc, but_frame, canvas_frame;
	Widget but1, but2, but3, but4, but5, but6;

    toplevel = XtAppInitialize(&app_context, "xmngview", NULL, 0, 
	user_data.argc_ptr, user_data.argv,
      0, 0, 0);

    main_form = XtVaCreateManagedWidget("main_form",
      xmFormWidgetClass, toplevel,
	  XmNhorizontalSpacing, SPACE_X, 
	  XmNverticalSpacing, SPACE_Y,
	  XmNresizable, True,
      NULL);
	but_frame = XtVaCreateManagedWidget("but_frame",
	  xmFrameWidgetClass, main_form,
	  XmNshadowType, XmSHADOW_ETCHED_OUT,
	  XmNtopAttachment, XmATTACH_FORM,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNshadowThickness, FRAME_SHADOW_WIDTH,
	  NULL);

    but_rc = XtVaCreateManagedWidget("but_rc",
      xmRowColumnWidgetClass,  but_frame,
      XmNentryAlignment, XmALIGNMENT_CENTER,
      XmNorientation, XmHORIZONTAL,
      XmNpacking, XmPACK_COLUMN,
      XmNnumColumns, 1,
	  XmNresizeWidth, True,
      XmNentryBorder, BUT_ENTRY_BORDER,
      NULL);

    but1 = XtVaCreateManagedWidget("Exit",
      xmPushButtonWidgetClass, but_rc,
      NULL);
    XtAddCallback(but1, XmNactivateCallback,
      player_exit_cb, (XtPointer)toplevel);

    but2 = XtVaCreateManagedWidget("Pause",
      xmPushButtonWidgetClass, but_rc,
      NULL);
    XtAddCallback(but2, XmNactivateCallback,
      player_pause_cb, (XtPointer)toplevel);

    but3 = XtVaCreateManagedWidget("GoOn",
      xmPushButtonWidgetClass, but_rc,
      NULL);
    XtAddCallback(but3, XmNactivateCallback,
      player_resume_cb, NULL);

    but4 = XtVaCreateManagedWidget("Restart",
      xmPushButtonWidgetClass, but_rc,
      NULL);
    XtAddCallback(but4, XmNactivateCallback,
      player_restart_cb, NULL);

    but5 = XtVaCreateManagedWidget("Stop",
      xmPushButtonWidgetClass, but_rc,
      NULL);
    XtAddCallback(but5, XmNactivateCallback,
      player_stop_cb, NULL);

    but6 = XtVaCreateManagedWidget("Browse",
      xmPushButtonWidgetClass, but_rc,
      NULL);
    XtAddCallback(but6, XmNactivateCallback,
      browse_file_cb, NULL);

	file_label = XtVaCreateManagedWidget("FILE: ",
	  xmLabelWidgetClass, main_form,
	  XmNalignment, XmALIGNMENT_BEGINNING,
	  XmNtopAttachment, XmATTACH_WIDGET,
	  XmNtopWidget, but_frame,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_FORM,
	  NULL);

	canvas_frame = XtVaCreateManagedWidget("canvas_frame",
	  xmFrameWidgetClass, main_form,
	  XmNshadowType, XmSHADOW_ETCHED_OUT,
      XmNtopAttachment, XmATTACH_WIDGET,
      XmNtopWidget, file_label,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_FORM,
	  NULL);

    canvas = XtVaCreateManagedWidget("canvas",
      xmDrawingAreaWidgetClass, canvas_frame,
      XmNheight, height,
      XmNwidth, width,
      NULL);

    XtAddEventHandler(canvas,
      ButtonReleaseMask|ButtonPressMask,
      False, release_event_cb, (XtPointer)toplevel);

	XtAddCallback(canvas, 
	  XmNexposeCallback, (XtCallbackProc)exposures_cb, (XtPointer)&user_data);
	
    XtRealizeWidget(toplevel);

	if(start_width == 0)
   {
	start_width = (FRAME_SHADOW_WIDTH<<1);
	XtVaGetValues(but1, XmNwidth, &width, NULL);
	start_width += width + (BUT_ENTRY_BORDER<<1) + ANY_WIDTH;
	XtVaGetValues(but2, XmNwidth, &width, NULL);
	start_width += width + (BUT_ENTRY_BORDER<<1) + ANY_WIDTH;
	XtVaGetValues(but3, XmNwidth, &width, NULL);
	start_width += width + (BUT_ENTRY_BORDER<<1) + ANY_WIDTH;
	XtVaGetValues(but4, XmNwidth, &width, NULL);
	start_width += width + (BUT_ENTRY_BORDER<<1) + ANY_WIDTH;
	XtVaGetValues(but5, XmNwidth, &width, NULL);
	start_width += width + (BUT_ENTRY_BORDER<<1) + ANY_WIDTH;
	XtVaGetValues(but6, XmNwidth, &width, NULL);
	start_width += width + (BUT_ENTRY_BORDER<<1);
   }

	user_data.canvas = canvas;
    user_data.dpy = XtDisplay(user_data.canvas);
    user_data.win = XtWindow(user_data.canvas);
    file_font = XmFontListAppendEntry(NULL,
        XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG,
        XmFONT_IS_FONT,
        XLoadQueryFont(user_data.dpy,
        "-*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1")));
}

static mng_bool user_process_header(mng_handle user_handle,
    mng_uint32 width, mng_uint32 height)
{
    image_data *data;
	Dimension cw, ch, tw, th, dh, dw, fw, fh;
	XmString xmstr;
	char *s, buf[128];

    data = (image_data*)mng_get_userdata(user_handle);
#ifdef DEBUG
fprintf(stderr,"\nuser_process_header: w %d h %d USER_WIN %lu\n",
width,height,data->user_win);
#endif

	if(data->restarted)
   {
	data->restarted = 0;
	return MNG_TRUE;
   }
	data->img_width = width;
	data->img_height = height;

	if(!data->user_win)
   {
	if(!data->canvas)
	  create_widgets(width, height);
	else
  {
	XtVaGetValues(toplevel, XmNwidth, &tw, XmNheight, &th, NULL);
	XtVaGetValues(main_form, XmNwidth, &fw, XmNheight, &fh, NULL);
	XtVaGetValues(data->canvas, XmNwidth, &cw, XmNheight, &ch, NULL);

	if(height > ch)
 {
	dh = height - ch;
	th += dh;
	fh += dh;
 }	else
	if(ch > height)
 {
	dh = ch - height;
	th -= dh;
	fh -= dh;
 }
	if(width > cw)
 {
	dw = width - cw;
	tw += dw;
	fw += dw;
 }	else
	if(cw > width)
 {
	if(width > start_width)
	  dw = cw - width;
	else
	  dw = cw - start_width;
	tw -= dw;
	fw -= dw;
 }
	if(fw < start_width)
 {
	tw = start_width + (SPACE_X<<1);
	fw = start_width;
 }
	XtVaSetValues(toplevel, XmNwidth,tw  , XmNheight,th , NULL);
	XtVaSetValues(main_form, XmNwidth,fw  , XmNheight,fh , NULL);
	XtVaSetValues(data->canvas, XmNwidth,width  , XmNheight,height , NULL);
  }
   }else
	if(data->user_win)
   {
	Display *dpy;

	XtToolkitInitialize();
	app_context = XtCreateApplicationContext();
	dpy = XtOpenDisplay(app_context, NULL,NULL,"xmngview",
		NULL, 0, data->argc_ptr, data->argv);
	data->dpy = dpy;

    data->win = 
	  XCreateSimpleWindow(dpy, data->user_win,
	    0,0,
        width, height,
        0, WhitePixel(dpy, DefaultScreen(dpy)),
	    data->bg_pixel);
    XMapWindow(dpy, data->win);
	
	XSelectInput(dpy, data->win, ExposureMask);
   }
	user_init_data(data);

	if(data->canvas)
   {
	s = strrchr(data->read_idf, '/');
	if(s == NULL) s = data->read_idf; else ++s;
	s = strdup(s);
	if(strlen(s) > 64) s[64] = 0;
	sprintf(buf, "%s (%d x %d)", s, data->img_width, data->img_height);
	xmstr = XmStringCreateLtoR(buf, XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(file_label, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);
	free(s);
   }
	gettimeofday(&start_tv, NULL);
	return MNG_TRUE;
}

static void wait_cb(XtPointer client, XtIntervalId * id)
{
	timeout_ID = 0;
	mng_display_resume(user_data.user_handle);
}

static mng_bool user_set_timer(mng_handle user_handle, mng_uint32 delay)
{
#ifdef DEBUG
fprintf(stderr,"\nuser_set_timer: %d\n", delay);
#endif
	timeout_ID = XtAppAddTimeOut(app_context,
	  delay, wait_cb, NULL);

	return MNG_TRUE;
}

static mng_uint32 user_get_tick_count(mng_handle user_handle)
{
	mng_uint32 ticks;
	
	gettimeofday(&now_tv, NULL);
	ticks = (now_tv.tv_sec - start_tv.tv_sec) * 1000
	   + (now_tv.tv_usec - start_tv.tv_usec)/ 1000;
#ifdef DEBUG_DELAY
fprintf(stderr,"\nuser_get_tick_count %d", ticks);
#endif
	return ticks;
}

static mng_ptr user_get_canvas_line(mng_handle user_handle, mng_uint32 line)
{
	image_data *data;
#ifdef DEBUG
fprintf(stderr,"\nuser_get_canvas_line %d",line);
#endif
	data = (image_data*)mng_get_userdata(user_handle);

	return data->mng_buf + data->mng_bytes_per_line * line;
}

#define FRAME_H 32

static mng_bool user_refresh(mng_handle user_handle, mng_uint32 x,
    mng_uint32 y, mng_uint32 width, mng_uint32 height)
{
    image_data *data;
    mng_uint32 src_len;
    unsigned char *src_start, *src_buf;
    int row, max_row;
    Display *dpy;
    GC gc;
    Window win;
    XImage *ximage;
    Visual *visual;
	int have_shmem;

	data = (image_data*)mng_get_userdata(user_handle);

#ifdef DEBUG_DRAW
fprintf(stderr,"\nuser_refresh:"
" RECT x %3d y %3d w %3d h %3d", x, y, width, height);
#endif

    data = (image_data*)mng_get_userdata(user_handle);
    win = data->win;
    gc = data->gc;
    dpy = data->dpy;
    ximage = data->ximage;
    visual = data->visual;
	have_shmem = data->have_shmem;

    max_row = y + height;
    row = y;
    src_len = data->mng_bytes_per_line;
    src_buf = src_start = data->mng_buf + data->mng_rgb_size * x + y * src_len;

    while(row < max_row)
  {
	viewer_renderline(data, src_start, row, x, width);

    ++row;
    src_start += src_len;
  }
	XPUTIMAGE(dpy, win, gc, ximage, x, y, x, y, width, height);
	XSync(dpy, False);	
    return MNG_TRUE;
}

static mng_bool user_error(mng_handle user_handle, mng_int32 code, 
	mng_int8 severity,
    mng_chunkid chunktype, mng_uint32 chunkseq,
    mng_int32 extra1, mng_int32 extra2, mng_pchar text)
{
    image_data *data;
    char        chunk[5];

	data = (image_data*)mng_get_userdata(user_handle);

    chunk[0] = (char)((chunktype >> 24) & 0xFF);
    chunk[1] = (char)((chunktype >> 16) & 0xFF);
    chunk[2] = (char)((chunktype >>  8) & 0xFF);
    chunk[3] = (char)((chunktype      ) & 0xFF);
    chunk[4] = '\0';

    fprintf(stderr, "\n\n%s: error playing '%s' chunk %s (%d):\n",
        prg_idf, data->read_idf, chunk, chunkseq);
    fprintf(stderr, "code %d severity %d extra1 %d extra2 %d"
	  "\ntext:'%s'\n\n", code, severity, extra1, extra2, text);

    return (0);
}

static mng_bool prelude(void)
{
#define MAXBUF 8
    unsigned char buf[MAXBUF];
	
#ifdef DEBUG
fprintf(stderr,"\nprelude\n");
#endif

    if(fread(buf, 1, MAXBUF, user_data.reader) != MAXBUF)
   {
	fprintf(stderr,"\n%s:prelude\n\tcannot read signature \n",
	  prg_idf);
      return MNG_FALSE;
   }
	
	if(memcmp(buf, MNG_MAGIC, 8) == 0)
	  user_data.type = MNG_TYPE;
	else
	if(memcmp(buf, JNG_MAGIC, 8) == 0)
	  user_data.type = JNG_TYPE;
	else
	if(memcmp(buf, PNG_MAGIC, 8) == 0)
	  user_data.type = PNG_TYPE;
	if(!user_data.type)
   {
	fprintf(stderr,"\n%s:'%s' is no MNG / JNG / PNG file\n", 
	prg_idf, user_data.read_idf);
    return MNG_FALSE;
   }

    fseek(user_data.reader, 0, SEEK_SET);
    fseek(user_data.reader, 0, SEEK_END);
    user_data.read_len = ftell(user_data.reader);
    fseek(user_data.reader, 0, SEEK_SET);

	if(!user_data.user_handle)
   {
    user_handle = mng_initialize(&user_data, user_alloc, user_free, MNG_NULL);

    if(user_handle == MNG_NULL)
  {
    fprintf(stderr, "\n%s: cannot initialize libmng.\n", prg_idf);
    return MNG_FALSE;
  }
	user_data.user_handle = user_handle;

	mng_set_canvasstyle(user_handle, MNG_CANVAS_RGB8);
	user_data.mng_rgb_size = CANVAS_RGB8_SIZE;

    if(mng_setcb_openstream(user_handle, user_open_stream) != OK
    || mng_setcb_closestream(user_handle, user_close_stream) != OK
    || mng_setcb_readdata(user_handle, user_read) != OK
	|| mng_setcb_settimer(user_handle, user_set_timer) != OK
	|| mng_setcb_gettickcount(user_handle, user_get_tick_count) != OK
    || mng_setcb_processheader(user_handle, user_process_header) != OK
	|| mng_setcb_getcanvasline(user_handle, user_get_canvas_line) != OK
	|| mng_setcb_refresh(user_handle, user_refresh) != OK
	|| mng_setcb_errorproc(user_handle, user_error) != OK
      )
  {
    fprintf(stderr,"\n%s: cannot set callbacks for libmng.\n",
	  prg_idf);
    return MNG_FALSE;
  }
   }
	user_data.read_buf = (unsigned char*)calloc(1, user_data.read_len + 2);
	fread(user_data.read_buf, 1, user_data.read_len, user_data.reader);
	fclose(user_data.reader);
	user_data.reader = NULL;

    return MNG_TRUE;
}

static void run_viewer(FILE *reader, char *read_idf)
{
	XEvent event;

	user_data.read_idf = read_idf;
	user_data.reader = reader;

	if(read_idf != NULL)
   {
	if(prelude() == MNG_FALSE)
	  return ;

	gettimeofday(&start_tv, NULL);
	mng_readdisplay(user_data.user_handle);
   }

	if(!user_data.user_win)
  {
	XtAppMainLoop(app_context);
  }
	else
	while(1)
  {
	XtAppNextEvent(app_context, &event);

	redraw(event.type);
  }
}

int main(int argc, char **argv)
{
	FILE *reader;
	char *read_idf, *s;
	int i, user_bg;
	Window user_win;
	Pixel bg_pixel;

    if((prg_idf = strrchr(argv[0], '/')) == NULL)
      prg_idf = argv[0];
    else
      ++prg_idf;

	user_win = 0; read_idf = NULL; reader = NULL;
	user_bg = 0; bg_pixel = 0;
	i = argc;

    while(--i > 0)
   {
    s = argv[i];

    if(strncmp(s, "-w", 2) == 0)
  {
    user_win = atol(s+2); 
	continue;
  }
    if(strncmp(s, "-bg", 3) == 0)
  {
    bg_pixel = atol(s+3);
	user_bg = 1;
	continue;
  }
    if(*s != '-')
  {
    read_idf = s; continue;
  }
   }
	if(read_idf != NULL)
   {
	reader = fopen(read_idf, "rb");
	if(reader == NULL)
  {
	perror(read_idf);
	fprintf(stderr, "\n\n%s: cannot open file '%s'\n\n", prg_idf, read_idf);
	return 0;
  }
   }
	memset(&user_data, 0, sizeof(image_data));
	user_data.argv = argv;
	user_data.argc_ptr = &argc;
	user_data.user_win = user_win;
	user_data.bg_pixel = bg_pixel;
	user_data.user_bg = user_bg;

	if(read_idf == NULL && user_win == 0)
	  create_widgets(5,5);

	run_viewer(reader, read_idf);

	Viewer_postlude();
	return 0;
}
