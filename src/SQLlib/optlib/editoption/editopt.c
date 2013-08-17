//#define DEFINE_DEFAULT_RENDER_INTERFACE
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

typedef struct list_fill_tag
{
	struct {
		_32 bSecondLevel : 1;
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
		_32 bOpened : 1;
	} flags;
	POPTION_TREE_NODE ID_Value;
	char description[128];
	POPTION_TREE_NODE ID_Option; // lookin for parent things...
	CTEXTSTR option_text;
	_32 nLevel;
	PLISTITEM pli_fake;
} NODE_DATA, *PNODE_DATA;

static POPTION_TREE tree;
static PNODE_DATA last_node;
static POPTION_TREE_NODE last_option;
static TEXTCHAR last_value[256];

int CPROC FillList( PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags );

void CPROC ListItem( PTRSZVAL psv, PCOMMON pc, PLISTITEM pli )
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
		EnumOptionsEx( lf.odbc, pnd->ID_Option, FillList, (PTRSZVAL)&lf );
		pnd->flags.bOpened = TRUE;
		pnd->pli_fake = NULL;
	}
}

void CPROC HandleItemOpened( PTRSZVAL psv, PCONTROL pc, PLISTITEM pli, LOGICAL bOpened )
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

int CPROC FillList( PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
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

		SetItemData( hli,(PTRSZVAL)pnd );
	}
	if( !plf->flags.bSecondLevel )
		EnumOptionsEx( lf.odbc, ID, FillList, (PTRSZVAL)&lf );
	plf->pLastItem = lf.pLastItem; 
	//lprintf( WIDE("done with all children under this node.") );
	return TRUE;
}



PUBLIC( int, InitOptionList )( PODBC odbc, PCONTROL pc, _32 ID )
{
	LISTFILL lf;
	lf.flags.bSecondLevel = 0;
	lf.pcList = pc;
	lf.nLevel = 0;
	lf.pLastItem = NULL;
	lf.odbc = odbc;
	EnableCommonUpdates( pc, FALSE );
	EnumOptionsEx( odbc, NULL, FillList, (PTRSZVAL)&lf );
	EnableCommonUpdates( pc, TRUE );
	SmudgeCommon( pc );
	return 0;
}


static void CPROC OptionSelectionChanged( PTRSZVAL psvUser, PCONTROL pc, PLISTITEM hli )
{
	static TEXTCHAR buffer[4096];
	PNODE_DATA pnd = (PNODE_DATA)GetItemData( hli );
	last_option = pnd->ID_Option;
	last_node = pnd;
	if( pnd->option_text )
	{
		if( !pnd->ID_Option )
			pnd->ID_Option = GetOptionIndexExx( (PODBC)psvUser, NULL, NULL, pnd->option_text, NULL, FALSE DBG_SRC );
		GetOptionStringValueEx( (PODBC)psvUser, pnd->ID_Option, buffer, sizeof( buffer ) DBG_SRC );
		StrCpyEx( last_value, buffer, sizeof(last_value)/sizeof(last_value[0]) );
		SetCommonText( GetNearControl( pc, EDT_OPTIONVALUE ), buffer );
	}
	else
	{
		if( pnd->ID_Value )
		{
			lprintf( WIDE("Set value to real value.") );
			GetOptionStringValueEx( (PODBC)psvUser, pnd->ID_Value, buffer, sizeof( buffer ) DBG_SRC );
			StrCpyEx( last_value, buffer, sizeof(last_value)/sizeof(last_value[0]) );
			SetCommonText( GetNearControl( pc, EDT_OPTIONVALUE ), buffer );
		}
		else
		{
			lprintf( WIDE("Set to blank value - no value on branch.") );
			last_value[0] = 0;
			SetCommonText( GetNearControl( pc, EDT_OPTIONVALUE ), WIDE("") );
		}
	}
}

static void CPROC UpdateValue( PTRSZVAL psv, PCOMMON pc )
{
	TEXTCHAR value[256];
	GetControlText( GetNearControl( pc, EDT_OPTIONVALUE ), value, sizeof(value) );
	if( StrCmp( value, last_value ) != 0 )
	{
		POPTION_TREE tree = GetOptionTreeExxx( (PODBC)psv, NULL DBG_SRC );
		SetOptionStringValue( tree, last_node->ID_Option, value );
	}
}

static void CPROC ResetButton( PTRSZVAL psv, PCOMMON pc )
{
	ResetList( GetNearControl( pc, LST_OPTIONMAP ) );
	ResetOptionMap( (PODBC)psv );
	last_option = NULL;
	InitOptionList( (PODBC)psv, GetNearControl( pc, LST_OPTIONMAP ), LST_OPTIONMAP );
}

static void CPROC DeleteBranch( PTRSZVAL psv, PCOMMON pc )
{
	if( last_option )
		DeleteOption( last_option );
	ResetList( GetNearControl( pc, LST_OPTIONMAP ) );
	ResetOptionMap( (PODBC)psv );
	last_option = NULL;
	InitOptionList( (PODBC)psv, GetNearControl( pc, LST_OPTIONMAP ), LST_OPTIONMAP );
}

static void CPROC CopyBranch( PTRSZVAL psv, PCOMMON pc )
{
	TEXTCHAR result[256];
	// there's a current state already ...
	//GetCurrentSelection( );
	if( SimpleUserQuery( result, sizeof( result ), WIDE("Enter New Branch Name"), GetFrame( pc ) ) )
	{
		DuplicateOption( last_option, result );
	}
	ResetList( GetNearControl( pc, LST_OPTIONMAP ) );
	ResetOptionMap( (PODBC)psv );
	last_option = NULL;
	InitOptionList( (PODBC)psv, GetNearControl( pc, LST_OPTIONMAP ), LST_OPTIONMAP );
}

static void CPROC CreateEntry( PTRSZVAL psv, PCOMMON pc )
{
	TEXTCHAR result[256];
	// there's a current state already ...
	//GetCurrentSelection( );
	if( SimpleUserQuery( result, sizeof( result ), WIDE("Enter New Branch Name"), GetFrame( pc ) ) )
	{
		GetOptionIndexExx( (PODBC)psv, last_option, result, NULL, NULL, TRUE DBG_SRC );
		//DuplicateOption( last_option, result );
		ResetList( GetNearControl( pc, LST_OPTIONMAP ) );
		ResetOptionMap( (PODBC)psv );
		last_option = NULL;
		InitOptionList( (PODBC)psv, GetNearControl( pc, LST_OPTIONMAP ), LST_OPTIONMAP );
	}
}

static void CPROC FindEntry( PTRSZVAL psv, PCOMMON pc );

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
		SetSelChangeHandler( list, OptionSelectionChanged, (PTRSZVAL)odbc );
		SetListItemOpenHandler( list, HandleItemOpened, (PTRSZVAL)odbc );
		MakeEditControl( frame, RIGHT_START, 35, 175, 25, EDT_OPTIONVALUE, WIDE("blah"), 0 );

		pc = MakeButton( frame, RIGHT_START, 95, 150, 25, BTN_UPDATE, WIDE("Update"), 0, 0, 0  );
		SetButtonPushMethod( pc, UpdateValue, (PTRSZVAL)odbc );
		if( tree )
		{
			pc = MakeButton( frame, RIGHT_START, 125, 150, 25, BTN_FIND, WIDE("Find Entries"), 0, 0, 0  );
			SetButtonPushMethod( pc, FindEntry, (PTRSZVAL)odbc );
			pc = MakeButton( frame, RIGHT_START, 155, 150, 25, BTN_CREATE, WIDE("Make Entry"), 0, 0, 0  );
			SetButtonPushMethod( pc, CreateEntry, (PTRSZVAL)odbc );
		}
 		pc = MakeButton( frame, RIGHT_START, 185, 150, 25, BTN_COPY, WIDE("Copy"), 0, 0, 0  );
		SetButtonPushMethod( pc, CopyBranch, (PTRSZVAL)odbc );
		pc = MakeButton( frame, RIGHT_START, 215, 150, 25, BTN_DELETE, WIDE("Delete"), 0, 0, 0  );
		SetButtonPushMethod( pc, DeleteBranch, (PTRSZVAL)odbc );
		pc = MakeButton( frame, RIGHT_START, 245, 150, 25, BTN_DELETE, WIDE("Reset"), 0, 0, 0  );
		SetButtonPushMethod( pc, ResetButton, (PTRSZVAL)odbc );
		AddCommonButtonsEx( frame, done, WIDE("Done"), NULL, NULL );
	}
	return frame;
}

static void CPROC FindEntry( PTRSZVAL psv, PCOMMON pc )
{
	TEXTCHAR result[256];
	// there's a current state already ...
	//GetCurrentSelection( );
	if( SimpleUserQuery( result, sizeof( result ), WIDE("Enter Option Name to Find"), GetFrame( pc ) ) )
	{
		CTEXTSTR name;
		INDEX idx;
		PLIST options = NULL;
		// this is a magic function.
		FindOptions( (PODBC)psv, &options, result );
		if( !options )
		{
			SimpleMessageBox( NULL, "No Options Found", "Could not find any matching options" );
			return;
		}
		LIST_FORALL( options, idx, CTEXTSTR, name )
		{
			lprintf( "Found : %s", name );
		}

		{
			int done = 0;
			PSI_CONTROL frame = CreateOptionFrame( (PODBC)psv, FALSE, &done );
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

				SetItemData( hli,(PTRSZVAL)pnd );

				lprintf( "Found : %s", name );
			}
			//InitOptionList( odbc, GetControl( frame, LST_OPTIONMAP ), LST_OPTIONMAP );
			DisplayFrame( frame );
			CommonWait( frame );
			DestroyFrame( &frame );
		}

	}
}

int EditOptions( PODBC odbc )
{
	PCOMMON frame;// = LoadFrame( WIDE("edit.frame"), NULL, NULL, 0 );
	int done = FALSE;


	//if( !frame )
	frame = CreateOptionFrame( odbc, TRUE, &done );
	InitOptionList( odbc, GetControl( frame, LST_OPTIONMAP ), LST_OPTIONMAP );

	DisplayFrame( frame );
	CommonWait( frame );

	lprintf( "frame is now %p", frame );
	WakeableSleep( 300 );
	lprintf( "frame is now %p", frame );
	DestroyFrame( &frame );
	return 1;
}

SaneWinMain( argc, argv )
{
	PODBC o = NULL;
	if( argc > 1 )
	{
		int arg_ofs = 0;
		if( argv[1][0] == '-' && argv[1][1] == 'o' )
		{
			o = GetOptionODBC( argv[2], 1 );
		}
		else if( argv[1][0] == '-' && argv[1][1] == 'n' )
		{
			o = GetOptionODBC( argv[2], 4 );
		}
		else if( argv[1][0] == '-' && argv[1][1] == '2' )
		{
			o = GetOptionODBC( argv[2], 2 );
		}
		else
		{
			o = GetOptionODBC( argv[1], 2 );
		}
	}
	else
		o = GetOptionODBC( NULL, 0 );
	EditOptions( o );
	return 0;
}
EndSaneWinMain()
