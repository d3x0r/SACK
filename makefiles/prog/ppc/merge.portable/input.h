#ifndef INPUT_DEFINED
#define INPUT_DEFINED

#include "sack_ucb_filelib.h"
#undef burst
//#include "text.h"  // must be present just to process this

PTEXT PPC_burstEx( PTEXT input DBG_PASS );
#define PPC_burst( i ) PPC_burstEx( i DBG_SRC )

PTEXT GatherLineEx( PTEXT *pOutput, int *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput );
#define GatherLine( out,idx,ins,cr,in) GatherLineEx( (out),(idx),(ins),(cr),(FALSE),(in))

PTEXT get_line(FILE *source, int *line);

#endif
