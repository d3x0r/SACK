
#define _INCLUDE_CLIPBOARD
#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>

#include "controlstruc.h"
#include <controls.h>
#include <keybrd.h>
#include <psi.h>

struct combobox {
   _32 data;
};
typedef struct combobox COMBOBOX, *PCOMBOBOX;

static int OnCreateCommon( WIDE("Combo Box") )( PSI_CONTROL pc )
{
   //ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
   return TRUE;
}

static int OnDrawCommon( WIDE("Combo Box") )( PSI_CONTROL pc )
{
	//ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
#if 0
   ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	SFTFont font;
	int ofs, x, CursX;
   _32 height;
   BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolors[EDIT_BACKGROUND] );
	//ClearImageTo( pc->Surface, basecolors[EDIT_BACKGROUND] );
	font = GetFrameFont( pc );
	// should probably figure some way to center the text vertical also...
	pe->MaxShowLen = GetMaxStringLengthFont( pc->surface_rect.width, font );
   //lprintf( WIDE("Drawing an edit ronctorl... %s"), GetText( pc->caption.text ));
	if( pe->cursor_pos > (pe->MaxShowLen - 2) )
	{
		pe->Start = pe->cursor_pos - (pe->MaxShowLen - 2);
		CursX = GetStringSizeFontEx( GetText( pc->caption.text) + pe->Start, pe->MaxShowLen - 2, NULL, &height, font );
	}
	else
	{
      _32 tmp;
		pe->Start = 0;
		if( ( tmp = GetTextSize( pc->caption.text ) - pe->Start ) < pe->MaxShowLen )
		{
         //lprintf( WIDE("Shortening from maximum to actual we have.. %d  %d"), pe->MaxShowLen, tmp );
         pe->MaxShowLen = tmp;
		}
		CursX = GetStringSizeFontEx( GetText( pc->caption.text) + pe->Start, pe->cursor_pos, NULL, &height, font );
	}
   if( height <= pc->Surface->height )
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
		//lprintf( WIDE("Caption used... %d %d %d")
		//		 , pc->flags.bFocused
		//		 , pe->select_start
		//		 , pe->select_end);
		if( pc->flags.bFocused
			&& pe->select_start >= 0 && pe->select_end >= 0 )
		{
			// this is a working copy of this variable
			// that steps through the ranges of marked-ness
         _32 Start = pe->Start;
			do
			{
				int nLen;
            //lprintf( WIDE("%d %d %d"), Start, pe->select_start, pe->select_end );
				if( Start < pe->select_start )
				{
					if( ( pe->select_start - Start ) < pe->MaxShowLen )
						nLen = pe->select_start - Start;
					else
						nLen = pe->MaxShowLen - ofs;
					//lprintf( WIDE("Showing %d of string in normal color before select..."), nLen );
					PutStringFontEx( pc->Surface, x, pe->top_side_pad
											 , basecolors[EDIT_TEXT], basecolors[EDIT_BACKGROUND]
											 , GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, font );
               x += GetStringSizeFontEx( GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, NULL, NULL, font );
				}
				else if( Start > pe->select_end ) // beyond the end of the display
				{
					nLen = pe->MaxShowLen - ofs;
               //lprintf( WIDE("Showing %d of string in normal color after select..."), nLen );
					PutStringFontEx( pc->Surface, x, pe->top_side_pad
											 , basecolors[EDIT_TEXT], basecolors[EDIT_BACKGROUND]
											 , GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, font );
               x += GetStringSizeFontEx( GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, NULL, NULL, font );
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
											 , basecolors[SELECT_TEXT], basecolors[SELECT_BACK]
											 , GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, font );
               x += GetStringSizeFontEx( GetString( pe, GetText( pc->caption.text) + Start, nLen ), nLen, NULL, NULL, font );
				}
				Start += nLen;
				ofs += nLen;
			} while( ofs < pe->MaxShowLen );
		}
		else
		{
			PutStringFontEx( pc->Surface, x, pe->top_side_pad
									 , basecolors[EDIT_TEXT], basecolors[EDIT_BACKGROUND]
									 , GetString( pe, GetText( pc->caption.text) + pe->Start, pe->MaxShowLen ), pe->MaxShowLen
									 , font
									 );
			x += GetStringSizeFontEx( GetString( pe, GetText( pc->caption.text) + pe->Start, pe->MaxShowLen ), pe->MaxShowLen, NULL, NULL, font );
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
#endif
	//else
	//   lprintf( WIDE("have NO focus in edit control, not drawing anything...") );
   return TRUE;
}

//---------------------------------------------------------------------------

static int OnMouseCommon( WIDE("Combo Box"))(PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
//CUSTOM_CONTROL_MOUSE(  MouseComboBoxControl, ( PCOMMON pc, S_32 x, S_32 y, _32 b ) )
{
   //ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	static int _b;
#if 0
   ValidatedControlData( PEDIT, EDIT_FIELD, pe, pc );
	int cx, cy, found = 0, len = pc->caption.text ? GetTextSize( pc->caption.text ) : 0;
	_32 width, _width = 0, height;
   LOGICAL moving_left, moving_right;
   SFTFont font = GetCommonFont( pc );
	//lprintf( WIDE("ComboBox mosue: %d %d %X"), x, y, b );
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
		//cx = ( x - LEFT_SIDE_PAD ) / characters...
		// so given any font - variable size, we have to figure out which
      // character is on this line...
		for( cx = 1; cx <= ( len - pe->Start ); cx++ )
		{
			if( GetStringSizeFontEx( GetText( pc->caption.text) + pe->Start, cx, &width, NULL, font ) )
			{
				//lprintf( WIDE("is %*.*s(%d) more than %d?")
				//		 , cx,cx,GetText(pc->caption.text)
				//		 , width
				//		 , x );
				if( ( width + LEFT_SIDE_PAD ) > x )
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
		if( pe->cursor_pos != cx )
		{
			// this updates the current cursor position.
			// this works very well.... (now)
			if( pe->cursor_pos > cx )
				moving_left = 1;
			else
            moving_right = 1;
			pe->cursor_pos = cx;
			SmudgeCommon( pc );
		}
	{
		//lprintf( WIDE("current character: %d %d"), cx, cy );
	}
	if( b & MK_LBUTTON )
	{
      //cx -= 1;
		if( !( _b & MK_LBUTTON ) )
		{
			// we're not really moving, we just started...
         pe->select_start = pe->select_end = -1;
         moving_left = moving_right = 0;
			if( cx < len )
			{
				//lprintf( WIDE("Setting select start, end at %d,%d"), cx, cx );
            pe->select_anchor
					= cx;
			}
			else
			{
            //lprintf( WIDE("Setting select start, end at %d,%d"), len-1, len-1 );
				pe->select_anchor
					= len;
			}
			//lprintf( WIDE("--- Setting begin and end... hmm first button I guess Select...") );
			SmudgeCommon( pc );
		}
		else
		{
			//lprintf( WIDE("still have that mouse button down....") );
			if( moving_left || moving_right )
			{
				if( cx < pe->select_anchor )
				{
					pe->select_start = cx;
               pe->select_end = pe->select_anchor - 1;
				}
				else
				{
					pe->select_end = cx;
               pe->select_start = pe->select_anchor;
				}
            SmudgeCommon( pc );
			}
		}
	}
#endif
	_b = b;
   return 1;
}



CONTROL_REGISTRATION
combobox_control = { COMBOBOX_CONTROL_NAME
					, { {73, 21}, sizeof( COMBOBOX ), BORDER_INVERT_THIN }
};

PRIORITY_PRELOAD( RegisterComboBox, PSI_PRELOAD_PRIORITY )
{
	DoRegisterControl( &combobox_control );
   //RegisterAlias( PSI_ROOT_REGISTRY WIDE("/control/") EDIT_FIELD_NAME "/rtti", WIDE("psi/control/combobox/rtti") );
}


