
//#include <procreg.h>
#include <controls.h>

#include "resource.h"
// control_type_name = the text name of a control
// that this ID is expected to identify... NORMAL_BUTTON_NAME for instance

//------------------------------------------------------
//#ifdef BUILD_NAMES
// and other C99 compilers





typedef struct resource_names *PRESOURCE_NAMES;



PRELOAD( RegisterInterShellResources )
{
#define f(name,range,stringname,typename) RegisterResource( WIDE( "intershell" ), stringname, name, range, typename )

 ; f( MNU_CREATE_CONTROL, 1, WIDE( "MNU_CREATE_CONTROL" ), WIDE( "Popup Menu" ) )
 ; f( MNU_EDIT_CONTROL, 1, WIDE( "MNU_EDIT_CONTROL" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_DESTROY_CONTROL, 1, WIDE( "MNU_DESTROY_CONTROL" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CREATE_CLOCK, 1, WIDE( "MNU_CREATE_CLOCK" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CREATE_MANAGER, 1, WIDE( "MNU_CREATE_MANAGER" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_EXTRA_CONTROL, 1, WIDE( "MNU_EXTRA_CONTROL" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CREATE_KEYPAD, 1, WIDE( "MNU_CREATE_KEYPAD" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CREATE_ISSUE, 1, WIDE( "MNU_CREATE_ISSUE" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CREATE_USERINFO, 1, WIDE( "MNU_CREATE_USERINFO" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CLONE, 1, WIDE( "MNU_CLONE" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_MAKE_CLONE, 1, WIDE( "MNU_MAKE_CLONE" ), WIDE( "Popup Menu" ) ) 

// start a whole new range
 ; f( MNU_CREATE_EXTRA, 1, WIDE( "MNU_CREATE_EXTRA" ), WIDE( "Popup Menu" ) ) 
		 ; f( MNU_CREATE_EXTRA, 1000, WIDE( "MNU_CREATE_EXTRA" ), WIDE( "Popup Menu" ) ) ; f( MNU_CREATE_EXTRA_MAX, 1, WIDE( "MNU_CREATE_EXTRA_MAX" ), WIDE( "Popup Menu" ) ) 
		 ; f( MNU_CHANGE_PAGE, 1, WIDE( "MNU_CHANGE_PAGE" ), WIDE( "Popup Menu" ) ) 
       ; f( MNU_CHANGE_PAGE, 256, WIDE( "MNU_CHANGE_PAGE" ), WIDE( "Popup Menu" ) ) ; f( MNU_CHANGE_PAGE_MAX, 1, WIDE( "MNU_CHANGE_PAGE_MAX" ), WIDE( "Popup Menu" ) ) 
		 ; f( MNU_DESTROY_PAGE, 1, WIDE( "MNU_DESTROY_PAGE" ), WIDE( "Popup Menu" ) ) 
       ; f( MNU_DESTROY_PAGE, 256, WIDE( "MNU_DESTROY_PAGE" ), WIDE( "Popup Menu" ) ) ; f( MNU_DESTROY_PAGE_MAX, 1, WIDE( "MNU_DESTROY_PAGE_MAX" ), WIDE( "Popup Menu" ) ) 
		 ; f( MNU_UNDELETE_PAGE, 1, WIDE( "MNU_UNDELETE_PAGE" ), WIDE( "Popup Menu" ) ) 
       ; f( MNU_UNDELETE_PAGE, 256, WIDE( "MNU_UNDELETE_PAGE" ), WIDE( "Popup Menu" ) ) ; f( MNU_UNDELETE_PAGE_MAX, 1, WIDE( "MNU_UNDELETE_PAGE_MAX" ), WIDE( "Popup Menu" ) ) 


 ; f( KEYPAD_KEYS, 1, WIDE( "KEYPAD_KEYS" ), WIDE( "Keypad Control" ) ) 

//--- Super menu options...
 ; f( MNU_EDIT_SCREEN, 1, WIDE( "MNU_EDIT_SCREEN" ), WIDE( "Popup Menu" ) ) 


//--- Edit menu options...
 ; f( MNU_ADD_KEYPAD, 1, WIDE( "MNU_ADD_KEYPAD" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_EDIT_DONE, 1, WIDE( "MNU_EDIT_DONE" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_PAGE_PROPERTIES, 1, WIDE( "MNU_PAGE_PROPERTIES" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_CREATE_PAGE, 1, WIDE( "MNU_CREATE_PAGE" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_RENAME_PAGE, 1, WIDE( "MNU_RENAME_PAGE" ), WIDE( "Popup Menu" ) ) 


//--- Keypad property menu...
 ; f( MNU_SETFONT, 1, WIDE( "MNU_SETFONT" ), WIDE( "Popup Menu" ) ) 

//--- Button property menu...
//SYMNAME( MNU_SETFONT


 ; f( TXT_CONTROL_TEXT, 1, WIDE( "TXT_CONTROL_TEXT" ), WIDE( "EditControl" ) ) 
//SYMNAME( TXT_TASK_NAME       , EDIT_FIELD_NAME )
//SYMNAME( TXT_TASK_PATH       , EDIT_FIELD_NAME )
//SYMNAME( TXT_TASK_ARGS       , EDIT_FIELD_NAME )
 ; f( CLR_BACKGROUND, 1, WIDE( "CLR_BACKGROUND" ), WIDE( "Color Well" ) ) 
 ; f( CHK_BACK_IMAGE, 1, WIDE( "CHK_BACK_IMAGE" ), WIDE( "CheckButton" ) ) 
 ; f( TXT_IMAGE_NAME, 1, WIDE( "TXT_IMAGE_NAME" ), WIDE( "EditControl" ) ) 
 ; f( TXT_ANIMATION_NAME, 1, WIDE( "TXT_ANIMATION_NAME" ), WIDE( "EditControl" ) ) 
 ; f( BTN_PICKFILE, 1, WIDE( "BTN_PICKFILE" ), WIDE( "Button" ) ) 
 ; f( BTN_ADD_PAGE_THEME, 1, WIDE( "BTN_ADD_PAGE_THEME" ), WIDE( "Button" ) )
 ; f( BTN_PICKANIMFILE, 1, WIDE( "BTN_PICKANIMFILE" ), WIDE( "Button" ) ) 
 ; f( CLR_TEXT_COLOR, 1, WIDE( "CLR_TEXT_COLOR" ), WIDE( "Color Well" ) ) 
 ; f( CHK_ROUND, 1, WIDE( "CHK_ROUND" ), WIDE( "CheckButton" ) ) 
 ; f( CHK_SQUARE, 1, WIDE( "CHK_SQUARE" ), WIDE( "CheckButton" ) ) 
 ; f( CHK_NOLENSE, 1, WIDE( "CHK_NOLENSE" ), WIDE( "CheckButton" ) ) 
 ; f( CHK_NOPRESS, 1, WIDE( "CHK_NOPRESS" ), WIDE( "CheckButton" ) ) 
 ; f( CLR_RING_BACKGROUND, 1, WIDE( "CLR_RING_BACKGROUND" ), WIDE( "Color Well" ) ) 
 ; f( LST_PAGES, 1, WIDE( "LST_PAGES" ), WIDE( "ListBox" ) )  // listbox for pages 
 ; f( LST_VARIABLES, 1, WIDE( "LST_VARIABLES" ), WIDE( "ListBox" ) ) 
 ; f( LISTBOX_PAGE_THEME, 1, WIDE( "LISTBOX_PAGE_THEME" ), WIDE( "ListBox" ) )
		 ; f( LST_FONTS, 1, WIDE( "LST_FONTS" ), WIDE( "ListBox" ) ) 
		 ; f( LST_SELECT_USER, 1, WIDE( "LST_SELECT_USER" ), WIDE( "ListBox" ) ) 
		 ; f( EDT_PASSWORD, 1, WIDE( "EDT_PASSWORD" ), WIDE( "EditControl" ) ) 
       ; f( LST_BUTTON_STYLE, 1, WIDE( "LST_BUTTON_STYLE" ), WIDE( "ListBox" ) ) 
 ; f( MNU_EDIT_BEHAVIORS, 1, WIDE( "MNU_EDIT_BEHAVIORS" ), WIDE( "Popup Menu" ) ) 

 ; f( LST_HALL, 1, WIDE( "LST_HALL" ), WIDE( "ListBox" ) ) 
 ; f( LST_BUTTON_MODES, 1, WIDE( "LST_BUTTON_MODES" ), WIDE( "ListBox" ) ) 
 ; f( LST_ELECTRONIC_ITEMS, 1, WIDE( "LST_ELECTRONIC_ITEMS" ), WIDE( "ListBox" ) ) 
 ; f( LST_REGION_SELECT, 1, WIDE( "LST_REGION_SELECT" ), WIDE( "ListBox" ) ) 
		 ; f( MNU_EDIT_FONTS, 1, WIDE( "MNU_EDIT_FONTS" ), WIDE( "Popup Menu" ) ) 
		 ; f( CLR_RING_HIGHLIGHT, 1, WIDE( "CLR_RING_HIGHLIGHT" ), WIDE( "Color Well" ) ) 

		/* glare set edit resources */
		/* menu set for global property plugins - auto loaded tasks, wait for caller flags, etc...
         first module to use this is tasks.isp */
		 ; f( MNU_GLOBAL_PROPERTIES, 1, WIDE( "MNU_GLOBAL_PROPERTIES" ), WIDE( "Popup Menu" ) ) 
       ; f( MNU_GLOBAL_PROPERTIES, 256, WIDE( "MNU_GLOBAL_PROPERTIES" ), WIDE( "Popup Menu" ) ) ; f( MNU_GLOBAL_PROPERTIES_MAX, 1, WIDE( "MNU_GLOBAL_PROPERTIES_MAX" ), WIDE( "Popup Menu" ) ) 
       ; f( CHECKBOX_LABEL_CENTER, 1, WIDE( "CHECKBOX_LABEL_CENTER" ), WIDE( "CheckButton" ) ) 
       ; f( CHECKBOX_LABEL_RIGHT, 1, WIDE( "CHECKBOX_LABEL_RIGHT" ), WIDE( "CheckButton" ) ) 
       ; f( CHECKBOX_LABEL_SCROLL, 1, WIDE( "CHECKBOX_LABEL_SCROLL" ), WIDE( "CheckButton" ) )
       ; f( CHECKBOX_LABEL_SHADOW, 1, WIDE( "CHECKBOX_LABEL_SHADOW" ), WIDE( "CheckButton" ) )
       ; f( CHECKBOX_LIST_MULTI_SELECT, 1, WIDE( "CHECKBOX_LIST_MULTI_SELECT" ), WIDE( "CheckButton" ) ) 
       ; f( CHECKBOX_LIST_LAZY_MULTI_SELECT, 1, WIDE( "CHECKBOX_LIST_LAZY_MULTI_SELECT" ), WIDE( "CheckButton" ) ) 

      //--- added additional options for menu, not implemented on menu
 ; f( MNU_COPY, 1, WIDE( "MNU_COPY" ), WIDE( "Popup Menu" ) ) 
 ; f( MNU_PASTE, 1, WIDE( "MNU_PASTE" ), WIDE( "Popup Menu" ) ) 
		 ; f( MNU_EDIT_CONTROL_COMMON, 1, WIDE( "MNU_EDIT_CONTROL_COMMON" ), WIDE( "Popup Menu" ) ) 
 ; f( TXT_IMAGE_H_MARGIN, 1, WIDE( "TXT_IMAGE_H_MARGIN" ), WIDE( "EditControl" ) ) 
 ; f( TXT_IMAGE_V_MARGIN, 1, WIDE( "TXT_IMAGE_V_MARGIN" ), WIDE( "EditControl" ) ) 
 ;
  f( LABEL_TEXT_COLOR           , 1, WIDE("LABEL_TEXT_COLOR"), STATIC_TEXT_NAME );
  f(LABEL_BACKGROUND_COLOR     , 1, WIDE( "LABEL_BACKGROUND_COLOR"), STATIC_TEXT_NAME );
  f(LABEL_RING_COLOR           , 1, WIDE( "LABEL_RING_COLOR"), STATIC_TEXT_NAME );
  f(LABEL_RING_HIGHLIGHT_COLOR , 1, WIDE( "LABEL_RING_HIGHTLIGHT_COLOR" ), STATIC_TEXT_NAME );
}
