/*
 * Crafted by: Jim Buckeyne
 *
 * Pretty buttons, based on bag.psi image class buttons
 * and extended to the point that all that really remains is
 * the click handler... easy enough to steal, but it does
 * nice things like changes behavior based on touch screen
 * presense, and particular operational modes...
 *
 * Keypads - with numeric accumulator displays, based on
 *   shared memory regions such that many displays might
 *   accumulate into the same memory, with different
 *   visual aspects, and might get updated in return
 *   (well that's the theory) mostly it's a container
 *   for coordinating key board like events, a full alpha-
 *   numeric keyboard option exists, a 10 key entry pad (12)
 *   and soon a 16 key extended input with clear, cancel
 *   double 00, and decimal, and backspace  and(?)
 *
 * Some common external images may be defined in a theme.ini
 *  applications which don't otherwise specify graphics effects
 *  for the buttons enables them to conform to a default theme
 *  or one of multiple selections...
5 *
 * (c)Freedom Collective 2006+  (copy header include/widgets/buttons.h)
 */


#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
// this is ugly , but it works, please consider
// a library init that will grab this...
#ifndef __cplusplus_cli
#define USE_IMAGE_INTERFACE GetImageInterface()
#endif

#include <controls.h>
#include <configscript.h>
#include <sharemem.h>
#include <system.h>  // GetProgramName()
#include <idle.h>
#include <keybrd.h>
#include <fractions.h>
#include <sqlgetoption.h>
#include <psi.h>
#include <timers.h>

#define KEYPAD_STRUCTURE_DEFINED
#include "accum.h"
#include "../include/keypad.h"
#include "../include/buttons.h"
#include "../../intershell_registry.h"
#include "../../intershell_export.h"

KEYPAD_NAMESPACE

typedef struct key_space_size_tag {
   int rows, cols;
	FRACTION DisplayOffsetX;
	FRACTION DisplayX;
	FRACTION DisplayOffsetY;
	FRACTION DisplayY;

	FRACTION KeySpacingX; // 1/19th of the display...
	FRACTION KeySizingX; // 5/19ths of the display...
	FRACTION KeySpacingY; // 1/25th of the display...
	FRACTION KeySizingY; // 5/25ths of the display...
	FRACTION KeyDisplaySpacingY; // 1/25th of the display...
	FRACTION KeyDisplaySizingY; // 5/25ths of the display...
} KEY_SPACE_SIZE, *PKEY_SPACE_SIZE;

static KEY_SPACE_SIZE keypad_sizing = { 4, 3
												  , { 1, 19 }
												  , { 17, 19 }
												  , { 1, 28 }
												  , { 3, 28 }

												  , { 1, 55 } // 1/19th of the display...
												  , { 17, 55 } // 5/19ths of the display...
												  , { 1, 55 } // 1/25th of the display...
												  , { 25, 110 } // 5/25ths of the display...
												  , { 28, 1540 } // 1/25th of the display...
												  , { 298, 1540 } }; // 5/25ths of the display...

static KEY_SPACE_SIZE keyboard_sizing = { 4, 13
													 , { 1, 19 }
													 , { 17, 19 }
													 , { 1, 28 }
													 , { 2, 28 }

													 , { 1, 99 + 10 + 20 + 2 } // 1/19th of the display...
													 , { 9, 99 + 10 + 20 + 2 } // 5/19ths of the display...
													 , { 1, 60 } // 1/25th of the display...
													 , { 25, 121 } // 5/25ths of the display...
													 , { 28, 1540 } // 1/25th of the display...
													 , { 308, 1540 } }; // 5/25ths of the display...

// the amount of space between keys...
//static FRACTION DisplayOffsetX = { 1, 19 };
//static FRACTION DisplayX = { 17, 19 };
//static FRACTION DisplayOffsetY = { 1, 28 };
//static FRACTION DisplayY = { 2, 28 };

//static FRACTION KeySpacingX = { 1, 55 }; // 1/19th of the display...
//static FRACTION KeySizingX  = { 17, 55 }; // 5/19ths of the display...
//static FRACTION KeySpacingY = { 1, 55 }; // 1/25th of the display...
//static FRACTION KeySizingY  = { 10, 55 }; // 5/25ths of the display...
//static FRACTION KeyDisplaySpacingY = { 28, 1540 }; // 1/25th of the display...
//static FRACTION KeyDisplaySizingY  = { 308, 1540 }; // 5/25ths of the display...

//static FRACTION KeyBoardSpacingX = { 1, 101 }; // 1/19th of the display...
//static FRACTION KeyBoardSizingX  = { 9, 101 }; // 5/19ths of the display...
//static FRACTION KeyBoardSpacingY = { 1, 55 }; // 1/25th of the display...
//static FRACTION KeyBoardSizingY  = { 10, 55 }; // 5/25ths of the display...
//static FRACTION KeyBoardDisplaySpacingY = { 28, 1540 }; // 1/25th of the display...
//static FRACTION KeyBoardDisplaySizingY  = { 308, 1540 }; // 5/25ths of the display...
// total of fracitions is (col * 4 * spacingx ) + (col * 3 * sizingx) = 1
// total of fracitions is (row * 5 * spacingy ) + (row * 4 * sizingy) = 1

typedef struct magic_sequence
{
	CTEXTSTR sequence;
	void (CPROC*event_proc)( PTRSZVAL );
	PTRSZVAL psv_sequence;

	// tracking the state of progress in matching this sequence.
	_32 last_tick_sequence_match;
	int index_match;
} MAGIC_SEQUENCE, *PMAGIC_SEQUENCE;

typedef struct display_struct
{
	PCONTROL control;
	SFTFont font;
	_32 width, height;
	struct keypad_struct *keypad;
} DISPLAY, *PDISPLAY;

typedef struct key_holder
{
	PKEY_BUTTON key;
	CTEXTSTR psv_key; // actually this is key_value
	CTEXTSTR psv_shifted_key; // actually this is key_value
} KEY_HOLDER, *PKEY_HOLDER;

typedef struct keypad_struct
{
	PCOMMON frame;
	PACCUMULATOR accum;
	PUSER_INPUT_BUFFER pciEntry; // alpha keypads need this sort of buffer...
	//PTEXT entry; 
	SFTFont font;
	SFTFont ControlKeyFont;

	struct {
		BIT_FIELD bDisplay : 1;
		BIT_FIELD bPassword : 1;
		BIT_FIELD bEntry : 1;
		BIT_FIELD bResult : 1;
		BIT_FIELD bResultStatus : 1;
		BIT_FIELD bHidden : 1;
		BIT_FIELD bAlphaNum : 1;
		BIT_FIELD bClearOnNext : 1;
		BIT_FIELD bGoClear : 1;
		BIT_FIELD bLeftJustify : 1;
		BIT_FIELD bCenterJustify : 1;
	} flags;

	_32 displaywidth, displayheight;
	// last known size... when draw is triggered
	// see if we need to rescale the buttoms...
	PKEY_SPACE_SIZE key_spacing;
	_32 nKeys;
	KEY_HOLDER *keys;
	_32 width, height;
	void (CPROC *keypad_enter_event)(PTRSZVAL psv, PSI_CONTROL keypad );
	PTRSZVAL psvEnterEvent;
	void (CPROC *keypad_cancel_event)(PTRSZVAL psv, PSI_CONTROL keypad );
	PTRSZVAL psvCancelEvent;

	PCOMMON display;
	DeclareLink( struct keypad_struct );

	CDATA numkey_color;      // Color( 220, 220, 12 )
	CDATA enterkey_color;
	CDATA cancelkey_color;
	CDATA numkey_text_color;      // Color( 220, 220, 12 )
	CDATA enterkey_text_color;
	CDATA cancelkey_text_color;
	CDATA background_color; // AColor( 0, 0, 64, 150 )
	CDATA display_background_color; // Color( 129, 129, 149 )
	CDATA display_text_color;       // Color( 0, 0, 0 )
	CDATA shiftkey_color;       // Color( 0, 0, 0 )
	CDATA shiftkey_text_color;       // Color( 0, 0, 0 )
	CDATA backspacekey_color;       // Color( 0, 0, 0 )
	CDATA backspacekey_text_color;       // Color( 0, 0, 0 )
	CDATA capskey_color;       // Color( 0, 0, 0 )
	int rows, cols;
	int shifted;
	int want_shifted;
	int shift_lock;
	int typed_with_shift;
	enum keypad_styles style;
	PLIST magic_sequences;
	CTEXTSTR display_format;
	int display_out_length;
} KEYPAD;

static PKEYPAD keypads;
static _32 magic_sequence_time_length = 1250;
static struct {
	BIT_FIELD log_key_events : 1;
} flags;
// hmm be handy to get a frame resize message...

// when buttons are pressed...


static TEXTCHAR *keytext[]= { WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
								, WIDE("A4\0"), WIDE("A5\0"), WIDE("A6\0")
								, WIDE("A1\0"), WIDE("A2\0"), WIDE("A3\0")
								, WIDE("A<-\0"), WIDE("A0\0"), WIDE("A00\0")  };
static TEXTCHAR *entrytext[]= {  WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
								  ,  WIDE( "A4\0" ), WIDE("A5\0"), WIDE("A6\0")
								  ,  WIDE( "A1\0" ), WIDE("A2\0"), WIDE("A3\0")
								  ,  WIDE( "~BRedAN\0" ), WIDE("A0\0"), WIDE("~BgreenAY\0") };
static TEXTCHAR *entrytext2[]= {  WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
									,  WIDE( "A4\0" ), WIDE("A5\0"), WIDE("A6\0")
									,  WIDE( "A1\0" ), WIDE("A2\0"), WIDE("A3\0")
									,  WIDE( "AC\0" ), WIDE("A0\0"), WIDE("~BgreenA*\0") };

static TEXTCHAR *entrytext3[]= {  WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
									,  WIDE( "A4\0" ), WIDE("A5\0"), WIDE("A6\0")
									,  WIDE( "A1\0" ), WIDE("A2\0"), WIDE("A3\0")
//									,  WIDE( "~BblueAC\0" ), WIDE("A0\0"), WIDE("~BgreenAE\0") };
									,  WIDE( "AC\0" ), WIDE("A0\0"), WIDE("AE\0") };
static TEXTCHAR *entrytext4[]= {  WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
									,  WIDE( "A4\0" ), WIDE("A5\0"), WIDE("A6\0")
									,  WIDE( "A1\0" ), WIDE("A2\0"), WIDE("A3\0")
										 ,  WIDE( "AClr\0" ), WIDE("A0\0"), WIDE("~BgreenAGo\0") };
static TEXTCHAR *entrytext5[]= {  WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
									,  WIDE( "A4\0" ), WIDE("A5\0"), WIDE("A6\0")
									,  WIDE( "A1\0" ), WIDE("A2\0"), WIDE("A3\0")
										 ,  WIDE( "~BredC\0" ), WIDE("A0\0"), WIDE("~BgreenAE\0") };

static TEXTCHAR *entrytext6[]= {  WIDE( "A7\0" ), WIDE("A8\0"), WIDE("A9\0")
									,  WIDE( "A4\0" ), WIDE("A5\0"), WIDE("A6\0")
									,  WIDE( "A1\0" ), WIDE("A2\0"), WIDE("A3\0")
										 ,  WIDE( "A<-\0" ), WIDE("A0\0"), WIDE("~BgreenAGo\0") };
static TEXTCHAR * keyval[] = {  WIDE( "7" ), WIDE("8"), WIDE("9")
							  ,  WIDE( "4" ), WIDE("5"), WIDE("6")
							  ,  WIDE( "1" ), WIDE("2"), WIDE("3")
							  ,  (TEXTSTR)-1, WIDE("0"), (TEXTSTR)-2 };


static TEXTCHAR *keyboardtext[]= { WIDE("AEs\0"),  WIDE("A1\0"), WIDE("A2\0"), WIDE("A3\0"), WIDE("A4\0"), WIDE("A5\0"), WIDE("A6\0"), WIDE("A7\0"), WIDE("A8\0"),    WIDE("A9\0"), WIDE("A0\0"), WIDE("A-\0" ), WIDE("A=\0" )
											, WIDE("A\\\0" ), WIDE("Aq\0"), WIDE("Aw\0"), WIDE("Ae\0"), WIDE("Ar\0"), WIDE("At\0"), WIDE("Ay\0"), WIDE("Au\0"), WIDE("Ai\0"),    WIDE("Ao\0"), WIDE("Ap\0"), WIDE("A[\0"), WIDE("A]\0")
											, WIDE("A^^\0" ), WIDE("Aa\0"), WIDE("As\0"), WIDE("Ad\0"), WIDE("Af\0"), WIDE("Ag\0"), WIDE("Ah\0"), WIDE("Aj\0"), WIDE("Ak\0"),    WIDE("Al\0"), WIDE("A;\0" ), WIDE( "A'\0" ), WIDE("A<-\0")
											, WIDE("A^\0"),   WIDE("Az\0"), WIDE("Ax\0"), WIDE("Ac\0"), WIDE("Av\0"), WIDE("Ab\0"), WIDE("An\0"), WIDE("Am\0"), WIDE("A[ ]\0" ), WIDE("A,\0"), WIDE("A.\0"), WIDE("A/\0"), WIDE("ABS\0")
};
static TEXTCHAR *keyboard_shifted_text[]= { WIDE("AEs\0"), WIDE("A!\0"), WIDE("A@\0"), WIDE("A#\0"), WIDE("A$\0"), WIDE("A%\0"), WIDE("A^\0"), WIDE("A&\0"), WIDE("A*\0"), WIDE("A(\0"), WIDE("A)\0"), WIDE("A_\0" ), WIDE("A+\0" )
														, WIDE("A|\0" ), WIDE("AQ\0"), WIDE("AW\0"), WIDE("AE\0"), WIDE("AR\0"), WIDE("AT\0"), WIDE("AY\0"), WIDE("AU\0"), WIDE("AI\0"), WIDE("AO\0"), WIDE("AP\0"), WIDE("A{\0" ), WIDE("A}\0" )
														, WIDE( "A^^\0"), WIDE("AA\0"), WIDE("AS\0"), WIDE("AD\0"), WIDE("AF\0"), WIDE("AG\0"), WIDE("AH\0"), WIDE("AJ\0"), WIDE("AK\0"), WIDE("AL\0"), WIDE("A:\0" ), WIDE("A\"\0" ), WIDE("A<-\0")
														, WIDE("A^\0") , WIDE("AZ\0"), WIDE("AX\0"), WIDE("AC\0"), WIDE("AV\0"), WIDE("AB\0"), WIDE("AN\0"), WIDE("AM\0"), WIDE("A[ ]\0" ), WIDE("A<\0"), WIDE("A>\0" ), WIDE("A?\0"), WIDE("ABS\0")
};
static CTEXTSTR keyboardval[] = { WIDE("\x1b"),  WIDE("1"), WIDE("2"), WIDE("3"), WIDE("4"), WIDE("5"), WIDE("6"), WIDE("7"), WIDE("8"), WIDE("9"), WIDE("0"), WIDE("-"), WIDE("=" )
										  , WIDE("\\" ),   WIDE("q"), WIDE("w"), WIDE("e"), WIDE("r"), WIDE("t"), WIDE("y"), WIDE("u"), WIDE("i"), WIDE("o"), WIDE("p"), WIDE("["), WIDE("]" )
										  , (CTEXTSTR)-1,  WIDE("a"), WIDE("s"), WIDE("d"), WIDE("f"), WIDE("g"), WIDE("h"), WIDE("j"), WIDE("k"), WIDE("l"), WIDE(";"), WIDE("'"), (CTEXTSTR)-2
										  , (TEXTCHAR*)-3, WIDE("z"), WIDE("x"), WIDE("c"), WIDE("v"), WIDE("b"), WIDE("n"), WIDE("m"), WIDE(" "), WIDE(","), WIDE("."), WIDE("/"), WIDE("\b")
};
static CTEXTSTR keyboard_shifted_val[] = {  WIDE("\x1b"), WIDE("!"), WIDE("@"), WIDE("#"), WIDE("$"), WIDE("%"), WIDE("^"), WIDE("&"), WIDE("*"), WIDE("("), WIDE(")"), WIDE("_" ), WIDE("+")
													  ,  WIDE( "|" ),  WIDE("Q"), WIDE("W"), WIDE("E"), WIDE("R"), WIDE("T"), WIDE("Y"), WIDE("U"), WIDE("I"), WIDE("O"), WIDE("P"), WIDE("{" ), WIDE("}")
													  , (CTEXTSTR)-1,  WIDE("A"), WIDE("S"), WIDE("D"), WIDE("F"), WIDE("G"), WIDE("H"), WIDE("J"), WIDE("K"), WIDE("L"), WIDE(":"), WIDE("\""),  (CTEXTSTR)-2
													  , (TEXTCHAR*)-3, WIDE("Z"), WIDE("X"), WIDE("C"), WIDE("V"), WIDE("B"), WIDE("N"), WIDE("M"), WIDE(" "), WIDE("<"), WIDE(">"), WIDE( "?" ), WIDE("\b")
};



//CONTROL_REGISTRATION keypad_display, keypad_control;
int CPROC InitKeypad( PSI_CONTROL pc );
//static int CPROC KeypadDraw( PCOMMON frame );
int CPROC InitKeypadDisplay( PCOMMON pc );
static int CPROC DrawKeypadDisplay( PCONTROL pc );


CONTROL_REGISTRATION keypad_control = { WIDE( "Keypad Control 2" )
												  , { { 240, 320 }, sizeof( KEYPAD ), BORDER_NONE }
												  , InitKeypad
												  , NULL
                                      , NULL // KeypadDraw
};
CONTROL_REGISTRATION keypad_display = { WIDE( "Keypad Display 2" )
												  , { { 180, 20 }, sizeof( DISPLAY ), BORDER_INVERT|BORDER_THIN }
												  , InitKeypadDisplay
												  , NULL
												  , DrawKeypadDisplay
};
void SetDisplayPadKeypad( PSI_CONTROL pc, PKEYPAD keypad );  //added this because this function is called in at least one place before it is defined gcc compiler version 3.4.4 doesn't like it but it compiles fine on 3.3.x.

static void InvokeMagicSequences( PSI_CONTROL pc, CTEXTSTR new_data )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		INDEX idx;
		PMAGIC_SEQUENCE magic_sequence;
		LIST_FORALL( keypad->magic_sequences, idx, PMAGIC_SEQUENCE, magic_sequence )
		{
			_32 now = timeGetTime();
			TEXTCHAR this_char;
			if( magic_sequence->last_tick_sequence_match && 
				( ( magic_sequence->last_tick_sequence_match + magic_sequence_time_length ) < now ) )
			{
				// time expired, reset match conditions.
				magic_sequence->index_match = 0;
				magic_sequence->last_tick_sequence_match = 0;
			}
			switch( this_char = magic_sequence->sequence[magic_sequence->index_match] )
			{
			case '\\':
				if( new_data[0] == '\\' )
				{
					if( new_data[1] == magic_sequence->sequence[magic_sequence->index_match+1] )
					{
						if( magic_sequence->sequence[magic_sequence->index_match + 2] == 0 )
						{
							magic_sequence->event_proc( magic_sequence->psv_sequence );
							magic_sequence->index_match = 0;
							magic_sequence->last_tick_sequence_match = 0;
						}
						else
						{
							magic_sequence->index_match += 2;
							magic_sequence->last_tick_sequence_match = timeGetTime();
						}
						break;
					}
				}
				magic_sequence->index_match = 0;
				magic_sequence->last_tick_sequence_match = 0;
				break;
			default:
				if( new_data[0] == this_char )
				{
					if( magic_sequence->sequence[magic_sequence->index_match + 1] == 0 )
					{
						magic_sequence->event_proc( magic_sequence->psv_sequence );
						magic_sequence->index_match = 0;
						magic_sequence->last_tick_sequence_match = 0;
					}
					else
					{
						magic_sequence->index_match++;
						magic_sequence->last_tick_sequence_match = timeGetTime();
					}
					break;
				}
				magic_sequence->index_match = 0;
				magic_sequence->last_tick_sequence_match = 0;
				break;
			}
		}
	}
}



static void InvokeEnterEvent( PCOMMON pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		if( keypad->keypad_enter_event )
		{
			keypad->keypad_enter_event( keypad->psvEnterEvent, pc );
		}
	}
}

static void InvokeCancelEvent( PCOMMON pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		if( keypad->keypad_cancel_event )
		{
			keypad->keypad_cancel_event( keypad->psvCancelEvent, pc );
		}
	}
}

void SetKeypadEnterEvent( PCOMMON pc, void (CPROC *event)(PTRSZVAL,PSI_CONTROL), PTRSZVAL psv )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->keypad_enter_event = event;
		keypad->psvEnterEvent = psv;
	}
}

void SetKeypadCancelEvent( PCOMMON pc, void (CPROC *event)(PTRSZVAL,PSI_CONTROL), PTRSZVAL psv )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->keypad_cancel_event = event;
		keypad->psvCancelEvent = psv;
	}
}

int CPROC InitKeypadDisplay( PCOMMON pc )
{
	ValidatedControlData( PDISPLAY, keypad_display.TypeID, display, pc );
	if( display )
	{
		Image surface = GetControlSurface( pc );
		display->width = surface->width;
		display->height = surface->height;
		display->font = RenderFontFile( WIDE("Crysta.ttf")
												, display->height - display->height/10
												, display->height - display->height/10
												, 3 );
		if( !display->font )
			display->font = RenderFontFile( WIDE("fonts/Crysta.ttf")
													, display->height - display->height/10
													, display->height - display->height/10
													, 3 );
		return 1;
	}
	return 0;
}

static void FormatKeypadDisplay( CTEXTSTR format, int out_start, TEXTCHAR *output, TEXTCHAR *input )
{
	int n;
	int outpos = out_start;
	int inpos = StrLen( input ) - 1;
	int escape = 0;
	output[outpos--] = 0;
	for( n = StrLen( format ) - 1; n >= 0; n-- )
	{
		switch( format[n] )
		{
		case '#':
			if( escape )
			{
				escape = 0;
				output[outpos--] = format[n];
			}
			else
			{
				if( inpos >= 0 )
					output[outpos--] = input[inpos--];
				else
					output[outpos--] = '0';
			}
			break;
		case '\\':
			escape = 1;
			break;
		default:
			output[outpos--] = format[n];
			break;
		}
	}
}


static int CPROC DrawKeypadDisplay( PCONTROL pc )
{
	TEXTCHAR tmp_text[128];
	TEXTCHAR text[128];
	TEXTCHAR *output;
		_32 width, height;
		ValidatedControlData( PDISPLAY, keypad_display.TypeID, display, pc );
		PKEYPAD pKeyPad = display->keypad;
		Image surface = GetControlSurface( pc );
		ClearImageTo( surface, pKeyPad->display_background_color );
		if( GetAccumulatorText( pKeyPad->accum, text, sizeof( text ) ) )
		{
			if( pKeyPad->flags.bPassword )
			{
				TEXTCHAR *from, *to;

				for( from = to = text; from[0]; from++ )
				{
					if( !pKeyPad->flags.bAlphaNum )
					{
						if( from[0] == '$' ) continue;
						if( from[0] == '.' ) continue;
						if( to == text && from[0] == '0' ) continue;
					}
					to[0] = '#';
					to++;
				}

				to[0] = 0;
			}
			if( pKeyPad->display_format )
			{
				FormatKeypadDisplay( pKeyPad->display_format, pKeyPad->display_out_length, tmp_text, text );
				output = tmp_text;
			}
			else
				output = text;
			GetStringSizeFont( output, &width, &height, display->font );
			//lprintf( "putting string(%s) at... %d,%d", output, display->width - ( width + 3 )
			//				 , ( display->height - height ) / 2 );
			if( pKeyPad->flags.bLeftJustify )
				PutStringFont( surface
								 , 3
								 , ( display->height - height ) / 2
								 , pKeyPad->display_text_color, 0
								 , output
								 , display->font );
			else if( pKeyPad->flags.bCenterJustify )
				PutStringFont( surface
								 , ( display->width - width ) / 2
								 , ( display->height - height ) / 2
								 , pKeyPad->display_text_color, 0
								 , output
								 , display->font );
			else
				PutStringFont( surface
								 , display->width - ( width + 3 )
								 , ( display->height - height ) / 2
								 , pKeyPad->display_text_color, 0
								 , output
								 , display->font );
		}

      return 1;
}

static void KeypadAccumUpdated( PTRSZVAL psvKeypad, PACCUMULATOR accum )
{
	PKEYPAD keypad = (PKEYPAD)psvKeypad;
	if( keypad->display )
		SmudgeCommon( (PCOMMON)keypad->display );

	//  if( keypad->flags.bDisplay )
	//	DrawDisplay( psvKeypad, keypad->display.control );
}

static int resize_keys( PKEYPAD keypad )
{
   unsigned int w, h;
	int row, col, rows, cols;
	FRACTION posy, posx, tmp;
	S_32 keyx, keyy, keywidth, keyheight;
	// we might get called here to resize
	// when we don't really have any children at all... and no spacing factor to
	// apply.


	if( keypad && keypad->key_spacing )
	{
		PKEY_SPACE_SIZE pKeySizing = keypad->key_spacing;
		w = keypad->width;
		h = keypad->height;
		rows = pKeySizing->rows;
		cols = pKeySizing->cols;
		keywidth = ReduceFraction( ScaleFraction( &tmp, w, &pKeySizing->KeySizingX ) );
		if( keypad->flags.bDisplay )
		{
			keyheight = ReduceFraction( ScaleFraction( &tmp, h, &pKeySizing->KeyDisplaySizingY ) );
			posy = pKeySizing->DisplayOffsetY;
			AddFractions( &posy, &pKeySizing->DisplayY );
			AddFractions( &posy, &pKeySizing->KeyDisplaySpacingY );
		}
		else
		{
			keyheight = ReduceFraction( ScaleFraction( &tmp, h, &pKeySizing->KeySizingY ) );
			posy = pKeySizing->KeySpacingY;
		}

      //lprintf( "New font render at %d,%d", keywidth - keywidth/3, keyheight - keyheight/3 );
		RerenderFont( keypad->font, keywidth - keywidth/3, keyheight - keyheight/4, NULL, NULL );
		if( keypad->flags.bDisplay )
		{
			int displayx, displayy, displaywidth, displayheight;
			displayx = ReduceFraction( ScaleFraction( &tmp, keypad->width, &pKeySizing->DisplayOffsetX ) );
			displayy = ReduceFraction( ScaleFraction( &tmp, keypad->height, &pKeySizing->DisplayOffsetY ) );
			displaywidth = ReduceFraction( ScaleFraction( &tmp, keypad->width, &pKeySizing->DisplayX ) );
			displayheight = ReduceFraction( ScaleFraction( &tmp, keypad->height, &pKeySizing->DisplayY ) );
			MoveSizeCommon( keypad->display
							  , displayx, displayy
							  , displaywidth, displayheight
							  );
			{
				ValidatedControlData( PDISPLAY, keypad_display.TypeID, display, keypad->display );
				display->width = displaywidth;
            display->height = displayheight;
				RerenderFont( display->font, displayheight - displayheight/10, displayheight - displayheight/10, NULL, NULL );
			}
		}
		for( row = 0; row < rows; row++ )
		{
			keyy = ReduceFraction( ScaleFraction( &tmp, keypad->height, &posy ) );
			posx = pKeySizing->KeySpacingX;
			for( col = 0; col < cols; col++ )
			{
				keyx = ReduceFraction( ScaleFraction( &tmp, keypad->width, &posx ) );
				//{
				//	TEXTCHAR tmp[32];
				//	sLogFraction( tmp, &posx );
				//	lprintf( WIDE("Position x: %s - %d %d"), tmp, keyx, keypad->width );
				//	sLogFraction( tmp, &posy );
				//	lprintf( WIDE("Position y: %s - %d %d"), tmp, keyy, keypad->height );
				//}
				MoveSizeCommon( GetKeyCommon( keypad->keys[row * cols + col].key )
								  , keyx, keyy
								  , keywidth, keyheight );
				AddFractions( AddFractions( &posx, &pKeySizing->KeySizingX )
								, &pKeySizing->KeySpacingX );
				;
				{
					//TEXTCHAR tmp[32];
					//sLogFraction( tmp, &posx );
					//lprintf( WIDE("NEW Position x: %s - %d %d"), tmp, keyx, keypad->width );
				}
			}
			if( keypad->flags.bDisplay )
			{
				AddFractions( AddFractions( &posy, &pKeySizing->KeyDisplaySizingY )
								, &pKeySizing->KeyDisplaySpacingY );
			}
			else
			{
				AddFractions( AddFractions( &posy, &pKeySizing->KeySizingY )
								, &pKeySizing->KeySpacingY );
			}
		}
	}
//cpg27dec2006 keypad\keypad.c(354): Warning! W107: Missing return value for function 'resize_keys'

   return 1;//cpg27dec2006
}


static int OnDrawCommon( WIDE( "Keypad Control 2" ) )( PSI_CONTROL frame )//CPROC KeypadDraw( PCOMMON frame )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, frame );
	//lprintf( WIDE("attempt Drawing blue keypad backgorund... on %p"), frame );
	if( keypad )
	{
		Image surface = GetControlSurface( frame );
		if( surface->width != keypad->width ||
			surface->height != keypad->height )
		{
			keypad->width = surface->width;
			keypad->height = surface->height;
			// need to resize the buttons hereupon.
			resize_keys( keypad );
         // and the display
		}
		// there is no border - so if we have a border this
		// needs to be drawn ourselves....
		//lprintf( WIDE("Drawing blue keypad backgorund...") );
      BlatColorAlpha( surface, 0, 0, surface->width, surface->height, keypad->background_color );
		//ClearImageTo( surface, Color( 0, 0, 64 ) );
		//UpdateControl( frame );
		return TRUE;
	}
   return FALSE;
}

static LOGICAL CPROC KeyboardHandler( PTRSZVAL psv
											, _32 key );
static void CPROC KeyPressed( PTRSZVAL psv, PKEY_BUTTON key );
static int new_flags;

#define NUM_KEYBOARD_KEYS ( sizeof( keyboard_keys ) / sizeof( keyboard_keys[0] ) )
static int keyboard_keys[] = {
   			KEY_0, 
			KEY_1, 
			KEY_2, 
			KEY_3, 
			KEY_4, 
			KEY_5, 
			KEY_6, 
			KEY_7, 
			KEY_8, 
			KEY_9, 
			KEY_A, 
			KEY_B, 
			KEY_C, 
			KEY_D, 
			KEY_E, 
			KEY_F, 
			KEY_G, 
			KEY_H, 
			KEY_I, 
			KEY_J, 
			KEY_K, 
			KEY_L, 
			KEY_M, 
			KEY_N, 
			KEY_O, 
			KEY_P, 
			KEY_Q, 
			KEY_R, 
			KEY_S, 
			KEY_T, 
			KEY_U, 
			KEY_V, 
			KEY_W, 
			KEY_X, 
			KEY_Y, 
			KEY_Z, 
			KEY_PERIOD,
			KEY_COMMA,
			KEY_SLASH,
			KEY_QUOTE,
			KEY_RIGHT_BRACKET,
			KEY_LEFT_BRACKET,

			KEY_BACKSLASH,
			KEY_BACKSPACE, 
			KEY_CAPS_LOCK, 
			KEY_DASH, 
			KEY_EQUAL, 
			KEY_SEMICOLON, 
			KEY_SPACE,
#ifdef WIN32
         VK_OEM_PLUS,
#endif
			KEY_ESCAPE,

};

#define NUM_KEYBOARD_KEYS2 ( sizeof( keyboard_keys2 ) / sizeof( keyboard_keys2[0] ) )
static int keyboard_keys2[] = {
	KEY_SHIFT,
#ifdef WIN32
			VK_LSHIFT,
			VK_RSHIFT,
#endif
 };


static int _InitKeypad( PSI_CONTROL frame )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, frame );
	PKEY_SPACE_SIZE pKeySizing;
	S_32 keyx, keyy, keywidth, keyheight;
	int row, col, rows, cols;
	FRACTION posy, posx, tmp;//, curposy;
	int w, h;
   //flags = new_flags; // remaining data is passed normally...

	keypad->frame = frame;
	keypad->display_background_color = Color( 129, 129, 149 );
	keypad->display_text_color = Color( 0, 0, 0 );
	keypad->background_color = AColor( 0, 0, 64, 150 );
	keypad->numkey_color = Color( 220, 220, 12 );
	keypad->numkey_text_color = Color( 0, 0, 0 );
	keypad->shiftkey_color = Color( 22, 43, 163 );
	keypad->shiftkey_text_color = Color( 0, 0, 0 );
	keypad->backspacekey_color = Color( 192, 93, 163 );
	keypad->backspacekey_text_color = Color( 0, 0, 0 );
	keypad->enterkey_color = Color( 32, 192, 32 );
	keypad->enterkey_text_color = Color( 0, 0, 0 );
	keypad->cancelkey_color = Color( 192, 75, 10 );
	keypad->cancelkey_text_color = Color( 0, 0, 0 );
	// this is the earliest that logging can take place (above here exist declarations...)
   //lprintf( WIDE("Making a keypad at %d,%d %d,%d"), x, w, w, h );
	LinkThing( keypads, keypad );

	{
		PRENDERER render = GetFrameRenderer( GetFrame( frame ) );
      //lprintf( "Bind Normal Keys... %p", render );
      BindEventToKey( render, KEY_PAD_0, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_1, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_2, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_3, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_4, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_5, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_6, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_7, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_8, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_9, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_INSERT, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_DELETE, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_DELETE, KEY_MOD_EXTENDED|KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_END, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_DOWN, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PGDN, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_LEFT, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_CENTER, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_RIGHT, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_HOME, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_UP, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PGUP, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_DELETE, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_ENTER, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_ENTER, KEY_MOD_EXTENDED|KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_MINUS, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
      BindEventToKey( render, KEY_PAD_DELETE, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_0, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_1, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_2, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_3, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_4, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_5, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_6, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_7, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_8, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		BindEventToKey( render, KEY_9, KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
		if( new_flags & KEYPAD_FLAG_ALPHANUM )
		{
			int n;
			for( n = 0; n < NUM_KEYBOARD_KEYS; n++ )
			{
            //lprintf( "Bind to key %02x %d", keyboard_keys[n], keyboard_keys[n] );
				BindEventToKey( render, keyboard_keys[n], 0, KeyboardHandler, (PTRSZVAL)frame );
				BindEventToKey( render, keyboard_keys[n], KEY_MOD_SHIFT, KeyboardHandler, (PTRSZVAL)frame );
			}
			for( n = 0; n < NUM_KEYBOARD_KEYS2; n++ )
			{
				BindEventToKey( render, keyboard_keys2[n], KEY_MOD_ALL_CHANGES, KeyboardHandler, (PTRSZVAL)frame );
				BindEventToKey( render, keyboard_keys2[n], KEY_MOD_ALL_CHANGES|KEY_MOD_SHIFT, KeyboardHandler, (PTRSZVAL)frame );
			}
			pKeySizing = &keyboard_sizing;
		}
		else
		{
			lprintf( WIDE("Just a keypad, not alphanum.") );
			pKeySizing = &keypad_sizing;
		}
	}
	// there will be one and only one keypad...
	{
		Image surface = GetControlSurface( frame );
		w = keypad->width = surface->width;
		h = keypad->height = surface->height;
	}
	keypad->flags.bDisplay = new_flags & KEYPAD_FLAG_DISPLAY;
	keypad->flags.bPassword = (new_flags & KEYPAD_FLAG_PASSWORD)!=0;
	keypad->flags.bEntry = (new_flags & KEYPAD_FLAG_ENTRY) != 0;
	keypad->flags.bAlphaNum = (new_flags & KEYPAD_FLAG_ALPHANUM) != 0;
	keypad->flags.bLeftJustify = (new_flags & KEYPAD_FLAG_DISPLAY_LEFT_JUSTIFY) != 0;
	keypad->flags.bCenterJustify = (new_flags & KEYPAD_FLAG_DISPLAY_CENTER_JUSTIFY) != 0;
	keypad->key_spacing = pKeySizing;
	keypad->accum = GetAccumulator( WIDE("Keypad Entry")
											, keypad->flags.bAlphaNum?ACCUM_TEXT
													 : (keypad->flags.bEntry?0:ACCUM_DOLLARS) );
	SetAccumulatorUpdateProc( keypad->accum, KeypadAccumUpdated, (PTRSZVAL)keypad );
	SetCommonTransparent( frame, TRUE );
	//AddCommonDraw( frame, KeypadDraw );

	keywidth = ReduceFraction( ScaleFraction( &tmp
														 , keypad->width
														 , &pKeySizing->KeySizingX ) );

	keypad->rows =
		rows = pKeySizing->rows;
	keypad->cols =
		cols = pKeySizing->cols;
	//rows = 4;
	//cols = 3;
	if( new_flags & KEYPAD_FLAG_DISPLAY )
	{
		keyheight = ReduceFraction( ScaleFraction( &tmp
															  , keypad->height
															  , &pKeySizing->KeyDisplaySizingY ) );
		posy = pKeySizing->DisplayOffsetY;
		AddFractions( &posy, &pKeySizing->DisplayY );
		AddFractions( &posy, &pKeySizing->KeyDisplaySpacingY );
	}
	else
	{
		keyheight = ReduceFraction( ScaleFraction( &tmp, h, &pKeySizing->KeySizingY ) );
		posy = pKeySizing->KeySpacingY;
	}

	//Log2( WIDE("Rendering Font sized: %ld, %ld"), keywidth / 2, keyheight / 2 );

	//lprintf( "Actually will be %d,%d", keywidth - keywidth/3, keyheight - keyheight/4 );

	keypad->font = RenderFontFile( WIDE("arialbd.ttf")
										  , keywidth - keywidth/3, keyheight - keyheight/4
										  , 3 );
   if( !keypad->font )
		keypad->font = RenderFontFile( WIDE("fonts/arialbd.ttf")
											  , keywidth - keywidth/3, keyheight - keyheight/4
											  , 3 );


	{
		int displayx, displayy, displaywidth, displayheight;
      displayx = ReduceFraction( ScaleFraction( &tmp, w, &pKeySizing->DisplayOffsetX ) );
      displayy = ReduceFraction( ScaleFraction( &tmp, h, &pKeySizing->DisplayOffsetY ) );
      displaywidth = ReduceFraction( ScaleFraction( &tmp, w, &pKeySizing->DisplayX ) );
		displayheight = ReduceFraction( ScaleFraction( &tmp, h, &pKeySizing->DisplayY ) );
		if( new_flags & KEYPAD_FLAG_DISPLAY )
		{
			keypad->display = MakeControl( frame
												  , keypad_display.TypeID
												  , displayx, displayy
												  , displaywidth, displayheight
												  , 1000
												  );
         SetDisplayPadKeypad( keypad->display, keypad );
		}
	}
    keypad->keys = NewArray( KEY_HOLDER,( keypad->nKeys = rows * cols ) );
	for( row = 0; row < rows; row++ )
	{
		keyy = ReduceFraction( ScaleFraction( &tmp, h, &posy ) );
		posx = pKeySizing->KeySpacingX;
		for( col = 0; col < cols; col++ )
		{
			keyx = ReduceFraction( ScaleFraction( &tmp, w, &posx ) );
			//{
			//	TEXTCHAR tmp[32];
         //   sLogFraction( tmp, &posx );
			//	lprintf( WIDE("Position x: %s - %d %d"), tmp, keyx, w );
         //   sLogFraction( tmp, &posy );
			//	lprintf( WIDE("Position y: %s - %d %d"), tmp, keyy, h );
			//}
			keypad->keys[row * cols + col].psv_key
				= keypad->flags.bAlphaNum
				? keyboardval[row*cols + col]
				: keyval[row * cols + col];
			keypad->keys[row * cols + col].psv_shifted_key
				= keypad->flags.bAlphaNum
				? keyboard_shifted_val[row*cols + col]
				: keyval[row * cols + col];

				keypad->keys[row * cols + col].key = MakeKeyEx( frame
																	, keyx, keyy
																	, keywidth, keyheight
																	, 0
																	, NULL //glare
																	, NULL // up
																	, NULL //down
																	, NULL //mask
																	, KEY_BACKGROUND_COLOR
																			 , BASE_COLOR_DARKBLUE

																	 // if flags is DISPLAY only (not password)
																	, (keypad->flags.bAlphaNum)?keyboardtext[row * cols + col]
																	 :(keypad->flags.bPassword)?entrytext[row * cols + col]
																	 :(keypad->flags.bEntry)?entrytext3[row * cols + col]
																	 :keytext[row * cols + col]
																	, keypad->font
																			 , KeyPressed
																			 , (PTRSZVAL)frame
																			 , (CTEXTSTR)(&keypad->keys[row * cols + col])
																			 );

				SetKeyShading( keypad->keys[row * cols + col].key,
								  (keypad->keys[row * cols + col].psv_key == (TEXTSTR)-1)
								  ?(keypad->flags.bAlphaNum?keypad->capskey_color:keypad->cancelkey_color)
								  :(keypad->keys[row * cols + col].psv_key == (TEXTSTR)-2)
								  ?keypad->enterkey_color
								  :(keypad->keys[row * cols + col].psv_key == (TEXTSTR)-3)
								  ?keypad->shiftkey_color
								  :(((CTEXTSTR)keypad->keys[row * cols + col].psv_key)[0] == '\x1b' )
								  ?keypad->cancelkey_color
								  :(((CTEXTSTR)keypad->keys[row * cols + col].psv_key)[0] == '\b' )
								  ?keypad->backspacekey_color
								  :keypad->numkey_color );
				AddFractions( AddFractions( &posx, &pKeySizing->KeySizingX )
							, &pKeySizing->KeySpacingX );
			;
			{
				//TEXTCHAR tmp[32];
            //sLogFraction( tmp, &posx );
				//Log3( WIDE("NEW Position x: %s - %d %d"), tmp, keyx, w );
			}
		}
		if( new_flags & KEYPAD_FLAG_DISPLAY )
		{
			AddFractions( AddFractions( &posy, &pKeySizing->KeyDisplaySizingY )
						  , &pKeySizing->KeyDisplaySpacingY );
		}
		else
		{
			AddFractions( AddFractions( &posy, &pKeySizing->KeySizingY )
						  , &pKeySizing->KeySpacingY );
		}
	}
   return TRUE;
}

int CPROC InitKeypad( PSI_CONTROL pc )
{
   return _InitKeypad( pc );
}

PRELOAD( KeypadDisplayRegister)
{
   DoRegisterControl( &keypad_display );
	DoRegisterControl( &keypad_control );
   flags.log_key_events = SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Widgets/Log key events for keypads"), 0 );
}

#if 0
#define BUFFER_SIZE 80

struct {
	TEXTCHAR *buffer;
	int used_chars;

}

void EnterKeyIntoBuffer( TEXTCHAR **ppBuffer, TEXTCHAR c )
{
	if( !ppBuffer )
		return;
	if( !(*ppBuffer ) )
	{
      (*ppBuffer) = Allocate( BUFFER_SIZE );
	}
}
#endif
static void CPROC KeyPressed( PTRSZVAL psv, PKEY_BUTTON key )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, pKeyPad, (PSI_CONTROL)psv );
	TEXTCHAR *string;
	int shift_changed = 0;
	PKEY_HOLDER holder = (PKEY_HOLDER)GetKeyValue( key );
	S_32 value;

	if( flags.log_key_events )
		lprintf( WIDE("Press Key %p"), key );

	string = (pKeyPad->shift_lock ^ pKeyPad->shifted)?(TEXTCHAR*)holder->psv_shifted_key:(TEXTCHAR*)holder->psv_key;

	{
		// trigger magic sequences (background process on keypads)
		switch( (int)string )
		{
		case 0:
			break;
		case -1:
			if( pKeyPad->flags.bAlphaNum )
				InvokeMagicSequences( pKeyPad->frame, "\\L" );  // Shift-Lock
			else
				InvokeMagicSequences( pKeyPad->frame, "\\C" );  // Clear/Correct
			break;
		case -2:
			InvokeMagicSequences( pKeyPad->frame, "\\E" );
			break;
		case -3: // shift
			if( pKeyPad->flags.bAlphaNum )
				InvokeMagicSequences( pKeyPad->frame, "\\S" );
			// there is no -3 on numeric pad.  (only 2 special keys)
			break;
		default:
			if( string[0] == '\\' )
				InvokeMagicSequences( pKeyPad->frame, "\\\\" );
			else
				InvokeMagicSequences( pKeyPad->frame, string );
			break;
		}
	}
	//lprintf( "keypressed %p %p", key, string );
	if( pKeyPad->flags.bAlphaNum )
	{
		if( string == (TEXTCHAR*)-3 )
		{
			pKeyPad->shifted = !pKeyPad->shifted;
			shift_changed = TRUE;
			//value = -3;
			// shift state..
		}
		else if( string == (TEXTCHAR*)-2 )
		{
			// wake waiting thread...
			pKeyPad->flags.bResult = 1;
			pKeyPad->flags.bResultStatus = 1;
			InvokeEnterEvent( (PSI_CONTROL)psv );
			if( pKeyPad->shifted )
			{
				shift_changed = TRUE;
				pKeyPad->shifted = 0;
			}
		}
		else if( string == (TEXTCHAR*)-1 )
		{
			pKeyPad->shift_lock = !pKeyPad->shift_lock;
			shift_changed = TRUE;
			// wake waiting thread...
			//pKeyPad->flags.bResult = 1;
			//pKeyPad->flags.bResultStatus = 0;
  			//InvokeEnterEvent( (PSI_CONTROL)psv );
			//shifted = 0;
		}
		else
		{
			if( string[0] == '\x1b' )
			{
				pKeyPad->flags.bResult = 1;
				pKeyPad->flags.bResultStatus = 0;
				InvokeCancelEvent( (PSI_CONTROL)psv );

			}
			else
			{
				if( flags.log_key_events )lprintf( WIDE("Add \'%s\'"), string );

				// if the shift key is still down.
				if( pKeyPad->want_shifted )
					pKeyPad->typed_with_shift = 1;

				KeyTextIntoAccumulator( pKeyPad->accum, string );

				// if shift key is now released...
				if( !pKeyPad->want_shifted )
				{
					// if we were in a shifted state...
					if( pKeyPad->shifted )
					{
						if( IsKeyDown( GetFrameRenderer( (PSI_CONTROL)psv ), KEY_SHIFT )
                     || IsKeyDown( GetFrameRenderer( (PSI_CONTROL)psv ), KEY_RIGHT_SHIFT )
							|| IsKeyDown( GetFrameRenderer( (PSI_CONTROL)psv ), KEY_LEFT_SHIFT ) )
						{
						}
						else
						{
							shift_changed = TRUE;
							pKeyPad->shifted = 0;
						}
					}
				}
			}
		}

		if( shift_changed )
		{
			int shift_state = pKeyPad->shift_lock ^ pKeyPad->shifted;
			int r, c;
			EnableCommonUpdates( (PSI_CONTROL)psv, 0 );
			for( r = 0; r < pKeyPad->rows; r++ )
				for( c = 0; c < pKeyPad->cols; c++ )
				{
					int n = r * pKeyPad->cols + c;
					SetKeyText( pKeyPad->keys[n].key
								 , shift_state?keyboard_shifted_text[n]:keyboardtext[n] );
				}
			EnableCommonUpdates( (PSI_CONTROL)psv, 1 );
			SmudgeCommon( (PSI_CONTROL)psv );
		}

	}
	else
	{
		if( string == (TEXTCHAR*)-3 )
			value = -3;
		else if( string == (TEXTCHAR*)-2 )
			value = -2;
		else if( string == (TEXTCHAR*)-1 )
			value = -1;
		else
			value = (S_32)IntCreateFromText( string );

		if( pKeyPad->flags.bPassword || !pKeyPad->flags.bDisplay || pKeyPad->flags.bEntry )
		{
			if( value == -1 )
			{
				if( pKeyPad->flags.bEntry )
				{
					if( pKeyPad->flags.bPassword )
						ClearAccumulatorDigit( pKeyPad->accum, 10 );
					else
						ClearAccumulator( pKeyPad->accum );
				}
				else
				{
					pKeyPad->flags.bResult = 1;
					pKeyPad->flags.bResultStatus = 0;
				}
			}
			else if( value == -2 )
			{
				InvokeEnterEvent( (PSI_CONTROL)psv );
				pKeyPad->flags.bResult = 1;
				pKeyPad->flags.bResultStatus = 1;
			}
			else
			{
				if( pKeyPad->flags.bClearOnNext )
				{
					pKeyPad->flags.bClearOnNext = 0;
					pKeyPad->flags.bResult = 0;
					pKeyPad->flags.bResultStatus = 0;
					ClearAccumulator( pKeyPad->accum );
				}
				KeyIntoAccumulator( pKeyPad->accum, value, 10 );
			}
		}
		else
		{
			if( value == -1 )
			{
				if( pKeyPad->flags.bClearOnNext )
				{
					pKeyPad->flags.bClearOnNext = 0;
					pKeyPad->flags.bResult = 0;
					pKeyPad->flags.bResultStatus = 0;
					ClearAccumulator( pKeyPad->accum );
				}
				ClearAccumulatorDigit( pKeyPad->accum, 10 );
			}
			else if( value == -2 )
			{
            // double-zero
				if( pKeyPad->flags.bClearOnNext )
				{
					pKeyPad->flags.bClearOnNext = 0;
					pKeyPad->flags.bResult = 0;
					pKeyPad->flags.bResultStatus = 0;
					ClearAccumulator( pKeyPad->accum );
				}
			
				// double-zero
				KeyIntoAccumulator( pKeyPad->accum, 0, 10 );
				KeyIntoAccumulator( pKeyPad->accum, 0, 10 );
			}
			else
			{
				if( pKeyPad->flags.bClearOnNext )
				{
					pKeyPad->flags.bClearOnNext = 0;
					pKeyPad->flags.bResult = 0;
					pKeyPad->flags.bResultStatus = 0;
					ClearAccumulator( pKeyPad->accum );
				}
				KeyIntoAccumulator( pKeyPad->accum, value, 10 );
			}
		}
	}
}

KEYPAD_PROC( void, KeyIntoKeypad )( PSI_CONTROL pc, S_64 value )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, pKeyPad, pc );
	if( pKeyPad )
	{
		ClearAccumulator( pKeyPad->accum );
		KeyIntoAccumulator( pKeyPad->accum, (S_32)value, 10 );
		InvokeEnterEvent( pc );
	}
}

KEYPAD_PROC( void, KeyIntoKeypadNoEnter )( PSI_CONTROL pc, _64 value )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, pKeyPad, pc );
	if( pKeyPad )
	{
		ClearAccumulator( pKeyPad->accum );
		KeyIntoAccumulator( pKeyPad->accum, value, 10 );
	}
}

KEYPAD_PROC( void, KeypadInvertValue )( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, pKeyPad, pc );
	if( pKeyPad )
	{
		SetAccumulator( pKeyPad->accum, -GetAccumulatorValue( pKeyPad->accum ) );
	}
}


static LOGICAL CPROC KeyboardHandler( PTRSZVAL psv
											, _32 key )
{
	PKEY_BUTTON button = NULL;
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, pKeyPad, (PSI_CONTROL)psv );
	if( flags.log_key_events )
		lprintf( WIDE("Key trigger : %d"), KEY_CODE(key) );

	if( IsControlHidden( (PSI_CONTROL)psv ) )
		return FALSE;

	if( IsKeyPressed( key ) && pKeyPad )
	{
		if( flags.log_key_events )
			lprintf( WIDE("press and .. %d"),pKeyPad->flags.bAlphaNum );
      //lprintf( "Key Trigger" );
		if( !pKeyPad->flags.bAlphaNum )
		{
			switch( KEY_CODE(key) )
			{
			case KEY_0:
			case KEY_PAD_0:
			case KEY_INSERT:
				KeyPressed( psv, button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 1].key );
				break;
			case KEY_1:
			case KEY_END:
			case KEY_PAD_1:
				if( pKeyPad->style & KEYPAD_INVERT )
					KeyPressed( psv, button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 0].key );
				else
					KeyPressed( psv, button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 0].key );
				break;
			case KEY_2:
			case KEY_DOWN:
			case KEY_PAD_2:
				if( pKeyPad->style & KEYPAD_INVERT )
					KeyPressed( psv, button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 1].key );
				else
					KeyPressed( psv, button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 1].key );
				break;
			case KEY_3:
			case KEY_PGDN:
			case KEY_PAD_3:
				if( pKeyPad->style & KEYPAD_INVERT )
					KeyPressed( psv, button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 2].key );
				else
					KeyPressed( psv, button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 2].key );
				break;
			case KEY_4:
			case KEY_LEFT:
			case KEY_PAD_4:
				KeyPressed( psv, button = pKeyPad->keys[1 * pKeyPad->key_spacing->cols + 0].key );
				break;
			case KEY_5:
			case KEY_CENTER:
			case KEY_PAD_5:
				KeyPressed( psv, button = pKeyPad->keys[1 * pKeyPad->key_spacing->cols + 1].key );
				break;
			case KEY_6:
			case KEY_RIGHT:
			case KEY_PAD_6:
				KeyPressed( psv, button = pKeyPad->keys[1 * pKeyPad->key_spacing->cols + 2].key );
				break;
			case KEY_7:
			case KEY_HOME:
			case KEY_PAD_7:
				if( pKeyPad->style & KEYPAD_INVERT )
					KeyPressed( psv, button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 0].key );
				else
					KeyPressed( psv, button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 0].key );
				break;
			case KEY_8:
			case KEY_UP:
			case KEY_PAD_8:
				if( pKeyPad->style & KEYPAD_INVERT )
					KeyPressed( psv, button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 1].key );
				else
					KeyPressed( psv, button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 1].key );
				break;
			case KEY_9:
			case KEY_PGUP:
			case KEY_PAD_9:
				if( pKeyPad->style & KEYPAD_INVERT )
					KeyPressed( psv, button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 2].key );
				else
					KeyPressed( psv, button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 2].key );
				break;
				//case KEY_ENTER:
			case KEY_PAD_ENTER:
				KeyPressed( psv, button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 2].key );
				break;
			case KEY_PAD_DELETE:
			case KEY_DELETE:
				KeyPressed( psv, button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 0].key );
				break;
			case KEY_PAD_MINUS:
				KeyPressed( psv, button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 0].key );
				break;
			}
		}
		else{
			if( flags.log_key_events )
				lprintf( WIDE("IS alphanum... %lx"), key );
			switch( KEY_CODE(key) )
			{
			case KEY_BACKSPACE:
				KeyPressed( psv, button = pKeyPad->keys[51].key );
				break;
			case KEY_SHIFT:
#ifdef WIN32
			case VK_LSHIFT:
			case VK_RSHIFT:
#endif
            // was in a released state...
				if( !pKeyPad->want_shifted )
				{
					pKeyPad->typed_with_shift = FALSE;
					pKeyPad->want_shifted = TRUE;
					KeyPressed( psv, button = pKeyPad->keys[39].key );
				}
				break;
			case KEY_BACKSLASH:
				KeyPressed( psv, button = pKeyPad->keys[13].key );
				break;
			case KEY_LEFT_BRACKET:
				KeyPressed( psv, button = pKeyPad->keys[24].key );
				break;
			case KEY_RIGHT_BRACKET:
				KeyPressed( psv, button = pKeyPad->keys[25].key );
				break;
			case KEY_CAPS_LOCK:
				KeyPressed( psv, button = pKeyPad->keys[26].key );
				break;
			case KEY_Q:
				KeyPressed( psv, button = pKeyPad->keys[14].key );
				break;
			case KEY_W:
				KeyPressed( psv, button = pKeyPad->keys[15].key );
				break;
			case KEY_E:
				KeyPressed( psv, button = pKeyPad->keys[16].key );
				break;
			case KEY_R:
				KeyPressed( psv, button = pKeyPad->keys[17].key );
				break;
			case KEY_T:
				KeyPressed( psv, button = pKeyPad->keys[18].key );
				break;
			case KEY_Y:
				KeyPressed( psv, button = pKeyPad->keys[19].key );
				break;
			case KEY_U:
				KeyPressed( psv, button = pKeyPad->keys[20].key );
				break;
			case KEY_I:
				KeyPressed( psv, button = pKeyPad->keys[21].key );
				break;
			case KEY_O:
				KeyPressed( psv, button = pKeyPad->keys[22].key );
				break;
			case KEY_P:
				KeyPressed( psv, button = pKeyPad->keys[23].key );
				break;
			case KEY_A:
				KeyPressed( psv, button = pKeyPad->keys[27].key );
				break;
			case KEY_S:
				KeyPressed( psv, button = pKeyPad->keys[28].key );
				break;
			case KEY_D:
				KeyPressed( psv, button = pKeyPad->keys[29].key );
				break;
			case KEY_F:
				KeyPressed( psv, button = pKeyPad->keys[30].key );
				break;
			case KEY_G:
				KeyPressed( psv, button = pKeyPad->keys[31].key );
				break;
			case KEY_H:
				KeyPressed( psv, button = pKeyPad->keys[32].key );
				break;
			case KEY_J:
				KeyPressed( psv, button = pKeyPad->keys[33].key );
				break;
			case KEY_K:
				KeyPressed( psv, button = pKeyPad->keys[34].key );
				break;
			case KEY_L:
				KeyPressed( psv, button = pKeyPad->keys[35].key );
				break;

			case KEY_SEMICOLON:
				KeyPressed( psv, button = pKeyPad->keys[36].key );
				break;
			case KEY_QUOTE:
				KeyPressed( psv, button = pKeyPad->keys[37].key );
				break;

			case KEY_Z:
				KeyPressed( psv, button = pKeyPad->keys[40].key );
				break;
			case KEY_X:
				KeyPressed( psv, button = pKeyPad->keys[41].key );
				break;
			case KEY_C:
				KeyPressed( psv, button = pKeyPad->keys[42].key );
				break;
			case KEY_V:
				KeyPressed( psv, button = pKeyPad->keys[43].key );
				break;
			case KEY_B:
				KeyPressed( psv, button = pKeyPad->keys[44].key );
				break;
			case KEY_N:
				KeyPressed( psv, button = pKeyPad->keys[45].key );
				break;
			case KEY_M:
				KeyPressed( psv, button = pKeyPad->keys[46].key );
				break;
			case KEY_SPACE:
				KeyPressed( psv, button = pKeyPad->keys[47].key );
				break;
			case KEY_COMMA:
				KeyPressed( psv, button = pKeyPad->keys[48].key );
				break;
			case KEY_PERIOD:
				KeyPressed( psv, button = pKeyPad->keys[49].key );
				break;
			case KEY_SLASH:
				KeyPressed( psv, button = pKeyPad->keys[50].key );
				break;
			case KEY_ESCAPE:
				KeyPressed( psv, button = pKeyPad->keys[0].key );
				break;
			case KEY_1:
			case KEY_END:
			case KEY_PAD_1:
				KeyPressed( psv, button = pKeyPad->keys[1].key );
				break;
			case KEY_2:
			case KEY_DOWN:
			case KEY_PAD_2:
				KeyPressed( psv, button = pKeyPad->keys[2].key );
				break;
			case KEY_3:
			case KEY_PGDN:
			case KEY_PAD_3:
				KeyPressed( psv, button = pKeyPad->keys[3].key );
				break;
			case KEY_4:
			case KEY_LEFT:
			case KEY_PAD_4:
				KeyPressed( psv, button = pKeyPad->keys[4].key );
				break;
			case KEY_5:
			case KEY_CENTER:
			case KEY_PAD_5:
				KeyPressed( psv, button = pKeyPad->keys[5].key );
				break;
			case KEY_6:
			case KEY_RIGHT:
			case KEY_PAD_6:
				KeyPressed( psv, button = pKeyPad->keys[6].key );
				break;
			case KEY_7:
			case KEY_HOME:
			case KEY_PAD_7:
				KeyPressed( psv, button = pKeyPad->keys[7].key );
				break;
			case KEY_8:
			case KEY_UP:
			case KEY_PAD_8:
				KeyPressed( psv, button = pKeyPad->keys[8].key );
				break;
			case KEY_9:
			case KEY_PGUP:
			case KEY_PAD_9:
				KeyPressed( psv, button = pKeyPad->keys[9].key );
				break;
			case KEY_0:
			case KEY_PAD_0:
			case KEY_INSERT:
				KeyPressed( psv, button = pKeyPad->keys[10].key );
				break;

			case KEY_DASH:
				KeyPressed( psv, button = pKeyPad->keys[11].key );
				break;
#ifdef WIN32
			case VK_OEM_PLUS:
#endif
			case KEY_EQUAL:
				KeyPressed( psv, button = pKeyPad->keys[12].key );
				break;
				//case KEY_ENTER:
			case KEY_PAD_ENTER:
				KeyPressed( psv, button = pKeyPad->keys[38].key );
				break;
			case KEY_PAD_DELETE:
			case KEY_DELETE:
				KeyPressed( psv, button = pKeyPad->keys[39+3].key );
				break;
			case KEY_PAD_MINUS:
				//KeyPressed( psv, button = pKeyPad->keys[].key );
				break;
			}
		}
		{
			PCOMMON pc = GetKeyCommon( button );
			PressButton( pc, TRUE );
		}
	}
	else if( pKeyPad )// key is being released...
	{
		if( flags.log_key_events )
			lprintf( WIDE("Key release %d"), KEY_CODE(key) )
				;
      switch( KEY_CODE(key) )
		{
		case KEY_SHIFT:
#ifdef WIN32
		case VK_LSHIFT:
		case VK_RSHIFT:
#endif
			pKeyPad->want_shifted = FALSE;
			if( pKeyPad->typed_with_shift )
			{
				if( pKeyPad->shifted )
					KeyPressed( psv, button = pKeyPad->keys[39].key );
			}
			else
			{
				//lprintf( "Was just a shift toggle, don't shift now?" );
            /*
				if( pKeyPad->shifted )
				{
               lprintf( "was shifted?" );
					KeyPressed( psv, button = pKeyPad->keys[39].key );
				}
				else
				lprintf( "wasn't shifted?" );
            */
			}
         break;
		case KEY_0:
		case KEY_PAD_0:
		case KEY_INSERT:
			button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 1].key;
			break;
		case KEY_1:
		case KEY_END:
		case KEY_PAD_1:
			if( pKeyPad->style & KEYPAD_INVERT )
				button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 0].key;
			else
				button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 0].key;
			break;
		case KEY_2:
		case KEY_DOWN:
		case KEY_PAD_2:
			if( pKeyPad->style & KEYPAD_INVERT )
				button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 1].key;
			else
				button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 1].key;
			break;
		case KEY_3:
		case KEY_PGDN:
		case KEY_PAD_3:
			if( pKeyPad->style & KEYPAD_INVERT )
				button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 2].key;
			else
				button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 2].key;
			break;
		case KEY_4:
		case KEY_LEFT:
		case KEY_PAD_4:
			button = pKeyPad->keys[1 * pKeyPad->key_spacing->cols + 0].key;
			break;
		case KEY_5:
		case KEY_CENTER:
		case KEY_PAD_5:
			button = pKeyPad->keys[1 * pKeyPad->key_spacing->cols + 1].key;
			break;
		case KEY_6:
		case KEY_RIGHT:
		case KEY_PAD_6:
			button = pKeyPad->keys[1 * pKeyPad->key_spacing->cols + 2].key;
			break;
		case KEY_7:
		case KEY_HOME:
		case KEY_PAD_7:
			if( pKeyPad->style & KEYPAD_INVERT )
				button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 0].key;
			else
				button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 0].key;
			break;
		case KEY_8:
		case KEY_UP:
		case KEY_PAD_8:
			if( pKeyPad->style & KEYPAD_INVERT )
				button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 1].key;
			else
				button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 1].key;
			break;
		case KEY_9:
		case KEY_PGUP:
		case KEY_PAD_9:
			if( pKeyPad->style & KEYPAD_INVERT )
				button = pKeyPad->keys[2 * pKeyPad->key_spacing->cols + 2].key;
			else
				button = pKeyPad->keys[0 * pKeyPad->key_spacing->cols + 2].key;
			break;
		//case KEY_ENTER:
		case KEY_PAD_ENTER:
			button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 2].key;
			//InvokeEnterEvent( (PSI_CONTROL)psv );
			break;
		case KEY_PAD_DELETE:
		case KEY_DELETE:
			button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 0].key;
			break;
		case KEY_PAD_MINUS:
			button = pKeyPad->keys[3 * pKeyPad->key_spacing->cols + 0].key;
			break;
		}
		{
			PCOMMON pc = GetKeyCommon( button );
			PressButton( pc, FALSE );
		}
	}
   return 1;
}

void SetDisplayPadKeypad( PSI_CONTROL pc, PKEYPAD keypad )
{
	ValidatedControlData( PDISPLAY, keypad_display.TypeID, display, pc );
	if( display )
	{
		display->keypad = keypad;
		if( keypad )
		{
			if( keypad->flags.bAlphaNum )
			{
				DestroyFont( &display->font );
				display->font = RenderFontFile( WIDE("arial.ttf")
														, (display->height - 5) * 8 / 8
														, display->height - 5
														, 1 );
				if( !display->font )
					display->font = RenderFontFile( WIDE("fonts/arial.ttf")
															, (display->height - 5) * 8 / 8
															, display->height - 5
															, 1 );

			}
		}
	}
}


void SetNewKeypadFlags( int flags )
{
	new_flags = flags;
}

PSI_CONTROL MakeKeypad( PCOMMON parent
							 , S_32 x, S_32 y, _32 w, _32 h
							  // show current value display...
							 , _32 ID
							 , _32 flags
                       , CTEXTSTR accumulator_name
							 )
{
	PSI_CONTROL frame;
	new_flags = flags; // remaining data is passed normally...
	frame = MakeControl( parent, keypad_control.TypeID, x, y, w, h, ID );
	{
		ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, frame );
		if( keypad && accumulator_name )
		{
			keypad->accum = GetAccumulator( accumulator_name
													, keypad->flags.bAlphaNum?ACCUM_TEXT
													 :(keypad->flags.bEntry?0:ACCUM_DOLLARS)
													);
			SetAccumulatorUpdateProc( keypad->accum, KeypadAccumUpdated, (PTRSZVAL)keypad );
		}
	}
   return frame;
}


static void OnHideCommon( WIDE("Keypad Control 2") )( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->flags.bHidden = 1;
		keypad->flags.bResult = 1;
		keypad->flags.bResultStatus = 0;
	}
}

static void OnRevealCommon( WIDE("Keypad Control 2") )( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->flags.bHidden = 0;
		keypad->flags.bResult = 0;
	}
}


int GetKeyedText( PSI_CONTROL pc, TEXTSTR buffer, int buflen )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		return (int)GetAccumulatorText( keypad->accum, buffer, buflen );
	}
   return 0;
}

S_64 GetKeyedValue( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		return GetAccumulatorValue( keypad->accum );
	}
   return 0;
}

void ClearKeyedEntry( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->flags.bResult = 0;
		keypad->flags.bResultStatus = 0;
		ClearAccumulator( keypad->accum );
	}
}

void ClearKeyedEntryOnNextKey( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->flags.bClearOnNext = 1;
	}
}

void CancelKeypadWait( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
	{
		keypad->flags.bResult = 1;
		keypad->flags.bResultStatus = 0;
	}
}

int WaitForKeypadResult( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad->flags.bPassword || !keypad->flags.bDisplay || keypad->flags.bEntry )
	{
		while( !keypad->flags.bResult )
			if( !Idle() )
				WakeableSleep( 100 );
		keypad->flags.bResult = 0;
		return keypad->flags.bResultStatus;
	}
	return -1;
}

void CPROC HotkeyPressed( PTRSZVAL psv, PKEY_BUTTON button )
{
}

LOGICAL GetKeypadGoClear( PSI_CONTROL pc )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc );
	if( keypad )
		return keypad->flags.bGoClear;
   return 0;
}

// so... lets allow a menu button external control...

// okay this isn't enough either... it'll be way too hard to configure (without evomenu framework)
PSI_CONTROL MakeKeypadHotkey( PSI_CONTROL frame
										, S_32 x
										, S_32 y
										, _32 w
										, _32 h
										, TEXTCHAR *keypad
									 )
{
   PKEY_BUTTON hotkey;
	hotkey = MakeKeyEx( frame
							, x, y
							, w, h
							, 0
							, NULL //glare
							, NULL // up
							, NULL //down
							, NULL //mask
							, KEY_BACKGROUND_COLOR
							, Color( 220, 220, 12 )
							 // if flags is DISPLAY only (not password)
							, WIDE("") // key text
							, NULL
							, HotkeyPressed, 0
							, WIDE("") // value
							);

	return NULL;
}


CDATA KeypadGetDisplayBackground( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->display_background_color;
	return 0;
}
CDATA KeypadGetBackgroundColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->background_color;
	return 0;
}
CDATA KeypadGetDisplayTextColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->display_text_color;
	return 0;
}
CDATA KeypadGetNumberKeyColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->numkey_color;
	return 0;
}
CDATA KeypadGetEnterKeyColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->enterkey_color;
	return 0;
}
CDATA KeypadGetC_KeyColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->cancelkey_color;
	return 0;
}
CDATA KeypadGetNumberKeyTextColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->numkey_text_color;
	return 0;
}
CDATA KeypadGetEnterKeyTextColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->enterkey_text_color;
	return 0;
}
CDATA KeypadGetC_KeyTextColor( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
		return keypad->cancelkey_text_color;
	return 0;
}
void KeypadSetDisplayBackground( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		keypad->display_background_color = color;
	}
}
void KeypadSetBackgroundColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		keypad->background_color = color;
	}
}
void KeypadSetDisplayTextColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		keypad->display_text_color = color;
	}
}
void KeypadSetNumberKeyColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		int row, col;
      PKEY_HOLDER holder;
		keypad->numkey_color = color;

		for( row = 0; row < keypad->rows; row++ )
			for( col = 0; col < keypad->cols; col++ )
			{
				int shift_state = keypad->shift_lock ^ keypad->shifted;
            CTEXTSTR value;
				holder = keypad->keys + ( row * keypad->cols + col );
            value = shift_state?(CTEXTSTR)holder->psv_shifted_key:(CTEXTSTR)holder->psv_key;
				//keypad->keys[row * cols + col]
				if( value == (CTEXTSTR)-1 )
					continue;
				if( value == (CTEXTSTR)-2 )
					continue;
				SetKeyColor( holder->key, color );
			}
	}
}
void KeypadSetEnterKeyColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		int row, col;
		keypad->enterkey_color = color;
		for( row = 0; row < keypad->rows; row++ )
			for( col = 0; col < keypad->cols; col++ )
			{
				int shift_state = keypad->shift_lock ^ keypad->shifted;
				CTEXTSTR value;
            PKEY_HOLDER holder = keypad->keys + ( row * keypad->cols + col );
            value = shift_state?(CTEXTSTR)holder->psv_shifted_key:(CTEXTSTR)holder->psv_key;
				//keypad->keys[row * cols + col]
				if( value == (CTEXTSTR)-1 )
					continue;
				if( value == (CTEXTSTR)-2 )
					SetKeyColor( holder->key, color );
				else
					;//SetKeyColor( keypad->keys[row * cols + col], color );
			}
	}
}
void KeypadSetC_KeyColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		int row, col;
		keypad->cancelkey_color = color;
		for( row = 0; row < keypad->rows; row++ )
			for( col = 0; col < keypad->cols; col++ )
			{
				int shift_state = keypad->shift_lock ^ keypad->shifted;
            CTEXTSTR value;
				PKEY_HOLDER holder = keypad->keys + ( row * keypad->cols + col );
            value = shift_state?(CTEXTSTR)holder->psv_shifted_key:(CTEXTSTR)holder->psv_key;
				//keypad->keys[row * cols + col]
				if( value == (CTEXTSTR)-1 )
					SetKeyColor( holder->key, color );
				if( value == (CTEXTSTR)-2 )
					continue;
				else
					;//SetKeyColor( keypad->keys[row * cols + col], color );
			}
	}
}
void KeypadSetNumberKeyTextColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
      int row, col;
		keypad->numkey_text_color = color;

		for( row = 0; row < keypad->rows; row++ )
			for( col = 0; col < keypad->cols; col++ )
			{
				int shift_state = keypad->shift_lock ^ keypad->shifted;
            CTEXTSTR value;
				PKEY_HOLDER holder = keypad->keys + ( row * keypad->cols + col );
            value = shift_state?(CTEXTSTR)holder->psv_shifted_key:(CTEXTSTR)holder->psv_key;
				//keypad->keys[row * cols + col]
				if( value == (CTEXTSTR)-1 )
					continue;
				if( value == (CTEXTSTR)-2 )
					continue;
				SetKeyTextColor( holder->key, color );
			}
	}
}
void KeypadSetEnterKeyTextColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		int row, col;
		keypad->enterkey_text_color = color;
		for( row = 0; row < keypad->rows; row++ )
			for( col = 0; col < keypad->cols; col++ )
			{
				int shift_state = keypad->shift_lock ^ keypad->shifted;
            CTEXTSTR value;
				PKEY_HOLDER holder = keypad->keys + ( row * keypad->cols + col );
            value = shift_state?(CTEXTSTR)holder->psv_shifted_key:(CTEXTSTR)holder->psv_key;
				//keypad->keys[row * cols + col]
				if( value == (CTEXTSTR)-1 )
					continue;
				if( value == (CTEXTSTR)-2 )
					SetKeyTextColor( holder->key, color );
				else
					;//SetKeyColor( keypad->keys[row * cols + col], color );
			}
	}
}
void KeypadSetC_KeyTextColor( PSI_CONTROL pc_keypad, CDATA color )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		int row, col;
		keypad->cancelkey_text_color = color;
		for( row = 0; row < keypad->rows; row++ )
			for( col = 0; col < keypad->cols; col++ )
			{
				int shift_state = keypad->shift_lock ^ keypad->shifted;
            CTEXTSTR value;
				PKEY_HOLDER holder = keypad->keys + ( row * keypad->cols + col );
            value = shift_state?(CTEXTSTR)holder->psv_shifted_key:(CTEXTSTR)holder->psv_key;
				//keypad->keys[row * cols + col]
				if( value == (CTEXTSTR)-1 )
					SetKeyTextColor( holder->key, color );
				if( value == (CTEXTSTR)-2 )
					continue;
				else
					;//SetKeyColor( keypad->keys[row * cols + col], color );
			}
	}
}

void KeypadWriteConfig( FILE *file, PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	fprintf( file, WIDE( "%skeypad background=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->background_color ) );
	fprintf( file, WIDE( "%skeypad display background=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->display_background_color ) );
	fprintf( file, WIDE( "%skeypad display text color=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->display_text_color ) );
	fprintf( file, WIDE( "%skeypad color numkey=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->numkey_color ) );
	fprintf( file, WIDE( "%skeypad color enterkey=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->enterkey_color ) );
	fprintf( file, WIDE( "%skeypad color cancelkey=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->cancelkey_color ) );
	fprintf( file, WIDE( "%skeypad color numkey text=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->numkey_text_color ) );
	fprintf( file, WIDE( "%skeypad color enterkey text=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->enterkey_text_color ) );
	fprintf( file, WIDE( "%skeypad color cancelkey text=%s\n" ), InterShell_GetSaveIndent(), FormatColor( keypad->cancelkey_text_color ) );
	fprintf( file, WIDE( "%skeypad style=%ld\n" ), InterShell_GetSaveIndent(), keypad->style );
	if( keypad->display_format )
	{
		TEXTCHAR outbuf[256];
		ExpandConfigString( outbuf, keypad->display_format );
		fprintf( file, WIDE( "%skeypad formatting='%s'\n" ), InterShell_GetSaveIndent(), outbuf );
	}
}

static PTRSZVAL *ppsv;

static PTRSZVAL CPROC SetDisplayBackgroundColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
   KeypadSetDisplayBackground( (PSI_CONTROL)(*ppsv), color );
	return psv;
}

static PTRSZVAL CPROC SetDisplayTextColor( PTRSZVAL psv, arg_list args )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, (PSI_CONTROL)(*ppsv) );
	PARAM( args, CDATA, color );
	keypad->display_text_color = color;
	return psv;
}

static PTRSZVAL CPROC SetNumkeyColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
   KeypadSetNumberKeyColor( (PSI_CONTROL)(*ppsv), color );
	return psv;
}

static PTRSZVAL CPROC SetEnterKeyColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
   KeypadSetEnterKeyColor( (PSI_CONTROL)(*ppsv), color );
	return psv;
}

static PTRSZVAL CPROC SetCancelKeyColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
   KeypadSetC_KeyColor( (PSI_CONTROL)(*ppsv), color );
	return psv;
}
static PTRSZVAL CPROC SetNumkeyTextColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
   KeypadSetNumberKeyTextColor( (PSI_CONTROL)(*ppsv), color );
	return psv;
}

static PTRSZVAL CPROC SetEnterKeyTextColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
   KeypadSetEnterKeyTextColor( (PSI_CONTROL)(*ppsv), color );
	return psv;
}

static PTRSZVAL CPROC SetCancelKeyTextColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CDATA, color );
	KeypadSetC_KeyTextColor( (PSI_CONTROL)(*ppsv), color );
	return psv;
}

static PTRSZVAL CPROC SetBackgroundColor( PTRSZVAL psv, arg_list args )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, (PSI_CONTROL)(*ppsv) );
	PARAM( args, CDATA, color );
	keypad->background_color = color;
	return psv;
}

static PTRSZVAL CPROC SetKeypadStyleConfig( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, style );
   SetKeypadStyle( (PSI_CONTROL)(*ppsv), style );
   return psv;
}

static PTRSZVAL CPROC SetKeypadFormatConfig( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, format );
   KeypadSetDisplayFormat( (PSI_CONTROL)(*ppsv), format );
   return psv;
}


void KeypadSetupConfig( PCONFIG_HANDLER pch, PTRSZVAL *psv )
{
	ppsv = psv;
	AddConfigurationMethod( pch, WIDE( "keypad background=%c" ), SetBackgroundColor );
	AddConfigurationMethod( pch, WIDE( "keypad display background=%c" ), SetDisplayBackgroundColor );
	AddConfigurationMethod( pch, WIDE( "keypad display text color=%c" ), SetDisplayTextColor );
	AddConfigurationMethod( pch, WIDE( "keypad color numkey=%c" ), SetNumkeyColor );
	AddConfigurationMethod( pch, WIDE( "keypad color enterkey=%c" ), SetEnterKeyColor );
	AddConfigurationMethod( pch, WIDE( "keypad color cancelkey=%c" ), SetCancelKeyColor );
	AddConfigurationMethod( pch, WIDE( "keypad color numkey text=%c" ), SetNumkeyTextColor );
	AddConfigurationMethod( pch, WIDE( "keypad color enterkey text=%c" ), SetEnterKeyTextColor );
	AddConfigurationMethod( pch, WIDE( "keypad color cancelkey text=%c" ), SetCancelKeyTextColor );
	AddConfigurationMethod( pch, WIDE( "keypad style=%i" ), SetKeypadStyleConfig );
	AddConfigurationMethod( pch, WIDE( "keypad formatting='%m'" ), SetKeypadFormatConfig );
}

static void OnThemeChanged( WIDE( "Keypad 2 Control" ) )( int theme_id )
{
	PKEYPAD keypad;

	for( keypad = keypads; keypad; keypad = NextThing( keypad ) )
	{
		struct key_image_data
		{
			Image mask;
			Image glare;
			Image down;
			Image up;
			int use_color;
			CDATA color;
			CDATA text_color;
		} image_data[4];

		image_data[0].color = keypad->numkey_color;
		image_data[1].color = keypad->enterkey_color;
		image_data[2].color = keypad->cancelkey_color;
		image_data[3].color = keypad->numkey_color;

		GetKeyLenses( WIDE("Keypad Data"), theme_id
						, &image_data[0].use_color
						, &image_data[0].color
						, &image_data[0].text_color
						, &image_data[0].glare
						, &image_data[0].down
						, &image_data[0].up
						, &image_data[0].mask
						);

		GetKeyLenses( WIDE("Keypad Shift"), theme_id
						, &image_data[3].use_color
						, &image_data[3].color
						, &image_data[3].text_color
						, &image_data[3].glare
						, &image_data[3].down
						, &image_data[3].up
						, &image_data[3].mask
						);
		GetKeyLenses( WIDE("Keypad Confirm"), theme_id
						, &image_data[1].use_color
						, &image_data[1].color
						, &image_data[1].text_color
						, &image_data[1].glare
						, &image_data[1].down
						, &image_data[1].up
						, &image_data[1].mask
						);
		GetKeyLenses( WIDE("Keypad Cancel"), theme_id
						, &image_data[2].use_color
						, &image_data[2].color
						, &image_data[2].text_color
						, &image_data[2].glare
						, &image_data[2].down
						, &image_data[2].up
						, &image_data[2].mask
						);

		{
			int row, col;
			for( row = 0; row < keypad->rows; row++ )
			{
				for( col = 0; col < keypad->cols; col++ )
				{
               PKEY_HOLDER holder = &keypad->keys[row * keypad->cols + col];
					CTEXTSTR psv = holder->psv_key;
					if( psv == (CTEXTSTR)-1 )
					{
						SetKeyLenses( holder->key
										, image_data[2].glare
										, image_data[2].down
										, image_data[2].up
										, image_data[2].mask
										);
						if( image_data[2].use_color )
						{
							SetKeyColor( holder->key, image_data[2].color );
							SetKeyTextColor( holder->key, image_data[2].text_color );
						}
						else
						{
							SetKeyColor( holder->key, 0 );
						}
					}
					else if( psv == (CTEXTSTR)-2 )
					{
						SetKeyLenses( holder->key
										, image_data[1].glare
										, image_data[1].down
										, image_data[1].up
										, image_data[1].mask
										);
						if( image_data[1].use_color )
						{
							SetKeyColor( holder->key, image_data[1].color );
							SetKeyTextColor( holder->key, image_data[1].text_color );
						}
						else
						{
							SetKeyColor( holder->key, 0 );
						}
					}
					else if( psv == (CTEXTSTR)-3 )
					{
						SetKeyLenses( holder->key
										, image_data[3].glare
										, image_data[3].down
										, image_data[3].up
										, image_data[3].mask
										);
						if( image_data[3].use_color )
						{
							SetKeyColor( holder->key, image_data[3].color );
							SetKeyTextColor( holder->key, image_data[3].text_color );
						}
						else
						{
							SetKeyColor( holder->key, 0 );
						}
					}
					else
					{
						SetKeyLenses( holder->key
										, image_data[0].glare
										, image_data[0].down
										, image_data[0].up
										, image_data[0].mask
										);
						if( image_data[0].use_color )
						{
							SetKeyColor( holder->key, image_data[0].color );
							SetKeyTextColor( holder->key, image_data[0].text_color );
						}
						else
						{
							SetKeyColor( holder->key, 0 );
						}
					}

				}
			}
		}
		SmudgeCommon( keypad->frame );
	}	
}

enum keypad_styles GetKeypadStyle( PSI_CONTROL pc_keypad )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		return keypad->style;
	}
	return (enum keypad_styles)0;
}

void SetKeypadStyle( PSI_CONTROL pc_keypad, enum keypad_styles style )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		TEXTCHAR **text_array;
		keypad->style = style;
		if( style & KEYPAD_DISPLAY_ALIGN_LEFT )
		{
			keypad->flags.bCenterJustify = 0;
			keypad->flags.bLeftJustify = 1;
		}
		else if( style & KEYPAD_DISPLAY_ALIGN_CENTER )
		{
			keypad->flags.bCenterJustify = 1;
			keypad->flags.bLeftJustify = 0;
		}
		else
		{
			keypad->flags.bCenterJustify = 0;
			keypad->flags.bLeftJustify = 0;
		}


		if( style & KEYPAD_STYLE_PASSWORD )
		{
			keypad->flags.bPassword = 1;
		}
		else
			keypad->flags.bPassword = 0;


		if( keypad->flags.bAlphaNum )
		{
         // maybe handle different keyboard styles here....
			return;
		}
		else
		{
			switch( style & KEYPAD_MODE_MASK )
			{
			case KEYPAD_STYLE_YES_NO:
				text_array = entrytext;
				break;
			case KEYPAD_STYLE_ENTER_CANCEL:
				text_array = entrytext5;
				break;
			case KEYPAD_STYLE_ENTER_CORRECT:
				text_array = entrytext6;
				break;
			default:
			case KEYPAD_STYLE_ENTER_CLEAR:
				text_array = entrytext4;
				break;
			case KEYPAD_STYLE_DOUBLE0_CLEAR:
				text_array = entrytext2;
				break;
			}

			if( style & KEYPAD_INVERT )
			{
				int row;
				int col;
				for( row = 0; row < 3; row++ )
				{
					for( col = 0; col < 3; col++ )
					{
						keypad->keys[row * keypad->cols + col].psv_key
							= keypad->flags.bAlphaNum
							? keyboardval[(2-row)*keypad->cols + col]
							: keyval[(2-row) * keypad->cols + col];
						keypad->keys[row * keypad->cols + col].psv_shifted_key
							= keypad->flags.bAlphaNum
							? keyboard_shifted_val[(2-row)*keypad->cols + col]
							: keyval[(2-row) * keypad->cols + col];
						SetKeyText( keypad->keys[ row * keypad->cols + col ].key, text_array[(2-row)*keypad->cols + col] );
					}
				}
				for( col = 0; col < 3; col++ )
				{
					keypad->keys[row * keypad->cols + col].psv_key
						= keypad->flags.bAlphaNum
						? keyboardval[row*keypad->cols + col]
						: keyval[row * keypad->cols + col];
					keypad->keys[row * keypad->cols + col].psv_shifted_key
						= keypad->flags.bAlphaNum
						? keyboard_shifted_val[row*keypad->cols + col]
						: keyval[row * keypad->cols + col];
					SetKeyText( keypad->keys[ row * keypad->cols + col ].key, text_array[row*keypad->cols + col] );
				}
			}
			else
			{
				int row;
				int col;
				for( row = 0; row < 3; row++ )
				{
					for( col = 0; col < 3; col++ )
					{
						keypad->keys[row * keypad->cols + col].psv_key
							= keypad->flags.bAlphaNum
							? keyboardval[row*keypad->cols + col]
							: keyval[row * keypad->cols + col];
						keypad->keys[row * keypad->cols + col].psv_shifted_key
							= keypad->flags.bAlphaNum
							? keyboard_shifted_val[row*keypad->cols + col]
							: keyval[row * keypad->cols + col];
						SetKeyText( keypad->keys[ row * keypad->cols + col ].key, text_array[(row)*keypad->cols + col] );
					}
				}
				for( col = 0; col < 3; col++ )
				{
					keypad->keys[row * keypad->cols + col].psv_key
						= keypad->flags.bAlphaNum
						? keyboardval[row*keypad->cols + col]
						: keyval[row * keypad->cols + col];
					keypad->keys[row * keypad->cols + col].psv_shifted_key
						= keypad->flags.bAlphaNum
						? keyboard_shifted_val[row*keypad->cols + col]
						: keyval[row * keypad->cols + col];
					SetKeyText( keypad->keys[ row * keypad->cols + col ].key, text_array[row*keypad->cols + col] );
				}
			}
		}
	}
}

void KeypadSetDisplayFormat( PSI_CONTROL pc_keypad, CTEXTSTR format )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		if( keypad->display_format )
			Release( (POINTER)keypad->display_format );
		if( format )
		{
			_64 mod_mask = 0;
			int escape = 0;
			int n;
			keypad->display_format = StrDup( format );
			keypad->display_out_length = 0;
			for( n = 0; keypad->display_format[n]; n++ )
			{
				if( escape )
				{
					// physical output character
					escape = 0;
					keypad->display_out_length++;
					continue;
				}
				if( keypad->display_format[n] == '\\' )
					escape = 1;
				else if( keypad->display_format[n] == '#' )
				{
					keypad->display_out_length++;
					if( mod_mask == 0 )
						mod_mask = 1;
               else
						mod_mask = mod_mask * 10;
				}
				else
				{
					keypad->display_out_length++;
				}
			}
			SetAccumulatorMask( keypad->accum, mod_mask );
		}
		else
		{
			SetAccumulatorMask( keypad->accum, 0 );
			keypad->display_out_length = 0;
		}
	}
}

void KeypadGetDisplayFormat( PSI_CONTROL pc_keypad, TEXTSTR format, int buflen )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
   if( keypad && keypad->display_format )
		StrCpyEx( format, keypad->display_format, buflen );
	else
      format[0] = 0;
}


void KeypadAddMagicKeySequence( PSI_CONTROL pc_keypad, CTEXTSTR sequence, void (CPROC*event_proc)( PTRSZVAL ), PTRSZVAL psv_sequence )
{
	ValidatedControlData( PKEYPAD, keypad_control.TypeID, keypad, pc_keypad );
	if( keypad )
	{
		PMAGIC_SEQUENCE magic_sequence = New( MAGIC_SEQUENCE );
		magic_sequence->last_tick_sequence_match = 0;
		magic_sequence->index_match = 0;
		magic_sequence->sequence = sequence;
		magic_sequence->event_proc = event_proc;
		magic_sequence->psv_sequence = psv_sequence;
		AddLink( &keypad->magic_sequences, magic_sequence );
	}
}


KEYPAD_NAMESPACE_END

