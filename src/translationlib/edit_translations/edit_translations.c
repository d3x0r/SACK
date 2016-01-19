#define DEFINE_DEFAULT_RENDER_INTERFACE

#include <stdhdrs.h>
#include <controls.h>
#include <translation.h>

SACK_NAMESPACE
/* Namespace of custom math routines.  Contains operators
for Vectors and fractions. */
_TRANSLATION_NAMESPACE

static struct local
{
	PTranslation selected_translation;
	INDEX selected_string;
	PLISTITEM pli_selected_string;
	PSI_CONTROL frame;
	PSI_CONTROL pc_translations;
	PSI_CONTROL pc_add_translation;
	PSI_CONTROL pc_edit_translated_text;
	PSI_CONTROL pc_translation_text;
	PSI_CONTROL pc_add_translatable;
	PSI_CONTROL pc_save_translation;
	PSI_CONTROL pc_load_translation;
	PSI_CONTROL pc_update_text;
	PLIST index_strings;
}l;

static void AddTranslation( PTRSZVAL psv, PSI_CONTROL pc )
{
	TEXTCHAR name[256];
	if( SimpleUserQuery( name, 256, WIDE("Enter Translation Name"), NULL ) )
	{
		PLISTITEM pli;
		PTranslation translation = CreateTranslation( name );
		pli = AddComboBoxItem( l.pc_translations, name );
		SetItemData( pli, (PTRSZVAL)translation );
	}
}

static void FillBaseInformation( void )
{
		PLISTITEM pli;
		PTranslation translation;
		INDEX idx;
		PLIST translations;

		translations = GetTranslations();
		ResetComboBox( l.pc_translations );
		AddComboBoxItem( l.pc_translations, WIDE("Native") );
		LIST_FORALL( translations, idx, PTranslation, translation )
		{
			CTEXTSTR name = GetTranslationName( translation );
			pli = AddComboBoxItem( l.pc_translations, name );
			SetItemData( pli, (PTRSZVAL)translation );
		}
}

static void SaveTranslation( PTRSZVAL psv, PSI_CONTROL pc )
{
	SaveTranslationData();
}

static void LoadTranslation( PTRSZVAL psv, PSI_CONTROL pc )
{
	LoadTranslationData();
	FillBaseInformation();
}

static void UpdateTranslation( PTRSZVAL psv, PSI_CONTROL pc )
{
	TEXTCHAR buf[256];
	GetControlText( l.pc_edit_translated_text, buf, 256 );
	SetTranslatedString( l.selected_translation, l.selected_string + 1, buf );
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT out;
		vtprintf( pvt, WIDE("%s\t%s"), GetLink( &l.index_strings, l.selected_string ), buf );
		out = VarTextGet( pvt );
		SetItemText( l.pli_selected_string, GetText( out ) );
		LineRelease( out );
		VarTextDestroy( &pvt );
	}
}


static void UpdateTranslatedStrings( void )
{
	PLIST strings;
	PLIST translation_strings;
	CTEXTSTR string;
	CTEXTSTR translation_string;
	INDEX idx;
	PVARTEXT pvt = VarTextCreate();
	ResetList( l.pc_translation_text );
	l.index_strings = strings = GetTranslationIndexStrings( );
	if( l.selected_translation )
		translation_strings = GetTranslationStrings( l.selected_translation );
	else
		translation_strings = NULL;
	LIST_FORALL( strings, idx, CTEXTSTR, string )
	{
		PTEXT out;
		PLISTITEM pli;
		if( translation_strings )
		{
			translation_string = (CTEXTSTR)GetLink( &translation_strings, idx );
			vtprintf( pvt, WIDE("%s\t%s"), string, translation_string );
			out = VarTextGet( pvt );
			pli = AddListItem( l.pc_translation_text, GetText( out ) );
			SetItemData( pli, (PTRSZVAL)idx );
			LineRelease( out );
		}
		else
		{
			pli = AddListItem( l.pc_translation_text, string );
			SetItemData( pli, (PTRSZVAL)idx );
		}
	}
	VarTextDestroy( &pvt );
}

static void CPROC TranslationSelected( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PTRSZVAL psvItem = GetItemData( pli );
	l.selected_translation = (PTranslation)psvItem;
	if( l.selected_translation )
		SetCurrentTranslation( GetTranslationName( l.selected_translation ) );
	else
		SetCurrentTranslation( NULL );
	UpdateTranslatedStrings();
}

static void AddTranslatable( PTRSZVAL psv, PSI_CONTROL pc )
{
	TEXTCHAR name[256];
	if( SimpleUserQuery( name, 256, WIDE("Enter Translatable String"), NULL ) )
	{
		TranslateText( name );
		UpdateTranslatedStrings();
	}
}

static void CPROC SelectString( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	if( l.selected_translation )
	{
		TEXTCHAR buf[256];
		TEXTSTR sep;
		GetItemText( pli, 256, buf );
		sep = StrChr( buf, '\t' );
		if( sep )
			sep[0] = 0;
		l.selected_string = GetItemData( pli );
		l.pli_selected_string = pli;
		SetControlText( l.pc_edit_translated_text, TranslateText( buf ) );
	}
}

static void InitFrame( void )
{
	if( !l.frame )
	{
		PLIST translations;
		l.frame = CreateFrame( WIDE("Edit Translations"), 0, 0, 900, 500, 0, NULL );
		l.pc_translations = MakeNamedControl( l.frame, COMBOBOX_CONTROL_NAME, 5, 5, 250, 21, 0 );
		SetComboBoxSelChangeHandler( l.pc_translations, TranslationSelected, 0 );
		l.pc_add_translation = MakeNamedCaptionedControl( l.frame, NORMAL_BUTTON_NAME, 260, 5, 120, 21, 0, WIDE("Add Translation") );
		SetButtonPushMethod( l.pc_add_translation, AddTranslation, 0 );
		l.pc_add_translatable = MakeNamedCaptionedControl( l.frame, NORMAL_BUTTON_NAME, 385, 5, 120, 21, 0, WIDE("Add Translatable") );
		SetButtonPushMethod( l.pc_add_translatable, AddTranslatable, 0 );
		l.pc_save_translation = MakeNamedCaptionedControl( l.frame, NORMAL_BUTTON_NAME, 510, 5, 60, 21, 0, WIDE("Save") );
		SetButtonPushMethod( l.pc_save_translation, SaveTranslation, 0 );
		l.pc_load_translation = MakeNamedCaptionedControl( l.frame, NORMAL_BUTTON_NAME, 575, 5, 60, 21, 0, WIDE("Reload") );
		SetButtonPushMethod( l.pc_load_translation, LoadTranslation, 0 );
		l.pc_edit_translated_text = MakeNamedControl( l.frame, EDIT_FIELD_NAME, 5, 31, 820, 21, 0 );
		l.pc_update_text = MakeNamedCaptionedControl( l.frame, NORMAL_BUTTON_NAME, 830, 31, 60, 21, 0, WIDE("Update") );
		SetButtonPushMethod( l.pc_update_text, UpdateTranslation, 0 );
		l.pc_translation_text = MakeNamedControl( l.frame, LISTBOX_CONTROL_NAME, 5, 57, 890, 400, 0 );
		SetSelChangeHandler( l.pc_translation_text, SelectString, 0 );
		{
			int stops[2] = { 0, 445 };
			SetListBoxTabStops( l.pc_translation_text, 2, stops );
		}
		translations = GetTranslations();
		FillBaseInformation();
	}
}

#ifdef EDITTRANSLATION_PLUGIN
PUBLIC( int, EditTranslations )
#else
int EditTranslations
#endif
                  ( PSI_CONTROL parent )
{
	InitFrame();
	if( !RenderIsInstanced() )
	{
		DisplayFrameOver( l.frame, parent );
#ifndef EDITTRANSLATION_PLUGIN
		CommonWait( l.frame );
		DestroyFrame( &l.frame );
#endif
	}
	else
	{
		while( 1 )
		{
			WakeableSleep( 1000000 );
		}
	}
	return 0;
}

/* Namespace of custom math routines.  Contains operators
for Vectors and fractions. */
  _TRANSLATION_NAMESPACE_END
SACK_NAMESPACE_END

#ifndef EDITTRANSLATION_PLUGIN

SaneWinMain( argc, argv )
{
	EditTranslations( NULL );
}
EndSaneWinMain()

#endif
