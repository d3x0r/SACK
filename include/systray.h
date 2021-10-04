
#include <sack_types.h>
#ifndef __NO_MSGSVR__
#  include <msgprotocol.h>
#endif



#ifdef SYSTRAY_LIBRARAY
#define SYSTRAY_PROC EXPORT_METHOD
#else
#define SYSTRAY_PROC IMPORT_METHOD
#endif

#define ICONFROMIMAGE 1
#define ICONFROMTEXT 2


SYSTRAY_PROC int RegisterIconEx( CTEXTSTR icon DBG_PASS);
#define RegisterIcon(icon) RegisterIconEx( icon DBG_SRC )
SYSTRAY_PROC void ChangeIconEx( CTEXTSTR icon DBG_PASS );
#define ChangeIcon(icon) ChangeIconEx( icon DBG_SRC )
SYSTRAY_PROC void UnregisterIcon( void );

SYSTRAY_PROC void SetIconDoubleClick( void (*DoubleClick)(void ) );

SYSTRAY_PROC void TerminateIcon( void );
SYSTRAY_PROC void AddSystrayMenuFunction( CTEXTSTR text, void (CPROC*function)(void) );
SYSTRAY_PROC void AddSystrayMenuFunction_v2( CTEXTSTR text, void (CPROC* function)(uintptr_t), uintptr_t );

// this may be important one day!
//void SetIconMenu( HMENU menu );
// $Log: systray.h,v $
// Revision 1.3  2003/03/25 08:38:11  panther
// Add logging
//
