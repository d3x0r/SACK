
#include <controls.h>
#include <psi/namespaces.h>
PSI_CLOCK_NAMESPACE 

// set color of the clock text
PSI_PROC( void, SetClockColor )( PSI_CONTROL pc, CDATA color );
// set color of the area behind the text, where the clock occupies
PSI_PROC( void, SetClockBackColor )( PSI_CONTROL pc, CDATA color );
// set the background image of the clock, used instead of the back color if present.
PSI_PROC( void, SetClockBackImage )( PSI_CONTROL pc, Image image );
// get the color of the clock widget
PSI_PROC( CDATA, GetClockColor )( PSI_CONTROL pc );
// stop the clock update
PSI_PROC( void, StopClock )( PSI_CONTROL pc );
// resume clock update
PSI_PROC( void, StartClock )( PSI_CONTROL pc );
// mark the curernt time
PSI_PROC( void, MarkClock )( PSI_CONTROL pc );

// unimplmeented really?
// this should result with the difference between the marked time and now?
PSI_PROC( void, ElapseClock )( PSI_CONTROL pc );

PSI_PROC( void, SetClockAmPm )( PSI_CONTROL pc, LOGICAL yes_no );
PSI_PROC( void, SetClockDate )( PSI_CONTROL pc, LOGICAL yes_no );
PSI_PROC( void, SetClockDayOfWeek )( PSI_CONTROL pc, LOGICAL yes_no );
PSI_PROC( void, SetClockSingleLine )( PSI_CONTROL pc, LOGICAL yes_no );


struct clock_image_thing {
   IMAGE_RECTANGLE rect_face;
   IMAGE_COORDINATE center_face;
	IMAGE_RECTANGLE rect_hourhand;
   IMAGE_COORDINATE center_hourhand;
   IMAGE_RECTANGLE rect_minutehand;
   IMAGE_COORDINATE center_minutehand;
	IMAGE_RECTANGLE rect_secondhand;
   IMAGE_COORDINATE center_secondhand;

};


PSI_PROC( void, MakeClockAnalogEx )( PSI_CONTROL pc, CTEXTSTR imagename, struct clock_image_thing *description );
// calls MakeClockAnalogEx( pc, "Clock.png" );
PSI_PROC( void, MakeClockAnalog )( PSI_CONTROL pc );

PSI_CLOCK_NAMESPACE_END
   USE_PSI_CLOCK_NAMESPACE
