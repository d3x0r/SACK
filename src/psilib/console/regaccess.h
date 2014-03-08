
#ifdef __cplusplus
extern "C" {
#endif

int GetRegistryItem( HKEY hRoot, char *pPrefix,
                     char *pProduct, char *pKey,
                     DWORD dwType,
                     char *nResult, int nSize );

int SetRegistryItem( HKEY hRoot, char *pPrefix,
                     char *pProduct, char *pKey,
                     DWORD dwType,
                     char *pValue, int nSize );


int GetRegistryInt( char *pProduct, char *pKey, int *Value );
int GetLocalRegistryInt( char *pProduct, char *pKey, int *Value );

int GetRegistryString( char *pProduct, char *pKey, char *Value, int nMaxLen );
int GetLocalRegistryString( char *pProduct, char *pKey, char *Value, int nMaxLen );

int GetRegistryBinary( char *pProduct, char *pKey, void *Value, int nMaxLen );
int GetLocalRegistryBinary( char *pProduct, char *pKey, void *Value, int nMaxLen );

int SetRegistryInt( char *pProduct, char *pKey, int Value );
int SetLocalRegistryInt( char *pProduct, char *pKey, int Value );

int SetRegistryString( char *pProduct, char *pKey, char *pValue );
int SetLocalRegistryString( char *pProduct, char *pKey, char *pValue );

int SetRegistryBinary( char *pProduct, char *pKey, void *pValue, int nLen );
int SetLocalRegistryBinary( char *pProduct, char *pKey, void *pValue, int nLen );

#ifdef __cplusplus
}
#endif 

// $Log: regaccess.h,v $
// Revision 1.5  2004/03/08 09:25:43  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.4  2004/01/20 08:21:34  d3x0r
// Common updates for merging more commonality
//
// Revision 1.3  2003/03/25 20:41:43  panther
// Fix what CVS logging addition broke
//
// Revision 1.2  2003/03/25 08:59:04  panther
// Added CVS logging
//
