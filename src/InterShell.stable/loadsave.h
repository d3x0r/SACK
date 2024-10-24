#ifndef LOADSAVE_EVOMENU_DEFINED
#define LOADSAVE_EVOMENU_DEFINED
#include "intershell_local.h"
#include <configscript.h>
#ifdef SACK_CORE_BUILD
#include <../src/contrib/genx/genx.h>
#else
#include <genx/genx.h>
#endif

INTERSHELL_NAMESPACE

void LoadButtonConfig( PSI_CONTROL pc_canvas, TEXTSTR filename );
//void LoadButtonConfig( void );
void SaveButtonConfig( PSI_CONTROL pc_canvas, TEXTSTR filename );
// for saving sub-canvases...
// or for controls to save a canavas control...

/* used by macros.c when loading the startup macro which is a button, but not really quite a button. */
void SetCurrentLoadingButton( PMENU_BUTTON button );

PSI_CONTROL CPROC GetCurrentLoadingCanvas( void );
PSI_CONTROL CPROC GetCurrentSavingCanvas( void );

INTERSHELL_NAMESPACE_END

#endif
