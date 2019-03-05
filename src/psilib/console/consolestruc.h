#ifndef MY_DATAPATH_DEFINED
#define MY_DATAPATH_DEFINED

#ifndef CORECON_SOURCE
#if defined( SACK_BAG_EXPORTS ) || defined( PSI_CONSOLE_SOURCE )
#define CORECON_SOURCE
#endif
#endif

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

#include <stdhdrs.h>
#include <image.h>
#include <render.h>
#include <controls.h>

#include <psi/console.h>

//#define DEBUG_OUTPUT

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
	int32_t top,left,right,bottom;
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
	//int tabsize;	// multiple size of tabs....
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

struct history_tracking_info
{
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

	uint32_t pending_spaces;
	uint32_t pending_tabs;
	
};

//----------------------------------------------------------------------------

typedef struct myconsolestruc {
	// these would otherwise exist within the common datapath structure...
	PUSER_INPUT_BUFFER CommandInfo;

	// physical width and height, (1:1 in console modes)
	uint32_t nWidth;	// in pixels
	uint32_t nColumns; // in character count width
	uint32_t nHeight;  // in pixels
	uint32_t nLines;	// in character count rows

	CRITICALSECTION Lock;
	int lockCount;

	void(CPROC*InputEvent)(uintptr_t, PTEXT);
	uintptr_t psvInputEvent;

	struct {
		BIT_FIELD bDirect:1; // alternative to direct is Line-Mode
		BIT_FIELD bWrapCommand : 1; // normal mode is ot not wrap, but to scroll off...
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

	// these are working parameters during output...
	uint32_t pending_spaces;
	uint32_t pending_tabs;

	RECT rArea; // pixel size of the display (if font height/width>1)
	//uint32_t nFontHeight;
	//uint32_t nFontWidth;
	int32_t nXPad; // pixels/lines to padd left/right side...
	int32_t nYPad; // pixels/lines to padd top/bottom side...
	int32_t nCmdLinePad; // pixels to raise bar above cmdline (wider for separated command line)

	//PHISTORY_BIOS pHistory;
	int nHistoryPercent;
	// these mark the bottom line, from these UP
	// are the regions... therefore if start = 0
	// the first line to show is above the display and
	// therefore that region has no information to show.
	int nCommandLineStart; // bottom-most line of the display
	int nHistoryLineStart;  // upper text area
	int nDisplayLineStartDynamic; // top visual line of those in 'display' (start of separator)
	int nNextCharacterBegin; // this is computed from the last position of the last renderd line ( continue for command line)
	int nSeparatorHeight; // how wide the separation line is. (previously had some arbitrary constants)
	uint32_t nFontHeight;

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

	PHISTORY_REGION pCommandHistory; // the data for pCommandDisplay to use... overlapping the input history had issues.
	// a display browser for formatting the command input line
	// in wrapped mode.
	PHISTORY_BROWSER pCommandDisplay;
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
	uint32_t dwControlKeyState;

	int mark_location;
	// 0 = command, 1 = display, 2 = history
	//, 3 = header?, 4 = footer?
	// display list this is a mark in.
	PDATALIST *CurrentMarkInfo;
	struct {
		int row, col;
	} mark_start;
	struct {
		int row, col;
	} mark_end;
	// something like a specialized footer here... probably a
	// PSTATBAR MenuBar; might fit in well also...
	//PSTATBAR StatusBar; // if present - then all lines must recompute!
	// ----------------
	// Rendering Methods for console_core library external methods...

	void (CPROC *FillConsoleRect)( struct myconsolestruc *pmdp, RECT *r, enum fill_color_type );
	void (CPROC *DrawString)( struct myconsolestruc *pmdp, int x, int y, RECT *r, CTEXTSTR s, int nShown, int nShow );
	void (CPROC *SetCurrentColor )( struct myconsolestruc *pmdp, enum current_color_type, PTEXT segment );
	int (CPROC *RenderSeparator )( struct myconsolestruc *pmdp, int pos ); // allow width to be returned.
	void (CPROC *KeystrokePaste )( struct myconsolestruc *pmdp );

	void (CPROC *RenderCursor )( struct myconsolestruc *pmdp, RECT *r, int column );
	void (CPROC *Update )( struct myconsolestruc *pmdp, RECT *upd );
	// void CPROC
	PLIST data_processors;
	union {
		// this is what this union has if nothing else defined
		// winlogic should need no member herein....
		uint32_t dwInterfaceData[32];

		struct
		{
			PRENDERER renderer;
			PSI_CONTROL frame;
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
