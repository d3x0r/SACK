
#include <stdhdrs.h>
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#include <render.h>
#include <image.h>
#include <controls.h>

//#define SALTY_RANDOM_GENERATOR_SOURCE
#include <salty_generator.h>
#include "plasma.h"
#include "grid_reader.h" 
#include "dds_image.h"

#define patch  512
//#define patch 65
//1025  
//#define patch 257

struct slider_panel
{
	PSI_CONTROL frame;
	PSI_CONTROL slider1;
	PSI_CONTROL slider2;
	PSI_CONTROL slider3;
	PSI_CONTROL slider4;
	PSI_CONTROL value1;
	PSI_CONTROL value2;
	PSI_CONTROL value3;

};

struct plasma_local
{
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	struct plasma_patch *plasma;
	PRENDERER render;
	struct random_context *entropy;
	int digits;
	int decimal;
	RCOORD horz_r_scale;
	int32_t ofs_x, ofs_y;
	int32_t mouse_x, mouse_y;
	uint32_t mouse_b; 

	struct grid_reader *grid_reader;
} l;

uintptr_t CPROC Mouse( uintptr_t psv, int32_t x, int32_t y, uint32_t b )
{
	if( MAKE_FIRSTBUTTON( b, l.mouse_b ) )
	{
		l.mouse_x = x;
		l.mouse_y = y;
	}
	else if( MAKE_SOMEBUTTONS( b ) )
	{
		l.ofs_x += l.mouse_x - x;
		l.ofs_y += l.mouse_y - y;
		l.mouse_x = x;
		l.mouse_y = y;
		Redraw( l.render );
	}
	else
	{

	}
	l.mouse_b = b;
	return 1;
}

int CPROC DrawPlasma( uintptr_t psv, PRENDERER render )
{
	Image surface = GetDisplayImage( render );
	LOGICAL use_grid_reader = FALSE;
	RCOORD *data 
		= use_grid_reader?GridReader_Read( l.grid_reader, 1000 + l.ofs_x, 1000 + l.ofs_y, patch, patch )
			: PlasmaReadSurface( l.plasma, l.ofs_x, l.ofs_y, 0 );
	PCDATA _output = GetImageSurface( surface );
	PCDATA output;
	int virtual_surface;
	int w;
	int h;
#undef plot
#define plot(a,b,c,d) do{ if( !virtual_surface ){ do { (*output) = d; output++; } while( 0 ); }else plotalpha( a, b, c,d ); }while(0)
	RCOORD min = 999999999;
	RCOORD max = 0;
	lprintf( "begin render" );
	if( surface->flags & IF_FLAG_FINAL_RENDER )
		virtual_surface = 1;
	else
		virtual_surface = 0;
	if( !data )
		return 0;

	for( h = 0; h < surface->height; h++ )
	{
		output = _output + ( surface->height - h - 1 ) * surface->pwidth;

		for( w = 0; w < surface->width; w++ )
		{
			RCOORD here = data[ h * patch + w ];
			if( 0 ) {
				if( here < 0.9 && here > 0.1 ) {
					plot( surface, w, h, BASE_COLOR_BLACK );
					continue;
				}
			}
			//plot( surface, w, h, ColorAverage( BASE_COLOR_BLACK,
			//									 BASE_COLOR_RED, (here) * 1000, 1000 ) );
//#if 0
			if( here <= 0.01 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_WHITE,
												 BASE_COLOR_BLACK, here * 1000, 10 ) );
			else if( here <= 0.25 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_BLACK,
												 BASE_COLOR_LIGHTBLUE, (here) * 1000, 250 ) );
			else if( here <= 0.5 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_LIGHTBLUE,
												 BASE_COLOR_LIGHTGREEN, (here-0.25) * 1000, 250 ) );
			else if( here <= 0.75 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_LIGHTGREEN,
												 BASE_COLOR_LIGHTRED, (here-0.5) * 1000, 250 ) );
			else if( here <= 0.99 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_LIGHTRED,
												 BASE_COLOR_WHITE, (here-0.75) * 1000, 250 ) );
			else //if( here <= 4.0 / 4 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_WHITE,
												 BASE_COLOR_BLACK, (here-0.99) * 10000, 100 ) );
//#endif
			//lprintf( "%d,%d  %g", w, h, data[ h * surface->width + w ] );
		}
	}
	lprintf( "Result is %g,%g", min, max );
	UpdateDisplay( render );
	if( use_grid_reader )
		Release( data );
	return 1;
}

static void FeedRandom( uintptr_t psvPlasma, POINTER *salt, size_t *salt_size )
{
	static uint32_t tick;
	tick = timeGetTime();
	(*salt) = &tick;
	(*salt_size) = sizeof( uint32_t );
}

static int CPROC KeyPlasma( uintptr_t psv, uint32_t key )
{
	if( IsKeyPressed( key ) )
	{
		RCOORD coords[4];
		if( KEY_CODE( key ) == KEY_W )
		{
			RCOORD *data = PlasmaReadSurface( l.plasma, l.ofs_x, l.ofs_y, 0 );
			lprintf( "begin render" );
			WriteImage( "plasma.dds", data, patch-1, patch-1, patch );
		}
		else if( KEY_CODE( key ) == KEY_R )
		{
			//RCOORD *data = PlasmaReadSurface( l.plasma, l.ofs_x, l.ofs_y, 0 );
			//lprintf( "begin render" );
			ReadImage( "TerrainNormal_Height.dds" );
		}
		else
		{
			coords[0] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 132.0 + 0.5;
			coords[1] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 132.0 + 0.5;
			coords[2] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 132.0 + 0.5;
			coords[3] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 132.0 + 0.5;
			PlasmaRender( l.plasma, coords );
			Redraw( l.render );
		}
	}
	return 1;
}


static void ComputeRoughness( struct slider_panel *panel )
{
	int n;
	RCOORD base = 1;
	RCOORD roughness;
	if( l.decimal > 0 )
	{
		for( n = 0; n < l.decimal; n++ )
			base = base * 10;
	}
	else
	{
		for( n = 0; n > l.decimal; n-- )
			base = base / 10;
	}
	roughness = (RCOORD)base * (RCOORD)l.digits;
	roughness = ( roughness / 512 )/* * patch*/;
	PlasmaSetGlobalRoughness( l.plasma, roughness, l.horz_r_scale );
	{
		TEXTCHAR val[25];
		snprintf( val, 25, "%d", l.digits );
		SetControlText( panel->value1, val );
		snprintf( val, 25, "%d", l.decimal );
		SetControlText( panel->value2, val );
		snprintf( val, 25, "%g", roughness);
		SetControlText( panel->value3, val );
	}
	PlasmaRender( l.plasma, NULL );
	Redraw( l.render );
}

static void CPROC Slider1UpdateProc(uintptr_t psv, PSI_CONTROL pc, int val)
{
	// value is 0-1000 for the digits place
	struct slider_panel *panel = (struct slider_panel *)psv;
	l.digits = val;
	ComputeRoughness( panel );
}

static void CPROC Slider2UpdateProc(uintptr_t psv, PSI_CONTROL pc, int val)
{
	// decimal shift left-right
	struct slider_panel *panel = (struct slider_panel *)psv;
	l.decimal = val;
	ComputeRoughness( panel );
}

static void CPROC Slider3UpdateProc(uintptr_t psv, PSI_CONTROL pc, int val)
{
	// hoirzonatal random modifier compared to center
	struct slider_panel *panel = (struct slider_panel *)psv;
	l.horz_r_scale = val / 100.0;
	ComputeRoughness( panel );
}

static void CPROC Slider4UpdateProc(uintptr_t psv, PSI_CONTROL pc, int val)
{
	// 
	struct slider_panel *panel = (struct slider_panel *)psv;
	//l.horz_r_scale = val / 100.0;
	//ComputeRoughness( panel );
}

struct slider_panel *MakeSliderFrame( void )
{
	struct slider_panel *panel = New( struct slider_panel );
	PSI_CONTROL pc;
	panel->frame = CreateFrame( "Plasma Sliders", -1, -1, 500, 200, 0, NULL );

	pc = panel->slider1 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 100, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, 1, 500, 1000 );
	SetSliderUpdateHandler( pc, Slider1UpdateProc, (uintptr_t)panel );

	pc = panel->slider2 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 120, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, -10, 0, 10 );
	SetSliderUpdateHandler( pc, Slider2UpdateProc, (uintptr_t)panel );

	pc = panel->slider3 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 140, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, 1, 100, 200 );
	SetSliderUpdateHandler( pc, Slider3UpdateProc, (uintptr_t)panel );

	pc = panel->slider4 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 160, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, 1, 100, 100 );
	SetSliderUpdateHandler( pc, Slider4UpdateProc, (uintptr_t)panel );

	panel->value1 = MakeNamedControl( panel->frame, STATIC_TEXT_NAME, 10, 10, 100, 20, -1 );
	panel->value2 = MakeNamedControl( panel->frame, STATIC_TEXT_NAME, 120, 10, 100, 20, -1 );
	panel->value3 = MakeNamedControl( panel->frame, STATIC_TEXT_NAME, 230, 10, 100, 20, -1 );
	return panel;
}

SaneWinMain( argc, argv )
{

	RCOORD coords[4];
	l.pii = GetImageInterface();
	l.pdi = GetDisplayInterface();
	l.entropy = SRG_CreateEntropy2( FeedRandom, 0 );
	//lprintf( "First log" );
	//SetAllocateLogging( 1 );
	l.render = OpenDisplaySized( 0, patch, patch );
	l.horz_r_scale = 0.75;
	l.decimal = 0;
	l.digits = 201;
	SetRedrawHandler( l.render, DrawPlasma, 0 );
	SetKeyboardHandler( l.render, KeyPlasma, 0 );
	SetMouseHandler( l.render, Mouse, 0 );


	//l.grid_reader = GridReader_Open( "c:/storage/maps/nevada_gis/usa_elevation/usa1_alt" );


	{
		struct slider_panel *panel = MakeSliderFrame();
		DisplayFrame( panel->frame );
	}
	coords[0] = 200.0;
	coords[1] = -200.0;
	coords[2] = 200.0;
	coords[3] = -200.0;
	l.plasma = PlasmaCreate( coords, 0.5/*patch * 2*/, patch, patch );
	UpdateDisplay( l.render );
	//RestoreDisplay( l.render );

	while( 1 )
	{
		WakeableSleep( 25000 );
		/*
		coords[0] = SRG_GetEntropy( entropy, 5, FALSE ) / 32.0;
		coords[1] = SRG_GetEntropy( entropy, 5, FALSE ) / 32.0;
		coords[2] = SRG_GetEntropy( entropy, 5, FALSE ) / 32.0;
		coords[3] = SRG_GetEntropy( entropy, 5, FALSE ) / 32.0;
		PlasmaRender( l.plasma, coords );
		UpdateDisplay( l.render );
		*/
	}
}
EndSaneWinMain()
