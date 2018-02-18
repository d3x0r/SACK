#include <stdhdrs.h>
#include <sharemem.h>
#include <filesys.h>

//#include <ft2build.h>
//#ifdef FT_FREETYPE_H
#if 1
//#include FT_FREETYPE_H

#include "global.h"

#include "../imglib/fntglobal.h"
#include <controls.h>
#include <psi.h>

#include "resource.h"
PSI_FONTS_NAMESPACE
#define DIALOG_WIDTH 420+50
#define DIALOG_HEIGHT 300+200

#define MAX_FONT_HEIGHT 250

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
		uint32_t show_mono_only : 1;
		uint32_t show_prop_only : 1;
		uint32_t showing_scalable : 1;
		uint32_t render_depth : 2;
	} flags;
	PSI_CONTROL pFrame;
	PSI_CONTROL pSample, pHorSlider, pVerSlider;
	PSI_CONTROL pVerValue, pVerLabel;
	PSI_CONTROL pHorValue, pHorLabel;
	SFTFont pFont;   // temp font for drawing sample
	int done, okay;
	PFONT_ENTRY pFontEntry;
	int nFontEntry;
	PFONT_STYLE pFontStyle;
	int nFontStyle;
	PSIZE_FILE  pSizeFile;
	int nSizeFile;
	TEXTCHAR *filename;
	int32_t nWidth, nSliderWidth;
	int32_t nHeight, nSliderHeight;
	PFRACTION width_scale, height_scale;
	void (CPROC* Update)(uintptr_t,SFTFont);
	uintptr_t psvUpdate;
} FONT_DIALOG, *PFONT_DIALOG;

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
	for( idx = 0; idx < pfe->nStyles; idx++ )
	{
		pfs = pfe->styles + idx;
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
	for( idx = 0; idx < pfe->nStyles; idx++ )
	{
		pfs = pfe->styles + idx;
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
		pfd->pFont = InternalRenderFont( (uint32_t)(pfd->pFontEntry - fg.pFontCache)
												 , (uint32_t)(pfd->nFontStyle)
												 , (uint32_t)(pfd->nSizeFile)
												 , pfd->nWidth
												 , pfd->nHeight
												 , (pfd->width_scale)
												 , (pfd->height_scale)
												 , (pfd->flags.render_depth == 3)?2/*FONT_FLAG_8BIT*/:(pfd->flags.render_depth==2)?1/*FONT_FLAG_2BIT*/:0/*FONT_FLAG_MONO*/ );
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
	int32_t width, height;
	PFONT_DIALOG pfd = (PFONT_DIALOG)GetFrameUserData( GetFrame( pc ) );
	Image surface = GetControlSurface( pc );
	//lprintf( WIDE("Drawing character size control...") );
	ClearImageTo( surface, Color( 92, 91, 42 ) );
	if( pfd->width_scale )
		(width = ScaleValue( pfd->width_scale, pfd->nSliderWidth ) );
	else 
		(width = pfd->nSliderWidth);
	if( width <= surface->width )
		x = (surface->width - width )/ 2;
	else
		x = 0;
	if( pfd->height_scale )
		(height = ScaleValue( pfd->height_scale, pfd->nSliderHeight) );
	else
		(height = pfd->nSliderHeight);
	if( height < surface->height )
		y = (surface->height - height )/ 2;
	else
		y = 0;
	BlatColor( surface, x, y
				, width
				, height, Color( 0, 0, 0 ) );
	return 1;
}

//-------------------------------------------------------------------------

void CPROC UpdateCharRect( uintptr_t psv, PSI_CONTROL pc, int val )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	if( GetControlID( pc ) == SLD_WIDTH )
	{
		if( pfd->nSliderWidth != val ) {
			pfd->nSliderWidth = val;
			{
				char buf[12];
				snprintf( buf, 12, "%d", val );
				SetEditControlSelection( pfd->pHorValue, 0, -1 );
				TypeIntoEditControl( pfd->pHorValue, buf );
			}

		}
		if( pfd->flags.showing_scalable )
			pfd->nWidth = pfd->nSliderWidth;
	}
	else if( GetControlID( pc ) == SLD_HEIGHT )
	{
		if( pfd->nSliderHeight != -val ) {
			pfd->nSliderHeight = -val;
			{
				char buf[12];
				snprintf( buf, 12, "%d", -val );
				SetEditControlSelection( pfd->pVerValue, 0, -1 );
				TypeIntoEditControl( pfd->pVerValue, buf );
			}
		}
		if( pfd->flags.showing_scalable )
			pfd->nHeight = pfd->nSliderHeight;
	}
	//lprintf( WIDE("Updated slider value - update control...") );
	if( pfd->flags.showing_scalable )
		UpdateSampleFont( pfd );
	SmudgeCommon( GetNearControl( pc, CST_CHAR_SIZE ) );
}

//-------------------------------------------------------------------------

static void CPROC SizeSelected( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	PSIZE_FILE psf;// = (PSIZE_FILE)GetItemData( pli );
	pfd->nSizeFile = (int)GetItemData( pli );
	psf = pfd->pSizeFile = pfd->pFontStyle?(pfd->pFontStyle->files + pfd->nSizeFile):NULL;
	if( psf )
	{
		TEXTCHAR size[15], *tmp;
		int32_t width, height;
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

static void CPROC StyleSelected( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
	PSI_CONTROL pcSizes;
	int npfs = (int)GetItemData( pli );
	PFONT_STYLE pfs = pfd->pFontEntry?(pfd->pFontEntry->styles + npfs):NULL;
	ResetList( pcSizes = GetNearControl( pc, LST_FONT_SIZES ) );
	pfd->pFontStyle = pfs;
	if( pfs )
	{
		INDEX idx;

		PSIZE_FILE psf;
		int bAdded = 0;

		//lprintf( WIDE("Style selected, filling in sizes...") );
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
					tnprintf( entry, 12, WIDE("%dx%d")
							  , size->width, size->height );
				}
				else
				{
					tnprintf( entry, 12, WIDE("%dx%d%s")
							  , size->width, size->height
							  , (pfs->flags.mono)?WIDE("(m)"):WIDE("") );
				}
				bAdded++;
				hli =	AddListItem( pcSizes, entry );
				SetItemData( hli, (uintptr_t)idx );
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

static void CPROC FamilySelected( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
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
		for( idx = 0; idx < pfe->nStyles; idx++ )
		{
			PLISTITEM hli;
			pfs = pfe->styles + idx;
			if( pfs->flags.unusable )
				continue;
			if( pfs->flags.mono && pfd->flags.show_prop_only )
				continue;
			if( !pfs->flags.mono && pfd->flags.show_mono_only )
				continue;
			bAdded++;
			hli = AddListItem( pcStyle, pfs->name );
			SetItemData( hli, (uintptr_t)idx );
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
				SetItemData( hli, (uintptr_t)pfe );
		}
		if( pfd->filename )
			pliSelect = AddListItem( pc, WIDE("DEFAULT") );
	}
	if( !pliSelect )
	{
		lprintf( "attempt select %d", pfd->pFontEntry?((uint32_t)(pfd->pFontEntry - fg.pFontCache)):0 );
		pliSelect = GetNthItem( pc
									 , pfd->pFontEntry?((uint32_t)(pfd->pFontEntry - fg.pFontCache)):0 );
	}
	SetSelectedItem( pc, pliSelect );
	DisableUpdateListBox( pc, FALSE );
}

//-------------------------------------------------------------------------

static void CPROC SetFontSpacingSelection( uintptr_t psv, PSI_CONTROL pc )
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

static void CPROC SetFontAlphaSelection( uintptr_t psv, PSI_CONTROL pc )
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

int CPROC SampleMouse( PCOMMON pc, int32_t x, int32_t y, uint32_t b )
{
	return 0;
//  PFONT_DIALOG pfd = (PFONT_DIALOG)psv;
}

//-------------------------------------------------------------------------

static void CPROC ButtonApply( uintptr_t psvControl, PSI_CONTROL pc )
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


void CPROC heightChanged( PSI_CONTROL pc ) {
	static int fixing;
	FONT_DIALOG  *pfdData = (FONT_DIALOG*)GetFrameUserData( GetParentControl( pc ) );
	char buf[12];
	int n;
	if( fixing ) return;
	GetControlText( pc, buf, 12 );
	n = (int)IntCreateFromText( buf );
	if( pfdData->nSliderHeight == n )
		return;
	if( n > MAX_FONT_HEIGHT ) {
		n = MAX_FONT_HEIGHT;
		snprintf( buf, 12, "%d", n );
		SetEditControlSelection( pc, 0, -1 );
		fixing = 1;
		TypeIntoEditControl( pc, buf );
		fixing = 0;
	}
	if( n )
	{
		//pfdData->nHeight = n;
		pfdData->nSliderHeight = n;
		SetSliderValues( pfdData->pVerSlider, -MAX_FONT_HEIGHT, -n, 0 );
		if( pfdData->flags.showing_scalable )
			UpdateSampleFont( pfdData );
		SmudgeCommon( GetNearControl( pc, CST_CHAR_SIZE ) );
	}
}
void CPROC widthChanged( PSI_CONTROL pc ) {
	static int fixing;
	FONT_DIALOG  *pfdData = (FONT_DIALOG*)GetFrameUserData( GetParentControl( pc ) );
	char buf[12];
	int n;
	if( fixing ) return;
	GetControlText( pc, buf, 12 );
	n = (int)IntCreateFromText( buf );
	if( pfdData->nSliderWidth == n )
		return;
	if( n > MAX_FONT_HEIGHT ) {
		n = MAX_FONT_HEIGHT;
		snprintf( buf, 12, "%d", n );
		SetEditControlSelection( pc, 0, -1 );
		fixing = 1;
		TypeIntoEditControl( pc, buf );
		fixing = 0;
	}
	if( n )
	{
		pfdData->nSliderWidth = n;
		SetSliderValues( pfdData->pHorSlider, 0, n, MAX_FONT_HEIGHT );
			//pfdData->nWidth = n;
		if( pfdData->flags.showing_scalable )
			UpdateSampleFont( pfdData );
		SmudgeCommon( GetNearControl( pc, CST_CHAR_SIZE ) );
	}
}

//-------------------------------------------------------------------------

PSI_FONTS_NAMESPACE_END
IMAGE_NAMESPACE
	  extern uint64_t fontcachetime;
IMAGE_NAMESPACE_END
PSI_FONTS_NAMESPACE


SFTFont PickScaledFontWithUpdate( int32_t x, int32_t y
														, PFRACTION width_scale
														, PFRACTION height_scale
														, size_t *pFontDataSize
														 // resulting parameters for the data and size of data
														 // which may be passe dto RenderFontData
														, POINTER *pFontData
														, PCOMMON pAbove
														, void (CPROC *UpdateFont)( uintptr_t psv, SFTFont font )
														, uintptr_t psvUpdate )
{
	PSI_CONTROL pc;
	FONT_DIALOG fdData;
	//Log( WIDE("Picking a font...") );
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
		if( ( pResult->magic != MAGIC_PICK_FONT ) && ( pResult->magic != MAGIC_RENDER_FONT ) )
		{
			if( strncmp( (char*)pResult, "pick,", 5 ) == 0 )
			{
				PFONTDATA pfd = New( FONTDATA );
				lprintf( "Recover pick font data..." );
				pfd->magic = MAGIC_PICK_FONT;
				{
					int family, style, file, width, height, flags;
					sscanf( (((char*)pResult) + 5), "%d,%d,%d,%d,%d,%d"
							, &family, &style, &file
							, &width, &height, &flags );
					lprintf( "%d,%d,%d,%d", family, style, file, flags );
					pfd->nWidth = width;
					pfd->nHeight = height;
					pfd->flags = flags;
					pfd->nFamily = family;
					pfd->nStyle = style;
					pfd->nFile = file;
				}
				pResult = pfd;
			}
			else
			{
				PRENDER_FONTDATA prfd;
				CTEXTSTR tail = StrRChr( (TEXTSTR)pResult, ',' );
				if( tail )
					tail++;
				else
					tail = "";
				prfd = NewPlus( RENDER_FONTDATA, (StrLen( tail ) + 1)*sizeof(TEXTCHAR) );
				StrCpy( prfd->filename, tail );
				prfd->magic = MAGIC_RENDER_FONT;
				{
					int width, height, flags;
					sscanf( (char*)pResult, "%d,%d,%d", &width, &height, &flags );
					prfd->nWidth = width;
					prfd->nHeight = height;
					prfd->flags = flags;
				}
				pResult = (PFONTDATA)prfd;
			}
		}
		if( pResult->magic == MAGIC_PICK_FONT )
		{
			if( pResult->nFamily < fg.nFonts )
			{
				fdData.pFontEntry = fg.pFontCache + pResult->nFamily;
				if( pResult->nStyle < fdData.pFontEntry->nStyles )
				{
					fdData.pFontStyle = fdData.pFontEntry->styles + pResult->nStyle;
					fdData.nFontStyle = pResult->nStyle;
					if( pResult->nFile < fdData.pFontStyle->nFiles )
						fdData.pSizeFile = fdData.pFontStyle->files + pResult->nFile;
				}
				fdData.flags.render_depth = pResult->flags;
			}
		}
		else
		{
			PRENDER_FONTDATA pResult = (PRENDER_FONTDATA)(*pFontData);
			lprintf( "wasn't a pick font stuct..." );
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
	SetFrameUserData( fdData.pFrame, (uintptr_t)&fdData );
	MakeEditControl( fdData.pFrame, 5, 5
						, 210, 15, TXT_STATIC, WIDE("name..."), 0 );
	pc = MakeListBox( fdData.pFrame, 5, 25
						 , 210, 210, LST_FONT_NAMES, 0 );
	SetSelChangeHandler( pc, FamilySelected, (uintptr_t)&fdData );

	pc = MakeListBox( fdData.pFrame
						 , 220, 25
						 , 125, 58, LST_FONT_STYLES, 0 );
	SetSelChangeHandler( pc, StyleSelected, (uintptr_t)&fdData );

	pc = MakeListBox( fdData.pFrame
						 , 350, 25
						 , 65, 58, LST_FONT_SIZES, 0 );
	SetSelChangeHandler( pc, SizeSelected, (uintptr_t)&fdData );

	MakeRadioButton( fdData.pFrame
						, 220, 88
						, 95, 15
						, CHK_MONO_SPACE, WIDE("Fixed"), 1
						, RADIO_CALL_CHECKED
						, SetFontSpacingSelection, (uintptr_t)&fdData );
	MakeRadioButton( fdData.pFrame
						, 220, 102
						, 95, 15
						, CHK_PROP_SPACE, WIDE("Variable"), 1
						, RADIO_CALL_CHECKED
						, SetFontSpacingSelection, (uintptr_t)&fdData );
	pc = MakeRadioButton( fdData.pFrame
						, 220, 116
						, 95, 15
							  , CHK_BOTH_SPACE, WIDE("Both"), 1
								, RADIO_CALL_CHECKED
							  , SetFontSpacingSelection, (uintptr_t)&fdData );

	pc = MakeRadioButton( fdData.pFrame
						, 320, 88
						, 95, 15
						, CHK_8BIT, WIDE("8 bit"), 1
						 , RADIO_CALL_CHECKED
						, SetFontAlphaSelection, (uintptr_t)&fdData );
	if( fdData.flags.render_depth == 3 )
		SetCheckState( pc, TRUE );
	pc = MakeRadioButton( fdData.pFrame
						, 320, 102
						, 95, 15
						, CHK_2BIT, WIDE("2 bit"), 1
						 , RADIO_CALL_CHECKED
						, SetFontAlphaSelection, (uintptr_t)&fdData );
	if( fdData.flags.render_depth == 2 )
		SetCheckState( pc, TRUE );
	pc = MakeRadioButton( fdData.pFrame
							  , 320, 116
							  , 95, 15
							  , CHK_MONO, WIDE("mono"), 1
								, RADIO_CALL_CHECKED
							  , SetFontAlphaSelection, (uintptr_t)&fdData );
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
											, UpdateCharRect, (uintptr_t)&fdData  );
	fdData.pVerSlider = MakeSlider( fdData.pFrame
											, 300,  135
											, 20, 80
											, SLD_HEIGHT
											 , SLIDER_VERT
											, UpdateCharRect, (uintptr_t)&fdData );
	// this makes the slider behave in a natural way

	fdData.pHorLabel = MakeTextControl( fdData.pFrame, 325, 161, 80, 20, TXT_STATIC, "Width", 0 );
	fdData.pHorValue = MakeEditControl( fdData.pFrame, 405, 161, 80, 20, TXT_STATIC, NULL, 0 );
	SetCaptionChangedMethod( fdData.pHorValue, widthChanged );
	//SetTextChan

	fdData.pVerLabel = MakeTextControl( fdData.pFrame, 325, 136, 80, 20, TXT_STATIC, "Height", 0 );
	fdData.pVerValue = MakeEditControl( fdData.pFrame, 405, 136, 80, 20, TXT_STATIC, NULL, 0 );
	SetCaptionChangedMethod( fdData.pVerValue, heightChanged );

	SetSliderValues( fdData.pHorSlider, 1, fdData.nSliderWidth, MAX_FONT_HEIGHT );
	SetSliderValues( fdData.pVerSlider, -MAX_FONT_HEIGHT, -fdData.nSliderHeight, -1 );

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
					 , BTN_OKAY, WIDE("OK"), 0, ButtonOkay, (uintptr_t)&fdData.okay );
		MakeButton( fdData.pFrame
					 , DIALOG_WIDTH - 60, 240 - ( COMMON_BUTTON_HEIGHT + 5 )
					 , 55, COMMON_BUTTON_HEIGHT
					 , BTN_CANCEL, WIDE("Cancel"), 0, ButtonOkay, (uintptr_t)&fdData.done );
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
							+ (l1=StrLen( fdData.pFontEntry->name ))
							+ (l2=StrLen( fdData.pFontStyle->name ))
							+ (l3=StrLen( fdData.pSizeFile->path ))
							+ (l4=StrLen( fdData.pSizeFile->file ))
							+ 4;
						TEXTCHAR buf[128];
						PFONTDATA pResult;// = (PFONTDATA)Allocate( resultsize );
						snprintf( buf, 256, "pick,%d,%d,%d,%d,%d,%d,%s/%s"
								  , (int)(fdData.nFontEntry)
								  , (int)(fdData.nFontStyle)
								  , (int)(fdData.nSizeFile)
								  , (int)fdData.nWidth
								  , (int)fdData.nHeight
								  , (int)fdData.flags.render_depth
								  , fdData.pSizeFile->path
								  , fdData.pSizeFile->file
								  );
						resultsize = StrLen( buf );
						pResult = (PFONTDATA)StrDup( buf );
						/*
						pResult->magic = MAGIC_PICK_FONT;
						pResult->cachefile_time = fontcachetime;
						pResult->nFamily = (uint32_t)(fdData.pFontEntry - fg.pFontCache);
						pResult->nStyle = (uint32_t)(fdData.pFontStyle - fdData.pFontEntry->styles);
						pResult->nFile = (uint32_t)(fdData.pSizeFile - fdData.pFontStyle->files);
						//fdData.pFontStyle->flags.mono
						pResult->flags = fdData.flags.render_depth;
						pResult->nWidth = fdData.nWidth;
						pResult->nHeight = fdData.nHeight;
						offset = 0;

						StrCpyEx( pResult->names + offset, fdData.pFontEntry->name, l1+1 );
						offset += l1 + 1;

						StrCpyEx( pResult->names + offset, fdData.pFontStyle->name, l2+1 );
						offset += l2 + 1;

						offset += tnprintf( pResult->names + offset*sizeof(TEXTCHAR), l3+l4+2, WIDE("%s/%s")
											  , fdData.pSizeFile->path
											  , fdData.pSizeFile->file
											  );

						*/

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
						TEXTCHAR buf[256];
						snprintf( buf, 256, "%d,%d,%d,%s"
								  , fdData.nWidth
								  , fdData.nHeight
								  , fdData.flags.render_depth
									, fdData.filename
								  );
						*pFontData = (POINTER)StrDup( buf );
						*pFontDataSize = strlen( buf ) + 1;
						/*
						size_t chars;
						size_t resultsize = sizeof(RENDER_FONTDATA)
							+ (chars=StrLen( fdData.filename ) + 1)*sizeof(TEXTCHAR);
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
						*/
						SetFontRendererData( fdData.pFont, *pFontData, *pFontDataSize );
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

PSI_PROC( SFTFont, PickFontWithUpdate )( int32_t x, int32_t y
										  , size_t *pFontDataSize
											// resulting parameters for the data and size of data
											// which may be passe dto RenderFontData
										  , POINTER *pFontData
										  , PCOMMON pAbove
										  , void (CPROC *UpdateFont)( uintptr_t psv, SFTFont font )
										  , uintptr_t psvUpdate )
{
	return PickScaledFontWithUpdate( x, y, NULL, NULL, pFontDataSize, pFontData, pAbove, UpdateFont, psvUpdate );
}

void CPROC UpdateCommonFont( uintptr_t psvCommon, SFTFont font )
{
	SetCommonFont( (PCOMMON)psvCommon, font );
}

PSI_PROC( SFTFont, PickFontFor )( int32_t x, int32_t y
									  , size_t *pFontDataSize
										// resulting parameters for the data and size of data
										// which may be passe dto RenderFontData
									  , POINTER *pFontData
									  , PCOMMON pAbove
									  , PCOMMON pUpdateFontFor )
{
	return PickFontWithUpdate( x, y, pFontDataSize, pFontData, pAbove, UpdateCommonFont, (uintptr_t)pUpdateFontFor );
}

PSI_PROC( SFTFont, PickFont )( int32_t x, int32_t y
								  , size_t * pFontDataSize
									// resulting parameters for the data and size of data
									// which may be passe dto RenderFontData
								  , POINTER *pFontData
								  , PCOMMON pAbove )
{
	return PickFontWithUpdate( x, y, pFontDataSize, pFontData, pAbove, NULL, 0 );
}

#else
PSI_PROC( SFTFont, PickFont )( int32_t x, int32_t y
								  , size_t * size, POINTER *pFontData
								  , PFRAME pAbove )
{
	return NULL;
}


#endif
PSI_FONTS_NAMESPACE_END
