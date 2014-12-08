#ifndef FORCE_NO_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#endif
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define CHAT_CONTROL_MAIN_SOURCE
#include <stdhdrs.h>
#include <controls.h>
#include <psi.h>
#include <sqlgetoption.h>
#include <psi/console.h> // text formatter

#include "../include/buttons.h"

#include "../../intershell_registry.h"
#include "../../intershell_export.h"

#include "chat_control_internal.h"   // l defintiion

#define CONTROL_NAME WIDE("Image Viewer Popup")

#define INTERSHELL_CONTROL_NAME WIDE("Intershell/test/Image Viewer")

//

typedef struct image_viewer ImageViewer;
struct image_viewer {
	Image image;
	int zoom; // scaled integer *1000 ... 1 = zoomed out 1000 = 1:1, 10000 = 10x magnification
	int xofs, yofs; // 1 = 1000; scaled coords...
	int x_click, y_click;
	int rotation;
	_32 b;
};

EasyRegisterControlWithBorder( CONTROL_NAME, sizeof( ImageViewer ), BORDER_CAPTION_CLOSE_BUTTON|BORDER_FIXED|BORDER_NORMAL|BORDER_RESIZABLE );

static int OnCreateCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	ImageViewer *pViewer = ControlData( ImageViewer *, pc );
	SetCommonText( pc, "Image Viewer" );
	pViewer->zoom = 1000;
}

static int OnDrawCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	ImageViewer *pViewer = ControlData( ImageViewer *, pc );
	Image surface = GetControlSurface( pc );

	// sanity clipping... could be more conservative.
	if( (pViewer->xofs / 1000) > (surface->width/2) )
		pViewer->xofs = ( surface->width / 2 ) * 1000;
	if( (pViewer->yofs / 1000) > (surface->height / 2) )
		pViewer->yofs = ( surface->height / 2 ) * 1000;
	if( pViewer->xofs + ( pViewer->image->width * pViewer->zoom ) < ((surface->width/2)*1000) )
		pViewer->xofs = ( surface->width / 2 ) * 1000 - pViewer->image->width * pViewer->zoom;
	if( pViewer->yofs + ( pViewer->image->height * pViewer->zoom ) < ((surface->height/2)*1000) )
		pViewer->yofs = ( surface->height / 2 ) * 1000 - pViewer->image->height * pViewer->zoom;


	ClearImageTo( surface, BASE_COLOR_BLACK );
	BlotScaledImageSized( surface, pViewer->image
		, ( pViewer->xofs ) / 1000
		, ( pViewer->yofs ) / 1000
		, ( pViewer->image->width * pViewer->zoom ) / 1000
		, ( pViewer->image->height * pViewer->zoom ) / 1000
		, 0, 0
		, pViewer->image->width
		, pViewer->image->height
		);
}

#if !defined( __WATCOMC__ ) && !defined( MINGW_SUX )
static int OnTouchCommon( CONTROL_NAME )( PSI_CONTROL pc, PINPUT_POINT pPoints,int nPoints )
{

	return 1;
}
#endif

static int OnMouseCommon( CONTROL_NAME )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	ImageViewer *pViewer = ControlData( ImageViewer *, pc );
	if( ( b & MK_LBUTTON ) && (!( pViewer->b & MK_LBUTTON ) ) ) // first-down
	{
		pViewer->x_click = x;
		pViewer->y_click = y;
	}
	else if( !( b & MK_LBUTTON ) && ( pViewer->b & MK_LBUTTON ) ) // last-up
	{
	}
	else if( ( b & MK_LBUTTON ) ) // still down
	{
		int delx = ( x - pViewer->x_click ) * 1000;
		int dely = ( y - pViewer->y_click ) * 1000;
		if( delx )
		{
			pViewer->xofs +=delx;
			pViewer->x_click = x;
		}
		if( dely ) 
		{
			pViewer->yofs += dely;
			pViewer->y_click = y;
		}
		SmudgeCommon( pc );
	}

	if( b & MK_SCROLL_UP ) 
	{
      // zoom = 2000  ... real_x = 2 * x  (zoom*x)/1000
		// zoom = 1000
		// zoom = 500

		int here_x = (  ( x *1000 / pViewer->zoom ) * 1000 - (pViewer->xofs/ pViewer->zoom ) * 1000 ) / 1000;
		int here_y = (  ( y *1000 / pViewer->zoom ) * 1000 - (pViewer->yofs/ pViewer->zoom ) * 1000 ) / 1000;

		pViewer->zoom *= 2;
		if( pViewer->zoom > 1000000 )	
			pViewer->zoom = 1000000;
		
		pViewer->xofs = ( x * 1000 / pViewer->zoom - here_x ) * pViewer->zoom;
		pViewer->yofs = ( y * 1000 / pViewer->zoom - here_y ) * pViewer->zoom;
		SmudgeCommon( pc );
	}
	if( b & MK_SCROLL_DOWN ) 
	{
		int here_x = (  ( x *1000 / pViewer->zoom ) * 1000 - (pViewer->xofs/ pViewer->zoom ) * 1000 ) / 1000;
		int here_y = (  ( y *1000 / pViewer->zoom ) * 1000 - (pViewer->yofs/ pViewer->zoom ) * 1000 ) / 1000;

		pViewer->zoom /= 2;
		if( pViewer->zoom == 0 )
			pViewer->zoom = 1;

		pViewer->xofs = ( x * 1000 / pViewer->zoom - here_x ) * pViewer->zoom;
		pViewer->yofs = ( y * 1000 / pViewer->zoom - here_y ) * pViewer->zoom;
		SmudgeCommon( pc );
	}

	pViewer->b = b;
}

static int OnKeyCommon( CONTROL_NAME )( PSI_CONTROL pc, _32 key )
{
	if( IsKeyPressed( key ) )
	{
		if( KEY_CODE( key ) == KEY_ESCAPE )
		{
			DestroyFrame( &pc );
			return 1;
		}
	}
	return 0;
}

PSI_CONTROL ImageViewer_ShowImage( Image image )
{
	PSI_CONTROL pc;
	ImageViewer *pViewer;
	int x = 10;
	int y = 10;
	pc = MakeNamedControl( NULL, CONTROL_NAME, x, y, image->width, image->height, -1 );
	pViewer = ControlData( ImageViewer *, pc );
	pViewer->image = image;
	DisplayFrame( pc );
	
}