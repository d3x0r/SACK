#define MAKE_RCOORD_SINGLE

#include <stdhdrs.h>
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#include <render.h>
#include <image.h>
#include <controls.h>

#define SALTY_RANDOM_GENERATOR_SOURCE
#include <salty_generator.h>
#include "plasma.h"

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
	struct plasma_state *plasma;
	PRENDERER render;
	struct random_context *entropy;
	int digits;
	int decimal;
	RCOORD horz_r_scale;
} l;

void CPROC DrawPlasma( PTRSZVAL psv, PRENDERER render )
{
	Image surface = GetDisplayImage( render );
	RCOORD *data = PlasmaGetSurface( l.plasma );
	PCDATA output = GetImageSurface( surface );
	int w;
	int h;
#undef plot
#define plot(a,b,c,d) do { (*output) = d; output++; } while( 0 )
	RCOORD min = 999999999;
	RCOORD max = 0;
	for( h = 0; h < surface->height; h++ )
	{
		for( w = 0; w < surface->width; w++ )
		{
			RCOORD here = data[ h * surface->width + w ];
			if( here > max )
				max = data[ h * surface->width + w ];
			if( here < min )
				min = data[ h * surface->width + w ];
			if( here <= 0.01 )
				plot( surface, w, h, ColorAverage( BASE_COLOR_LIGHTBLUE,
												 BASE_COLOR_WHITE, here * 1000, 250 ) );
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
				plot( surface, w, h, ColorAverage( BASE_COLOR_BLACK,
												 BASE_COLOR_RED, (here-0.99) * 10000, 100 ) );
			//lprintf( "%d,%d  %g", w, h, data[ h * surface->width + w ] );
		}
	}
	lprintf( "Result is %g,%g", min, max );
	UpdateDisplay( render );
}

static void FeedRandom( PTRSZVAL psvPlasma, POINTER *salt, size_t *salt_size )
{
	static _32 tick;
	tick = timeGetTime();
	(*salt) = &tick;
	(*salt_size) = sizeof( _32 );
}

static int CPROC KeyPlasma( PTRSZVAL psv, _32 key )
{
	if( IsKeyPressed( key ) )
	{
		RCOORD coords[4];
		coords[0] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 32.0;
		coords[1] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 32.0;
		coords[2] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 32.0;
		coords[3] = SRG_GetEntropy( l.entropy, 5, FALSE ) / 32.0;
		PlasmaRender( l.plasma, coords );
		Redraw( l.render );
	}
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
	PlasmaSetRoughness( l.plasma, roughness, l.horz_r_scale );
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

static void CPROC Slider1UpdateProc(PTRSZVAL psv, PSI_CONTROL pc, int val)
{
	// value is 0-1000 for the digits place
	struct slider_panel *panel = (struct slider_panel *)psv;
	l.digits = val;
	ComputeRoughness( panel );
}

static void CPROC Slider2UpdateProc(PTRSZVAL psv, PSI_CONTROL pc, int val)
{
	// decimal shift left-right
	struct slider_panel *panel = (struct slider_panel *)psv;
	l.decimal = val;
	ComputeRoughness( panel );
}

static void CPROC Slider3UpdateProc(PTRSZVAL psv, PSI_CONTROL pc, int val)
{
	// hoirzonatal random modifier compared to center
	struct slider_panel *panel = (struct slider_panel *)psv;
	l.horz_r_scale = val / 100.0;
	ComputeRoughness( panel );
}

static void CPROC Slider4UpdateProc(PTRSZVAL psv, PSI_CONTROL pc, int val)
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
	SetSliderUpdateHandler( pc, Slider1UpdateProc, (PTRSZVAL)panel );

	pc = panel->slider2 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 120, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, -10, 0, 10 );
	SetSliderUpdateHandler( pc, Slider2UpdateProc, (PTRSZVAL)panel );

	pc = panel->slider3 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 140, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, 1, 100, 200 );
	SetSliderUpdateHandler( pc, Slider3UpdateProc, (PTRSZVAL)panel );

	pc = panel->slider4 = MakeNamedControl( panel->frame, SLIDER_CONTROL_NAME, 10, 160, 480, 20, -1 );
	SetSliderOptions( pc, SLIDER_HORIZ );
	SetSliderValues( pc, 1, 100, 100 );
	SetSliderUpdateHandler( pc, Slider4UpdateProc, (PTRSZVAL)panel );

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

	l.entropy = SRG_CreateEntropy( FeedRandom, 0 );
	l.render = OpenDisplaySized( 0, 256, 256 );
	l.horz_r_scale = 0.75;
	l.decimal = 0;
	l.digits = 201;
	SetRedrawHandler( l.render, DrawPlasma, 0 );
	SetKeyboardHandler( l.render, KeyPlasma, 0 );

	{
		struct slider_panel *panel = MakeSliderFrame();
		DisplayFrame( panel->frame );
	}
	coords[0] = 1.0;
	coords[1] = 0.0;
	coords[2] = 0.0;
	coords[3] = 1.0;
	l.plasma = PlasmaCreate( coords, 1.0, 256, 256 );
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
