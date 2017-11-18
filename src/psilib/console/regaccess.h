#ifdef __LINUX__
typedef int HKEY;
typedef int DWORD;
#define HKEY_CURRENT_USER 0 
#define HKEY_LOCAL_MACHINE 1 
enum enum_type { REG_SZ,REG_DWORD,REG_BINARY };
#endif


int GetRegistryItem( HKEY hRoot, CTEXTSTR pPrefix,
                     CTEXTSTR pProduct, CTEXTSTR pKey,
                     DWORD dwType,
                     TEXTSTR nResult, int nSize );

int SetRegistryItem( HKEY hRoot, CTEXTSTR pPrefix,
                     CTEXTSTR pProduct, CTEXTSTR pKey,
                     DWORD dwType,
                     CTEXTSTR pValue, int nSize );


int GetRegistryInt( CTEXTSTR pProduct, CTEXTSTR pKey, int *Value );
int GetLocalRegistryInt( CTEXTSTR pProduct, CTEXTSTR pKey, int *Value );

int GetRegistryString( CTEXTSTR pProduct, CTEXTSTR pKey, TEXTSTR Value, int nMaxLen );
int GetLocalRegistryString( CTEXTSTR pProduct, CTEXTSTR pKey, TEXTSTR Value, int nMaxLen );

int GetRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pKey, TEXTSTR Value, int nMaxLen );
int GetLocalRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pKey, TEXTSTR Value, int nMaxLen );

int SetRegistryInt( CTEXTSTR pProduct, CTEXTSTR pKey, int Value );
int SetLocalRegistryInt( CTEXTSTR pProduct, CTEXTSTR pKey, int Value );

int SetRegistryString( CTEXTSTR pProduct, CTEXTSTR pKey, CTEXTSTR pValue );
int SetLocalRegistryString( CTEXTSTR pProduct, CTEXTSTR pKey, CTEXTSTR pValue );

int SetRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pKey, CTEXTSTR pValue, int nLen );
int SetLocalRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pKey, CTEXTSTR pValue, int nLen );


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
