
#include <controls.h>
// filter == something like "Bodies\0*.Body\0"
LOGICAL SelectExistingFile( PSI_CONTROL parent, TEXTCHAR * szFile, uint32_t buflen, TEXTCHAR * filter );
LOGICAL SelectNewFile( PSI_CONTROL parent, TEXTCHAR * szFile, uint32_t buflen, TEXTCHAR * filter );
