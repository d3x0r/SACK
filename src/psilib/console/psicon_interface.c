
#define USE_IMAGE_INTERFACE ImageInterface
#define USE_RENDER_INTERFACE RenderInterface

#include <stdhdrs.h>
#include <psi.h>
#include <psi/console.h>

#include "consolestruc.h"
#include "WinLogic.h"

extern PIMAGE_INTERFACE ImageInterface;

PSI_CONSOLE_NAMESPACE

	extern CONTROL_REGISTRATION ConsoleClass;
static PTEXT eol;

PSI_Console_Phrase PSIConsoleOutput( PSI_CONTROL pc, PTEXT lines )
{
	PSI_Console_Phrase phrase;
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

		remainder = parsed = burst( lines );
		for( tmp = remainder; tmp; tmp = next )
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
					prior->format.position.offset.spaces += (uint16_t)console->pending_spaces;
					prior->format.position.offset.tabs += (uint16_t)console->pending_tabs;
					que = BuildLine( prior );
					if( console->flags.bNewLine )
						que->flags |= TF_NORETURN;
					phrase  = PSI_WinLogicWriteEx( console, que, 0 );
					LineRelease( prior );
				}
				else
					phrase  = PSI_WinLogicWriteEx( console, SegCreate( 0 ), 0 );

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
			phrase = PSI_WinLogicWriteEx( console, que, 0 );
			console->flags.bNewLine = 0;
		}
		else
		{
			console->flags.bNewLine = 1;
		}
		SmudgeCommon( pc );
	}
	return phrase;
}

PSI_Console_Phrase PSIConsoleDirectOutput( PSI_CONTROL pc, PTEXT lines )
{
	PSI_Console_Phrase phrase;
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	// ansi filter?
	// conditions for getting text lines which have format elements
	// break lines?
	phrase  = PSI_WinLogicWriteEx( console, lines, 0 );

	SmudgeCommon( pc );

	return phrase;
}

static void sendInputEvent( uintptr_t arg, PTEXT line ) {
	PCONSOLE_INFO console = (PCONSOLE_INFO)arg;
	int n;
	for( n = 0; n < console->lockCount; n++ )
		LeaveCriticalSec( &console->Lock );
	console->InputEvent( console->psvInputEvent, line );
	for( n = 0; n < console->lockCount; n++ )
		EnterCriticalSec( &console->Lock );
}

void PSIConsoleInputEvent( PSI_CONTROL pc, void(CPROC*Event)(uintptr_t,PTEXT), uintptr_t psv )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		// this should be set with an appropriate method.
		console->InputEvent = Event;
		console->psvInputEvent = psv;
		console->CommandInfo->CollectedEvent = sendInputEvent;
		console->CommandInfo->psvCollectedEvent = (uintptr_t)console;
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

struct history_tracking_info *PSIConsoleSaveHistory( PSI_CONTROL pc )
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

void PSIConsoleSetHistory( PSI_CONTROL pc, struct history_tracking_info *history_info )
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
			console->pCurrentDisplay = PSI_CreateHistoryBrowser( console->pHistory, PSIMeasureString, (uintptr_t)console );
			console->pHistoryDisplay = PSI_CreateHistoryBrowser( console->pHistory, PSIMeasureString, (uintptr_t)console );
			console->pending_spaces = 0;
			console->pending_tabs = 0;
			PSI_SetHistoryBrowserNoPageBreak( console->pHistoryDisplay );

		}
		//GetStringSizeFont( WIDE(" "), &console->nFontWidth, &console->nFontHeight, GetCommonFont( pc ) );
		PSI_ConsoleCalculate( console, GetCommonFont( pc ) );
	}
}

// mode 0 = inline/scrolling
// mode 1 = line buffer/scrolling
// mode 2 = line buffer/wrap
void PSIConsoleSetInputMode( PSI_CONTROL pc, int mode )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console )
	{
		if( mode )
		{
			console->flags.bDirect = 0; // direct is inline, instead of line-mode
			if( mode == 2 )
			{
				SetBrowserHeight( console->pCommandDisplay, 3 * console->nFontHeight + 2 * console->nYPad );
				console->flags.bWrapCommand = 1;
			}
			else {
				//SetBrowserHeight( console->pCommandDisplay, 1 * nLineHeight + 2 * console->nYPad );
				console->flags.bWrapCommand = 0;
			}
		}
		else
			console->flags.bDirect = 1; // direct in with text... (0) mode only
		if( console->nHeight )
		{
			// may not have gotten visual fittting yet...
			EnterCriticalSec( &console->Lock );
			console->lockCount++;
			PSI_ConsoleCalculate( console, GetCommonFont( pc ) );
			console->lockCount--;
			LeaveCriticalSec( &console->Lock );
			SmudgeCommon( console->psicon.frame );
		}
	}
}

void PSI_SetConsoleBackingFile( PSI_CONTROL pc, FILE *file )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	SetHistoryBackingFile( console->pHistory, file );

}

PSI_CONSOLE_NAMESPACE_END
