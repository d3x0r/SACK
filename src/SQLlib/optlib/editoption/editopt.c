// need to check if is instanced.
#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <controls.h>
#include <idle.h>
#include <sqlgetoption.h>
#include <sharemem.h>

#define BTN_FIND 1008
#define BTN_CREATE 1007
#define BTN_RESET 1006
#define BTN_UPDATE 1005
#define TXT_DESCRIPTION 1004
#define BTN_DELETE 1003
#define BTN_COPY 1002
#define EDT_OPTIONVALUE 1001
#define LST_OPTIONMAP 1000

struct query_params 
{
	PSI_CONTROL pc;
	TEXTSTR result;
};


typedef struct list_fill_tag
{
	struct {
		uint32_t bSecondLevel : 1;
	} flags;
	PCONTROL pcList;
	int nLevel;
	PLISTITEM pLastItem;
	PODBC odbc;
} LISTFILL, *PLISTFILL;

typedef struct node_data_tag
{
	struct {
		// if it has not been opened, then there is a fake item under....
		uint32_t bOpened : 1;
	} flags;
	POPTION_TREE_NODE ID_Value;
	char description[128];
	POPTION_TREE_NODE ID_Option; // lookin for parent things...
	CTEXTSTR option_text;
	uint32_t nLevel;
	PLISTITEM pli_fake;
} NODE_DATA, *PNODE_DATA;

struct instance_local
{
	POPTION_TREE tree;
	PNODE_DATA last_node;
	POPTION_TREE_NODE last_option;
	TEXTCHAR last_value[256];
	int done1;
	int done2;
	int done3;
};


#if HAS_TLS
DeclareThreadLocal struct instance_local *option_thread;
#define l (*option_thread)
#else
struct instance_local *option_thread;
#define l (*option_thread)
#endif

// to support older interface
struct instance_local *default_local;

int CPROC FillList( uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags );

void CPROC ListItem( uintptr_t psv, PCOMMON pc, PLISTITEM pli )
{
	PNODE_DATA pnd = (PNODE_DATA)GetItemData( pli );
	if( !pnd->flags.bOpened )
	{
		LISTFILL lf;
		lf.flags.bSecondLevel = 0;
		lf.pcList = pc;
		lf.nLevel = pnd->nLevel;
		lf.pLastItem = pli;
		lf.odbc = (PODBC)psv;
		//DeleteListItem( pc, pnd->pli_fake );
		EnumOptionsEx( lf.odbc, pnd->ID_Option, FillList, (uintptr_t)&lf );
		pnd->flags.bOpened = TRUE;
		pnd->pli_fake = NULL;
	}
}

void CPROC HandleItemOpened( uintptr_t psv, PCONTROL pc, PLISTITEM pli, LOGICAL bOpened )
{
	if( bOpened )
	{
		PNODE_DATA pnd = (PNODE_DATA)GetItemData( pli );
		if( pnd )
		{
			if( pnd->flags.bOpened )
				return;
			else
			{
				EnumListItems( pc, pli, ListItem, psv );
				pnd->flags.bOpened = TRUE;
			}
		}
	}
}

int CPROC FillList( uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
{
	PLISTFILL plf = (PLISTFILL)psv;
	LISTFILL lf = *plf;
	PLISTITEM hli;
	lf.nLevel++;
	lf.flags.bSecondLevel = 1;
	//lprintf( WIDE("%d - %s (%p)"), plf->nLevel, name, ID );
	lf.pLastItem = hli = InsertListItemEx( plf->pcList, plf->pLastItem, plf->nLevel, name );
	{
		PNODE_DATA pnd = New(NODE_DATA);//Allocate( sizeof( NODE_DATA ) );
		pnd->flags.bOpened = FALSE;
		//pnd->nListIndex = GetItemIndex( pc, hli );
		pnd->option_text = NULL;
		pnd->ID_Value = ID;//GetOptionValueIndex( ID );
		pnd->ID_Option = ID;
		pnd->nLevel = lf.nLevel;
		pnd->pli_fake = 0; //InsertListItemEx( plf->pcList, hli, plf->nLevel+1, WIDE("fake") );

		SetItemData( hli,(uintptr_t)pnd );
	}
	if( !plf->flags.bSecondLevel )
		EnumOptionsEx( lf.odbc, ID, FillList, (uintptr_t)&lf );
	plf->pLastItem = lf.pLastItem; 
	//lprintf( WIDE("done with all children under this node.") );
	return TRUE;
}



PUBLIC( int, InitOptionList )( PODBC odbc, PCONTROL pc, uint32_t ID )
{
	LISTFILL lf;
	lf.flags.bSecondLevel = 0;
	lf.pcList = pc;
	lf.nLevel = 0;
	lf.pLastItem = NULL;
	lf.odbc = odbc;
	EnableCommonUpdates( pc, FALSE );
	EnumOptionsEx( odbc, NULL, FillList, (uintptr_t)&lf );
	EnableCommonUpdates( pc, TRUE );
	SmudgeCommon( pc );
	return 0;
}


static void CPROC OptionSelectionChanged( uintptr_t psvUser, PCONTROL pc, PLISTITEM hli )
{
	//static TEXTCHAR buffer[4096];
	PNODE_DATA pnd = (PNODE_DATA)GetItemData( hli );
	if( !option_thread )
		option_thread = default_local;
	l.last_option = pnd->ID_Option;
	l.last_node = pnd;
	if( pnd->option_text )
	{
		size_t buflen;
		char *buffer;
		if( !pnd->ID_Option )
			pnd->ID_Option = GetOptionIndexExx( (PODBC)psvUser, NULL, pnd->option_text, NULL, NULL, NULL, FALSE, FALSE DBG_SRC );
		GetOptionStringValueEx( (PODBC)psvUser, pnd->ID_Option, &buffer, &buflen DBG_SRC );
		StrCpyEx( l.last_value, buffer, buflen );
		SetControlText( GetNearControl( pc, EDT_OPTIONVALUE ), buffer );
	}
	else
	{
		if( pnd->ID_Value )
		{
			//lprintf( WIDE("Set value to real value.") );
			char *buffer;
			size_t buflen;
			GetOptionStringValueEx( (PODBC)psvUser, pnd->ID_Value, &buffer, &buflen DBG_SRC );
			StrCpyEx( l.last_value, buffer, buflen );
			SetControlText( GetNearControl( pc, EDT_OPTIONVALUE ), buffer );
		}
		else
		{
			//lprintf( WIDE("Set to blank value - no value on branch.") );
			l.last_value[0] = 0;
			SetControlText( GetNearControl( pc, EDT_OPTIONVALUE ), WIDE("") );
		}
	}
}

static void CPROC UpdateValue( uintptr_t psv, PCOMMON pc )
{
	if( !option_thread )
		option_thread = default_local;
	if( l.last_node )
	{
		TEXTCHAR value[256];
		GetControlText( GetNearControl( pc, EDT_OPTIONVALUE ), value, sizeof(value) );
		if( StrCmp( value, l.last_value ) != 0 )
		{
			POPTION_TREE tree = GetOptionTreeExxx( (PODBC)psv, NULL DBG_SRC );
			SetOptionStringValue( tree, l.last_node->ID_Option, value );
		}
	}
}

static void CPROC ResetButton( uintptr_t psv, PCOMMON pc )
{
	if( !option_thread )
		option_thread = default_local;
	ResetList( GetNearControl( pc, LST_OPTIONMAP ) );
	ResetOptionMap( (PODBC)psv );
	l.last_option = NULL;
	InitOptionList( (PODBC)psv, GetNearControl( pc, LST_OPTIONMAP ), LST_OPTIONMAP );
}

static void CPROC DoneButton( uintptr_t psv, PCOMMON pc )
{
	PSI_CONTROL pc_frame = (PSI_CONTROL)GetParentControl( pc );
	DestroyFrame( &pc_frame );
}
static void CPROC DeleteBranch( uintptr_t psv, PCOMMON pc )
{
	if( !option_thread )
		option_thread = default_local;
	if( l.last_option )
		DeleteOption( l.last_option );
	ResetList( GetNearControl( pc, LST_OPTIONMAP ) );
	ResetOptionMap( (PODBC)psv );
	l.last_option = NULL;
	InitOptionList( (PODBC)psv, GetNearControl( pc, LST_OPTIONMAP ), LST_OPTIONMAP );
}

static void CPROC CopyBranchQueryResult( uintptr_t psv, LOGICAL success )
{
	struct query_params  *params = (struct query_params  *)psv;
	if( success )
		DuplicateOption( l.last_option, params->result );
	ResetList( GetNearControl( params->pc, LST_OPTIONMAP ) );
	ResetOptionMap( (PODBC)psv );
	l.last_option = NULL;
	InitOptionList( (PODBC)psv, GetNearControl( params->pc, LST_OPTIONMAP ), LST_OPTIONMAP );
}

static void CPROC CopyBranch( uintptr_t psv, PCOMMON pc )
{
	struct query_params  *params = New( struct query_params );
	params->pc = pc;
	params->result = NewArray( TEXTCHAR, 256 );

	// there's a current state already ...
	//GetCurrentSelection( );
	SimpleUserQueryEx( params->result, 256, WIDE("Enter New Branch Name"), GetFrame( pc ), CopyBranchQueryResult, (uintptr_t)params );
}

static void CPROC CreateEntryQueryResult( uintptr_t psv, LOGICAL success )
{
	struct query_params  *params = (struct query_params  *)psv;
	if( success )
	{
		GetOptionIndexExx( (PODBC)psv, l.last_option, NULL, params->result, NULL, NULL, TRUE, FALSE DBG_SRC );
		//DuplicateOption( l.last_option, result );
		ResetList( GetNearControl( params->pc, LST_OPTIONMAP ) );
		ResetOptionMap( (PODBC)psv );
		l.last_option = NULL;
		InitOptionList( (PODBC)psv, GetNearControl( params->pc, LST_OPTIONMAP ), LST_OPTIONMAP );
	}
	Release( params->result );
	Release( params );
}

static void CPROC CreateEntry( uintptr_t psv, PCOMMON pc )
{
	struct query_params  *params = New( struct query_params );
	params->pc = pc;
	params->result = NewArray( TEXTCHAR, 256 );

	// there's a current state already ...
	//GetCurrentSelection( );
	SimpleUserQueryEx( params->result, 256, WIDE("Enter New Branch Name"), GetFrame( pc ), CreateEntryQueryResult, (uintptr_t)params );
}

static void CPROC FindEntry( uintptr_t psv, PCOMMON pc );

static PSI_CONTROL CreateOptionFrame( PODBC odbc, LOGICAL tree, int *done )
{
	PSI_CONTROL frame;
	{
		PCOMMON pc;
		PCONTROL list;
#define SIZE_BASE 430
#define NEW_SIZE 720
#define LIST_SIZE           240 - SIZE_BASE + NEW_SIZE
#define RIGHT_START   250 - SIZE_BASE + NEW_SIZE
		frame = CreateFrame( WIDE("Edit Options"), -1, -1, NEW_SIZE, 320, BORDER_NORMAL, NULL );
		list = MakeListBox( frame, 5, 5, LIST_SIZE, 310, LST_OPTIONMAP, 0 );
		SetListboxIsTree( list, tree );
		SetSelChangeHandler( list, OptionSelectionChanged, (uintptr_t)odbc );
		SetListItemOpenHandler( list, HandleItemOpened, (uintptr_t)odbc );
		MakeEditControl( frame, RIGHT_START, 35, 175, 25, EDT_OPTIONVALUE, WIDE("blah"), 0 );

		pc = MakeButton( frame, RIGHT_START, 95, 150, 25, BTN_UPDATE, WIDE("Update"), 0, 0, 0  );
		SetButtonPushMethod( pc, UpdateValue, (uintptr_t)odbc );
		if( tree )
		{
			pc = MakeButton( frame, RIGHT_START, 125, 150, 25, BTN_FIND, WIDE("Find Entries"), 0, 0, 0  );
			SetButtonPushMethod( pc, FindEntry, (uintptr_t)odbc );
			pc = MakeButton( frame, RIGHT_START, 155, 150, 25, BTN_CREATE, WIDE("Make Entry"), 0, 0, 0  );
			SetButtonPushMethod( pc, CreateEntry, (uintptr_t)odbc );
		}
		pc = MakeButton( frame, RIGHT_START, 185, 150, 25, BTN_COPY, WIDE("Copy"), 0, 0, 0  );
		SetButtonPushMethod( pc, CopyBranch, (uintptr_t)odbc );
		pc = MakeButton( frame, RIGHT_START, 215, 150, 25, BTN_DELETE, WIDE("Delete"), 0, 0, 0  );
		SetButtonPushMethod( pc, DeleteBranch, (uintptr_t)odbc );
		pc = MakeButton( frame, RIGHT_START, 245, 150, 25, BTN_DELETE, WIDE("Reset"), 0, 0, 0  );
		SetButtonPushMethod( pc, ResetButton, (uintptr_t)odbc );
		if( !done ) {
			pc = MakeButton( frame, NEW_SIZE - 70, 320 - 40, 60, 25, IDCANCEL, WIDE( "Done" ), 0, 0, 0 );
			SetButtonPushMethod( pc, DoneButton, (uintptr_t)odbc );
		} else {
			AddCommonButtonsEx( frame, done, WIDE( "Done" ), NULL, NULL );
		}
	}
	return frame;
}

struct find_entry_external {
	TEXTSTR result;
	PSI_CONTROL frame;
	PODBC odbc;
};

static void CPROC FindEntryResult( uintptr_t psv, LOGICAL success )
{
	struct find_entry_external *params = (struct find_entry_external*)psv;
	TEXTSTR result = params->result;
	if( success )
	{
		CTEXTSTR name;
		INDEX idx;
		PLIST options = NULL;
		// this is a magic function.
		FindOptions( params->odbc, &options, result );
		if( !options )
		{
			SimpleMessageBox( params->frame, WIDE("No Options Found"), WIDE("Could not find any matching options") );
			Release( params->result );
			Release( params );
			return;
		}
		LIST_FORALL( options, idx, CTEXTSTR, name )
		{
			lprintf( WIDE("Found : %s"), name );
		}

		{
			int done = 0;
			PSI_CONTROL frame = CreateOptionFrame( params->odbc, FALSE, &done );
			PSI_CONTROL list = GetControl( frame, LST_OPTIONMAP );
			LIST_FORALL( options, idx, CTEXTSTR, name )
			{
				PLISTITEM hli = AddListItem( list, name );
				PNODE_DATA pnd = New(NODE_DATA);//Allocate( sizeof( NODE_DATA ) );
				pnd->flags.bOpened = TRUE;
				pnd->ID_Value = NULL;
				pnd->ID_Option = NULL;
				pnd->option_text = StrDup( name );
				pnd->nLevel = 0;
				pnd->pli_fake = 0; //InsertListItemEx( plf->pcList, hli, plf->nLevel+1, WIDE("fake") );

				SetItemData( hli,(uintptr_t)pnd );

				lprintf( WIDE("Found : %s"), name );
			}
			//InitOptionList( odbc, GetControl( frame, LST_OPTIONMAP ), LST_OPTIONMAP );
			DisplayFrame( frame );
			CommonWait( frame );
			DestroyFrame( &frame );
		}
	}
	Release( params->result );
	Release( params );
}

static void CPROC FindEntry( uintptr_t psv, PCOMMON pc )
{
	struct find_entry_external *params = New( struct find_entry_external );
	params->frame = GetFrame( pc );
	params->result = NewArray( TEXTCHAR, 256 );//[256];
	params->odbc = (PODBC)psv;
	// there's a current state already ...
	//GetCurrentSelection( );
	SimpleUserQueryEx( params->result, 256, WIDE("Enter Option Name to Find"), GetFrame( pc )
							, FindEntryResult, (uintptr_t)params );
}

static void OnDisplayConnect( WIDE( "EditOption Display" ) )( struct display_app * app, struct display_app_local***local )
{
	PSI_CONTROL frame;
	option_thread = New( struct instance_local );
	MemSet( option_thread, 0, sizeof( option_thread ) );
	(*local) = (struct display_app_local**)&option_thread;

	frame = CreateOptionFrame( NULL, TRUE, &l.done1 );
	InitOptionList( GetOptionODBC( NULL ), GetControl( frame, LST_OPTIONMAP ), LST_OPTIONMAP );
	DisplayFrame( frame );
}

#ifdef EDITOPTION_PLUGIN
PUBLIC( int, EditOptionsEx )
#else
int EditOptionsEx
#endif
                  ( PODBC odbc, PSI_CONTROL parent, LOGICAL wait )
{
	PCOMMON frame;// = LoadFrame( WIDE("edit.frame"), NULL, NULL, 0 );
	int done = FALSE;
	if( !odbc )
		odbc = GetOptionODBC( NULL );

	//if( !frame )
	if( !RenderIsInstanced() )
	{
		default_local = option_thread = New( struct instance_local );
		MemSet( option_thread, 0, sizeof( option_thread[0] ) );
		frame = CreateOptionFrame( odbc, TRUE, wait?&done:NULL );
		InitOptionList( odbc, GetControl( frame, LST_OPTIONMAP ), LST_OPTIONMAP );

		DisplayFrameOver( frame, parent );
		if( wait ) {
			CommonWait( frame );
			DestroyFrame( &frame );
		}
	}
	else
	{
		while( 1 )
		{
			WakeableSleep( 1000000 );
		}
	}
	return 1;
}

#ifdef EDITOPTION_PLUGIN
PUBLIC( int, EditOptions )
#else
int EditOptions
#endif
( PODBC odbc, PSI_CONTROL parent ) {
	EditOptionsEx( odbc, parent, FALSE );
}


#ifndef EDITOPTION_PLUGIN
SaneWinMain( argc, argv )
{
	PODBC o = NULL;
	if( argc > 1 )
	{
		o = GetOptionODBC( argv[1] );
	}
	else
		o = GetOptionODBC( NULL );
	EditOptionsEx( o, NULL, TRUE );
	return 0;
}
EndSaneWinMain()
#endif
