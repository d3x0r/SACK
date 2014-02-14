
#include <controls.h>
#include <psi/console.h>
#include "consolestruc.h"
#include "histstruct.h"
#include "history.h"
#include "WinLogic.h"

PSI_CONSOLE_NAMESPACE



void FormatTextToBlock( CTEXTSTR input, TEXTSTR *output, int char_width, int char_height )
{
	PCONSOLE_INFO console;
	TEXTSTR block;

	console = New( CONSOLE_INFO );
	MemSet( console, 0, sizeof( CONSOLE_INFO ) );
	{
   		//console->common.pName = SegCreateFromText( "Auto Console" );
		//Log( "Create frame!!" );
		console->psicon.frame = NULL;

		console->psicon.image = GetFrameSurface( console->psicon.frame );
		console->psicon.hFont = NULL;
		console->nFontWidth = 1;
		console->nFontHeight = 1;

		InitializeCriticalSec( &console->Lock );

		console->rArea.left = 0;
		console->rArea.right = char_width;
		console->rArea.top = 0;
		console->rArea.bottom = char_height;
		// this is destroyed when the common closes...
		console->CommandInfo = NULL;  //CreateUserInputBuffer();

		//console->common.Type = myTypeID;
		console->flags.bDirect = 1;
		//console->common.flags.Formatted = TRUE;

		console->pHistory = PSI_CreateHistoryRegion();
		console->pCursor = PSI_CreateHistoryCursor( console->pHistory );
		console->pCurrentDisplay = PSI_CreateHistoryBrowser( console->pHistory );
		console->pHistoryDisplay = PSI_CreateHistoryBrowser( console->pHistory );
		PSI_SetHistoryBrowserNoPageBreak( console->pHistoryDisplay );

		console->nXPad = 0;
		console->nYPad = 0;
		console->nCmdLinePad = 0;

		PSI_ConsoleCalculate( console );
	}

	console->nLines = char_height;
	console->nColumns = char_width;

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
	console->mark_start.row = (char_height - 1);
	console->mark_start.col = 0;
	console->mark_end.row = 0;
	console->mark_end.col = char_width - 1;


	block = PSI_GetDataFromBlock( console );
	(*output) = block;

	PSI_DestroyHistoryBrowser( console->pCurrentDisplay );
	PSI_DestroyHistoryBrowser( console->pHistoryDisplay );
	PSI_DestroyHistoryCursor( console->pCursor );
	PSI_DestroyHistoryRegion( console->pHistory );

	Release( console );
}

PSI_CONSOLE_NAMESPACE_END


