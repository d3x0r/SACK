
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
#define f(name,range,stringname,typename) RegisterResource( "intershell", stringname, name, range, typename )

 ; f( MNU_CREATE_CONTROL, 1, "MNU_CREATE_CONTROL", "Popup Menu" )
 ; f( MNU_EDIT_CONTROL, 1, "MNU_EDIT_CONTROL", "Popup Menu" ) 
 ; f( MNU_DESTROY_CONTROL, 1, "MNU_DESTROY_CONTROL", "Popup Menu" ) 
 ; f( MNU_CREATE_CLOCK, 1, "MNU_CREATE_CLOCK", "Popup Menu" ) 
 ; f( MNU_CREATE_MANAGER, 1, "MNU_CREATE_MANAGER", "Popup Menu" ) 
 ; f( MNU_EXTRA_CONTROL, 1, "MNU_EXTRA_CONTROL", "Popup Menu" ) 
 ; f( MNU_CREATE_KEYPAD, 1, "MNU_CREATE_KEYPAD", "Popup Menu" ) 
 ; f( MNU_CREATE_ISSUE, 1, "MNU_CREATE_ISSUE", "Popup Menu" ) 
 ; f( MNU_CREATE_USERINFO, 1, "MNU_CREATE_USERINFO", "Popup Menu" ) 
 ; f( MNU_CLONE, 1, "MNU_CLONE", "Popup Menu" ) 
 ; f( MNU_MAKE_CLONE, 1, "MNU_MAKE_CLONE", "Popup Menu" ) 

// start a whole new range
 ; f( MNU_CREATE_EXTRA, 1, "MNU_CREATE_EXTRA", "Popup Menu" ) 
		 ; f( MNU_CREATE_EXTRA, 1000, "MNU_CREATE_EXTRA", "Popup Menu" ) ; f( MNU_CREATE_EXTRA_MAX, 1, "MNU_CREATE_EXTRA_MAX", "Popup Menu" ) 
		 ; f( MNU_CHANGE_PAGE, 1, "MNU_CHANGE_PAGE", "Popup Menu" ) 
       ; f( MNU_CHANGE_PAGE, 256, "MNU_CHANGE_PAGE", "Popup Menu" ) ; f( MNU_CHANGE_PAGE_MAX, 1, "MNU_CHANGE_PAGE_MAX", "Popup Menu" ) 
		 ; f( MNU_DESTROY_PAGE, 1, "MNU_DESTROY_PAGE", "Popup Menu" ) 
       ; f( MNU_DESTROY_PAGE, 256, "MNU_DESTROY_PAGE", "Popup Menu" ) ; f( MNU_DESTROY_PAGE_MAX, 1, "MNU_DESTROY_PAGE_MAX", "Popup Menu" ) 
		 ; f( MNU_UNDELETE_PAGE, 1, "MNU_UNDELETE_PAGE", "Popup Menu" ) 
       ; f( MNU_UNDELETE_PAGE, 256, "MNU_UNDELETE_PAGE", "Popup Menu" ) ; f( MNU_UNDELETE_PAGE_MAX, 1, "MNU_UNDELETE_PAGE_MAX", "Popup Menu" ) 


 ; f( KEYPAD_KEYS, 1, "KEYPAD_KEYS", "Keypad Control" ) 

//--- Super menu options...
 ; f( MNU_EDIT_SCREEN, 1, "MNU_EDIT_SCREEN", "Popup Menu" ) 


//--- Edit menu options...
 ; f( MNU_ADD_KEYPAD, 1, "MNU_ADD_KEYPAD", "Popup Menu" ) 
 ; f( MNU_EDIT_DONE, 1, "MNU_EDIT_DONE", "Popup Menu" ) 
 ; f( MNU_PAGE_PROPERTIES, 1, "MNU_PAGE_PROPERTIES", "Popup Menu" ) 
 ; f( MNU_CREATE_PAGE, 1, "MNU_CREATE_PAGE", "Popup Menu" ) 
 ; f( MNU_RENAME_PAGE, 1, "MNU_RENAME_PAGE", "Popup Menu" ) 


//--- Keypad property menu...
 ; f( MNU_SETFONT, 1, "MNU_SETFONT", "Popup Menu" ) 

//--- Button property menu...
//SYMNAME( MNU_SETFONT


 ; f( TXT_CONTROL_TEXT, 1, "TXT_CONTROL_TEXT", "EditControl" ) 
//SYMNAME( TXT_TASK_NAME       , EDIT_FIELD_NAME )
//SYMNAME( TXT_TASK_PATH       , EDIT_FIELD_NAME )
//SYMNAME( TXT_TASK_ARGS       , EDIT_FIELD_NAME )
 ; f( CLR_BACKGROUND, 1, "CLR_BACKGROUND", "Color Well" ) 
 ; f( CHK_BACK_IMAGE, 1, "CHK_BACK_IMAGE", "CheckButton" ) 
 ; f( TXT_IMAGE_NAME, 1, "TXT_IMAGE_NAME", "EditControl" ) 
 ; f( TXT_ANIMATION_NAME, 1, "TXT_ANIMATION_NAME", "EditControl" ) 
 ; f( BTN_PICKFILE, 1, "BTN_PICKFILE", "Button" ) 
 ; f( BTN_ADD_PAGE_THEME, 1, "BTN_ADD_PAGE_THEME", "Button" )
 ; f( BTN_PICKANIMFILE, 1, "BTN_PICKANIMFILE", "Button" ) 
 ; f( CLR_TEXT_COLOR, 1, "CLR_TEXT_COLOR", "Color Well" ) 
 ; f( CHK_ROUND, 1, "CHK_ROUND", "CheckButton" ) 
 ; f( CHK_SQUARE, 1, "CHK_SQUARE", "CheckButton" ) 
 ; f( CHK_NOLENSE, 1, "CHK_NOLENSE", "CheckButton" ) 
 ; f( CHK_NOPRESS, 1, "CHK_NOPRESS", "CheckButton" ) 
 ; f( CLR_RING_BACKGROUND, 1, "CLR_RING_BACKGROUND", "Color Well" ) 
 ; f( LST_PAGES, 1, "LST_PAGES", "ListBox" )  // listbox for pages 
 ; f( LST_VARIABLES, 1, "LST_VARIABLES", "ListBox" ) 
 ; f( LISTBOX_PAGE_THEME, 1, "LISTBOX_PAGE_THEME", "ListBox" )
		 ; f( LST_FONTS, 1, "LST_FONTS", "ListBox" ) 
		 ; f( LST_SELECT_USER, 1, "LST_SELECT_USER", "ListBox" ) 
		 ; f( EDT_PASSWORD, 1, "EDT_PASSWORD", "EditControl" ) 
       ; f( LST_BUTTON_STYLE, 1, "LST_BUTTON_STYLE", "ListBox" ) 
 ; f( MNU_EDIT_BEHAVIORS, 1, "MNU_EDIT_BEHAVIORS", "Popup Menu" ) 

 ; f( LST_HALL, 1, "LST_HALL", "ListBox" ) 
 ; f( LST_BUTTON_MODES, 1, "LST_BUTTON_MODES", "ListBox" ) 
 ; f( LST_ELECTRONIC_ITEMS, 1, "LST_ELECTRONIC_ITEMS", "ListBox" ) 
 ; f( LST_REGION_SELECT, 1, "LST_REGION_SELECT", "ListBox" ) 
		 ; f( MNU_EDIT_FONTS, 1, "MNU_EDIT_FONTS", "Popup Menu" ) 
		 ; f( CLR_RING_HIGHLIGHT, 1, "CLR_RING_HIGHLIGHT", "Color Well" ) 

		/* glare set edit resources */
		/* menu set for global property plugins - auto loaded tasks, wait for caller flags, etc...
         first module to use this is tasks.isp */
		 ; f( MNU_GLOBAL_PROPERTIES, 1, "MNU_GLOBAL_PROPERTIES", "Popup Menu" ) 
       ; f( MNU_GLOBAL_PROPERTIES, 256, "MNU_GLOBAL_PROPERTIES", "Popup Menu" ) ; f( MNU_GLOBAL_PROPERTIES_MAX, 1, "MNU_GLOBAL_PROPERTIES_MAX", "Popup Menu" ) 
       ; f( CHECKBOX_LABEL_CENTER, 1, "CHECKBOX_LABEL_CENTER", "CheckButton" ) 
       ; f( CHECKBOX_LABEL_RIGHT, 1, "CHECKBOX_LABEL_RIGHT", "CheckButton" ) 
       ; f( CHECKBOX_LABEL_SCROLL, 1, "CHECKBOX_LABEL_SCROLL", "CheckButton" )
       ; f( CHECKBOX_LABEL_SHADOW, 1, "CHECKBOX_LABEL_SHADOW", "CheckButton" )
       ; f( CHECKBOX_LIST_MULTI_SELECT, 1, "CHECKBOX_LIST_MULTI_SELECT", "CheckButton" ) 
       ; f( CHECKBOX_LIST_LAZY_MULTI_SELECT, 1, "CHECKBOX_LIST_LAZY_MULTI_SELECT", "CheckButton" ) 

      //--- added additional options for menu, not implemented on menu
 ; f( MNU_COPY, 1, "MNU_COPY", "Popup Menu" ) 
 ; f( MNU_PASTE, 1, "MNU_PASTE", "Popup Menu" ) 
		 ; f( MNU_EDIT_CONTROL_COMMON, 1, "MNU_EDIT_CONTROL_COMMON", "Popup Menu" ) 
 ; f( TXT_IMAGE_H_MARGIN, 1, "TXT_IMAGE_H_MARGIN", "EditControl" ) 
 ; f( TXT_IMAGE_V_MARGIN, 1, "TXT_IMAGE_V_MARGIN", "EditControl" ) 
 ;
  f( LABEL_TEXT_COLOR           , 1, "LABEL_TEXT_COLOR", STATIC_TEXT_NAME );
  f(LABEL_BACKGROUND_COLOR     , 1, "LABEL_BACKGROUND_COLOR", STATIC_TEXT_NAME );
  f(LABEL_RING_COLOR           , 1, "LABEL_RING_COLOR", STATIC_TEXT_NAME );
  f(LABEL_RING_HIGHLIGHT_COLOR , 1, "LABEL_RING_HIGHTLIGHT_COLOR", STATIC_TEXT_NAME );
}
