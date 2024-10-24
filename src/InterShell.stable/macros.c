#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include "widgets/include/banner.h"
#include "intershell_local.h"
#include "intershell_registry.h"
#include "menu_real_button.h"
#include "resource.h"
#include "fonts.h"

INTERSHELL_NAMESPACE

#define MACRO_BUTTON_NAME "Macro"



typedef struct macro_element{
	DeclareLink( struct macro_element );
	//TEXTCHAR *name; // name to apply for macro // use button->text instead.
	PMENU_BUTTON button;
	struct {
		BIT_FIELD allow_continue : 1; // support for external macro elements to control macro execution.
	} flags;
} MACRO_ELEMENT, *PMACRO_ELEMENT;

typedef struct macro_button
{
	PMACRO_ELEMENT elements;
	PMENU_BUTTON button;
}MACRO_BUTTON, *PMACRO_BUTTON;

static struct local_macro_data {

	// need to figure out a good way to heirarch these buttons?
	//PLIST buttons; // these are my buttons. I need the push events...
	MACRO_BUTTON startup;
	MACRO_BUTTON shutdown;
	LOGICAL finished_startup;
	PSI_CONTROL configuration_parent;
} local_macro_data;

enum {
	LIST_CONTROL_TYPES = 2000
		, BUTTON_ADD_CONTROL
	  , LIST_MACRO_ELEMENTS
	  , BUTTON_ELEMENT_UP
	  , BUTTON_ELEMENT_DOWN
	  , BUTTON_ELEMENT_CONFIGURE
	  , BUTTON_ELEMENT_CLONE
	  , BUTTON_ELEMENT_REMOVE
	 , BUTTON_ELEMENT_CLONE_ELEMENT
};

PRELOAD( RegisterMacroDialogResources )
{

	EasyRegisterResource( "intershell/macros", LIST_CONTROL_TYPES, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ADD_CONTROL, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ELEMENT_CONFIGURE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ELEMENT_UP, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ELEMENT_DOWN, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ELEMENT_REMOVE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ELEMENT_CLONE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", BUTTON_ELEMENT_CLONE_ELEMENT, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/macros", LIST_MACRO_ELEMENTS, LISTBOX_CONTROL_NAME );
#define EasyAlias( x, y )	\
	RegisterClassAlias( "psi/resources/intershell/macros/" y "/" #x, "psi/resources/application/" y "/" #x )
	// migration path...
	EasyAlias(LIST_CONTROL_TYPES, LISTBOX_CONTROL_NAME );
	EasyAlias(BUTTON_ADD_CONTROL, NORMAL_BUTTON_NAME );
	EasyAlias(LIST_MACRO_ELEMENTS, LISTBOX_CONTROL_NAME );
	
}


void FillList( PSI_CONTROL list,PMACRO_BUTTON button )
{
	if( list )
	{
		PMACRO_ELEMENT pme = button->elements;
		ResetList( list );
		while( pme )
		{
			SetItemData( AddListItem( list, (pme->button->text&&pme->button->text[0])?pme->button->text:pme->button->pTypeName ), (uintptr_t)pme );
			//SetItemText( pli, (pme->button->text&&pme->button->text[0])?pme->button->text:pme->button->pTypeName );
			pme = NextThing( pme );
		}
	}
}


static void CPROC MoveElementClone( uintptr_t psv, PSI_CONTROL control )
{
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	PMENU_BUTTON pmbNew = GetCloneButton( NULL, 0, 0, TRUE );
	if( pmbNew )
	{
		PMACRO_ELEMENT pme = New( MACRO_ELEMENT );
		pme->me = NULL;
		pme->next = NULL;
		pme->button = pmbNew;
		LinkLast( button->elements, PMACRO_ELEMENT, pme );
		FillList( GetNearControl( control, LIST_MACRO_ELEMENTS ), button );
	}
}

static void CPROC CloneElement( uintptr_t psv, PSI_CONTROL control )
{
	PSI_CONTROL list = GetNearControl( control, LIST_MACRO_ELEMENTS );
	PLISTITEM pli = GetSelectedItem( list );
	PMACRO_ELEMENT pme = (PMACRO_ELEMENT)GetItemData( pli );
	if( pme )
		InterShell_SetCloneButton( pme->button );
}


static void CPROC AddButtonType( uintptr_t psv, PSI_CONTROL control )
{
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	PSI_CONTROL list = GetNearControl( control, LIST_CONTROL_TYPES );
	PLISTITEM pli = GetSelectedItem( list );
	TEXTCHAR *name = (TEXTCHAR*)GetItemData( pli );
	if( pli && button )
	{
		PMACRO_ELEMENT pme = New( MACRO_ELEMENT );
		pme->me = NULL;
		pme->next = NULL;
		pme->button = CreateInvisibleControl( configure_key_dispatch.frame, name );
		if( pme->button )
		{
			pme->button->container_button = button->button;
			//lprintf( "Setting container of %p to %p", pme->button, button->button );

			LinkLast( button->elements, PMACRO_ELEMENT, pme );
			{
				// fake this lock...
				// global code allows this to continue... which means loss of current_configuration datablock.
				// faking this starts a configuration thread.
				//g.flags.bIgnoreKeys = 0;
				ConfigureKeyExx( configure_key_dispatch.frame, pme->button, TRUE, FALSE );
				//g.flags.bIgnoreKeys = 1;
				 // I'm still editing this mode...
			}
			FillList( GetNearControl( control, LIST_MACRO_ELEMENTS ), button );
		}
		else
			DebugBreak();
	}
}


int FillControlsList( PSI_CONTROL control, int nLevel, CTEXTSTR basename, CTEXTSTR priorname )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	PVARTEXT pvt = NULL;
	int added = 0;
	PLISTITEM pli;
	if( priorname )
	{
		pli = AddListItemEx( control, nLevel, priorname );
	}
	for( name = GetFirstRegisteredName( basename, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		pvt = VarTextCreate();
		vtprintf( pvt, "%s/%s", basename, name );
		if( priorname &&
			( strcmp( name, "button_create" ) == 0 ) )
		{
			// okay then add this one...
			//snprintf( newname, sizeof( newname ), "%s/%s", basename, name );
			//if( NameHasBranches( &data ) )
			{
				// eat the first two parts - intershell/controls/
				// create the control name as that...
				CTEXTSTR controlpath = strchr( basename, '/' );
				if( controlpath )
				{
					controlpath++;
					controlpath = strchr( controlpath, '/' );
					if( controlpath )
						controlpath++;
				}
				//AddLink( &g.extra_types, StrDup( basename ) );
				added++;
				SetItemData( pli, (uintptr_t)StrDup( controlpath ) );
				break;
			}
		}
		else
		{
			if( NameHasBranches( &data ) )
			{
				added += FillControlsList( control, nLevel+1, GetText( VarTextPeek( pvt ) ), name );
			}
		}
	}
	if( !added )
		DeleteListItem( control, pli );
	VarTextDestroy( &pvt );
	return added;
}

static void CPROC MoveElementUp( uintptr_t psv, PSI_CONTROL control )
{
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	PSI_CONTROL list = GetNearControl( control, LIST_MACRO_ELEMENTS );
	PLISTITEM pli = GetSelectedItem( list );
	PMACRO_ELEMENT pme = (PMACRO_ELEMENT)GetItemData( pli );
	if( pme )
	{
		PMACRO_ELEMENT _prior = ((PMACRO_ELEMENT)pme->me);
		if( (uintptr_t)&pme->next != (uintptr_t)pme )
		{
			lprintf( "Failure, structure definition does not have DeclareLink() as first member." );
			DebugBreak();
		}
		if( _prior != (PMACRO_ELEMENT)(&button->elements) )
		{
			UnlinkThing( _prior );
			LinkThingAfter( pme, _prior );
			FillList( list, button );
		}
	}
}

static void CPROC MoveElementDown( uintptr_t psv, PSI_CONTROL control )
{
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	PSI_CONTROL list = GetNearControl( control, LIST_MACRO_ELEMENTS );
	PLISTITEM pli = GetSelectedItem( list );
	PMACRO_ELEMENT pme = (PMACRO_ELEMENT)GetItemData( pli );
	if( pme )
	{
		PMACRO_ELEMENT _next = NextThing( pme );
		if( (uintptr_t)&pme->next != (uintptr_t)pme )
		{
			lprintf( "Failure, structure definition does not have DeclareLink() as first member." );
			DebugBreak();
		}
		if( _next && pme )
		{
			UnlinkThing( pme );
			LinkThingAfter( _next, pme );
			FillList( list, button );
		}
	}
}

static void CPROC ConfigureElement( uintptr_t psv, PSI_CONTROL control )
{
	//PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	PSI_CONTROL list = GetNearControl( control, LIST_MACRO_ELEMENTS );
	PLISTITEM pli = GetSelectedItem( list );
	PMACRO_ELEMENT pme = (PMACRO_ELEMENT)GetItemData( pli );
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	if( pme )
	{
		lprintf( "editing element %p in button %p", pme->button, button->button );
		{
			// fake this lock...
			// global code allows this to continue... which means loss of current_configuration datablock.
			PCanvasData canvas = configure_key_dispatch.canvas;
			lprintf( "canvas is now %p", canvas );
			if( canvas )
			{
				canvas->flags.bIgnoreKeys = 0;
				// wait for completion.
				configure_key_dispatch.button = pme->button;
				ConfigureKeyExx( control, pme->button, TRUE, FALSE );
				configure_key_dispatch.button = button->button;
				SetItemText( pli, (pme->button->text&&pme->button->text[0])?pme->button->text:pme->button->pTypeName );
				canvas->flags.bIgnoreKeys = 1;
			}
			else
			{
				SimpleMessageBox( control, "General Failure", "Failed to get canvas\nAborting element edit" );
			}
		}
	}
}

static void CPROC MoveElementRemove( uintptr_t psv, PSI_CONTROL control )
{
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	PSI_CONTROL list = GetNearControl( control, LIST_MACRO_ELEMENTS );
	PLISTITEM pli = GetSelectedItem( list );
	PMACRO_ELEMENT pme = (PMACRO_ELEMENT)GetItemData( pli );
	if( pme )
	{
		DeleteListItem( list, pli );
		UnlinkThing( pme );
		 DestroyButton( pme->button );
		Release( pme );
		FillList( list, button );
	}
}

static void ConfigureMacroButton( PMACRO_BUTTON button, PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "ConfigureMacroButton.isFrame" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		local_macro_data.configuration_parent = parent;
		SetCommonButtons( frame, &done, &okay );
		{
			PSI_CONTROL list;
			SetListboxIsTree( list = GetControl( frame, LIST_CONTROL_TYPES ), 1 );
			ResetList( list );
			FillControlsList( list, 1, TASK_PREFIX "/control", NULL );
			SetCommonButtonControls( frame );
			SetButtonPushMethod( GetControl( frame, BUTTON_ADD_CONTROL ), AddButtonType, (uintptr_t)button );
			//SetButtonPushMethod( GetControl( frame, BUTTON_EDIT_CONTROL ), AddButtonType, (uintptr_t)button );
			{
				PSI_CONTROL list = GetControl( frame, LIST_MACRO_ELEMENTS );
				if( list )
				{
					PMACRO_ELEMENT pme = button->elements;
					while( pme )
					{
						SetItemData( AddListItem( list, (pme->button->text&&pme->button->text[0])?pme->button->text:pme->button->pTypeName ), (uintptr_t)pme );
						pme = NextThing( pme );
					}
				}
			}
			SetButtonPushMethod( GetControl( frame, BUTTON_ELEMENT_UP ), MoveElementUp, (uintptr_t)button );
			SetButtonPushMethod( GetControl( frame, BUTTON_ELEMENT_CLONE ), MoveElementClone, (uintptr_t)button );
			SetButtonPushMethod( GetControl( frame, BUTTON_ELEMENT_CLONE_ELEMENT ), CloneElement, (uintptr_t)button );
			SetButtonPushMethod( GetControl( frame, BUTTON_ELEMENT_DOWN ), MoveElementDown, (uintptr_t)button );
			SetButtonPushMethod( GetControl( frame, BUTTON_ELEMENT_CONFIGURE ), ConfigureElement, (uintptr_t)button );
			SetButtonPushMethod( GetControl( frame, BUTTON_ELEMENT_REMOVE ), MoveElementRemove, (uintptr_t)button );
		}
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			GetCommonButtonControls( frame );
		}
		DestroyFrame( &frame );
	}
}


static uintptr_t OnEditControl( MACRO_BUTTON_NAME )( uintptr_t psv, PSI_CONTROL parent )
{
	ConfigureMacroButton( (PMACRO_BUTTON)psv, parent );
	return psv;
}

struct current_macro_state_during_invoke {
	PMACRO_ELEMENT element;
	PMACRO_BUTTON macro;
} ;

PLINKSTACK pls_macros;

void SetMacroResult( int allow_continue )
{
	struct current_macro_state_during_invoke *current_invoke_state = (struct current_macro_state_during_invoke *)PeekLink( &pls_macros );
	if( current_invoke_state && current_invoke_state->element )
		current_invoke_state->element->flags.allow_continue = allow_continue;
}

static void InvokeMacroButton( PMACRO_BUTTON button, LOGICAL bBannerMessage )
{
	struct current_macro_state_during_invoke new_current_macro;

	if( !pls_macros )
		pls_macros = CreateLinkStack();

	PushLink( &pls_macros, &new_current_macro );
	new_current_macro.macro = button;
	for( new_current_macro.element = button->elements; new_current_macro.element; new_current_macro.element = NextThing( new_current_macro.element ) )
	{
		if( !QueryShowControl( new_current_macro.element->button ) )
			continue;
		if( new_current_macro.element->button->original_keypress )
		{
			// there really should be some sort of way to abort, and unpress these buttons...
			// that would be cool, but I don't think that will happen.
			new_current_macro.element->flags.allow_continue = 1;
			if( bBannerMessage )
			{
				TEXTCHAR msg[256];
				TEXTCHAR msg2[256];
				int cur = 0;
				int ofs;
				InterShell_TranslateLabelText( NULL, msg2, sizeof( msg2 ), new_current_macro.element->button->text );
				for( ofs = 0; msg2[ofs]; ofs++ )
				{
					if( msg2[ofs] == '\\' )
					{
						if( msg2[ofs+1] == 'n' )
						{
							msg2[cur++] = '\n';
							ofs++;
						}
					}
					else
						msg2[cur++] = msg2[ofs];
				}
				msg2[cur++] = msg2[ofs];
				snprintf( msg, sizeof( msg ), "Invoke Startup\n%s", msg2 );
				// friggin visual studio... the spec claims 'will have a nul in buffer'
				msg[255] = 0;
				Banner2NoWaitAlpha( msg );
			}
			new_current_macro.element->button->original_keypress( new_current_macro.element->button->psvUser );
			if( !new_current_macro.element->flags.allow_continue )
			{
				// abort the page change too.
				if( new_current_macro.macro->button )
					new_current_macro.macro->button->flags.bIgnorePageChange = 1;
				break; // stop this macro button.
			}
		}
		if( new_current_macro.element->button->pPageName )
		{
			if( !new_current_macro.element->button->canvas ) // probably part of starup or shutdown macro
			{
				if( !new_current_macro.element->button->flags.bIgnorePageChange )
				{
					ShellSetCurrentPage( InterShell_GetCanvas( new_current_macro.macro->button->page )
						, new_current_macro.element->button->pPageName );
				}
			}
			else if( new_current_macro.element->button->canvas && !new_current_macro.element->button->flags.bIgnorePageChange )
			{
				//lprintf( "Changing pages, but only virtually don't activate the page always" );
				ShellSetCurrentPage( new_current_macro.element->button->canvas->pc_canvas, new_current_macro.element->button->pPageName );
			}
			new_current_macro.element->button->flags.bIgnorePageChange = 0;
		}
	}
	PopLink( &pls_macros );
}



static void OnKeyPressEvent( MACRO_BUTTON_NAME )( uintptr_t psv )
{
	InvokeMacroButton( (PMACRO_BUTTON)psv, FALSE );
}

static void OnShowControl( MACRO_BUTTON_NAME )( uintptr_t psv )
{
	struct current_macro_state_during_invoke current_invoke_state;

	current_invoke_state.macro = (PMACRO_BUTTON)psv;
	if( current_invoke_state.macro )
		for( current_invoke_state.element = current_invoke_state.macro->elements;
			 current_invoke_state.element;
			 current_invoke_state.element = NextThing( current_invoke_state.element ) )
		{
			InvokeShowControl( current_invoke_state.element->button  );
		}
	current_invoke_state.element = NULL;
	current_invoke_state.macro = NULL;
}


static uintptr_t OnCreateMenuButton( MACRO_BUTTON_NAME )( PMENU_BUTTON common )
{
	PMACRO_BUTTON button = New( MACRO_BUTTON );
	button->button = common;
	button->elements = NULL;
	//AddLink( &local_macro_data.buttons, button );
	return (uintptr_t)button;
}

void WriteMacroButton( CTEXTSTR leader, FILE *file, uintptr_t psv )
{
	PMACRO_BUTTON button = (PMACRO_BUTTON)psv;
	// save buttons...
	{
		PMACRO_ELEMENT element;
		for( element = button->elements; element; element = NextThing( element ) )
		{
			sack_fprintf( file, "%s%sMacro Element \'%s\'\n", InterShell_GetSaveIndent(), leader?leader:"", element->button->pTypeName );
	  		DumpGeneric( file, element->button ); /* begins another sub configuration... */
			sack_fprintf( file, "%s%smacro element done\n", InterShell_GetSaveIndent(), leader?leader:"" );
			//sack_fprintf( file, "%sMacro Element Text \'%s\'\n", leader?leader:"", element->button->text );
		}
		sack_fprintf( file, "%s%smacro element list done\n", InterShell_GetSaveIndent(), leader?leader:"" );
	}
}

static void OnSaveControl( MACRO_BUTTON_NAME )( FILE *file, uintptr_t psv )
{
	WriteMacroButton( NULL, file, psv );
}

static uintptr_t CPROC LoadMacroElements( uintptr_t psv, arg_list args )
{
	PMACRO_BUTTON pmb = (PMACRO_BUTTON)psv;
	PARAM( args, TEXTCHAR *, name );
	PMACRO_ELEMENT element = New( MACRO_ELEMENT );
	if( !pmb )
		if( local_macro_data.finished_startup )
		{
			if( !local_macro_data.shutdown.button )
				local_macro_data.shutdown.button = CreateButton( InterShell_GetCurrentLoadingCanvas() );
			pmb = &local_macro_data.shutdown;
		}
		else
		{
			if( !local_macro_data.startup.button )
				local_macro_data.startup.button = CreateButton( InterShell_GetCurrentLoadingCanvas() );
			pmb = &local_macro_data.startup;
		}
	element->me = NULL;
	element->next = NULL;
	element->button = CreateInvisibleControl( pmb->button?InterShell_GetCanvas( pmb->button->page ):NULL
	                                        , name );
	if( element->button )
	{
		//lprintf( "Setting container of %p to %p", element->button, pmb->button );
		element->button->container_button = pmb->button;
		LinkLast( pmb->elements, PMACRO_ELEMENT, element );
		if( !BeginSubConfigurationEx( element->button, name, (pmb==&local_macro_data.startup)
											?"Startup macro element done"
											:(pmb==&local_macro_data.shutdown)
											?"Shutdown macro element done"
											:"macro element done" ) )
		{
			PublicAddCommonButtonConfig( element->button );
		}
		//SetCurrentLoadingButton( element->button );
		//lprintf( "Resulting with psv %08x", element->button->psvUser );
		return element->button->psvUser;
	}
	else
		lprintf( "Failed to create a %s", name );
	return 0;
}

static uintptr_t CPROC FinishMacro( uintptr_t psv, arg_list args )
{
	PMACRO_BUTTON pmb = (PMACRO_BUTTON)psv;
	if( !pmb )
	{
		if( !local_macro_data.finished_startup )
		{
			local_macro_data.finished_startup = 1;
		}
	}
	return psv;
}

static void OnLoadControl( MACRO_BUTTON_NAME )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "Macro Element \'%m\'", LoadMacroElements );
	AddConfigurationMethod( pch, "Macro Element List Done", FinishMacro );
}

static void OnGlobalPropertyEdit( "Startup Macro")( PSI_CONTROL parent )
{
	// calling this direct causes a break for lack 
	// of a g.configure_key... okay?
	configure_key_dispatch.canvas = GetCanvas( parent );
	ConfigureMacroButton( &local_macro_data.startup, parent );
	configure_key_dispatch.canvas = NULL;
}
static void OnGlobalPropertyEdit( "Shutdown Macro")( PSI_CONTROL parent )
{
	// calling this direct causes a break for lack 
	// of a g.configure_key... okay?
	configure_key_dispatch.canvas = GetCanvas( parent );
	ConfigureMacroButton( &local_macro_data.shutdown, parent );
	configure_key_dispatch.canvas = NULL;
}

void InvokeStartupMacro( void )
{
	Banner2NoWaitAlpha( "Running Startup Macro..." );
	InvokeMacroButton( &local_macro_data.startup, TRUE );
}

void InvokeShutdownMacro( void )
{
	InvokeMacroButton( &local_macro_data.shutdown, TRUE );
}

static void OnSaveCommon( "Startup Macro" )( FILE *file )
{
	sack_fprintf( file, "\n" );
	WriteMacroButton( "Startup ", file, (uintptr_t)&local_macro_data.startup );
	sack_fprintf( file, "\n" );
	WriteMacroButton( "Shutdown ", file, (uintptr_t)&local_macro_data.shutdown );
}

static void OnLoadCommon( "Startup Macro" )( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, "Startup Macro Element \'%m\'", LoadMacroElements );
	AddConfigurationMethod( pch, "Startup Macro Element List Done", FinishMacro );
	
	AddConfigurationMethod( pch, "Shutdown Macro Element \'%m\'", LoadMacroElements );
	AddConfigurationMethod( pch, "Shutdown Macro Element List Done", FinishMacro );
}

static void OnCloneControl( MACRO_BUTTON_NAME )( uintptr_t psvNew, uintptr_t psvOriginal )
{
	PMACRO_BUTTON pmbOriginal = (PMACRO_BUTTON)psvOriginal;
	PMACRO_BUTTON pmbNew = (PMACRO_BUTTON)psvNew;

	{
		PMACRO_ELEMENT element;
		for( element = pmbOriginal->elements; element; element = element->next )
		{
			PMACRO_ELEMENT new_element = New( MACRO_ELEMENT );
			new_element->me = NULL;
			new_element->next = NULL;
			new_element->button = CreateInvisibleControl( element->button->canvas->pc_canvas
			                                            , element->button->pTypeName );
			if( new_element->button )
			{
				new_element->button->container_button = pmbNew->button;
				//lprintf( "Setting container of %p to %p", new_element->button, pmbNew->button );

				CloneCommonButtonProperties( new_element->button, element->button );
				InvokeCloneControl( new_element->button, element->button );

				new_element->flags.allow_continue = element->flags.allow_continue;

				LinkLast( pmbNew->elements, PMACRO_ELEMENT, new_element );
			}
		}
	}
}

INTERSHELL_NAMESPACE_END
