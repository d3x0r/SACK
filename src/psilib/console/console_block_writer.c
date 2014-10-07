#include "../global.h"

#include <controls.h>
#include <psi/console.h>
#include "consolestruc.h"
#include "histstruct.h"
#include "history.h"
#include "WinLogic.h"

PSI_CONSOLE_NAMESPACE

struct BlockFormatter
{
	PCONSOLE_INFO console;
   SFTFont font;
};


static void CPROC AsciiMeasureString( PTRSZVAL psvConsole, CTEXTSTR s, int nShow, _32 *w, _32 *h )
{
	struct BlockFormatter *block = (struct BlockFormatter*)psvConsole;
#ifndef __NO_GUI__
	if( block->font )
	{
		GetStringSizeFontEx( s, nShow, w, h, block->font );
	}
	else
#endif
	{
		(*w) = nShow;
		(*h) = 1;
	}
}


void FormatTextToBlockEx( CTEXTSTR input, TEXTSTR *output, int* pixel_width, int* pixel_height, SFTFont font )
{
	struct BlockFormatter *block_data = New( struct BlockFormatter );
	PCONSOLE_INFO console;
	TEXTSTR block;
	GetMyInterface();
	block_data->console
		= console = New( CONSOLE_INFO );
	block_data->font = font;
	MemSet( console, 0, sizeof( CONSOLE_INFO ) );
	{
		//console->common.pName = SegCreateFromText( "Auto Console" );
		//Log( "Create frame!!" );
		console->psicon.frame = NULL;

		console->psicon.image = GetFrameSurface( console->psicon.frame );
		console->nFontWidth = 1;
		console->nFontHeight = 1;

		InitializeCriticalSec( &console->Lock );

		console->rArea.left = 0;
		console->rArea.right = (*pixel_width);
		console->rArea.top = 0;
		console->rArea.bottom = (*pixel_height);
		// this is destroyed when the common closes...
		console->CommandInfo = NULL;  //CreateUserInputBuffer();

		//console->common.Type = myTypeID;
		console->flags.bDirect = 1;
		//console->common.flags.Formatted = TRUE;

		console->pHistory = PSI_CreateHistoryRegion();
		console->pCursor = PSI_CreateHistoryCursor( console->pHistory );
		console->pCurrentDisplay = PSI_CreateHistoryBrowser( console->pHistory, AsciiMeasureString, (PTRSZVAL)block_data );

		console->nXPad = 0;
		console->nYPad = 0;
		console->nCmdLinePad = 0;

		PSI_ConsoleCalculate( console );
	}

	//console->nLines = char_height;
	//console->nColumns = char_width;

	{
		// ansi filter?
		// conditions for getting text lines which have format elements
		// break lines?
		PTEXT parsed;
		PTEXT next;
		PTEXT remainder = NULL;
		PTEXT tmp;
		PTEXT lines = SegCreateFromText( input );

		parsed = burst( lines );

		LineRelease( lines );

		remainder = parsed;

		for( tmp = parsed; tmp; tmp = next )
		{
			next = NEXTLINE( tmp );
			if( !GetTextSize( tmp ) )
			{
				PTEXT prior;
				PTEXT que;
				prior = SegBreak( tmp );
				if( prior )
				{
					SetStart( prior );
					prior->format.position.offset.spaces += (_16)console->pending_spaces;
					prior->format.position.offset.tabs += (_16)console->pending_tabs;
					que = BuildLine( prior );
					if( !console->flags.bNewLine )
						que->flags |= TF_NORETURN;
					PSI_WinLogicWriteEx( console, que, 0 );
					LineRelease( prior );
				}
				else
					PSI_WinLogicWriteEx( console, SegCreate( 0 ), 0 );

				console->flags.bNewLine = 1;

				// throw away the blank... don't really need it on the display
				SegGrab( tmp );
				console->pending_spaces = tmp->format.position.offset.spaces;
				console->pending_tabs = tmp->format.position.offset.tabs;
				LineRelease( tmp );
				remainder = next;
			}
		}
		if( remainder )
		{
			PTEXT que = BuildLine( remainder );
			LineRelease( remainder );
			PSI_WinLogicWriteEx( console, que, 0 );
 			console->flags.bNewLine = 0;
		}
		else
		{
			console->flags.bNewLine = 1;
		}
	}

	// make sure we think we're showing the top 5 lines, not top 0

	//console->pCurrentDisplay->nOffset = char_height - 1;
	BuildDisplayInfoLines( console->pCurrentDisplay );

	console->CurrentLineInfo =
		console->CurrentMarkInfo = &console->pCurrentDisplay->DisplayLineInfo;

	if( font )
	{
      int len;
		int maxlen = 0;
		int lines = 1;
		PDISPLAYED_LINE pdl;
		for( lines = 0; pdl = (PDISPLAYED_LINE)GetDataItem( console->CurrentMarkInfo, lines ) ; lines++ )
		{
			len = pdl->nToShow;
			if( pdl->nToShow > maxlen )
				maxlen = len;
			//lprintf( "line %d len %d", lines, len );
			if( len == 0 )
            break;
		}
		//lprintf( "measured block in characters %d,%d", maxlen, lines );
		console->mark_start.row = lines;
		console->mark_start.col = 0;
		console->mark_end.row = 0;
		console->mark_end.col = maxlen + 1;
	}
	else
	{
		console->mark_start.row = ((*pixel_height) - 1);
		console->mark_start.col = 0;
		console->mark_end.row = 0;
		console->mark_end.col = (*pixel_width) - 1;
	}

	block = PSI_GetDataFromBlock( console );
	if( *output )
	{
		//lprintf( "Release %p", (*output ) );
		//DebugBreak();
		Deallocate( TEXTCHAR *, (*output) );
	}
	(*output) = block;
	if( font )
	{
		_32 width, height;
		GetStringSizeFont( (*output), &width, &height, font );
		(*pixel_width) = width;
		(*pixel_height) = height;
	}
	PSI_DestroyHistoryBrowser( console->pCurrentDisplay );
	PSI_DestroyHistoryCursor( console->pCursor );
	PSI_DestroyHistoryRegion( console->pHistory );

	Release( console );
}

void FormatTextToBlock( CTEXTSTR input, TEXTSTR *output, int char_width, int char_height )
{
   FormatTextToBlockEx( input, output, &char_width, &char_height, NULL );

}

PSI_CONSOLE_NAMESPACE_END


