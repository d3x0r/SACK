#define _INCLUDE_CLIPBOARD
#include <stdhdrs.h>
#include <sharemem.h>
#include "controlstruc.h"
//#include <actimg.h> // alias to device output independance(?)
#include <logging.h>

#include <keybrd.h>
#include <controls.h>
#include <psi.h>

PSI_EDIT_NAMESPACE
//------------------------------------------------------------------------------
//---------------------------------------------------------------------------

typedef struct edit {
	PSI_CONTROL pc;
	struct {
		BIT_FIELD insert:1;
		BIT_FIELD bPassword : 1; // hide data content.
		BIT_FIELD bFocused : 1;
		BIT_FIELD bInternalUpdate : 1;
		BIT_FIELD bReadOnly : 1;
		BIT_FIELD bSelectSet : 1;
	}flags;
	TEXTCHAR *content; // our quick and dirty buffer...
	size_t nCaptionSize, nCaptionUsed;
	int top_side_pad;
	size_t Start; // first character in edit control
	size_t cursor_pos; // cursor position
	size_t MaxShowLen;
	size_t select_anchor;
	size_t select_start, select_end;
} EDIT, *PEDIT;

#define LEFT_SIDE_PAD 2
//---------------------------------------------------------------------------

CTEXTSTR GetString( PEDIT pe, CTEXTSTR text, size_t length )
{
   static size_t lastlen;
	static TEXTCHAR *temp;
	if( lastlen < length || !temp )
	{
		if( temp )
			Release( temp );
		temp = NewArray( TEXTCHAR, length+1 );
		lastlen = length;
	}
	if( pe->flags.bPassword )
	{
		size_t n;
		for( n = 0; text[n] && n < length; n++ )
		{
			temp[n] = '*';
		}
		temp[n] = 0;
      return temp;
	}

	return text;
}

//---------------------------------------------------------------------------

static int OnDrawCommon( EDIT_FIELD_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	SFTFont font;
	size_t ofs;
	int x, CursX;
   LOGICAL string_fits = FALSE;
	CTEXTSTR caption_text = GetText( pc->caption.text);
   CTEXTSTR output_string;

	_32 height;
	BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolor(pc)[EDIT_BACKGROUND] );
	//ClearImageTo( pc->Surface, basecolor(pc)[EDIT_BACKGROUND] );
	font = GetFrameFont( pc );
	// should probably figure some way to center the text vertical also...
	pe->MaxShowLen = GetMaxStringLengthFont( pc->surface_rect.width, font );
	
	if( ( pe->nCaptionUsed - pe->Start ) < pe->MaxShowLen )
	{
      // and don't move start, it's good where it's at...
		string_fits = TRUE;
		//pe->Start = 0;
		//pe->MaxShowLen = pe->nCaptionUsed - pe->Start;
	}
	if( ( (pe->cursor_pos-pe->Start) > (pe->MaxShowLen - 3) ) )
	{
		pe->Start = pe->cursor_pos - (pe->MaxShowLen - 3);
	}
	else if( (pe->cursor_pos-pe->Start) < ( 5 ) )
	{
		if( pe->cursor_pos > 5 )
		{
			pe->Start = pe->cursor_pos - ( 5 );
		}
		else
			pe->Start = 0;
	}
	else
	{
      // Start position is good, cursor is in the visible part of the line...
		//pe->Start = 0;
	}
	//lprintf( "drawing %d,%d,%d,%d", pe->Start, pe->cursor_pos, pe->MaxShowLen, string_fits );
	output_string = GetString( pe, caption_text + pe->Start, pe->cursor_pos-pe->Start );
	CursX = GetStringSizeFontEx( output_string, pe->cursor_pos-pe->Start, NULL, &height, font );
	if( USS_LTE( height, _32, pc->Surface->height, int ) )
		pe->top_side_pad = (pc->Surface->height - height) / 2;
	else
		pe->top_side_pad = 0;
	//lprintf( WIDE("------- CURSX = %d --- "), CursX );
	CursX += LEFT_SIDE_PAD;
	ofs = 0;
	x = LEFT_SIDE_PAD;
	// yuck - need to ... do this and that and the other...
	// if something is selected...
	if( pe->nCaptionUsed )
	{
      CTEXTSTR tmp;
		//lprintf( WIDE("Caption used... %d %d %d")
		//		 , pc->flags.bFocused
		//		 , pe->select_start
		//		 , pe->select_end);
		if( pc->flags.bFocused && pe->flags.bSelectSet )
		{
			// this is a working copy of this variable
			// that steps through the ranges of marked-ness
			size_t Start = pe->Start;
			do
			{
				size_t nLen;
				//lprintf( WIDE("%d %d %d"), Start, pe->select_start, pe->select_end );
				if( !pe->flags.bSelectSet || ( Start < pe->select_start ) )
				{
					if( pe->flags.bSelectSet && ( pe->select_start - Start < pe->MaxShowLen ) )
						nLen = pe->select_start - Start;
					else
						nLen = pe->MaxShowLen - ofs;
					//lprintf( WIDE("Showing %d of string in normal color before select..."), nLen );
					PutStringFontEx( pc->Surface, x, pe->top_side_pad
											 , basecolor(pc)[EDIT_TEXT], basecolor(pc)[EDIT_BACKGROUND]
										, tmp = GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, font );
					x += GetStringSizeFontEx( tmp, nLen, NULL, NULL, font );
				}
				else if( Start > pe->select_end ) // beyond the end of the display
				{
					nLen = pe->MaxShowLen - ofs;
					//lprintf( WIDE("Showing %d of string in normal color after select..."), nLen );
					PutStringFontEx( pc->Surface, x, pe->top_side_pad
											 , basecolor(pc)[EDIT_TEXT], basecolor(pc)[EDIT_BACKGROUND]
											 , tmp = GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, font );
					x += GetStringSizeFontEx( tmp, nLen, NULL, NULL, font );
				}
				else // start is IN select start to select end...
				{
					if( ( ofs + pe->select_end - Start ) < pe->MaxShowLen )
					{
						nLen = pe->select_end - Start + 1;
					}
					else
						nLen = pe->MaxShowLen - ofs;
					//lprintf( WIDE("Showing %d of string in selected color..."), nLen );
					PutStringFontEx( pc->Surface, x, pe->top_side_pad
											 , basecolor(pc)[SELECT_TEXT], basecolor(pc)[SELECT_BACK]
											 , tmp = GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, font );
					x += GetStringSizeFontEx( tmp, nLen, NULL, NULL, font );
				}
				Start += nLen;
				ofs += nLen;
			} while( ofs < pe->MaxShowLen );
		}
		else
		{
			PutStringFontEx( pc->Surface, x, pe->top_side_pad
								, basecolor(pc)[EDIT_TEXT], basecolor(pc)[EDIT_BACKGROUND]
								, tmp = GetString( pe, GetText( pc->caption.text) + pe->Start, pe->MaxShowLen )
								, pe->MaxShowLen
								, font
								);
			x += GetStringSizeFontEx( tmp, pe->MaxShowLen, NULL, NULL, font );
		}
	}
	//else
	//   lprintf( WIDE("NO caption used.")) ;
	if( pc->flags.bFocused )
	{
		//lprintf( WIDE("Have focus in edit control - drawing cursor thingy at %d"), CursX );
		do_line( pc->Surface, CursX
				 , 1
				 , CursX
				 , pc->surface_rect.height
				 , Color( 255,255,255 ) );
		do_line( pc->Surface, CursX+1
				 , 1
				 , CursX+1
				 , pc->surface_rect.height
				 , Color( 0,0,0 ) );
		do_line( pc->Surface, CursX-1
				 , 1
				 , CursX-1
				 , pc->surface_rect.height
				 , Color( 0,0,0 ) );
	}
	//else
	//   lprintf( WIDE("have NO focus in edit control, not drawing anything...") );
	return TRUE;
}

//---------------------------------------------------------------------------

static int OnMouseCommon( EDIT_FIELD_NAME )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	static int _b;
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	size_t cx, cy;
	int found = 0;
	size_t len = pc->caption.text ? GetTextSize( pc->caption.text ) : 0;
	_32 width, _width = 0, height;
	LOGICAL moving_left, moving_right;
	SFTFont font = GetCommonFont( pc );
	//lprintf( WIDE("Edit mosue: %d %d %X"), x, y, b );
	GetStringSize( GetText(pc->caption.text), &width, &height );
	// how to find the line/character we're interested in....
	cy = (y - pe->top_side_pad) / height;
	if( x < LEFT_SIDE_PAD )
	{
      found = 1;
		cx = 0;
	}
	else
	{
      CTEXTSTR caption_text = GetText( pc->caption.text);
		//cx = ( x - LEFT_SIDE_PAD ) / characters...
		// so given any font - variable size, we have to figure out which
      // character is on this line...
		for( cx = 1; ( cx <= ( len - pe->Start ) ); cx++ )
		{
			if( GetStringSizeFontEx( caption_text + pe->Start, cx, &width, NULL, font ) )
			{
				//lprintf( WIDE("is %*.*s(%d) more than %d?")
				//		 , cx,cx,GetText(pc->caption.text)
				//		 , width
				//		 , x );
				if( USS_GT(( width + LEFT_SIDE_PAD ),_32, x,S_32) )
				{
					// left 1/3 of the currnet character sets the cursor to the left
					// of the character, otherwise cursor is set on(after) the
					// current character.
					// OOP! - previously this test was backwards resulting in seemingly
					// very erratic cursor behavior..
					if( ((width+LEFT_SIDE_PAD)-x) > (width - _width)/3 )
						cx = cx-1;
					//lprintf( WIDE("Why yes, yes it is.") );
					found = 1;
					break;
				}
			}
			_width = width;
		}
	}
	// cx will be strlen + 1 if past end
   // cx is 0 at beginning.
	if( !found )
	{
		cx = len - pe->Start;
		// cx max...
      //lprintf( WIDE("Past end of string...") );
	}
	moving_left = 0;
	moving_right = 0;
	if( b & MK_LBUTTON )
		if( pe->cursor_pos != (cx+pe->Start) )
		{
			// this updates the current cursor position.
			// this works very well.... (now)
			if( pe->cursor_pos > (cx+pe->Start) )
				moving_left = 1;
			else
            moving_right = 1;
			pe->cursor_pos = cx + pe->Start;
			SmudgeCommon( pc );
		}
	{
		//lprintf( WIDE("current character: %d %d"), cx, cy );
	}
   //lprintf( "alright we have %d,%d,%d,%d", pe->select_anchor, pe->Start, pe->select_start, cx );
	if( b & MK_LBUTTON )
	{
      //cx -= 1;
		if( !( _b & MK_LBUTTON ) )
		{
			// we're not really moving, we just started...
			pe->flags.bSelectSet = 0;
			moving_left = moving_right = 0;
			if( (cx + pe->Start) < len )
			{
				//lprintf( WIDE("Setting select start, end at %d,%d,%d"), cx + pe->Start, cx, pe->Start );
            pe->select_anchor = cx + pe->Start;
			}
			else
			{
            //lprintf( WIDE("Setting select start, end at %d,%d"), len-1, len-1 );
				pe->select_anchor = len;
			}
			//lprintf( WIDE("--- Setting begin and end... hmm first button I guess Select...") );
			SmudgeCommon( pc );
		}
		else
		{
			//lprintf( WIDE("still have that mouse button down.... %d,%d,%d"), moving_left, moving_right, cx );
			if( moving_left || moving_right )
			{
				if( (cx+pe->Start) < pe->select_anchor )
				{
					pe->flags.bSelectSet = 1;
					pe->select_start = cx + pe->Start;
					pe->select_end = pe->select_anchor - 1;
				}
				else
				{
					pe->flags.bSelectSet = 1;
					pe->select_end = cx + pe->Start;
					pe->select_start = pe->select_anchor;
				}
				SmudgeCommon( pc );
			}
		}
	}
	_b = b;
   return 1;
}

//---------------------------------------------------------------------------

void CutEditText( PEDIT pe, PTEXT *caption )
{
	// any selected text is now deleted... the buffer is shortened...
    if( pe->flags.bSelectSet )
	{
		pe->nCaptionUsed -= (pe->select_end - pe->select_start) + 1;
		if( pe->nCaptionUsed && pe->nCaptionUsed > pe->select_start )
		{
			MemCpy( GetText( *caption ) + pe->select_start
			       , GetText( *caption ) + pe->select_end+1
			       , pe->nCaptionUsed - pe->select_start );
			SetTextSize( *caption, pe->nCaptionUsed );
			GetText( *caption )[pe->nCaptionUsed] = 0;
		}
		else
		{
			SetTextSize( *caption, pe->nCaptionUsed );
			GetText( *caption )[pe->select_start] = 0;
		}
		pe->cursor_pos = pe->select_start;
	 }
	 pe->flags.bSelectSet = 0;
}

static void InsertAChar( PEDIT pe, PTEXT *caption, TEXTCHAR ch )
{
	if( (pe->nCaptionUsed+1) >= pe->nCaptionSize )
	{
		PTEXT newtext;
		pe->nCaptionSize += 16;
		newtext = SegCreate( pe->nCaptionSize );
		StrCpyEx( GetText( newtext ), GetText( *caption )
				 , (pe->nCaptionUsed+1) ); // include the NULL, the buffer will be large enough.
		SetTextSize( newtext, pe->nCaptionUsed );
		GetText( newtext )[pe->nCaptionUsed] = 0;
		LineRelease( *caption );
		*caption = newtext;
	}
	{
		size_t n;
		pe->nCaptionUsed++;
		for( n = pe->nCaptionUsed; ( n > pe->cursor_pos ); n-- )
		{
			GetText( *caption )[n] =
				GetText( *caption )[n-1];
		}
		GetText( *caption )[pe->cursor_pos] = ch;
		pe->cursor_pos++;
	}
	SetTextSize( *caption, pe->nCaptionUsed );
}

void TypeIntoEditControl( PSI_CONTROL pc, CTEXTSTR text )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	if( pc )
	{
		while( text[0] )
		{
			InsertAChar( pe, &pc->caption.text, text[0] );
			text++;
		}
	}
	SmudgeCommon( pc );
}

//---------------------------------------------------------------------------

#ifdef _WIN32
static void GetMarkedText( PEDIT pe, PTEXT *caption, TEXTCHAR *buffer, size_t nSize )
{
	if( pe->flags.bSelectSet )
	{
		if( USS_GT( nSize, size_t, (pe->select_end - pe->select_start) + 1, int ) )
		{
			nSize = (pe->select_end - pe->select_start) + 1;
		}
		else
			nSize--; // leave room for the nul.

		// otherwise nSize is maximal copy or correct amount to copy
		MemCpy( buffer
				, GetText( *caption ) + pe->select_start
				, nSize );
		buffer[nSize] = 0; // set nul terminator.
	}
	else
	{
		MemCpy( buffer
				, GetText( *caption ) + pe->select_start
				, nSize );
		buffer[nSize-1] = 0; // set nul terminator.
	}
}

//---------------------------------------------------------------------------

static void Paste( PEDIT pe, PTEXT *caption )
{
	if( OpenClipboard(NULL) )
	{
		_32 format;
        // successful open...
		format = EnumClipboardFormats( 0 );
		if( pe->flags.bSelectSet )
			CutEditText( pe, caption );
        while( format )
        {
            if( format == CF_TEXT )
            {
                HANDLE hData = GetClipboardData( CF_TEXT );
					 char *pData = (char*)GlobalLock( hData );
					 {
						 while( pData && pData[0] )
						 {
							 InsertAChar( pe, caption, pData[0] );
							 pData++;
						 }
					 }
                break;
            }
            format = EnumClipboardFormats( format );
        }
		CloseClipboard();
    }
    else
    {
        //DECLTEXT( msg, WIDE("Clipboard was not available") );
        //EnqueLink( &pdp->ps->Command->Output, &msg );
    }
}

//---------------------------------------------------------------------------

static void Copy( PEDIT pe, PTEXT *caption )
{
	TEXTCHAR data[1024];
	GetMarkedText( pe, caption, data, sizeof( data ) );
#ifndef UNDER_CE
	if( data[0] && OpenClipboard(NULL) )
	{
		size_t nLen = strlen( data ) + 1;
		HGLOBAL mem = GlobalAlloc( 
#ifndef _ARM_
			GMEM_MOVEABLE
#else
				0
#endif
			, nLen );
		MemCpy( GlobalLock( mem ), data, nLen );
		GlobalUnlock( mem );
		EmptyClipboard();
		SetClipboardData( CF_TEXT, mem );
		CloseClipboard();
		GlobalFree( mem );
	}
#endif
}

//---------------------------------------------------------------------------

static void Cut( PEDIT pe, PTEXT *caption )
{
	Copy( pe, caption );
	if( pe->flags.bSelectSet )
		CutEditText( pe, caption );
	else
		SetCommonText( (PCONTROL)pe, NULL );

}
#endif

static int OnKeyCommon( EDIT_FIELD_NAME )( PSI_CONTROL pc, _32 key )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	int used_key = 0;
	int updated = 0;
	TEXTCHAR ch;
	if( !pe || pe->flags.bReadOnly )
		return 0;
	if( KEY_CODE(key) == KEY_TAB )
		return 0;
	if( key & KEY_PRESSED )
	{
		if( key & KEY_CONTROL_DOWN )
		{
			switch( KEY_CODE( key ) )
			{
#ifdef _WIN32
			case KEY_C:
				Copy( pe, &pc->caption.text );
				SmudgeCommon( pc );
				break;
			case KEY_X:
				Cut( pe, &pc->caption.text );
				SmudgeCommon( pc );
				break;
			case KEY_V:
				Paste( pe, &pc->caption.text );
				SmudgeCommon( pc );
				break;
#endif
			}
		}
		else switch( KEY_CODE( key ) )
		{
		case KEY_LEFT:
			{
				size_t oldpos = pe->cursor_pos;
				if( pe->cursor_pos )
				{
					pe->cursor_pos--;
					if( ! ( key & KEY_SHIFT_DOWN ) )
					{
						pe->flags.bSelectSet = 0;
					}
					else
					{
						if( pe->flags.bSelectSet )
						{
							if( oldpos == pe->select_start )
								pe->select_start--;
							else if( pe->cursor_pos == pe->select_end )
								pe->select_end--;
							else
							{
								pe->select_start =
									pe->select_end = pe->cursor_pos;
								pe->flags.bSelectSet = 1;
							}
							if( pe->select_start > pe->select_end )
								pe->flags.bSelectSet = 0;
						}
						else
						{
							if( oldpos > 0 )
							{
								pe->select_start =
									pe->select_end = oldpos-1;
								pe->flags.bSelectSet = 1;
							}
						}
					}
					SmudgeCommon( pc );
				}
				used_key = 1;
				break;
			}
		case KEY_RIGHT:
			{
				size_t oldpos = pe->cursor_pos;
				if( pe->cursor_pos < pe->nCaptionUsed )
				{
					pe->cursor_pos++;
					if( !(key & KEY_SHIFT_DOWN ) )
					{
						pe->flags.bSelectSet = 0;
					}
					else
					{
                  if( pe->flags.bSelectSet )
						{
							if( oldpos == (pe->select_end+1) )
							{
								pe->select_end++;
							}
							else if( oldpos == pe->select_start )
								pe->select_start++;
							else
							{
								pe->select_start =
									pe->select_end = oldpos;
								pe->flags.bSelectSet = 1;
							}
							if( pe->select_start > pe->select_end )
								pe->flags.bSelectSet = 0;
						}
						else
						{
							pe->select_start =
								pe->select_end = oldpos;
							pe->flags.bSelectSet = 1;
						}
					}
					SmudgeCommon( pc );
				}
				used_key = 1;
				break;
			}
#ifndef __ANDROID__
		case KEY_END:
			if( key & KEY_SHIFT_DOWN )
			{
				if( !pe->flags.bSelectSet )
				{
					pe->select_start = pe->cursor_pos;
					pe->flags.bSelectSet = 1;
				}
				else
				{
					if( pe->select_end == pe->cursor_pos )
					{
					}
					else if( pe->select_start == pe->cursor_pos )
					{
						pe->select_start = pe->select_end;
					}
					else
					{
						pe->select_start = pe->cursor_pos;
					}
				}
				pe->select_end = pe->nCaptionUsed-1;
			}
			else
			{
				pe->flags.bSelectSet = 0;
			}
			pe->cursor_pos = pe->nCaptionUsed;
			SmudgeCommon( pc );
			used_key = 1;
			break;
		case KEY_HOME:
			if( key & KEY_SHIFT_DOWN )
			{
				if( pe->select_start == pe->cursor_pos )
				{
				}
				else if(  pe->select_end == pe->cursor_pos )
				{
					pe->select_end = pe->select_start;
				}
				else
				{
					pe->select_end = pe->cursor_pos-1;
				}
				pe->flags.bSelectSet = 1;
				pe->select_start = 0;
			}
			else
			{
				pe->flags.bSelectSet = 0;
			}
			pe->cursor_pos = 0;
			pe->Start = 0;
			SmudgeCommon( pc );
			used_key = 1;
			break;
#endif
		case KEY_DELETE:
			if( pe->flags.bSelectSet )
			{
				updated = 1;
				CutEditText( pe, &pc->caption.text );
			}
			else
			{
				if( pe->cursor_pos != pe->cursor_pos )
				{
					pe->flags.bSelectSet = 1;
					pe->select_end =
						pe->select_start = pe->cursor_pos;
					CutEditText( pe, &pc->caption.text );
					updated = 1;
				}
			}
			if( updated )
				SmudgeCommon( pc );
			used_key = 1;
			break;
		case KEY_BACKSPACE:
			//Log( WIDE("Backspace?!") );
			if( pe->flags.bSelectSet )
			{
				updated = 1;
				CutEditText( pe, &pc->caption.text );
			}
			else
			{
				if( pe->cursor_pos )
				{
					pe->flags.bSelectSet = 1;
					pe->select_end =
						pe->select_start = pe->cursor_pos-1;
					CutEditText( pe, &pc->caption.text );
					updated = 1;
				}
			}
			if( updated )
				SmudgeCommon( pc );
			used_key = 1;
			break;
#ifndef __ANDROID__
		case KEY_ESCAPE:
			InvokeDefault( (PCONTROL)pc, INV_CANCEL );
			used_key = 1;
			break;
#endif
		case KEY_ENTER:
			InvokeDefault( (PCONTROL)pc, INV_OKAY );
			used_key = 1;
			break;
		default:
			//Log2( WIDE("Got Key: %08x(%c)"), key, key & 0xFF );
			ch = GetKeyText( key );
			if( ch )
			{
				if( (unsigned char)ch == 0xFF )
					ch = 0;
				if( pe->flags.bSelectSet )
					CutEditText( pe, &pc->caption.text );
				InsertAChar( pe, &pc->caption.text, ch );
				SmudgeCommon( pc );
				//printf( WIDE("Key: %d(%c)\n"), ch,ch );
				used_key = 1;
			}
			break;
		}
	}
	return used_key;
}

//---------------------------------------------------------------------------

PSI_CONTROL SetEditControlReadOnly( PSI_CONTROL pc, LOGICAL bEnable )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	if( pe )
	{
      //lprintf( WIDE("Setting readonly attribut of control to %d"), bEnable );
		pe->flags.bReadOnly = bEnable;
	}
	return pc;
}

//---------------------------------------------------------------------------

PSI_CONTROL SetEditControlPassword( PSI_CONTROL pc, LOGICAL bEnable )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	if( pe )
	{
      //lprintf( WIDE("Setting readonly attribut of control to %d"), bEnable );
		pe->flags.bPassword = bEnable;
	}
	return pc;
}

//---------------------------------------------------------------------------

static void OnChangeCaption( EDIT_FIELD_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	if( !pc->caption.text )
	{
		pc->caption.text = SegCreate(1);
		SetTextSize( pc->caption.text, 0 );
	}
	if( GetTextSize( pc->caption.text ) )
		pe->cursor_pos = GetTextSize( pc->caption.text );
	else
		pe->cursor_pos = 0;

   pe->nCaptionSize = pe->cursor_pos+1;
	pe->nCaptionUsed = pe->nCaptionSize-1;

	pe->Start = 0;
	pe->flags.bSelectSet = 1;
	pe->select_start = 0;
	pe->select_end = pe->nCaptionUsed-1;
	
	SmudgeCommon(pc);
}

//---------------------------------------------------------------------------

static int OnCommonFocus( EDIT_FIELD_NAME )( PCONTROL pc, LOGICAL bFocused )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	if( pe )
	{
		//lprintf( WIDE("Setting active focus here!") );
#ifdef __ANDROID__
		if( bFocused )
			SACK_Vidlib_ShowInputDevice();
      else
         SACK_Vidlib_HideInputDevice();
#endif
		pe->flags.bFocused = bFocused;
		if( pe->flags.bFocused )
		{
			size_t len = pc->caption.text ? GetTextSize( pc->caption.text ) : 0;
			pe->flags.bSelectSet = 1;
			pe->select_start = 0;
			pe->select_end = len - 1;
			pe->select_anchor = 0;
			pe->cursor_pos = len;
		}
		SmudgeCommon( pc );
	}
	return TRUE;
}

//---------------------------------------------------------------------------
#undef MakeEditControl

int CPROC InitEditControl( PSI_CONTROL pControl );
int CPROC ConfigEditControl( PSI_CONTROL pc )
{
	return InitEditControl(pc);
}

PSI_CONTROL CPROC MakeEditControl( PSI_CONTROL pFrame, int attr
									  , int x, int y, int w, int h
									  , _32 nID, TEXTCHAR *caption )
{
	return VMakeCaptionedControl( pFrame, EDIT_FIELD
										 , x, y, w, h
										 , nID, caption );
}

void CPROC GrabFilename( PSI_CONTROL pc, CTEXTSTR name, S_32 x, S_32 y )
{
	SetControlText( pc, name );
}

int CPROC InitEditControl( PSI_CONTROL pc )
{
	ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	if( pe )
	{
		pe->pc = pc;
		AddCommonAcceptDroppedFiles( pc, GrabFilename );
		return TRUE;
	}
	return FALSE;
}

static int OnCreateCommon( EDIT_FIELD_NAME )( PSI_CONTROL pc )
{
	return InitEditControl( pc );
}

#include <psi.h>
CONTROL_REGISTRATION
edit_control = { EDIT_FIELD_NAME
					, { {73, 21}, sizeof( EDIT ), BORDER_INVERT_THIN|BORDER_NOCAPTION|BORDER_FIXED }
					, NULL // InitEditControl
					, NULL
					, NULL //DrawEditControl
					, NULL //MouseEditControl
					, NULL //KeyEditControl
};

PRIORITY_PRELOAD( RegisterEdit, PSI_PRELOAD_PRIORITY )
{
   DoRegisterControl( &edit_control );
}

PSI_EDIT_NAMESPACE_END


