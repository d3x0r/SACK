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
	uint32_t first_x, first_y, _b;
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
	void (CPROC*InputData)( uintptr_t psv, PTEXT input );
	uintptr_t psvInputData;
	void (CPROC*InputPaste)( uintptr_t psv );
	uintptr_t psvInputPaste;
	LOGICAL (CPROC*InputDrop)( uintptr_t psv, CTEXTSTR input, int32_t x, int32_t y );
	uintptr_t psvInputDrop;
	void (CPROC*MessageSeen)( uintptr_t psv );
	void (CPROC*ImageSeen)( uintptr_t psv, Image image );
	uintptr_t psvMessageSeen;
	void (CPROC*MessageDeleted)( uintptr_t psvSeen );
	void (CPROC*ImageDeleted)( uintptr_t psvSeen, Image image );
	void (CPROC*PopupEvent)( uintptr_t,LOGICAL );
	uintptr_t psvPopup;
	//PHISTORY_LINE_CURSOR phlc_Input;
	PSI_CONTROL send_button;
	uint32_t send_button_width;  // if 0, is 55
	uint32_t send_button_height; // if 0, sizes to the height of the input area
	int32_t send_button_x_offset;
	int32_t send_button_y_offset;
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
		uint32_t cursor_timer;
		PSI_CONTROL pc;
		PMENU popup_text_entry;
	void (CPROC*OnImageAutoClose)( uintptr_t );
	uintptr_t psvImageAutoClose;
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
		int32_t back_x, back_y;
		uint32_t back_w, back_h;
		int32_t div_x1, div_x2;
		int32_t div_y1, div_y2;
		Image BorderSegment[9];
		CDATA text_color;
	} send;
	struct {
		int32_t back_x, back_y;
		uint32_t back_w, back_h;
		int32_t arrow_x, arrow_y;
		uint32_t arrow_w, arrow_h;
		int32_t arrow_x_offset, arrow_y_offset;
		int32_t div_x1, div_x2;
		int32_t div_y1, div_y2;
		Image BorderSegment[9];
		Image arrow;
		CDATA text_color;
	} sent;
	struct {
		int32_t back_x, back_y;
		uint32_t back_w, back_h;
		int32_t arrow_x, arrow_y;
		uint32_t arrow_w, arrow_h;
		int32_t arrow_x_offset, arrow_y_offset;
		int32_t div_x1, div_x2;
		int32_t div_y1, div_y2;
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
	SFTFont image_help_font;
	CDATA image_grid_background_color;
	CDATA image_grid_color;
	CDATA help_text_background;
	CDATA help_text_color;

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
						  , uint8_t key_index
						  , uint8_t mod
						  , PTEXT characters
						  );
void Widget_KeyPressHandlerRelease( PCHAT_LIST list
						  , uint8_t key_index
						  , uint8_t mod
						  , PTEXT characters
						  );
void ReformatInput( PCHAT_LIST list );

int GetInputCursorIndex( PCHAT_LIST list, int x, int y );
void GetInputCursorPos( PCHAT_LIST list, int *x, int *y );


#endif
