
#ifndef HISTORY_STRUCTURES_DEFINED
#define HISTORY_STRUCTURES_DEFINED

#include "consolestruc.h"
PSI_CONSOLE_NAMESPACE

struct history_line_tag {
	struct history_line_flags_tag {
		BIT_FIELD nLineLength : 16;
		BIT_FIELD deleted : 1;
		//BIT_FIELD updated : 1;
	} flags;
	PTEXT pLine;
};

#if 0
typedef struct STRUC_PREFIX( history_line_tag ) TEXTLINE, *PTEXTLINE;
#define MAXTEXTLINESPERSET 256
DeclareSet( TEXTLINE );
#endif

struct history_block_link_tag {
	struct history_block_tag *next;
	union {
		struct history_block_tag **me;
		struct history_block_tag *prior;
	};
};

struct history_block_tag {
	// (me) == coincidentally the address of prior
	// block if we define *next as the first element...
	struct {
		struct history_block_tag *next;
		union {
			struct history_block_tag **me;
			struct history_block_tag *prior;
		};
	};

	INDEX nLinesUsed;
	// INDEX nLinesAvail; // = MAX_HISTORY_LINES
#define MAX_HISTORY_LINES 250
	struct history_line_tag pLines[MAX_HISTORY_LINES];
};


struct history_region_tag {
	struct {
		// if next segment is NO_RETURN, force a return
		// if segment is a newline, use that, clear this anyhow.
		uint32_t bForceNewline: 1; // set after entering a command 
		uint32_t bInsert : 1; // when adding text at a cursor, otherwise overwrite
		uint32_t bUpdated : 1;
	} flags;
	int nMaxHistoryBlocks;
	int nHistoryBlocks;
	int nHistoryBlockSize; // lines per block
	int nLines;
	int32_t tabsize;

	union {
		struct history_block_link_tag root;
		struct {
			PHISTORYBLOCK next;
			union {
				PHISTORYBLOCK *me;
				PHISTORYBLOCK last;
			};
		};
	} pHistory;
	// list of cursors on this block
	PLIST pCursors;
	PLIST pBrowsers;
	FILE *file_backing;
};

//----------------------------------------------------------------------------

struct displayed_line_info_tag
{
	 INDEX nLine; // history line index...
	 PTEXT start; // actual segment here
	 INDEX nFirstSegOfs;	 // offset into start which begins this line.
	 INDEX nToShow; // length of data we intend to show...
	 INDEX nPixelEnd; // how long measure resulted this should be at...
	 INDEX nLineStart; // absolute character that is the start line character index
	 INDEX nLineEnd; // absolute character that is the last line character index
	 INDEX nLineHeight;  // how high this line is...
	 INDEX nLineTop; // top coordiante of this line...
};

//----------------------------------------------------------------------------

struct history_browser_cursor_tag {
	PHISTORY_REGION region;
	//struct history_line_cursor_tag *phlc; // display width/height characteristics...
	// cursor position...
	// -1 = lastline, -2 next to last line..
	// pBlock->nLinesUsed + nCursorY = nLine;
	struct {
		uint32_t bWrapText : 1;
		uint32_t bNoPrompt : 1;
		uint32_t bNoPageBreak : 1; // don't stop at page_breaks...
		uint32_t bOwnPageBreak : 1; // dont' stop, but also return the page break segments... I want to render them.
		uint32_t bSetLineCounts : 1;
		uint32_t bUpdated : 1; // history region content has been updated.
	} flags;

	// visible region...
	//INDEX nLines;
	INDEX nHeight;
	INDEX nPageLines; // number of lines to scroll up/down for 1 page.
	INDEX nFirstLines; // set as page lines... and cleared after first move.
	INDEX nColumns; // rough character count/ depricated....

	INDEX nLineHeight;  // height of lines...
	INDEX nWidth; // this is pixel size (using measure string)

	INDEX nLineStart;  // first position of line to start at...
	uint32_t nFirstLine;

	PDATALIST DisplayLineInfo;
	// current line of historyc block;
	// if nLine > 0, then the current block
	// is the base of stuff...
	int32_t nLine;
	/// this is more of a reference for the current output block;
	// it points to the last block (or the block recieving lines if seeked)
	PHISTORYBLOCK pBlock;

	// character offset within current segment
	int nOffset;
	CRITICALSECTION cs;

	MeasureString measureString;
	uintptr_t psvMeasure;
};

struct history_line_cursor_tag {
	PHISTORY_REGION region;
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
	PREFIX_PACKED struct {
		uint16_t nWidth;
		uint16_t nHeight;
	} PACKED;
#ifdef _MSC_VER
#pragma pack (pop)
#endif
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
	PREFIX_PACKED struct {
		struct {
			PHISTORYBLOCK block;
			int32_t line;
			// may consider storing these for historical usage...
			// at the moment though, one current top for clear_page
			// works well.
		} top_of_form;
		// cursor position...
		// -1 = lastline, -2 next to last line..
		// pBlock->nLinesUsed + nCursorY = nLine;
		int16_t nCursorY;
		int16_t nCursorX;

		//PHISTORYBLOCK pBlock;
		// current line of historyc block;
		uint32_t nLine;

		// current line segment pointer, within which offset is.
		PTEXTLINE pLine;

		// the current segment
		PTEXT pSegment;

		// character offset within current segment
		int nOffset;

		// cursor paints prior color if no attributes
		// are specified on the text tag.
		FORMAT PriorColor;
		// when reset, the cursor gains this color.
		FORMAT DefaultColor;
	} output;
#ifdef _MSC_VER
#pragma pack (pop)
#endif
} ;

//----------------------------------------------------------------------------

struct history_tracking_tag {
	int nRegions;
	PHISTORY_REGION pRegion;
	struct myconsolestruc *pdp;
} ;

struct history_bios_tag
// basic io system... takes all the above and
// brings together everything needed for
// a commandline, display, and fractional history
// anything more complex than this is going to
// have to be constructed...
{
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
};

struct PSI_console_word
{
	PTEXTLINE line;
	PTEXT segment;
	int cursor_position_added_at;
};


struct PSI_console_phrase
{
	PLIST words;  // list of the words which may span multiple lines
	uintptr_t data;
};

struct PSI_console_feedback
{
	uintptr_t psv_user_data;
	PSI_Console_FeedbackClick feedback_handler;
};



PSI_CONSOLE_NAMESPACE_END

#endif
