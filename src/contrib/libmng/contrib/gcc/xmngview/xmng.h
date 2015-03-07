#ifndef _XMNG_H_
#define _XMNG_H_
#define CANVAS_RGB8_SIZE		3
#define CANVAS_RGBA8_SIZE 		4
#define CANVAS_ARGB8_SIZE 		4
#define CANVAS_RGB8_A8_SIZE 	4
#define CANVAS_BGR8_SIZE 		3
#define CANVAS_BGRA8_SIZE 		4
#define CANVAS_BGRA8PM_SIZE 	4
#define CANVAS_ABGR8_SIZE 		4

#define MNG_MAGIC "\x8aMNG\x0d\x0a\x1a\x0a"
#define JNG_MAGIC "\x8bJNG\x0d\x0a\x1a\x0a"
#define PNG_MAGIC "\x89PNG\x0d\x0a\x1a\x0a"
#define PSEUDOCOLOR 1
#define TRUECOLOR   2

#define MNG_TYPE 1
#define JNG_TYPE 2
#define PNG_TYPE 3

#define SPACE_X	10
#define SPACE_Y 10
#define BUT_ENTRY_BORDER 0
#define FRAME_SHADOW_WIDTH 2
#define ANY_WIDTH 4

#define OK MNG_NOERROR 

typedef struct
{
	mng_handle user_handle;
	Widget canvas;
	int type;
    mng_uint32 delay;
    mng_uint32 img_width, img_height;
    mng_uint32 read_len;
	mng_uint32 read_pos;
	unsigned char *read_buf;
	unsigned char *mng_buf;
	unsigned char *dither_line;

	Window user_win;
	Window frame_win;
	Window control_win;
	GC gc;
	Display *dpy;
	Window win;
	unsigned short mng_rgb_size;
	unsigned short mng_bytes_per_line;
	XImage *ximage;
	void *shm;
	int gray;
	int display_depth, display_type;
	int have_shmem;
	Pixel bg_pixel;
	int user_bg;

	Visual *visual;
	unsigned int depth;
	int bpl;/* ximage->bytes_per_line */
	unsigned short frozen;
	unsigned short restarted;
	unsigned short stopped;
/* do not free */
    char *read_idf;
    FILE *reader;
	int *argc_ptr;
	char **argv;
} image_data, *image_data_ptr;

#define XPUTIMAGE(dpy,dr,gc,xi,a,b,c,d,w,h)                          \
    if (have_shmem)                                                  \
    XShmPutImage(dpy,dr,gc,xi,a,b,c,d,w,h,True);                 \
    else                                                             \
    XPutImage(dpy,dr,gc,xi,a,b,c,d,w,h)

extern void Viewer_postlude(void);
extern XImage *x11_create_ximage(image_data *data);
extern void x11_destroy_ximage(image_data *data);
extern void x11_init_color(image_data *data);
extern void viewer_renderline(image_data *data, unsigned char *scanline, 
	unsigned int row, unsigned int x, unsigned int width);

#endif
