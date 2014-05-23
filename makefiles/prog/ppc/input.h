#ifndef INPUT_DEFINED
#define INPUT_DEFINED

#include "text.h"  // must be present just to process this

PTEXT burstEx( PTEXT input DBG_PASS );
#define burst(i) burstEx(i DBG_SRC )

PTEXT GatherLineEx( PTEXT *pOutput, int *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput );
#define GatherLine( out,idx,ins,cr,in) GatherLineEx( (out),(idx),(ins),(cr),(FALSE),(in))

PTEXT get_line(FILE *source, int *line);

#endif
