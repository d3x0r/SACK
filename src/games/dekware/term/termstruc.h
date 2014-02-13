#include "network.h"

#define PLUGIN_MODULE
#include "plugin.h"

typedef struct mydatapath_tag {
   DATAPATH common;
   COMMAND_INFO CommandInfo;
   PTEXT    Buffer;
   PCLIENT  handle;
   struct {
      int      bBinary:1;
      int      bAnsi:1;
      int      bIRC:1;
      int      bNewLine:1; // prior line parsed was terminated with newline...
   }flags;
   int      nBinaryRead;
   PTEXT    pBinaryRead; // pointer to next buffer to read into...

   int bCommandOut;  // when building output insert prefixed newlines...

   unsigned char iac_data[256];
   int iac_count;

   // ANSI burst state variables...
   int nState;
   int nParams;
   int ParamSet[12];

   FORMAT attribute; // keep this for continuous attributes across buffers
   int bAttribute;   // store that this attribute has not been used yet...

} MYDATAPATH, *PMYDATAPATH;
// $Log: termstruc.h,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
