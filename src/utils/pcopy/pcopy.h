;
#include <sack_types.h>

typedef struct global_tag
{
	struct {
		BIT_FIELD bVerbose : 1;
		BIT_FIELD bDoNotCopy : 1;
		BIT_FIELD bWas32 : 1;
		BIT_FIELD bWas64 : 1;
		BIT_FIELD bIncludeSystem : 1;
	} flags;
	TEXTCHAR SystemRoot[256];
	_32 copied;
	PLIST excludes;
   PLIST additional_paths;
} GLOBAL;



typedef struct file_source
{
	struct
	{
		BIT_FIELD bScanned : 1;
		BIT_FIELD bInvalid : 1;
		BIT_FIELD bSystem : 1;
		BIT_FIELD bExternal : 1;
	} flags;
	TEXTSTR name;
   struct file_source *children, *next;
} FILESOURCE, *PFILESOURCE;

void AddFileCopy( CTEXTSTR name );

int ScanFile( PFILESOURCE pfs );
PFILESOURCE AddDependCopy( PFILESOURCE pfs, CTEXTSTR name );

void ScanFileCopyTree( void );
void CopyFileCopyTree( CTEXTSTR dest );

