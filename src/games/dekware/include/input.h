#ifndef INPUT_DEFINED
#define INPUT_DEFINED

#include "space.h"
#include "datapath.h"

CORE_PROC( PCOMMAND_INFO, CreateCommandHistoryEx )( PromptCallback prompt );

CORE_PROC( void, DestroyCommandHistory )( PCOMMAND_INFO *pci );

// negative with SEEK_SET is SEEK_END -nPos
#define COMMAND_POS_SET SEEK_SET
#define COMMAND_POS_CUR SEEK_CUR
CORE_CPROC( int, SetCommandPosition )( PCOMMAND_INFO pci, int nPos, int whence );

// bInsert < 0 toggle insert.  bInsert == 0 clear isnert(set overwrite) else
// set insert (clear overwrite )
CORE_CPROC( void, SetCommandInsert )( PCOMMAND_INFO pci, int bInsert );


CORE_CPROC( PTEXT, GatherLineEx )( PTEXT *pOutput, INDEX *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput );
#define GatherLine( out,idx,ins,cr,in) GatherLineEx( (out),(idx),(ins),(cr),(FALSE),(in))
CORE_CPROC( void, RecallCommand )( PCOMMAND_INFO pci, int bUp );
CORE_CPROC( void, EnqueHistory )( PCOMMAND_INFO pci, PTEXT pHistory );

CORE_CPROC( PTEXT, GatherCommand )( PCOMMAND_INFO pci, PTEXT stroke );



#endif
// $Log: input.h,v $
// Revision 1.8  2004/06/10 00:41:46  d3x0r
// Progress..
//
// Revision 1.7  2004/06/07 08:32:59  d3x0r
// checkpoint.
//
// Revision 1.6  2004/05/14 18:24:45  d3x0r
// Extend command input module to provide full command queue functionality through methods.
//
// Revision 1.5  2003/03/25 08:59:02  panther
// Added CVS logging
//
