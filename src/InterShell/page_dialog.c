/*
 *  Creator:Jim Buckeyne
 *  Purpose: Page related controls for the POS framework.  Page titles, page changing buttons
 *           (more)?
 *
 *
 *  (c)Freedom Collective 2006++
 *
 */


#if 0
// obsoleted.
#include <sharemem.h>

#include "intershell_local.h"
#include "resource.h"
#include "intershell_registry.h"
#include <controls.h>

#include "pages.h"

// is invoking this frame control on the button is bad.
// and we get all kinda lock-jawed...

typedef struct page_changer_dialog_struct
{
	PPAGE_CHANGER page_changer;
   int *pOkay, *pDone;
} PAGE_DIALOG, *PPAGE_DIALOG;


OnEditControl( PAGE_CHANGER_NAME )( PTRSZVAL psv, PSI_CONTROL parent_frame )
{
	PPAGE_CHANGER page_changer = (PPAGE_CHANGER)psv;
	if( page_changer )
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
		frame = LoadXMLFrame( WIDE("page_change_property.isframe") );
		if( frame )
		{
			//could figure out a way to register methods under
			// the filename of the property thing being loaded
			// for future use...
			SetCommonButtons( frame, &done, &okay );
			SetCommonButtonControls( frame );
         created = 1;
		}
		if( frame )
		{
			DisplayFrameOver( frame, parent_frame );
			//if( created )
				EditFrame( frame, TRUE );
			//edit frame must be done after the frame has a physical surface...
			// it's the surface itself that allows the editing...
			CommonWait( frame );
			if( okay )
			{
				// save properties from controls...
				GetCommonButtonControls( frame );
				{
					TEXTCHAR buffer[128];
					int i,o;
					for( i = o = 0; buffer[i]; i++,o++ )
					{
						if( buffer[o] == '\\' )
						{
							switch( buffer[o+1] )
							{
							case 'n':
                        i++;
                        buffer[o] = '\n';
                        break;
							}
						}
					}
					GetControlText( GetControl( frame, TXT_CONTROL_TEXT ), buffer, sizeof( buffer ) );
					//if( page_changer->button_label )
               //   Release( page_changer->button_label );
					//page_changer->button_label = StrDup( buffer );
				}
			}
			DestroyFrame( &frame );
		}
	}
	return psv;
}

#endif
