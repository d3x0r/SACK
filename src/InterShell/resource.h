#ifndef InterShell_RESOURCE_ENUMERATION_DEFINED
#define InterShell_RESOURCE_ENUMERATION_DEFINED
/*
 *  Well known resource symbols....
 *
 *
 */


// define sufficiently high symbol number









enum resource_enum {


 FIRST_SYMBOL = 1000, MNU_CREATE_CONTROL = 1000 
 , MNU_EDIT_CONTROL 
 , MNU_DESTROY_CONTROL 
 , MNU_CREATE_CLOCK 
 , MNU_CREATE_MANAGER 
 , MNU_EXTRA_CONTROL 
 , MNU_CREATE_KEYPAD 
 , MNU_CREATE_ISSUE 
 , MNU_CREATE_USERINFO 
 , MNU_CLONE 
 , MNU_MAKE_CLONE 


 , BTN_PICKFONT 
 , BTN_PICKFONT_PRICE 
 , BTN_PICKFONT_QTY 
 , BTN_EDITFONT 

// start a whole new range
 , MNU_CREATE_EXTRA 
		 , MNU_CREATE_EXTRA_MAX = MNU_CREATE_EXTRA+1000 
		 , MNU_CHANGE_PAGE 
       , MNU_CHANGE_PAGE_MAX = MNU_CHANGE_PAGE+256 
		 , MNU_DESTROY_PAGE 
       , MNU_DESTROY_PAGE_MAX = MNU_DESTROY_PAGE+256 
		 , MNU_UNDELETE_PAGE 
       , MNU_UNDELETE_PAGE_MAX = MNU_UNDELETE_PAGE+256 


 , KEYPAD_KEYS 

//--- Super menu options...
 , MNU_EDIT_SCREEN 


//--- Edit menu options...
 , MNU_ADD_KEYPAD 
 , MNU_EDIT_DONE 
 , MNU_PAGE_PROPERTIES 
 , MNU_CREATE_PAGE 
 , MNU_RENAME_PAGE 


//--- Keypad property menu...
 , MNU_SETFONT 

//--- Button property menu...
//SYMNAME( MNU_SETFONT


 , TXT_CONTROL_TEXT 
//SYMNAME( TXT_TASK_NAME       , EDIT_FIELD_NAME )
//SYMNAME( TXT_TASK_PATH       , EDIT_FIELD_NAME )
//SYMNAME( TXT_TASK_ARGS       , EDIT_FIELD_NAME )
 , CLR_BACKGROUND 
 , CHK_BACK_IMAGE 
 , TXT_IMAGE_NAME 
 , TXT_ANIMATION_NAME 
 , BTN_PICKFILE 
 , BTN_PICKANIMFILE 
 , CLR_TEXT_COLOR 
 , CHK_ROUND 
 , CHK_SQUARE 
 , CHK_NOLENSE 
 , CHK_NOPRESS 
 , CLR_RING_BACKGROUND 
 , LST_PAGES  // listbox for pages 
 , LST_VARIABLES 
		 , LST_FONTS 
		 , LST_SELECT_USER 
		 , EDT_PASSWORD 
       , LST_BUTTON_STYLE 
 , MNU_EDIT_BEHAVIORS 

 , LST_HALL 
 , LST_BUTTON_MODES 
 , LST_ELECTRONIC_ITEMS 
 , LST_REGION_SELECT 
		 , MNU_EDIT_FONTS 
		 , CLR_RING_HIGHLIGHT 

		/* glare set edit resources */

		 , MNU_EDIT_GLARES 
		 , LISTBOX_GLARE_SETS 
       , EDIT_GLARESET_GLARE 
       , EDIT_GLARESET_UP 
       , EDIT_GLARESET_DOWN 
		 , EDIT_GLARESET_MASK 
       , CHECKBOX_GLARESET_MULTISHADE 
		 , CHECKBOX_GLARESET_SHADE 
		 , CHECKBOX_GLARESET_FIXED 
		 , GLARESET_APPLY_CHANGES 
		 , GLARESET_CREATE 

		/* menu set for global property plugins - auto loaded tasks, wait for caller flags, etc...
         first module to use this is tasks.isp */
		 , MNU_GLOBAL_PROPERTIES 
       , MNU_GLOBAL_PROPERTIES_MAX = MNU_GLOBAL_PROPERTIES+256 
       , CHECKBOX_LABEL_CENTER 
       , CHECKBOX_LABEL_RIGHT 
       , CHECKBOX_LIST_MULTI_SELECT 
       , CHECKBOX_LIST_LAZY_MULTI_SELECT 

      //--- added additional options for menu, not implemented on menu
 , MNU_COPY 
 , MNU_PASTE 
		 , MNU_EDIT_CONTROL_COMMON 

		// these are registered with a proper domain
      // do not auto register, but do enumerate here...
		 , LIST_ALLOW_SHOW 
		 , LIST_DISALLOW_SHOW 
		 , LIST_SYSTEMS  // known systems list 
		 , EDIT_SYSTEM_NAME 
       , BTN_ADD_SYSTEM 
       , BTN_ADD_SYSTEM_TO_DISALLOW 
       , BTN_ADD_SYSTEM_TO_ALLOW 
       , BTN_REMOVE_SYSTEM_FROM_DISALLOW 
       , BTN_REMOVE_SYSTEM_FROM_ALLOW 
       , EDIT_PAGE_GRID_PARTS_X 
		 , EDIT_PAGE_GRID_PARTS_Y 
		 , LISTBOX_SECURITY_MODULE 
       , EDIT_SECURITY 

 , TXT_IMAGE_H_MARGIN
 , TXT_IMAGE_V_MARGIN
 
						 , CHECKBOX_LABEL_SCROLL
                   , CHECKBOX_LABEL_SHADOW
						 , LISTBOX_GLARE_SET_THEME
						 , GLARESET_ADD_THEME
						 , BTN_ADD_PAGE_THEME
                   , LISTBOX_PAGE_THEME
	  , LABEL_TEXT_COLOR
	  , LABEL_BACKGROUND_COLOR
	  , LABEL_RING_COLOR
	  , LABEL_RING_HIGHLIGHT_COLOR
};








#endif
