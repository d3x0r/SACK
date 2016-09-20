
#include <intershell_registry.h>

OnCreateControl( "Any Generic" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
   return (uintptr_t)MakeNamedControl( parent, "Whiteboard Client Surface", x, y, w, h, -1 );
}

OnGetControl( "Any Generic" )( uintptr_t psv )
{
   return (PSI_CONTROL)psv;
}


