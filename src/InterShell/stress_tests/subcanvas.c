#define DEFINES_INTERSHELL_INTERFACE
#define USES_INTERSHELL_INTERFACE
#include <InterShell_export.h>

#include <InterShell_registry.h>

#define MY_CONTROL_TYPE PSI_CONTROL

#define MY_CONTROL_NAME "stress/sub canvas"

OnSaveControl( WIDE(MY_CONTROL_NAME))( FILE *file,uintptr_t psv )
{
	SaveCanvasConfiguration( file, (MY_CONTROL_TYPE)psv );
}

OnLoadControl( WIDE(MY_CONTROL_NAME))( PCONFIG_HANDLER pch, uintptr_t psvUnused )
{
	BeginCanvasConfiguration( (MY_CONTROL_TYPE)psvUnused );
}

OnGetControl( WIDE(MY_CONTROL_NAME) )( uintptr_t psv )
{
   return (PSI_CONTROL) ( (MY_CONTROL_TYPE)psv );
}

OnCreateControl( WIDE(MY_CONTROL_NAME))( PSI_CONTROL frame, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
   // return a MY_CONTROL_TYPE as a uintptr_t
	return (uintptr_t)((MY_CONTROL_TYPE)MakeNamedControl( frame // use this as the container
								  , "Menu Canvas"
								  ,x, y, w, h
								  , -1 ));
}

