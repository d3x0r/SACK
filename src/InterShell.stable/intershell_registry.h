#ifndef INTERSHELL_REGISTRY_SHORTCUTS_DEFINED
#define INTERSHELL_REGISTRY_SHORTCUTS_DEFINED

#define InterShell_REGISTRY // symbol to test if this was included....
#define TASK_PREFIX WIDE("intershell")
#include <psi.h>
#include <configscript.h>
#ifdef SACK_CORE_BUILD
#ifndef INTERSHELL_CORE_BUILD
#define INTERSHELL_CORE_BUILD
#endif
#endif
#if defined( INTERSHELL_CORE_BUILD ) || defined( LEGACY_MAKE_SYSTEM )
#include "widgets/include/buttons.h"
#else
#include <InterShell/widgets/buttons.h>
#endif
#ifndef POSTFIX
#define POSTFIX WIDE("")
#endif

#ifdef __cplusplus
namespace sack {
	namespace intershell {
#endif

#ifndef MENU_BUTTON_DEFINED
#define MENU_BUTTON_DEFINED
		/* Pointer to the basic control that InterShell uses to track
		   controls on pages. This is given to a plugin when a control
		   is created, and the control should just treat this as a
		   handle and use proper interface methods with intershell.core
		   to get information.                                          */
		typedef struct menu_button *PMENU_BUTTON;
#endif

// static uintptr_t OnCreateMenuButton(WIDE("name"))(PMENU_BUTTON button ) { /*create button data*/ }
#define OnCreateMenuButton(name) \
	DefineRegistryMethod(TASK_PREFIX,CreateMenuButton,WIDE( "control" ),name POSTFIX,WIDE( "button_create" ),uintptr_t,(PMENU_BUTTON))

// parametrs to this are the parent control, x, y, width and height
// static uintptr_t OnCreateControl(WIDE( "" ))(PSI_CONTROL parent,int32_t x,int32_t y,uint32_t w,uint32_t h)
#define OnCreateControl(name) \
	DefineRegistryMethod(TASK_PREFIX,CreateControl,WIDE( "control" ),name,WIDE( "control_create" ),uintptr_t,(PSI_CONTROL,int32_t,int32_t,uint32_t,uint32_t))
#define OnCreateListbox(name) \
	DefineRegistryMethod(TASK_PREFIX,CreateMenuListbox,WIDE( "control" ),name,WIDE( "listbox_create" ),uintptr_t,(PSI_CONTROL))

#define OnDestroyMenuButton(name) \
	  DefineRegistryMethod(TASK_PREFIX,DestroyMenuButton,WIDE( "control" ),name,WIDE( "button_destroy" ),void,(uintptr_t))
#define OnDestroyControl OnDestroyMenuButton
#define OnDestroyListbox OnDestroyMenuButton

// Return the real PSI_CONTROL; which may not be the uintptr_t result from before
// static PSI_CONTROL OnGetControl(WIDE(""))(uintptr_t psvInit);
// { return (PSI_CONTROL)psvInit; }
#define OnGetControl(name) \
	DefineRegistryMethod(TASK_PREFIX,GetControl,WIDE( "control" ),name,WIDE( "get_control" ),PSI_CONTROL,(uintptr_t))
#define OnFixupControl(name) \
	  DefineRegistryMethod(TASK_PREFIX,FixupControl,WIDE( "control" ),name,WIDE( "control_fixup" ),void,(uintptr_t))
#define OnFixupMenuButton OnFixupControl

// things like clock can do a clock unlock
// things like lists can requery databases to show new items....
#define OnShowControl(name) \
	DefineRegistryMethod(TASK_PREFIX,ShowControl,WIDE( "control" ),name,WIDE( "show_control" ),void,(uintptr_t))
// static void OnHideControl( WIDE("") )( uintptr_t psv )
#define OnHideControl(name) \
	DefineRegistryMethod(TASK_PREFIX,HideControl,WIDE( "control" ),name,WIDE( "hide_control" ),void,(uintptr_t))

// static void OnLoadControl( WIDE(""))( PCONFIG_HANDLER, uintptr_t psv )
#define OnLoadControl( name ) \
     DefineRegistryMethod(TASK_PREFIX,LoadControl,WIDE( "control" ),name,WIDE( "control_config" ),void,(PCONFIG_HANDLER,uintptr_t))
#define OnLoad( name ) \
     _DefineRegistryMethod(TASK_PREFIX,LoadControl,WIDE( "control" ),name,WIDE( "control_config" ),void,(XML_Parser,uintptr_t))

#define OnConfigureControl(name) \
	DefineRegistryMethod(TASK_PREFIX,ConfigureControl,WIDE( "control" ),name,WIDE( "control_edit" ),uintptr_t,(uintptr_t,PSI_CONTROL))
#define OnEditControl OnConfigureControl

#define OnEditBegin(name) \
	  DefineRegistryMethod(TASK_PREFIX,EditBegin,WIDE( "control" ),name,WIDE( "on_menu_edit_begin" ),void,(uintptr_t))
#define OnEditEnd(name) \
	  DefineRegistryMethod(TASK_PREFIX,EditEnd,WIDE( "control" ),name,WIDE( "on_menu_edit_end" ),void,(uintptr_t))

#define OnEditModeBegin(name) \
	  DefineRegistryMethod(TASK_PREFIX,EditBegin,WIDE( "common" ),WIDE( "Begin Edit Mode" ), name WIDE( "_on_menu_edit_begin" ),void,(void))
#define OnEditModeEnd(name) \
	  DefineRegistryMethod(TASK_PREFIX,EditEnd,WIDE( "common" ),WIDE( "End Edit Mode" ),name WIDE( "_on_menu_edit_end" ),void,(void))

#define OnSelectListboxItem(name,othername) \
	DefineRegistrySubMethod(TASK_PREFIX,ListSelectionChanged,WIDE( "control" ),name,WIDE( "listbox_selection_changed" ),othername,void,(uintptr_t,PLISTITEM))
#define OnDoubleSelectListboxItem(name,othername) \
	DefineRegistrySubMethod(TASK_PREFIX,ListDoubleSelectionChanged,WIDE( "control" ),name,WIDE( "listbox_double_changed" ),othername,void,(uintptr_t,PLISTITEM))

/* 
 * this is depreicated, buttons shouldn't really HAVE to know the button they are... 
 *
 */
//#define OnSaveMenuButton(name)
//	  DefineRegistryMethod(TASK_PREFIX,SaveButton,WIDE( "control" ),name,WIDE( "button_save" ),void,(FILE*,PMENU_BUTTON,uintptr_t))
//#define OnSaveCommon OnSaveMenuButton
#define OnSave(name) \
	  _DefineRegistryMethod(TASK_PREFIX,SaveButton,WIDE( "control" ),name,WIDE( "control_save_xml" ),void,(genxWriter,uintptr_t))

#define OnSaveControl(name) \
	  DefineRegistryMethod(TASK_PREFIX,SaveButton,WIDE( "control" ),name,WIDE( "control_save" ),void,(FILE*,uintptr_t))

#define OnSaveControl(name) \
	DefineRegistryMethod(TASK_PREFIX,SaveButton,WIDE( "control" ),name,WIDE( "control_save" ),void,(FILE*,uintptr_t))
/* this method is depricated also, and will soon be obsolete.... perfer usage of OnSave and OnLoad for further development */
#define OnSaveMenuButton(name) \
	  DefineRegistryMethod(TASK_PREFIX,SaveButton,WIDE( "control" ),name,WIDE( "button_save" ),void,(FILE*,PMENU_BUTTON,uintptr_t))

// return TRUE/FALSE whether the control should be shown, else it remains hidden.
// this method is used both with MenuButton and Control.
// This is called after FixupButton(on MenuButtonOnly) and EndEdit( on Controls and MenuButtons )
// this is called during RestorePage(Full), which is used by ChangePage() after HidePage().
#define OnQueryShowControl( name )  \
	  DefineRegistryMethod(TASK_PREFIX,QueryShowControl,WIDE( "control" ),name,WIDE( "query can show" ),LOGICAL,(uintptr_t))

// static int OnChangePage(WIDE(""))( PSI_CONTROL pc_canvas )
#define OnChangePage(name) \
	  DefineRegistryMethod(TASK_PREFIX,ChangePage,WIDE( "common" ),WIDE( "change page" ),name WIDE( "_on_change_page" ),int,(PSI_CONTROL))

// static void OnLoadCommon(WIDE(""))(PCONFIG_HANDLER pch )
// can use InterShell_GetCurrentLoadingCanvas() to get the current canvas being loaded
#define OnLoadCommon( name )  \
	  DefineRegistryMethod(TASK_PREFIX,LoadCommon,WIDE( "common" ),WIDE( "common_config" ),name WIDE( "_on_load" ),void,(PCONFIG_HANDLER))

// static void OnSaveCommon(WIDE(""))(FILE*file_out )
// can use InterShell_GetCurrentSavingCanvas() to get the current canvas being loaded
#define OnSaveCommon(name) \
	   DefineRegistryMethod(TASK_PREFIX,SaveCommon,WIDE( "common" ),WIDE( "save common" ),name WIDE( "_on_save_common" ),void,(FILE*))

// handler to catch event of when a page is saved.  The currnet ppage_data is passed as a convenience
#define OnSavePage(name) \
	   DefineRegistryMethod(TASK_PREFIX,SavePage,WIDE( "common" ),WIDE( "save page" ),name WIDE( "_on_save_page" ),void,(FILE*,PPAGE_DATA))
#define OnSaveXMLPage(name) \
	   DefineRegistryMethod(TASK_PREFIX,SavePage,WIDE( "common" ),WIDE( "save xml page" ),name WIDE( "_on_save_page" ),void,(genxWriter,PPAGE_DATA))

// invoked when all other initization is done, and before the main applciation actually runs and displays stuff.
//
// static void OnFinishInit(WIDE(""))(PSI_CONTROL pc_canvas);
#define OnFinishInit( name ) \
      DefineRegistryMethod(TASK_PREFIX,FinishInit,WIDE( "common" ),WIDE( "finish init" ),name WIDE( "_on_finish_init" ),void,(PSI_CONTROL))

// invoked when ALL initialzation is run, just after the menu is shown.  (tasks, first load)
#define OnFinishAllInit( name ) \
      DefineRegistryMethod(TASK_PREFIX,FinishAllInit,WIDE( "common" ),WIDE( "finish all init" ),name WIDE( "_on_finish_all_init" ),void,(void))

// invoked when ALL initialzation is run, just after the menu is shown.  (tasks, first load)
#define OnTaskLaunchComplete( name ) \
      __DefineRegistryMethod(TASK_PREFIX,TaskLaunchComplete,WIDE( "common" ),WIDE( "task launch complete" ),name WIDE( "_on_task_launch_complete" ),void,(void),__LINE__)

// passed the PSI_CONTROL of the menu canvas, used to give the control the parent to display its frame against.
// may also be used to get the current page.
#define OnGlobalPropertyEdit( name ) \
      DefineRegistryMethod(TASK_PREFIX,GlobalProperty,WIDE( "common" ),WIDE( "global properties" ),name,void,(PSI_CONTROL))

#define OnInterShellShutdown( name ) \
      DefineRegistryMethod(TASK_PREFIX,InterShellShutdown,WIDE( "common" ),WIDE( "intershell shutdown" ),name WIDE( "_on_intershell_shutdown" ),void,(void))

// static LOGICAL OnDropAccept(WIDE(""))(PSI_CONTROL pc_canvas,CTEXTSTR filepath,int x,int y)
#define OnDropAccept(name) \
	__DefineRegistryMethod(TASK_PREFIX,DropAccept,WIDE( "common" ),WIDE( "Drop Accept" ),name WIDE( "_on_drop_accept" ),LOGICAL,(PSI_CONTROL,CTEXTSTR,int,int),__LINE__)

//GETALL_REGISTERED( WIDE("issue_pos/common/common_config") )

#define GETALL_REGISTERED( root,type,args )	{          \
		CTEXTSTR name;                           \
		PCLASSROOT data = NULL;                     \
		for( name = GetFirstRegisteredName( root, &data );  \
			 name;                                           \
			  name = GetNextRegisteredName( &data ) )        \
		{ type (CPROC *f)args; f = GetRegisteredProcedure2(root,type,name,args);

#define ENDALL_REGISTERED()		} }


//-------------
// second generation (post documentation)

// Method invoked when a control is selected and a copy operation invoked.
// New method requires 'static &lt;return type&gt;' to be applied...
// static void OnCopyControl( "blah" )( uintptr_t psvYourControl );
#define OnCopyControl(name) \
	__DefineRegistryMethod(TASK_PREFIX,CopyControl,WIDE( "control" ),name,WIDE( "copy_control" ),void,(uintptr_t),__LINE__)

// Method invoked when a control is selected and a paste operation invoked.
// New method requires 'static &lt;return type&gt;' to be applied...
// static void OnPasteControl( WIDE( "blah" ) )( uintptr_t psvYourControl );
#define OnPasteControl(name) \
	__DefineRegistryMethod(TASK_PREFIX,PasteControl,WIDE( "control" ),name,WIDE( "paste_control" ),void,(uintptr_t),__LINE__)

// Event handler invoked when a control is cloned (using existing interface)
// static void OnCloneControl( WIDE( "blah" ) )( uintptr_t psvControlThatIsNewClone, uintptr_t psvControlThatIsBeingCloned )
#define OnCloneControl(name) \
	__DefineRegistryMethod(TASK_PREFIX,CloneControl,WIDE( "control" ),name,WIDE( "clone_control" ),void,(uintptr_t,uintptr_t),__LINE__)

#define OnLoadSecurityContext( name ) \
	__DefineRegistryMethod(TASK_PREFIX,LoadSecurityContext,WIDE( "common/security" ),WIDE( "Load Security" ),name WIDE( "_load_security" ),void,(PCONFIG_HANDLER),__LINE__)

#define OnSaveSecurityContext( name ) \
	__DefineRegistryMethod(TASK_PREFIX,SaveSecurityContext,WIDE( "common/security" ),WIDE( "Save Security" ),name WIDE( "_save_security" ),void,(FILE*,uintptr_t),__LINE__)

#define OnAddSecurityContextToken( name ) \
	__DefineRegistryMethod(TASK_PREFIX,AddSecurityContextToken_,WIDE( "common/security/Add Security Token" ),name, WIDE( "add_token" ),void,(uintptr_t,CTEXTSTR),__LINE__)

#define OnGetSecurityContextTokens( name ) \
	__DefineRegistryMethod(TASK_PREFIX,GetSecurityContextToken_,WIDE( "common/security/Get Security Tokens" ),name, WIDE( "get_tokens" ),void,(uintptr_t,PLIST*),__LINE__)

/* result INVALID_INDEX - premission denied
 result 0 - no context
 any other result is a result handle that is also closed when complete */
#define TestSecurityContext( name ) \
	__DefineRegistryMethod(TASK_PREFIX,TestSecurityContext,WIDE( "common/security" ),WIDE( "Test Security" ),name WIDE( "_test_security" ),uintptr_t,(uintptr_t),__LINE__)
#define EndSecurityContext( name ) \
	__DefineRegistryMethod(TASK_PREFIX,TestSecurityContext,WIDE( "common/security" ),WIDE( "Close Security" ),name WIDE( "_close_security" ),void,(uintptr_t,uintptr_t),__LINE__)

#define OnEditSecurityContext( name ) \
	__DefineRegistryMethod(TASK_PREFIX,EditSecurityContext,WIDE( "common/security" ),WIDE( "Edit Security" ),name,void,(uintptr_t),__LINE__)

/* Intended use:edits properties regarding page security... OnPageChange return FALSE to disallow page change...*/
#define OnEditPageSecurityContext( name ) \
	__DefineRegistryMethod(TASK_PREFIX,EditSecurityContext,WIDE( "common" ),WIDE( "global properties" ),WIDE( "Page Security:" )name,void,(PPAGE_DATA),__LINE__)

#define OnApplicationQuit( name ) \
	__DefineRegistryMethod(TASK_PREFIX,IntershellQuitApplication,WIDE( "common" ),WIDE( "quit application" ),name,void,(void),__LINE__)

/* Intended use: Supply configuration slots for theme_id
 static void OnThemeAdded( name )( int theme_id )
 */
#define OnThemeAdded( name ) \
	__DefineRegistryMethod(TASK_PREFIX,ThemeAdded,WIDE( "common" ),WIDE( "theme/add" ),WIDE( "Theme Add:" )name,void,(int theme_id),__LINE__)

/* Intended use: Theme is changing, the theme_id that is given was the prior theme set
 static void OnThemeChanging( name )( int theme_id )
*/
#define OnThemeChanging( name ) \
	__DefineRegistryMethod(TASK_PREFIX,ThemeChanging,WIDE( "common" ),WIDE( "theme/changing" ),WIDE( "Theme Changing:" )name,void,(int theme_id),__LINE__)

/* Intended use: Receive event that theme has changed, use theme_id to update graphics if available
 static void OnThemeChanging( name )( int theme_id )
*/
#define OnThemeChanged( name ) \
	__DefineRegistryMethod(TASK_PREFIX,ThemeChanged,WIDE( "common" ),WIDE( "theme/changed" ),WIDE( "Theme Change:" )name,void,(int theme_id),__LINE__)

#ifdef __cplusplus
	} }
using namespace sack::intershell;
#endif

#endif

