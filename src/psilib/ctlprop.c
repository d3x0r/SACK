#include <stdhdrs.h>
#include <sharemem.h>
#include "controlstruc.h"
#include <procreg.h>
#include <psi.h>

PSI_PROP_NAMESPACE

extern ControlInitProc KnownControlInit[];

#include "resource.h"

/*
#define EDT_X       100
#define EDT_Y       101
#define EDT_WIDTH   102
#define EDT_HEIGHT  103
#define EDT_CAPTION 104
#define EDT_ID      105
#define LABEL_X       200
#define LABEL_Y       201
#define LABEL_WIDTH   202
#define LABEL_HEIGHT  203
#define LABEL_CAPTION 204
#define LABEL_ID      205
#define LISTBOX_IDS   300
*/

// this is a base - probably 100 is good
// space to have
#define MNU_ADDCONTROL 1000

#define MNU_EDITTHING  2000
#define MNU_DONE 2001
#define MNU_DELETETHING 2002
#define MNU_SAVEFRAME 2003

static PMENU pFrameEditMenu, pControlEditMenu;
//static PSI_CONTROL pEditProperties;

typedef struct edit_property_data_tag {
	PSI_CONTROL *ppFrame;
	PSI_CONTROL pEditCurrent;
	PSI_CONTROL pPropertySheet;
	int32_t x,y;
	int bDone, bOkay;
} EDIT_PROP_DATA, *PEDIT_PROP_DATA;

//---------------------------------------------------------------------------

#define DEFAULT_BUTTON_WIDTH  150
#define DEFAULT_BUTTON_HEIGHT 20
#define DEFAULT_BUTTON_BORDER BORDER_NORMAL

void CreateAControl( PSI_CONTROL frame, uint32_t type, PEDIT_PROP_DATA pepd )
{
	//if( type < USER_CONTROL )
	{
		int32_t x = pepd->x;
		int32_t y = pepd->y;
		lprintf( WIDE("new control at %")_32fs WIDE(",%")_32fs WIDE(" %")_32f WIDE(",%")_32f WIDE(""), x, y, frame->rect.x, frame->rect.y );
		MakeControl( frame, type, x, y, 0, 0, -1 );
	}
}

//---------------------------------------------------------------------------

struct list_item_data
{
	CTEXTSTR appname; // app name
	CTEXTSTR resname; // resource name
	CTEXTSTR _typename; // control type name
	int value;
	int range;
};

void CPROC SetControlIDProperty( uintptr_t psv, PSI_CONTROL list, PLISTITEM item )
{
	struct list_item_data *data = (struct list_item_data*)GetItemData( item );
	if( data )
	{
		//uint32_t ID = GetItemData( item );
		PSI_CONTROL edit = GetNearControl( list, EDT_ID );
		if( edit )
		{
			TEXTCHAR buffer[32];
			if( data->range > 1 )
				tnprintf( buffer, sizeof( buffer ), WIDE("%d + %d"), data->value, data->range );
			else
				tnprintf( buffer, sizeof( buffer ), WIDE("%d"), data->value );
			SetControlText( edit, buffer );
			{
				TEXTCHAR buffer[256];
				tnprintf( buffer, sizeof( buffer ), WIDE("%s/%s/%s"), data->appname, data->_typename, data->resname );
				SetControlText( GetNearControl( list, EDT_IDNAME ), buffer );
			}
		}
	}
}

int FillControlIDList( CTEXTSTR root, PSI_CONTROL listbox, PSI_CONTROL pc, int level, CTEXTSTR priorname )
{
	int status = FALSE;
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	TEXTCHAR rootname[256];
	//lprintf( "Look for resoruces under %s at %d", root, level );
	tnprintf( rootname, sizeof( rootname ), PSI_ROOT_REGISTRY WIDE("/resources/%s%s%s")
			  , pc->pTypeName
			  , root?WIDE("/"):WIDE("")
			  , root?root:WIDE("") );
	for( name = GetFirstRegisteredName( rootname, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		if( !NameIsAlias( &data ) )
		{
			int value = (int)(uintptr_t)GetRegisteredValueExx( (CTEXTSTR)data, name, WIDE("value"), TRUE );
			if( value )
			{
				struct list_item_data *itemdata = (struct list_item_data*)Allocate( sizeof( *itemdata ) );
				itemdata->appname = StrDup( root );
				itemdata->_typename = pc->pTypeName;
				itemdata->resname = name;
				/* ETHICALITY DISCLAIMED: this is an okay conversion, cause we're asking for an INT type anyhow...*/
				itemdata->value = (int)(uintptr_t)GetRegisteredValueExx( (CTEXTSTR)data, name, WIDE("value"), TRUE );
				/* ETHICALITY DISCLAIMED: this is an okay conversion, cause we're asking for an INT type anyhow...*/
				itemdata->range = (int)(uintptr_t)GetRegisteredValueExx( (CTEXTSTR)data, name, WIDE("range"), TRUE );
				//lprintf( WIDE("Found Name %s"), name2 );
				SetItemData( AddListItemEx( listbox, level, name ), (uintptr_t)itemdata );
				status = TRUE;
			}
			else
			{
				PLISTITEM pli;
				if( !NameIsAlias( &data ) )
				{
					tnprintf( rootname, sizeof(rootname),WIDE("%s%s%s"), root?root:WIDE(""), root?WIDE("/"):WIDE(""), name );
					pli = AddListItemEx( listbox, level, name );
					if( !FillControlIDList( rootname, listbox, pc, level+1, name ) )
					{
						DeleteListItem( listbox, pli );
					}
					else
						status = TRUE;
				}
			}
		}
	}
	return status;
}

void InitFrameControls( PSI_CONTROL pcFrame, PSI_CONTROL pc )
{
	TEXTCHAR buffer[128];
	SetControlText( GetControl( pcFrame, LABEL_CAPTION ), WIDE("Caption") );
	SetControlText( GetControl( pcFrame, LABEL_X ), WIDE("X") );
	SetControlText( GetControl( pcFrame, LABEL_Y ), WIDE("Y") );
	SetControlText( GetControl( pcFrame, LABEL_WIDTH ), WIDE("Width") );
	SetControlText( GetControl( pcFrame, LABEL_HEIGHT ), WIDE("Height") );
	SetControlText( GetControl( pcFrame, LABEL_ID ), WIDE("ID") );
	tnprintf( buffer, sizeof( buffer ), WIDE("%s"), pc->caption.text?GetText( pc->caption.text ):"" );
	SetControlText( GetControl( pcFrame, EDT_CAPTION ), buffer );
	tnprintf( buffer, sizeof( buffer ), WIDE("%")_32fs WIDE(""), pc->rect.x );
	SetControlText( GetControl( pcFrame, EDT_X ), buffer );
	tnprintf( buffer, sizeof( buffer ), WIDE("%")_32fs WIDE(""), pc->rect.y );
	SetControlText( GetControl( pcFrame, EDT_Y ), buffer );
	tnprintf( buffer, sizeof( buffer ), WIDE("%")_32f WIDE(""), pc->rect.width );
	SetControlText( GetControl( pcFrame, EDT_WIDTH ), buffer );
	tnprintf( buffer, sizeof( buffer ), WIDE("%")_32f WIDE(""), pc->rect.height );
	SetControlText( GetControl( pcFrame, EDT_HEIGHT ), buffer );
	tnprintf( buffer, sizeof( buffer ), WIDE("%d"), pc->nID );
	SetControlText( GetControl( pcFrame, EDT_ID ), buffer );
	SetControlText( GetControl( pcFrame, EDT_IDNAME ), pc->pIDName );
	{

		PSI_CONTROL list = GetControl( pcFrame, LISTBOX_IDS );
		if( list )
		{
//cpg26dec2006 c:\work\sack\src\psilib\ctlprop.c(214): Warning! W202: Symbol 'name' has been defined, but not referenced
//cpg26dec2006 c:\work\sack\src\psilib\ctlprop.c(215): Warning! W202: Symbol 'data' has been defined, but not referenced

//cpg26dec2006 			CTEXTSTR name;
//cpg26dec2006 			POINTER data = NULL;
			SetListboxIsTree( list, TRUE );
			SetSelChangeHandler( list, SetControlIDProperty, (uintptr_t)pc );
			FillControlIDList( NULL, list, pc, 0, NULL );
#if 0
			for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY WIDE("/resources"), &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				{
					char rootname[256];
					CTEXTSTR name2;
					int first = 1;
					POINTER data2 = NULL;
					tnprintf( rootname, sizeof(rootname),PSI_ROOT_REGISTRY WIDE("/resources/%s/%s"), name, pc->pTypeName );
					//lprintf( WIDE("newroot = %s"), rootname );
					for( name2 = GetFirstRegisteredName( rootname, &data2 );
						 name2;
						  name2 = GetNextRegisteredName( &data2 ) )
					{
						struct list_item_data *data = (struct list_item_data*)Allocate( sizeof( *data ) );
						data->appname = name;
						data->_typename = pc->pTypeName;
						data->resname = name2;
						/* ETHICALITY DISCLAIMED: this is an okay conversion, cause we're asking for an INT type anyhow...*/
						data->value = (int)(long)GetRegisteredValueExx( (CTEXTSTR)data2, name2, "value", TRUE );
						/* ETHICALITY DISCLAIMED: this is an okay conversion, cause we're asking for an INT type anyhow...*/
						data->range = (int)(long)GetRegisteredValueExx( (CTEXTSTR)data2, name2, "range", TRUE );
						lprintf( WIDE("Found Name %s"), name2 );
						if( first )
							AddListItemEx( list, 0, name );
						first = 0;
						SetItemData( AddListItemEx( list, 1, name2 ), (uintptr_t)data );
					}
				}
			}
#endif
		}
#if 0
		char rootname[256];
		void (CPROC*f)(uintptr_t);
		tnprintf( rootname, sizeof( rootname ), PSI_ROOT_REGISTRY WIDE("/resources/"), button->pTypeName );
		f = GetRegisteredProcedure2( rootname, void, WIDE("button_destroy"), (uintptr_t) );
		if( f )
			f(button->psvUser);
#endif
	}
}

// LOL that's pretty sexy, huh? LOL
#ifndef _MSC_VER
// vc8 pukes on statick initialziation here
static
#endif
TEXTCHAR control_property_frame_xml[] = {
//#define stuff(a) #a
//stuff(
#include "CommonEdit.Frame"
//    )
 };


PSI_PROC( int, EditControlProperties )( PSI_CONTROL control )
{
	PEDIT_PROP_DATA pEditProps = (PEDIT_PROP_DATA)Allocate( sizeof( EDIT_PROP_DATA ) );
	PSI_CONTROL pf, pSheet = NULL;
	int32_t x, y;
	int32_t select;
	GetMousePosition( &pEditProps->x, &pEditProps->y );
	select = TrackPopup( pControlEditMenu, GetFrame( control ) );
	if( select < 0 )
		return 1;

	switch( select )
	{
	case MNU_DONE:
		EditFrame( GetFrame( control ), FALSE );
		break;
	case MNU_DELETETHING:
		DestroyCommon( &control );
		return 0;
		break;
	case MNU_EDITTHING:

		if( pEditProps )
		{
			pEditProps->bDone = FALSE;
			pEditProps->bOkay = FALSE;
			pEditProps->pEditCurrent = control;
		}
		GetMousePosition( &x, &y );
		pf = CreateFrame( WIDE("Control Properties")
							 , (x - PROP_WIDTH/2)>0?(x - PROP_WIDTH/2):0, y
							 , PROP_WIDTH + 20
							 , PROP_HEIGHT + 20 + 25, BORDER_NORMAL, GetFrame( control ) );
		{
			static int bAnotherLayer;
			PSI_CONTROL pc = MakeSheetControl( pf
													, 5, 5
													, PROP_WIDTH+10, PROP_HEIGHT + 10
													, -1 );
			if( !bAnotherLayer )
			{
				bAnotherLayer++;
				pSheet = ParseXMLFrame( control_property_frame_xml, sizeof( control_property_frame_xml ) );
				if( !pSheet )
					pSheet = LoadXMLFrame( WIDE("Common Edit.Frame") );
				//DumpFrameContents( pSheet );
				bAnotherLayer--;
			}
			if( pSheet )
			{
				//lprintf( "****************" );
				InitFrameControls( pSheet, control );
			}
			else
			{
				pSheet = CreateFrame( WIDE("Common")
										  , 0, 0, PROP_WIDTH
										  , PROP_HEIGHT, BORDER_NONE|BORDER_WITHIN, NULL );
				if( pSheet )
				{
					TEXTCHAR buffer[256];
					MakeTextControl( pSheet, PROP_PAD, 05, 58, 14, TXT_STATIC, WIDE("Caption"), 0 );
					MakeTextControl( pSheet, PROP_PAD, 21, 58, 14, TXT_STATIC, WIDE("X"), 0 );
					MakeTextControl( pSheet, PROP_PAD, 37, 58, 14, TXT_STATIC, WIDE("Y"), 0 );
					MakeTextControl( pSheet, PROP_PAD, 53, 58, 14, TXT_STATIC, WIDE("Width"), 0 );
					MakeTextControl( pSheet, PROP_PAD, 69, 58, 14, TXT_STATIC, WIDE("Height"), 0 );
					MakeTextControl( pSheet, PROP_PAD, 85, 58, 14, TXT_STATIC, WIDE("ID"), 0 );
					MakeTextControl( pSheet, PROP_PAD, 101, 58, 14, TXT_STATIC, WIDE("ID Name"), 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%s"), GetText( control->caption.text ) );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 04, PROP_WIDTH-10-(58+5), 14, EDT_CAPTION, buffer, 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%")_32fs WIDE(""), control->rect.x );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 20, 56, 14, EDT_X, buffer, 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%")_32fs WIDE(""), control->rect.y );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 36, 56, 14, EDT_Y, buffer, 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%")_32f WIDE(""), control->rect.width );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 52, 56, 14, EDT_WIDTH, buffer, 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%")_32f WIDE(""), control->rect.height );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 68, 56, 14, EDT_HEIGHT, buffer, 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%d"), control->nID );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 84, 56, 14, EDT_ID, buffer, 0 );
					tnprintf( buffer, sizeof( buffer ), WIDE("%s"), control->pIDName );
					MakeEditControl( pSheet, PROP_PAD + PROP_PAD + 58, 84, 56, 14, EDT_IDNAME, buffer, 0 );
					MakeListBox( pSheet, PROP_PAD, 117, 400, 200, LISTBOX_IDS, 0 );
					//SaveXMLFrame( pSheet, WIDE("Common Edit.Frame") );
					InitFrameControls( pSheet, control );
					//DumpFrameContents( pSheet );
				}
 			}
			//DebugBreak();
			if( !pSheet )
			{
				DebugBreak(); // there's more to cleanup here.
				return 0; // failed, no properties avaialable for some reason.
			}
			AddSheet( pc,pSheet );
			{
				TEXTCHAR classname[32];
				GetControlPropSheet gcps;
				tnprintf( classname, sizeof( classname ), PSI_ROOT_REGISTRY WIDE("/control/%d/rtti"), control->nType );
				gcps = GetRegisteredProcedure( classname, PSI_CONTROL, get_property_page, (PSI_CONTROL) );

				if( gcps )
				{
					PSI_CONTROL pCustomSheet;
					pCustomSheet = gcps( (PSI_CONTROL)control );
					lprintf( WIDE("Got the page...") );
					AddSheet( pc
							  , (PSI_CONTROL)(pEditProps->pPropertySheet = pCustomSheet) );
				}
				else
					lprintf( WIDE("can't Get the page...") );
			}
			AddCommonButtons( pf, &pEditProps->bDone, &pEditProps->bOkay );
			DisplayFrame( pf );
			//EditFrame( pf, TRUE );
			//DumpFrameContents( pf );
			CommonWait( pf );
			if( pEditProps->bOkay )
			{
				PSI_CONTROL pc;
				int32_t x, y;
				uint32_t w, h, id;
				static TEXTCHAR buffer[32000];
				GetControlText( GetControl(pSheet, EDT_CAPTION ), buffer, sizeof( buffer ) );
				SetControlText( control, buffer );
				GetControlText( pc = GetControl(pSheet, EDT_X ), buffer, sizeof( buffer ) );
				if( pc )
					x = (int32_t)IntCreateFromText( buffer );
				if( pc )
				{
					GetControlText( pc = GetControl( pSheet, EDT_Y), buffer, sizeof( buffer ) );
					if( pc )
					{
						y = (int32_t)IntCreateFromText( buffer );
						MoveControl( control, x, y );
					}
				}
				GetControlText( pc = GetControl(pSheet, EDT_WIDTH ), buffer, sizeof( buffer ) );
				if( pc )
				{
					w = (uint32_t)IntCreateFromText( buffer );
					GetControlText( pc = GetControl(pSheet, EDT_HEIGHT ), buffer, sizeof( buffer ) );
					if( pc )
					{
						h = (uint32_t)IntCreateFromText( buffer );
						SizeControl( control, w, h );
					}
				}
				GetControlText( GetControl(pSheet, EDT_ID ), buffer, sizeof( buffer ) );
				id = (uint32_t)IntCreateFromText( buffer );
				SetControlID( (PSI_CONTROL)control, id );
				GetControlText( GetControl( pSheet, EDT_IDNAME ), buffer, sizeof( buffer ) );
				if( buffer[0] )
				{
					if( control->pIDName )
						Release( (POINTER)control->pIDName );
					control->pIDName = StrDup( buffer );
				}
				if( pEditProps->pPropertySheet )
				{
					TEXTCHAR classname[32];
					ApplyControlPropSheet Apply;
					tnprintf( classname, sizeof( classname ), PSI_ROOT_REGISTRY WIDE("/control/%d/rtti"), control->nType );
					Apply = GetRegisteredProcedure( classname, void, read_property_page, (PSI_CONTROL, PSI_CONTROL) );
					if( Apply )
					{
						Apply( (PSI_CONTROL)pEditProps->pPropertySheet, (PSI_CONTROL)pc );
					}
				}
			}
			// not sure if this gets killed...
			//DestroyFrame( pSheet );
			DestroyCommon( &pf );
				{
					TEXTCHAR classname[32];
					DoneControlPropSheet Done;
					tnprintf( classname, sizeof( classname ), PSI_ROOT_REGISTRY WIDE("/control/%d/rtti"), control->nType );
					Done = GetRegisteredProcedure( classname, void, done_property_page, (PSI_CONTROL) );
					if( Done )
					{
						Done( (PSI_CONTROL)pEditProps->pPropertySheet );
					}
				}
			break;
		}
		default:
			lprintf( WIDE("Unknown menu option chosen.  Custom control?") );
			break;
	}
	Release( pEditProps );
	return 1;
}

	//---------------------------------------------------------------------------

PSI_PROC( int, EditFrameProperties )( PSI_CONTROL frame, int32_t x, int32_t y )
{
	PEDIT_PROP_DATA pEditProps = (PEDIT_PROP_DATA)Allocate( sizeof( EDIT_PROP_DATA ) );
	PSI_CONTROL pf;
	uint32_t select;
	pEditProps->x = x - frame->surface_rect.x;
	pEditProps->y = y - frame->surface_rect.y;
	select = TrackPopup( pFrameEditMenu, frame );
	if( select >= MNU_ADDCONTROL &&
		select < ( MNU_ADDCONTROL + 1000 ) )
	{
		CreateAControl( frame, select - MNU_ADDCONTROL, pEditProps );
	}
	else switch( select )
	{
	case MNU_DONE:
		EditFrame( frame, FALSE );
		break;
	case MNU_DELETETHING:
		{
			PTHREAD thread = frame->pCommonButtonData.thread;
			DestroyFrame( &frame );
			WakeThread( thread );
		}
		break;
	case MNU_SAVEFRAME:
		// should pick a file here...
		SaveXMLFrame( frame, NULL );
		break;
	case MNU_EDITTHING:

		if( pEditProps )
		{
			pEditProps->bDone = FALSE;
			pEditProps->bOkay = FALSE;
			pEditProps->pEditCurrent = (PSI_CONTROL)frame;
		}
		GetMousePosition( &x, &y );
		pf = CreateFrame( WIDE("Frame Properties")
							 , (x - PROP_WIDTH/2)>0?(x - PROP_WIDTH/2):0, y, PROP_WIDTH
							 , 120, BORDER_NORMAL, GetFrame( frame ) );
		if( pf )
		{
			TEXTCHAR buffer[128];
			MakeTextControl( pf, PROP_PAD, 05, 58, 12, TXT_STATIC, WIDE("Caption"), 0 );
			MakeTextControl( pf, PROP_PAD, 21, 58, 12, TXT_STATIC, WIDE("X"), 0 );
			MakeTextControl( pf, PROP_PAD, 37, 58, 12, TXT_STATIC, WIDE("Y"), 0 );
			MakeTextControl( pf, PROP_PAD, 53, 58, 12, TXT_STATIC, WIDE("Width"), 0 );
			MakeTextControl( pf, PROP_PAD, 69, 58, 12, TXT_STATIC, WIDE("Height"), 0 );
			MakeTextControl( pf, PROP_PAD, 85, 58, 12, TXT_STATIC, WIDE("ID"), 0 );
			tnprintf( buffer, sizeof( buffer ), WIDE("%s"), GetText( frame->caption.text ) );
			MakeEditControl( pf
								, PROP_PAD + PROP_PAD + 58, 04
								, PROP_WIDTH-10-(58+5), 14
								, EDT_CAPTION, buffer, 0 );
			tnprintf( buffer, sizeof( buffer ), WIDE("%")_32fs WIDE(""), frame->rect.x );
			MakeEditControl( pf
								, PROP_PAD + PROP_PAD + 58, 20
								, 56, 14, EDT_X, buffer, 0 );
			tnprintf( buffer, sizeof( buffer ), WIDE("%")_32fs WIDE(""), frame->rect.y );
			MakeEditControl( pf
								, PROP_PAD + PROP_PAD + 58, 36
								, 56, 14, EDT_Y, buffer, 0 );
			tnprintf( buffer, sizeof( buffer ), WIDE("%")_32f WIDE(""), frame->rect.width );
			MakeEditControl( pf
								, PROP_PAD + PROP_PAD + 58, 52
								, 56, 14, EDT_WIDTH, buffer, 0 );
			tnprintf( buffer, sizeof( buffer ), WIDE("%")_32f WIDE(""), frame->rect.height );
			MakeEditControl( pf
								, PROP_PAD + PROP_PAD + 58, 68
								, 56, 14, EDT_HEIGHT, buffer, 0 );
			tnprintf( buffer, sizeof( buffer ), WIDE("%d"), frame->nID );
			MakeEditControl( pf
								, PROP_PAD + PROP_PAD + 58, 84
								, 56, 14, EDT_ID, buffer, 0 );

			AddCommonButtons( pf, &pEditProps->bDone, &pEditProps->bOkay );
			DisplayFrame( pf );
			CommonWait( pf );
			if( pEditProps->bOkay )
			{
				int32_t x, y;
				uint32_t w, h, id;
				GetControlText( GetControl(pf, EDT_CAPTION ), buffer, sizeof( buffer ) );
				SetControlText( frame, buffer );
				GetControlText( GetControl(pf, EDT_X ), buffer, sizeof( buffer ) );
				x = (int32_t)IntCreateFromText( buffer );
				GetControlText( GetControl(pf, EDT_Y), buffer, sizeof( buffer ) );
				y = (int32_t)IntCreateFromText( buffer );
				MoveFrame( frame, x, y );
				GetControlText( GetControl(pf, EDT_WIDTH ), buffer, sizeof( buffer ) );
				w = (uint32_t)IntCreateFromText( buffer );
				GetControlText( GetControl(pf, EDT_HEIGHT ), buffer, sizeof( buffer ) );
				h = (uint32_t)IntCreateFromText( buffer );
				SizeFrame( frame, w, h );
				GetControlText( GetControl(pf, EDT_ID ), buffer, sizeof( buffer ) );
				id = (uint32_t)IntCreateFromText( buffer );
				SetControlID( frame, id );
			}
			DestroyCommon( &pf );
		}
		break;
	}
	Release( pEditProps );
	return 1;
}

//---------------------------------------------------------------------------

void DetachChildFrames( PSI_CONTROL pc )
{
	// LOL this will be fun... watch the world fall to peiced
	if( pc && pc->child )
	{
		for( pc = pc->child; pc; pc = pc->next )
		{
			if( pc->BeginEdit )
			{
				// this allows the sheet control to detach the sheets before edit begins.
				pc->BeginEdit( pc );
			}
			DetachChildFrames( pc );
			if( !pc->flags.private_control &&
				 pc->nType == CONTROL_FRAME )
			{
				//DebugBreak();
				pc->detached_at = pc->original_rect;
				DisplayFrame( pc );
				EditFrame( pc, TRUE );
				pc->flags.detached = 1;
			}
		}
	}
}

//---------------------------------------------------------------------------

void RetachChildFrames( PSI_CONTROL pc )
{
	// LOL this will be fun... watch the world fall to peiced
	if( pc )
	{
		PSI_CONTROL parent = pc->parent;
		for( pc = pc; pc; pc = pc->next )
		{
			if( pc->child )
				RetachChildFrames( pc->child );
			if( pc->flags.detached )
			{
				uint32_t BorderType = pc->device->EditState.BorderType;
				// adopt should close the device, if any, and quietly
				// insert this control into it's parent
				if( parent )
					AdoptCommonEx( parent, NULL, pc, FALSE );
				SetCommonBorder( pc, BorderType );
				//DebugBreak(); // this is a crazy surface rect thing...
				MoveSizeCommon( pc
								  , pc->detached_at.x
								  , pc->detached_at.y
								  , pc->detached_at.width
								  , pc->detached_at.height);
				//DisplayFrame( pc );
				//EditFrame( pc, TRUE );
			}
			if( pc->EndEdit )
			{
				// this allows the sheet control to detach the sheets before edit begins.
				pc->EndEdit( pc );
			}
		}
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, EditFrame )( PSI_CONTROL pc, int bEnable )
{
	if( !pc || pc->flags.bNoEdit )
	{
		//lprintf( "!\n!\n!\n!\n!\n! XML SAVED FILE HAS EDIT=0 !\n!\n!\n!\n!\n!\n!\n!\n!" );
		return;
	}
	if( bEnable && pc->flags.detached )
	{
		xlprintf(LOG_NOISE+100)( WIDE("Already in edit mode, don't be stupid.") );
		return;
	}

	if( !bEnable )
	{
		if( pc->flags.detached )
		{
			while( pc && pc->parent) // && pc->device && pc->device->EditState.flags.bActive )
				pc = pc->parent;
		}
	}
	if( bEnable )
	{
		// this is initialization code, and should be moved external....
		//
		if( bEnable && !pFrameEditMenu )
	{
			PMENU pControls;
			PMENU pCurrentControls;
			CTEXTSTR name;
			int nControls = 0;
			PCLASSROOT data = NULL;
			pFrameEditMenu = CreatePopup();
			pCurrentControls = pControls = CreatePopup();
			for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY WIDE("/control"), &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				PMENUITEM pli;
				TEXTCHAR text[256];
				int nItem;
				int n;
				for( n = 0; name[n]; n++ )
					if( name[n] < '0' || name[n] > '9' )
						break;
				if( name[n] )
				{
					tnprintf( text, sizeof( text ), PSI_ROOT_REGISTRY WIDE("/control/%s"), name );
					nItem = GetRegisteredIntValue( text, WIDE("Type") );
					if( nControls == 10 )
					{
						PMENU tmp = CreatePopup();
						pli = AppendPopupItem( pCurrentControls, MF_POPUP, (uintptr_t)tmp, WIDE("More...") );
						pCurrentControls = tmp;
					}
					pli = AppendPopupItem( pCurrentControls, MF_BYCOMMAND, MNU_ADDCONTROL + nItem, (POINTER)name );
					nControls++;
					//SetItemData( pli, (uintptr_t)GetRegisteredProcedure( PSI_ROOT_REGISTRY WIDE("/control/Button/Click"), int, name, (uintptr_t, PSI_CONTROL) ) );
				}
			}
			AppendPopupItem( pFrameEditMenu, MF_POPUP, (uintptr_t)pControls, WIDE("Add Control") );
			AppendPopupItem( pFrameEditMenu, MF_STRING, MNU_EDITTHING, WIDE("Properties") );
			AppendPopupItem( pFrameEditMenu, MF_STRING, MNU_SAVEFRAME, WIDE("Save") );
			AppendPopupItem( pFrameEditMenu, MF_STRING, MNU_DONE, WIDE("Done") );
			AppendPopupItem( pFrameEditMenu, MF_STRING, MNU_DELETETHING, WIDE("Cancel") );
		}
		if( bEnable && !pControlEditMenu )
		{
			pControlEditMenu = CreatePopup();
			AppendPopupItem( pControlEditMenu, MF_STRING, MNU_EDITTHING, WIDE("Properties") );
			AppendPopupItem( pControlEditMenu, MF_STRING, MNU_DELETETHING, WIDE("Delete") );
			AppendPopupItem( pControlEditMenu, MF_STRING, MNU_DONE, WIDE("Done") );
		}
	}
	{
		PPHYSICAL_DEVICE pf = pc->device;
		if( pf )
		{
			if( !bEnable )
			{
				if( pf->EditState.pCurrent )
				{
					lprintf( WIDE("Restoring keyproc to control.") );
					pf->EditState.pCurrent->n_KeyProc = pf->EditState.n_KeyProc;
					pf->EditState.pCurrent->_KeyProc = pf->EditState._KeyProc;
				}
			}
		}
		else
		{
			pc->flags.bEditSet = TRUE;
			pc->flags.bNoEdit = !bEnable;
		}
	}
#if 0
	// this is code that is not init...
	if( bEnable )
	{
		// LOL this will be fun... watch the world fall to peiced
		DetachChildFrames( pc );
	}
	else
		RetachChildFrames( pc );
#endif
	if( bEnable && !pc->device )
	{
		if( !pc->flags.auto_opened )
		{
			pc->flags.auto_opened = 1;
			DisplayFrame( pc );
		}
	}
	if( !bEnable && pc->device )
	{
		if( pc->flags.auto_opened )
		{
			DetachFrameFromRenderer( pc );
			pc->flags.auto_opened = 0;
		}
	}
	{
		PPHYSICAL_DEVICE pf = pc->device;
		if( pf )
		{
			//lprintf( WIDE("Border type for control is %08") _32fx, pc->BorderType );
			pf->EditState.BorderType = pc->BorderType;
			pf->EditState.flags.bActive = bEnable;
			if( bEnable )
			{
				SetCommonBorder( pc, BORDER_FRAME|BORDER_NORMAL|BORDER_RESIZABLE );
			}
			else
			{
				if( pc->pCommonButtonData.flags.bWaitOnEdit )
				{
					// clear wait.
					pc->pCommonButtonData.flags.bWaitOnEdit = 0;
					WakeThread( pc->pCommonButtonData.thread );
				}
			}
		}
		SmudgeCommon( pc );
	}
}

PSI_PROP_NAMESPACE_END


//---------------------------------------------------------------------------
//
// $Log: ctlprop.c,v $
// Revision 1.24  2005/03/12 23:31:21  panther
// Edit controls nearly works... have some issues with those dang popups.
//
// Revision 1.23  2005/03/07 00:03:04  panther
// Reformatting, removed a lot of superfluous logging statements.
//
// Revision 1.22  2005/02/09 21:23:44  panther
// Update macros and function definitions to follow the common MakeControl parameter ordering.
//
// Revision 1.21  2005/01/08 00:00:15  panther
// Break up submenus for adding controls, fix simple message box
//
// Revision 1.20  2004/12/20 19:45:15  panther
// Fixes for protected sheet control init
//
// Revision 1.19  2004/12/05 10:48:21  panther
// Fix palette result color.  Fix check button for set alpha.  To make a radio/check button, 4 parameters, not two are needed.  Edit controls respond to mouse better.  Copy and paste function returned, no right click on edit fields :(
//
// Revision 1.18  2004/12/05 00:22:44  panther
// Fix focus flag reference and edit controls begin to work.  Sheet controls are still flaky
//
// Revision 1.17  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.6  2004/10/12 08:10:51  d3x0r
// checkpoint... frames are controls, and anything can be displayed...
//
// Revision 1.5  2004/10/07 04:37:16  d3x0r
// Okay palette and listbox seem to nearly work... controls draw, now about that mouse... looks like my prior way of cheating is harder to step away from than I thought.
//
// Revision 1.4  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.3  2004/09/28 16:52:18  d3x0r
// compiles - might work... prolly not.
//
// Revision 1.2  2004/09/27 20:44:28  d3x0r
// Sweeping changes only a couple modules left...
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.15  2004/09/04 18:49:48  d3x0r
// Changes to support scaling and font selection of dialogs
//
// Revision 1.14  2004/08/25 15:01:06  d3x0r
// Checkpoint - more vc compat fixes
//
// Revision 1.13  2004/06/16 03:02:50  d3x0r
// checkpoint
//
// Revision 1.12  2004/05/23 09:50:44  d3x0r
// Updates to extend dynamic edit dialogs.
//
// Revision 1.11  2004/05/22 00:39:57  d3x0r
// Lots of progress on dynamic editing of frames.
//
// Revision 1.12  2004/05/22 00:42:20  jim
// Score - specific property pages will work also.
//
// Revision 1.11  2004/05/21 21:52:36  jim
// Create control code creates in the right place now.
//
// Revision 1.10  2004/05/21 18:12:59  jim
// Checkpoint, need to add registered functions to link to.
//
// Revision 1.9  2004/05/21 16:23:15  jim
// Fixed issues with load/save
//
// Revision 1.10  2004/05/21 15:39:38  d3x0r
// Fixed some warnings, other issues with loading controls.
//
// Revision 1.9  2004/05/21 07:48:10  d3x0r
// track popup takes a PSI_CONTROL NOT a PSI_CONTROL
//
// Revision 1.8  2004/01/31 01:30:20  d3x0r
// Mods to extend/test procreglib.
//
// Revision 1.7  2004/01/29 18:01:34  d3x0r
// Generalize get/set control text functions, reformat
//
// Revision 1.6  2004/01/29 11:17:46  d3x0r
// Generalized first registered names...
//
// Revision 1.5  2004/01/26 23:47:25  d3x0r
// Misc edits.  Fixed filemon.  Export net startup, added def to edit frame
//
// Revision 1.4  2004/01/16 17:02:35  d3x0r
// move frame/control save/load external - working on properties, editing
//
// Revision 1.3  2003/11/29 00:10:28  panther
// Minor fixes for typecast equation
//
// Revision 1.2  2003/09/18 12:14:49  panther
// MergeRectangle Added.  Seems Control edit near done (fixing move/size errors)
//
// Revision 1.1  2003/09/18 08:00:53  panther
// Fix ctlprop so it compiles...
//
//
