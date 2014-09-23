/* Defines the interface InterShell exports for plugin modules
   to use.
   
   
   
   
   Note
   When using the intershell interface in a project, each file
   must define USES_INTERSHELL_INTERFACE, and a single file must
   define DEFINES_INTERSHELL_INTERFACE.
   
   <code lang="c++">
   
   // only define this in a single source
   \#define DEFINES_INTERSHELL_INTERFACE
   // define this for each source that uses methods in InterShell.
   \#define USES_INTERSHELL_INTERFACE
   \#include \<InterShell_export.h\>
   </code>
   
   
   
                                                                   */
#define DEPRICATE_USE_A_FONT
#ifndef InterShell_EXPORT
/* One-short inclusion protection symbol for this file. */
#define InterShell_EXPORT
#include <sack_types.h>
#ifdef SACK_CORE_BUILD
#ifndef INTERSHELL_CORE_BUILD
#define INTERSHELL_CORE_BUILD
#endif
#endif
#ifdef SACK_CORE_BUILD
#include <../src/genx/genx.h>
#else
#include <genx/genx.h>
#endif
#ifdef INTERSHELL_CORE_BUILD
#include "widgets/include/buttons.h"
#else
#include "InterShell/widgets/buttons.h"
#endif
#include <configscript.h>

//DOM-IGNORE-BEGIN

/* Used to declare a function in the interface table. */
#define INTERSHELL_PROC_PTR(type,name)  type (CPROC* name)

#ifdef INTERSHELL_SOURCE
#define INTERSHELL_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define INTERSHELL_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

//DOM-IGNORE-END

#ifdef __cplusplus
/* A macro used to open the InterShell namespace. */
#define INTERSHELL_NAMESPACE SACK_NAMESPACE namespace intershell {
/* A macro used to close the InterShell namespace */
#define INTERSHELL_NAMESPACE_END  } SACK_NAMESPACE_END


namespace sack {
	/* \ \ 
	   Example
	   A Simple Button
	   
	   This simple button, when clicked will show a message box
	   indicating that the button was clicked. For button controls
	   there is a default configuration dialog that is used for
	   controls that do not themselves define a
	   OnConfigureControl(OnEditControl) event handler. The default
	   dialog, depending on its current design is able to set all
	   relavent properties common to buttons, such as colors, font,
	   button style, perhaps a page change indicator.
	   
	   
	   
	   <code lang="c++">
	   
	   
	   OnCreateMenuButton( “basic/Hello World” )( PMENU_BUTTON button )
	   {
	       return 1; // result OK to create.
	   }
	   
	   OnKeyPressEvent( “basic/Hello World” )( PTRSZVAL psvUnused )
	   {
	       SimpleMessageBox( NULL  // the parent frame, NULL is okay
	                       , “Hello World!”   // the message within the message box
	                       , “Button Clicked” );  // the title of the message box.
	       // SimpleMessageBox displays a simple frame with a message
	       // and a title, and an OK button. The function waits
	       // until the OK button is clicked before returning.
	   }
	   
	   
	   </code>
	   
	   A Simple Listbox
	   
	   <code lang="c++">
	   OnCreateListbox( “basic/List Test” )( PSI_CONTROL pc_list )
	   {
	         return pc_list; // result non-zero OK to create.
	         // this result can also be used in subsequent events
	   // by typecasting it back to the original PSI_CONTROL
	   // type that it is.
	   }
	   
	   
	   
	   </code>
	   // several implementations of listboxes change their content
	   
	   based on the state of other controls around them, and/or
	   
	   database content. The OnShowControl is a convenient place
	   
	   to re-populate the listbox with new data.
	   
	   There is no requirement to do this in this way.
	   
	   Some listboxes may populate their content during OnCreate.
	   
	   <code lang="c++">
	   
	   OnShowControl( “basic/List Test” )( PTRSZVAL psvList )
	   {
	         PSI_CONTROL pc_list = (PSI_CONTROL)psvList;
	         ResetList( pc_list );
	         AddListItem( pc_list, “One List Item” );
	         AddListItem( pc_list, “Another List Item” );
	   }
	   
	   
	   
	   
	   
	   
	   
	   
	   
	   A Simple Control
	   
	   
	   
	   There is no such thing as a ‘simple’ control.
	   
	   
	   
	   OnCreateControl( “basic/Other Control” )( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
	   
	   {
	         // this code results with a create PSI control.
	         // the PSI control must have been previously registered
	         // or defined in some way.
	         // the result of creating an invalid control,
	   // or of creating a control that fails creation
	   // for one reason or another is NULL, which will in turn
	   // fails creation of the InterShell control.
	   
	         return (PTRSZVAL)MakeNamedControl( frame
	                 , “Some PSI Control type-name”
	                 , x, y  // control position passed to event
	   </code>
	   <code>
	                 , w, h  // control size passed to event
	                 , \-1 ); // control ID; typically unused.
	   </code>
	   <code lang="c++">
	   }
	   
	   
	   
	   // For InterShell to be able to hide the control when pages change,
	   // show the control when pages change, move the control to
	   // a new position, or to resize the control, this method MUST
	   // be defined for InterShell widgets created through OnCreateControl.
	   
	   
	   
	   OnQueryGetControl( PTRSZVAL psv )
	   {
	         // since we know that we returned a PSI_CONTROL from the
	         // creation event, this can simply be typecast and returned.
	         return (PSI_CONTROL)psv;
	   }
	   </code>                                                                                     */
	namespace intershell {
#else
#define INTERSHELL_NAMESPACE
#define INTERSHELL_NAMESPACE_END
#endif



#ifndef MENU_BUTTON_DEFINED
#define MENU_BUTTON_DEFINED
typedef struct menu_button *PMENU_BUTTON;
#endif
/* An abstract type used to point to a Page. A canvas manages a
   list of pages that will show at any given time one of.       */
typedef struct page_data   *PPAGE_DATA;


/* label is actually just a text label thing... It's only known
   about internally, so it's better to use the function for
   translating label text which takes your menu button control
   container.                                                   */
typedef struct page_label      *PPAGE_LABEL;
/* A named font preset. The font may be extracted from this at
   any given time, but internally handles scaling of the font to
   match the one-size fits all.                                  */
typedef struct font_preset     *PFONT_PRESET;


//-------------------------------------------------------
//   Dynamic variable declaration - used within button/text_label contexts...
//-------------------------------------------------------
//
// PVARIABLE types are used in Other/Text Label
//   a %&lt;varname&gt; matches the name exactly
//   a revision needs to be done to offer a different variable sepearateor that can
//   imply sub function... suppose a simple xml stealing of &lt;varname&gt; will work
//   implying that optional paramters may be &lt;function param1=value1 ...&gt;
// CreateLabelVariable returns a value that may be used in...
// LabelVariableChanged
//  to cause the update of all boxes which may or may not be visible at the time, and
//  will cause approrpiate refresh
//

typedef struct variable_tag *PVARIABLE;

/* moved from text_label.h these values need to be exposed to
   peer modules
   
   
   
   This enumeration defines values used in CreateLabelVariable
   and CreateLabelVariableEx.                                  */
enum label_variable_types {
   /* POINTER data should be the address of a CTEXTSTR (a pointer
      to the pointer of a string )                                */
	LABEL_TYPE_STRING,
      /* POINTER data should be the address of an integer, changing the integer will reflect in the output*/
	LABEL_TYPE_INT,
   /* POINTER data is the address of a routine which takes (void) parameters and returns a CTEXTSTR*/
	LABEL_TYPE_PROC,
	/* POINTER data is the address of a routine which takes a (PTRSZVAL) and returns a CTEXTSTR */
   /* PTRSZVAL psv is user data to pass to the procedure */
	LABEL_TYPE_PROC_EX,

	/* POINTER data is the address of a routine which takes a (PTRSZVAL) and returns a CTEXTSTR */
	/* PTRSZVAL psv is user data to pass to the procedure */
   /* routine also gets the control the text is contained on? */
	LABEL_TYPE_PROC_CONTROL_EX,
   /* POINTER data is a pointer to a simple string, the value is copied and used on the control */
	LABEL_TYPE_CONST_STRING,
	/* POINTER data is a pointer to a function to call that returns an index.  After the variable name :option1,option2,option3,; can be specified.  User data is passed to proc also.  The option list is ended with ,;
	 */
	LABEL_TYPE_VALUE_STRING,
	/* POINTER data is the address of a routine that takes a S_64 integer type; the integer is
    specified in the text of the label.  */
	LABEL_TYPE_PROC_PARAMETER,
};
/* This is the type of the variable expected if a label is
   created with LABEL_TYPE_STRING.
   
   
   See Also
   CreateLabelVariable
   
   CreateLabelVariableEx                                   */
typedef CTEXTSTR  *label_string_value;
/* This is the type of the variable expected if a label is
   created with LABEL_TYPE_INT.
   See Also
   CreateLabelVariable
   
   CreateLabelVariableEx                                   */
typedef _32       *label_int_value;
/* This is the type of the variable expected if a label is
   created with LABEL_TYPE_PROC.
   
   
   See Also
   CreateLabelVariable
   
   CreateLabelVariableEx                                   */
typedef CTEXTSTR (*label_gettextproc)(void);
/* This is the type of the variable expected if a label is
   created with LABEL_TYPE_PROC_EX.
   See Also
   CreateLabelVariable
   
   CreateLabelVariableEx                                   */
typedef CTEXTSTR (*label_gettextproc_ex)(PTRSZVAL);
/* This is the type of the variable expected if a label is
   created with LABEL_TYPE_PROC_CONTROL_EX.
   
   
   See Also
   CreateLabelVariable
   
   CreateLabelVariableEx                                   */
typedef CTEXTSTR (*label_gettextproc_control)(PTRSZVAL, PMENU_BUTTON);
/* This is the type of the variable expected if a label is
   created with LABEL_TYPE_PROC_PARAMETER.
   
   
   See Also
   CreateLabelVariable
   
   CreateLabelVariableEx                                   */
typedef CTEXTSTR (*label_value_proc_parameter)( S_64 value );
   /* this is the type of function to pass LABEL_TYPE_VALUE_STRING */
typedef int (*label_value_proc)(PTRSZVAL, PMENU_BUTTON);

struct intershell_interface {


/* <combine sack::intershell::GetCommonButtonControls@PSI_CONTROL>
   
   \ \                                                             */
INTERSHELL_PROC_PTR( void, GetCommonButtonControls )( PSI_CONTROL frame );

INTERSHELL_PROC_PTR( void, SetCommonButtonControls )( PSI_CONTROL frame );


INTERSHELL_PROC_PTR( void, RestartMenu )( PTRSZVAL psv, _32 keycode );
INTERSHELL_PROC_PTR( void, ResumeMenu )( PTRSZVAL psv, _32 keycode );


// a zero (0) passed as a primary/secondary or tertiary color indicates no change. (err or disable)
#define COLOR_DISABLE 0x00010000 // okay transparent level 1 red is disable key. - cause that's such a useful color alone.
#define COLOR_IGNORE  0x00000000
/* <combine sack::intershell::InterShell_GetButtonColors@PMENU_BUTTON@CDATA *@CDATA *@CDATA *@CDATA *>
   
   \ \                                                                                                 */
INTERSHELL_PROC_PTR( void, InterShell_GetButtonColors )( PMENU_BUTTON button
													, CDATA *cText
													, CDATA *cBackground1
													, CDATA *ring_color
													, CDATA *highlight_ring_color );
INTERSHELL_PROC_PTR( void, InterShell_SetButtonColors )( PMENU_BUTTON button, CDATA cText, CDATA cBackground1, CDATA cBackground2, CDATA cBackground3 );
INTERSHELL_PROC_PTR( void, InterShell_SetButtonColor )( PMENU_BUTTON button, CDATA primary, CDATA secondary );
INTERSHELL_PROC_PTR( void, InterShell_SetButtonText )( PMENU_BUTTON button, CTEXTSTR text );
/* <combine sack::intershell::InterShell_GetButtonText@PMENU_BUTTON@TEXTSTR@int>
   
   \ \                                                                           */
INTERSHELL_PROC_PTR( void, InterShell_GetButtonText )( PMENU_BUTTON button, TEXTSTR text, int text_buf_len );
INTERSHELL_PROC_PTR( void, InterShell_SetButtonImage )( PMENU_BUTTON button, CTEXTSTR name );
#ifndef __NO_ANIMATION__

#endif
/* <combine sack::intershell::InterShell_CommonImageLoad@CTEXTSTR>
   
   \ \                                                             */
INTERSHELL_PROC_PTR( Image, InterShell_CommonImageLoad )( CTEXTSTR name );
/* <combine sack::intershell::InterShell_CommonImageUnloadByName@CTEXTSTR>
   
   \ \                                                                     */
INTERSHELL_PROC_PTR( void, InterShell_CommonImageUnloadByName )( CTEXTSTR name );
/* <combine sack::intershell::InterShell_CommonImageUnloadByImage@Image>
   
   \ \                                                                   */
INTERSHELL_PROC_PTR( void, InterShell_CommonImageUnloadByImage )( Image unload );

INTERSHELL_PROC_PTR( void, InterShell_SetButtonImageAlpha )( PMENU_BUTTON button, S_16 alpha );



INTERSHELL_PROC_PTR( LOGICAL, InterShell_IsButtonVirtual )( PMENU_BUTTON button );

INTERSHELL_PROC_PTR( void, InterShell_SetButtonFont )( PMENU_BUTTON button, SFTFont *font );

INTERSHELL_PROC_PTR( SFTFont*, InterShell_GetCurrentButtonFont )( void );
INTERSHELL_PROC_PTR( void, InterShell_SetButtonStyle )( PMENU_BUTTON button, TEXTCHAR *style );
INTERSHELL_PROC_PTR( void, InterShell_SaveCommonButtonParameters )( FILE *file );
INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetSystemName )( void );

INTERSHELL_PROC_PTR( void, UpdateButtonExx )( PMENU_BUTTON button, int bEndingEdit DBG_PASS );
#define UpdateButtonEx( button, edit ) UpdateButtonExx( button, edit DBG_SRC )
//#define UpdateButtonEx( button, edit ) UpdateButtonExx( button, edit DBG_SRC )
#define UpdateButton(button) UpdateButtonEx( button, TRUE )

// fixup button has been depricated for external usage
//  please use UpdateButton instead. (which does also invoke fixup)
// UpdateButton
//INTERSHELL_PROC_PTR( void, FixupButtonEx )( PMENU_BUTTON button DBG_PASS);
#define FixupButton(b) FixupButtonEx((b) DBG_SRC)


INTERSHELL_PROC_PTR(PPAGE_DATA, ShellGetCurrentPage)( PSI_CONTROL pc_canvas_or_control_in_canvas);

INTERSHELL_PROC_PTR(PPAGE_DATA, ShellGetNamedPage)( PSI_CONTROL pc, CTEXTSTR pagename );

// Page name can be... 'first', 'next',  'here', 'return'
// otherwise page name can be any name of any other page.
INTERSHELL_PROC_PTR( int, ShellSetCurrentPage )( PSI_CONTROL pc, CTEXTSTR name );

INTERSHELL_PROC_PTR( int, ShellCallSetCurrentPage )( PSI_CONTROL pc_canvas, CTEXTSTR name );

INTERSHELL_PROC_PTR( void, ShellReturnCurrentPage )( PSI_CONTROL pc_canvas );
/* <combine sack::intershell::ClearPageList>
   
   \ \                                       */
INTERSHELL_PROC_PTR( void, ClearPageList )( PSI_CONTROL pc_canvas );

/* <combine sack::intershell::InterShell_DisablePageUpdate@LOGICAL>
   
   \ \                                                              */
INTERSHELL_PROC_PTR( void, InterShell_DisablePageUpdate )( PSI_CONTROL pc_canvas, LOGICAL bDisable );
INTERSHELL_PROC_PTR( void, RestoreCurrentPage )( PSI_CONTROL pc_canvas );
/* <combine sack::intershell::HidePageExx@PSI_CONTROL pc_canvas>
   
   \ \                                                           */
INTERSHELL_PROC_PTR( void, HidePageExx )( PSI_CONTROL pc_canvas DBG_PASS);
#define HidePageEx2(page) HidePageExx( page DBG_SRC )




/* <combine sack::intershell::InterShell_DisableButtonPageChange@PMENU_BUTTON>
   
   \ \                                                                         */
INTERSHELL_PROC_PTR( void, InterShell_DisableButtonPageChange )( PMENU_BUTTON button );

/* <combine sack::intershell::CreateLabelVariable@CTEXTSTR@enum label_variable_types@CPOINTER>
   
   \ \                                                                                         */
INTERSHELL_PROC_PTR( PVARIABLE, CreateLabelVariable )( CTEXTSTR name, enum label_variable_types type, CPOINTER data );
/* <combine sack::intershell::CreateLabelVariableEx@CTEXTSTR@enum label_variable_types@CPOINTER@PTRSZVAL>
   
   \ \                                                                                                    */
INTERSHELL_PROC_PTR( PVARIABLE, CreateLabelVariableEx )( CTEXTSTR name, enum label_variable_types type, CPOINTER data, PTRSZVAL psv );
INTERSHELL_PROC_PTR( void, LabelVariableChanged )( PVARIABLE ); 
INTERSHELL_PROC_PTR( void, LabelVariablesChanged )( PLIST ); 


INTERSHELL_PROC_PTR( void, InterShell_HideEx )( PSI_CONTROL pc_canvas DBG_PASS );
#define InterShell_Hide(c)   InterShell_HideEx( c DBG_SRC )
INTERSHELL_PROC_PTR( void, InterShell_RevealEx )( PSI_CONTROL pc_canvas DBG_PASS );

#define InterShell_Reveal( c )  InterShell_RevealEx( c DBG_SRC )

/* <combine sack::intershell::GetPageSize@P_32@P_32>
   
   \ \                                               */
INTERSHELL_PROC_PTR( void, GetPageSize )( P_32 width, P_32 height );


INTERSHELL_PROC_PTR( void, SetButtonTextField )( PMENU_BUTTON pKey, PTEXT_PLACEMENT pField, TEXTCHAR *text );
/* <combine sack::intershell::AddButtonLayout@PMENU_BUTTON@int@int@SFTFont *@CDATA@_32>
   
   \ \                                                                               */
INTERSHELL_PROC_PTR( PTEXT_PLACEMENT, AddButtonLayout )( PMENU_BUTTON pKey, int x, int y, SFTFont *font, CDATA color, _32 flags );



/* <combine sack::intershell::InterShell_GetButtonControl@PMENU_BUTTON>
   
   \ \                                                                  */
INTERSHELL_PROC_PTR( PSI_CONTROL, InterShell_GetButtonControl )( PMENU_BUTTON button );


INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetLabelText )( PPAGE_LABEL label, CTEXTSTR variable );

INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_TranslateLabelText )( PPAGE_LABEL label, TEXTSTR output, int buffer_len, CTEXTSTR variable );


/* <combine sack::intershell::InterShell_GetControlLabelText@PMENU_BUTTON@PPAGE_LABEL@CTEXTSTR>
   
   \ \                                                                                          */
INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetControlLabelText )( PMENU_BUTTON button, PPAGE_LABEL label, CTEXTSTR variable );

INTERSHELL_PROC_PTR( SFTFont *, SelectACanvasFont )( PSI_CONTROL canvas, PSI_CONTROL parent, CTEXTSTR *default_name );


/* <combine sack::intershell::BeginCanvasConfiguration@PSI_CONTROL>
   
   \ \                                                              */
INTERSHELL_PROC_PTR( void, BeginCanvasConfiguration )( PSI_CONTROL pc_canvas );
INTERSHELL_PROC_PTR( void, SaveCanvasConfiguration )( FILE *file, PSI_CONTROL pc_canvas );
INTERSHELL_PROC_PTR( void, SaveCanvasConfiguration_XML )( genxWriter w, PSI_CONTROL pc_canvas );
INTERSHELL_PROC_PTR( PCONFIG_HANDLER, InterShell_GetCurrentConfigHandler )( void );




/* <combine sack::intershell::BeginSubConfiguration@TEXTCHAR *@TEXTCHAR *>
   
   \ \                                                                     */
INTERSHELL_PROC_PTR( LOGICAL, BeginSubConfiguration )( TEXTCHAR *control_type_name, const TEXTCHAR *end_type_name );
/* <combine sack::intershell::EscapeMenuString@CTEXTSTR>
   
   \ \                                                   */
INTERSHELL_PROC_PTR( CTEXTSTR, EscapeMenuString )( CTEXTSTR string );
INTERSHELL_PROC_PTR( PMENU_BUTTON, InterShell_GetCurrentLoadingControl )( void );



INTERSHELL_PROC_PTR( SFTFont*, InterShell_GetButtonFont )( PMENU_BUTTON pc );
INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetButtonFontName )( PMENU_BUTTON pc );
INTERSHELL_PROC_PTR( PMENU_BUTTON, InterShell_GetCurrentButton )( void );
INTERSHELL_PROC_PTR( void, InterShell_SetButtonFontName )( PMENU_BUTTON button, CTEXTSTR name );


INTERSHELL_PROC_PTR( PMENU_BUTTON, InterShell_GetPhysicalButton )( PMENU_BUTTON button );

INTERSHELL_PROC_PTR( void, InterShell_SetButtonHighlight )( PMENU_BUTTON button, LOGICAL bEnable );
/* <combine sack::intershell::InterShell_GetButtonHighlight@PMENU_BUTTON>
   
   \ \                                                                    */
INTERSHELL_PROC_PTR( LOGICAL, InterShell_GetButtonHighlight )( PMENU_BUTTON button );

INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_TranslateLabelTextEx )( PMENU_BUTTON button, PPAGE_LABEL label, TEXTSTR output, int buffer_len, CTEXTSTR variable );

/* <combine sack::intershell::InterShell_CreateControl@CTEXTSTR@int@int@int@int>
   
   \ \                                                                           */
INTERSHELL_PROC_PTR( PTRSZVAL,  InterShell_CreateControl )( PSI_CONTROL canvas, CTEXTSTR type, int x, int y, int w, int h );


INTERSHELL_PROC_PTR( void, InterShell_CreateNamedPage )( PSI_CONTROL canvas, CTEXTSTR page_name );

INTERSHELL_PROC_PTR( void, InterShell_AddCommonButtonConfig )( PCONFIG_HANDLER pch );

INTERSHELL_PROC_PTR( PSI_CONTROL, InterShell_GetCanvas )( PPAGE_DATA page );
INTERSHELL_PROC_PTR( void, InterShell_SetPageLayout )( PSI_CONTROL canvas, _32 cols, _32 rows );  // width/height, x/y

INTERSHELL_PROC_PTR( PMENU_BUTTON, InterShell_CreateSomeControl )( PSI_CONTROL pc_canvas, int x, int y, int w, int h
							   , CTEXTSTR name );
INTERSHELL_PROC_PTR( PMENU_BUTTON, InterShell_GetCurrentlyCreatingButton )( void );
INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetSaveIndent )( void );
INTERSHELL_PROC_PTR( LOGICAL, BeginSubConfigurationEx )( PMENU_BUTTON button, TEXTCHAR *control_type_name, const TEXTCHAR *end_type_name );

INTERSHELL_PROC_PTR( void, InterShell_SetTheme )( PSI_CONTROL pc_canvas, int ID );
INTERSHELL_PROC_PTR( void, DisplayMenuCanvas )( PSI_CONTROL pc_canvas, PRENDERER under, _32 width, _32 height, S_32 x, S_32 y );
INTERSHELL_PROC_PTR( void, InterShell_SetPageColor )( PPAGE_DATA page, CDATA color );

INTERSHELL_PROC_PTR( PTRSZVAL, InterShell_GetButtonUserData )( PMENU_BUTTON button );

// this can be used to get the default canvas by passing NULL for the button.
INTERSHELL_PROC_PTR( PSI_CONTROL, InterShell_GetButtonCanvas )( PMENU_BUTTON button );

INTERSHELL_PROC_PTR( SFTFont *, UseACanvasFont )( PSI_CONTROL pc_canvas, CTEXTSTR name );

INTERSHELL_PROC_PTR( PTRSZVAL, InterShell_GetButtonExtension )( PMENU_BUTTON button );

INTERSHELL_PROC_PTR( void, InterShell_SetTextLabelOptions )( PMENU_BUTTON label, LOGICAL center, LOGICAL right, LOGICAL scroll, LOGICAL shadow );

INTERSHELL_PROC_PTR( PSI_CONTROL, InterShell_GetCurrentLoadingCanvas )( void );
INTERSHELL_PROC_PTR( PSI_CONTROL, InterShell_GetCurrentSavingCanvas )( void );
INTERSHELL_PROC_PTR( SFTFont *, CreateACanvasFont )( PSI_CONTROL pc_canvas, CTEXTSTR name, SFTFont font, POINTER data, size_t datalen );

INTERSHELL_PROC_PTR( void, SetupSecurityEdit )( PSI_CONTROL frame, PTRSZVAL object_to_secure );
INTERSHELL_PROC_PTR( PTRSZVAL, CreateSecurityContext )( PTRSZVAL object );
INTERSHELL_PROC_PTR( void, CloseSecurityContext )( PTRSZVAL button, PTRSZVAL psv_context_to_Destroy );
INTERSHELL_PROC_PTR( void, InterShell_SaveSecurityInformation )( FILE *file, PTRSZVAL psv );

INTERSHELL_PROC_PTR( SFTFont *, CreateACanvasFont2 )( PSI_CONTROL pc_canvas, CTEXTSTR name, CTEXTSTR fontfilename, int size_x, int size_y );


INTERSHELL_PROC_PTR( void, AddSecurityContextToken )( PTRSZVAL object, CTEXTSTR module, CTEXTSTR token );
INTERSHELL_PROC_PTR( void, GetSecurityContextTokens )( PTRSZVAL object, CTEXTSTR module, PLIST *list );
INTERSHELL_PROC_PTR( void, GetSecurityModules )( PLIST *list );
//INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetSaveIndent1 )( void ); // returns one level more than here
INTERSHELL_PROC_PTR( void, InterShell_SetCloneButton )( PMENU_BUTTON button );
INTERSHELL_PROC_PTR( CTEXTSTR, InterShell_GetCurrentPageName )( PSI_CONTROL canvas );

};  //struct intershell_interface {


#ifdef INTERSHELL_SOURCE
// magically knows which button we're editing at the moment.
// intended to only be used during OnEditControl() Event Handler
INTERSHELL_PROC( void, GetCommonButtonControls )( PSI_CONTROL frame );
// magically knows which button we're editing at the moment.
// intended to only be used during OnEditControl() Event Handler
INTERSHELL_PROC( void, SetCommonButtonControls )( PSI_CONTROL frame );
INTERSHELL_PROC( PMENU_BUTTON, InterShell_GetCurrentlyCreatingButton )( void );

// wake up menu processing... there's a flag that was restart that this thinks
// it might want...
INTERSHELL_PROC( void, RestartMenu )( PTRSZVAL psv, _32 keycode );
INTERSHELL_PROC( void, ResumeMenu )( PTRSZVAL psv, _32 keycode );


// a zero (0) passed as a primary/secondary or tertiary color indicates no change. (err or disable)
#define COLOR_DISABLE 0x00010000 // okay transparent level 1 red is disable key. - cause that's such a useful color alone.
/* A Special color symbol used to pass to InterShell_
   SetButtonColors which causes the color to remain whatever it
   was, ignore any change.                                      */
#define COLOR_IGNORE  0x00000000
INTERSHELL_PROC( void, InterShell_GetButtonColors )( PMENU_BUTTON button
													, CDATA *cText
													, CDATA *cBackground1
													, CDATA *ring_color
													, CDATA *highlight_ring_color );
INTERSHELL_PROC( void, InterShell_SetButtonColors )( PMENU_BUTTON button, CDATA cText, CDATA cBackground1, CDATA cBackground2, CDATA cBackground3 );
INTERSHELL_PROC( void, InterShell_SetButtonColor )( PMENU_BUTTON button, CDATA primary, CDATA secondary );
INTERSHELL_PROC( void, InterShell_SetButtonText )( PMENU_BUTTON button, CTEXTSTR text );
INTERSHELL_PROC( void, InterShell_GetButtonText )( PMENU_BUTTON button, TEXTSTR text, int text_buf_len );
INTERSHELL_PROC( void, InterShell_SetButtonImage )( PMENU_BUTTON button, CTEXTSTR name );
INTERSHELL_PROC( void, InterShell_SetButtonHighlight )( PMENU_BUTTON button, LOGICAL bEnable );
INTERSHELL_PROC( LOGICAL, InterShell_GetButtonHighlight )( PMENU_BUTTON button );
#ifndef __NO_ANIMATION__
//INTERSHELL_PROC( void, InterShell_SetButtonAnimation )( PMENU_BUTTON button, CTEXTSTR name );
#endif
INTERSHELL_PROC( Image, InterShell_CommonImageLoad )( CTEXTSTR name );
INTERSHELL_PROC( void, InterShell_CommonImageUnloadByName )( CTEXTSTR name );
INTERSHELL_PROC( void, InterShell_CommonImageUnloadByImage )( Image unload );
/*
 *  For InterShell_SetButtonImageAlpha.....
 *    0 is no alpha change.
 *    alpha < 0 increases transparency
 *    alpha > 0 increases opacity.
 *    Max value is +/-255
 */
INTERSHELL_PROC( void, InterShell_SetButtonImageAlpha )( PMENU_BUTTON button, S_16 alpha );


// return if the button is just virtual (part of a macro)
INTERSHELL_PROC( LOGICAL, InterShell_IsButtonVirtual )( PMENU_BUTTON button );

INTERSHELL_PROC( PMENU_BUTTON, InterShell_GetPhysicalButton )( PMENU_BUTTON button );

INTERSHELL_PROC( void, InterShell_SetButtonFont )( PMENU_BUTTON button, SFTFont *font );
// THis function returns the font of the current button being edited...
// this result can be used for controls that are not really buttons to get the common
// properties of the font being used for this control...
INTERSHELL_PROC( void, InterShell_SetButtonStyle )( PMENU_BUTTON button, TEXTCHAR *style );
INTERSHELL_PROC( void, InterShell_SaveCommonButtonParameters )( FILE *file );
INTERSHELL_PROC( void, InterShell_AddCommonButtonConfig )( PCONFIG_HANDLER pch );
INTERSHELL_PROC( CTEXTSTR, InterShell_GetSystemName )( void );
//INTERSHELL_PROC( void, UpdateButtonEx )( PMENU_BUTTON button, int bEndingEdit );
INTERSHELL_PROC( void, UpdateButtonExx )( PMENU_BUTTON button, int bEndingEdit DBG_PASS );
#define UpdateButtonEx( button, edit ) UpdateButtonExx( button, edit DBG_SRC )
//#define UpdateButtonEx( button, edit ) UpdateButtonExx( button, edit DBG_SRC )
#define UpdateButton(button) UpdateButtonEx( button, TRUE )

// fixup button has been depricated for external usage
//  please use UpdateButton instead. (which does also invoke fixup)
// UpdateButton
//INTERSHELL_PROC( void, FixupButtonEx )( PMENU_BUTTON button DBG_PASS);
#define FixupButton(b) FixupButtonEx((b) DBG_SRC)

//---------------------------------------
// pages controls here...
//
INTERSHELL_PROC(PPAGE_DATA, ShellGetCurrentPage)( PSI_CONTROL pc_canvas_or_control_in_canvas);
// pass pc NULL defaults internally to using the main frame surface.  The page
// name returns the current page of that name.
INTERSHELL_PROC(PPAGE_DATA, ShellGetNamedPage)( PSI_CONTROL pc, CTEXTSTR pagename );
// special names
// start, next, prior are keywords that imply direct
// page stacking.
INTERSHELL_PROC( int, ShellSetCurrentPage )( PSI_CONTROL pc, CTEXTSTR name );

// a call will push the current page on a stack
// which will be returned to if returncurrentpage is used.
// clear page list will flush the stack cause
// there is such a temptation to call to all pages, providing
// near inifinite page recall (back back back)
INTERSHELL_PROC( int, ShellCallSetCurrentPage )( PSI_CONTROL pc_canvas, CTEXTSTR name );

INTERSHELL_PROC( void, ShellReturnCurrentPage )( PSI_CONTROL pc_canvas );
INTERSHELL_PROC( void, ClearPageList )( PSI_CONTROL pc_canvas );
// disable updates on the page, disable updating of buttons...
INTERSHELL_PROC( void, InterShell_DisablePageUpdate )( PSI_CONTROL pc_canvas, LOGICAL bDisable );
INTERSHELL_PROC( void, RestoreCurrentPage )( PSI_CONTROL pc_canvas );
INTERSHELL_PROC( void, HidePageExx )( PSI_CONTROL pc_canvas DBG_PASS);



//---------------------------------------
// this sets a one time flag on a button which disables
// the auto page change which may be assigned to a button.
//   tend/issue/perform these types of verbs may fail, and this is the only
//   sort of thing at the moment that happens... perhaps renaming this to
// button_abort_function could be done?
INTERSHELL_PROC( void, InterShell_DisableButtonPageChange )( PMENU_BUTTON button );

INTERSHELL_PROC( PVARIABLE, CreateLabelVariable )( CTEXTSTR name, enum label_variable_types type, CPOINTER data );
INTERSHELL_PROC( PVARIABLE, CreateLabelVariableEx )( CTEXTSTR name, enum label_variable_types type, CPOINTER data, PTRSZVAL psv );
// pass NULL to update all labels, otherwise, one can pass the result of a CreateLableVariable
// to update only text labels using that variable.
INTERSHELL_PROC( void, LabelVariableChanged )( PVARIABLE ); // update any labels which are using this variable.
INTERSHELL_PROC( void, LabelVariablesChanged )( PLIST ); // update any labels which are using this variable. list of PVARIABLE types

// local export to allow exxternal plugins to control whether the main display is showing...
//  (specially for Tasks at this time... when an exclusive task is launched, display is hidden)
INTERSHELL_PROC( void, InterShell_HideEx )( PSI_CONTROL canvas DBG_PASS );
INTERSHELL_PROC( void, InterShell_RevealEx )( PSI_CONTROL canvas DBG_PASS );


//----------------------------------------------------------
//
INTERSHELL_PROC( void, GetPageSize )( P_32 width, P_32 height );

//-----------------------------------------------------
// layout members which have a position x, y, font, text and color of their own
// may be created on buttons.  They are displayed below the lense/ridge[up/down] and above the background.
INTERSHELL_PROC( void, SetButtonTextField )( PMENU_BUTTON pKey, PTEXT_PLACEMENT pField, TEXTCHAR *text );
INTERSHELL_PROC( PTEXT_PLACEMENT, AddButtonLayout )( PMENU_BUTTON pKey, int x, int y, SFTFont *font, CDATA color, _32 flags );


//-----------------------------------------------------
// this provides low level access to a button, let the programmer beware.
INTERSHELL_PROC( PSI_CONTROL, InterShell_GetButtonControl )( PMENU_BUTTON button );
INTERSHELL_PROC( PSI_CONTROL, InterShell_GetButtonCanvas )( PMENU_BUTTON button );

//---------------------------------------------------
// text_label.h
INTERSHELL_PROC( CTEXTSTR, InterShell_GetLabelText )( PPAGE_LABEL label, CTEXTSTR variable );
// use of this is preferred, otherwise thread conflicts will destroy the buffer.
INTERSHELL_PROC( CTEXTSTR, InterShell_TranslateLabelText )( PPAGE_LABEL label, TEXTSTR output, int buffer_len, CTEXTSTR variable );
INTERSHELL_PROC( CTEXTSTR, InterShell_TranslateLabelTextEx )( PMENU_BUTTON button, PPAGE_LABEL label, TEXTSTR output, int buffer_len, CTEXTSTR variable );

/* actual worker function for InterShell_GetLabelText - but suport dispatch to bProcControlEx*/
INTERSHELL_PROC( CTEXTSTR, InterShell_GetControlLabelText )( PMENU_BUTTON button, PPAGE_LABEL label, CTEXTSTR variable );

//  ---- FONTS ------
INTERSHELL_PROC( SFTFont *, SelectACanvasFont )( PSI_CONTROL canvas, PSI_CONTROL parent, CTEXTSTR *default_name );
INTERSHELL_PROC( SFTFont *, UseACanvasFont )( PSI_CONTROL pc_canvas, CTEXTSTR name );
INTERSHELL_PROC( SFTFont *, CreateACanvasFont )( PSI_CONTROL pc_canvas, CTEXTSTR name, SFTFont font, POINTER data, size_t datalen );
INTERSHELL_PROC( SFTFont *, CreateACanvasFont2 )( PSI_CONTROL pc_canvas, CTEXTSTR name, CTEXTSTR fontfilename, int sizex, int sizey );;

// depricated - used for forward migration...
INTERSHELL_PROC( SFTFont *, CreateAFont )( CTEXTSTR name, SFTFont font, POINTER data, size_t datalen );

// ----- LOAD SAVE Stuff--------------------------------------
INTERSHELL_PROC( void, BeginCanvasConfiguration )( PSI_CONTROL pc_canvas );
INTERSHELL_PROC( void, SaveCanvasConfiguration )( FILE *file, PSI_CONTROL pc_canvas );
INTERSHELL_PROC( void, SaveCanvasConfiguration_XML )( genxWriter w, PSI_CONTROL pc_canvas );
INTERSHELL_PROC( PCONFIG_HANDLER, InterShell_GetCurrentConfigHandler )( void );
INTERSHELL_PROC( PMENU_BUTTON, InterShell_GetCurrentLoadingControl )( void );
INTERSHELL_PROC( PSI_CONTROL, InterShell_GetCurrentLoadingCanvas )( void );
INTERSHELL_PROC( PSI_CONTROL, InterShell_GetCurrentSavingCanvas )( void );

//PTRSZVAL GetButtonExtension( PMENU_BUTTON button );

//void AddCommonButtonConfig( PCONFIG_HANDLER pch, PMENU_BUTTON button );
//void DumpCommonButton( FILE *file, PMENU_BUTTON button );

//PTRSZVAL GetButtonExtension( PMENU_BUTTON button );


// BeginSubConfiguration....
//   colntrol_type_name is a InterShell widget path/name for the type of
//   other info to save... the method for setting additional configuration methods
//   is invoked by thisname.
//   Then end_type_name is the last string which will close the subconfiguration.
INTERSHELL_PROC( LOGICAL, BeginSubConfiguration )( TEXTCHAR *control_type_name, const TEXTCHAR *end_type_name );
INTERSHELL_PROC( LOGICAL, BeginSubConfigurationEx )( PMENU_BUTTON button, TEXTCHAR *control_type_name, const TEXTCHAR *end_type_name );
INTERSHELL_PROC( CTEXTSTR, InterShell_GetSaveIndent )( void );
INTERSHELL_PROC( CTEXTSTR, EscapeMenuString )( CTEXTSTR string );

INTERSHELL_PROC( PTRSZVAL,  InterShell_CreateControl )( PSI_CONTROL canvas, CTEXTSTR type, int x, int y, int w, int h );

// might get a seperate canvas per page if it's opened in multi-frame mode.
// otherwise pass NULL to get the default canvas
INTERSHELL_PROC( PSI_CONTROL, InterShell_GetCanvas )( PPAGE_DATA page );
INTERSHELL_PROC( void, InterShell_SetPageLayout )( PSI_CONTROL canvas, _32 cols, _32 rows );  // width/height, x/y

INTERSHELL_PROC( PMENU_BUTTON, InterShell_CreateSomeControl )( PSI_CONTROL pc_canvas, int x, int y, int w, int h
																				 , CTEXTSTR name );
INTERSHELL_PROC( PTRSZVAL, InterShell_GetButtonExtension )( PMENU_BUTTON button );

INTERSHELL_PROC( void, InterShell_SetTheme )( PSI_CONTROL pc_canvas, int ID );

INTERSHELL_PROC( void, DisplayMenuCanvas )( PSI_CONTROL pc_canvas, PRENDERER under, _32 width, _32 height, S_32 x, S_32 y );
INTERSHELL_PROC( void, InterShell_SetPageColor )( PPAGE_DATA page, CDATA color );

INTERSHELL_PROC( void, InterShell_SetCloneButton )( PMENU_BUTTON button );

#endif

#ifdef USES_INTERSHELL_INTERFACE
#  ifndef DEFINES_INTERSHELL_INTERFACE
extern 
#  endif
	 struct intershell_interface *InterShell
#ifdef __GCC__
	 __attribute__((visibility("hidden")))
#endif
	 ;

#  ifdef DEFINES_INTERSHELL_INTERFACE
// this needs to be done before most modules can run their PRELOADS...so just move this one.
// somehow this ended up as 69 and 69 was also PRELOAD() priority... bad.
PRIORITY_PRELOAD( InitInterShellInterface, DEFAULT_PRELOAD_PRIORITY - 3)
{
	InterShell = (struct intershell_interface*)GetInterface( WIDE("intershell") );
}

#  endif

#endif

#ifndef INTERSHELL_SOURCE
#define InterShell_CreateControl                                ( !InterShell )?0:InterShell->InterShell_CreateControl
#define  GetCommonButtonControls                               if( InterShell )InterShell->GetCommonButtonControls 
#define  SetCommonButtonControls							   if( InterShell )InterShell->SetCommonButtonControls 
#define  InterShell_AddCommonButtonConfig						if( InterShell )InterShell->InterShell_AddCommonButtonConfig
#define  RestartMenu										   if( InterShell )InterShell->RestartMenu 
#define  ResumeMenu											   if( InterShell )InterShell->ResumeMenu

#define  InterShell_SetPageLayout 							   if( InterShell ) InterShell->InterShell_SetPageLayout
#define  InterShell_GetCanvas								   ( !InterShell )?NULL:InterShell->InterShell_GetCanvas
#define  InterShell_CreateSomeControl								   ( !InterShell )?NULL:InterShell->InterShell_CreateSomeControl

#define InterShell_GetCurrentlyCreatingButton                (!InterShell)?NULL:InterShell->InterShell_GetCurrentlyCreatingButton
#define  InterShell_GetButtonColors								   if( InterShell )InterShell->InterShell_GetButtonColors 
#define  InterShell_SetButtonColors								   if( InterShell )InterShell->InterShell_SetButtonColors 
#define  InterShell_SetButtonColor								   if( InterShell )InterShell->InterShell_SetButtonColor 
#define  InterShell_SetButtonText									   if( InterShell )InterShell->InterShell_SetButtonText 
#define  InterShell_GetButtonText									   if( InterShell )InterShell->InterShell_GetButtonText 
#define  InterShell_SetButtonImage								   if( InterShell )InterShell->InterShell_SetButtonImage 
#define  InterShell_SetButtonAnimation							   if( InterShell )InterShell->InterShell_SetButtonAnimation 
#define  InterShell_CommonImageLoad								   if( InterShell )InterShell->InterShell_CommonImageLoad 
#define  InterShell_CommonImageUnloadByName						   if( InterShell )InterShell->InterShell_CommonImageUnloadByName 
#define  InterShell_CommonImageUnloadByImage						   if( InterShell )InterShell->InterShell_CommonImageUnloadByImage 
#define  InterShell_SetButtonImageAlpha							   if( InterShell )InterShell->InterShell_SetButtonImageAlpha 
#define  InterShell_IsButtonVirtual								   ( !InterShell )?FALSE:InterShell->InterShell_IsButtonVirtual
#define  InterShell_SetButtonFont									   if( InterShell )InterShell->InterShell_SetButtonFont 
#define  InterShell_GetCurrentButtonFont							   ( !InterShell )?NULL:InterShell->InterShell_GetCurrentButtonFont
#define  InterShell_SetButtonStyle								   if( InterShell )InterShell->InterShell_SetButtonStyle 
#define  InterShell_SaveCommonButtonParameters					   if( InterShell )InterShell->InterShell_SaveCommonButtonParameters 
#define  InterShell_GetSystemName									   ( !InterShell )?WIDE("NoInterShell"):InterShell->InterShell_GetSystemName
#define  UpdateButtonExx									   if( InterShell )InterShell->UpdateButtonExx 
#define  ShellGetCurrentPage(x)								   (( !InterShell )?NULL:InterShell->ShellGetCurrentPage(x))
#define  ShellGetNamedPage									   ( !InterShell )?NULL:InterShell->ShellGetNamedPage
#define  ShellSetCurrentPage								   if( InterShell )InterShell->ShellSetCurrentPage
#define  ShellCallSetCurrentPage							   ( InterShell )?FALSE:InterShell->ShellCallSetCurrentPage
#define  ShellCallSetCurrentPageEx							   if( InterShell )InterShell->ShellCallSetCurrentPageEx 
#define  ShellReturnCurrentPage								   if( InterShell )InterShell->ShellReturnCurrentPage 
#define  ClearPageList										   if( InterShell )InterShell->ClearPageList 
#define  InterShell_DisablePageUpdate								   if( InterShell )InterShell->InterShell_DisablePageUpdate 
#define  RestoreCurrentPage									   if( InterShell )InterShell->RestoreCurrentPage 
#define  HidePageExx											   if( InterShell )InterShell->HidePageExx
#define  InterShell_DisableButtonPageChange						   if( InterShell )InterShell->InterShell_DisableButtonPageChange 
#define  CreateLabelVariable								  ( !InterShell )?NULL:InterShell->CreateLabelVariable
#define  CreateLabelVariableEx								   ( !InterShell )?NULL:InterShell->CreateLabelVariableEx
#define  LabelVariableChanged								   if( InterShell )InterShell->LabelVariableChanged 
#define  LabelVariablesChanged								   if( InterShell )InterShell->LabelVariablesChanged 
#define  InterShell_HideEx											   if( InterShell )InterShell->InterShell_HideEx
//#define  InterShell_Reveal										   if( InterShell )InterShell->InterShell_Reveal
#define  InterShell_RevealEx										   if( InterShell )InterShell->InterShell_RevealEx
#define  GetPageSize										   if( InterShell )InterShell->GetPageSize 
#define  SetButtonTextField									   if( InterShell )InterShell->SetButtonTextField 
#define  AddButtonLayout									   ( !InterShell )?NULL:InterShell->AddButtonLayout
#define  InterShell_GetButtonControl								   ( !InterShell )?NULL:InterShell->InterShell_GetButtonControl
#define  InterShell_GetButtonCanvas								   ( !InterShell )?NULL:InterShell->InterShell_GetButtonCanvas
#define  InterShell_GetPhysicalButton								   ( !InterShell )?NULL:InterShell->InterShell_GetPhysicalButton
#define  InterShell_GetLabelText(label,string)								   ( !InterShell )?(string):InterShell->InterShell_GetLabelText(label,string)
#define  InterShell_TranslateLabelText								   ( !InterShell )?NULL:InterShell->InterShell_TranslateLabelText
#define  InterShell_GetControlLabelText								   if( InterShell )InterShell->InterShell_GetControlLabelText 
#define  BeginCanvasConfiguration								   if( InterShell )InterShell->BeginCanvasConfiguration 
#define  SaveCanvasConfiguration								   if( InterShell )InterShell->SaveCanvasConfiguration 
#define  SaveCanvasConfiguration_XML								   if( InterShell )InterShell->SaveCanvasConfiguration_XML 
#define  InterShell_GetCurrentConfigHandler								   if( InterShell )InterShell->InterShell_GetCurrentConfigHandler 
#define  InterShell_GetCurrentLoadingControl								   ( !InterShell )?NULL:InterShell->InterShell_GetCurrentLoadingControl
#define  BeginSubConfiguration								   if( InterShell )InterShell->BeginSubConfiguration 
#define  EscapeMenuString								   ( !InterShell )?NULL:InterShell->EscapeMenuString

#define  InterShell_SetButtonHighlight(a,b)     if(InterShell)InterShell->InterShell_SetButtonHighlight(a,b)
#define  SelectACanvasFont								   ( !InterShell )?NULL:InterShell->SelectACanvasFont
#define  UseACanvasFont								   ( !InterShell )?NULL:InterShell->UseACanvasFont
#define  CreateACanvasFont								   ( !InterShell )?NULL:InterShell->CreateACanvasFont
#define  CreateACanvasFont2								   ( !InterShell )?NULL:InterShell->CreateACanvasFont2
#define  InterShell_GetButtonExtension        ( !InterShell )?0:InterShell->InterShell_GetButtonExtension
#define  CreateAFont								   ( !InterShell )?NULL:InterShell->CreateAFont
#define  InterShell_CreateNamedPage								   if( InterShell ) InterShell->InterShell_CreateNamedPage
#define InterShell_SetTextLabelOptions         if(InterShell)InterShell->InterShell_SetTextLabelOptions

#define InterShell_GetButtonFont         ( !InterShell )?NULL:InterShell->InterShell_GetButtonFont
#define InterShell_GetButtonFontName  ( !InterShell )?NULL:InterShell->InterShell_GetButtonFontName
#define InterShell_SetButtonFontName  if( InterShell )InterShell->InterShell_SetButtonFontName
#define InterShell_GetCurrentButton  ( !InterShell )?NULL:InterShell->InterShell_GetCurrentButton
#define InterShell_GetSaveIndent     ( !InterShell )?WIDE("\t"):InterShell->InterShell_GetSaveIndent
#define InterShell_SetTheme          if( InterShell ) InterShell->InterShell_SetTheme
#define DisplayMenuCanvas            if( InterShell ) (InterShell)->DisplayMenuCanvas
#define InterShell_SetPageColor        if( InterShell ) (InterShell)->InterShell_SetPageColor
#define InterShell_GetCurrentLoadingCanvas				  ( !InterShell )?NULL:InterShell->InterShell_GetCurrentLoadingCanvas
#define InterShell_GetCurrentSavingCanvas				  ( !InterShell )?NULL:InterShell->InterShell_GetCurrentSavingCanvas


#define  SetupSecurityEdit								   if( InterShell ) (InterShell)->SetupSecurityEdit
#define  CreateSecurityContext							( !InterShell )?0:InterShell->CreateSecurityContext
#define  AddSecurityContextToken							if( InterShell ) InterShell->AddSecurityContextToken
#define  GetSecurityContextTokens						if( InterShell ) InterShell->GetSecurityContextTokens
#define  GetSecurityModules    						if( InterShell ) InterShell->GetSecurityModules

#define  CloseSecurityContext								if( InterShell ) (InterShell)->CloseSecurityContext
#define  InterShell_SaveSecurityInformation        if( InterShell ) (InterShell)->InterShell_SaveSecurityInformation

#define InterShell_SetCloneButton								if( InterShell) (InterShell)->InterShell_SetCloneButton
#define InterShell_GetCurrentPageName                    ( !InterShell)?WIDE("first"):(InterShell)->InterShell_GetCurrentPageName


#endif

# ifndef HidePageEx
#define HidePageEx(page) HidePageExx( page DBG_SRC )
# endif

# ifdef __cplusplus
} }
using namespace sack::intershell;
# endif

#endif
