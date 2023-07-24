#ifndef USE_IMAGE_INTERFACE
#define USE_IMAGE_INTERFACE ImageInterface
#endif
#ifndef USE_RENDER_INTERFACE
#define USE_RENDER_INTERFACE RenderInterface
#endif

#include <stdhdrs.h>
#include <psi.h>
#include <psi/console.h>

#include "consolestruc.h"
#include "WinLogic.h"
#include "ansi.h"

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
		eol = SegCreateFromText( "\n" );
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
					if( !console->flags.bNewLine )
						que->flags |= TF_NORETURN;
					phrase  = PSI_WinLogicWriteEx( console, que, 0 );
					LineRelease( prior );
				}
				else {
					que = SegCreate( 0 );
					if( !console->flags.bNewLine )
						que->flags |= TF_NORETURN;
					phrase = PSI_WinLogicWriteEx( console, que, 0 );
				}

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
			if( !console->flags.bNewLine ) {
				que->flags |= TF_NORETURN;
			}
			phrase = PSI_WinLogicWriteEx( console, que, 0 );
			console->flags.bNewLine = 0;
		}
		else
		{
			console->flags.bNewLine = 1;
		}
		//lprintf( "Smudge pc %p", pc );
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
	//lprintf( "Direct Output:%s", GetText( lines ) );
	phrase  = PSI_WinLogicWriteEx( console, lines, 0 );

	SmudgeCommon( pc );

	return phrase;
}

static void sendInput( uintptr_t psv, PTEXT output ) {
	PCONSOLE_INFO console = (PCONSOLE_INFO)psv;
	if( console->InputEvent )
		console->InputEvent( console->psvInputEvent, output );
	else LineRelease( output );
}


static void sendInputEvent( uintptr_t arg, PTEXT line ) {
	PCONSOLE_INFO console = (PCONSOLE_INFO)arg;
	int n;

	if( console->flags.bDirect && !console->flags.bNoLocalEcho ) {
		PTEXT pEcho;
		pEcho = BuildLine( line );
		//pEcho->data.size--; // trim the last character (probably cr)
		pEcho->flags |= TF_NORETURN;
		PSI_WinLogicWriteEx( console, pEcho, 1 );
		console->flags.bNewLine = 1;
		//lprintf( "Should this local echo be marked somehow?" );
		//pdp->History.flags.bEnqueuedLocalEcho = 1;
		//pdp->flags.bLastEnqueCommand = TRUE;
	}

	for( n = 0; n < console->lockCount; n++ )
		LeaveCriticalSec( &console->Lock );
	console->InputEvent( console->psvInputEvent, line );

	//PSI_Console_SetWriteCallback( console->ansi, sendInput, (uintptr_t)console );

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
		//lprintf( "printf Output:%s", GetText( output ) );

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

LOGICAL PSIConsoleGetLocalEcho( PSI_CONTROL pc )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( console ) {
		return !console->flags.bNoLocalEcho;
	}
	return FALSE;
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
		//GetStringSizeFont( " ", &console->nFontWidth, &console->nFontHeight, GetCommonFont( pc ) );
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
		else {
			console->flags.bDirect = 1; // direct in with text... (0) mode only
			console->flags.bWrapCommand = 1;
		}
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

void PSI_Console_SetSizeCallback( PSI_CONTROL pc, void (*update)(uintptr_t, int width, int height, int cols, int rows ), uintptr_t psv){
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	console->UpdateSize = update;
	console->psvUpdateSize = psv;
}

PSI_CONSOLE_NAMESPACE_END
