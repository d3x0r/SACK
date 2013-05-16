#ifndef TEXT_SUPPORT
#define TEXT_SUPPORT

#include <string.h>
#include <sack_types.h>

#define TF_ENTITY_FLAG 0x00200000
#define TF_ENTITY    (TF_ENTITY_FLAG|TF_APPLICATION|TF_BINARY|TF_INDIRECT) // pointers to internal object types
#define TF_SENTIENT_FLAG 0x00400000
#define TF_SENTIENT  (TF_SENTIENT_FLAG|TF_APPLICATION|TF_BINARY|TF_INDIRECT) // pointers to internal object types
#define TF_PROMPT    0x00800000   // Segment was generated as a prompt
#define TF_ADDRESS   (TF_BINARY|0x01000000)
#define TF_PLUGIN    0xFC000000   // set aside some flags for usage only by plugins
#define TF_RELAY     (0x02000000|TF_APPLICATION|TF_BINARY)

#endif
// $Log: text.h,v $
// Revision 1.9  2004/05/04 07:30:27  d3x0r
// Checkpoint for everything else.
//
// Revision 1.8  2003/04/13 22:28:51  panther
// Seperate computation of lines and rendering... mark/copy works (wincon)
//
// Revision 1.7  2003/03/25 08:59:02  panther
// Added CVS logging
//
