/*
 * Crafted by: Jim Buckeyne
 *
 * Pretty buttons, based on bag.psi image class buttons
 * and extended to the point that all that really remains is
 * the click handler... easy enough to steal, but it does
 * nice things like changes behavior based on touch screen
 * presense, and particular operational modes...
 *
 * Keypads - with numeric accumulator displays, based on
 *   shared memory regions such that many displays might
 *   accumulate into the same memory, with different
 *   visual aspects, and might get updated in return
 *   (well that's the theory) mostly it's a container
 *   for coordinating key board like events, a full alpha-
 *   numeric keyboard option exists, a 10 key entry pad (12)
 *   and soon a 16 key extended input with clear, cancel
 *   double 00, and decimal, and backspace  and(?)
 *
 * Some common external images may be defined in a theme.ini
 *  applications which don't otherwise specify graphics effects
 *  for the buttons enables them to conform to a default theme
 *  or one of multiple selections...
 *
 * (c)Freedom Collective 2005
 */


#ifndef GLASS_LIKE_BUTTONS_DEFINED_HERE
#define GLASS_LIKE_BUTTONS_DEFINED_HERE
#include <controls.h>

#if defined( INTERSHELL_CORE_BUILD ) || defined( KEYPAD_SOURCE )
#include "keypad.h"
#else
#include <InterShell/widgets/keypad.h>
#endif

#ifdef __cplusplus
namespace sack {
/* This namespace contains a few external widgets which are PSI
   based. They are used in InterShell for a bit of its basic
   functionality.
   
   
   
   <link sack::widgets::buttons, Buttons>
   
   <link sack::widgets::banner, Banners>                        */

	namespace widgets {
		namespace buttons {
#endif
#define KEY_BACKGROUND_IMAGE 0x0001 // passing an image for the background...
#define KEY_BACKGROUND_COLOR 0x0000 // !image

// format characters which much preceed each content
// item of a button...
// "A1" - all center (vert/horiz) 1
// "LElec: BLUE\0LSession 1\0r$1.00\0\0"
//      First line left align "Elec:Blue"
//      Second line left align "Session 1"
//      Same(Second line right align "$1.00"
// double \0 terminator - end of content.

#define CONTENT_NORMAL    WIDE( "N" )
#define CONTENT_LEFT      WIDE( "L" )
#define CONTENT_RIGHT     WIDE( "R" )
#define CONTENT_CENTER    WIDE( "C" )
#define CONTENT_VCENTER   WIDE( "V" )
#define CONTENT_CENTERALL WIDE( "A" )
#define CONTENT_SAMELINE_NORMAL    WIDE( "n" )
#define CONTENT_SAMELINE_LEFT      WIDE( "l" )
#define CONTENT_SAMELINE_RIGHT     WIDE( "r" )
#define CONTENT_SAMELINE_CENTER    WIDE( "c" )
#define CONTENT_SAMELINE_VCENTER   WIDE( "v" )
#define CONTENT_SAMELINE_CENTERALL WIDE( "a" )

//-- buttons.c --------------------
typedef struct key_button_tag *PKEY_BUTTON;
typedef struct text_placement_tag *PTEXT_PLACEMENT;

// call this to invoke default images...
// these will be used when all image pointers
// passed to make key are NULL.
KEYPAD_PROC( void, LoadButtonTheme )( void );


#define BUTTON_FLAG_TEXT_ON_TOP 1

#define OnKeyPressEvent(name)  \
	  DefineRegistryMethod(WIDE( "sack/widgets" ),KeyPressHandler,WIDE( "keypad" ),WIDE( "press handler/" )name, WIDE( "on_keypress_event" ),void,(uintptr_t),__LINE__)

	/*
	 * {
	 *  CTEXTSTR name;
	 *  POINTER data = NULL;
	 *  for( name = GetFirstRegisteredName( WIDE("sack/widgets/keypad/press handler"), &data );
	 *  	 name;
	 *  	  name = GetNextRegisteredName( &data ) )
	 *  {
	 *     // add name to a list of available methods to hook to keys for external configuration...
	 *    SetItemData( AddListItem( (PSI_CONTROL)list, name )
	 *               ,(uintptr_t)GetRegisteredProcedure2( data, void, name, (uintptr_t,PKEY_BUTTON) );
	 *  }
	 * }
	 *
	 *
    */

typedef void (CPROC *PressHandler)( uintptr_t psv, PKEY_BUTTON key );
typedef void (CPROC *SimplePressHandler)( uintptr_t psv );
KEYPAD_PROC( PKEY_BUTTON, MakeKeyExx )( PSI_CONTROL frame
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
												  , PressHandler
                                       , CTEXTSTR PressHandlerName
											  , uintptr_t psvPress
											  , CTEXTSTR value
											  );
KEYPAD_PROC( PKEY_BUTTON, MakeKeyEx )( PSI_CONTROL frame
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
											  , PressHandler
											  , uintptr_t psvPress
											  , CTEXTSTR value
											  );
KEYPAD_PROC( PKEY_BUTTON, MakeKey )( PSI_CONTROL frame
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
											  , PressHandler
											  , uintptr_t psvPress
											  , CTEXTSTR value
											  );
KEYPAD_PROC( void, DestroyKey )( PKEY_BUTTON *key );

CTEXTSTR GetKeyValue( PKEY_BUTTON pKey );

KEYPAD_PROC( void, SetKeyPressEvent )( PKEY_BUTTON key, PressHandler function, uintptr_t psvPress );
KEYPAD_PROC( void, GetKeyPressEvent )( PKEY_BUTTON key, PressHandler *function, uintptr_t *psvPress );
KEYPAD_PROC( void, GetKeySimplePressEvent )( PKEY_BUTTON key, SimplePressHandler *function, uintptr_t *psvPress );
KEYPAD_PROC( void, SetKeyPressNamedEvent )( PKEY_BUTTON key, CTEXTSTR PressHandler, uintptr_t psvPress );
KEYPAD_PROC( void, SetKeyText )( PKEY_BUTTON key, CTEXTSTR newtext );

KEYPAD_PROC(void, SetKeyTextColor )( PKEY_BUTTON key, CDATA color );
KEYPAD_PROC(void, SetKeyColor )( PKEY_BUTTON key, CDATA color );
// return TRUE if image name loaded successfully...
KEYPAD_PROC(int, SetKeyImage )( PKEY_BUTTON key, Image image );
KEYPAD_PROC(int, SetKeyImageByName )( PKEY_BUTTON key, CTEXTSTR name );
KEYPAD_PROC(void, SetKeyImageAlpha )( PKEY_BUTTON key, int16_t alpha );

KEYPAD_PROC(void, SetKeyGreyed )( PKEY_BUTTON key, int greyed );
KEYPAD_PROC(void, UpdateKey )( PKEY_BUTTON key );

KEYPAD_PROC( void, HideKey )( PKEY_BUTTON pKey );
KEYPAD_PROC( void, ShowKey )( PKEY_BUTTON pKey );
KEYPAD_PROC( void, SetKeyPressed )( PKEY_BUTTON key );
KEYPAD_PROC( void, SetKeyReleased )( PKEY_BUTTON key );


KEYPAD_PROC( PSI_CONTROL, GetKeyCommon )( PKEY_BUTTON pKey );

KEYPAD_PROC( void, SetKeyLenses )( PKEY_BUTTON key, Image lense, Image down, Image up, Image mask );
KEYPAD_PROC( LOGICAL, GetKeyLenses )( CTEXTSTR style, int theme_id, int *use_color, CDATA *color, CDATA *text_color, Image *lense, Image *down, Image *up, Image *mask );
KEYPAD_PROC( void, EnableKeyPresses )( PKEY_BUTTON key, LOGICAL bEnable );

KEYPAD_PROC( void, SetKeyTextField )( PTEXT_PLACEMENT pField, CTEXTSTR text );
KEYPAD_PROC( void, SetKeyTextFieldColor )( PTEXT_PLACEMENT pField, CDATA color );
// this field is centered horizontally using only the Y coordinate passed.. (x is ignored)
#define BUTTON_FIELD_CENTER 1
// this field is displayed using the x as the right side of the string to show.
#define BUTTON_FIELD_RIGHT 2
KEYPAD_PROC( PTEXT_PLACEMENT, AddKeyLayout )( PKEY_BUTTON pKey, int x, int y, SFTFont *font, CDATA color, uint32_t flags );

KEYPAD_PROC( void, SetKeyMultiShading )( PKEY_BUTTON key, CDATA r_channel, CDATA b_channel, CDATA g_channel );
KEYPAD_PROC( void, SetKeyMultiShadingHighlights )( PKEY_BUTTON key, CDATA r_channel, CDATA b_channel, CDATA g_channel );
KEYPAD_PROC( void, SetKeyShading )( PKEY_BUTTON key, CDATA grey_channel ); // set grey_channel to 0 to disable shading.

KEYPAD_PROC( void, SetKeyImageMargin )( PKEY_BUTTON key, uint32_t hMargin, uint32_t vMargin );
KEYPAD_PROC( void, SetKeyHighlight )( PKEY_BUTTON key, LOGICAL bEnable );
KEYPAD_PROC( LOGICAL, GetKeyHighlight )( PKEY_BUTTON key );
KEYPAD_PROC( PRENDERER, GetButtonAnimationLayer )( PSI_CONTROL pc_key_button );

KEYPAD_PROC( void, AddKeyMultiShadingHighlights )( PKEY_BUTTON key, int highlight_level, CDATA r_channel, CDATA b_channel, CDATA g_channel );
KEYPAD_PROC( void, SetKeyLayer2Color )( PKEY_BUTTON key, int color_index );
KEYPAD_PROC( int, GetKeyLayer2Color )( PKEY_BUTTON key );



KEYPAD_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::widgets::buttons;
#endif

#endif
