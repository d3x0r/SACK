//#define DEBUG_FONT_CREATION

#include <stdhdrs.h>

#include "intershell_local.h"
#include "resource.h"

#include "widgets/include/banner.h"
#include "fonts.h"

#include <psi.h>

INTERSHELL_NAMESPACE

extern CONTROL_REGISTRATION menu_surface;


PRELOAD( RegisterFontConfigurationIDs )
{
	EasyRegisterResource( WIDE( "intershell/font" ), BTN_PICKFONT, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE( "intershell/font" ), BTN_PICKFONT_PRICE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE( "intershell/font" ), BTN_PICKFONT_QTY, NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE( "intershell/font" ), BTN_EDITFONT, NORMAL_BUTTON_NAME );
}

typedef struct font_preset
{
	TEXTCHAR *name;
	SFTFont font;
	POINTER fontdata;
	size_t fontdatalen;
	SFTFont base_font;
	PLIST _font_theme;
	PLIST *font_theme;
} FONT_PRESET;

typedef struct font_select_tag
{
	// structure for font selection dialog...
	SFTFont font;
	POINTER *fontdata;
	size_t *fontdatalen;
	PFONT_PRESET selected_font;
	PSI_CONTROL canvas;
} FONT_SELECT, *PFONT_SELECT;

static struct intershell_font_local
{
	PLIST canvas;
#define l local_font_data
}local_font_data;


PFONT_PRESET _CreateAFont( PCanvasData canvas, CTEXTSTR name, SFTFont font, POINTER data, size_t datalen )
{
	PFONT_PRESET theme_font_preset = NULL;
	PFONT_PRESET font_preset;
	INDEX idx;
	TEXTSTR check_name = StrDup( name );
	TEXTSTR theme_check;
	int theme_index = 0;
#ifdef DEBUG_FONT_CREATION
	lprintf( "CreateAFont : %p (%s)", canvas, name );
#endif
	if( theme_check = (TEXTSTR)StrRChr( check_name, '.' ) )
	{
		theme_index = atoi( theme_check + 1 );
		if( theme_index != 0 )
		{
			theme_check[0] = 0;
			theme_check++;
		}
	}

#ifdef DEBUG_FONT_CREATION
	lprintf( "looking for %s", check_name );
#endif
	LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, font_preset )
	{
		if( StrCaseCmp( check_name, font_preset->name ) == 0 )
		{
			theme_font_preset = font_preset;
			font_preset = (PFONT_PRESET)GetLink( font_preset->font_theme, theme_index );
			break;
		}
	}

	if( !font_preset )
	{
		font_preset = New( FONT_PRESET );
		MemSet( font_preset, 0, sizeof( FONT_PRESET ) );
		font_preset->name = StrDup( name );
		if( theme_font_preset )
		{
			font_preset->font_theme = theme_font_preset->font_theme;
		}
		else
		{
			font_preset->font_theme = &font_preset->_font_theme;
		}
		SetLink( font_preset->font_theme, theme_index, font_preset );
		if( !canvas->fonts )
		{
			if( !font && !data )
			{
				font = RenderScaledFontEx( WIDE("Times New Roman"), 25, 35, &canvas->width_scale, &canvas->height_scale, 3, &font_preset->fontdatalen, &font_preset->fontdata );
			}
		}
		if( !theme_index )
			AddLink( &canvas->fonts, font_preset );
	}
	else
		DestroyFont( &font_preset->font );
	if( datalen )
	{
		font_preset->fontdata = NewArray( _8, datalen );
		MemCpy( font_preset->fontdata, data, datalen );
		font_preset->fontdatalen = datalen;
	}

	if( font )
		font_preset->font = font;
	else if( font_preset->fontdata )
		font_preset->font = RenderScaledFontData( (struct font_data_tag *)font_preset->fontdata
															 , &canvas->width_scale
															 , &canvas->height_scale );
	else
	{
#ifdef DEBUG_FONT_CREATION
		lprintf( WIDE("Font was %p... and so it stays."), font_preset->font );
#endif
		if( theme_font_preset )
		{
#ifdef DEBUG_FONT_CREATION
			lprintf( WIDE("Actually update this one's font to %p"), theme_font_preset->font );
#endif
			font_preset->font = theme_font_preset->font;
		}
		//font_preset->font = NULL;
	}

	font_preset->base_font = font_preset->font;

	return font_preset;
}

SFTFont *CreateACanvasFont2( PSI_CONTROL pc_canvas, CTEXTSTR name, CTEXTSTR fontfilename, int sizex, int sizey )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	PFONT_PRESET theme_font_preset = NULL;
	PFONT_PRESET font_preset;
	INDEX idx;
	TEXTSTR check_name = StrDup( name );
	TEXTSTR theme_check;
	int theme_index = 0;
#ifdef DEBUG_FONT_CREATION
	lprintf( "Font %s = %s(%d,%d)", name, fontfilename, sizex, sizey );
#endif
	if( theme_check = (TEXTSTR)StrRChr( check_name, '.' ) )
	{
		theme_index = atoi( theme_check + 1 );
		if( theme_index != 0 )
		{
			theme_check[0] = 0;
			theme_check++;
		}
	}

	LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, font_preset )
	{
		if( StrCaseCmp( check_name, font_preset->name ) == 0 )
		{
#ifdef DEBUG_FONT_CREATION
			lprintf( "found existing name:%s", check_name );
#endif
			theme_font_preset = font_preset;
			font_preset = (PFONT_PRESET)GetLink( font_preset->font_theme, theme_index );
			break;
		}
	}

	if( !font_preset )
	{
#ifdef DEBUG_FONT_CREATION
		lprintf( "Create a new thing..." );
#endif
		font_preset = New( FONT_PRESET );
		MemSet( font_preset, 0, sizeof( FONT_PRESET ) );
		font_preset->name = StrDup( name );
		if( theme_font_preset )
		{
			font_preset->font_theme = theme_font_preset->font_theme;
		}
		else
		{
			font_preset->font_theme = &font_preset->_font_theme;
		}
		SetLink( font_preset->font_theme, theme_index, font_preset );
		if( !theme_index )
			AddLink( &canvas->fonts, font_preset );
	}
	else
		return &font_preset->font;

#ifdef DEBUG_FONT_CREATION
	lprintf( "Rendering %s", fontfilename );
#endif
	font_preset->font = RenderFontFileScaledEx( fontfilename
															, sizex, sizey
															, &canvas->width_scale
															, &canvas->height_scale
															, 3//FONT_FLAG_8BIT
															, NULL, NULL
															);

	font_preset->base_font = font_preset->font;

	return &font_preset->font;
}



static void CPROC EditPageFont(PTRSZVAL psv, PSI_CONTROL pc )
{
	PFONT_SELECT font_select = (PFONT_SELECT)psv;
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, font_select->canvas );
	if( font_select->selected_font )
	{
		POINTER tmp = font_select->selected_font->fontdata;
		size_t tmplen = font_select->selected_font->fontdatalen;
		SFTFont font = PickScaledFont( 0, 0
										  , &canvas->width_scale, &canvas->height_scale
										  , &tmplen //font_select->fontdata
										  , &tmp //font_select->fontdatalen
										  , (PCOMMON)GetFrame(pc) );
		if( font )
		{
			font_select->selected_font->font = font;
			font_select->selected_font->fontdata = tmp;
			font_select->selected_font->fontdatalen = tmplen;
		}

	}
}

SFTFont* CreateACanvasFont( PSI_CONTROL pc_canvas, CTEXTSTR name, SFTFont font, POINTER data, size_t datalen )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	PFONT_PRESET preset;
	preset = _CreateAFont( canvas, name, font, data, datalen );
	if( preset )
		return &preset->font;
	return NULL;
}

void CPROC SetCurrentPreset( PTRSZVAL psv, PSI_CONTROL list, PLISTITEM pli )
{
	PFONT_SELECT font_select = (PFONT_SELECT)psv;

	font_select->selected_font = (PFONT_PRESET)GetItemData( pli );
}

static void CPROC CreatePageFont( PTRSZVAL psv, PSI_CONTROL pc )
{
	PFONT_SELECT font_select = (PFONT_SELECT)psv;
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, font_select->canvas );
	TEXTCHAR name_buffer[256];
	//if( !font_select->selected_font )
	{
		if( !SimpleUserQuery(  name_buffer, sizeof( name_buffer )
								  , WIDE("Enter new font preset name"), GetFrame( pc ) ) )
			return;
		{
			PFONT_PRESET font_preset;
			INDEX idx;
			LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, font_preset )
			{
				if( StrCaseCmp( font_preset->name, name_buffer ) == 0 )
				{
					Banner2Message( WIDE("Font name already exists") );
					return;
				}
			}
		}
	}
	{
		POINTER tmp = NULL;
		size_t tmplen = 0;
		SFTFont font = PickScaledFont( 0, 0
										  , &canvas->width_scale, &canvas->height_scale
										  , &tmplen //font_select->fontdata
										  , &tmp //font_select->fontdatalen
										  , (PCOMMON)GetFrame(pc) );
		if( font )
		{
			PLISTITEM pli;
			PFONT_PRESET font_preset;
			font_preset =  _CreateAFont( canvas, name_buffer
																, font
																, tmp
																, tmplen );
			font_select->fontdata = &font_preset->fontdata;
			font_select->fontdatalen = &font_preset->fontdatalen;
			pli= AddListItem( GetNearControl( pc, LST_FONTS ), font_preset->name );
			SetItemData( pli, (PTRSZVAL)font_preset );
			SetSelectedItem( GetNearControl( pc, LST_FONTS ), pli );
		}
	}
}

SFTFont * UseACanvasFont( PSI_CONTROL pc_canvas, CTEXTSTR name )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	PFONT_PRESET font_preset;
	INDEX idx;
	if( !name )
		name = WIDE( "Default" );
	if( FindLink( &l.canvas, canvas ) == INVALID_INDEX )
		AddLink( &l.canvas, canvas );
	LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, font_preset )
	{
		if( !StrCaseCmp( font_preset->name, name ) )
		{
			if( !font_preset->font )
				font_preset->font = RenderScaledFontData( (struct font_data_tag *)font_preset->fontdata
																	 , &canvas->width_scale
																	 , &canvas->height_scale );
			return &font_preset->font;
		}
	}
#ifdef DEBUG_FONT_CREATION
	lprintf( "UseACanvasFont..." );
#endif
	return CreateACanvasFont( pc_canvas, name, NULL, NULL, 0 );
//	return NULL;
}


// default name for having already chosen a font preset...
// result is a pointer to the preset's name.
SFTFont *SelectACanvasFont( PSI_CONTROL pc_canvas, PSI_CONTROL parent, CTEXTSTR*default_name )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	FONT_SELECT font_select;
	PSI_CONTROL frame;
	int okay = 0;
	int done = 0;
	font_select.selected_font = NULL;
	font_select.canvas = pc_canvas;
	//font_select.fontdata = pfontdata;
	//font_select.fontdatalen = pfontdatalen;
	frame = LoadXMLFrame( WIDE("font_preset_property.isframe") );
	if( frame )
	{

		SetCommonButtons( frame, &done, &okay );
		{
			//could figure out a way to register methods under
			PSI_CONTROL list = GetControl( frame, LST_FONTS );
			SetButtonPushMethod( GetControl( frame, BTN_PICKFONT ), CreatePageFont, (PTRSZVAL)&font_select );
			SetButtonPushMethod( GetControl( frame, BTN_EDITFONT ), EditPageFont, (PTRSZVAL)&font_select );
			if( list )
			{
 				PFONT_PRESET font_preset;
				INDEX idx;
				SetListboxIsTree( list, TRUE );
				LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, font_preset )
				{
					PFONT_PRESET theme_font_preset;
					INDEX idx2;
					PLISTITEM pli = AddListItem( list, font_preset->name );
					SetItemData( pli, (PTRSZVAL)font_preset );
					LIST_FORALL( (*font_preset->font_theme), idx2, PFONT_PRESET, theme_font_preset )
					{
						if( !idx2 )
							continue;
						SetItemData( AddListItemEx( list, 1, theme_font_preset->name )
							, (PTRSZVAL)theme_font_preset );
					}
					if( default_name && default_name[0] && StrCaseCmp( font_preset->name, default_name[0] )==0 )
					{
						SetSelectedItem( list, pli );
						font_select.selected_font = font_preset;
					}
				}
				SetSelChangeHandler( list, SetCurrentPreset, (PTRSZVAL)&font_select );
			}
		}
		DisplayFrameOver( frame, parent );
		//EditFrame( frame, TRUE );
		CommonWait( frame );
		if( !okay )
		{
			font_select.selected_font = NULL;
		}
		DestroyFrame( &frame );
		if( font_select.selected_font )
		{
			if( default_name )
			{
				if( default_name[0] )
					Release( (POINTER)default_name[0] );
				default_name[0] = StrDup( font_select.selected_font->name );
			}
			return &font_select.selected_font->font;
		}
	}
	return NULL;
}


static void OnSaveCommon( WIDE( "Common Fonts" ) )( FILE *out )
{
	PCanvasData canvas = g.current_saving_canvas;
	PFONT_PRESET preset;
	INDEX idx;
	LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, preset )
	{
		INDEX idx2;
		PFONT_PRESET theme_preset;
		LIST_FORALL( (*preset->font_theme), idx2, PFONT_PRESET, theme_preset )
		{
			TEXTCHAR *data;
			if( theme_preset->fontdata && theme_preset->fontdatalen )
			{
            TEXTSTR tmp;
				//EncodeBinaryConfig( &data, theme_preset->fontdata, theme_preset->fontdatalen );
				sack_fprintf( out, WIDE("font preset %s=%s\n")
						 , theme_preset->name
						 , tmp = EscapeMenuString( theme_preset->fontdata )/*data*/ );
				//Release( data );
			}
			// if there's no data, don't bother saving anything.
			//else
			//	sack_fprintf( out, WIDE("font preset %s={}\n"), theme_preset->name );
		}
	}
}


static PTRSZVAL CPROC RecreateFont( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	//PARAM( args, size_t, length );
	PARAM( args, CTEXTSTR, data );
	size_t length;
	if( data[0] == '[' )
	{
		POINTER pData;
		DecodeBinaryConfig( data, &pData, &length );
      data = (CTEXTSTR)pData;
	}
	else
      length = StrLen( data ) + 1;
#ifdef DEBUG_FONT_CREATION
	lprintf( "RecreateFont..." );
#endif
	CreateACanvasFont( InterShell_GetCurrentLoadingCanvas(), name, NULL, (POINTER)data, length );
	return 0;
}

static void OnLoadCommon( WIDE( "Common Fonts" ) )( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, WIDE("font preset %m=%m"), RecreateFont );
}

static void OnThemeAdded( WIDE( "Fonts" ) )( int theme_id )
{
	INDEX idx;
	PFONT_PRESET preset;
	PCanvasData canvas;
	INDEX idx_canvas;
	LIST_FORALL( l.canvas, idx_canvas, PCanvasData, canvas )
	{
		LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, preset )
		{
			TEXTCHAR buf[256];
			PFONT_PRESET theme_preset = (PFONT_PRESET)GetLink( preset->font_theme, theme_id );
			if( !theme_preset )
			{
				snprintf( buf, sizeof( buf ), WIDE("%s.%d"), preset->name, theme_id );
#ifdef DEBUG_FONT_CREATION
				lprintf( "Theme Added..." );
#endif
				CreateACanvasFont( canvas->pc_canvas, buf, preset->font, NULL, 0 );
			}
		}
	}
}

void UpdateFontTheme( PCanvasData canvas, int theme_id )
{
	INDEX idx;
	PFONT_PRESET preset;
	LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, preset )
	{
		PFONT_PRESET theme_preset = (PFONT_PRESET)GetLink( preset->font_theme, theme_id );
		if( theme_id && theme_preset )
			preset->font = theme_preset->font;
		else
			preset->font = preset->base_font;
	}
}

void UpdateFontScaling( PCanvasData canvas )
{
	PFONT_PRESET font_preset;
	INDEX idx;
#ifdef DEBUG_FONT_CREATION
	lprintf( WIDE("Updating font scaling....") );
#endif
	LIST_FORALL( canvas->fonts, idx, PFONT_PRESET, font_preset )
	{
		INDEX theme_idx;
		PFONT_PRESET theme_font;
#ifdef DEBUG_FONT_CREATION
		lprintf( WIDE("Rescale font %p"), font_preset->base_font );
#endif
		RerenderFont( font_preset->base_font
						, 0, 0
						, &canvas->width_scale, &canvas->height_scale );
		LIST_FORALL( (*font_preset->font_theme), theme_idx, PFONT_PRESET, theme_font )
		{
			RerenderFont( theme_font->font
							, 0, 0
							, &canvas->width_scale, &canvas->height_scale );

		}
	}
}

INTERSHELL_NAMESPACE_END

