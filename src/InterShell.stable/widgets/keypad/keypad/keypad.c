#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE

#include <stdhdrs.h> // DebugBreak()
#include <intershell_export.h>
#include <intershell_registry.h>
#include "../../include/banner.h"
#include "keypad.h"
#include "resource.h" // This resource.h is actually in InterShell 

#define KEYPAD_ISP_SOURCE
// KEYPAD_KEYS
// SYMNAME( KEYPAD_KEYS, "Keypad Control" )

enum {
	LISTBOX_KEYPAD_TYPE = 4000,
	BTN_EDIT_VISUAL,
	COLOR_NUMKEYS,
	COLOR_ENTER,
	COLOR_CANCEL,
	COLOR_NUMKEYS_TEXT,
	COLOR_ENTER_TEXT,
	COLOR_CANCEL_TEXT,
	COLOR_DISPLAY_BACKGROUND,
	COLOR_DISPLAY_TEXT,
	COLOR_KEYPAD_BACKGROUND,
	EDIT_WIDTH,
	EDIT_HEIGHT,
	RADIO_STYLE_CORRECT_ENTER,
	RADIO_STYLE_CANCEL_ENTER,
	RADIO_STYLE_DOUBLE0_CLEAR,
	RADIO_STYLE_YES_NO,
	CHECKBOX_INVERT,
	CHECKBOX_PASSWORD,
	RADIO_STYLE_CLEAR_ENTER,
	EDIT_FORMAT,
	CHECKBOX_ALIGN_CENTER,
	CHECKBOX_ALIGN_LEFT,
};

PRELOAD( RegisterKeypadIDs )
{
	EasyRegisterResource( "InterShell/Keypad", LISTBOX_KEYPAD_TYPE, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "InterShell/Keypad", BTN_EDIT_VISUAL, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", COLOR_NUMKEYS, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_ENTER, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_CANCEL, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_NUMKEYS_TEXT, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_ENTER_TEXT, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_CANCEL_TEXT, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_KEYPAD_BACKGROUND, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_DISPLAY_BACKGROUND, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", COLOR_DISPLAY_TEXT, "Color Well" );
	EasyRegisterResource( "InterShell/Keypad", EDIT_WIDTH, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/Keypad", EDIT_HEIGHT, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/Keypad", EDIT_FORMAT, EDIT_FIELD_NAME );

	EasyRegisterResource( "InterShell/Keypad", RADIO_STYLE_CORRECT_ENTER, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", RADIO_STYLE_CANCEL_ENTER,  RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", RADIO_STYLE_YES_NO,  RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", RADIO_STYLE_DOUBLE0_CLEAR, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", RADIO_STYLE_CLEAR_ENTER, RADIO_BUTTON_NAME );

	EasyRegisterResource( "InterShell/Keypad", CHECKBOX_INVERT,			  RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", CHECKBOX_PASSWORD,			 RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", CHECKBOX_ALIGN_CENTER,			  RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Keypad", CHECKBOX_ALIGN_LEFT,			  RADIO_BUTTON_NAME );

}

typedef struct page_keypad
{
	PPAGE_DATA page;
	PSI_CONTROL keypad;
	CTEXTSTR keypad_type;
	int32_t x;
	int32_t y;
	uint32_t w;
	uint32_t h;

} PAGE_KEYPAD, *PPAGE_KEYPAD;

typedef struct hotkey
{
	struct {

		uint32_t bNegative : 1;

	} flags;

	PMENU_BUTTON button;
	uint64_t value;
	SFTFont *font;
	SFTFont *new_font;
	CTEXTSTR preset_name;
	CTEXTSTR keypad_type;
} HOTKEY, *PHOTKEY;

static struct {

	//PSI_CONTROL keypad; // only use one of these, else keybindings suck.
	PLIST keypads;
	PLIST keypad_types;
	uintptr_t psv_read_keypad;
} l;

void CPROC InvokeKeypadEnterEvent( uintptr_t psv, PSI_CONTROL pcKeypad )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;

	if( keypad->keypad_type )
	{
		INDEX idx;
		CTEXTSTR name;
		LIST_FORALL( l.keypad_types, idx, CTEXTSTR, name )
		{
			TEXTCHAR buffer[256];
			if( StrCmp( name, keypad->keypad_type ) == 0 )
			{
				snprintf( buffer, sizeof( buffer ), TASK_PREFIX "/common/%s/keypad enter", name );

				GETALL_REGISTERED( buffer, void, (PSI_CONTROL) )
				{ /* creates a magic f variable :( */
					if(f) f(pcKeypad);
				}

				ENDALL_REGISTERED();
			}
		}
	}

	else
	{
		LOGICAL did_one = 0;
		GETALL_REGISTERED( TASK_PREFIX "/common/keypad enter", void,(PSI_CONTROL) )
		{  /* creates a magic f variable :( */
			if(f) { 
				did_one = 1;
				f(pcKeypad); 
			}
		}

		ENDALL_REGISTERED();
		if( !did_one )
			ClearKeyedEntry( keypad->keypad );
	}
}


void CPROC InvokeKeypadCancelEvent( uintptr_t psv, PSI_CONTROL pcKeypad )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;

	if( keypad->keypad_type )
	{
		INDEX idx;
		CTEXTSTR name;
		LIST_FORALL( l.keypad_types, idx, CTEXTSTR, name )
		{
			TEXTCHAR buffer[256];
			if( StrCmp( name, keypad->keypad_type ) == 0 )
			{
				snprintf( buffer, sizeof( buffer ), TASK_PREFIX "/common/%s/keypad cancel", name );

				GETALL_REGISTERED( buffer, void, (PSI_CONTROL) )
				{ /* creates a magic f variable :( */
					if(f) f(pcKeypad);
				}

				ENDALL_REGISTERED();
			}
		}
	}

	else
	{
		GETALL_REGISTERED( TASK_PREFIX "/common/keypad cancel", void,(PSI_CONTROL) )
		{  /* creates a magic f variable :( */
			if(f) f(pcKeypad);
		}

		ENDALL_REGISTERED();
	}
}



static int IsGetKeypadControlForType( PSI_CONTROL frame, PPAGE_KEYPAD keypad )
{
	INDEX idx;
	PPAGE_KEYPAD ppk;
	int found = 0;

	LIST_FORALL( l.keypads, idx, PPAGE_KEYPAD, ppk )
	{
		if( keypad != ppk )
		{
			if( StrCmp( keypad->keypad_type, ppk->keypad_type ) == 0 )
			{
				found = 1;
				if( ppk->keypad )
				{
					keypad->keypad = ppk->keypad;
					return TRUE;
				}
			}
		}
	}

	keypad->keypad = MakeKeypad( frame
										, keypad->x
										, keypad->y
										, keypad->w
										, keypad->h
										, KEYPAD_KEYS
										, KEYPAD_FLAG_ENTRY|KEYPAD_FLAG_DISPLAY
										, keypad->keypad_type
										);
	SetKeypadEnterEvent( keypad->keypad, InvokeKeypadEnterEvent, (uintptr_t)keypad );
	SetKeypadCancelEvent( keypad->keypad, InvokeKeypadCancelEvent, (uintptr_t)keypad );
	//AddLink( &l.keypads, keypad );
	return TRUE;
}


PUBLIC( PSI_CONTROL, GetKeypadOfType )( CTEXTSTR type )
{
	INDEX idx;
	PPAGE_KEYPAD keypad;

	LIST_FORALL( l.keypads, idx, PPAGE_KEYPAD, keypad )
	{
		if( keypad->keypad_type && ( StrCaseCmp( keypad->keypad_type, type ) == 0 ) )
			return keypad->keypad;
	}

	return NULL;
}

PUBLIC( void, GetKeypadsOfType )( PLIST *ppResultList, CTEXTSTR type )
{
	INDEX idx;
	PPAGE_KEYPAD keypad;

	LIST_FORALL( l.keypads, idx, PPAGE_KEYPAD, keypad )
	{
		if( keypad->keypad_type && ( StrCaseCmp( keypad->keypad_type, type ) == 0 ) )
			AddLink( ppResultList, keypad->keypad );
	}
}

static void CPROC EditVisualProperties( uintptr_t psvKeypad, PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "ConfigureKeypadVisual.isFrame" );
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psvKeypad;

	if( frame )
	{
		PSI_CONTROL invert;
		PSI_CONTROL password;
		PSI_CONTROL yesno;
		PSI_CONTROL correct_enter;
		PSI_CONTROL cancel_enter;
		PSI_CONTROL clear_enter;
		PSI_CONTROL enter_00;
		enum keypad_styles prior_style;
		enum keypad_styles style;
		TEXTCHAR oldformat[256];
			
		int done = 0;
		int okay = 0;

		KeypadGetDisplayFormat( keypad->keypad, oldformat, 256 );
		SetCommonButtons( frame, &done, &okay );
		{
			PSI_CONTROL color_well;
			color_well = GetControl( frame, COLOR_DISPLAY_BACKGROUND );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetDisplayBackground(keypad->keypad) );

			color_well = GetControl( frame, COLOR_DISPLAY_TEXT );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetDisplayTextColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_NUMKEYS );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetNumberKeyColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_ENTER );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetEnterKeyColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_CANCEL );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetC_KeyColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_NUMKEYS_TEXT );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetNumberKeyTextColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_ENTER_TEXT );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetEnterKeyTextColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_CANCEL_TEXT );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetC_KeyTextColor(keypad->keypad) );

			color_well = GetControl( frame, COLOR_KEYPAD_BACKGROUND );
			EnableColorWellPick( color_well, TRUE );
			SetColorWell( color_well, KeypadGetBackgroundColor(keypad->keypad) );
		}

		SetControlText( GetControl( frame, EDIT_FORMAT ), oldformat );

		{
			prior_style = style = GetKeypadStyle( keypad->keypad );
			SetButtonGroupID( yesno = GetControl( frame, RADIO_STYLE_YES_NO ), 1 );
			SetButtonGroupID( correct_enter = GetControl( frame, RADIO_STYLE_CORRECT_ENTER ), 1 );
			SetButtonGroupID( cancel_enter = GetControl( frame, RADIO_STYLE_CANCEL_ENTER ), 1 );
			SetButtonGroupID( enter_00 = GetControl( frame, RADIO_STYLE_DOUBLE0_CLEAR ), 1 );
			SetButtonGroupID( clear_enter = GetControl( frame, RADIO_STYLE_CLEAR_ENTER ), 1 );
			invert = GetControl( frame, CHECKBOX_INVERT );
			SetCheckState( invert, ( style & KEYPAD_INVERT ) != 0 );
			password = GetControl( frame, CHECKBOX_PASSWORD );
			SetCheckState( password, ( style & KEYPAD_STYLE_PASSWORD ) != 0 );
			if( style & KEYPAD_DISPLAY_ALIGN_LEFT )
				SetCheckState( GetControl( frame, CHECKBOX_ALIGN_LEFT ), TRUE );
			if( style & KEYPAD_DISPLAY_ALIGN_CENTER )
				SetCheckState( GetControl( frame, CHECKBOX_ALIGN_CENTER ), TRUE );
			switch( style & KEYPAD_MODE_MASK )
			{
			case KEYPAD_STYLE_YES_NO:
				SetCheckState( yesno, 1 );
				break;
			case KEYPAD_STYLE_ENTER_CANCEL:
				SetCheckState( cancel_enter, 1 );
				break;
			case KEYPAD_STYLE_ENTER_CORRECT:
				SetCheckState( correct_enter, 1 );
				break;
			case KEYPAD_STYLE_ENTER_CLEAR:
				SetCheckState( clear_enter, 1 );
				break;
			case KEYPAD_STYLE_DOUBLE0_CLEAR:
				SetCheckState( enter_00, 1 );
				break;
			}
			// checkbox states....
			// checkbox_inverse
			// checkbox_correct_enter
			// checkbox_cancel_enter
			// checkbox_double0_clear
			//
		}
		
		DisplayFrameOver( frame, parent );
		CommonWait( frame );

		if( okay )
		{
			PSI_CONTROL color_well;
			color_well = GetControl( frame, COLOR_DISPLAY_BACKGROUND );
			KeypadSetDisplayBackground( keypad->keypad, GetColorFromWell( color_well ) );
			color_well = GetControl( frame, COLOR_DISPLAY_TEXT );
			KeypadSetDisplayTextColor( keypad->keypad, GetColorFromWell( color_well ) );

			color_well = GetControl( frame, COLOR_NUMKEYS );
			KeypadSetNumberKeyColor( keypad->keypad, GetColorFromWell( color_well ) );
			color_well = GetControl( frame, COLOR_ENTER );
			KeypadSetEnterKeyColor( keypad->keypad, GetColorFromWell( color_well ) );
			color_well = GetControl( frame, COLOR_CANCEL );
			KeypadSetC_KeyColor( keypad->keypad, GetColorFromWell( color_well ) );

			color_well = GetControl( frame, COLOR_NUMKEYS_TEXT );
			KeypadSetNumberKeyTextColor( keypad->keypad, GetColorFromWell( color_well ) );
			color_well = GetControl( frame, COLOR_ENTER_TEXT );
			KeypadSetEnterKeyTextColor( keypad->keypad, GetColorFromWell( color_well ) );
			color_well = GetControl( frame, COLOR_CANCEL_TEXT );
			KeypadSetC_KeyTextColor( keypad->keypad, GetColorFromWell( color_well ) );

			color_well = GetControl( frame, COLOR_KEYPAD_BACKGROUND );
			KeypadSetBackgroundColor( keypad->keypad, GetColorFromWell( color_well ) );

			style = (enum keypad_styles)0;

			if( GetCheckState( GetControl( frame, CHECKBOX_ALIGN_CENTER ) ) )
				style |= KEYPAD_DISPLAY_ALIGN_CENTER;
			if( GetCheckState( GetControl( frame, CHECKBOX_ALIGN_LEFT ) ) )
				style |= KEYPAD_DISPLAY_ALIGN_LEFT;

			if( invert && GetCheckState( invert ) )
				style |= KEYPAD_INVERT;
			if( password && GetCheckState( password ) )
				style |= KEYPAD_STYLE_PASSWORD;
			if( yesno && GetCheckState( yesno ) )
				style |= KEYPAD_STYLE_YES_NO;
			else if( enter_00 && GetCheckState( enter_00 ) )
				style |= KEYPAD_STYLE_DOUBLE0_CLEAR;
			else if( correct_enter && GetCheckState( correct_enter ) )
				style |= KEYPAD_STYLE_ENTER_CORRECT;
			else if( cancel_enter && GetCheckState( cancel_enter ) )
				style |= KEYPAD_STYLE_ENTER_CANCEL;
			else if( clear_enter && GetCheckState( clear_enter ) )
				style |= KEYPAD_STYLE_ENTER_CLEAR;
			if( style != prior_style )
				SetKeypadStyle( keypad->keypad, style );
		}
		{
			PSI_CONTROL pc;
			TEXTCHAR tmp[256];
			GetControlText( pc = GetControl( frame, EDIT_FORMAT ), tmp, 256 );
			if( pc )
				if( StrCmp( oldformat, tmp ) )
					KeypadSetDisplayFormat( keypad->keypad, tmp );

		}
		DestroyFrame( &frame );

	}

}

static uintptr_t OnEditControl( "Keypad 2" )( uintptr_t psv, PSI_CONTROL pc_parent )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;
	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, "ConfigureKeypad.isFrame" );

	if( frame )
	{
		PSI_CONTROL list;
		int done = 0;
		int okay = 0;
		SetCommonButtons( frame, &done, &okay );
		{
			list = GetControl( frame, LISTBOX_KEYPAD_TYPE );
			if( list )
			{
				int selected = 0;
				INDEX idx;
				CTEXTSTR name;
				PLISTITEM pli;
				pli = AddListItem( list, "- None" );

				LIST_FORALL( l.keypad_types, idx, CTEXTSTR, name )
				{
					PLISTITEM pli2;
					pli2 = AddListItem( list, name );
					if( StrCmp( name, keypad->keypad_type ) == 0 )
					{
						selected = 1;
						SetSelectedItem( list, pli2 );
					}
				}

				if( !selected )
					SetSelectedItem( list, pli );

			}
		}
		
		SetButtonPushMethod( GetControl( frame, BTN_EDIT_VISUAL ), EditVisualProperties, (uintptr_t)keypad );
		DisplayFrameOver( frame, pc_parent );
		CommonWait( frame );

		if( okay )
		{
			if( list )
			{
				PLISTITEM pli = GetSelectedItem( list );
				if( pli )
				{
					TEXTCHAR buffer[256];
					GetListItemText( pli, buffer, sizeof( buffer ) );

					if( keypad->keypad_type )
						Release( (POINTER)keypad->keypad_type );

					if( buffer[0] != '-' )
					{
						keypad->keypad_type = StrDup( buffer );
						KeypadSetAccumulator( keypad->keypad, buffer );

					}
					else
					{
						keypad->keypad_type = NULL;
					}
				}
			}
		}
		DestroyFrame( &frame );
	}
	return psv;
}

static uintptr_t OnCreateControl( "Keypad 2" )( PSI_CONTROL frame, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PPAGE_KEYPAD page_keypad = NULL;
	{

		if( !page_keypad )
		{
			page_keypad = New( PAGE_KEYPAD );
			page_keypad->page = ShellGetCurrentPage( frame );
			page_keypad->keypad_type = NULL; // no type... thereofre is default.
			page_keypad->keypad = NULL; // no control
			page_keypad->x = x;
			page_keypad->y = y;
			page_keypad->w = w;
			page_keypad->h = h;

			page_keypad->keypad = MakeKeypad( frame
												, page_keypad->x
												, page_keypad->y
												, page_keypad->w
												, page_keypad->h
												, KEYPAD_KEYS
												, KEYPAD_FLAG_ENTRY|KEYPAD_FLAG_DISPLAY
												, page_keypad->keypad_type
												);
			SetKeypadEnterEvent( page_keypad->keypad
									 , InvokeKeypadEnterEvent
									 , (uintptr_t)page_keypad );
			SetKeypadCancelEvent( page_keypad->keypad
									 , InvokeKeypadCancelEvent
									 , (uintptr_t)page_keypad );
			AddLink( &l.keypads, page_keypad );
		}

		else
			return 0;
	}

	return (uintptr_t)page_keypad;
}

static int OnChangePage( "Keypad 2" )( PSI_CONTROL pc_canvas )
{
	PPAGE_KEYPAD keypad;
	INDEX idx;
	LIST_FORALL( l.keypads, idx, PPAGE_KEYPAD, keypad )
	{
		ClearKeyedEntry( keypad->keypad );
	}
	return TRUE;
}

static PSI_CONTROL OnGetControl( "Keypad 2" )(uintptr_t psv )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;
	return keypad->keypad;
}


static void OnSaveControl( "Keypad 2" )( FILE *file, uintptr_t psv )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;
	sack_fprintf( file, "%sKeypad type='%s'\n", InterShell_GetSaveIndent(), keypad->keypad_type?keypad->keypad_type:"." );
	KeypadWriteConfig( file, InterShell_GetSaveIndent(), keypad->keypad );
	//sack_fprintf( file, "Keypad Option Go-Clear=%s", GetKeypadGoClear( keypad->keypad ) );
}

static uintptr_t CPROC MySetKeypadType( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;

	if( StrCmp( name, "." ) )
	{
		keypad->keypad_type = StrDup( name );
		KeypadSetAccumulator( keypad->keypad, name );
	}

	l.psv_read_keypad = (uintptr_t)keypad->keypad;

	return psv;
}

static void OnLoadControl( "Keypad 2" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "Keypad type='%m'", MySetKeypadType );
	KeypadSetupConfig( pch, &l.psv_read_keypad );
}

//-------------------------------------------------------------------------------------------

static uintptr_t OnEditControl( "Keyboard 2" )( uintptr_t psv, PSI_CONTROL pc_parent )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;
	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, "ConfigureKeypad.isFrame" );

	if( frame )
	{
		PSI_CONTROL list;
		int done = 0;
		int okay = 0;
		SetCommonButtons( frame, &done, &okay );
		{
			list = GetControl( frame, LISTBOX_KEYPAD_TYPE );
			if( list )
			{
				int selected = 0;
				INDEX idx;
				CTEXTSTR name;
				PLISTITEM pli;
				pli = AddListItem( list, "- None" );

				LIST_FORALL( l.keypad_types, idx, CTEXTSTR, name )
				{
					PLISTITEM pli2;
					pli2 = AddListItem( list, name );
					if( StrCmp( name, keypad->keypad_type ) == 0 )
					{
						selected = 1;
						SetSelectedItem( list, pli2 );
					}
				}

				if( !selected )
					SetSelectedItem( list, pli );

			}
		}
		
		SetButtonPushMethod( GetControl( frame, BTN_EDIT_VISUAL ), EditVisualProperties, (uintptr_t)keypad );
		DisplayFrameOver( frame, pc_parent );
		CommonWait( frame );

		if( okay )
		{
			if( list )
			{
				PLISTITEM pli = GetSelectedItem( list );
				if( pli )
				{
					TEXTCHAR buffer[256];
					GetListItemText( pli, buffer, sizeof( buffer ) );

					if( keypad->keypad_type )
						Release( (POINTER)keypad->keypad_type );

					if( buffer[0] != '-' )
					{
						keypad->keypad_type = StrDup( buffer );
						KeypadSetAccumulator( keypad->keypad, buffer );
						// builds the control attached to keypad thing for typename (only one per type)
					}

					else
					{
						keypad->keypad_type = NULL;
					}
				}
			}
		}
		DestroyFrame( &frame );
	}
	return psv;
}

static uintptr_t OnCreateControl( "Keyboard 2" )( PSI_CONTROL frame, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PPAGE_KEYPAD page_keypad = NULL;
	{
		//INDEX idx;
		//LIST_FORALL( l.keypads, idx, PPAGE_KEYPAD, page_keypad )
		//{
		//	if( page_keypad->page == ShellGetCurrentPage() )
		  //		break;
		//}

		if( !page_keypad )
		{
			page_keypad = New( PAGE_KEYPAD );
			page_keypad->page = ShellGetCurrentPage(frame);
			page_keypad->keypad_type = NULL; // no type... thereofre is default.
			page_keypad->keypad = NULL; // no control
			page_keypad->x = x;
			page_keypad->y = y;
			page_keypad->w = w;
			page_keypad->h = h;

			page_keypad->keypad = MakeKeypad( frame
												, page_keypad->x
												, page_keypad->y
												, page_keypad->w
												, page_keypad->h
												, KEYPAD_KEYS
												, KEYPAD_FLAG_DISPLAY|KEYPAD_FLAG_ALPHANUM
												, page_keypad->keypad_type
												);
			SetKeypadEnterEvent( page_keypad->keypad
									 , InvokeKeypadEnterEvent
									 , (uintptr_t)page_keypad );
			SetKeypadCancelEvent( page_keypad->keypad
									 , InvokeKeypadCancelEvent
									 , (uintptr_t)page_keypad );
			AddLink( &l.keypads, page_keypad );
		}

		else
			return 0;
	}

	return (uintptr_t)page_keypad;
}

static PSI_CONTROL OnGetControl( "Keyboard 2" )(uintptr_t psv )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;
	return keypad->keypad;
}


static void OnSaveControl( "Keyboard 2" )( FILE *file, uintptr_t psv )
{
	PPAGE_KEYPAD keypad = (PPAGE_KEYPAD)psv;
	sack_fprintf( file, "Keypad type='%s'\n", keypad->keypad_type?keypad->keypad_type:"." );
	KeypadWriteConfig( file, InterShell_GetSaveIndent(), keypad->keypad );
	//sack_fprintf( file, "Keypad Option Go-Clear=%s", GetKeypadGoClear( keypad->keypad ) );
}

static void OnLoadControl( "Keyboard 2" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "Keypad type='%m'", MySetKeypadType );
	KeypadSetupConfig( pch, &l.psv_read_keypad );
}

//-------------------------------------------------------------------------------------------

static void OnKeyPressEvent( "Keypad Hotkey 2" )( uintptr_t psv )
{
	// Because of the way this has to be created, this event has a funny
	// rule about its parameters...
	PHOTKEY hotkey = (PHOTKEY)(psv);
	//BannerMessage( "HAH!" );
	if( hotkey->flags.bNegative )
	{
		PSI_CONTROL keypad = GetKeypadOfType( hotkey->keypad_type );
		KeypadInvertValue( keypad );
	}
	else
	{
		PSI_CONTROL keypad = GetKeypadOfType( hotkey->keypad_type );
		KeyIntoKeypad( keypad, hotkey->value );
	}
}

static uintptr_t OnCreateMenuButton( "Keypad Hotkey 2" )( PMENU_BUTTON button )
{
	PHOTKEY hotkey = New( HOTKEY );
	MemSet( hotkey, 0, sizeof( *hotkey ) );
	hotkey->button = button;
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "10" );
	hotkey->value = 10;
	return (uintptr_t)hotkey;
}

static void CPROC PickHotkeyFont( uintptr_t psv, PSI_CONTROL pc )
{
	PHOTKEY hotkey = (PHOTKEY)psv;
	SFTFont *font = SelectACanvasFont( GetFrame( pc ), GetFrame( pc ), &hotkey->preset_name );

	//PickScaledFont( 0, 0, &g.width_scale, &g.height_scale, &page_label->fontdatalen, &page_label->fontdata, (PSI_CONTROL)GetFrame(pc) );
	if( font )
	{
		hotkey->new_font = font;
	}
}


static uintptr_t OnEditControl( "Keypad Hotkey 2" )( uintptr_t psv, PSI_CONTROL parent_frame )
{
	PHOTKEY hotkey = (PHOTKEY)psv;
	int okay = 0, done = 0;
	PSI_CONTROL frame = LoadXMLFrameOver( parent_frame, "hotkey_change_property.isframe" );

	if( frame )
	{
		SetCommonButtons( frame, &done, &okay );
		SetCommonButtonControls( frame );

		if( hotkey->flags.bNegative )
		{
			SetControlText( GetControl( frame, TXT_CONTROL_TEXT ), "-" );
		}

		else
		{
			TEXTCHAR value[32];
			snprintf( value, sizeof( value ), "%" _64fs, hotkey->value );
			SetControlText( GetControl( frame, TXT_CONTROL_TEXT ), value );
		}

		SetButtonPushMethod( GetControl( frame, BTN_PICKFONT ), PickHotkeyFont, psv );

		EditFrame( frame, TRUE );
		DisplayFrameOver( frame, parent_frame );
		CommonWait( frame );

		if( okay )
		{
			TEXTCHAR buffer[128];
			GetCommonButtonControls( frame );
			GetControlText( GetControl( frame, TXT_CONTROL_TEXT ), buffer, sizeof( buffer ) );
			hotkey->font = hotkey->new_font;
			InterShell_SetButtonFont( hotkey->button, hotkey->font );

			if( strcmp( buffer, "-" ) == 0 )
			{
				hotkey->flags.bNegative = TRUE;
			}
			
			else if( !sscanf( buffer, "%" _64fs, &hotkey->value ) )
				Banner2Message("It's No Good!" );

			UpdateButton( hotkey->button );
		}

		 DestroyFrame( &frame );
	}

	return psv;
}

static void OnSaveControl( "Keypad Hotkey 2" )( FILE *file, uintptr_t psv )
{
	PHOTKEY hotkey = (PHOTKEY)psv;

	if( hotkey )
	{

		if( hotkey->flags.bNegative )
			sack_fprintf( file, "%shotkey is negative sign\n", InterShell_GetSaveIndent() );
		else
			sack_fprintf( file, "%shotkey value=%Ld\n", InterShell_GetSaveIndent(), hotkey->value );
		sack_fprintf( file, "%shotkey target keypad type=%s\n", InterShell_GetSaveIndent(), hotkey->keypad_type );
	}
}

static uintptr_t CPROC SetHotkeyValue( uintptr_t psv, arg_list args )
{
	PARAM( args, uint64_t, value );
	PHOTKEY hotkey = (PHOTKEY)psv;

	if( hotkey )
	{
		hotkey->value = value;
	}

	return psv;
}

static uintptr_t CPROC SetHotkeyFontByName( uintptr_t psv, arg_list args )
{
	PHOTKEY hotkey = (PHOTKEY)psv;
	PARAM( args, TEXTCHAR *, name );
	hotkey->font = UseACanvasFont( InterShell_GetButtonCanvas( hotkey->button ), name );
	hotkey->preset_name = StrDup( name );
	return psv;
}

static uintptr_t CPROC SetHotkeyNegative( uintptr_t psv, arg_list args )
{
	PHOTKEY hotkey = (PHOTKEY)psv;
	hotkey->flags.bNegative = 1;
	return psv;
}

static uintptr_t CPROC SetHotkeyTarget( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PHOTKEY hotkey = (PHOTKEY)psv;
	
	if( hotkey->keypad_type )
	{
		Deallocate( TEXTSTR, hotkey->keypad_type );
		hotkey->keypad_type = NULL;
	}
	if( StrCmp( name, "." ) )
	{
		hotkey->keypad_type = StrDup( name );
	}

	return psv;
}

static void OnLoadControl( "Keypad Hotkey 2" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "hotkey font=%m", SetHotkeyFontByName );
	AddConfigurationMethod( pch, "hotkey value=%i", SetHotkeyValue );
	AddConfigurationMethod( pch, "hotkey is negative sign", SetHotkeyNegative );
	AddConfigurationMethod( pch, "hotkey target keypad type=%m", SetHotkeyTarget );
}


static void OnFixupControl( "Keypad Hotkey 2" )( uintptr_t psv )
{
	PHOTKEY hotkey = (PHOTKEY)psv;
	TEXTCHAR buffer[256];
	int len;

	InterShell_SetButtonFont( hotkey->button, hotkey->font );

	if( hotkey->flags.bNegative )
		InterShell_SetButtonText( hotkey->button, "-" );

	else
	{
		len = snprintf( buffer, sizeof(buffer), "%" _64fs, hotkey->value );
		buffer[len+1] = 0; // double null terminate
		InterShell_SetButtonText( hotkey->button, buffer );
	}
}

PUBLIC( void, CreateKeypadType )( CTEXTSTR name )
{
	AddLink( &l.keypad_types, StrDup( name ) );
}


PUBLIC( void, SetKeypadType )( PSI_CONTROL keypad, CTEXTSTR type )
{
	PPAGE_KEYPAD real_keypad;
	INDEX idx;
	LIST_FORALL( l.keypads, idx, PPAGE_KEYPAD, real_keypad )
		if( real_keypad->keypad == keypad )
			break;
	if( real_keypad )
	{
		if( real_keypad->keypad_type )
			Deallocate( TEXTSTR, (TEXTSTR)real_keypad->keypad_type );

		real_keypad->keypad_type = StrDup( type );
		KeypadSetAccumulator( real_keypad->keypad, type );
	}
}



PUBLIC( void, Keypad_AddMagicKeySequence )( PSI_CONTROL keypad, CTEXTSTR sequence, void (CPROC*event_proc)( uintptr_t ), uintptr_t psv_sequence )
{
	KeypadAddMagicKeySequence( keypad, sequence, event_proc, psv_sequence );
}