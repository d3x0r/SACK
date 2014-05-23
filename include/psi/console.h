
#ifndef PSI_CONSOLE_INTERFACE_DEFINED
#define PSI_CONSOLE_INTERFACE_DEFINED

#ifdef PSI_CONSOLE_SOURCE
#define PSI_CONSOLE_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PSI_CONSOLE_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
#define _CONSOLE_NAMESPACE namespace console {
#define PSI_CONSOLE_NAMESPACE PSI_NAMESPACE namespace console {
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


typedef struct PSI_phase *PSI_Phrase;
typedef struct PSI_feedback *PSI_Feedback;
typedef void (*PSI_FeedbackClick)( PTRSZVAL psv_user, PSI_Feedback, PTEXT line );
typedef struct PSI_draw_context *PSI_DrawContext;

PSI_CONSOLE_PROC( void, PSIConsoleLoadFile )( PSI_CONTROL pc, CTEXTSTR filename );
PSI_CONSOLE_PROC( void, PSIConsoleSaveFile )( PSI_CONTROL pc, CTEXTSTR filename );

PSI_CONSOLE_PROC( int, vpcprintf )( PSI_CONTROL pc, CTEXTSTR format, va_list args );
PSI_CONSOLE_PROC( int, pcprintf )( PSI_CONTROL pc, CTEXTSTR format, ... );
PSI_CONSOLE_PROC( int, PSIConsoleOutput )( PSI_CONTROL pc, PTEXT lines );

// this results with a context structure that allows referencing the line in the console in the future..
PSI_CONSOLE_PROC( PSI_Phrase, PSIConsolePhraseOutput )( PSI_CONTROL pc, PTEXT lines );

/* measure and draw */

PSI_CONSOLE_PROC( PSI_DrawContext, PSIConsoleCreateDrawContext )( PSI_CONTROL pc, PSI_ConsoleMeausre, PSI_ConsoleOutput );
PSI_CONSOLE_PROC( PSI_Phrase, PSIConsoleOwnerDrawnOutput )( PSI_CONTROL pc, PSI_DrawContext, PTRSZVAL psv_user_data );

PSI_CONSOLE_PROC( void, PSIConsoleInputEvent )( PSI_CONTROL pc, void(CPROC*Event)(PTRSZVAL,PTEXT), PTRSZVAL psv );
PSI_CONSOLE_PROC( void, PSIConsoleSetLocalEcho )( PSI_CONTROL pc, LOGICAL yesno );


PSI_CONSOLE_PROC( PSI_Feedback, PSI_DefineFeedback )( click, PTRSZVAL );

// during a feedback event, this can be used to remove the current mesasge.
PSI_CONSOLE_PROC( PSI_Feedback, PSI_FeedbackDelete )( PSI_Feedback context );

PSI_CONSOLE_PROC( void, PSIConsoleSetMessageFeedbackHandler )( PSI_CONTROL pc, PSI_Feedback );

// this is an access into to use console features to wrap long text into a block.
PSI_CONSOLE_PROC( void, FormatTextToBlock )( CTEXTSTR input, TEXTSTR *output, int char_width, int char_height );

PSI_CONSOLE_NAMESPACE_END;

USE_PSI_CONSOLE_NAMESPACE;

#endif
