#ifndef DATAPATH_DEFINITION
#define DATAPATH_DEFINITION

#include "text.h"
#include <procreg.h>
//#include "links.h"


typedef int CPROC _OptionHandler( PDATAPATH pdp, PSENTIENT ps, PTEXT text );
typedef int (CPROC *OptionHandler)( PDATAPATH pdp, PSENTIENT ps, PTEXT text );

typedef struct option_entry
{
   DECLTEXTSZTYPE(name, 32);
   int8_t significant; // minimum match
   int8_t maxlen;      // maximum usable characters
   DECLTEXTSZTYPE(description, 128);
	OptionHandler function;
   POINTER unused; // need this and cmd_entry to match!
//   DECLTEXTSZ(extended_help, 256);
}option_entry;

#define HandleOption( device, name, desc )   \
	__DefineRegistryMethod2(WIDE("Dekware"),HandleOption,WIDE("devices/") device WIDE("/options"),name,desc,int,(PDATAPATH,PSENTIENT,PTEXT),__LINE__)


BLOBTYPE datapath_flags
{
//	   struct {
      //int Formatted; // eliminate leading cr/lf
      //int KeepCR;  // output wants CR with LF if possible...
      bool Closed;  // device closed connection...
      bool Dont_Poll;    // 
      bool Data_Source;  // data is generated here... (ie not filter)
      //bool Filter; // filters relay close... non filters do not relay close.
		bool Dont_Relay_Prior;  // data_source, but don't auto relay prior here.
		bool bWriteAgain;
//   } flags;

};

typedef BLOBTYPE datapath_tag {
	// callbacks to move data into and out of queues....
   PCLASSROOT pDeviceRoot;
   //struct registered_device_tag *pDevice;
	OptionHandler Option;

	// return 1 if you wish to be called again
	// return 0 you will be called next time there
   // is data on the output queue.
   int (CPROC *Write)( PDATAPATH pdp );
   int (CPROC *Read)( PDATAPATH pdp );
   int (CPROC *Close)( PDATAPATH pdp );
   int Type;      // when stream providers register, they receive an ID for this.
	PTEXT pName;
   BLOBREF datapath_flags flags;
	// generic queues....
   PLINKQUEUE Input;
   PLINKQUEUE Output;

   // used for input instructions like GETLINE/GETWORD and GETPARTIAL
   PTEXT Partial;     // used in burst on input channel... GETPARTIAL
   PTEXT CurrentWord; // used to read input with GETWORD
   PTEXT CurrentLine; // used to read input with GETWORD/GETLINE
   PDATAPATH *ppMe; // pointer to the pointer which points at me.
   PDATAPATH pPrior; // pushes prior open datapath to here...
                                // no longer requiring to close the old one...
                                // can also be used for layered
                                // datapaths such as 'trigger', 'log', ...
	struct command_info_tag *CommandInfo;
	PSENTIENT Owner;
} DATAPATH;

typedef void (CPROC *PromptCallback)( PDATAPATH );

typedef struct command_info_tag {
// -------------------- custom cmd buffer extension 
   int nHistory;  // position counter for pulling history
   PLINKQUEUE InputHistory;
   int   bRecallBegin; // set to TRUE when nHistory has wrapped...

   uint32_t   CollectionBufferLock;
   INDEX CollectionIndex;  // used to store index.. for insert type operations...
   int   CollectionInsert; // flag for whether we are inserting or overwriting
	PTEXT CollectionBuffer; // used to store partial from GatherLine
   PromptCallback Prompt;
} COMMAND_INFO, *PCOMMAND_INFO;


#endif
// $Log: datapath.h,v $
// Revision 1.20  2005/06/10 10:32:19  d3x0r
// Modify type of option functions....
//
// Revision 1.19  2005/02/21 12:08:42  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.18  2005/01/28 16:02:19  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.17  2004/09/27 16:06:47  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.16  2004/06/10 00:41:45  d3x0r
// Progress..
//
// Revision 1.15  2004/04/06 01:50:27  d3x0r
// Update to standardize device options and the processing thereof.
//
// Revision 1.14  2004/03/26 10:47:16  d3x0r
// Begin implementing /option <devname> help
//
// Revision 1.13  2003/10/26 12:38:16  panther
// Updates for newest scheduler
//
// Revision 1.12  2003/08/20 08:05:48  panther
// Fix for watcom build, file usage, stuff...
//
// Revision 1.11  2003/08/15 13:16:15  panther
// Add name to datapath - since it's not always a real device
//
// Revision 1.10  2003/03/25 08:59:02  panther
// Added CVS logging
//
