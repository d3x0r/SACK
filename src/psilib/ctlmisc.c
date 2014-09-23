

#include "global.h"
#include <sharemem.h>
#include <controls.h>
#include <psi.h>

PSI_NAMESPACE 

PSI_PROC( void, SimpleMessageBox )( PCOMMON parent, CTEXTSTR title, CTEXTSTR content )
{
	PCOMMON msg;
	CTEXTSTR start, end;
	TEXTCHAR msgtext[256];
	int okay = 0;
	int y = 5;
	_32 width, height;
	_32 title_width, greatest_width;
#ifdef USE_INTERFACES
	GetMyInterface();
#endif
	GetStringSize( content, &width, &height );
	title_width = GetStringSize( title, NULL, NULL );
	if( title_width > width )
		greatest_width = title_width;
	else
		greatest_width = width;
	msg = CreateFrame( title, 0, 0
						 , greatest_width + 10, height + (COMMON_BUTTON_PAD * 3) + COMMON_BUTTON_HEIGHT
						 , 0, parent );
	end = start = content;
	do
	{
		while( end[0] && end[0] != '\n' )
			end++;
		if( end[0] )
		{
			MemCpy( msgtext, (POINTER)start, end-start );
			msgtext[end-start] = 0;
			//end[0] = 0;
			MakeTextControl( msg, COMMON_BUTTON_PAD, y
							  , greatest_width, height
							  , -1, msgtext, 0 );
			//end[0] = '\n';
			end = start = end+1;
			y += height;
		}
		else
			MakeTextControl( msg, COMMON_BUTTON_PAD, y
							  , greatest_width, height
							  , -1, start, 0 );
	} while( end[0] );
	//AddExitButton( msg, &done );
	AddCommonButtons( msg, NULL, &okay );
	lprintf("show message box" ); 
	DisplayFrame( msg );
	CommonWait( msg );
	DestroyFrame( &msg );
}

int SimpleUserQuery( TEXTSTR result, int reslen, CTEXTSTR question, PCOMMON pAbove )
{
	PCOMMON pf, pc;
	PCONTROL edit;
	S_32 mouse_x, mouse_y;

	int Done = FALSE, Okay = FALSE;
	pf = CreateFrame( NULL, 0, 0, 280, 65, 0, pAbove );
	pc = MakeTextControl( pf, 5, 2, 320, 18, TXT_STATIC, question, TEXT_NORMAL );

	edit = MakeEditControl( pf, 5, 23, 270, 14, TXT_STATIC, NULL, 0 );
	AddCommonButtons( pf, &Done, &Okay );
	GetMousePosition( &mouse_x, &mouse_y );
	MoveFrame( pf, mouse_x - 140, mouse_y - 30 );
	SetCommonFocus( edit );
	lprintf( "Show query...." );
	DisplayFrame( pf );
	CommonWait( pf );
	if( Okay )
	{
		GetControlText( edit, result, reslen );
	}
	DestroyFrame( &pf );
	return Okay;
}

void RegisterResource( CTEXTSTR appname, CTEXTSTR resource_name, int resource_name_id, int resource_name_range, CTEXTSTR type_name )
{
	static int nextID = 10000;
	if( !resource_name_id )
	{
		resource_name_id = nextID;
		if( !resource_name_range )
			resource_name_range = 1;
		nextID += resource_name_range;
	}

	{
		TEXTCHAR root[256];
		TEXTCHAR old_root[256];
		//lprintf( "resource name = %s", resource_names[n].type_name );
		//lprintf( "resource name = %s", resource_names[n].resource_name );
		snprintf( root, sizeof( root ), PSI_ROOT_REGISTRY WIDE( "/resources/") WIDE("%s/%s/%s" )
				  , type_name
				  , appname
				  , resource_name 
				  );
			RegisterIntValue( root
								 , WIDE("value")
								 , resource_name_id );
			RegisterIntValue( root
								 , WIDE("range")
								 , resource_name_range );
			snprintf( root, sizeof( root ), PSI_ROOT_REGISTRY WIDE("/resources/") WIDE("%s") WIDE("/%s")
					, type_name 
					  , appname
						);
			snprintf( old_root, sizeof( old_root ), PSI_ROOT_REGISTRY WIDE("/resources/") WIDE("%s") WIDE("/%s")
					  , appname
					  , type_name
						);
			RegisterIntValue( root
								 , resource_name
								 , resource_name_id );
			RegisterClassAlias( root, old_root );
	}
}

PSI_NAMESPACE_END

// $Log: ctlmisc.c,v $
// Revision 1.13  2005/02/10 16:55:51  panther
// Fixing warnings...
//
// Revision 1.12  2005/01/08 00:00:15  panther
// Break up submenus for adding controls, fix simple message box
//
// Revision 1.11  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.2  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.9  2003/09/21 16:25:28  panther
// Removed much noisy logging, all in the interest of sheet controls.
// Fixed some linking of services.
// Fixed service close on dead client.
//
// Revision 1.8  2003/09/15 17:06:37  panther
// Fixed to image, display, controls, support user defined clipping , nearly clearing correct portions of frame when clearing hotspots...
//
// Revision 1.7  2003/05/01 21:31:57  panther
// Cleaned up from having moved several methods into frame/control common space
//
// Revision 1.6  2003/04/02 07:31:25  panther
// Enable local GetMyInterface which will auto configure library to current link
//
// Revision 1.5  2003/03/31 23:39:53  panther
// Clean up some passing of string fucntions to be CTEXTSTR
//
// Revision 1.4  2003/03/30 19:40:14  panther
// Encapsulate pick color data better.
//
// Revision 1.3  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
