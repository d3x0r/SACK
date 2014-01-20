#include <sack_types.h>

#ifndef ACCOUNT_STRUCTURES_DEFINED
#include "accstruc.h"
#endif

PACCOUNT LoginEx( PCLIENT pc, char *user, _32 dwIP, _32 version DBG_PASS );
#define Login( pc, user, ip, ver ) LoginEx( pc, user, ip, ver DBG_SRC )
void Logout( PACCOUNT current, PCLIENT_CONNECTION pcc );
void ReadAccounts( const char *configname );
//int NextChange( PCLIENT_CONNECTION pcc );
int SendChange( PACCOUNT account, _32 start, _32 size );
char *GetAccountBuffer( PCLIENT_CONNECTION account, int length );
void UpdateAccountFile( PACCOUNT account, int start, int size, PNETWORK_STATE pns );
int OpenFileOnAccount( PNETWORK_STATE pns
                       , _32 PathID
                       , char *filename
                       , _32 size
                       , FILETIME time_create
                       , FILETIME time_modify
                       , FILETIME time_access
                       , _32 *crc
                       , _32 crclen );

void CloseCurrentFile( PACCOUNT account, PNETWORK_STATE pns );

int CheckDirectoryOnAccount( PACCOUNT account
                           , PDIRECTORY pDir
									, char *filename );

void CloseAllAccounts( void );

int SendFileChange( PACCOUNT account, PCLIENT pc, _32 PathID, _32 ID, char *file, _32 start, _32 length );

// callback to handle ScanFile
void CPROC ProcessScannedFile( PTRSZVAL psv, CTEXTSTR name, int flags );

void SendTimeEx( PCLIENT pc, int bExtended );
int NextChange( PCLIENT_CONNECTION pcc );

LOGICAL ProcessManifest( PNETWORK_STATE pns, PACCOUNT account, _32 *buffer, size_t length );
void ProcessFileChanges( PACCOUNT account, PCLIENT_CONNECTION pcc );
void ScanForDeletes( PACCOUNT account );
void ExpandManifest( PACCOUNT account, LOGICAL mark_deleted );
int ReadValidateCRCs( PCLIENT_CONNECTION pcc, _32 *crc, size_t crclen
						  , char *name, size_t finalsize
						  , PFILE_INFO pFileInfo
						  );
