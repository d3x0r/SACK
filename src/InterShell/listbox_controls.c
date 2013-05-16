#include <stdhdrs.h>
#include <configscript.h>

#include "intershell_local.h"
#include "intershell_registry.h"
#include "menu_listbox.h"



void InterShell_EditListbox( void )
{
	PSI_CONTROL list = (PSI_CONTROL)psv;
	if( list)
	{
      PSI_CONTROL frame;
		int okay = 0;
		int done = 0;
		// psv may be passed as NULL, and therefore there was no task assicated with this
		// button before.... the button is blank, and this is an initial creation of a button of this type.
		// basically this should call (psv=CreatePaper(button)) to create a blank button, and then launch
		// the config, and return the button created.
		//PPAPER_INFO issue = button->paper;
		int created = 0;
		frame = LoadXMLFrame( WIDE("ListboxProperty.isframe") );
		if( frame )
		{
			//could figure out a way to register methods under
			// the filename of the property thing being loaded
			// for future use...
			{ // init frame which was loaded..
				SetCommonButtons( frame, &done, &okay );
            l.new_font = NULL;
				SetButtonPushMethod( GetControl( frame, BTN_PICKFONT ), PickSessionListFont, psv );
			}
         SetCheckState( GetControl( frame, CHECKBOX_LIST_MULTI_SELECT ), GetListboxMultiSelect( list ) );
			DisplayFrameOver( frame, parent_frame );

			//EditFrame( frame, TRUE );
			//edit frame must be done after the frame has a physical surface...
			// it's the surface itself that allows the editing...
			CommonWait( frame );
			if( okay )
			{
				// blah get the resuslts...
            if( l.new_font )
					l.font = l.new_font;
            //DebugBreak();
				SetListboxMultiSelect( list, GetCheckState( GetControl( frame, CHECKBOX_LIST_MULTI_SELECT ) ) );
			}
         DestroyFrame( &frame );
		}
	}
   return psv;
}

