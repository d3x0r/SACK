#include <stdhdrs.h>
#include <stdio.h>
#include <sharemem.h>
#include <colordef.h>
#include "global.h" // for interfaces
//#include "controlstruc.h"
#include <psi.h>
#include <psi/shadewell.h>
#include <filedotnet.h>
#include "resource.h"

PSI_COLORWELL_NAMESPACE

typedef struct colorwell {
	struct {
		uint32_t bPickColor : 1;
		uint32_t bPicking : 1;
	} flags;
	CDATA color;
	
	void(CPROC*UpdateProc)(uintptr_t,CDATA);
	uintptr_t psvUpdate;
} COLOR_WELL, *PCOLOR_WELL;

enum {
	SHADER_LIGTDARK
	  , SHADER_RED
	  , SHADER_BLUE
	  , SHADER_GREEN
};
typedef struct shadewell {
	//int nShaderType;
	CDATA color_min;
	CDATA color_mid;
	CDATA color_max;
} SHADE_WELL, *PSHADE_WELL;

extern CONTROL_REGISTRATION shade_well, color_well;


typedef struct PickColor_tag
{
	PSI_CONTROL frame;
	struct {
		BIT_FIELD bSettingShade : 1;
		BIT_FIELD bMatrixChanged : 1;
	} flags;
	int nGreen; // level of green...
	int Alpha;	 // level of alpha...
	CDATA CurrentColor;
	CDATA Presets[36];
	PSI_CONTROL LastPreset;
	PSI_CONTROL pcZoom;
	PSI_CONTROL psw, pShadeRed, pShadeBlue, pShadeGreen; // shade well data...
	int bSetPreset;
	int ColorDialogDone, ColorDialogOkay;
	Image pColorMatrix; // fixed size image in local memory that is block copied for output.
	void ( *ok )( uintptr_t, CDATA );
	uintptr_t psvOk;
} PICK_COLOR_DATA, *PPICK_COLOR_DATA;

void CPROC InitColorDataDefault( POINTER );
void SetShaderControls( PPICK_COLOR_DATA ppcd, PSI_CONTROL source );

PUBLIC_DATA( "Color Choice Data", PICK_COLOR_DATA, InitColorDataDefault, NULL );

#define nScale 4


#define xbias 1
#define ybias 1
#define xsize 133
#define ysize 133

#define COLOR Color( (255-red)*nScale, (255-green)*nScale, (blue)*nScale )

//----------------------------------------------------------------------------

static void UpdateImage( Image pImage, int nGreen )
{
	int red, blue, green=0;
	if( !pImage )
		return;

	ClearImageTo( pImage, Color( 0, 0, 0 ) );

	for( green = 0; green < nGreen/nScale; green++ )
	{
		blue = 0;
		for( red = 0; red < 256/nScale; red++ )
		{
			plot( pImage, xbias+red + blue+(blue&1), ybias+(green + 128/nScale) - red/2 + blue/2 , COLOR );
		}
		red = 255/nScale;
		for( blue = 0; blue < 256/nScale; blue++ )
		{
			plot( pImage, xbias+red+(red&1) + blue, ybias+(green + 128/nScale) - red/2 + blue/2 , COLOR );
		}
	}

	for( blue = 0; blue <= 255/nScale; blue++ )
	{
		for( red = 0; red <= 255/nScale; red++ )
		{
			plot( pImage, xbias+red + blue+(blue&1), ybias+(green + 128/nScale) - red/2 + blue/2 , COLOR );
		}
	}

	for( ; green <= 255/nScale; green++ )
	{
		blue = 255/nScale;
		for( red = 0; red <= 255/nScale; red++ )
		{
			plot( pImage, xbias+red + blue+(blue&1), ybias+(green + 128/nScale) - red/2 + blue/2 , COLOR );
		}
		red = 0;
		for( blue = 0; blue < 256/nScale; blue++ )
		{
			plot( pImage, xbias+red + blue, ybias+(green + 128/nScale) - red/2 + blue/2 , COLOR );
		}
	}
}

//----------------------------------------------------------------------------

static CDATA ScaleColor( CDATA original, CDATA new_color, int max, int cur )
{
	int orig_r = RedVal( original );
	int orig_g = GreenVal( original );
	int orig_b = BlueVal( original );
	int nr = RedVal( new_color );
	int ng = GreenVal( new_color );
	int nb = BlueVal( new_color );
	orig_r *= max-cur;
	orig_r /= max;
	orig_g *= max-cur;
	orig_g /= max;
	orig_b *= max-cur;
	orig_b /= max;

	nr *= cur;
	nr /= max;
	ng *= cur;
	ng /= max;
	nb *= cur;
	nb /= max;
	
	orig_r += nr; 
	if( orig_r > 255 ) orig_r = 255;
	orig_g += ng; 
	if( orig_g > 255 ) orig_g = 255;
	orig_b += nb; 
	if( orig_b > 255 ) orig_b = 255;
	return Color( orig_r, orig_g, orig_b );
}
//----------------------------------------------------------------------------

static int OnDrawCommon("Shade Well") ( PSI_CONTROL pcShade )
{
	ValidatedControlData( PSHADE_WELL, shade_well.TypeID, psw, pcShade );
	uint32_t black;
	uint32_t width;
	uint32_t height;
	Image pSurface;
	if( psw )
	{
		pSurface = GetControlSurface( pcShade );
		width = pSurface->width;
		height = pSurface->height;
		//lprintf( "----------- DRAW SHADE CONTORL ------------------- %08x %08x %08x"
		//		 , psw->color_min
		//		 , psw->color_mid
		//		 , psw->color_max );
		for( black = 0; black < height/2; black++ )
		{
			BlatColor( pSurface, 0, black, width-1, 1, ScaleColor( psw->color_min, psw->color_mid, height/2, black ) );
			//do_hline( pSurface, black, 0, width-1, ScaleColor( psw->color_min, psw->color_mid, height/2, black ) );
		}
		for( black = height/2; black < height; black++ )
		{
			BlatColor( pSurface, 0, black, width-1, 1, ScaleColor( psw->color_mid, psw->color_max, height/2, black - height/2 ) );
			//do_hline( pSurface, black, 0, width-1, ScaleColor( psw->color_mid, psw->color_max, height/2, black - height/2 ) );
		}
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------

#define PPCD(pc) (PPICK_COLOR_DATA)GetControlUserData( GetFrame( pc ) )

//----------------------------------------------------------------------------

static int OnMouseCommon( "Color Matrix" )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t buttons )
{
	PPICK_COLOR_DATA ppcd = PPCD(pc);
	if( buttons == -1 )
		return FALSE;
	if( buttons & MK_LBUTTON )
	{
		if( ppcd->psw )
		{
			//lprintf( "Setting new mid color... update... ------------------------- " );
			ppcd->CurrentColor = SetAlpha( getpixel( ppcd->pColorMatrix, x, y )
												  , ppcd->Alpha );
			SetShaderControls( ppcd, pc );
			//SetColorWell( ppcd->pcZoom, c );
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------

int CPROC DrawPalette( PSI_CONTROL pc )
{
	PPICK_COLOR_DATA ppcd = PPCD(pc);
	if( ppcd )
	{
		if( ppcd->flags.bMatrixChanged )
		{
			Image Surface// = GetControlSurface( pc );
				= ppcd->pColorMatrix;
			UpdateImage( Surface, ppcd->nGreen );
			ppcd->flags.bMatrixChanged = 0;
		}
		BlotImage( GetControlSurface( pc ), ppcd->pColorMatrix, 0, 0 );
		if( GetCheckState( GetControl( ppcd->frame, CHK_ALPHA) ) )
		{
			Image Surface = GetControlSurface( pc );
			ppcd->CurrentColor = SetAlpha( ppcd->CurrentColor, ppcd->Alpha );
			BlatColorAlpha( Surface, 0, 0
							  , Surface->width
							  , Surface->height
							  , ppcd->CurrentColor );
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------

SLIDER_UPDATE( SetGreenLevel, (uintptr_t psv, PSI_CONTROL pc, int val) )
{
	//PPALETTE_CONTROL
	CDATA c;
	PPICK_COLOR_DATA ppcd  = PPCD(pc);
	if( GetCheckState( GetControl( ppcd->frame, CHK_ALPHA ) ) )
	{
		c = SetAlpha( ppcd->CurrentColor, val );
		ppcd->Alpha = val;
	}
	else
	{
		//c = SetGreen( ppcd->CurrentColor, val );
		ppcd->nGreen = val;
	}

	ppcd->flags.bMatrixChanged = 1;

	SmudgeCommon( GetControl( ppcd->frame, PAL_COLORS ) );
}

//----------------------------------------------------------------------------

static void LoadPresets( PPICK_COLOR_DATA ppcd )
{
	FILE *file;
	file = sack_fopen( 0, "Palette.Presets", "rt" );
	if( file )
	{
		int i;
		TEXTCHAR buf[64];
		for( i = 0; i < 36 && sack_fgets( buf, 64, file ); i++ )
		{
			int red, green, blue, alpha;
			if( tscanf( buf, "%d,%d,%d,%d\n", &red, &green, &blue, &alpha ) == 4 )
			{
				ppcd->Presets[i] = AColor( red, green, blue, alpha );
			}
			else
			{
				ppcd->Presets[i] = 0;
			}
		}
		sack_fclose( file );
	}
	else
	{
		int i;
		for( i = 0; i < 36; i++ )
		{
			ppcd->Presets[i] = 0;
		}
	}
}

//----------------------------------------------------------------------------

static void SavePresets( PPICK_COLOR_DATA ppcd )
{
	FILE *file;
	file = sack_fopen( 0, "Palette.Presets", "wt" );
	if( file )
	{
		int i;
		for( i = 0; i < 36; i++ )
		{
			int red = RedVal( ppcd->Presets[i] )
			  , green = GreenVal( ppcd->Presets[i] )
			  , blue = BlueVal( ppcd->Presets[i] )
			  , alpha = AlphaVal( ppcd->Presets[i] );
			sack_fprintf( file, "%d,%d,%d,%d\n", red, green, blue, alpha );
		}
		sack_fclose( file );
	}
}

//----------------------------------------------------------------------------

BUTTON_CLICK( PresetButton, ( uintptr_t psv, PSI_CONTROL pc ))
{
	// button was pressed...
	PPICK_COLOR_DATA ppcd = (PPICK_COLOR_DATA)psv;
	int idx = GetControlID(pc) - BTN_PRESET_BASE;
	if( ppcd->LastPreset )
		PressButton( ppcd->LastPreset, FALSE );

	if( ppcd->bSetPreset )
	{
		SetCheckState(GetControl(ppcd->frame, BTN_PRESET), FALSE);

		ppcd->Presets[idx] = ppcd->CurrentColor;
		ppcd->bSetPreset = FALSE;
		SavePresets( ppcd );
	}
	PressButton( pc, TRUE );
	ppcd->CurrentColor = ppcd->Presets[GetControlID(pc) - BTN_PRESET_BASE ];
	ppcd->Alpha = AlphaVal( ppcd->CurrentColor );
	// green level is backwards :(
	ppcd->flags.bSettingShade = 1;
	if( GetCheckState( GetControl( ppcd->frame, CHK_ALPHA ) ) )
		SetSliderValues( GetControl( ppcd->frame, SLD_GREENBAR ), 0, ppcd->Alpha, 255 );
	else
		SetSliderValues( GetControl( ppcd->frame, SLD_GREENBAR ), 0, 255-GreenVal(ppcd->CurrentColor ), 255 );
	SetShaderControls( ppcd, NULL );
	ppcd->flags.bSettingShade = 0;


	SmudgeCommon( GetControl( ppcd->frame, CST_ZOOM ) );
	ppcd->LastPreset = pc;
}

//----------------------------------------------------------------------------

BUTTON_DRAW( PresetDraw, ( uintptr_t psv, PSI_CONTROL pc ) )
{
	PPICK_COLOR_DATA ppcd = PPCD(pc);
	if( ppcd )
	{
		CDATA color = ppcd->Presets[GetControlID( pc ) - BTN_PRESET_BASE ];
		Image pSurface = GetControlSurface( pc );
		if( !AlphaVal( color ) )
			color = AColor( 0, 0, 0, 1 );
		ClearImageTo( pSurface, color );
	}
}

//----------------------------------------------------------------------------

BUTTON_CLICK( DefinePreset, ( uintptr_t unused, PSI_CONTROL pc ) )
{
	PPICK_COLOR_DATA ppcd = (PPICK_COLOR_DATA)unused;
	// put up a message box... 
	ppcd->bSetPreset = TRUE;
}

//----------------------------------------------------------------------------

BUTTON_CHECK( AlphaPressed, ( uintptr_t unused, PSI_CONTROL pc ) )
{
	PPICK_COLOR_DATA ppcd = (PPICK_COLOR_DATA)unused;
	if( GetCheckState( pc ) )
	{
		SetSliderValues( GetControl( ppcd->frame, SLD_GREENBAR ), 0, ppcd->Alpha, 255 );
	}
	else
	{
		SetSliderValues( GetControl( ppcd->frame, SLD_GREENBAR ), 0, ppcd->nGreen, 255 );
	}
}

//----------------------------------------------------------------------------

int CPROC PaletteLoad( uintptr_t psv, PSI_CONTROL pf, uint32_t ID )
{
	// hmm don't think there's really anything special I need to...
	// okay yeah...
	((PPICK_COLOR_DATA)psv)->frame = pf;
	//SetFrameUserData( pf, psv );
	return TRUE;
}

//----------------------------------------------------------------------------

void CPROC InitColorData( PPICK_COLOR_DATA ppcd, CDATA original )
{
	ppcd->ColorDialogDone = ppcd->ColorDialogOkay = FALSE;
	ppcd->Alpha= AlphaVal( original );
	ppcd->nGreen = GreenVal( original );
	ppcd->flags.bMatrixChanged = 1;
	ppcd->CurrentColor = original;
	ppcd->LastPreset = NULL;
	ppcd->bSetPreset = FALSE;
	LoadPresets( ppcd );

}

void CPROC InitColorDataDefault( POINTER p )
{
	PPICK_COLOR_DATA ppcd = (PPICK_COLOR_DATA)p;
	InitColorData( ppcd, Color( 127, 127, 127 ) );
}
//----------------------------------------------------------------------------

void SetShaderControls( PPICK_COLOR_DATA ppcd, PSI_CONTROL source )
{
	if( source != ppcd->pShadeRed )
	{
		SetShadeMin( ppcd->pShadeRed, Color(   0, GreenVal(ppcd->CurrentColor), BlueVal( ppcd->CurrentColor ) ) );
		SetShadeMax( ppcd->pShadeRed, Color( 255, GreenVal(ppcd->CurrentColor), BlueVal( ppcd->CurrentColor ) ) );
		SetShadeMid( ppcd->pShadeRed, Color( 127, GreenVal(ppcd->CurrentColor), BlueVal( ppcd->CurrentColor ) ) );
	}
	if( source != ppcd->pShadeGreen )
	{
		SetShadeMin( ppcd->pShadeGreen, Color( RedVal(ppcd->CurrentColor),   0 , BlueVal( ppcd->CurrentColor ) ));
		SetShadeMax( ppcd->pShadeGreen, Color( RedVal(ppcd->CurrentColor), 255 , BlueVal( ppcd->CurrentColor ) ));
		SetShadeMid( ppcd->pShadeGreen, Color( RedVal(ppcd->CurrentColor), 127 , BlueVal( ppcd->CurrentColor ) ));
	}
	if( source != ppcd->pShadeBlue )
	{
		SetShadeMin( ppcd->pShadeBlue, Color( RedVal(ppcd->CurrentColor), GreenVal( ppcd->CurrentColor ),   0 ) );
		SetShadeMax( ppcd->pShadeBlue, Color( RedVal(ppcd->CurrentColor), GreenVal( ppcd->CurrentColor ), 255 ) );
		SetShadeMid( ppcd->pShadeBlue, Color( RedVal(ppcd->CurrentColor), GreenVal( ppcd->CurrentColor ), 127 ) );
	}
	if( source != ppcd->psw )
		SetShadeMid( ppcd->psw, ppcd->CurrentColor );

	SetColorWell( ppcd->pcZoom, ppcd->CurrentColor );
}

//----------------------------------------------------------------------------

// LOL that's pretty sexy, huh? LOL
static TEXTCHAR palette_frame_xml[] = {
//#define stuff(...) #__VA_ARGS__
//stuff(
#include "palette.frame"
//    )
 };

static void cleanUserData( PICK_COLOR_DATA* pcd ) {
	UnmakeImageFile( pcd->pColorMatrix );
	Release( pcd );

}

static void ColorOkay( PSI_CONTROL button ) {
	PSI_CONTROL frame = GetFrame( button );
	PICK_COLOR_DATA* pcd = (PICK_COLOR_DATA*)GetFrameUserData( frame );
	if( pcd->ok ) pcd->ok( pcd->psvOk, pcd->CurrentColor );
	DestroyCommon( &frame );
	cleanUserData( pcd );
}

static void ColorDone( PSI_CONTROL button ) {
	PSI_CONTROL frame = GetFrame( button );
	PICK_COLOR_DATA* pcd = (PICK_COLOR_DATA*)GetFrameUserData( frame );
	//if( pcd->ok ) pcd->ok( pcd->psvOk, pcd->CurrentColor );
	DestroyCommon( &frame );
	cleanUserData( pcd );
}

static void commonButtonStatus( uintptr_t psv, PSI_CONTROL pc, int done, int ok ) {
	//PICK_COLOR_DATA* pcd = (PICK_COLOR_DATA*)psv;// GetFrameUserData( frame );
	if( done ) ColorDone( pc );
	if( ok ) ColorOkay( pc );
}

PSI_PROC( PSI_CONTROL, PickColorEx )( CDATA *result, CDATA original, PSI_CONTROL hAbove, int x, int y, void ( *ok )( uintptr_t, CDATA ), uintptr_t psv )
{
	PSI_CONTROL pf = NULL;
	PICK_COLOR_DATA *pcd_ = NewArray( PICK_COLOR_DATA, 1 );
#define pcd (pcd_[0])
	MemSet( &pcd, 0, sizeof( pcd ) );
	pcd.ok = ok;
	pcd.psvOk = psv;
	GetMyInterface();
	InitColorData( &pcd, original );
	// remove test for debugging save/load..
	// don't parse the NUL at the end.
	pf = ParseXMLFrame( hAbove, palette_frame_xml, sizeof( palette_frame_xml ) - 1 );
	if( !pf )
		pf = LoadXMLFrame( "palette.frame" /*, NULL, PaletteLoad, (uintptr_t)&pcd*/ );;
	if( pf )
	{
		int i;
		pcd.frame = pf;
		pcd.pColorMatrix = MakeImageFile( 128, 128 );
		SetFrameUserData( pf, (uintptr_t)&pcd );
		SetSliderValues( GetControl( pf, SLD_GREENBAR ), 0, 255-GreenVal( pcd.CurrentColor ), 255 );
		pcd.pcZoom =  GetControl( pf, CST_ZOOM );
		pcd.psw = GetControl( pf, CST_SHADE );
		pcd.pShadeRed = GetControl( pf, CST_SHADE_RED );
		pcd.pShadeGreen = GetControl( pf, CST_SHADE_GREEN );
		pcd.pShadeBlue = GetControl( pf, CST_SHADE_BLUE );
		SetButtonPushMethod( GetControl( pf, BTN_PRESET ), DefinePreset, (uintptr_t)&pcd );
		SetCheckButtonHandler( GetControl( pf, CHK_ALPHA ), AlphaPressed, (uintptr_t)&pcd );
		for( i = 0; i < 36; i++ )
		{
			PSI_CONTROL button = GetControl( pf, BTN_PRESET_BASE + i );
			if( button )
			{
				SetButtonDrawMethod( button, PresetDraw, (uintptr_t)&pcd );
				SetButtonPushMethod( button, PresetButton, (uintptr_t)&pcd );
			}
		}
		SetSliderUpdateHandler( GetControl( pf, SLD_GREENBAR ), SetGreenLevel, (uintptr_t)&pcd );
		SetCommonButtons( pf, &pcd.ColorDialogDone, &pcd.ColorDialogOkay );
	}

#define SHADER_PAD 3
#define SHADER_WIDTH 15
#define FRAME_WIDTH 226 + ( 3 * ( SHADER_WIDTH + SHADER_PAD ) )
#define FRAME_HEIGHT 259
	if( !pf )
	{
		PSI_CONTROL pc;
		pf = CreateFrame( "Color Select"
							 , x - FRAME_WIDTH/2, y - FRAME_HEIGHT/2
							 , FRAME_WIDTH, FRAME_HEIGHT, BORDER_NORMAL, NULL );
		if( !pf )
			return FALSE;
		// the space for colormatrix to draw in.
		pcd.pColorMatrix = MakeImageFile( 128, 128 );
		pcd.flags.bMatrixChanged = 1;
		pcd.frame = pf;
		SetFrameUserData( pf, (uintptr_t)&pcd );
		//MoveFrame( pf, x - FRAME_WIDTH/2, y - FRAME_HEIGHT/2 );
		pc = MakeNamedControl( pf, "Color Matrix"
									, 5, 5, xsize, ysize
									, PAL_COLORS );

		MakeSlider( pf
					 , 5 + xsize + 3, 1
					 , 18, ysize + 6
					 , SLD_GREENBAR, SLIDER_VERT, SetGreenLevel, (uintptr_t)&pcd );

		//	MakeTextControl( pf, TEXT_VERTICAL, 8 + xsize + 15, 20, 88, 12, TXT_STATIC, "Green Level" );
		pcd.psw = MakeNamedControl( pf, "Shade Well", 8 + xsize + 18 + 12, 5
												 , SHADER_WIDTH, ysize
												 , CST_SHADE );

		pcd.pShadeRed = MakeNamedControl( pf, "Shade Well", 8 + xsize + 18 + 12 + (SHADER_WIDTH + SHADER_PAD), 5
												  , SHADER_WIDTH, ysize
												  , CST_SHADE_RED );

		pcd.pShadeBlue = MakeNamedControl( pf, "Shade Well", 8 + xsize + 18 + 12 + (SHADER_WIDTH + SHADER_PAD)*2, 5
													, SHADER_WIDTH, ysize
													, CST_SHADE_BLUE );
		pcd.pShadeGreen = MakeNamedControl( pf, "Shade Well", 8 + xsize + 18 + 12 + (SHADER_WIDTH + SHADER_PAD)*3, 5
													 , SHADER_WIDTH, ysize
													 , CST_SHADE_GREEN );

		pcd.pcZoom = MakeNamedControl( pf, "Color Well"
											  , 8 + xsize + 18 + 12 + (SHADER_WIDTH+SHADER_PAD)*4, 5
											  , 2 * SHADER_WIDTH, ysize
											  , CST_ZOOM
											  );

		MakeTextControl( pf, 5, ysize + 14, 150, 12, TXT_STATIC, "User-Defined Colors", 0 );
		{
			int i;
			PSI_CONTROL pc;
			for( i = 0; i < 12; i++ )
			{
				pc = MakeCustomDrawnButton( pf, 5 + 18 * i, ysize + 15 + 13     , 16, 16, BTN_PRESET_BASE+i, 0, PresetDraw, PresetButton, (uintptr_t)&pcd );
				SetCommonBorder( pc, BORDER_THINNER );
				SetNoFocus( pc );
			}
			for( i = 0; i < 12; i++ )
			{
				pc = MakeCustomDrawnButton( pf, 5 + 18 * i, ysize + 15 + 13 + 18, 16, 16, BTN_PRESET_BASE+i+12, 0, PresetDraw, PresetButton, (uintptr_t)&pcd );
				SetCommonBorder( pc, BORDER_THINNER );
				SetNoFocus( pc );
			}
			for( i = 0; i < 12; i++ )
			{
				pc = MakeCustomDrawnButton( pf, 5 + 18 * i, ysize + 15 + 13 + 36, 16, 16, BTN_PRESET_BASE+i+24, 0, PresetDraw, PresetButton, (uintptr_t)&pcd );
				SetCommonBorder( pc, BORDER_THINNER );
				SetNoFocus( pc );
			}
		}
		// button style normal button
		MakeCheckButton( pf, 5, 235, 95, 19
							, BTN_PRESET, "Set Preset"
							, 0, DefinePreset, (uintptr_t)&pcd );
		MakeCheckButton( pf, 5, 218, 95, 14
							, CHK_ALPHA, "Set Alpha"
							, 0, AlphaPressed, (uintptr_t)&pcd );

		AddCommonButtons( pf, &pcd.ColorDialogDone, &pcd.ColorDialogOkay );
		SaveXMLFrame( pf, "palette.frame" );
	}
	PSI_HandleStatusEvent( pf, commonButtonStatus, (uintptr_t)&pcd );
	SetShaderControls( &pcd, NULL );
	SetSliderValues( GetControl( pf, SLD_GREENBAR ), 0, 255 - GreenVal( pcd.CurrentColor ), 255 );

	DisplayFrameOver( pf, hAbove );
	EditFrame( pf, TRUE );
#undef pcd
	return pf;

}

//----------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, PickColor )( CDATA *result, CDATA original, PSI_CONTROL hAbove, void (*ok)(uintptr_t, CDATA), uintptr_t psv )
{
	int32_t x, y;
	GetMyInterface();
	GetMousePosition( &x, &y );
	return PickColorEx( result, original, hAbove, x, y, ok, psv );
}

//----------------------------------------------------------------------------

static int CPROC ColorWellDraw( PSI_CONTROL pc )
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	if( pcw )
	{
		Image surface = GetControlSurface( pc );
		CDATA color = pcw->color;
		if( !AlphaVal( color ) )
			color = SetAlpha( color, 1 );
		//lprintf( "Clear color well surface to %lX", pcw->color );
		ClearImageTo( surface, color );
	}
	return TRUE;
}

static void ok( uintptr_t psv, CDATA color ) {
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	//PCOLOR_WELL pcw = (PCOLOR_WELL)psv;
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	pcw->color = color;
	if( pcw->UpdateProc )
		pcw->UpdateProc( pcw->psvUpdate, color );
	SmudgeCommon( pc );

}

static int CPROC ColorWellMouse( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	//if( pc->flags.bDisable ) // ignore mouse on these...
	//	return FALSE;
	if( b == -1 )
	{
		return FALSE;
	}
	if( pcw->flags.bPickColor )
	{
		if( b & ( MK_LBUTTON | MK_RBUTTON ) )
		{
			CDATA result = pcw->color;
			//if( !pcw->flags.bPicking )
			{
				//pcw->flags.bPicking = 1;
				lprintf( "PICK_COLOR" );
				PickColorEx( &result, pcw->color, GetFrame( pc ), x + FRAME_WIDTH, y + FRAME_WIDTH, ok, (intptr_t)pc );
				/*
				if(  )
				{
					lprintf( "PICK_COLOR_DONE" );
				   lprintf( "Updating my color to %08" _32fx "", result );
					pcw->color = result;
					if( pcw->UpdateProc )
				      pcw->UpdateProc( pcw->psvUpdate, result );
					SmudgeCommon( pc );
				}
				else
				{
					lprintf( "Failing to set the color.." );
					lprintf( "PICK_COLOR_DONE2" );
				}
				*/
				//DebugBreak();
				//pcw->flags.bPicking = 0;
			}
		}
	}
	return TRUE;
}

PSI_CONTROL EnableColorWellPick( PSI_CONTROL pc, LOGICAL bEnable )
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	if( pcw )
		pcw->flags.bPickColor = bEnable;
	return pc;
}

PSI_CONTROL SetOnUpdateColorWell( PSI_CONTROL pc, void(CPROC*update_proc)(uintptr_t,CDATA), uintptr_t psvUpdate)
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	if( pcw )
	{
		pcw->UpdateProc = update_proc;
		pcw->psvUpdate = psvUpdate;
	}
	return pc;
}

//----------------------------------------------------------------------------

static int CPROC ShadeWellMouse( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PSHADE_WELL, shade_well.TypeID, psw, pc );
	if( b == -1 )
		return FALSE;
	if( y < 0 )
		return 0;
	if( psw && ( b & MK_LBUTTON ) )
	{
		PPICK_COLOR_DATA ppcd = PPCD(pc);
		CDATA c;
		uint32_t height;
		Image pSurface = GetControlSurface( pc );
		height = pSurface->height;
		//lprintf( "Setting new mid color... update... ------------------------- " );
		if( SUS_LT( y, int32_t, height/2, uint32_t ) )
		{
			c = ScaleColor( psw->color_min, psw->color_mid, height/2, y );
		}
		else
		{
			c = ScaleColor( psw->color_mid, psw->color_max, height/2, y - height/2 );
		}

		SetAlpha( c
				  , ppcd->Alpha );
		//SetColorWell( ppcd->pcZoom, c );
		ppcd->CurrentColor = c;
		ppcd->flags.bSettingShade = 1;
		SetShaderControls( ppcd, pc );
		SetSliderValues( GetNearControl( pc, SLD_GREENBAR ), 0, 255-GreenVal( c ), 255 );
		ppcd->flags.bSettingShade = 0;
		// update ppcd ... how do I get that?
		// it's attached to the frame, right?  so I just
		// have to indicicate that ppcd has changed...
		SmudgeCommon( pc );
	}
	return TRUE;
}

//----------------------------------------------------------------------------

PSI_PROC( void, SetShadeMin )( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PSHADE_WELL, shade_well.TypeID, psw, pc );
	if( psw )
	{
		psw->color_min = color;
		SmudgeCommon( pc );
	}
}

//----------------------------------------------------------------------------

PSI_PROC( void, SetShadeMax )( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PSHADE_WELL, shade_well.TypeID, psw, pc );
	if( psw )
	{
		psw->color_max = color;
		SmudgeCommon( pc );
	}
}

//----------------------------------------------------------------------------

PSI_PROC( void, SetShadeMid )( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PSHADE_WELL, shade_well.TypeID, psw, pc );
	if( psw )
	{
		psw->color_mid = color;
		SmudgeCommon( pc );
	}
}

//----------------------------------------------------------------------------
PSI_PROC( CDATA, GetColorFromWell )( PSI_CONTROL pc )
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	if( pcw )
		return pcw->color;
	return 0;
}

//----------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, SetColorWell )( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	if( pcw )
	{
		pcw->color = color;
		SmudgeCommon( pc );
	}
	return pc;
}
//----------------------------------------------------------------------------

//CONTROL_PROC_DEF( COLOR_WELL, ColorWell, BORDER_THIN|BORDER_INVERT
//					 , CDATA color )
int CPROC InitColorWell( PSI_CONTROL pc )
{
	if( pc )
	{
		ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
		pcw->color = 0;
	}
	return TRUE;
}

//----------------------------------------------------------------------------

int CPROC InitShadeWell( PSI_CONTROL pc )
{
	SetShadeMin( pc, BASE_COLOR_BLACK );
	SetShadeMid( pc, BASE_COLOR_DARKGREY );
	SetShadeMax( pc, BASE_COLOR_WHITE );
	SetNoFocus( pc );
	return TRUE;
}

//----------------------------------------------------------------------------

int CPROC InitPalette( PSI_CONTROL pc )
{
	SetNoFocus( pc );
	{
		PPICK_COLOR_DATA ppcd = PPCD(pc);
	}

	return TRUE;
}

void CPROC ColorWellDestroy( PSI_CONTROL pc )
{
	ValidatedControlData( PCOLOR_WELL, color_well.TypeID, pcw, pc );
	if( pcw )
	{
		if( pcw->flags.bPicking )
		{
			lprintf( "Uhmm need to kill the parent color picking dialog!" );
		}

	}
}

//----------------------------------------------------------------------------

CONTROL_REGISTRATION color_well = { "Color Well"
											 , { {32, 32}, sizeof( COLOR_WELL ), BORDER_INVERT|BORDER_THIN }
											 , InitColorWell
											 , NULL
											 , ColorWellDraw
											 , ColorWellMouse
											 , NULL // key
				                      , ColorWellDestroy // if picking a color, destroy dialog
},
shade_well = { "Shade Well"
				 , { {32, 32}, sizeof( SHADE_WELL ), BORDER_INVERT|BORDER_THIN }
											 , InitShadeWell
											 , NULL
											 , NULL //DrawShadeControl
											 , ShadeWellMouse
											 , NULL
},
color_matrix_well = { "Color Matrix"
						  , { {xsize, ysize}, 0, BORDER_INVERT|BORDER_THIN }
											 , InitPalette
											 , NULL
											 , DrawPalette
											 , NULL //PaletteMouse
											 , NULL

}
;
PRIORITY_PRELOAD( register_well, PSI_PRELOAD_PRIORITY )
{
	DoRegisterControl( &color_well );
	DoRegisterControl( &shade_well );
	DoRegisterControl( &color_matrix_well );
}
PSI_COLORWELL_NAMESPACE_END

