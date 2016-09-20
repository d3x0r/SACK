// this is ugly , but it works, please consider
// a library init that will grab this...
#ifndef FORCE_NO_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pri
#endif
// don't use multi-layer buttons... there's issues with them leaving lense artifacts when parent window hides...
//#define MULTI_LAYER_BUTTONS

//GetImageInterface()
#include <stdhdrs.h>
#include <controls.h>
#include <psi.h>
#include <sqlgetoption.h>
#define BUTTON_KEY_STRUCTURE_DEFINED

#include "../include/buttons.h"

#include "../../intershell_registry.h"



#ifdef __cplusplus
namespace sack {
	namespace widgets {
		namespace buttons {
#endif

#define BUTTON_NAME WIDE("fancy button 2")

typedef struct text_placement_tag
{
	DeclareLink( struct text_placement_tag );
	struct {
		uint32_t bHorizCenter : 1;
		uint32_t bDrawRightJust : 1;
	} flags;
	int x, y;
	int orig_x, orig_y;
	// also causes a retrigger of reposition....
	int prior_width, prior_height;
	// keep the reference of the application's font
	// so that we don't have to do anything but redraw to
	// get a new font on a tag.
	SFTFont *font;
	SFTFont last_font; // last time we looked at (*font) this is what it was.
	CDATA text;
	TEXTCHAR *content;
}TEXT_PLACEMENT;

typedef struct color_triplet {
	CDATA r,g,b;
} TRIPLET, *PTRIPLET;

typedef struct key_button_tag
{
	PSI_CONTROL button;
	int32_t x, y;
	uint32_t width, height;  // scales images to this size...
	int32_t real_x, real_y; // physical device coordinates to track the layers above.
	PRENDERER lense_layer;
	PRENDERER animation_layer;
	struct {
		BIT_FIELD background_image : 1;
		BIT_FIELD bGreyed : 1;
		BIT_FIELD bLenseUnderText : 1;
		BIT_FIELD bRidgeUnderText : 1;
		BIT_FIELD bNoPress : 1;
		BIT_FIELD bMultiShade : 1;
		BIT_FIELD bShade : 1;
		BIT_FIELD background_by_name : 1; // background was set by name of file.
		BIT_FIELD bHighlight : 1;
		BIT_FIELD bShown : 1;
		BIT_FIELD bOrdered : 1; // had a chance to apply renderer ordering
		BIT_FIELD bLayered : 1;
		BIT_FIELD bCreating : 1;
		BIT_FIELD bThemeSetExternal : 1;
	}flags;

   // extended highlight generalized...

	Image lense;
	int lense_alpha;
	Image frame_up;
	Image frame_down;
	Image mask;
	struct {
		CDATA r,g,b;
		CDATA r2,g2,b2; // highlight color.
		PDATALIST layer2_colors;
		int layer2_color;
	} multi_shade;
	struct {
		Image image;
		uint32_t hMargin, vMargin;
		CDATA color;
		int16_t alpha;
	}background;
	TEXTCHAR *content; // nul terminated strings concatenated...
	//SFTFont font;
	CDATA text_color;
	void (CPROC *PressHandler)( uintptr_t psv, PKEY_BUTTON key );
	void (CPROC *SimplePressHandler)( uintptr_t psv );
	uintptr_t psvPress;
	CTEXTSTR value;
	PTEXT_PLACEMENT layout;

	struct {
		uint32_t pressed:1;
	}buttonflags;
   int _b;
} KEY_BUTTON;

#define NUM_COLORS sizeof( color_defs ) / sizeof( color_defs[0] )
struct {
	CDATA color;
	CTEXTSTR name;
}color_defs[4];
#if 0
= { { BASE_COLOR_BLACK, WIDE("black") }
					 , { BASE_COLOR_BLUE, WIDE("blue") }
					 , { BASE_COLOR_GREEN, WIDE("green") }
					 , { BASE_COLOR_RED, WIDE("red") }
};
#endif

typedef struct theme_tag {

	struct {
		Image iMask;		
		Image iGlare;		
		Image iNormal;		
		Image iPressed;		
		CDATA color;		
		CDATA text_color;
		int style; // multi/mono shade...		
	} buttons;
	CTEXTSTR name;
	PLIST _theme_list;
	PLIST *theme_list;
} THEME, *PTHEME;

typedef struct widget_button_local_tag {
	struct {
		uint32_t theme_loaded : 1;
	} flags;
	PTHEME default_theme;
	PLIST theme_name_list;
	PLIST theme_list;
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pri;
    PLIST buttons; // PKEY_BUTTON list
} LOCAL, *PLOCAL;

static LOCAL l;


 // since the fancy_button is actually a custom-drawn button
 // with methods overridden to handle default mouse behavior, etc
// have to figure out a way to define subclasses...
EasyRegisterControlWithBorder( BUTTON_NAME, sizeof( KEY_BUTTON ), BORDER_NONE );

void CPROC PressKeyButton( PKEY_BUTTON key, PCONTROL pc )
{
	// not really sure I need to do anything here...
	if( !key->flags.bNoPress && key->PressHandler )
	{
		//ThreadTo( (uintptr_t(CPROC*)(PTHREAD))PressKeyThread, (uintptr_t)key );
		//ReleaseCommonUse( pc );
		if( key->PressHandler )
			key->PressHandler( key->psvPress, key );
		else if( key->SimplePressHandler )
			key->SimplePressHandler( key->psvPress );
	}
	//  logging to figure out why a button doesn't press
	//   turns out that bNoPress was never set.
	//else
	//{
	//	lprintf( WIDE("Key is %s or %s"), key->flags.bNoPress?"NoPress":"(ok-pressable)",
	//			  key->PressHandler?"(ok-Has Handler)":"Has no handler" );
	//}
}

void CPROC DrawGlareLayer( uintptr_t psv_control, PRENDERER renderer );


static int CPROC HandleMouse( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	Image image = GetControlSurface( pc );
	MyValidatedControlData( PKEY_BUTTON, button, pc );
	//lprintf( "Mouse Event %d %d %d", x, y, b );
	if( b == -1 )
	{
		if( button->buttonflags.pressed )
		{
			//lprintf( WIDE("releaseing press state sorta...") );
			button->buttonflags.pressed = FALSE;
			SmudgeCommon( pc );
		}
		button->_b = 0;
		return 1;
	}
	if( x < 0
		 || y < 0
		 || x > image->width
		 || y > image->height )
	{
		if( button->buttonflags.pressed )
		{
			//lprintf( WIDE("Releasing button.") );
			button->buttonflags.pressed = FALSE;
			if( button->flags.bLayered )
			{
				DrawGlareLayer( (uintptr_t)pc, button->lense_layer );
				Redraw( button->lense_layer );
			}
			else
				SmudgeCommon( pc );
		}
		button->_b = 0; // pretend no mouse buttons..
		return 1;
	}
	if( b & MK_LBUTTON )
	{
		if( !(button->_b & MK_LBUTTON ) )
		{
			if( !button->buttonflags.pressed )
			{
				button->buttonflags.pressed = TRUE;
				//lprintf( "Draw pressed state?" );
				if( button->flags.bLayered )
				{
					DrawGlareLayer( (uintptr_t)pc, button->lense_layer );
					Redraw( button->lense_layer );
				}
				else
					SmudgeCommon( pc );
			}
			//if( l.flags.bTouchDisplay )
			//	InvokeButton( pc );
		}
	}
	else
	{
		if( ( button->_b != INVALID_INDEX ) && (button->_b & MK_LBUTTON ) )
		{
			button->_b = b;
			if( button->buttonflags.pressed )
			{
				button->buttonflags.pressed = FALSE;

			}
			//if( !l.flags.bTouchDisplay )
			PressKeyButton( button, pc );
			if( !button->buttonflags.pressed )
			{
				if( button->flags.bLayered )
				{
					DrawGlareLayer( (uintptr_t)pc, button->lense_layer );
					Redraw( button->lense_layer );
				}
				else
					SmudgeCommon( pc );
			}
		}
	}
	button->_b = b;

   return 1;
}

static void OnMoveCommon( BUTTON_NAME )( PSI_CONTROL pc, LOGICAL bMoving )
{
	if( !bMoving )
	{
		MyValidatedControlData( PKEY_BUTTON, button, pc );
		int32_t x = 0, y = 0;

		GetPhysicalCoordinate( pc, &x, &y, FALSE );
		button->real_x = x;
		button->real_y = y;
		if( button->flags.bLayered )
		{
			if( button->animation_layer )
				MoveDisplay( button->animation_layer, x, y );
			if( button->lense_layer )
			{
				MoveDisplay( button->lense_layer, x, y );
				//UpdateDisplay( button->lense_layer );
			}
		}
	}
}

static void OnSizeCommon( BUTTON_NAME )( PSI_CONTROL pc, LOGICAL bSizing )
{
	if( !bSizing )
	{
		MyValidatedControlData( PKEY_BUTTON, button, pc );
		if( button && button->flags.bLayered )
		{
			Image surface = GetControlSurface( pc );
			if( button->animation_layer )
				SizeDisplay( button->animation_layer
							  , surface->width, surface->height );
			if( button->lense_layer )
			{
				SizeDisplay( button->lense_layer
							  , surface->width, surface->height );
				UpdateDisplay( button->lense_layer );
			}
		}
	}
}

static int  OnMouseCommon(BUTTON_NAME)( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
   return HandleMouse( pc, x, y, b );
}

void CPROC DrawGlareLayer( uintptr_t psv_control, PRENDERER renderer )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv_control;
	MyValidatedControlData( PKEY_BUTTON, key, pc );
	if( key )
	{
		Image glare_surface = GetDisplayImage( key->lense_layer );
		//lprintf( "drawing the glare layer.." );
		ClearImageTo( glare_surface, 0 );
			{
				if( key->lense )
				{
					//lprintf( "Output glare surface." );
					BlotScaledImageSizedToAlpha( glare_surface, key->lense
														, 0, 0
														, key->width, key->height
														, ALPHA_TRANSPARENT );
				}
			}
			{
				if( !key->flags.bNoPress && ( key->buttonflags.pressed ) )
				{
					if( key->frame_down )
					{
						//lprintf( "Output glare surface." );
						BlotScaledImageSizedToAlpha( glare_surface, key->frame_down
															, 0, 0
															, key->width, key->height
															 //, ALPHA_TRANSPARENT_INVERT + 170
															, ALPHA_TRANSPARENT
															);
					}
				}
				else
				{
					if( key->frame_up )
					{
						//lprintf( "Output glare surface." );
						BlotScaledImageSizedToAlpha( glare_surface, key->frame_up
															, 0, 0
															, key->width, key->height
															 //, ALPHA_TRANSPARENT_INVERT + 170
															, ALPHA_TRANSPARENT
															);
					}
				}
			}

	}
	UpdateDisplay( renderer );
}
//------------------------------------------------------------------------------------

static int OnCreateCommon(BUTTON_NAME)( PCOMMON pc )
{
	MyValidatedControlData( PKEY_BUTTON, button, pc );
	if( button )
	{
		Image surface = GetControlSurface( pc );
		PRENDERER r = GetFrameRenderer( GetFrame( pc ) );
		int32_t x = 0;
		int32_t y = 0;
		GetPhysicalCoordinate( pc, &x, &y, FALSE );
		HideControl( pc ); // hide this control - we'll show it when we finish the proper init...
		button->real_x = x;
		button->real_y = y;
		button->button = pc;
		if( r )
			button->flags.bOrdered = 1;
#ifdef MULTI_LAYER_BUTTONS
		button->flags.bLayered = 1;
#endif
		if( button->flags.bLayered )
		{
			button->lense_layer = OpenDisplayAboveSizedAt( DISPLAY_ATTRIBUTE_LAYERED
																		 |DISPLAY_ATTRIBUTE_CHILD // mark that this is uhmm intended to not be a alt-tabbable window
																		, surface->width
																		, surface->height
																		, x, y
																		, r // r may not exist yet... we might just be over a control that is frameless... later we'll relate as child
																		);
			SetRedrawHandler( button->lense_layer, DrawGlareLayer, (uintptr_t)pc );
			SetMouseHandler( button->lense_layer, (MouseCallback)HandleMouse, (uintptr_t) pc );
		}
	}
	return 1;
}

static void OnHideCommon( BUTTON_NAME )( PSI_CONTROL pc )
{
	MyValidatedControlData( PKEY_BUTTON, button, pc );
	if( button->flags.bLayered )
	{
		HideDisplay( button->lense_layer );
	}
	button->flags.bShown = 0;
}

static void OnRevealCommon( BUTTON_NAME )( PSI_CONTROL pc )
{
	MyValidatedControlData( PKEY_BUTTON, button, pc );

	PRENDERER renderer = GetFrameRenderer( GetFrame( pc ) );
	if( !renderer )
	{
      //DebugBreak();
      //lprintf( "renderer doesn't exist..." );
		return;
	}
	if( button->flags.bLayered )
	{
		if( button->animation_layer )
		{
			PutDisplayAbove( button->animation_layer, GetFrameRenderer( GetFrame( pc ) ) );
			if( button->flags.bLayered )
				PutDisplayAbove( button->lense_layer, button->animation_layer );
		}
		else
		{
			if( button->flags.bLayered )
				PutDisplayAbove( button->lense_layer, GetFrameRenderer( GetFrame( pc ) ) );
		}
	}
	// could just wait for the draw to happen after the reveal notification
   // which will at that time put the display in the correct place.


	//SmudgeCommon( pc );
	//lprintf( "Update lense layer..." );
	//UpdateDisplay( button->lense_layer );
	//button->flags.bShown = 1;
	//lprintf( "Update lense layer..." );

}


void CPROC PressKeyThread( PTHREAD thread )
{
	PKEY_BUTTON key = (PKEY_BUTTON)GetThreadParam( thread );
	if( key->PressHandler )
		key->PressHandler( key->psvPress, key );
	else if( key->SimplePressHandler )
		key->SimplePressHandler( key->psvPress );
	// and then we exit gracefully...
	// this is sick! :)
}

CTEXTSTR GetKeyValue( PKEY_BUTTON pKey )
{
   return pKey->value;
}


PTHEME LoadButtonThemeByNameEx( CTEXTSTR name, int theme_id, CDATA default_color, CDATA default_text_color )
{
	size_t retval;
	TEXTCHAR szTheme[256];
	TEXTCHAR szBuffer[256];
   TEXTCHAR szThemeId[12];
	struct theme_tag *theme;
	static const CTEXTSTR gIniFileName = WIDE( "theme.ini" );
	PTHEME base_theme;
   INDEX idx;
	// Allocate memory for structure

	if( !l.pii )
		l.pii = GetImageInterface();
	if( !l.pri )
		l.pri = GetDisplayInterface();

	theme = New( THEME );

	theme->name = StrDup( name );

	LIST_FORALL( l.theme_name_list, idx, PTHEME, base_theme )
	{
		if( StrCaseCmp( base_theme->name, theme->name ) == 0 )
		{
         break;
		}
	}
	if( base_theme )
		theme->theme_list = &base_theme->_theme_list;
	else
	{
		theme->_theme_list = NULL;
		theme->theme_list = &theme->_theme_list;
      AddLink( &l.theme_name_list, theme );
	}


   snprintf( szThemeId, 12, WIDE("%d"), theme_id );
	// Create Root Path for reading edit options ini
	snprintf( szBuffer, 256, WIDE( "THEME/%s%s%s" ), name, theme_id?WIDE("."):WIDE(""), theme_id?szThemeId:WIDE("") );
	
	// Copy path
	memcpy( szTheme, szBuffer, ( sizeof (szTheme) ) );

	// Get Mask 
	retval =  SACK_GetPrivateProfileStringEx( szTheme, WIDE( "MASK" ), WIDE( "ridge_mask.png" ), szBuffer, sizeof( szBuffer ), gIniFileName, TRUE );

	// Load Mask
	if ( StrCmpEx( szBuffer, WIDE( "NULL" ), 4 ) )
	{
		theme->buttons.iMask = LoadImageFileFromGroup( GetFileGroup( WIDE("Button Resources"), WIDE("%Resources%/images") ), szBuffer );
		xlprintf(LOG_NOISE+1)( WIDE(" LoadImageFile returned a pointer to %p ") , theme->buttons.iMask );
	}

	else
	{		
		theme->buttons.iMask = NULL;
	}
	
	// Get Glare
	retval =  SACK_GetPrivateProfileStringEx( szTheme, WIDE( "GLARE" ), WIDE( "glare.png" ), szBuffer, sizeof( szBuffer ), gIniFileName, TRUE );	
	
	// Load Glare
	if (StrCmpEx(szBuffer, WIDE("NULL"), 4)  )
	{
		theme->buttons.iGlare = LoadImageFileFromGroup( GetFileGroup( WIDE("Button Resources"), WIDE("%Resources%/images") ), szBuffer );
		xlprintf(LOG_NOISE+1)( WIDE(" LoadImageFile returned a pointer to %p ") , theme->buttons.iGlare );
	}

	else
	{
		xlprintf(LOG_NOISE+1)( WIDE("setting an image file to null") );
		theme->buttons.iGlare = NULL;
	}

	// Get Pressed
	retval =  SACK_GetPrivateProfileStringEx( szTheme, WIDE( "PRESSED" ), WIDE( "ridge_down.png" ), szBuffer, sizeof( szBuffer ), gIniFileName, TRUE );
	
	// Load Pressed
	if (StrCmpEx(szBuffer, WIDE("NULL"), 4)  )
	{
		theme->buttons.iPressed = LoadImageFileFromGroup( GetFileGroup( WIDE("Button Resources"), WIDE("%Resources%/images") ), szBuffer );
		xlprintf(LOG_NOISE+1)( WIDE(" LoadImageFile returned a pointer to %p ") , theme->buttons.iPressed );
	}

	else
	{
		xlprintf(LOG_NOISE+1)( WIDE("setting an image file to null") );
		theme->buttons.iPressed = NULL;
	}

	// Get Normal
	retval =  SACK_GetPrivateProfileStringEx( szTheme, WIDE( "NORMAL" ), WIDE( "ridge_up.png" ), szBuffer, sizeof( szBuffer ), gIniFileName, TRUE );

	// Load Normal
	if (StrCmpEx(szBuffer, WIDE("NULL"), 4)  )
	{
		theme->buttons.iNormal = LoadImageFileFromGroup( GetFileGroup( WIDE("Button Resources"), WIDE("%Resources%/images") ), szBuffer );
		xlprintf(LOG_NOISE+1)( WIDE(" LoadImageFile returned a pointer to %p ") , theme->buttons.iNormal );
	}

	else
	{
		xlprintf(LOG_NOISE+1)( WIDE("setting an image file to null") );
		theme->buttons.iNormal = NULL;
	}
	
	// Get color and style
	theme->buttons.color =  SACK_GetPrivateProfileIntEx( szTheme, WIDE( "Color" ), default_color?default_color:0x63F00013, gIniFileName, TRUE );
	theme->buttons.color =  SACK_GetPrivateProfileIntEx( szTheme, WIDE( "Color" ), default_text_color?default_text_color:0x63F0F0D3, gIniFileName, TRUE );
	theme->buttons.style =  SACK_GetPrivateProfileIntEx( szTheme, WIDE( "Style" ), 1, gIniFileName, TRUE );

   SetLink( theme->theme_list, theme_id, theme );

	return theme;
}

PTHEME LoadButtonThemeByName( CTEXTSTR name, int theme_id )
{
   return LoadButtonThemeByNameEx( name, theme_id, 0, 0 );
}


void LoadButtonTheme( void )
{
#ifndef __NO_OPTIONS__
	if( !l.flags.theme_loaded )
	{
		color_defs[0].color = BASE_COLOR_BLACK;
		color_defs[0].name = WIDE("black");
		color_defs[1].color = BASE_COLOR_BLUE;
		color_defs[1].name = WIDE("blue");
		color_defs[2].color = BASE_COLOR_GREEN;
		color_defs[2].name = WIDE("green");
		color_defs[3].color = BASE_COLOR_RED;
		color_defs[3].name = WIDE("red");		

		lprintf( "normal butto theme..." );
		{
			PTHEME theme = LoadButtonThemeByName( WIDE("Normal Button"), 0 );
			SetLink( &l.theme_list, 0, theme );
			l.default_theme = theme;
		}
		l.flags.theme_loaded = 1;
	}
#endif
	return;
}

//------------------------------------------------------------------------------------

void UpdateLayoutPosition( PKEY_BUTTON pKey, PTEXT_PLACEMENT layout )
{
	if( layout->font &&
		( ((*layout->font) != layout->last_font )
		 || ( layout->prior_width != pKey->width )
		 || ( layout->prior_height != pKey->height ) ) )
	{
		uint32_t w, h;
		layout->prior_width = pKey->width;
		layout->prior_height = pKey->height;
		if( layout->font )
			layout->last_font = (*layout->font);
		else
			layout->last_font = NULL;
		GetStringSizeFont( WIDE("M"), &w, &h, layout->font?(*layout->font):NULL );
		if( layout->orig_x < 0 )
			layout->x = pKey->width - ( (-layout->orig_x) * w ) / 10;
		else
			layout->x = ( layout->orig_x * w ) / 10;
		if( layout->orig_y < 0 )
			layout->y = pKey->height - ( (-layout->orig_y) * h ) / 10;
		else
			layout->y = ( layout->orig_y * h ) / 10;
		//lprintf( WIDE("output %s at %ld measured by %d,%d"), layout->content, layout->y, w, h );
	}
}

//------------------------------------------------------------------------------------

int CPROC DrawButtonText( PSI_CONTROL pc, Image surface, PKEY_BUTTON key )
{
	if( key->layout )
	{
		PTEXT_PLACEMENT layout;
		for( layout = key->layout; layout; layout = NextThing( layout ) )
		{
			uint32_t w, h;
			int32_t xofs = 0, yofs = 0;
			UpdateLayoutPosition( key, layout );
			if( layout->flags.bHorizCenter || layout->flags.bDrawRightJust)
			{
				GetStringSizeFont( layout->content, &w, &h, layout->last_font );
				if( layout->flags.bDrawRightJust )
				{
					xofs = -(signed)w;
				}
				if( layout->flags.bHorizCenter )
				{
					if( w >= key->width )
					{
						// string too wide... just drop it to the left...
						xofs = -layout->x;
					}
					else
					{
						xofs = ( ( key->width - w ) / 2 );
						xofs -= layout->x; // then rebiasing later corrects this ...
					}
				}
			}
			//lprintf( WIDE("output %s at %ld"), layout->content, layout->y );
			PutStringFont( surface
							 , layout->x + xofs, layout->y + yofs
							 , key->flags.bGreyed?BASE_COLOR_WHITE:layout->text
							 , 0
							 , layout->content
							 , layout->last_font );
		}
	}
	else
	{
		CTEXTSTR text;
		CDATA foreground = 0;
		//CDATA background = 0;
		text = key->content;

		while( text && text[0] )
		{
			while( text && text[0] && text[0] == '~' )
			{
				int code;
				text++;
				switch( code = text[0] )
				{
				case 'B':
				case 'b':
				case 'F':
				case 'f':
					text++;
					{
						int n;
						size_t len;
						for( n = 0; n < NUM_COLORS; n++ )
						{
							//lprintf( WIDE("text %s =%s?"), text
							//		 , color_defs[n].name );
							if( StrCaseCmpEx( text
											, color_defs[n].name
											, len = strlen( color_defs[n].name ) ) == 0 )
							{
								break;
							}
						}
						if( n < NUM_COLORS )
						{
							text += len;
							if( code == 'b' || code == 'B' )
							{
								if( key->background.color != color_defs[n].color )
								{
									key->background.color = color_defs[n].color;
                           return 0; // return failure... need to redo this...
								}
							}
							else if( code == 'f' || code == 'F' )
								foreground = color_defs[n].color;
						}
					}
					break;
				}
			}
			{
				int textx = 0, texty = 0, first = 1, lines = 0, line = 0;
				CTEXTSTR p;
				p = text;
				while( p && p[0] )
				{
					lines++;
					p += strlen( p ) + 1;
				}
				while( text && text[0] )
				{
					SFTFont font = GetCommonFont( key->button );
					size_t len = strlen( text );
					uint32_t text_width, text_height;
					// handle content formatting.
					text++;
					GetStringSizeFontEx( text, len-1, &text_width, &text_height, font );
					if( 0 )
						lprintf( WIDE("%d lines %d line  width %d  height %d"), lines, line, text_width, text_height );
					switch( text[-1] )
					{
					case 'A':
					case 'a':
						textx = ( key->width - text_width ) / 2;
						texty = ( ( key->height - ( text_height * lines ) ) / 2 ) + ( text_height * line );;
						break;
					case 'c':
						textx = ( key->width - text_width ) / 2;
						break;
					case 'C':
						textx = ( key->width - text_width ) / 2;
						if( !first )
							texty += text_height;
						break;
					default:
						break;
					}
					if( 0 )
						lprintf( WIDE("Finally string is %s at %d,%d max %d"), text, textx, texty, key->height );
					PutStringFontEx( surface
										, textx, texty
										, key->flags.bGreyed?BASE_COLOR_WHITE:key->text_color
										, 0
										, text
										, len
										, font );
					first = 0;
					line++;
					text += len; // next string if any...
				}
			}
		}
	}
	return 1;
}

//------------------------------------------------------------------------------------

void DrawButtonLayers( PKEY_BUTTON key, Image surface, PSI_CONTROL pc)
{
		int layer;
		int textlayer = 1;
		if( key->flags.bLenseUnderText )
			textlayer = 2;
		if( key->flags.bRidgeUnderText )
			textlayer = 3;
		for( layer = 0; ;layer++ )
		{
			if( layer == 0 )
			{
				{
					if( key->mask )
					{
                  //lprintf( "Has a mask..." );
						if( key->flags.bGreyed )
						{
                     //lprintf( "greyed" );
							BlotScaledImageSizedToShadedAlpha( surface, key->mask
																		, 0, 0
																		, key->width, key->height
																		, ALPHA_TRANSPARENT/*ALPHA_TRANSPARENT*/
																		, AColor( 1, 1, 1, 64 )
																		);
						}
						else
						{
							if( key->flags.bMultiShade )
							{
                        //lprintf( "Multishade" );
								if( key->multi_shade.layer2_color )
								{
									PTRIPLET color = (PTRIPLET)
										GetDataItem( &key->multi_shade.layer2_colors, key->multi_shade.layer2_color-1 );
									//lprintf( "color %08x %08x %08x", color->r, color->g, color->b );
									BlotScaledImageSizedToMultiShadedAlpha( surface, key->mask
																					  , 0, 0
																					  , key->width, key->height
																					  , ALPHA_TRANSPARENT
																					  , color->r
																					  , color->g
																					  , color->b
																					  );
								}
								else if( key->flags.bHighlight )
									BlotScaledImageSizedToMultiShadedAlpha( surface, key->mask
																					  , 0, 0
																					  , key->width, key->height
																					  , ALPHA_TRANSPARENT
																					  , key->multi_shade.r2
																					  , key->multi_shade.g2
																					  , key->multi_shade.b2
																					  );
								else
									BlotScaledImageSizedToMultiShadedAlpha( surface, key->mask
																					  , 0, 0
																					  , key->width, key->height
																					  , ALPHA_TRANSPARENT
																					  , key->multi_shade.r
																					  , key->multi_shade.g
																					  , key->multi_shade.b
																					  );
							}
							else if( key->flags.bShade )
							{
								//lprintf( "mono shade %08x", ColorAverage( key->background.color, BASE_COLOR_WHITE, IsControlFocused( pc )? 64:0, 255 ) );
								BlotScaledImageSizedToShadedAlpha( surface, key->mask
																			, 0, 0
																			, key->width, key->height
																			, ALPHA_TRANSPARENT
																			, ColorAverage( key->background.color, BASE_COLOR_WHITE, IsControlFocused( pc )? 64:0, 255 )
																			);
							}
							else
							{
                        //lprintf( "no shade" );
								BlotScaledImageSizedToAlpha( surface, key->mask
																	, 0, 0
																	, key->width, key->height
																	, ALPHA_TRANSPARENT
																	);
							}
						}
					}
					else
					{
						//lprintf( "No mask..." );
#if 0
						if( keys_without_mask_get_color )
							if( key->flags.bGreyed )
								BlatColorAlpha( surface
												  , 0, 0
												  , key->width, key->height
												  , ColorAverage( key->background.color
																	 , BASE_COLOR_WHITE
																	 , ((!key->flags.bNoPress)&&key->buttonflags.pressed)?50:0
																	 , 100 )
												  );
							else
							{
								BlatColorAlpha( surface
												  , 0, 0
												  , key->width, key->height
												  , ColorAverage( key->background.color
																	 , BASE_COLOR_WHITE, ((!key->flags.bNoPress)&&key->buttonflags.pressed)?50:0, 100 )
												  );
							}
#endif
					}
				}
				if( key->flags.background_image )
				{
					//lprintf( WIDE("background image...") );
					if( key->flags.bGreyed )
						BlotScaledImageSizedToMultiShadedAlpha( surface, key->background.image
																		  , 0 + key->background.hMargin
																		  , 0 + key->background.vMargin
																		  , key->width - ( key->background.hMargin * 2 )
																		  , key->height - ( key->background.vMargin * 2 )
																		  , key->background.alpha
																			?(key->background.alpha<0)
																			?(ALPHA_TRANSPARENT_INVERT+(-key->background.alpha))
																			:(ALPHA_TRANSPARENT + key->background.alpha)
																			:(ALPHA_TRANSPARENT)
																		  , Color( 172, 172, 172 )
																		  , Color( 192, 192, 192 )
																		  , Color( 122, 255, 122 )//Color( 162, 162, 162 )
																		  );
               else
						BlotScaledImageSizedToAlpha( surface, key->background.image
															, 0 + key->background.hMargin
															, 0 + key->background.vMargin
															, key->width - ( key->background.hMargin * 2 )
															, key->height - ( key->background.vMargin * 2 )
															, key->background.alpha
															 ?(key->background.alpha<0)
															 ?(ALPHA_TRANSPARENT_INVERT+(-key->background.alpha))
															 :(ALPHA_TRANSPARENT + key->background.alpha)
															 :(ALPHA_TRANSPARENT)
															);
					if( !key->flags.bNoPress && (key->buttonflags.pressed) )
					{
						BlatColorAlpha( surface, 0, 0, key->width, key->height
										  , AColor( 255, 255, 255, 32 ) );
					}
					if( IsControlFocused( pc ) )
					{
						BlatColorAlpha( surface, 0, 0, key->width, key->height
                                 , AColor( 255, 255, 255, 48 ) );
					}
				}
			}

			if( layer == 2 && !key->flags.bLayered )
			{
				if( key->lense )
				{
					//lprintf( "Output glare surface." );
					BlotScaledImageSizedToAlpha( surface, key->lense
														, 0, 0
														, key->width, key->height
														, ALPHA_TRANSPARENT );
				}
				if( !key->flags.bNoPress && ( key->buttonflags.pressed ) )
				{
					if( key->frame_down )
					{
						//lprintf( "Output glare surface." );
						BlotScaledImageSizedToAlpha( surface, key->frame_down
															, 0, 0
															, key->width, key->height
															 //, ALPHA_TRANSPARENT_INVERT + 170
															, ALPHA_TRANSPARENT
															);
					}
				}
				else
				{
					if( key->frame_up )
					{
						//lprintf( "Output glare surface." );
						BlotScaledImageSizedToAlpha( surface, key->frame_up
															, 0, 0
															, key->width, key->height
															 //, ALPHA_TRANSPARENT_INVERT + 170
															, ALPHA_TRANSPARENT
															);
					}
				}
			}

			if( layer == textlayer )
			{
				if( !DrawButtonText( pc, surface, key ) )
				{
					layer = -1;
					continue;
				}
			}

			if( layer > 3 )
				break;
			// and viola - there's an image.
			// and perhaps even a location to update...
		}
}

static int OnCommonFocus( BUTTON_NAME )( PSI_CONTROL pc, LOGICAL focused )
{
   if( !focused )
		SmudgeCommon( pc );
   return 1;
}

static int OnDrawCommon( BUTTON_NAME )( PCONTROL pc )
{
	CTEXTSTR text;
	//CDATA foreground = 0;
   //CDATA background = 0;
	MyValidatedControlData( PKEY_BUTTON, key, pc );
   //lprintf( "Drawing Frame!? ");
	if( !key->button )
		return 0;
	{
		Image surface = GetControlSurface( pc );
		key->width = surface->width;
		key->height = surface->height;
		text = key->content;
		//Log4( WIDE("Drawing ... %d,%d   %d,%d"), key->width, key->height, surface->width, surface->height );
		//lprintf( "Update the lense layer which was draw already.." );
		if( key->flags.bLayered )
		{
			if( !key->flags.bOrdered )
			{
				PRENDERER r = GetFrameRenderer( GetFrame( pc ) );
				if( r )
				{
					//lprintf( "----------        Ordering Windows!" );
					if( key->animation_layer )
					{
						PutDisplayAbove( key->animation_layer, r );
						PutDisplayAbove( key->lense_layer, key->animation_layer );
					}
					else
						PutDisplayAbove( key->lense_layer, r );
					key->flags.bOrdered = 1;
				}
			}
			if( key->flags.bOrdered )
			{
				//lprintf( "post redraw..." );
				if( key->flags.bShown )
					Redraw( key->lense_layer );
				else
				{
					UpdateDisplay( key->lense_layer );
					key->flags.bShown = 1;
				}
			}
		}
		else
			DrawButtonLayers( key, surface, pc );

	}
	return 1;
}


//------------------------------------------------------------------------------------

void SetButtonPosition( PKEY_BUTTON key
							 , int32_t x, int32_t y )
{
	MoveControl( key->button, x, y );
}

//------------------------------------------------------------------------------------

void SetButtonSize( PKEY_BUTTON key
						, uint32_t width, uint32_t height )
{
	SizeControl( key->button, width, height );
}

//------------------------------------------------------------------------------------

void SetKeyColor( PKEY_BUTTON key, CDATA color )
{
	if( key )
	{
		key->background.color = color;
		key->flags.background_image = 0;
		SmudgeCommon( (PCOMMON)key->button );
	}
}

//------------------------------------------------------------------------------------

void SetKeyImageAlpha( PKEY_BUTTON key, int16_t alpha )
{
	if( key )
      key->background.alpha = alpha;
}

//------------------------------------------------------------------------------------
void SetKeyImageMargin( PKEY_BUTTON key, uint32_t hMargin, uint32_t vMargin )
{
	if( key )
	{
		key->background.hMargin = hMargin;
		key->background.vMargin = vMargin;
	}
}

int SetKeyImage( PKEY_BUTTON key, Image image )
{
	if( key )
	{
		if( key->flags.background_by_name )
		{
			if( key->background.image )
			{
				UnmakeImageFile(key->background.image );
			}
		}
		if( ( key->background.image = image ) )
			key->flags.background_image = 1;
		else
			key->flags.background_image = 0;
		key->flags.background_by_name = 0;
		key->background.alpha = 0;

		return TRUE;
	}
	return FALSE;
}

//------------------------------------------------------------------------------------

int SetKeyImageByName( PKEY_BUTTON key, CTEXTSTR name )
{
	if( key )
	{
		key->background.alpha = 0;
		if( name )
		{
			//TEXTSTR tmpGroup[256];
			if( ( key->background.image = LoadImageFileFromGroup( GetFileGroup( WIDE( "Button Resources" ), NULL ), name ) ) )
			{
				key->flags.background_by_name = 1;
				key->flags.background_image = 1;
				//SmudgeCommon( (PCOMMON)key->button );
				return TRUE;
			}
		}
		else
		{
			if( key->flags.background_by_name )
			{
				UnmakeImageFile( key->background.image );
			}
			key->background.image = NULL;
			key->flags.background_by_name = 0;
			key->flags.background_image = 0;
			return TRUE;
		}
	}
	return FALSE;
}

//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

void SetKeyGreyed( PKEY_BUTTON key, int greyed )
{
	if( key )
	{
		key->flags.bGreyed = greyed;
		SmudgeCommon( (PCOMMON)key->button );
	}
}

//------------------------------------------------------------------------------------

void SetKeyTextColor( PKEY_BUTTON key, CDATA color )
{
	if( key )
	{
		key->text_color = color;
		SmudgeCommon( (PCOMMON)key->button );
	}
}

//------------------------------------------------------------------------------------

void SetKeyText( PKEY_BUTTON key, CTEXTSTR newtext )
{
	CTEXTSTR end;
	size_t len;
	if( key )
	{
		LOGICAL updated = FALSE;
		if( newtext )
		{
			end = newtext;
			while( ( len = strlen( end ) ) )
				end += len + 1;
			end++;
			if( MemCmp( key->content, newtext, sizeof( TEXTCHAR ) * ( end - newtext + 1 ) ) != 0 )
			{
				if( key->content )
					Release( key->content );
				key->content = NewArray(TEXTCHAR, end - newtext + 1 );
				MemCpy( key->content, newtext, sizeof( TEXTCHAR ) * ( end - newtext + 1 ) );
				updated = TRUE;
			}
		}
		else
		{
			if( key->content )
			{
				updated = TRUE;
				Release( key->content );
			}
			key->content = NULL;
		}
      if( updated )
			if( !key->flags.bCreating )
				SmudgeCommon( (PCOMMON)key->button );
	}
}

//------------------------------------------------------------------------------------

void UpdateKey( PKEY_BUTTON key )
{
	if( key )
		SmudgeCommon( key->button );
}

//------------------------------------------------------------------------------------

void SetKeyLenses( PKEY_BUTTON key, Image lense, Image down, Image up, Image mask )
{
	if( key )
	{
		key->lense = lense;
		key->lense_alpha = 0;
		key->frame_up = up;
		key->frame_down = down;
		key->mask = mask;
      key->flags.bThemeSetExternal = 1;
		SmudgeCommon( key->button );
	}

}

//------------------------------------------------------------------------------------

void EnableKeyPresses( PKEY_BUTTON key, LOGICAL bEnable )
{
	if( key )
		key->flags.bNoPress = !bEnable;
}

//------------------------------------------------------------------------------------

void SetKeyPressEvent( PKEY_BUTTON key
						 , void (CPROC *PressHandler)( uintptr_t psv, PKEY_BUTTON key )
							, uintptr_t psvPress
							)
{
	if( key )
	{
		key->PressHandler = PressHandler;
		key->psvPress = psvPress;
	}
}

//------------------------------------------------------------------------------------

void GetKeyPressEvent( PKEY_BUTTON key
						 , void (CPROC **PressHandler)( uintptr_t psv, PKEY_BUTTON key )
							, uintptr_t *psvPress
							)
{
	if( key )
	{
		if( PressHandler )
			(*PressHandler) = key->PressHandler;
		if( psvPress )
			(*psvPress) = key->psvPress;
	}
}

void GetKeySimplePressEvent( PKEY_BUTTON key
						 , void (CPROC **SimplePressHandler)( uintptr_t psv )
							, uintptr_t *psvPress
							)
{
	if( key )
	{
		if( SimplePressHandler )
			(*SimplePressHandler) = key->SimplePressHandler;
		if( psvPress )
			(*psvPress) = key->psvPress;
	}
}

void SetKeyPressNamedEvent( PKEY_BUTTON key, CTEXTSTR PressHandlerName, uintptr_t psvPress )
{
	SimplePressHandler handler;
	TEXTCHAR realname[256];
	snprintf( realname, sizeof(realname), WIDE("sack/widgets/keypad/press handler/%s"), PressHandlerName );

	handler = GetRegisteredProcedure2( realname, void, WIDE("on_keypress_event"), (uintptr_t) );
	if( handler )
	{
		key->SimplePressHandler = handler;
		key->psvPress = psvPress;
	}
}

//CONTROL_REGISTARTION glass_like_button = { "Glasslike Button", { { 125, 25 }, sizeof( KEY_BUTTON ), BORDER_NO_BORDER }
//													  , NULL
//													  , NULL
//                                         ,

PKEY_BUTTON MakeKeyExx( PCOMMON frame
							 , int32_t x, int32_t y
							 , uint32_t width, uint32_t height
							 , uint32_t ID
							 , Image lense
							 , Image frame_up
							 , Image frame_down
							 , Image mask
							 , uint32_t flags
							 , CDATA background
							 , CTEXTSTR content
							 , SFTFont font
							 , void (CPROC *PressHandler)( uintptr_t psv, PKEY_BUTTON key )
							 , CTEXTSTR PressHandlerName
							 , uintptr_t psvPress
							 , CTEXTSTR value
							 )
{

	int loaded_default = 0;
	PCONTROL pc;
	if( !l.pii )
		l.pii = GetImageInterface();
	if( !l.pri )
		l.pri = GetDisplayInterface();
	if( !lense && !frame_up && !frame_down && !mask )
	{
		LoadButtonTheme();
		loaded_default = 1;
		lense = l.default_theme->buttons.iGlare;
		frame_up = l.default_theme->buttons.iNormal;
		frame_down = l.default_theme->buttons.iPressed;
		mask = l.default_theme->buttons.iMask;
	}
	// make sure everything is zero... (specially with release mode)
	pc = MakeNamedControl( frame, BUTTON_NAME
								, x, y, width, height // position?
								, 0  // ID
								);
	{
		MyValidatedControlData( PKEY_BUTTON, result, pc );
		result->flags.bCreating = 1;
		result->button = pc;
		if( flags & BUTTON_FLAG_TEXT_ON_TOP )
			result->flags.bLenseUnderText = TRUE;
		else
			result->flags.bLenseUnderText = FALSE;
		result->flags.bRidgeUnderText = TRUE;
		result->flags.bNoPress = FALSE;
		result->flags.bGreyed = FALSE;
		if( loaded_default )
		{
			if( l.default_theme->buttons.style == 2 )
			{
				result->multi_shade.layer2_colors = CreateDataList( sizeof( TRIPLET ) );
				result->flags.bMultiShade = 1;
				result->multi_shade.r = l.default_theme->buttons.color;
				result->multi_shade.g = l.default_theme->buttons.color;
				result->multi_shade.b = l.default_theme->buttons.color;
				SetDataItem( &result->multi_shade.layer2_colors, 0, &result->multi_shade );
			}
         if( !background )
				background = l.default_theme->buttons.color;
		}
		//SetCustomButtonMethods( );
		SetCommonTransparent( result->button, TRUE );
		SetCommonBorder( result->button, BORDER_NONE|BORDER_FIXED );
		//result->flags.bMultiShade = 0;
		result->lense = lense;
		//result->lense_alpha = 0;
		result->frame_up = frame_up;
		result->frame_down = frame_down;
		result->mask = mask;
		//result->background.alpha = 0;
		if( flags & KEY_BACKGROUND_IMAGE )
		{
			result->flags.background_image = 1;
			result->background.image = (Image)background;
		}
		else
		{
			result->flags.background_image = 0;
			result->background.color = background;
		}
		result->text_color = Color( 0, 0, 0 );
		SetCommonFont( result->button, font );
		SetKeyText( result, content );
		result->PressHandler = PressHandler;
		result->psvPress = psvPress;
		if( !PressHandler && PressHandlerName )
			SetKeyPressNamedEvent( result, PressHandlerName, psvPress );
		result->width = width;
		result->height = height;
		result->value = value;
		result->layout = NULL;
		result->flags.bCreating = 0;
		AddLink( &l.buttons, result );
		return result;
	}
}

PKEY_BUTTON MakeKeyEx( PCOMMON frame
							, int32_t x, int32_t y
							, uint32_t width, uint32_t height
							, uint32_t ID
							, Image lense
							, Image frame_up
							, Image frame_down
							, Image mask
							, uint32_t flags
							, CDATA background
							, CTEXTSTR content
							, SFTFont font
							, void (CPROC *PressHandler)( uintptr_t psv, PKEY_BUTTON key )
							 , uintptr_t psvPress
							, CTEXTSTR value
							)
{
	return MakeKeyExx( /*PCOMMON */frame
						  , /*int32_t*/ x
						  , /*int32_t*/ y
						  , /*uint32_t*/ width
						  , /*uint32_t*/ height
						  , /*uint32_t*/ ID
						  , /*Image*/ lense
						  , /*Image*/ frame_up
						  , /*Image*/ frame_down
						  , /*Image*/ mask
						  , /*uint32_t*/ flags
						  , /*CDATA*/ background
						  , /*CTEXTSTR */content
						  , /*SFTFont*/ font
						  , /*void (CPROC*PressHandler)( uintptr_t psv, PKEY_BUTTON key )*/ PressHandler
						  , NULL
						  , /*uintptr_t*/ psvPress
						  , /*CTEXTSTR */value
						  );
}


PKEY_BUTTON MakeKey( PCOMMON frame
							, int32_t x, int32_t y
							, uint32_t width, uint32_t height
							, uint32_t ID
							, Image lense
							, Image frame_up
							, Image frame_down
							, uint32_t flags
							, CDATA background
							, CTEXTSTR content
							, SFTFont font
							, void (CPROC *PressHandler)( uintptr_t psv, PKEY_BUTTON key )
							, uintptr_t psvPress
							, CTEXTSTR value
							)
{

	return MakeKeyEx( frame, x, y, width, height, ID
						 , lense, frame_up, frame_down, NULL
						 , flags, background, content, font, PressHandler, psvPress, value );
}

void DestroyKey( PKEY_BUTTON *key )
{
	if( key && *key )
	{
		PSI_CONTROL pc = (*key)->button;
		// if I pass &(*key)->button, the pointer in the controls' user space
		// is updated after it is freed.  This address passed should not be
		// IN the same control that is being deleted.
		DestroyCommon( &pc );
		(*key) = NULL;
	}
}

void HideKey( PKEY_BUTTON pKey )
{
	HideControl( pKey->button );
}

void ShowKey( PKEY_BUTTON pKey )
{
	RevealCommon( pKey->button );
}

PCOMMON GetKeyCommon ( PKEY_BUTTON pKey )
{
	if( pKey )
		return pKey->button;
	return NULL;
}

void SetKeyTextField( PTEXT_PLACEMENT pField, CTEXTSTR text )
{
	if( pField )
	{
		pField->content = StrDup( text );
	}
}

void SetKeyTextFieldColor( PTEXT_PLACEMENT pField, CDATA color )
{
	if( pField )
	{
      pField->text = color;
	}
}

PTEXT_PLACEMENT AddKeyLayout( PKEY_BUTTON pKey, int x, int y, SFTFont *font, CDATA color, uint32_t flags )
{
	PTEXT_PLACEMENT layout;
	//int count;
   //uint32_t w, h;
	layout = New( TEXT_PLACEMENT );
	MemSet( layout, 0, sizeof( TEXT_PLACEMENT ) ); // easiest when dealing with flags struct...
	layout->orig_x = x;
	layout->orig_y = y;
	layout->last_font = (SFTFont)INVALID_INDEX;
	layout->font = font;
	layout->text = color;
	layout->content = WIDE( "" );
	if( flags & BUTTON_FIELD_CENTER )
		layout->flags.bHorizCenter = 1;
	if( flags & BUTTON_FIELD_RIGHT )
		layout->flags.bDrawRightJust = 1;
	LinkThing( pKey->layout, layout );
	return layout;
}

void SetKeyMultiShading( PKEY_BUTTON key, CDATA r_channel, CDATA b_channel, CDATA g_channel )
{
	if( key )
	{
      if( !key->multi_shade.layer2_colors )
			key->multi_shade.layer2_colors = CreateDataList( sizeof( TRIPLET ) );
		key->flags.bShade = 0;
		key->flags.bMultiShade = 1;
		key->flags.bRidgeUnderText = FALSE;
		key->multi_shade.r = r_channel;
		key->multi_shade.g = g_channel;
		key->multi_shade.b = b_channel;
		{
			TRIPLET color;
			color.r = r_channel;
			color.g = g_channel;
			color.b = b_channel;
			SetDataItem( &key->multi_shade.layer2_colors, 0, &color );
		}

	}
}

void SetKeyMultiShadingHighlights( PKEY_BUTTON key, CDATA r_channel, CDATA b_channel, CDATA g_channel )
{
	if( key )
	{
		if( key->flags.bMultiShade )
		{
			PTRIPLET Normal = (PTRIPLET)GetDataItem( &key->multi_shade.layer2_colors, 0 );
			TRIPLET color;
			color.r = r_channel?r_channel:Normal->r;
			color.g = g_channel?g_channel:Normal->g;
			color.b = b_channel?b_channel:Normal->b;
			SetDataItem( &key->multi_shade.layer2_colors, 1, &color );

			if( r_channel )
				key->multi_shade.r2 = r_channel;
			else
				key->multi_shade.r2 = key->multi_shade.r;

			if( g_channel )
				key->multi_shade.g2 = g_channel;
			else
				key->multi_shade.g2 = key->multi_shade.g;

			if( b_channel )
				key->multi_shade.b2 = b_channel;
			else
				key->multi_shade.b2 = key->multi_shade.b;
		}
	}
}

void AddKeyMultiShadingHighlights( PKEY_BUTTON key, int highlight_level, CDATA r_channel, CDATA b_channel, CDATA g_channel )
{
	if( key )
	{
		if( key->flags.bMultiShade )
		{
			PTRIPLET Normal = (PTRIPLET)GetDataItem( &key->multi_shade.layer2_colors, 0 );
			TRIPLET color;
			color.r = r_channel;
			color.g = g_channel;
			color.b = b_channel;
			SetDataItem( &key->multi_shade.layer2_colors, highlight_level, &color );
		}
	}
}

void SetKeyShading( PKEY_BUTTON key, CDATA grey_channel ) // set grey_channel to 0 to disable shading.
{
	if( key )
	{
		if( grey_channel )
		{
			key->flags.bShade = 1;
			key->flags.bMultiShade = 0;
			key->background.color = grey_channel;
			key->flags.bRidgeUnderText = FALSE;
		}
		else
		{
			key->flags.bShade = 0;
			key->flags.bMultiShade = 0;
			key->flags.bRidgeUnderText = TRUE;
		}
	}
}

void SetKeyHighlight( PKEY_BUTTON key, LOGICAL bHighlight )
{
	if( key )
	{
		key->multi_shade.layer2_color = 1+bHighlight;
		key->flags.bHighlight = bHighlight;
		SmudgeCommon( key->button );
	}
}

void SetKeyLayer2Color( PKEY_BUTTON key, int color_index )
{
	if( key )
	{
		key->multi_shade.layer2_color = color_index+1;
		SmudgeCommon( key->button );
	}
}

LOGICAL GetKeyHighlight( PKEY_BUTTON key )
{
	if( key )
	{
		return key->flags.bHighlight;
	}
	return 0;
}

int GetKeyLayer2Color( PKEY_BUTTON key )
{
	if( key )
	{
		return key->multi_shade.layer2_color-1;
	}
	return 0;
}


PRENDERER GetButtonAnimationLayer( PSI_CONTROL pc_key_button )
{
	MyValidatedControlData( PKEY_BUTTON, key, pc_key_button );
	if( key )
	{
		// could enable the uhmm layered flag now, (change drawing) and seperate the layers dynamically!

		//if( !key->animation_layer )
		{
			key->animation_layer = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD
																		 , key->width - 40 , key->height - 40
																		 , key->real_x + 20 , key->real_y + 20 );
			//SetRedrawHandler( button->lense_layer, DrawGlareLayer, (uintptr_t)pc );
			if( key->flags.bOrdered )
			{
				//lprintf( "Inserting the animation layer between lense_layer" );
				if( key->flags.bLayered )
					PutDisplayAbove( key->lense_layer, key->animation_layer );
				else
				{
					// position/size needs to be computed if not layered.
					PutDisplayAbove( GetFrameRenderer( GetFrame( pc_key_button ) ), key->animation_layer );
				}
			}
		}
		return key->animation_layer;
	}
	return NULL;
}



 // Intended use: Supply configuration slots for theme_id
static void OnThemeAdded( WIDE( "Button Widget" ) )( PCanvasData canvas, int theme_id )
{
	// Allocate memory for structure
	PTHEME theme = LoadButtonThemeByName( WIDE("Normal Button"), theme_id );

	// Add Theme Struct pointer to theme list
	SetLink( &l.theme_list, theme_id, theme );
	
	return;
}

/* Intended use: Theme is changing, the theme_id that is given was the prior theme set */
static void OnThemeChanging( WIDE( "Button Widget" ) )( PCanvasData canvas, int theme_id )
{

	return;
}

static void OnThemeChanged( WIDE( "Button Widget" ) )(PCanvasData canvas,  int theme_id )
{
	INDEX idx;
	PTHEME use_theme = (PTHEME)GetLink( &l.theme_list, theme_id );
	PKEY_BUTTON key;
	LIST_FORALL( l.buttons, idx, PKEY_BUTTON, key )
	{
		if( !key->flags.bThemeSetExternal )
		{
			key->lense = use_theme->buttons.iGlare;
			key->lense_alpha = 0;
			key->frame_up = use_theme->buttons.iNormal;
			key->frame_down = use_theme->buttons.iPressed;
			key->mask = use_theme->buttons.iMask;
			if( use_theme->buttons.style == 2 )
				key->flags.bMultiShade = 1;
		}
	}

	return;
}

LOGICAL GetKeyLenses( CTEXTSTR style, int theme_id, int *use_color, CDATA *color, CDATA *text_color, Image *lense, Image *down, Image *up, Image *mask )
{

	PTHEME use_theme = LoadButtonThemeByNameEx( style, theme_id, color?(*color):0, text_color?(*text_color):0 );
	if( use_theme )
	{
		if( lense )
			(*lense) = use_theme->buttons.iGlare;
		if( down )
			(*down) = use_theme->buttons.iPressed;
		if( up )
			(*up) = use_theme->buttons.iNormal;
		if( mask )
			(*mask) = use_theme->buttons.iMask;
		if( use_color )
			(*use_color) = use_theme->buttons.style;
		if( color )
         (*color ) = use_theme->buttons.color;
		if( text_color )
         (*text_color ) = use_theme->buttons.text_color;
      return TRUE;
	}
   return FALSE;
}



#ifdef __cplusplus
		} //namespace buttons {
	} // namespace widgets {
 } // namespace sack {
#endif


