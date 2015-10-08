#ifndef CHAT_CONTROL_INTERNALS_DEFINED
#define CHAT_CONTROL_INTERNALS_DEFINED

// --- cheat
#include "consolestruc.h"
#include "../../../psilib/console/history.h"
#include "../../../psilib/console/histstruct.h"

// ----- common, everyone needs...

typedef struct chat_list_tag
{
	PLINKQUEUE contexts; // PCHAT_CONTEXT list which contain messages
	//PLINKQUEUE messages; //
	Image message_window;
	int first_button;
	int control_offset;
	_32 first_x, first_y, _b;
	struct {
		int message_top; // working variable used while drawing
	} display;
	struct {
		BIT_FIELD begin_button : 1;
		BIT_FIELD checked_drag : 1;
		BIT_FIELD long_vertical_drag : 1;
		BIT_FIELD long_horizontal_drag : 1;
		BIT_FIELD bCursorOn : 1;
		BIT_FIELD bSizing : 1; // prevent smudge while resizing
		BIT_FIELD in_command : 1;
		BIT_FIELD allow_command_popup : 1;
	} flags;
	struct {
		PHISTORY_REGION pHistory;
		PHISTORY_BROWSER phb_Input;
		PUSER_INPUT_BUFFER CommandInfo;
		int command_lines; // how many lines of text are formatted
		int command_skip_lines; // how many lines of the input are skipped
		int command_mark_cursor_down;
		int command_mark_start;
		int command_mark_end;
		int control_key_state;
	} input;
	void (CPROC*InputData)( PTRSZVAL psv, PTEXT input );
	PTRSZVAL psvInputData;
	void (CPROC*InputPaste)( PTRSZVAL psv );
	PTRSZVAL psvInputPaste;
	LOGICAL (CPROC*InputDrop)( PTRSZVAL psv, CTEXTSTR input, S_32 x, S_32 y );
	PTRSZVAL psvInputDrop;
	void (CPROC*MessageSeen)( PTRSZVAL psv );
	void (CPROC*ImageSeen)( PTRSZVAL psv, Image image );
	PTRSZVAL psvMessageSeen;
	void (CPROC*MessageDeleted)( PTRSZVAL psvSeen );
	void (CPROC*ImageDeleted)( PTRSZVAL psvSeen, Image image );
	void (CPROC*PopupEvent)( PTRSZVAL,LOGICAL );
	PTRSZVAL psvPopup;
	//PHISTORY_LINE_CURSOR phlc_Input;
	PSI_CONTROL send_button;
	_32 send_button_width;  // if 0, is 55
	_32 send_button_height; // if 0, sizes to the height of the input area
	S_32 send_button_x_offset;
	S_32 send_button_y_offset;
	SFTFont input_font;
	SFTFont date_font;
	int nFontHeight;
	int command_height; // height of text in input area
	int command_size;   // total rendered height, including skip and frame
	int send_button_size; // separate from 
	SFTFont message_font;
		struct
		{
			Image image;
			CDATA  crCommand;
			CDATA  crCommandBackground;
			CDATA  crBackground;
			CDATA  crMark;
			CDATA  crMarkBackground;
			// current working parameters...
			CDATA crText;
			CDATA crBack;
			CDATA background_color;
		} colors;
		SFTFont sent_font;
		SFTFont sender_font;
		SFTFont received_font;
		_32 cursor_timer;
		PSI_CONTROL pc;
		PMENU popup_text_entry;
	void (CPROC*OnImageAutoClose)( PTRSZVAL );
	PTRSZVAL psvImageAutoClose;
} CHAT_LIST;
typedef struct chat_list_tag *PCHAT_LIST;


#define l local_scollable_list_data
#ifndef CHAT_CONTROL_MAIN_SOURCE
extern
#endif
struct scollable_list_local
{
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	CTEXTSTR decoration_name;
	Image decoration;
	struct {
		S_32 back_x, back_y;
		_32 back_w, back_h;
		S_32 div_x1, div_x2;
		S_32 div_y1, div_y2;
		Image BorderSegment[9];
		CDATA text_color;
	} send;
	struct {
		S_32 back_x, back_y;
		_32 back_w, back_h;
		S_32 arrow_x, arrow_y;
		_32 arrow_w, arrow_h;
		S_32 arrow_x_offset, arrow_y_offset;
		S_32 div_x1, div_x2;
		S_32 div_y1, div_y2;
		Image BorderSegment[9];
		Image arrow;
		CDATA text_color;
	} sent;
	struct {
		S_32 back_x, back_y;
		_32 back_w, back_h;
		S_32 arrow_x, arrow_y;
		_32 arrow_w, arrow_h;
		S_32 arrow_x_offset, arrow_y_offset;
		S_32 div_x1, div_x2;
		S_32 div_y1, div_y2;
		Image BorderSegment[9];
		Image arrow;
		CDATA text_color;
	} received;
	struct {
		BIT_FIELD sent_justification : 2;
		BIT_FIELD received_justification : 2;
		BIT_FIELD sent_text_justification : 2;
		BIT_FIELD received_text_justification : 2;
	} flags;
	Image button_pressed, button_normal;
	struct chat_time_tag now;
	int side_pad;
	int time_pad;
	int context_message_pad;
	int context_sender_pad;

} l;


//-------------- chat_command_line.c

void RenderTextLine( 
	PCHAT_LIST list
	, Image window
	, PDISPLAYED_LINE pCurrentLine
	, RECT *r
	, int nLine
	, SFTFont font
	, int nFirstLine
	, int nMinLine
	, int left_pad
	, int mark_start
	, int mark_end
						);

//----- console_keydefs.c
void Widget_WinLogicDoStroke( PCHAT_LIST list, PTEXT stroke );
int Widget_DoStroke( PCHAT_LIST list, PTEXT stroke );
void Widget_KeyPressHandler( PCHAT_LIST list
						  , _8 key_index
						  , _8 mod
						  , PTEXT characters
						  );
void Widget_KeyPressHandlerRelease( PCHAT_LIST list
						  , _8 key_index
						  , _8 mod
						  , PTEXT characters
						  );
void ReformatInput( PCHAT_LIST list );

int GetInputCursorIndex( PCHAT_LIST list, int x, int y );
void GetInputCursorPos( PCHAT_LIST list, int *x, int *y );


#endif
