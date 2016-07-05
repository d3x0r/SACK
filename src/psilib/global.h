#ifndef GLOBAL_STRUCTURE_DEFINED
#define GLOBAL_STRUCTURE_DEFINED

#include <stdhdrs.h>

//DOM-IGNORE-BEGIN
#define g global_psi_structure
//DOM-IGNORE-END
#ifndef PSI_SERVICE
#  ifndef FORCE_NO_INTERFACE
#    ifndef USE_IMAGE_INTERFACE // some people including this may have already defined this
#define USE_IMAGE_INTERFACE g.MyImageInterface
#    endif
#  endif
#endif
#include <image.h>

#ifdef __cplusplus_cli
//#  undef USE_INTERFACES
#else
#  ifdef FORCE_NO_INTERFACE
#    undef USE_INTERFACES
#  else
#    define USE_INTERFACES
#  endif
#endif

#ifndef PSI_SERVICE
#  ifndef FORCE_NO_INTERFACE
#define USE_RENDER_INTERFACE g.MyDisplayInterface
#  endif
#endif
#include <render.h>

#include <timers.h>
#include <psi/namespaces.h>
#include <controls.h>


PSI_NAMESPACE

#if !defined( CONTROL_BASE )
extern
#endif
	CDATA DefaultColors[14];


//DOM-IGNORE-BEGIN
typedef struct psi_global_tag
{
#ifndef PSI_SERVICE
#  ifndef FORCE_NO_INTERFACE
	PIMAGE_INTERFACE MyImageInterface;
	PRENDER_INTERFACE MyDisplayInterface;
#endif
#endif
	struct {
		// will be set optionally when on a touch display
		// which will auto-magically grow certain controls
      // listbox, scrollbar...
		BIT_FIELD touch_display : 1;
		BIT_FIELD fancy_border : 1;
		BIT_FIELD bLogDebugUpdate : 1;
		BIT_FIELD always_draw : 1; // removes dirty, and expects to have to draw every display frame
		BIT_FIELD bLogDetailedMouse : 1;
		BIT_FIELD bLogSuperDetailedMouse : 1;
		BIT_FIELD bLogKeyEvents : 1;
		BIT_FIELD allow_threaded_draw : 1;
		BIT_FIELD system_color_set : 1; // don't use fancy border... unless it's also set externally
		BIT_FIELD allow_copy_from_render : 1;
	} flags;
	PLIST borders; 
	PFrameBorder DefaultBorder;
#if 0
	CDATA *defaultcolors;
	S_32 BorderWidth;
	S_32 BorderHeight;
	struct psi_global_border_info {
		BIT_FIELD bAnchorTop : 2; // 0 = none, 1=left, 2=center, 3=right
		BIT_FIELD bAnchorBottom : 2; // 0 = none, 1=left, 2=center, 3=right
		BIT_FIELD bAnchorLeft : 2; // 0 = none, 1=top, 2=center, 3=bottom
		BIT_FIELD bAnchorRight : 2; // 0 = none, 1=top, 2=center, 3=bottom
	} Border;
	Image BorderImage;
	Image BorderSegment[9]; // really 8, but symetry is kept
#endif
	Image FrameCaptionImage;
	Image FrameCaptionFocusedImage;
	Image StopButton;
	Image StopButtonPressed;
	int   StopButtonPad;
	PLIST shown_frames;
	SFTFont default_font;
}PSI_GLOBAL;



enum {
	SEGMENT_TOP_LEFT
	  , SEGMENT_TOP
	  , SEGMENT_TOP_RIGHT
	  , SEGMENT_LEFT
	  , SEGMENT_CENTER
	  , SEGMENT_RIGHT
	  , SEGMENT_BOTTOM_LEFT
	  , SEGMENT_BOTTOM
	  , SEGMENT_BOTTOM_RIGHT
     // border segment index's
};
//#define defaultcolor g.defaultcolors
//#define basecolor(pc) ((pc)?((pc)->border?(pc)->border->defaultcolors:(pc)->basecolors):(g.DefaultBorder?g.DefaultBorder->defaultcolors:DefaultColors))

#if !defined( CONTROL_BASE ) && (defined( SOURCE_PSI2 ) || defined( __cplusplus_cli ))
extern
#endif
#ifndef __cplusplus_cli
#  ifdef WIN32
#    if !defined( SOURCE_PSI2 )
#      if !defined( CONTROL_BASE )
__declspec(dllimport)
#      else
__declspec(dllexport)
#      endif
#    endif
#  else
#    if !defined( SOURCE_PSI2 )
#      if !defined( CONTROL_BASE )
extern
#      else

#      endif
#    endif
#  endif
#endif
/* This is the structure PSI uses to track .... what? The
   application has to know its own handles... what does PSI keep
   anyhow? most methods are registered now.                      */
PSI_GLOBAL g
#ifndef CONTROL_BASE
;
#else
= { 
#ifndef PSI_SERVICE
#  ifndef FORCE_NO_INTERFACE
	NULL, NULL,
#  endif
#endif
  { 0 }
  // none of these should be 0 (black) may be 1 (nearest black)
  , 0 };

#endif
//DOM-IGNORE-END

#ifndef CONTROL_BASE
extern
#endif
	void GetMyInterface( void );

// --------- borders.c --------------
void UpdateSurface( PSI_CONTROL pc );
void CPROC DrawFancyFrame( PSI_CONTROL pc );
void CPROC DrawNormalFrame( PSI_CONTROL pc );

PSI_NAMESPACE_END



#endif
// $Log: global.h,v $
// Revision 1.13  2005/03/23 12:20:53  panther
// Fix positioning of common buttons.  Also do a quick implementation of fancy borders.
//
// Revision 1.12  2005/03/23 02:43:07  panther
// Okay probably a couple more badly initialized 'unusable' flags.. but font rendering/picking seems to work again.
//
// Revision 1.11  2005/03/22 12:41:58  panther
// Wow this transparency thing is going to rock! :) It was much closer than I had originally thought.  Need a new class of controls though to support click-masks.... oh yeah and buttons which have roundable scaleable edged based off of a dot/circle
//
// Revision 1.10  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.3  2004/10/07 04:37:16  d3x0r
// Okay palette and listbox seem to nearly work... controls draw, now about that mouse... looks like my prior way of cheating is harder to step away from than I thought.
//
// Revision 1.2  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.9  2003/04/02 07:31:25  panther
// Enable local GetMyInterface which will auto configure library to current link
//
// Revision 1.8  2003/03/29 15:52:17  panther
// Fix service PSI compilation (direct link to render/image).  Just for grins make caption text 3dlike
//
// Revision 1.7  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
