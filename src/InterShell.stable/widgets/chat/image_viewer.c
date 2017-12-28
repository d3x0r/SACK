#ifndef FORCE_NO_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#endif
#define USES_INTERSHELL_INTERFACE
#include <stdhdrs.h>
#include <controls.h>
#include <psi.h>
#include <sqlgetoption.h>
#include <psi/console.h> // text formatter
#include <imglib/fontstruct.h>
#include <translation.h>
#include "../include/buttons.h"

#include "../../intershell_registry.h"
#include "../../intershell_export.h"

#define CHAT_CONTROL_SOURCE
#include "chat_control.h"
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
	uint32_t b;
	int done;
	PSI_CONTROL parent;
	PSI_CONTROL self;
	PRENDERER popup_renderer;
	void (CPROC*PopupEvent)( uintptr_t,LOGICAL );
	uintptr_t psvPopup;
	void (CPROC*OnAutoClose)( uintptr_t );
	uintptr_t psvAutoClose;
};

EasyRegisterControlWithBorder( CONTROL_NAME, sizeof( ImageViewer ), BORDER_CAPTION_CLOSE_IS_DONE|BORDER_CAPTION_CLOSE_BUTTON|BORDER_FIXED|BORDER_NORMAL|BORDER_RESIZABLE );

static int OnCreateCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	ImageViewer *pViewer = ControlData( ImageViewer *, pc );
	if( !l.image_help_font )
	{
		TEXTCHAR buf[256];
		int w, h;
		SACK_GetProfileString( "widgets/Image Viewer", "Help Font Name", "arial", buf, 256 );
		w = SACK_GetProfileInt( "widgets/Image Viewer", "Help Font Width", 14 );
		h = SACK_GetProfileInt( "widgets/Image Viewer", "Help Font Height", 12 );
		l.image_help_font = RenderFontFile( buf, w, h, FONT_FLAG_8BIT );
		l.image_grid_background_color = AColor( 32, 32, 32, 255 );
		l.image_grid_color = AColor( 48, 48, 48, 255 );
		l.help_text_background = AColor( 0, 0, 0, 64 );
		l.help_text_color = BASE_COLOR_WHITE;
	}
	SetControlText( pc, TranslateText( "Image Viewer" )/* (Scroll Wheel Zooms; Click&Drag Pan)" )*/ );
	pViewer->zoom = 1000;
	//AddCaptionButton( pc, NULL, NULL, 0, NULL );
	return 1;
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
	if(  pViewer->image )
	{
		if( pViewer->xofs + ( pViewer->image->width * pViewer->zoom ) < ((surface->width/2)*1000) )
			pViewer->xofs = ( surface->width / 2 ) * 1000 - pViewer->image->width * pViewer->zoom;
		if( pViewer->yofs + ( pViewer->image->height * pViewer->zoom ) < ((surface->height/2)*1000) )
			pViewer->yofs = ( surface->height / 2 ) * 1000 - pViewer->image->height * pViewer->zoom;
	}

	ClearImageTo( surface, l.image_grid_background_color );
	{
		int n;
		int c = l.image_grid_color;
		for( n = 0; n < surface->width; n += 20 )
			do_vline( surface, n, 0, surface->height, c );
		for( n = 0; n < surface->height; n += 20 )
			do_hline( surface, n, 0, surface->width, c );
	}
	if(  pViewer->image )
	{
		BlotScaledImageSizedEx( surface, pViewer->image
			, ( pViewer->xofs ) / 1000
			, ( pViewer->yofs ) / 1000
			, ( pViewer->image->width * pViewer->zoom ) / 1000
			, ( pViewer->image->height * pViewer->zoom ) / 1000
			, 0, 0
			, pViewer->image->width
			, pViewer->image->height
			, ALPHA_TRANSPARENT
			, BLOT_COPY
			);
		{
			CTEXTSTR message = TranslateText( "Escape to Exit\nScroll Wheel Zooms\nClick&Drag Pan" );
			uint32_t w, h;
			GetStringSizeFont( message, &w, &h, l.image_help_font );
			BlatColorAlpha( surface, 0, 0, w + 15, h + 6, l.help_text_background );
			PutStringFont( surface, 5, 3, l.help_text_color, 0
						, message
						, l.image_help_font );
		}
	}
	return 1;
}

#if !defined( NO_TOUCH )
static int OnTouchCommon( CONTROL_NAME )( PSI_CONTROL pc, PINPUT_POINT pPoints,int nPoints )
{

	return 1;
}
#endif

static int OnMouseCommon( CONTROL_NAME )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
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

		pViewer->zoom = pViewer->zoom * 4 / 3;
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

		pViewer->zoom = pViewer->zoom * 3 / 4;
		if( pViewer->zoom == 0 )
			pViewer->zoom = 1;

		pViewer->xofs = ( x * 1000 / pViewer->zoom - here_x ) * pViewer->zoom;
		pViewer->yofs = ( y * 1000 / pViewer->zoom - here_y ) * pViewer->zoom;
		SmudgeCommon( pc );
	}

	pViewer->b = b;
	return 1;
}

static int OnKeyCommon( CONTROL_NAME )( PSI_CONTROL pc, uint32_t key )
{
	if( IsKeyPressed( key ) )
	{
		if( KEY_CODE( key ) == KEY_ESCAPE )
		{
			ImageViewer *pViewer = ControlData( ImageViewer *, pc );
			if( pViewer )
			{
				SetCommonFocus( pViewer->parent );
				if( IsControlHidden( GetFrame( pViewer->parent ) ) )
					RevealCommon( GetFrame( pViewer->parent ) ); 
			}
			{
				void (CPROC*PopupEvent)( uintptr_t,LOGICAL );
				uintptr_t psvPopup;
				PopupEvent = pViewer->PopupEvent;
				psvPopup = pViewer->psvPopup;
				DestroyFrame( &pc );
				if( PopupEvent )
					PopupEvent( psvPopup, FALSE );
			}
			return 1;
		}
	}
	return 0;
}

static void CPROC DoneButton( uintptr_t psv, PCOMMON pc )
{
	PSI_CONTROL pc_frame = (PSI_CONTROL)GetParentControl( pc );
	ImageViewer *pViewer = ControlData( ImageViewer *, pc_frame );
	void (CPROC*PopupEvent)( uintptr_t,LOGICAL );
	uintptr_t psvPopup;
	PopupEvent = pViewer->PopupEvent;
	psvPopup = pViewer->psvPopup;
	DestroyFrame( &pc_frame );
	if( PopupEvent )
		PopupEvent( psvPopup, FALSE );
}

static void CPROC HandleLoseFocus( uintptr_t dwUser, PRENDERER pGain )
{
	ImageViewer *pViewer = ( ImageViewer *)dwUser;
	//lprintf( "combobox - HandleLoseFocus %p is gaining (we're losing) else we're gaining", pGain );
	if( pGain && pGain != pViewer->popup_renderer )
	{
		void (CPROC*Event)( uintptr_t ) = pViewer->OnAutoClose;
		uintptr_t psvEvent = pViewer->psvAutoClose;
		void (CPROC*PopupEvent)( uintptr_t,LOGICAL );
		uintptr_t psvPopup;
		PopupEvent = pViewer->PopupEvent;
		psvPopup = pViewer->psvPopup;
		DestroyControl( pViewer->self );
		if( PopupEvent )
			PopupEvent( psvPopup, FALSE );
		if( Event )
			Event( psvEvent );
	}
}

void ImageViewer_SetAutoCloseHandler( PSI_CONTROL pc, void (CPROC*Event)( uintptr_t ), uintptr_t psvEvent )
{
	ImageViewer *pViewer;
	pViewer = ControlData( ImageViewer *, pc );
	pViewer->OnAutoClose = Event;
	pViewer->psvAutoClose = psvEvent;
}


PSI_CONTROL ImageViewer_ShowImage( PSI_CONTROL parent, Image image
								  , void (CPROC*PopupEvent)( uintptr_t,LOGICAL ), uintptr_t psvEvent 
								  )
{
	PSI_CONTROL pc;
	uint32_t zoom;
	ImageViewer *pViewer;
	int x = 10;
	int y = 10;
	uint32_t w, h;
	uint32_t show_w, show_h;
	GetDisplaySize( &w, &h );
	if( SUS_LT( image->width, int, w * 3 / 4, uint32_t ) )
	{
		show_w = image->width;
		if( SUS_LT( image->height, int, h * 3 / 4, uint32_t ) )
		{
			show_h = image->height;
		}
		else
		{
			show_h = h * 3 / 4;
			show_w = show_h * image->width / image->height;
		}
	}
	else
	{
		show_w = w * 3 / 4;
		show_h = show_w * image->height / image->width;
		if( show_h < ( ( h * 3 ) / 4 ) )
		{
			//show_h = image->height;
		}
		else
		{
			show_h = ( h * 3 ) / 4;
			show_w = show_h * image->width / image->height;
		}
	}
	if( show_w < 250 )
	{
		show_h = ( show_h * 250 ) / show_w;
		show_w = 250;

	}
	if( show_h < 250 )
	{
		show_w = ( show_w * 250 ) / show_h;
		show_h = 250;
	}
	zoom = 1000 * show_w / image->width;

	show_w = ( image->width * zoom ) / 1000;
	show_h = ( image->height * zoom ) / 1000;

	x = (w - show_w ) / 2;
	y = ( h - show_h ) / 2;
	pc = MakeNamedControl( NULL, CONTROL_NAME, x, y, show_w, show_h, -1 );
	pViewer = ControlData( ImageViewer *, pc );
	{
		PSI_CONTROL done_button;
		done_button = MakeButton( pc, -10, -10, 1, 1, IDCANCEL, WIDE(""), 0, 0, 0  );
		SetButtonPushMethod( done_button, DoneButton, 0 );
	}
	//AddCommonButtons( pc, &pViewer->done, NULL );
	//HideCommon( GetControl( pc, IDCANCEL ) );
	pViewer->PopupEvent = PopupEvent;
	pViewer->psvPopup = psvEvent;
	pViewer->zoom = zoom;
	pViewer->image = image;
	pViewer->parent = parent;
	pViewer->self = pc;
	//DisplayFrameOver( pc, parent );
	if( PopupEvent )
		PopupEvent( psvEvent, TRUE );
	DisplayFrame( pc );
	{
		pViewer->popup_renderer = GetFrameRenderer( pc );
		SetLoseFocusHandler( pViewer->popup_renderer, HandleLoseFocus, (uintptr_t)pViewer );
	}
	return pc;
}