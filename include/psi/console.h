
#ifndef PSI_CONSOLE_INTERFACE_DEFINED
#define PSI_CONSOLE_INTERFACE_DEFINED

#ifdef PSI_CONSOLE_SOURCE
#define PSI_CONSOLE_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PSI_CONSOLE_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
#define _CONSOLE_NAMESPACE namespace console {
#define PSI_CONSOLE_NAMESPACE PSI_NAMESPACE _CONSOLE_NAMESPACE
#define PSI_CONSOLE_NAMESPACE_END } PSI_NAMESPACE_END
#define USE_PSI_CONSOLE_NAMESPACE using namespace sack::PSI::console
#else
#define _CONSOLE_NAMESPACE
#define PSI_CONSOLE_NAMESPACE 
#define PSI_CONSOLE_NAMESPACE_END 
#define USE_PSI_CONSOLE_NAMESPACE
#endif



#include <controls.h>

PSI_NAMESPACE
	_CONSOLE_NAMESPACE


typedef struct PSI_console_phrase *PSI_Console_Phrase;
typedef struct PSI_console_feedback *PSI_Console_Feedback;
typedef void (*PSI_Console_FeedbackClick)( uintptr_t psv_user, PSI_Console_Phrase );
typedef struct PSI_console_draw_context *PSI_Console_DrawContext;
typedef void (*PSI_ConsoleMeasure)(void);
typedef void(* PSI_ConsoleOutput)( void );

PSI_CONSOLE_PROC( void, PSIConsoleLoadFile )( PSI_CONTROL pc, CTEXTSTR filename );
PSI_CONSOLE_PROC( void, PSIConsoleSaveFile )( PSI_CONTROL pc, CTEXTSTR filename );

PSI_CONSOLE_PROC( int, vpcprintf )( PSI_CONTROL pc, CTEXTSTR format, va_list args );
PSI_CONSOLE_PROC( int, pcprintf )( PSI_CONTROL pc, CTEXTSTR format, ... );

// this results with a context structure that allows referencing the line in the console in the future..
PSI_CONSOLE_PROC( PSI_Console_Phrase, PSIConsoleOutput )( PSI_CONTROL pc, PTEXT lines );
PSI_CONSOLE_PROC( PSI_Console_Phrase, PSIConsoleDirectOutput )( PSI_CONTROL pc, PTEXT lines );

PSI_CONSOLE_PROC( void, PSI_Console_SetPhraseData )( PSI_Console_Phrase phrase, uintptr_t psv );
PSI_CONSOLE_PROC( uintptr_t, PSI_Console_GetPhraseData )( PSI_Console_Phrase phrase );
/* measure and draw */

PSI_CONSOLE_PROC( PSI_Console_DrawContext, PSIConsoleCreateDrawContext )( PSI_CONTROL pc, PSI_ConsoleMeasure, PSI_ConsoleOutput );
PSI_CONSOLE_PROC( PSI_Console_Phrase, PSIConsoleOwnerDrawnOutput )( PSI_CONTROL pc, PSI_Console_DrawContext, uintptr_t psv_user_data );

/* register a callback to get the line that was input into the console by the user */
PSI_CONSOLE_PROC( void, PSIConsoleInputEvent )( PSI_CONTROL pc, void(CPROC*Event)(uintptr_t,PTEXT), uintptr_t psv );

/* if TRUE, then the line that was read is immediately queued to the output stream */
PSI_CONSOLE_PROC( void, PSIConsoleSetLocalEcho )( PSI_CONTROL pc, LOGICAL yesno );
PSI_CONSOLE_PROC( LOGICAL, PSIConsoleGetLocalEcho )( PSI_CONTROL pc );

PSI_CONSOLE_PROC( PSI_Console_Feedback, PSI_ConsoleDefineFeedback )( PSI_Console_FeedbackClick, uintptr_t );

// during a feedback event, this can be used to remove the current mesasge.
//PSI_CONSOLE_PROC( PSI_Console_Feedback, PSI_Console_FeedbackDelete )( PSI_Feedback context );

//PSI_CONSOLE_PROC( void, PSIConsoleSetMessageFeedbackHandler )( PSI_CONTROL pc, PSI_Feedback );

// this is an access into to use console features to wrap long text into a block.
// passed width in characters, no font.
PSI_CONSOLE_PROC( void, FormatTextToBlock )( CTEXTSTR input, TEXTSTR *output, int char_width, int char_height );
// the width and height passed are updated for the actual pixel width and height of the text block
// this allows a larger height to be passed; and allows the box to be shrunk slightly for width if
// words and roundings made it smaller than it appears.  (allow better centering)
// this is an access into to use console features to wrap long text into a block.
// font can be NULL and 0,0 for pixel size to format by characters only.
PSI_CONSOLE_PROC( void, FormatTextToBlockEx )( CTEXTSTR input, TEXTSTR *output, int *pixel_width, int *pixel_height, SFTFont font );

PSI_CONSOLE_PROC( struct history_tracking_info *, PSIConsoleSaveHistory )( PSI_CONTROL pc );
PSI_CONSOLE_PROC( void, PSIConsoleSetHistory )( PSI_CONTROL pc, struct history_tracking_info *history_info );

// mode 0 = inline/scrolling
// mode 1 = line buffer/scrolling
// mode 2 = line buffer/wrap
PSI_CONSOLE_PROC( void, PSIConsoleSetInputMode )( PSI_CONTROL pc, int mode );
PSI_CONSOLE_PROC( void, PSI_SaveConsoleToFile )( PSI_CONTROL pc, FILE *file );
PSI_CONSOLE_PROC( void, PSI_ReadConsoleFromFile )( PSI_CONTROL pc, FILE *file );


PSI_CONSOLE_NAMESPACE_END;

USE_PSI_CONSOLE_NAMESPACE;

#endif
