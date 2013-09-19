//#include "autovers.h"
#define LOCALVERSION WIDE("0")

// should get the last portion of the version
// from plugin/module specific information...

// 2.5 changes the way command work when told to
// other entities via the /object/command or //command
// notation...  the command is performed on the object before
// the next command is processed on the teller.  Also, the true/false
// success condition comes back.  (result? ... /reply/result blah I guess will work,
// avoiding the need to automate that...)

// 2.6 changes the length of command buffer
//   - adds additional, actually preliminary, interface to C direct usage.
//

#define DEKVERSION WIDE("2.6.") LOCALVERSION

#ifdef _DEBUG
//#pragma message "Debug Version: " DEKVERSION "dbg" 
#define DekVersion WIDE("dbg") DEKVERSION 
//static TEXTCHAR *DekVersion="DEKVERSION 
#else
//#pragma message "Release Version: " DEKVERSION 
#define DekVersion DEKVERSION 
//static TEXTCHAR *DekVersion=DEKVERSION;
#endif
// $Log: my_ver.h,v $
// Revision 1.9  2005/02/21 12:08:42  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.8  2004/01/15 02:58:50  d3x0r
// Mark version as CVS.
//
// Revision 1.7  2003/04/12 20:51:18  panther
// Updates for new window handling module - macro usage easer...
//
// Revision 1.6  2003/03/25 08:59:02  panther
// Added CVS logging
//
