#ifndef CHAT_CONTROL_INTERNALS_DEFINED
#define CHAT_CONTROL_INTERNALS_DEFINED

// --- cheat
#include "consolestruc.h"
#include "history.h"
#include "histstruct.h"

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
	PHISTORY_REGION pHistory;
	PHISTORY_BROWSER phb_Input;
	PUSER_INPUT_BUFFER CommandInfo;
	void (CPROC*InputData)( PTRSZVAL psv, PTEXT input );
	PTRSZVAL psvInputData;
	void (CPROC*InputPaste)( PTRSZVAL psv );
	PTRSZVAL psvInputPaste;
	void (CPROC*InputDrop)( PTRSZVAL psv, CTEXTSTR input, S_32 x, S_32 y );
	PTRSZVAL psvInputDrop;
	void (CPROC*MessageSeen)( PTRSZVAL psv );
	PTRSZVAL psvMessageSeen;
	void (CPROC*MessageDeleted)( PTRSZVAL psvSeen );
	PHISTORY_LINE_CURSOR phlc_Input;
	PSI_CONTROL send_button;
	SFTFont input_font;
	SFTFont date_font;
	int nFontHeight;
	int command_height; // height of text in input area
	int command_size;   // total rendered height, including skip and frame
	int command_lines; // how many lines of text are showing.
	int command_skip_lines; // how many lines of the input are skipped
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
	, LOGICAL mark_applies
	, LOGICAL allow_segment_coloring
						);

//----- console_keydefs.c
void Widget_WinLogicDoStroke( PCHAT_LIST list, PTEXT stroke );
int Widget_DoStroke( PCHAT_LIST list, PTEXT stroke );
void Widget_KeyPressHandler( PCHAT_LIST list
						  , _8 key_index
						  , _8 mod
						  , PTEXT characters
						  );

#endif
