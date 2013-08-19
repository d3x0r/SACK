

// some inter module linkings...
// these are private for access between utils and the core interface lib

#define OPTION_ROOT_VALUE 0

typedef struct sack_option_tree_family OPTION_TREE;
//typedef struct sack_option_tree_family *POPTION_TREE;

typedef struct sack_option_tree_family_node OPTION_TREE_NODE;

struct sack_option_tree_family_node {
	INDEX id;
	INDEX name_id;
	INDEX value_id;
	CTEXTSTR name;
	CTEXTSTR guid;
	CTEXTSTR name_guid;
	CTEXTSTR value_guid;
	CTEXTSTR value;
	PFAMILYNODE node;
	struct {
		BIT_FIELD bExpanded : 1;
	} flags;
	PODBC uncommited_write; // connection this was written on for the commit event.
   _32 expansion_tick;
};
#define MAXOPTION_TREE_NODESPERSET 256
DeclareSet( OPTION_TREE_NODE );

struct sack_option_tree_family {
	POPTION_TREE_NODE root;
	PFAMILYTREE option_tree;
	PODBC odbc;  // each option tree associates with a ODBC connection.
	PODBC odbc_writer; // a second connection which handles all inserts and updates
	PFAMILYNODE system_mask_root;
	struct {
		BIT_FIELD bNewVersion : 1;
		BIT_FIELD bVersion4 : 1;
		BIT_FIELD bCreated : 1;
	} flags;
	PLIST uncommited; // list of option values that were written.
};

struct option_odbc_tracker
{
	CTEXTSTR name;
   int version;
	PLINKQUEUE available;
	PLIST outstanding;
   PFAMILYTREE shared_option_tree;
};

struct sack_option_global_tag {
	struct {
		BIT_FIELD bRegistered : 1;
		BIT_FIELD bInited : 1;
		BIT_FIELD bUseProgramDefault : 1;
		BIT_FIELD bUseSystemDefault : 1;
		BIT_FIELD bPromptDefault : 1;
		BIT_FIELD bEnableSystemMapping : 2;
   } flags;
   TEXTCHAR SystemName[128];
   INDEX SystemID;
   _32 Session;
	//PFAMILYTREE option_tree;
   PLIST trees; // list of struct sack_option_family_tree's
	PODBC Option; // primary ODBC for option use.
	CRITICALSECTION cs_option;
   PLIST odbc_list;
};

//INDEX GetOptionIndexEx( INDEX parent, CTEXTSTR file, CTEXTSTR pBranch, CTEXTSTR pValue, int bCreate DBG_PASS );
//#define GetOptionIndex( f,b,v ) GetOptionIndexEx( OPTION_ROOT_VALUE, f, b, v, FALSE DBG_SRC )
POPTION_TREE_NODE New4DuplicateValue( PODBC odbc, POPTION_TREE_NODE iOriginalValue, POPTION_TREE_NODE iNewValue );
CTEXTSTR New4ReadOptionNameTable( POPTION_TREE tree, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS );
void InitMachine( void );
#define GetOptionTreeExx(o,... )  GetOptionTreeExxx( o,NULL,##__VA_ARGS__ )
#define GetOptionTreeEx(o) GetOptionTreeExx(o DBG_SRC )
POPTION_TREE GetOptionTreeExxx( PODBC odbc, PFAMILYTREE existing_tree DBG_PASS );
//POPTION_TREE GetOptionTreeEx( PODBC odbc );

LOGICAL SetOptionValueEx( POPTION_TREE tree, POPTION_TREE_NODE optval );
//INDEX SetOptionValue( INDEX optval, INDEX iValue );

POPTION_TREE_NODE DuplicateValue( POPTION_TREE_NODE iOriginalValue, POPTION_TREE_NODE iNewValue );
POPTION_TREE_NODE NewDuplicateValue( PODBC odbc, POPTION_TREE_NODE iOriginalValue, POPTION_TREE_NODE iNewValue );
POPTION_TREE_NODE New4DuplicateValue( PODBC odbc, POPTION_TREE_NODE iOriginalValue, POPTION_TREE_NODE iNewValue );




//POPTION_TREE GetOptionTreeEx( PODBC odbc );
PFAMILYTREE* GetOptionTree( PODBC odbc );


POPTION_TREE_NODE NewGetOptionIndexExxx( PODBC odbc, POPTION_TREE_NODE parent, CTEXTSTR file, CTEXTSTR pBranch, CTEXTSTR pValue, int bCreate, int bIKnowItDoesntExist DBG_PASS );
POPTION_TREE_NODE New4GetOptionIndexExxx( PODBC odbc, POPTION_TREE_NODE parent, CTEXTSTR file, CTEXTSTR pBranch, CTEXTSTR pValue, int bCreate, int bIKnowItDoesntExist DBG_PASS );
size_t NewGetOptionStringValue( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len DBG_PASS );
size_t New4GetOptionStringValue( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len DBG_PASS );
LOGICAL NewCreateValue( POPTION_TREE odbc, POPTION_TREE_NODE value, CTEXTSTR pValue );
LOGICAL New4CreateValue( POPTION_TREE odbc, POPTION_TREE_NODE value, CTEXTSTR pValue );
void New4DeleteOption( PODBC odbc, POPTION_TREE_NODE iRoot );

void NewEnumOptions( PODBC odbc, POPTION_TREE_NODE parent
					 , int (CPROC *Process)(PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
												  , PTRSZVAL psvUser );
void New4EnumOptions( PODBC odbc, POPTION_TREE_NODE parent
					 , int (CPROC *Process)(PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
												  , PTRSZVAL psvUser );

void NewDeleteOption( PODBC odbc, POPTION_TREE_NODE iRoot );
void OpenWriterEx( POPTION_TREE option DBG_PASS);
#define OpenWriter(o) OpenWriterEx( o DBG_SRC )

INDEX ReadOptionNameTable( POPTION_TREE tree, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS );

LOGICAL SetOptionStringValue( POPTION_TREE tree, POPTION_TREE_NODE optval, CTEXTSTR pValue );

void NewDuplicateOption( PODBC odbc, POPTION_TREE_NODE iRoot, CTEXTSTR pNewName );
INDEX IndexCreateFromText( CTEXTSTR string );

int NewGetOptionBlobValueOdbc( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len );
void New4FindOptions( POPTION_TREE odbc, PLIST *result_list, CTEXTSTR name );

