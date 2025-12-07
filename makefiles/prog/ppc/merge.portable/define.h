
#include "sack_ucb_filelib.h"
#include "fileio.h"

typedef struct define_tag
{
	PTEXT pName;
   int   nType; // DEFINE_FILE, COMMANDLINE, etc..
   PLIST pParams; // list of parameter names
	int   bVarParams; // set if there was a ... parameter...
   PTEXT pData;  // content which is to be substituted
   int   bUsed;  // set to avoid circular substitution
   // file and line which originally made this define...
   char  pFile[__MAX_PATH__]; 
   int   nLine;
   struct define_tag *pLesser, *pGreater, *pSame, **me;
} DEF, *PDEF;


void InitDefines( void );
void DeinitDefines( void );
void CommitDefinesToCommandLine( void );

#define IGNORE_PARAMS 0x7fff
PDEF FindDefineName( PTEXT pName, int params );

#define DEFINE_ALL         0
#define DEFINE_COMMANDLINE 1
#define DEFINE_FILE        2
#define DEFINE_INTERNAL    3

void  DeleteDefine( PDEF *ppDef );
void  DeleteAllDefines( int type );
void  DefineDefine( char *name, char *value );
int   ProcessDefine( int type );
INDEX FindArg( PLIST pArgs, PTEXT pName );
void  EvalSubstitutions( PTEXT *subst, int more );


void FixQuoting( PTEXT test );

