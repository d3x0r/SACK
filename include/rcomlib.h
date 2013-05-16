#ifndef RCOM_LIBRARY_INCLUDED
#define RCOM_LIBRARY_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int GetRegistryItem( HKEY hRoot, TEXTCHAR *pPrefix, 
                     TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                     DWORD dwType,  
                     TEXTCHAR *nResult, int nSize );

int SetRegistryItem( HKEY hRoot, TEXTCHAR *pPrefix,
                     TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                     DWORD dwType,
                     TEXTCHAR *pValue, int nSize );


int GetRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pKey, int *Value );
int GetLocalRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pKey, int *Value );

int GetRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, TEXTCHAR *Value, int nMaxLen );
int GetLocalRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, TEXTCHAR *Value, int nMaxLen );

int SetRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pKey, int Value );
int SetLocalRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pKey, int Value );

int SetRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, TEXTCHAR *pValue );
int SetLocalRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, TEXTCHAR *pValue );

//---------------------------------------------------------------
// rc_dialog.c
BOOL SimpleQuery( TEXTCHAR *pText, TEXTCHAR *pResult, int nResultLen );
void CenterDialog( HWND hWnd );

BOOL OpenRegister( void );
void ShowRegister( void ); // show dialog...

//---------------------------------------------------------------
// register.cpp
int IsRegistered( TEXTCHAR *pProgramID, TEXTCHAR *pProgramName, int bSilent );
// return 0 - registered
// return 1 - not regitered
// return 2 - expired
// return 3 - did not accept liscensing
// return 4 - tampered / invalid regcode
// return ... more to come ... 

int DoRegister( void );
int  GetRegisterResult( void );
void GetRegisterError( TEXTCHAR *pResult );
int RegisterProduct( TEXTCHAR *pResult, TEXTCHAR *pProductID, TEXTCHAR *pProductName );

//---------------------------------------------------------------
// file.c
TEXTCHAR *GetFile( TEXTCHAR *pMatch, void *pi );


int ProcessRegistryKeys(HKEY root, CTEXTSTR pClass, int (CPROC*f)(PTRSZVAL psv, CTEXTSTR key), PTRSZVAL psv );

#ifdef __cplusplus
}
#endif

#endif
// $Log: $
