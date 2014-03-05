
#ifndef ADDRESS_INCLUDED
#define ADDRESS_INCLUDED

#define METH_NONE -1
#define METH_GET   0
#define METH_POST  1
#define METH_HEAD  2
#include "field.h"

#define MAX_URL_LEN 1024 // long cgi parameters.... 

typedef struct address_tag{
   char URL[MAX_URL_LEN];
   char Text[256];
   int  nTextLen;
   BOOL bMustCreate;

   int nUsed; // already stored some of the URL here...
   int lAddr;
   char *pAddr;
   int lPort;
   char *pPort;
   int lPath;
   char *pPath;  // path + '/' + Page is actual path...
   int lPage;
   char *pPage;  // pointers into URL 
   int lExtension;
   char *pExtension; // start of page extension... ('.')
   int lRelative;
   char *pRelative;
   int lCGI;
   char *pCGI;      // start of CGI string ( '?' )
   SOCKADDR sa; // created this too as we parsed this URL...
   WORD wPort;   // numeric port representation of pPort...
   struct {
      WORD bSecure:1; // build https instead of http
      WORD wFiller:15;  // DWORD align this thing.... shrug
   };

   int nMethod;
   int CollectionType; // 0 = <A HREF= ; 1 = <FRAME Src=
   int AdrCount;    // count of address peices... 
   struct field_tag *FormData;
   int   nFields;
   struct address_tag *pNext, *pPrior,   // active/inactive lists
                 *pParent, *pChild, // keep web relationship...
                 *pElder, // since parent can only point at one child...
                 *pLess, *pMore;    // alphabetic sort for duplicate reduction
} ADDRESS, *PADDRESS;


PADDRESS CreatePageAddress(void);
PADDRESS CreateAddress_URL(char *_URL);
PADDRESS CreateAddressURL( char *_URL );
void     FreeAddress(PADDRESS pa);

int BuildRequest( PADDRESS pa, char *buf, BOOL bPost );
int BuildAddress( PADDRESS pa, char *buf );
BOOL SLogAddress( PADDRESS pa, char *buf );
void LogAddress(PADDRESS pa);
PBYTE FindAddress( PADDRESS pa, PBYTE pBuffer, int BufLen );
void QueueActive( PADDRESS pa );
void QueueDone(PADDRESS pa  );
PADDRESS SortAddress( PADDRESS pa, PADDRESS pRoot );
PADDRESS AddAddress( PADDRESS pa, PADDRESS paParent );
//PADDRESS MakeAddress( PADDRESS pa, PADDRESS paParent, char *URL );  // forward declaraion...
PADDRESS ParseAddress( PADDRESS pa, PADDRESS paParent, BOOL bSort );
void DeQueue( PADDRESS pa );
//PADDRESS GetActive( void );
PADDRESS GetActive( int *nMethod, char **pAddr, char **pRequest, char **pCGI );
void DumpAddressTree( PADDRESS pa );
void DumpSite( PADDRESS pa, int level );
SOCKADDR *GetNetworkAddress( PADDRESS pa );
//void SetMethod( PADDRESS pa, int nMethod );
void SetMethod( PADDRESS pa, int nMethod, int nFields, struct field_tag *pFields );
int GetMethod( PADDRESS pa );
int GetCGI( PADDRESS pa, char **ppCGI );
void Clear( PADDRESS pa );


//typedef ADDRESS *PADDRESS;

#define ANYADDRESS ((PADDRESS)(1))


//ADDRESS *CreatePageAddress( void );
PADDRESS MakeAddress( PADDRESS paParent, char *_URL );

#endif

// $Log: address.h,v $
// Revision 1.5  2003/03/25 20:43:27  panther
// Fix what CVS logging addition broke
//
// Revision 1.4  2003/03/25 08:59:03  panther
// Added CVS logging
//
