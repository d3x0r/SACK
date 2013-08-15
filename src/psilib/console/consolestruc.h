#ifndef MY_DATAPATH_DEFINED
#define MY_DATAPATH_DEFINED

#ifndef CORECON_SOURCE
#if defined( SACK_BAG_EXPORTS ) || defined( PSI_CONSOLE_SOURCE )
#define CORECON_SOURCE
#endif
#endif

#ifdef BCC16
#ifdef CORECON_SOURCE
#define CORECON_PROC(type,name) type STDPROC _export name
#else
#define CORECON_PROC(type,name) type STDPROC name
#endif
#else
#if !defined(__STATIC__) && !defined(__UNIX__)
#ifdef CORECON_SOURCE
#define CORECON_NPROC(type,name) EXPORT_METHOD type name
#define CORECON_PROC(type,name) EXPORT_METHOD type CPROC name
// for defining variables.
#define CORECON_EXPORT(type,name) EXPORT_METHOD type name
#else
#define CORECON_NPROC(type,name) IMPORT_METHOD type name
#define CORECON_PROC(type,name) IMPORT_METHOD type CPROC name
// for defining variables.
#define CORECON_EXPORT(type,name) IMPORT_METHOD type name
#endif
#else
#ifdef CORECON_SOURCE
#define CORECON_PROC(type,name) type CPROC name
#define CORECON_NPROC(type,name) type name
#define CORECON_EXPORT(type,name) type name
#else
#define CORECON_PROC(type,name) extern type CPROC name
#define CORECON_NPROC(type,name) extern type name
#define CORECON_EXPORT(type,name) extern type name
#endif
#endif
#endif

#include <stdhdrs.h>
#include <image.h>
#include <render.h>
#include <controls.h>

#include <psi/console.h>


#ifdef DEKWARE_PLUGIN
#define PLUGIN_MODULE
#include "plugin.h"

#include "datapath.h"
#include "space.h"
#endif

#include "history.h"  // history_track

PSI_CONSOLE_NAMESPACE

#if !defined( WIN32 ) && !defined( _WIN32 )
typedef struct rect_tag {
   S_32 top,left,right,bottom;
} RECT;
#endif
//----------------------------------------------------------------------------
// unused STILL - but one day status bars on output may be useful!

typedef struct statfield_tag {
   PTEXT *pLine; // line to pass to macroduplicate to get the actual string
   int nLength; // length of this field - text will not exceed this

	struct statfield_tag **me;
	struct statfield_tag *pNext; // next to the right or next to the left...
} STATFIELD, *PSTATFIELD;

typedef struct statbar_tag {
   PSTATFIELD *pLeft, *pRight;
} STATBAR, *PSTATBAR;


//----------------------------------------------------------------------------

typedef struct keybind_tag { // overrides to default definitions
   struct {
        int bMacro:1;
        int bFunction:1;
      int bStroke:1;
   } flags;
	union {
      PTEXT stroke;
   } data;
} KEYBIND, *PKEYBIND;

//----------------------------------------------------------------------------
// virtual buffer?
// video buffer?
//typedef struct vbuffer_tag {
	//int nDisplayLines;
	// nLines>0
	// ? nLines - nDisplayLines == nHistoryLines
	// : nHistoryLines = nLines
	//int nHistoryPercent; // 0 = 25, 1 = 50, 2 = 75, 3 = 100
	//int nCursorX; // current offset on current line
	//int nCursorY; // line offset from last line ... ( -1-> - lines)
   //int tabsize;   // multiple size of tabs....
//} VBUFFER, *PVBUFFER;

enum current_color_type
{
	COLOR_COMMAND
, COLOR_MARK
, COLOR_DEFAULT
, COLOR_SEGMENT
};

enum fill_color_type
{
	FILL_COMMAND_BACK
, FILL_DISPLAY_BACK
};

//----------------------------------------------------------------------------

typedef struct myconsolestruc {
   // these would otherwise exist within the common datapath structure...
   PUSER_INPUT_BUFFER CommandInfo;

	// physical width and height, (1:1 in console modes)
	_32 nWidth;   // in pixels
	_32 nColumns; // in character count width
	_32 nHeight;  // in pixels
	_32 nLines;   // in character count rows

	CRITICALSECTION Lock;

	struct {
		BIT_FIELD bDirect:1; // alternative to direct is Line-Mode
		BIT_FIELD bLastEnqueCommand:1; // set if the last thing output was the command.
		BIT_FIELD bUpdatingEnd : 1;
		BIT_FIELD bMarking : 1;
		BIT_FIELD bMarkingBlock : 1;
		BIT_FIELD bNoDisplay : 1;
		BIT_FIELD bNoHistoryRender : 1;
		BIT_FIELD bForceNewline : 1;
		// character mode input, instead of line buffered
		// just because it's in direct mode doesn't mean it
		// has to be direct send also...  But CharMode is only
		// available if mode is also direct.
		BIT_FIELD bCharMode : 1;
		BIT_FIELD bNoLocalEcho : 1;
		BIT_FIELD bHistoryShow : 1;
		BIT_FIELD bNewLine : 1; // set if the next line is NEW else it's to be appended.
		BIT_FIELD bBuildDataWithCarriageReturn : 1;
	} flags;

	_32 pending_spaces;
   _32 pending_tabs;

	RECT rArea; // pixel size of the display (if font height/width>1)
	_32 nFontHeight;
	_32 nFontWidth;
	S_32 nXPad; // pixels/lines to padd left/right side...
	S_32 nYPad; // pixels/lines to padd top/bottom side...
	S_32 nCmdLinePad; // pixels to raise bar above cmdline

	//PHISTORY_BIOS pHistory;
	int nHistoryPercent;
	// these mark the bottom line, from these UP
	// are the regions... therefore if start = 0
	// the first line to show is above the display and
	// therefore that region has no information to show.
	int nHistoryLineStart;
	int nDisplayLineStart; // top visual line of those in 'display' (start of separator)
	int nCommandLineStart; // marks the top of the separator line... bottom of text

	// these are within the history cursor...
	// and this is the reason I need another cursor for
	// history and for display...
	// perhaps I could just always have two versions of browsing
	// browse_end, browse_current ?
	PHISTORY_REGION pHistory;
	// region history is browsed here,
	// this cursor is controlled for the top/bottom of form
	// control
	PHISTORY_BROWSER pHistoryDisplay;
	// output is performed here.
	// view is always built from tail backward.(?)
	PHISTORY_BROWSER pCurrentDisplay;
	// history, but on the outbound side?
	//
	// cursor as output has a cursor position
	// and a seperate cursor position for browsing...
	// I should probably seperate the data, but they
	// share the same width/height...
	PHISTORY_LINE_CURSOR pCursor;
   
	PDATALIST *CurrentLineInfo;

	KEYBIND Keyboard[256][8];
	// is actually current keymod state.
	_32 dwControlKeyState;

	int mark_location;
	// 0 = command, 1 = display, 2 = history
	//, 3 = header?, 4 = footer?
	// display list this is a mark in.
	PDATALIST *CurrentMarkInfo;
	struct {
		INDEX row, col;
	} mark_start;
	struct {
		INDEX row, col;
	} mark_end;
	// something like a specialized footer here... probably a
	// PSTATBAR MenuBar; might fit in well also...
	//PSTATBAR StatusBar; // if present - then all lines must recompute!
	// ----------------
	// Rendering Methods for console_core library external methods...

	void (CPROC *FillConsoleRect)( struct myconsolestruc *pmdp, RECT *r, enum fill_color_type );
	void (CPROC *DrawString)( struct myconsolestruc *pmdp, int x, int y, RECT *r, CTEXTSTR s, int nShown, int nShow );
	void (CPROC *SetCurrentColor )( struct myconsolestruc *pmdp, enum current_color_type, PTEXT segment );
	void (CPROC *RenderSeparator )( struct myconsolestruc *pmdp, int pos );
	void (CPROC *KeystrokePaste )( struct myconsolestruc *pmdp );
	void (CPROC *RenderCursor )( struct myconsolestruc *pmdp, RECT *r, int column );
	void (CPROC *Update )( struct myconsolestruc *pmdp, RECT *upd );
   // void CPROC
   PLIST data_processors;
	union {
		// this is what this union has if nothing else defined
		// winlogic should need no member herein....
		_32 dwInterfaceData[32];

		struct
		{
			PRENDERER renderer;
			PCOMMON frame;
			SFTFont hFont;
			Image image;
			CDATA  crCommand;
			CDATA  crCommandBackground;
			CDATA  crBackground;
			CDATA  crMark;
			CDATA  crMarkBackground;
			// current working parameters...
			CDATA crText;
         CDATA crBack;
		} psicon;
	};

} CONSOLE_INFO, *PCONSOLE_INFO;

#include "keydefs.h"

PSI_CONSOLE_NAMESPACE_END

#endif
// $Log: consolestruc.h,v $
// Revision 1.43  2005/02/23 11:38:59  d3x0r
// Modifications/improvements to get MSVC to build.
//
// Revision 1.42  2005/01/27 17:31:37  d3x0r
// psicon works now, added some locks to protect multiple accesses to datapath (render/update_write)
//
// Revision 1.41  2005/01/23 04:07:57  d3x0r
// Hmm somehow between display rendering stopped working.
//
// Revision 1.40  2005/01/20 20:04:10  d3x0r
// Wincon nearly functions, consolecon ported to new coreocn lib, psicon is next.
//
// Revision 1.39  2005/01/20 06:10:19  d3x0r
// One down, 3 to convert... concore library should serve to encapsulate drawing logic and history code...
//
// Revision 1.38  2005/01/19 15:49:33  d3x0r
// Begin defining external methods so WinLogic and History can become a core-console library
//
// Revision 1.37  2004/10/11 08:45:34  d3x0r
// checkpoint.
//
// Revision 1.36  2004/09/20 10:00:10  d3x0r
// Okay line up, wrapped, partial line, line up, history alignment, all sorts of things work very well now.
//
// Revision 1.35  2004/09/10 08:48:50  d3x0r
// progress not perfection... a grand dawning on the horizon though.
//
// Revision 1.34  2004/09/09 13:41:04  d3x0r
// works much better passing correct structures...
//
// Revision 1.33  2004/07/30 14:08:40  d3x0r
// More tinkering...
//
// Revision 1.32  2004/06/12 08:42:34  d3x0r
// Well it initializes, first character causes failure... all windows targets build...
//
// Revision 1.31  2004/06/12 06:21:51  d3x0r
// One link error from compilation - now to revisit this and weed it some...
//
// Revision 1.30  2004/06/12 01:24:10  d3x0r
// History cursors are being seperated into browse types and output types...
//
// Revision 1.29  2004/06/10 10:01:45  d3x0r
// Okay so cursors have input and output characteristics... history and display are run by seperate cursors.
//
// Revision 1.28  2004/06/10 00:41:49  d3x0r
// Progress..
//
// Revision 1.27  2004/06/07 15:19:54  d3x0r
// Done with history up to key-associated path (pHistoryShow)
//
// Revision 1.26  2004/05/14 18:35:40  d3x0r
// Checkpoint
//
// Revision 1.25  2004/05/14 16:58:36  d3x0r
// More mangling of structures...
//
// Revision 1.24  2004/05/13 23:35:07  d3x0r
// checkpoint
//
// Revision 1.23  2004/05/13 23:18:17  d3x0r
// Seperating of history and redinrg code...
//
// Revision 1.22  2004/05/04 07:15:01  d3x0r
// Fix hinstance, implement relay segments out, clean, straighten and shape up
//
// Revision 1.21  2004/03/24 19:24:55  d3x0r
// Progress on fixing console con...
//
// Revision 1.20  2004/03/08 09:25:42  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.19  2004/03/05 21:04:32  d3x0r
// Well console nearly works!
//
// Revision 1.18  2004/01/21 08:45:22  d3x0r
// Okay final edit then we fix cursecon
//
// Revision 1.17  2004/01/21 08:43:47  d3x0r
// Fix types of mark/background
//
// Revision 1.16  2004/01/21 08:42:30  d3x0r
// Fix crmark/background
//
// Revision 1.15  2004/01/21 08:41:14  d3x0r
// Fix consolecon def in makefile
//
// Revision 1.14  2004/01/21 08:32:08  d3x0r
// Massive mods - working on unifying rendercommandline
//
// Revision 1.13  2004/01/21 06:45:23  d3x0r
// Compiles okay - windows.  Test point
//
// Revision 1.12  2004/01/20 07:31:33  d3x0r
// Added include keybrd to satisfy some warnings
//
// Revision 1.11  2004/01/19 23:42:26  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.17  2004/01/19 00:10:37  panther
// Cursecon compat edits
//
// Revision 1.16  2004/01/19 00:05:18  panther
// Cursecon compat edits
//
// Revision 1.15  2004/01/19 00:02:06  panther
// Cursecon compat edits
//
// Revision 1.14  2004/01/18 22:56:41  panther
// Cursecon compat edits
//
// Revision 1.13  2004/01/18 22:54:39  panther
// Cursecon compat edits
//
// Revision 1.12  2004/01/18 22:28:23  panther
// Fix header include
//
// Revision 1.11  2004/01/18 22:02:31  panther
// Merge wincon/psicon/cursecon
//
// Revision 1.10  2003/04/20 16:36:57  panther
// Use KEY_ instead of VK_.  Use _32 not DWORD
//
// Revision 1.9  2003/04/20 16:19:53  panther
// Fixes for history usage... cleaned some logging messages.
//
// Revision 1.8  2003/04/17 06:40:33  panther
// More merging of wincon/psicon.  Should nearly have the renderer totally seperate now from the logic
//
// Revision 1.7  2003/04/13 22:28:51  panther
// Seperate computation of lines and rendering... mark/copy works (wincon)
//
// Revision 1.6  2003/04/12 20:51:19  panther
// Updates for new window handling module - macro usage easer...
//
// Revision 1.5  2003/03/25 08:59:04  panther
// Added CVS logging
//
