#include <sack_types.h>

#ifndef ACCOUNT_STRUCTURES_DEFINED
#include "accstruc.h"
#endif

PACCOUNT LoginEx( PCLIENT pc, char *user, uint32_t dwIP, uint32_t version DBG_PASS );
#define Login( pc, user, ip, ver ) LoginEx( pc, user, ip, ver DBG_SRC )
void Logout( PACCOUNT current, PCLIENT_CONNECTION pcc );
void ReadAccounts( const char *configname );
//int NextChange( PCLIENT_CONNECTION pcc );
int SendChange( PACCOUNT account, uint32_t start, uint32_t size );
char *GetAccountBuffer( PCLIENT_CONNECTION account, int length );
void UpdateAccountFile( PACCOUNT account, int start, int size, PNETWORK_STATE pns );
int OpenFileOnAccount( PNETWORK_STATE pns
                       , uint32_t PathID
                       , char *filename
                       , uint32_t size
                       , FILETIME time_create
                       , FILETIME time_modify
                       , FILETIME time_access
                       , uint32_t *crc
                       , uint32_t crclen );

void CloseCurrentFile( PACCOUNT account, PNETWORK_STATE pns );

int CheckDirectoryOnAccount( PACCOUNT account
                           , PDIRECTORY pDir
									, char *filename );

void CloseAllAccounts( void );

int SendFileChange( PACCOUNT account, PCLIENT pc, uint32_t PathID, uint32_t ID, char *file, uint32_t start, uint32_t length );

// callback to handle ScanFile
void CPROC ProcessScannedFile( uintptr_t psv, CTEXTSTR name, int flags );

void SendTimeEx( PCLIENT pc, int bExtended );
int NextChange( PCLIENT_CONNECTION pcc );

LOGICAL ProcessManifest( PNETWORK_STATE pns, PACCOUNT account, uint32_t *buffer, size_t length );
void ProcessFileChanges( PACCOUNT account, PCLIENT_CONNECTION pcc );
void ScanForDeletes( PACCOUNT account );
void ExpandManifest( PACCOUNT account, LOGICAL mark_deleted );
int ReadValidateCRCs( PCLIENT_CONNECTION pcc, uint32_t *crc, size_t crclen
						  , char *name, size_t finalsize
						  , PFILE_INFO pFileInfo
						  );
