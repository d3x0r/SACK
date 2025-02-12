#undef g
#define g global_calender_structure
//#ifndef __cplusplus_cli
#ifndef USE_RENDER_INTERFACE
#  define USE_RENDER_INTERFACE (g.MyDisplayInterface?g.MyDisplayInterface:(g.MyDisplayInterface=GetDisplayInterface() ))
#endif
#ifndef USE_IMAGE_INTERFACE
#  define USE_IMAGE_INTERFACE (g.MyImageInterface?g.MyImageInterface:(g.MyImageInterface=GetImageInterface() ))
#endif
//#endif


#include <stdhdrs.h>
#include <sharemem.h>
#include <sqlgetoption.h>
#include <psi.h>
#include <psi/clock.h>
#include "local.h"

PSI_CLOCK_NAMESPACE

#define CLOCK_NAME "Basic Clock Widget"
extern CONTROL_REGISTRATION clock_control;


struct analog_clock
{
	struct {
		uint32_t bLocked : 1;
	} flags;
	PRENDERER render;
	Image image;
	Image face;
	Image composite; // size of face, work space to add hands.  Clock face created always.
	uint32_t w, h;
	PSPRITE second_hand;
	PSPRITE minute_hand;
	PSPRITE hour_hand;
	struct {
		uint32_t xofs;
		uint32_t yofs;
	} face_center;
	PCLOCK_CONTROL clock;
};


static void psiAnalogDrawClock( Image surface, PANALOG_CLOCK analog )
{
	{
		if( analog )
		{
			int remake = 0;
			if( analog->flags.bLocked )
				return;
			analog->flags.bLocked = 1;

			SetSpritePosition( analog->second_hand, surface->width/2, surface->height/2 );
			SetSpritePosition( analog->minute_hand, surface->width/2, surface->height/2 );
			SetSpritePosition( analog->hour_hand, surface->width/2, surface->height/2 );
			if( SUS_GT(surface->height,int32_t,analog->h,uint32_t) )
			{
				remake = 1;
				analog->h = surface->height;
			}
			if( SUS_GT(surface->width ,int32_t,analog->w,uint32_t) )
			{
				remake = 1;
				analog->w = surface->width;
			}
			if( remake )
			{
				UnmakeImageFile( analog->composite );
				analog->composite = MakeImageFile( analog->w, analog->h );
			}
			analog->composite = surface;

			ClearImageTo( analog->composite, 0 );
			//BlotScaledImageAlpha( surface, analog->composite, ALPHA_TRANSPARENT );
			BlotScaledImageSizedToAlpha( analog->composite, analog->face
												, 0, 0
												, surface->width, surface->height, ALPHA_TRANSPARENT );
			//BlotImage( analog->composite, analog->face, 0, 0);
			{
				//PANALOG_CLOCK analog = (PANALOG_CLOCK)psv;
				//Image surface = GetDisplayImage( renderer );
				rotate_scaled_sprite( analog->composite, analog->second_hand
										  , ( analog->clock->time_data.sc * 0x100000000LL ) / 60
										  + ( analog->clock->time_data.ms * 0x100000000LL ) / (1000*60)
										  , ( 0x10000 * surface->width ) / analog->face->width
										  , ( 0x10000 * surface->height ) / analog->face->height
										  );
				rotate_scaled_sprite( analog->composite, analog->minute_hand
										  , ( analog->clock->time_data.mn * 0x100000000LL ) / 60
											+ ( analog->clock->time_data.sc * 0x100000000LL ) / (60*60)
										  , ( 0x10000 * surface->width ) / analog->face->width
										  , ( 0x10000 * surface->height ) / analog->face->height
										  );
				rotate_scaled_sprite( analog->composite, analog->hour_hand
										  , ( analog->clock->time_data.hr * 0x100000000LL ) / 12
											+ ( analog->clock->time_data.mn * 0x100000000LL ) / (60*12)
										  , ( 0x10000 * surface->width ) / analog->face->width
										  , ( 0x10000 * surface->height ) / analog->face->height
										  );
			}
			//xlprintf(LOG_NOISE-1)( "Surface is %ld,%ld,%ld", surface, surface->x, surface->y );
			if( surface != analog->composite )
				BlotImageAlpha( surface, analog->composite, 0, 0, ALPHA_TRANSPARENT );
			//BlotImageSizedAlpha( surface, analog->composite, 0, 0, surface->width, surface->height, ALPHA_TRANSPARENT );
			//BlotScaledImageAlpha( surface, analog->composite, ALPHA_TRANSPARENT );
			analog->flags.bLocked = 0;
		}
	}
}

static void OnRevealCommon( CLOCK_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, (PSI_CONTROL)pc );
	if( clock )
	{
#if draw_on_renderer
		PANALOG_CLOCK analog = clock->analog_clock;
		if( analog )
		{
			RestoreDisplay( analog->render );
		}
#endif
	}
}

static void OnHideCommon( CLOCK_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, (PSI_CONTROL)pc );
	if( clock )
	{
		PANALOG_CLOCK analog = clock->analog_clock;
		if( analog )
		{
#if draw_on_renderer
			HideDisplay( analog->render );
#endif
		}
	}

}

static void MoveSurface( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, (PSI_CONTROL)pc );
	if( clock )
	{
		PANALOG_CLOCK analog = clock->analog_clock;
		if( analog )
		{
			//Image surface = GetControlSurface( pc );
			//PRENDERER r = GetFrameRenderer( GetFrame( pc ) );
			int32_t x = 0;
			int32_t y = 0;
			GetPhysicalCoordinate( pc, &x, &y, TRUE, FALSE );
#if draw_on_renderer
			if( analog->render )
				MoveDisplay( analog->render, x, y );
#endif
		}
	}
}


static void OnMoveCommon( CLOCK_NAME )( PSI_CONTROL pc, LOGICAL changing )
{
	if( !changing )
		MoveSurface( pc );
}

static void OnMotionCommon( CLOCK_NAME )( PSI_CONTROL pc, LOGICAL changing )
{
	if( !changing )
		MoveSurface( pc );
}



static void OnSizeCommon( CLOCK_NAME )( PSI_CONTROL pc, LOGICAL changing )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, (PSI_CONTROL)pc );
	if( !changing && clock )
	{
		PANALOG_CLOCK analog = clock->analog_clock;
		if( analog )
		{
#if draw_on_renderer
			Image surface = GetControlSurface( pc );
			if( analog->render )
				SizeDisplay( analog->render, surface->width, surface->height );
#endif
		}
	}
}


#ifdef draw_on_renderer
void CPROC DrawClockLayers( uintptr_t psv, PRENDERER renderer )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, (PSI_CONTROL)psv );
	if( clock )
	{
		PANALOG_CLOCK analog = clock->analog_clock;
		Image surface = GetDisplayImage( renderer );
		psiAnalogDrawClock( surface, analog );
		UpdateDisplay( renderer );
	}
}
#endif

void DrawAnalogClock( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, pc );
	if( clock )
	{
		PANALOG_CLOCK analog = clock->analog_clock;
		Image surface = GetControlSurface( pc );
		//Redraw( analog->render );
		psiAnalogDrawClock( surface, analog );
	}
}
void MakeClockAnalogEx( PSI_CONTROL pc, CTEXTSTR imagename, struct clock_image_thing *description )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, clock, pc );
	if( clock )
	{
		PANALOG_CLOCK analog = clock->analog_clock;
		if( !analog )
		{
			if( !imagename )
				return; // no image? well then no analog clock for you.
			analog = (PANALOG_CLOCK)Allocate( sizeof( ANALOG_CLOCK ) );
			MemSet( analog,0,sizeof( ANALOG_CLOCK ) );
			analog->clock = clock;
			analog->image = LoadImageFile( imagename );
			if( !analog->image )
			{
				Release( analog );
				return;
			}
			{
				int x, y, w, h;
				int face_center_x, face_center_y;
				int second_hand_center, second_hand_width, second_hand_pivot;
				int minute_hand_center, minute_hand_width, minute_hand_pivot;
				int hour_hand_center, hour_hand_width, hour_hand_pivot;
				TEXTCHAR tmp[256];
				tnprintf( tmp, sizeof( tmp ), "Analog Clock/%s", imagename );
#ifdef __NO_OPTIONS__
#undef SACK_GetProfileInt
#define SACK_GetProfileInt(a,b,c) (c)
#endif

				x = SACK_GetProfileInt( tmp, "face x", 0 );
				y = SACK_GetProfileInt( tmp, "face y", 0 );
				w = SACK_GetProfileInt( tmp, "face width", 358 );
				h = SACK_GetProfileInt( tmp, "face height", 358 );
				face_center_x = SACK_GetProfileInt( tmp, "face.center.x", 178 );
				face_center_y = SACK_GetProfileInt( tmp, "face.center.y", 179 );
				second_hand_center = SACK_GetProfileInt( tmp, "hand.second.center", 400 );
				second_hand_width = SACK_GetProfileInt( tmp, "hand.second.width", 40 );
				second_hand_pivot = SACK_GetProfileInt( tmp, "hand.second.pivot", 179 );
				minute_hand_center = SACK_GetProfileInt( tmp, "hand.minute.center", 440 );
				minute_hand_width = SACK_GetProfileInt( tmp, "hand.minute.width", 40 );
				minute_hand_pivot = SACK_GetProfileInt( tmp, "hand.minute.pivot", 179 );
				hour_hand_center = SACK_GetProfileInt( tmp, "hand.hour.center", 480 );
				hour_hand_width = SACK_GetProfileInt( tmp, "hand.hour.width", 40 );
				hour_hand_pivot = SACK_GetProfileInt( tmp, "hand.hour.pivot", 179 );
			analog->face = MakeSubImage( analog->image, x, y, w, h );
			analog->composite = MakeImageFile( w, h );
			analog->w = w;
			analog->h = h;
			analog->second_hand = MakeSpriteImage( MakeSubImage( analog->image, second_hand_center-(second_hand_width/2), y, second_hand_width, h ) );
			analog->minute_hand = MakeSpriteImage( MakeSubImage( analog->image, minute_hand_center-(minute_hand_width/2), y, minute_hand_width, h ) );
			analog->hour_hand = MakeSpriteImage( MakeSubImage( analog->image, hour_hand_center-(hour_hand_width/2), y, hour_hand_width, h ) );
			analog->face_center.xofs = face_center_x;
			analog->face_center.yofs = face_center_y;
			SetSpritePosition( analog->second_hand, analog->face_center.xofs, analog->face_center.yofs );
			SetSpritePosition( analog->minute_hand, analog->face_center.xofs, analog->face_center.yofs );
			SetSpritePosition( analog->hour_hand, analog->face_center.xofs, analog->face_center.yofs );
			SetSpriteHotspot( analog->second_hand, minute_hand_width/2, second_hand_pivot );
			SetSpriteHotspot( analog->minute_hand, minute_hand_width/2, minute_hand_pivot );
			SetSpriteHotspot( analog->hour_hand, hour_hand_width/2, hour_hand_pivot );
			}
#if draw_on_renderer
			if( 0 )
			{
				Image surface = GetControlSurface( pc );
				PRENDERER r = GetFrameRenderer( GetFrame( pc ) );
				int32_t x = 0;
				int32_t y = 0;
				GetPhysicalCoordinate( pc, &x, &y, FALSE );
					lprintf( "Making clock uhm... %d %d %d %d over %p", x, y,surface->width
																	 , surface->height );
				analog->render = OpenDisplayAboveSizedAt( DISPLAY_ATTRIBUTE_LAYERED
																	  |DISPLAY_ATTRIBUTE_NO_MOUSE
																	  |DISPLAY_ATTRIBUTE_CHILD // mark that this is uhmm intended to not be a alt-tabbable window
																	 , surface->width
																	 , surface->height
																	 , x, y
																	 , r // r may not exist yet... we might just be over a control that is frameless... later we'll relate as child
																	 );
				UpdateDisplay( analog->render );
				SetRedrawHandler( analog->render, DrawClockLayers, (uintptr_t)pc );
			}
#endif
			clock->analog_clock = analog;
			//EnableSpriteMethod( GetFrameRenderer( GetFrame( pc ) ), DrawAnalogHands, (uintptr_t)analog );
		}
		else
		{
			if( !imagename )
			{
				UnmakeImageFile( analog->composite );
				UnmakeImageFile( analog->face );
				UnmakeSprite( analog->second_hand, TRUE );
				UnmakeSprite( analog->minute_hand, TRUE );
				UnmakeSprite( analog->hour_hand, TRUE );
				Release( analog );
				clock->analog_clock = NULL;
			}
			// reconfigure
		}
	}

}


void MakeClockAnalog( PSI_CONTROL pc )
{
	TEXTCHAR namebuf[256];
#ifndef __NO_OPTIONS__
	SACK_GetProfileString( GetProgramName(), "Analog Clock/Use Image", "images/Clock.png", namebuf, 256 );
#else
   StrCpy( namebuf, "images/Clock.png" );
#endif
	MakeClockAnalogEx( pc, namebuf, NULL );
}

PSI_CLOCK_NAMESPACE_END




#undef g
