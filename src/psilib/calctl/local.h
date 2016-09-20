
#include <psi.h>
#include <image.h>

typedef struct global_tag
{
	PIMAGE_INTERFACE MyImageInterface;
	PRENDER_INTERFACE MyDisplayInterface;
	PLIST clocks;
} GLOBAL;
#ifndef CLOCK_CORE
extern
#endif
GLOBAL global_calender_structure;


#if !defined( CLOCK_CORE ) || defined( _MSC_VER ) || defined( __cplusplus )
extern
	CONTROL_REGISTRATION clock_control;
#endif

PSI_CLOCK_NAMESPACE

typedef struct analog_clock ANALOG_CLOCK, *PANALOG_CLOCK;

typedef struct clock_control_tag
{
	struct {
		BIT_FIELD bStopped : 1;
		BIT_FIELD bLocked : 1;
		BIT_FIELD bHighTime : 1;
		BIT_FIELD bAmPm : 1;
		BIT_FIELD bDate : 1;
		BIT_FIELD bDayOfWeek : 1;
		BIT_FIELD bSingleLine : 1;
	} flags;
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
	PREFIX_PACKED struct {
      uint16_t ms; // milliseconds
		uint8_t sc;
		uint8_t mn;
		uint8_t hr;
		uint8_t dy;
		uint8_t dow;
		uint8_t mo;
		uint16_t yr;
	} PACKED time_data;
#ifdef _MSC_VER
#pragma pack (pop)
#endif
	CDATA textcolor;
	PTEXT time;
	CDATA backcolor;
	Image back_image;
	PANALOG_CLOCK analog_clock;
	TEXTSTR last_time;
} CLOCK_CONTROL, *PCLOCK_CONTROL;

void DrawAnalogClock( PSI_CONTROL pc );


PSI_CLOCK_NAMESPACE_END
