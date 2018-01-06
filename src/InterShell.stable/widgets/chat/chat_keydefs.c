#define CORECON_SOURCE
#define NO_LOGGING
#define KEYS_DEFINED
#include <stdhdrs.h>
#include <keybrd.h>
// included to have pending struct available to pass to
// command line update....
//#include "WinLogic.h"

#define CHAT_CONTROL_SOURCE
#include "chat_control.h"
#include "chat_control_internal.h"


extern int myTypeID;

enum{
   KS_DELETE
};

//----------------------------------------------------------------------------

int CPROC KeyGetGatheredLine( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	PTEXT tmp_input = GetUserInputLine( pci );
	PTEXT line = BuildLine( tmp_input );
	// input is in segments of 256 characters... collapse into a single line.
	list->input.command_mark_start = list->input.command_mark_end = 0;
	if( line && GetTextSize( line ) )
	{
		list->input.phb_Input->pBlock->pLines[0].flags.nLineLength = (int)LineLengthExEx( list->input.CommandInfo->CollectionBuffer, FALSE, 8, NULL );
		list->input.phb_Input->pBlock->pLines[0].pLine = list->input.CommandInfo->CollectionBuffer;
		list->input.phb_Input->flags.bUpdated = 1;
		BuildDisplayInfoLines( list->input.phb_Input, 0, list->input_font );
		if( line )
			list->InputData( list->psvInputData, line );
	}
	else if( line )
		LineRelease( line );

	return UPDATE_COMMAND; 
}

/*
#if(WINVER >= 0x0400)
#define VK_PROCESSKEY     0xE5
#endif

#define VK_ATTN           0xF6
#define VK_CRSEL          0xF7
#define VK_EXSEL          0xF8
#define VK_EREOF          0xF9
#define VK_PLAY           0xFA
#define VK_ZOOM           0xFB
#define VK_NONAME         0xFC
#define VK_PA1            0xFD
#define VK_OEM_CLEAR      0xFE
*/
//----------------------------------------------------------------------------

int Chat_CommandKeyUp( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	int x, y;
	int cursorpos;
	GetInputCursorPos( list, &x, &y );
	cursorpos = GetInputCursorIndex( list, x, y + 1 );
	SetUserInputPosition( pci, cursorpos, COMMAND_POS_SET );
   
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int Chat_HandleKeyDown( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	int x, y;
	int cursorpos;
	GetInputCursorPos( list, &x, &y );
	if( y )
	{
		cursorpos = GetInputCursorIndex( list, x, y - 1 );
		SetUserInputPosition( pci, cursorpos, COMMAND_POS_SET );
	}
	else
		SetUserInputPosition( pci, -1, COMMAND_POS_SET );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

static size_t GetInputIndex(  PUSER_INPUT_BUFFER pci )
{
			PTEXT start = pci->CollectionBuffer;
			size_t index = 0;
			SetStart( start );
			while( start != pci->CollectionBuffer )
			{
				index += GetTextSize( start );
				start = NEXTLINE( start );
			}
			index += pci->CollectionIndex;
			return index;
}

//----------------------------------------------------------------------------

int Chat_KeyHome( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	size_t old_index;
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		if( list->input.command_mark_start == list->input.command_mark_end )
		{
			size_t index = GetInputIndex( pci );
			old_index = index;
			list->input.command_mark_start = list->input.command_mark_end = (int)index;
		}
	}
	else
	{
		list->input.command_mark_start = list->input.command_mark_end = 0;
	}
	SetUserInputPosition( pci, 0, COMMAND_POS_SET );
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		size_t index = GetInputIndex( pci );
		if( index < list->input.command_mark_start )
			list->input.command_mark_start = (int)index;
		else if( index != old_index )
			list->input.command_mark_end = (int)index;
	}
	return UPDATE_COMMAND; 
}


int Chat_KeyEndCmd( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	size_t old_index;
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		if( list->input.command_mark_start == list->input.command_mark_end )
		{
			size_t index = GetInputIndex( pci );
			old_index = index;
			list->input.command_mark_start = list->input.command_mark_end = (int)index;
		}
	}
	else
	{
		list->input.command_mark_start = list->input.command_mark_end = 0;
	}
	SetUserInputPosition( pci, -1, COMMAND_POS_SET );
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		size_t index = GetInputIndex( pci );
		if( index > list->input.command_mark_end )
			list->input.command_mark_start = (int)index;
		else if( index != old_index )
			list->input.command_mark_start = (int)index;
	}
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int Chat_KeyInsert( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	SetUserInputInsert( pci, -1 );
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int Chat_KeyRight( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	size_t old_index = list->input.command_mark_end;
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		if( list->input.command_mark_start == list->input.command_mark_end )
		{
			size_t index = GetInputIndex( pci );
			old_index = index;
			list->input.command_mark_start = list->input.command_mark_end = (int)index;
		}
	}
	else
	{
		list->input.command_mark_start = list->input.command_mark_end = 0;
	}
	SetUserInputPosition( pci, 1, COMMAND_POS_CUR );
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		size_t index = GetInputIndex( pci );
		if( index > list->input.command_mark_end )
			list->input.command_mark_end = (int)index;
		else if( index != old_index )
			list->input.command_mark_start = (int)index;
	}
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

int Chat_KeyLeft( PCHAT_LIST list, PUSER_INPUT_BUFFER pci )
{
	size_t old_index = list->input.command_mark_start;
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		if( list->input.command_mark_start == list->input.command_mark_end )
		{
			size_t index = GetInputIndex( pci );
			old_index = index;
			list->input.command_mark_start = list->input.command_mark_end = (int)index;
		}
	}
	else
	{
		list->input.command_mark_start = list->input.command_mark_end = 0;
	}

	SetUserInputPosition( pci, -1, COMMAND_POS_CUR );
	if( list->input.control_key_state & KEY_MOD_SHIFT )
	{
		size_t index = GetInputIndex( pci );
		if( index < list->input.command_mark_start )
			list->input.command_mark_start = index;
		else if( index != old_index )
			list->input.command_mark_end = index;
	}
	return UPDATE_COMMAND;
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Extensions and usage of keybinding data
// -- so far seperated so that perhaps it could be a seperate module...
//----------------------------------------------------------------------------

// Usage: /KeyBind shift-F1
//        ... #commands
//        /endmac
// Usage: /KeyBind shift-F1 kill
// Usage: /KeyBind $F1 ... ^F1 $^F1
//  if parameters follow the keybind key-def, those params
//  are taken as keystrokes to type...
//  if no parameters follow, the definition is assumed to
//  be a macro definition, and the macro is invoked by
//  the processing entity...
//----------------------------------------------------------------------------

int Widget_DoStroke( PCHAT_LIST list, PTEXT stroke )
{
	INDEX i;
	int bOutput = FALSE;
	DECLTEXT( key, WIDE(" ") );
	//Log1( WIDE("Do Stroke with %c"), stroke->data.data[0] );
	if( list->input.command_mark_start != list->input.command_mark_end )
	{
		int n;
		SetUserInputPosition( list->input.CommandInfo, list->input.command_mark_start, COMMAND_POS_SET );
		for( n = 0; n < ( list->input.command_mark_end - list->input.command_mark_start ); n++ )
			DeleteUserInput( list->input.CommandInfo );
		list->input.command_mark_start = list->input.command_mark_end = 0;
	}
	{
		PTEXT seg;
		size_t outchar;
		int had_cr = 0;
		seg = stroke;
		while( seg )
		{
			outchar = 0;
			for( i = 0; i < seg->data.size; i++ )
			{
				if( seg->data.data[i] == '\n' ) {
					if( !had_cr )
						seg->data.data[outchar++] = '\n'; // carriage return = linefeed
					else
						had_cr = FALSE;
				} else if( seg->data.data[i] == '\r' ) {
					had_cr = TRUE;
					seg->data.data[outchar++] = '\n'; // carriage return = linefeed
				}  else
					seg->data.data[outchar++] = seg->data.data[i]; // save any other character
			}
			if( outchar != i )
				seg->data.size = outchar;
			seg = NEXTLINE( seg );
		}
		GatherUserInput( list->input.CommandInfo
						, (PTEXT)stroke );
	}
	return TRUE;
}

void ReformatInput( PCHAT_LIST list )
{
		if( !list->input.phb_Input->pBlock )
		{
			list->input.phb_Input->pBlock = list->input.phb_Input->region->pHistory.root.next;
			list->input.phb_Input->pBlock->nLinesUsed = 1;
			list->input.phb_Input->pBlock->pLines[0].flags.deleted = 0;
			list->input.phb_Input->nLine = 1;
			list->input.phb_Input->nFirstLine = 1;
		}
		list->input.phb_Input->pBlock->pLines[0].pLine = list->input.CommandInfo->CollectionBuffer;
		SetStart( list->input.phb_Input->pBlock->pLines[0].pLine );
		{
			DECLTEXT( blank_eol, "" );
			list->input.phb_Input->pBlock->pLines[0].flags.nLineLength = LineLengthExEx( list->input.phb_Input->pBlock->pLines[0].pLine, FALSE, 8, (PTEXT)&blank_eol );
		}
		list->input.phb_Input->flags.bUpdated = 1;
		//if( !pdp->flags.bDirect && pdp->flags.bWrapCommand )
		BuildDisplayInfoLines( list->input.phb_Input, 0, list->input_font );
}

//----------------------------------------------------------------------------
void Widget_WinLogicDoStroke( PCHAT_LIST list, PTEXT stroke )
{
	//EnterCriticalSec( &pdp->Lock );
	if( Widget_DoStroke( list, stroke ) )
	{
		ReformatInput( list );
	}

}

//----------------------------------------------------------------------------

