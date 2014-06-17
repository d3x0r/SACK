#define DEFINES_INTERSHELL_INTERFACE
#define USES_INTERSHELL_INTERFACE
#include <InterShell_export.h>

#include <InterShell_registry.h>

#define MY_CONTROL_TYPE PSI_CONTROL

#define MY_CONTROL_NAME "stress/sub canvas"

OnSaveControl( WIDE(MY_CONTROL_NAME))( FILE *file,PTRSZVAL psv )
{
	SaveCanvasConfiguration( file, (MY_CONTROL_TYPE)psv );
}

OnLoadControl( WIDE(MY_CONTROL_NAME))( PCONFIG_HANDLER pch, PTRSZVAL psvUnused )
{
	BeginCanvasConfiguration( (MY_CONTROL_TYPE)psvUnused );
}

OnGetControl( WIDE(MY_CONTROL_NAME) )( PTRSZVAL psv )
{
   return (PSI_CONTROL) ( (MY_CONTROL_TYPE)psv );
}

OnCreateControl( WIDE(MY_CONTROL_NAME))( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
{
   // return a MY_CONTROL_TYPE as a PTRSZVAL
	return (PTRSZVAL)((MY_CONTROL_TYPE)MakeNamedControl( frame // use this as the container
								  , "Menu Canvas"
								  ,x, y, w, h
								  , -1 ));
}

