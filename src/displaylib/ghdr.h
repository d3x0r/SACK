
#ifdef __ARM__
#define __NO_SDL__
#endif

#  define USE_IMAGE_INTERFACE g.ImageInterface
//#  define USE_RENDER_INTERFACE g.RenderInterface
//typedef struct panel_tag PANEL, *PPANEL;


#if !defined( __ARM__ ) && !defined( __arm__ ) && !defined( __NO_SDL__ ) && !defined( WIN32 )
#  define __SDL__
//#  define PRENDERER PPANEL
#  include <SDL/SDL.h>
#else
#  ifndef __NO_SDL__
#     define __NO_SDL__
#  endif
#  ifdef __LINUX__
#     include <linux/fb.h>
#     define __RAW_FRAMEBUFFER__
#     define ALLOW_DIRECT_UPDATE
#     include <msgclient.h>
#  elif defined( WIN32 )
// under windows we fake using VIDLIB for the render services...
//#    define PRENDERER PPANEL
#    include <vidlib/vidstruc.h>
#    include <render.h>
#    include <msgclient.h>
#  endif
#endif


#include <sack_types.h>
#include <logging.h>
#include <timers.h>

//#include <imglib/imagestruct.h>
//#ifdef _WIN32
//#endif
#include <imglib/imagestruct.h>
#include <image.h>

#include <render.h>
#include "displaystruc.h"
#include <keybrd.h>

#include "spacetree.h"

#include <controls.h>

