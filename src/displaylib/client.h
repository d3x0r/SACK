typedef struct display_tag *PPANEL;

#include <image.h>
#include <vidlib/vidstruc.h>
#include <render.h>



#ifdef DISPLAY_CLIENT
#define g global_display_client_data
typedef struct global_tag {
	struct {
		_32 connected : 1;
		_32 disconnected : 1;
		_32 redraw_dispatched : 1;
	} flags;
	_32 MsgBase;
	pid_t MessageSource;
	PLIST pDisplayList;  // list of displays which are open (probably very short)
   PKEYDEFINE KeyDefs;

} GLOBAL;
#elif defined( DISPLAY_IMAGE_CLIENT )
#define g global_image_client_data
typedef struct my_font_tag {
	SFTFont font; // font as the service knows it.
   SFTFont data; // font data as the service would kknow it.
} MYFONT, *PMYFONT;

typedef struct global_tag {
	struct {
		_32 connected : 1;
		_32 disconnected : 1;
	} flags;
	_32 MsgBase;

   PMYFONT DefaultFont;     // actual value returned from the service
	PMYFONT DefaultFontData; // actual data contained in the service

	pid_t MessageSource;
   PLIST pDisplayList;  // list of displays which are open (probably very short)
} GLOBAL;
#endif

#ifndef GLOBAL_STRUCTURE_DEFINED
extern
#endif
GLOBAL g;


// this is what the local client knows about
// displays, including things like the callbacks
// since on the server side it knows nothing about
// the client's program space.
typedef struct my_image_tag {
	ImageData;       // our relavent info about the image...
	Image RealImage; // handle passed from server
	IMAGE_RECTANGLE auxrect;
} *PMyImage, MyImage;

typedef struct display_tag {
	PRENDERER hDisplay; // handle this display represents.
	// need to keep this so that when we destroy it we can destroy
	// the image - this keeps the burden of knowing what's valid to
	// destroy to a mimimum... but if the application keeps the handle
	// and insists on using it, then of course that's an application fault.
	struct {
		_32 mouse_pending : 1;
		_32 draw_pending : 1;
		_32 redraw_dispatched : 1;
	} flags;
	PMyImage DisplayImage;  // this has to be updated whenever the server's is
	                        // on event callbacks - move, resize, etc.

	S_32 x, y;
	_32 width, height;

	CloseCallback CloseMethod;
	PTRSZVAL CloseData;
	struct {
		S_32 x, y;
      _32 b;
	} mouse_pending;
	MouseCallback MouseMethod;
	PTRSZVAL MouseData;
	// every move/size is accompanied by a redraw
   // but a redraw may happen without a resize/move
	RedrawCallback RedrawMethod;
	PTRSZVAL RedrawData;
	KeyProc KeyMethod;
	PTRSZVAL KeyData;
	LoseFocusCallback LoseFocusMethod;
	PTRSZVAL LoseFocusData;
#ifdef ACTIVE_MESSAGE_IMPLEMENTED
	GeneralCallback GeneralMethod;
	PTRSZVAL GeneralData;
#endif

   KEYDEFINE KeyDefs[256];
	FLAGSET( KeyboardState, 512 );
	FLAGSET( KeyboardMetaState, 256 ); //
	_32 KeyDownTime[256];
} DISPLAY, *PDISPLAY;



