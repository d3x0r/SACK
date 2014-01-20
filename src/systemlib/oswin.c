#include <windows.h>
#include <objbase.h>

#ifdef __cplusplus
namespace sack { namespace system{
#endif

void InitCo( void )
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
}

#ifdef __cplusplus
}
}
#endif
