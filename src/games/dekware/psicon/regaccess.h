
#ifdef __cplusplus
//extern "C" {
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

int GetRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pKey, void *Value, int nMaxLen );
int GetLocalRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pKey, void *Value, int nMaxLen );

int SetRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pKey, int Value );
int SetLocalRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pKey, int Value );

int SetRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, TEXTCHAR *pValue );
int SetLocalRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, TEXTCHAR *pValue );

int SetRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pKey, void *pValue, int nLen );
int SetLocalRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pKey, void *pValue, int nLen );

#ifdef __cplusplus
//}
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
