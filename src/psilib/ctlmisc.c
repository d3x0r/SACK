

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
	lprintf( WIDE("show message box") );
	DisplayFrame( msg );
	CommonWait( msg );
	DestroyFrame( &msg );
}

int SimpleUserQuery( TEXTSTR result, int reslen, CTEXTSTR question, PCOMMON pAbove )
{
	return SimpleUserQueryEx( result, reslen, question, pAbove, NULL, 0 );
}

struct user_query_info{
	PSI_CONTROL pf; // frame
	PSI_CONTROL edit;
	int Done;
	int Okay;
	void (CPROC*query_success)(PTRSZVAL,LOGICAL);
	PTRSZVAL query_user_data;
	TEXTSTR result;
	int reslen;
};

static void query_success_trigger( void )
{
	//struct user_query_info *query_state = (struct user_query_info*)GetCommonUserData( pc );

}

static void CPROC OkayClicked( PTRSZVAL psv, PSI_CONTROL pc )
{
	struct user_query_info *query_state = (struct user_query_info *)psv;
	query_state->Okay = 1;
	query_state->Done = 1;
	GetControlText( query_state->edit, query_state->result, query_state->reslen );
	if( query_state->query_success )
	{
		query_state->query_success( query_state->query_user_data, TRUE );
		{
			PSI_CONTROL pf = GetFrame( pc );
			DestroyFrame( &pf );
		}
		Release( query_state );
	}
	//Release( query_state->result );
}

static void CPROC CancelClicked( PTRSZVAL psv, PSI_CONTROL pc )
{
	struct user_query_info *query_state = (struct user_query_info *)psv;
	query_state->Done = 1;
	if( query_state->query_success )
	{
		query_state->query_success( query_state->query_user_data, FALSE );
		{
			PSI_CONTROL pf = GetFrame( pc );
			DestroyFrame( &pf );
		}
		Release( query_state );
	}
	//Release( query_state->result );
}

int SimpleUserQueryEx( TEXTSTR result, int reslen, CTEXTSTR question, PSI_CONTROL pAbove, void (CPROC*query_success_callback)(PTRSZVAL, LOGICAL), PTRSZVAL query_user_data )
{
	PSI_CONTROL pf, pc;
	struct user_query_info *query_state = New( struct user_query_info );
	S_32 mouse_x, mouse_y;

	//int Done = FALSE, Okay = FALSE;
	pf = CreateFrame( NULL, 0, 0, 280, 65, 0, pAbove );
	SetCommonUserData( pf, (PTRSZVAL)query_state );
	query_state->pf = pf;
	query_state->Done = FALSE;
	query_state->Okay = FALSE;
	query_state->result = result;
	query_state->reslen = reslen;
	pc = MakeTextControl( pf, 5, 2, 320, 18, TXT_STATIC, question, TEXT_NORMAL );

	query_state->edit = MakeEditControl( pf, 5, 23, 270, 14, TXT_STATIC, NULL, 0 );
	AddCommonButtons( pf, &query_state->Done, &query_state->Okay );
	SetButtonPushMethod( GetControl( pf, IDOK ), OkayClicked, (PTRSZVAL)query_state );
	SetButtonPushMethod( GetControl( pf, IDCANCEL ), CancelClicked, (PTRSZVAL)query_state );
	GetMousePosition( &mouse_x, &mouse_y );
	MoveFrame( pf, mouse_x - 140, mouse_y - 30 );
	//lprintf( WIDE("Show query....") );
	DisplayFrame( pf );
	SetCommonFocus( query_state->edit );

	query_state->query_success = query_success_callback;
	if( !query_success_callback )
	{
		int okay;
		CommonWait( pf );
		if( query_state->Okay )
		{
			GetControlText( query_state->edit, result, reslen );
		}
		DestroyFrame( &pf );
		okay = query_state->Okay;
		Release( query_state );
		return okay;
	}
	else
	{
		query_state->query_success = query_success_callback;
		query_state->query_user_data = query_user_data;
		return 0;
	}

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

