
#define CONTROL_SCROLL_KNOB_NAME WIDE("Scroll Knob" )

typedef void (CPROC *KnobEvent)( uintptr_t psv, int ticks );

PSI_PROC( void, SetScrollKnobEvent )( PSI_CONTROL pc, KnobEvent event, uintptr_t psvEvent );
PSI_PROC( void, SetScrollKnobImageName )( PSI_CONTROL pc, CTEXTSTR image );
PSI_PROC( void, SetScrollKnobImage )( PSI_CONTROL pc, Image image );

// fixed is defined in image.h
// angle is a fixed scaled integer with 0x1 0000 0000 being the full circle.
PSI_PROC( void, SetScrollKnobImageZeroAngle )( PSI_CONTROL pc, fixed angle );

