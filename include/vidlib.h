#ifndef VIDLIB2_INCLUDED
#define VIDLIB2_INCLUDED

#include "keybrd.h" // required for keyboard support ever....
#include <sack_types.h>
#include <render.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define WM_RUALIVE 5000 // lparam = pointer to alive variable expected to set true

#ifdef VIDEO_LIBRARY_SOURCE 
#define VIDEO_PROC EXPORT_METHOD
#else
#define VIDEO_PROC IMPORT_METHOD
#endif

#ifdef __cplusplus
#define VIDLIB_NAMESPACE RENDER_NAMESPACE namespace vidlib {
#define VIDLIB_NAMESPACE_END } RENDER_NAMESPACE_END
#else
#define VIDLIB_NAMESPACE
#define VIDLIB_NAMESPACE_END
#endif

VIDLIB_NAMESPACE

#define WD_HVIDEO   0   // WindowData_HVIDEO

// in case "VideoOutputClass" was used as a control in a dialog...
#define GetVideoHandle( hWndDialog, nControl ) ((HVIDEO)(GetWindowLong( GetDlgItem( hWndDialog, nControl ), 0 )))

RENDER_PROC( PKEYBOARD, GetDisplayKeyboard )( PRENDERER hVideo );

#ifndef _WIN32
#define HICON int
#endif

VIDLIB_NAMESPACE_END
# ifdef __cplusplus
	using namespace sack::image::render;
#ifdef _D3D_DRIVER
using namespace sack::image::render::d3d::vidlib;
#elif defined( _D3D10_DRIVER )
using namespace sack::image::render::d3d10::vidlib;
#else
using namespace sack::image::render::vidlib;
#endif
# endif


#endif

