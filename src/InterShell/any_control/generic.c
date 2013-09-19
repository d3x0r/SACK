
#include <intershell_registry.h>

OnCreateControl( "Any Generic" )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
   return (PTRSZVAL)MakeNamedControl( parent, "Whiteboard Client Surface", x, y, w, h, -1 );
}

OnGetControl( "Any Generic" )( PTRSZVAL psv )
{
   return (PSI_CONTROL)psv;
}


