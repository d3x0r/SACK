

typedef struct page_tag {
   int HdrSepCount; // count of \r\n\r\n for header-page seperator...
   int PageLength;  // updated if the server returns a 'Content-Length: ' specification
   int PageRead;
   HANDLE hSave;
} PAGE, *PPAGE;


PPAGE CreatePage( char *pAddr, char *pRequest, char *pCGI );
void FreePage(PPAGE pp);
PBYTE Begin( PPAGE pp, PBYTE pBuffer, int BufLen );
BOOL Done( PPAGE pp );

// $Log: page.h,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
