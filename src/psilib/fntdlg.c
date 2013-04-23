#include <stdhdrs.h>
#include <sharemem.h>
#include <filesys.h>

#include <ft2build.h>
#ifdef FT_FREETYPE_H

#include FT_FREETYPE_H

#include "global.h"

#include "../imglib/fntglobal.h"
#include <controls.h>
#include <psi.h>

#include "resource.h"
PSI_FONTS_NAMESPACE
#define DIALOG_WIDTH 420+50
#define DIALOG_HEIGHT 300+200

enum {
	LST_FONT_NAMES = 100
	  , LST_FONT_STYLES
	  , LST_FONT_SIZES
	  , CHK_MONO_SPACE
	  , CHK_PROP_SPACE
	  , CHK_BOTH_SPACE
	  , CHK_MONO
	  , CHK_2BIT
	  , CHK_8BIT
	  , CST_FONT_SAMPLE
	  , CST_CHAR_SIZE
	  , SLD_WIDTH
	  , SLD_HEIGHT
};

typedef struct font_dialog_tag
{
	struct {
		_32 show_mono_only : 1;
		_32 show_prop_only : 1;
		_32 showing_scalable : 1;
		_32 render_depth : 2;
	} flags;
	PSI_CONTROL pFrame;
	PSI_CONTROL pSample, pHorSlider, pVerSlider;
	SFTFont pFont;   // temp font for drawing sample
	int done, okay;
	PFONT_ENTRY pFontEntry;
	PFONT_STYLE pFontStyle;
	PSIZE_FILE  pSizeFile;
	TEXTCHAR *filename;
	S_32 nWidth, nSliderWidth;
	S_32 nHeight, nSliderHeight;
	PFRACTION width_scale, height_scale;
	void (CPROC* Update)(PTRSZVAL,SFTFont);
	PTRSZVAL psvUpdate;
} FONT_DIALOG, *PFONT_DIALOG;

//PUBLIC_DATA( WIDE("Font Choice Data"), FONTDIALOG, InitFontDialog, NULL );

//#undef fg
//#define fg (*GetGlobalFonts())
//-------------------------------------------------------------------------

PRIORITY_PRELOAD( InitFontDialogGlobal, IMAGE_PRELOAD_PRIORITY + 2 )
{
   // static variable.
#define fg (*global_font_data)
	SimpleRegisterAndCreateGlobal( global_font_data );
	if( !fg.library )
	{
      lprintf( WIDE( "image library didn't load?" ) );
	}
}

typedef struct font_info_tag
{
	// should contain enough information to
	// recreate any font selected with the dialog...
 char unused;	
} FONT_INFO, *PFONT_INFO;

//-------------------------------------------------------------------------

static int HasMonoSpacing( PFONT_ENTRY pfe )
{
	INDEX idx;
	PFONT_STYLE pfs;
   for( idx = 0; pfs = pfe->styles + idx, idx < pfe->nStyles; idx++ )
	{
		if( pfs->flags.mono )
         return TRUE;
	}
   return FALSE;
}

//-------------------------------------------------------------------------

static int HasPropSpacing( PFONT_ENTRY pfe )
{
	INDEX idx;
   PFONT_STYLE pfs;
   for( idx = 0; pfs = pfe->styles + idx, idx < pfe->nStyles; idx++ )
	{
		if( !pfs->flags.mono )
         return TRUE;
	}
   return FALSE;
}

//-------------------------------------------------------------------------

static int CPROC UpdateSample( PCOMMON pc )
{
	Image surface = GetControlSurface( pc );
	PFONT_DIALOG pfd = (PFONT_DIALOG)GetFrameUserData( GetFrame( pc ) );
	if( pfd->pFont )
	{
		if( pfd->Update )
			pfd->Update( pfd->psvUpdate, pfd->pFont );
		ClearImageTo( surface, Color( 92, 91, 42 ) );
		PutStringFont( surface
						 , 5, 5
						 , Color( 255,255,255 ), Color( 82, 82, 82 )
						 , WIDE("The Quick Brown Fox")
						 , pfd->pFont );
	}
   return 1;
}

//-------------------------------------------------------------------------

int UpdateSampleFont( PFONT_DIALOG pfd )
{
	if( pfd->pFont )
	{
		UnloadFont( pfd->pFont );
		pfd->pFont = NULL;
	}
	if( pfd->pFontEntry && pfd->pFontStyle && pfd->pSizeFile )
	{
		// need to build this into a structure that can be resulted to render a font
		pfd->pFont = InternalRenderFont( (_32)(pfd->pFontEntry - fg.pFontCache)
												 , (_32)(pfd->pFontStyle - pfd->pFontEntry->styles)
												 , (_32)(pfd->pSizeFile - pfd->pFontStyle->files)
												 , pfd->nWidth
												 , pfd->nHeight
												 , (pfd->width_scale)
												 , (pfd->height_scale)
												 , pfd->flags.render_depth );
		if( !pfd->pFont )
		{
         return FALSE;
		}
		SmudgeCommon( pfd->pSample );
	}
	else
	{
		pfd->pFont = RenderFontFile( pfd->filename
											, pfd->nWidth
											, pfd->nHeight
											, pfd->flags.render_depth );
		SmudgeCommon( pfd->pSample );
	}
   return TRUE;
}


//-------------------------------------------------------------------------

static int CPROC DrawCharacterSize( PCOMMON pc )
{
	int x, y;
   _32 width, height;
	PFONT_DIALOG pfd = (PFONT_DIALOG)GetFrameUserData( GetFrame( pc ) );
	Image surface = GetControlSurface( pc );
   lprintf( WIDE("Drawing character size control...") );
	ClearImageTo( surface, Color( 92, 91, 42 ) );
   if( pfd->width_scale )
		x = (surface->width - (width = ScaleValue( pfd->width_scale, pfd->nSliderWidth ) ) ) / 2;
   else 
		x = (surface->width - (width = pfd->nSliderWidth) ) / 2;
	if( pfd->height_scale )
		y = (surface->height - (height = ScaleValue( pfd->height_scale, pfd->nSliderHeight) ) ) / 2;
	else
		y = (surface->height - (height = pfd->nSliderHeight) ) / 2;
	BlatColor( surface, x, y
				, width
				, height, Color( 0, 0, 0 ) );
	if( pfd->flags.showing_scalable )
		UpdateSampleFont( pfd );
   return 1;
}

//-------------------------------------------------------------------------

void CPROC UpdateCharRect( PTRSZVAL psv, PSI_CONTROL pc, int val )
{
   PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	if( GetControlID( pc ) == SLD_WIDTH )
	{
		pfd->nSliderWidth = val;
		if( pfd->flags.showing_scalable )
			pfd->nWidth = pfd->nSliderWidth;
	}
	else if( GetControlID( pc ) == SLD_HEIGHT )
	{
		pfd->nSliderHeight = -val;
		if( pfd->flags.showing_scalable )
         pfd->nHeight = pfd->nSliderHeight;
	}
	lprintf( WIDE("Updated slider value - update control...") );
   SmudgeCommon( GetNearControl( pc, CST_CHAR_SIZE ) );
}

//-------------------------------------------------------------------------

static void CPROC SizeSelected( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	PSIZE_FILE psf = (PSIZE_FILE)GetItemData( pli );
	pfd->pSizeFile = psf;
	if( psf )
	{
		TEXTCHAR size[15], *tmp;
		S_32 width, height;
		GetListItemText( pli, size, sizeof( size ) );
		if( ( tmp = strchr( size, '(' ) ) )
			*tmp = 0;
#ifdef __cplusplus_cli
	char *mybuf = CStrDup( size );
#define SCANBUF mybuf
#else
#define SCANBUF size
#endif
#ifdef UNICODE
	swscanf
#else
	sscanf
#endif
		( size, WIDE("%") _32f WIDE("x%") _32f WIDE(""), &width, &height );
#ifdef __cplusplus_cli
	Release( mybuf );
#endif
	// need to pull this info from the size sliiders...
	if( width == -1 )
	{
		pfd->flags.showing_scalable = 1;
		pfd->nWidth = pfd->nSliderWidth;
		pfd->nHeight = pfd->nSliderHeight;
	}
	else
	{
		pfd->flags.showing_scalable = 0;
		pfd->nWidth = width;
		pfd->nHeight = height;
	}
	if( !UpdateSampleFont( pfd ) )
	{
		DeleteListItem( pc, pli );
		if( !GetNthItem( pc, 0 ) )
		{
			PCOMMON pcStyle;
			lprintf( WIDE("------------------------------------------------") );
			lprintf( WIDE("setting size_file as unuable.... rendering got us no data.") );
			psf->flags.unusable = 1;
			pcStyle = GetNearControl( pc, LST_FONT_STYLES );
			lprintf( WIDE("Selecting tyle...") );
			SetSelectedItem( pcStyle, GetSelectedItem( pcStyle ) );
		}
	}
	}
	else
	{
		pfd->nWidth = pfd->nSliderWidth;
		pfd->nHeight = pfd->nSliderHeight;
		pfd->flags.showing_scalable = 1;
		UpdateSampleFont( pfd );
	}
}

//-------------------------------------------------------------------------

static void CPROC StyleSelected( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	PSI_CONTROL pcSizes;
	PFONT_STYLE pfs = (PFONT_STYLE)GetItemData( pli );
	ResetList( pcSizes = GetNearControl( pc, LST_FONT_SIZES ) );
	pfd->pFontStyle = pfs;
	if( pfs )
	{
		INDEX idx;

		PSIZE_FILE psf;
		int bAdded = 0;

		lprintf( WIDE("Style selected, filling in sizes...") );
		for( idx = 0; psf = pfs->files + idx, idx < pfs->nFiles; idx++ )
		{
			TEXTCHAR entry[12];
			PLISTITEM hli;
			PSIZES size;
			INDEX idx2;
			if( psf->flags.unusable )
				continue;
			for( idx2 = 0; idx2 < psf->nSizes; idx2++ )
			{
				size = psf->sizes + idx2;
				if( size->flags.unusable )
				{
					lprintf( WIDE("Size is unusable...") );
					continue;
				}
				if( pfd->flags.show_mono_only ||
					pfd->flags.show_prop_only )
				{
					snprintf( entry, 12, WIDE("%dx%d")
							  , size->width, size->height );
				}
				else
				{
					snprintf( entry, 12, WIDE("%dx%d%s")
							  , size->width, size->height
							  , (pfs->flags.mono)?WIDE("(m)"):WIDE("") );
				}
				bAdded++;
				hli =	AddListItem( pcSizes, entry );
				SetItemData( hli, (PTRSZVAL)psf );
			}
		}
		if( !bAdded )
		{
			PCOMMON pcFamily;
			lprintf( WIDE("Hmm had no sizes, have to remove my own item also.") );
			pfs->flags.unusable = 1;
			DeleteListItem( pc, pli );
			pcFamily = GetNearControl( pc, LST_FONT_NAMES );
			SetSelectedItem( pcFamily, GetSelectedItem( pcFamily ) );
			return;
		}
		SetSelectedItem( pcSizes, GetNthItem( pcSizes, 0 ) );
	}
	else
	{
		SetSelectedItem( pcSizes, AddListItem( pcSizes, WIDE("only size") ) );
	}
}

//-------------------------------------------------------------------------

static void CPROC FamilySelected( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PFONT_ENTRY pfe = (PFONT_ENTRY)GetItemData( pli );
   PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	INDEX idx;
   int selected = 0;
	PFONT_STYLE pfs;
	PSI_CONTROL pcStyle;
	int bAdded = 0;
	ResetList( pcStyle = GetNearControl( pc, LST_FONT_STYLES ) );
	DisableUpdateListBox( GetNearControl( pc, LST_FONT_SIZES ), TRUE );
	DisableUpdateListBox( pcStyle, TRUE );
	pfd->pFontEntry = pfe;
	if( pfe )
	{
		for( idx = 0; pfs = pfe->styles + idx, idx < pfe->nStyles; idx++ )
		{
			PLISTITEM hli;
			if( pfs->flags.unusable )
				continue;
			if( pfs->flags.mono && pfd->flags.show_prop_only )
				continue;
			if( !pfs->flags.mono && pfd->flags.show_mono_only )
				continue;
			bAdded++;
			hli = AddListItem( pcStyle, pfs->name );
			SetItemData( hli, (PTRSZVAL)pfs );
			if( pfd->pFontStyle == pfs )
			{
				SetSelectedItem( pcStyle, hli );
				selected = 1;
			}
		}
		if( !bAdded )
		{
			lprintf( WIDE("No styles, deleting my family name from the list... and then uhmm something") );
			DeleteListItem( pc, pli );
			SetSelectedItem( pc, GetNthItem( pc, 0 ) );
			pfe->flags.unusable = 1;
			return;
		}
	}
	else
	{
		SetSelectedItem( pcStyle, AddListItem( pcStyle, WIDE("only style") ) );
      selected = 1;
	}
   if( !selected )
		SetSelectedItem( pcStyle, GetNthItem( pcStyle, 0 ) );
   DisableUpdateListBox( GetNearControl( pc, LST_FONT_SIZES ), FALSE );
   DisableUpdateListBox( pcStyle, FALSE );
}

//-------------------------------------------------------------------------

static void CPROC FillFamilyList( PFONT_DIALOG pfd )
{
	PLISTITEM pliSelect = NULL;
	PSI_CONTROL pc = GetControl( pfd->pFrame, LST_FONT_NAMES );
	DisableUpdateListBox( pc, TRUE );
	{
		PFONT_ENTRY pfe;
		INDEX idx;
		ResetList( pc );
		for( idx = 0; idx < fg.nFonts; idx++ )
		{
			PLISTITEM hli = NULL;
			pfe = fg.pFontCache + idx;
			if( pfe->flags.unusable )
				continue;
			if	( pfd->flags.show_mono_only )
			{
				if( HasMonoSpacing( pfe ) )
					hli = AddListItem( pc, pfe->name );
			}
			else if( pfd->flags.show_prop_only )
			{
				if( HasPropSpacing( pfe ) )
					hli = AddListItem( pc, pfe->name );
			}
			else
			{
				hli = AddListItem( pc, pfe->name );
			}
			if( hli )
				SetItemData( hli, (PTRSZVAL)pfe );
		}
		if( pfd->filename )
			pliSelect = AddListItem( pc, WIDE("DEFAULT") );
	}
	if( !pliSelect )
		pliSelect = GetNthItem( pc
									 , pfd->pFontEntry?((_32)(pfd->pFontEntry - fg.pFontCache)):0 );
	SetSelectedItem( pc, pliSelect );
   DisableUpdateListBox( pc, FALSE );
}

//-------------------------------------------------------------------------

static void CPROC SetFontSpacingSelection( PTRSZVAL psv, PSI_CONTROL pc )
{
   PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	switch( GetControlID( pc ) )
	{
	case CHK_MONO_SPACE:
      pfd->flags.show_mono_only = 1;
      pfd->flags.show_prop_only = 0;
      break;
	case CHK_PROP_SPACE:
      pfd->flags.show_mono_only = 0;
      pfd->flags.show_prop_only = 1;
      break;
	case CHK_BOTH_SPACE:
      pfd->flags.show_mono_only = 0;
      pfd->flags.show_prop_only = 0;
      break;
	}
	pfd->pFontEntry = NULL;
	FillFamilyList( pfd );
}

//-------------------------------------------------------------------------

static void CPROC SetFontAlphaSelection( PTRSZVAL psv, PSI_CONTROL pc )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	switch( GetControlID( pc ) )
	{
	case CHK_MONO:
		pfd->flags.render_depth = 1;
		break;
	case CHK_2BIT:
		pfd->flags.render_depth = 2;
		break;
	case CHK_8BIT:
		pfd->flags.render_depth = 3;
		break;
	}
	UpdateSampleFont( pfd );
}

//-------------------------------------------------------------------------

int CPROC SampleMouse( PCOMMON pc, S_32 x, S_32 y, _32 b )
{
	return 0;
//  PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
}

//-------------------------------------------------------------------------

static void CPROC ButtonApply( PTRSZVAL psvControl, PSI_CONTROL pc )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)GetFrameUserData( GetFrame( pc ) );
	// should duplicate or add a font reference or something...
	// next rendering is going to invalidate this...
	if( pfd->Update )
		pfd->Update( pfd->psvUpdate, pfd->pFont );

}

//-------------------------------------------------------------------------

extern CONTROL_REGISTRATION font_sample;

int CPROC InitFontSample( PCOMMON pc, va_list args )
{
   return 1;
}

int CPROC InitFontSizer( PCOMMON pc, va_list args )
{
   return 1;
}

//-------------------------------------------------------------------------
CONTROL_REGISTRATION font_sample = { WIDE("Font Sample"), { { DIALOG_WIDTH - 10, DIALOG_HEIGHT - 245 }, 0, BORDER_THIN|BORDER_INVERT }
											  , NULL // InitFont
											  , NULL
											  , UpdateSample
											  , SampleMouse
};
CONTROL_REGISTRATION char_size = { WIDE("Font Size Control"), { { 80, 80 }, 0, BORDER_THIN|BORDER_INVERT }
											  , NULL // InitFont
											  , NULL
											  , DrawCharacterSize
											  , NULL
};
PRIORITY_PRELOAD( RegisterFont, PSI_PRELOAD_PRIORITY )
{
   DoRegisterControl( &font_sample );
   DoRegisterControl( &char_size );
}


//-------------------------------------------------------------------------

PSI_FONTS_NAMESPACE_END
IMAGE_NAMESPACE
     extern _64 fontcachetime;
IMAGE_NAMESPACE_END
PSI_FONTS_NAMESPACE


SFTFont PickScaledFontWithUpdate( S_32 x, S_32 y
														, PFRACTION width_scale
														, PFRACTION height_scale
														, size_t *pFontDataSize
														 // resulting parameters for the data and size of data
														 // which may be passe dto RenderFontData
														, POINTER *pFontData
														, PCOMMON pAbove
														, void (CPROC *UpdateFont)( PTRSZVAL psv, SFTFont font )
														, PTRSZVAL psvUpdate )
{
	PSI_CONTROL pc;
	FONT_DIALOG fdData;
	Log( WIDE("Picking a font...") );
	LoadAllFonts();
#ifdef USE_INTERFACES
	GetMyInterface();
#endif
	//SetControlInterface( GetDisplayInterface() );
	//SetControlImageInterface( GetImageInterface() );
   //fdData.pFont = GetDefaultFont();
	MemSet( &fdData, 0, sizeof( fdData ) );
	// attempt to see if passed in data is reasonable
   // then try and use it as default dialogdata...
	if( pFontData
		&& (*pFontData )
		&& pFontDataSize
		&& ( (*pFontDataSize) > sizeof( RENDER_FONTDATA ) )
		&& ( (*pFontDataSize) < sizeof( FONTDATA ) + 4096 ) )
	{
      PFONTDATA pResult = (PFONTDATA)(*pFontData);
      if( pResult->magic == MAGIC_PICK_FONT )
		{
			fdData.pFontEntry = fg.pFontCache + pResult->nFamily;
			fdData.pFontStyle = fdData.pFontEntry->styles + pResult->nStyle;
			fdData.pSizeFile = fdData.pFontStyle->files + pResult->nFile;
			fdData.flags.render_depth = pResult->flags;
		}
		else
		{
			PRENDER_FONTDATA pResult = (PRENDER_FONTDATA)(*pFontData);
			fdData.flags.render_depth = pResult->flags;
			fdData.filename = pResult->filename;
		}
		fdData.nSliderWidth = pResult->nWidth;
		fdData.nSliderHeight = pResult->nHeight ;
	}
	else
	{
		fdData.nSliderWidth = 10;
		fdData.nSliderHeight = 14;
	}
	fdData.width_scale = width_scale;
	fdData.height_scale = height_scale;
	fdData.pFrame = CreateFrame( WIDE("Choose Font")
										, 0, 0
										, DIALOG_WIDTH, DIALOG_HEIGHT
										, 0
										, pAbove );
	if( !fdData.pFrame )
		return NULL;
   SetFrameUserData( fdData.pFrame, (PTRSZVAL)&fdData );
	MakeEditControl( fdData.pFrame, 5, 5
						, 210, 15, TXT_STATIC, WIDE("name..."), 0 );
	pc = MakeListBox( fdData.pFrame, 5, 25
						 , 210, 210, LST_FONT_NAMES, 0 );
	SetSelChangeHandler( pc, FamilySelected, (PTRSZVAL)&fdData );

	pc = MakeListBox( fdData.pFrame
						 , 220, 25
						 , 125, 58, LST_FONT_STYLES, 0 );
   SetSelChangeHandler( pc, StyleSelected, (PTRSZVAL)&fdData );

	pc = MakeListBox( fdData.pFrame
						 , 350, 25
						 , 65, 58, LST_FONT_SIZES, 0 );
   SetSelChangeHandler( pc, SizeSelected, (PTRSZVAL)&fdData );

	MakeRadioButton( fdData.pFrame
						, 220, 88
						, 95, 15
						, CHK_MONO_SPACE, WIDE("Fixed"), 1
						, RADIO_CALL_CHECKED
						, SetFontSpacingSelection, (PTRSZVAL)&fdData );
	MakeRadioButton( fdData.pFrame
						, 220, 102
						, 95, 15
						, CHK_PROP_SPACE, WIDE("Variable"), 1
						, RADIO_CALL_CHECKED
						, SetFontSpacingSelection, (PTRSZVAL)&fdData );
	pc = MakeRadioButton( fdData.pFrame
						, 220, 116
						, 95, 15
							  , CHK_BOTH_SPACE, WIDE("Both"), 1
                        , RADIO_CALL_CHECKED
							  , SetFontSpacingSelection, (PTRSZVAL)&fdData );

	pc = MakeRadioButton( fdData.pFrame
						, 320, 88
						, 95, 15
						, CHK_8BIT, WIDE("8 bit"), 1
                   , RADIO_CALL_CHECKED
						, SetFontAlphaSelection, (PTRSZVAL)&fdData );
	if( fdData.flags.render_depth == 3 )
		SetCheckState( pc, TRUE );
	pc = MakeRadioButton( fdData.pFrame
						, 320, 102
						, 95, 15
						, CHK_2BIT, WIDE("2 bit"), 1
                   , RADIO_CALL_CHECKED
						, SetFontAlphaSelection, (PTRSZVAL)&fdData );
	if( fdData.flags.render_depth == 2 )
		SetCheckState( pc, TRUE );
	pc = MakeRadioButton( fdData.pFrame
							  , 320, 116
							  , 95, 15
							  , CHK_MONO, WIDE("mono"), 1
                        , RADIO_CALL_CHECKED
							  , SetFontAlphaSelection, (PTRSZVAL)&fdData );
	if( fdData.flags.render_depth == 0 ||
		fdData.flags.render_depth == 1 )
		SetCheckState( pc, TRUE );
	fdData.pSample = MakeNamedControl( fdData.pFrame, WIDE("Font Sample")
                                     , 5, 240
												, DIALOG_WIDTH - 10, DIALOG_HEIGHT - 245
												, CST_FONT_SAMPLE );
	pc = MakeNamedControl( fdData.pFrame, WIDE("Font Size Control")
								, 220, 135
								, 80, 80
								, CST_CHAR_SIZE );
	//SetControlDraw( fdData.pVerSlider, DrawCharacterSize, 0 );
	fdData.pHorSlider = MakeSlider( fdData.pFrame
											, 220,  215
											, 80, 20
											, SLD_WIDTH
                                  , SLIDER_HORIZ
											, UpdateCharRect, (PTRSZVAL)&fdData  );
	SetSliderValues( fdData.pHorSlider, 1, fdData.nSliderWidth, 220 );
	fdData.pVerSlider = MakeSlider( fdData.pFrame
											, 300,  135
											, 20, 80
											, SLD_HEIGHT
                                  , SLIDER_VERT
											, UpdateCharRect, (PTRSZVAL)&fdData );
   // this makes the slider behave in a natural way
   SetSliderValues( fdData.pVerSlider, -220, -fdData.nSliderHeight, -1 );
	AddCommonButtons( fdData.pFrame, &fdData.done, &fdData.okay );
	{
		PCOMMON pc = GetControl( fdData.pFrame, BTN_OKAY );
#define COMMON_BUTTON_HEIGHT 19
		MoveSizeCommon( pc
						  , DIALOG_WIDTH - 60, 240 - ( ( COMMON_BUTTON_HEIGHT + 5 ) * 2 )
						  , 55, COMMON_BUTTON_HEIGHT );
		pc = GetControl( fdData.pFrame, BTN_CANCEL );
		MoveSizeCommon( pc
						  , DIALOG_WIDTH - 60, 240 - ( COMMON_BUTTON_HEIGHT + 5 )
						  , 55, COMMON_BUTTON_HEIGHT );
		fdData.Update = UpdateFont;
      fdData.psvUpdate = psvUpdate;
		if( fdData.Update )
		{
			MakeButton( fdData.pFrame, DIALOG_WIDTH-60, 240 - ( ( COMMON_BUTTON_HEIGHT + 5 ) * 3 )
						 , 55, COMMON_BUTTON_HEIGHT, -1, WIDE("Apply"), 0
						 , ButtonApply, 0 );
		}
#if 0
		MakeButton( fdData.pFrame
					 , DIALOG_WIDTH - 60, 240 - ( ( COMMON_BUTTON_HEIGHT + 5 ) * 2 )
					 , 55, COMMON_BUTTON_HEIGHT
					 , BTN_OKAY, WIDE("OK"), 0, ButtonOkay, (PTRSZVAL)&fdData.okay );
		MakeButton( fdData.pFrame
					 , DIALOG_WIDTH - 60, 240 - ( COMMON_BUTTON_HEIGHT + 5 )
					 , 55, COMMON_BUTTON_HEIGHT
					 , BTN_CANCEL, WIDE("Cancel"), 0, ButtonOkay, (PTRSZVAL)&fdData.done );
#endif
	}
	FillFamilyList( &fdData );
	DisplayFrameOver( fdData.pFrame, pAbove );
	while( 1 )
	{
      CommonWait( fdData.pFrame );
		if( fdData.done || fdData.okay )
		{
#ifdef _DEBUG
			//DumpLoadedFontCache();
#endif
			if( fdData.okay )
			{
				if( pFontData )
				{
					if( fdData.pFontEntry )
					{
						size_t l1, l2, l3, l4;
						size_t resultsize = sizeof(FONTDATA)
							+ (l1=strlen( fdData.pFontEntry->name ))
							+ (l2=strlen( fdData.pFontStyle->name ))
							+ (l3=strlen( fdData.pSizeFile->path ))
							+ (l4=strlen( fdData.pSizeFile->file ))
							+ 4, offset;
						PFONTDATA pResult = (PFONTDATA)Allocate( resultsize );
						pResult->magic = MAGIC_PICK_FONT;
						pResult->cachefile_time = fontcachetime;
						pResult->nFamily = (_32)(fdData.pFontEntry - fg.pFontCache);
						pResult->nStyle = (_32)(fdData.pFontStyle - fdData.pFontEntry->styles);
						pResult->nFile = (_32)(fdData.pSizeFile - fdData.pFontStyle->files);
						//fdData.pFontStyle->flags.mono
						pResult->flags = fdData.flags.render_depth;
						pResult->nWidth = fdData.nWidth;
						pResult->nHeight = fdData.nHeight;
						offset = 0;

						StrCpyEx( pResult->names + offset, fdData.pFontEntry->name, l1+1 );
						offset += l1 + 1;

						StrCpyEx( pResult->names + offset, fdData.pFontStyle->name, l2+1 );
						offset += l2 + 1;

						offset += snprintf( pResult->names + offset*sizeof(TEXTCHAR), l3+l4+2, WIDE("%s/%s")
											  , fdData.pSizeFile->path
											  , fdData.pSizeFile->file
											  );

						//if( *pFontData )
						//{
						// unsafe to do, but we result in a memory leak otherwise...
						//	Release( *pFontData );
						//}
						*pFontData = (POINTER)pResult;
						if( pFontDataSize )
							*pFontDataSize = resultsize;
						SetFontRendererData( fdData.pFont, pResult, resultsize );
					}
					else
					{
                  size_t chars;
						size_t resultsize = sizeof(RENDER_FONTDATA)
							+ (chars=strlen( fdData.filename ) + 1)*sizeof(TEXTCHAR);
						PRENDER_FONTDATA pResult = (PRENDER_FONTDATA)Allocate( resultsize );
						pResult->magic = MAGIC_RENDER_FONT;
						StrCpyEx( pResult->filename, fdData.filename, chars );
						pResult->flags = fdData.flags.render_depth;
						pResult->nWidth = fdData.nWidth;
						pResult->nHeight = fdData.nHeight;
						//if( *pFontData )
						//{
						// unsafe to do, but we result in a memory leak otherwise...
						//	Release( *pFontData );
						//}
						*pFontData = (POINTER)pResult;
						if( pFontDataSize )
							*pFontDataSize = resultsize;
						SetFontRendererData( fdData.pFont, pResult, resultsize );
					}
				}
			}
			else
			{
				if( fdData.pFont )
				{
					UnloadFont( fdData.pFont );
					fdData.pFont = NULL;
				}
			}
         break;
		}
		fdData.okay = 0;
      fdData.done = 0;
	}
	DestroyFrame( &fdData.pFrame );
   UnloadAllFonts();
   return fdData.pFont;
}

//-------------------------------------------------------------------------

PSI_PROC( SFTFont, PickFontWithUpdate )( S_32 x, S_32 y
										  , size_t *pFontDataSize
											// resulting parameters for the data and size of data
											// which may be passe dto RenderFontData
										  , POINTER *pFontData
										  , PCOMMON pAbove
										  , void (CPROC *UpdateFont)( PTRSZVAL psv, SFTFont font )
										  , PTRSZVAL psvUpdate )
{
   return PickScaledFontWithUpdate( x, y, NULL, NULL, pFontDataSize, pFontData, pAbove, UpdateFont, psvUpdate );
}

void CPROC UpdateCommonFont( PTRSZVAL psvCommon, SFTFont font )
{
   SetCommonFont( (PCOMMON)psvCommon, font );
}

PSI_PROC( SFTFont, PickFontFor )( S_32 x, S_32 y
									  , size_t *pFontDataSize
										// resulting parameters for the data and size of data
										// which may be passe dto RenderFontData
									  , POINTER *pFontData
									  , PCOMMON pAbove
									  , PCOMMON pUpdateFontFor )
{
   return PickFontWithUpdate( x, y, pFontDataSize, pFontData, pAbove, UpdateCommonFont, (PTRSZVAL)pUpdateFontFor );
}

PSI_PROC( SFTFont, PickFont )( S_32 x, S_32 y
								  , size_t * pFontDataSize
									// resulting parameters for the data and size of data
                           // which may be passe dto RenderFontData
								  , POINTER *pFontData
								  , PCOMMON pAbove )
{
	return PickFontWithUpdate( x, y, pFontDataSize, pFontData, pAbove, NULL, 0 );
}

#else
PSI_PROC( SFTFont, PickFont )( S_32 x, S_32 y
								  , size_t * size, POINTER *pFontData
								  , PFRAME pAbove )
{
   return NULL;
}


#endif
PSI_FONTS_NAMESPACE_END
// $Log: fntdlg.c,v $
// Revision 1.20  2005/03/21 20:41:35  panther
// Protect against super large fonts, remove edit frame from palette, and clean up some warnings.
//
// Revision 1.19  2005/02/09 21:23:44  panther
// Update macros and function definitions to follow the common MakeControl parameter ordering.
//
// Revision 1.18  2004/12/20 19:45:15  panther
// Fixes for protected sheet control init
//
// Revision 1.17  2004/12/15 03:00:19  panther
// Begin coding to only show valid, renderable fonts in dialog, and update cache, turns out that we'll have to postprocess the cache to remove unused dictionary entries
//
// Revision 1.16  2004/12/14 14:40:23  panther
// Fix slider control to handle the attr flags again for direction.  Fixed font selector dialog to close coorectly, update between controls correctly, etc
//
// Revision 1.15  2004/10/25 10:39:58  d3x0r
// Linux compilation cleaning requirements...
//
// Revision 1.14  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.4  2004/10/12 23:55:10  d3x0r
// checkpoint
//
// Revision 1.3  2004/10/08 13:07:42  d3x0r
// Okay beginning to look a lot like PRO-GRESS
//
// Revision 1.2  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.13  2004/05/27 09:17:48  d3x0r
// Add font alpha depth to font choice dialog
//
// Revision 1.12  2003/10/07 00:37:34  panther
// Prior commit in error - Begin render fonts in multi-alpha.
//
// Revision 1.11  2003/09/22 10:45:08  panther
// Implement tree behavior in standard list control
//
// Revision 1.10  2003/09/11 13:09:25  panther
// Looks like we maintained integrety while overhauling the Make/Create/Init/Config interface for controls
//
// Revision 1.9  2003/08/27 07:58:39  panther
// Lots of fixes from testing null pointers in listbox, font generation exception protection
//
// Revision 1.8  2003/08/21 13:34:42  panther
// include font render project with windows since there's now freetype
//
// Revision 1.7  2003/07/03 12:19:21  panther
// Define RenderFontFile for those places that doen't
//
// Revision 1.6  2003/06/16 10:17:42  panther
// Export nearly usable renderfont routine... filename, size
//
// Revision 1.5  2003/05/01 21:55:48  panther
// Update server/client interfaces
//
// Revision 1.4  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
