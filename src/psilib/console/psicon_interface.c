
#include <psi.h>
#include <psi/console.h>

#include "consolestruc.h"
#include "WinLogic.h"

PSI_CONSOLE_NAMESPACE

	extern CONTROL_REGISTRATION ConsoleClass;
static PTEXT eol;

int PSIConsoleOutput( PSI_CONTROL pc, PTEXT lines )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	// ansi filter?
	// conditions for getting text lines which have format elements
	// break lines?
	if( !eol )
		eol = SegCreateFromText( WIDE("\n") );
	if( console )
	{
		PTEXT parsed;
		PTEXT next;
		PTEXT remainder = NULL;
		PTEXT tmp;

		parsed = burst( lines );
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
			PSI_WinLogicWriteEx( console, que, 0 );
			console->flags.bNewLine = 0;
		}
		else
		{
			console->flags.bNewLine = 1;
		}
		SmudgeCommon( pc );
	}
	return 0;
}

PSI_Phrase PSIConsolePhraseOutput( PSI_CONTROL pc, PTEXT lines )
{

}

void PSIConsoleInputEvent( PSI_CONTROL pc, void(CPROC*Event)(PTRSZVAL,PTEXT), PTRSZVAL psv )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		// this should be set with an appropriate method.
		console->CommandInfo->CollectedEvent = Event;
		console->CommandInfo->psvCollectedEvent = psv;
	}
}

void PSIConsoleLoadFile( PSI_CONTROL pc, CTEXTSTR file )
{
	// reset history and read a file into history buffer
	// this also implies setting the cursor position at the start of the history buffer

}

int vpcprintf( PSI_CONTROL pc, CTEXTSTR format, va_list args )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT output;
		vvtprintf( pvt, format, args );
		output = VarTextGet( pvt );
		PSIConsoleOutput( pc, output );
	}
	return 1;
}

int pcprintf( PSI_CONTROL pc, CTEXTSTR format, ... )
{
	va_list args;
	va_start( args, format );
	return vpcprintf( pc, format, args );
}

void PSIConsoleSetLocalEcho( PSI_CONTROL pc, LOGICAL yesno )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		console->flags.bNoLocalEcho = !yesno;
	}
}

struct history_tracking_info *PSIConsoleSaveHistory( PSI_CONTORL pc )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		struct history_tracking_info *history_info = New( struct history_tracking_info );
		history_info->pHistory = console->pHistory;
		history_info->pHistoryDisplay = console->pHistoryDisplay;
		history_info->pCurrentDisplay = console->pCurrentDisplay;
		history_info->pCursor = console->pCursor;
      history_info->pending_spaces = console->pending_spaces;
		history_info->pending_tabs = console->pending_tabs;
      return history_info;
	}
   return NULL;
}

void PSIConsoleSetHistory( PSI_CONTORL pc, struct history_tracking_info *history_info )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		if( history_info )
		{
			console->pHistory = history_info->pHistory;
			console->pHistoryDisplay = history_info->pHistoryDisplay;
			console->pCurrentDisplay = history_info->pCurrentDisplay;
			console->pCursor = history_info->pCursor;
			console->pending_spaces = history_info->pending_spaces;
			console->pending_tabs = history_info->pending_tabs;
		}
		else
		{
			console->pHistory = PSI_CreateHistoryRegion();
			console->pCursor = PSI_CreateHistoryCursor( console->pHistory );
			console->pCurrentDisplay = PSI_CreateHistoryBrowser( console->pHistory );
			console->pHistoryDisplay = PSI_CreateHistoryBrowser( console->pHistory );
         console->pending_spaces = 0;
         console->pending_tabs = 0;
			PSI_SetHistoryBrowserNoPageBreak( console->pHistoryDisplay );

		}
	}
}

PSI_CONSOLE_NAMESPACE_END
