
#include "sack_ucb_filelib.h"
#include "fileio.h"


#ifdef __cplusplus
namespace d3x0r {
namespace ppc {
#endif

enum define_type {
	DEFINE_ALL,
	DEFINE_COMMANDLINE,
	DEFINE_FILE,
	DEFINE_INTERNAL,
	DEFINE_EXTERNAL
};

typedef struct define_tag {
	PTEXT pName;
	enum define_type nType;      // DEFINE_FILE, COMMANDLINE, etc..
	PLIST pParams;  // list of parameter names
	int bVarParams; // set if there was a ... parameter...
	PTEXT pData;    // content which is to be substituted
	int bUsed;      // set to avoid circular substitution
	// file and line which originally made this define...
	char pFile[ __MAX_PATH__ ];
	int nLine;
	void ( *cbExternal )( uintptr_t psvProcessInfo, PLIST pArgs, PTEXT *pOutput );
	struct define_tag *pLesser, *pGreater, *pSame, **me;
} DEF, *PDEF;

void InitDefines( void );
void DeinitDefines( void );
void CommitDefinesToCommandLine( void );

#define IGNORE_PARAMS 0x7fff
PDEF FindDefineName( PTEXT pName, int params );


void DeleteDefine( PDEF *ppDef );
void DeleteAllDefines( int type );
PDEF DefineDefine( char *name, char *value );
int ProcessDefine( enum define_type type );
INDEX FindArg( PLIST pArgs, PTEXT pName );
void EvalSubstitutions( PTEXT *subst, int more );

void FixQuoting( PTEXT test );

#ifdef __cplusplus

} // namespace ppc
} // namespace d3x0r
#endif