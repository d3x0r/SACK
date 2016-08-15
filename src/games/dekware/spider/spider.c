#define DEFINES_DEKWARE_INTERFACE

#include "stdhdrs.h"
#include "logging.h"
#include "network.h"
#include "sharemem.h"

#include "resource.h"

#include "spider.h"
#include "page.h"
#include "address.h"
#include "form.h"

#include "plugin.h"


#undef OutputDebugString
#define OutputDebugString(s) (fputs(s,fLog), fflush(fLog))

FILE *fLog;

#define MAX_URL_LEN 1024

HWND ghWndDlg;
BOOL bLimitToAddress; // if address is not the same as source - don't process...
BOOL bFollowLinks;
BOOL bFollowForms;
int  nRequests;
int  nMaxRequests = 8;  
int  nRequestLimit = 0;
int  nPosted;
BOOL bFollowLinkText;
char pFollowText[256];
char pDirectory[MAX_PATH] = "z:\\uswestdex3.com";

#define BUFFER_SIZE 16384

void CPROC ReadComplete( PCLIENT pc, POINTER pBuffer, int nSize )
{
	uint8_t* pBuf = (uint8_t*)pBuffer;
   if( pBuf )
   {
      PBYTE pPage, pAddress, pStart, pFormData;
      int len;
      PADDRESS pa       = (ADDRESS*)GetNetworkLong( pc, 1 );
      PADDRESS paParent = (ADDRESS*)GetNetworkLong( pc, 2 );
      PAGE *page        = (PAGE*)GetNetworkLong( pc, 3 );
      FORM *pForm       = (FORM*)GetNetworkLong( pc, 4 ); // current form finder

      if( nSize < BUFFER_SIZE )
         pBuf[nSize] = 0; // terminate....

      pPage = Begin( page, pBuf, nSize );


//      pPage = FindPage( paParent, pBuf, nSize ); // use parent for this...
      if( pPage )
      {
         nSize -= pPage - pBuf; // remove header amount from leader into buffer...
         // should go through and log META tags also.......
         if( bFollowLinks )
         {
            pStart = pAddress = pPage;
            len = nSize;
            while( pAddress = FindAddress( pa, pAddress, len ) ) // use new tempsapce for this....
            {
               len -= pAddress - pStart;
               pStart = pAddress;

               if( bFollowLinkText )
                  if( stricmp( pa->Text, pFollowText ) )
                  {
                     FreeAddress( pa ); // toss old , and get a new.
                     SetNetworkLong( pc, 1, (DWORD)(pa=CreatePageAddress()) ); // get new address to build into...
                     continue;
                  }

               SetMethod( pa, METH_GET, 0, NULL );
               AddAddress( pa, paParent ); // pa may get freed...
               SetNetworkLong( pc, 1, (DWORD)(pa=CreatePageAddress()) ); // get new address to build into...
            }
         }

         if( bFollowForms )
         {
            pStart = pFormData = pPage;
            len = nSize;
            while( pFormData = FindForm( pForm, pFormData, len ) )
            {
               SetNetworkLong( pc, 4, (DWORD)(pForm=CreateForm(paParent)) ); // get new address to build into...
               len -= pFormData - pStart;
               pStart = pFormData;
            }
         }

         if( Done(page) )
         {
            // FreePage( page ); page is freed by Done()
            FreeForm( pForm );
            FreeAddress( pa );
            SetNetworkLong( pc, 0, 0 );
            SetNetworkLong( pc, 1, 0 );
            SetNetworkLong( pc, 3, 0 );
            SetNetworkLong( pc, 4, 0 );
            RemoveClient( pc );
            return;
         }
      }
   }
   else
   {
      pBuf = (PBYTE)Allocate( BUFFER_SIZE );
      SetNetworkLong( pc, 0, (DWORD)pBuf );
   }
   ReadStream( pc, pBuf, BUFFER_SIZE );
}

void ProcessActive( void );

void CPROC CloseCallback( PCLIENT pc )
{
   void *pbuf;
   PADDRESS pa;
   PPAGE pp;
   PFORM pf;
   {
      char stuff[32];
      sprintf( stuff, "Connections: (%d)", nRequests );
      SetDlgItemText( ghWndDlg, TXT_CONNECTIONS, stuff );
   }
   InterlockedDecrement( (PLONG)&nRequests );
   pbuf = (void*)GetNetworkLong( pc, 0 );
   pa = (PADDRESS)GetNetworkLong( pc, 1 );
   pp = (PPAGE)GetNetworkLong( pc, 3 );
   pf = (PFORM)GetNetworkLong( pc, 4 );
   if( pp )
      FreePage( pp );
   if( pf )
      FreeForm( pf );

   // can no longer free - can move to Done List???
//   FreeAddress( pa );
   // QueueDone( pa ); // apparently this is kept in a list somewhere anyhow.... 
   ProcessActive(); // 
   if( pbuf )
      Release( pbuf );
}

#define RAW         -1  // not browser (ROBOT)
#define PANTHER      0
#define NETSCAPE_45  1
// soon to add NETSCAPE 46
#define EXPLORER_50  2

#define NETSCAPE_3
#define IEXPLORE_3
#define IEXPLORE_4
#define NETSCAPE_5

int Browser = RAW; // selected from radio buttons(?)

#define PANTHER_HDR_S     " HTTP/1.0\r\n"\
                          "Connection: Keep-Alive\r\n"\
                          "User-Agent: Panther/1.0 AlphaBeta\r\n"\
                          "Accept: */*\r\n"\
                          "Accept-Encoding: *\r\n"\
                          "Host: "
#define PANTHER_HDR_E     "\r\n"\
                          "Accept-Language: *\r\n\r\n"

#define NETSCAPE_HDR_45_S " HTTP/1.0\r\n"\
                          "Connection: Keep-Alive\r\n"\
                          "User-Agent: Mozilla/4.5 [en] (Win95; U)\r\n"\
                          "Host: "
#define NETSCAPE_HDR_45_E "\r\n"\
                          "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, image/png, */*\r\n"\
                          "Accept-Language: en\r\n"\
                          "Accept-Charset: iso-8859-1,*,utf-8\r\n"\
                          "Extension: Security/Remote-Passphrase\r\n\r\n"

#define EXPLORER_HDR_50_S " HTTP/1.1\r\n"\
                          "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword,\r\n" \
                          " application/vnd.ms-excel, application/ms-bpc, */*\r\n" \
                          "Accept-Language: en-us\r\n"\
                          "Content-Type: application/x-www-form-urlencoded\r\n" \
                          "Accept-Encoding: gzip, deflate\r\n" \
                          "User-Agent: Mozilla/4.0 (compatible; MSIE 5.0b2; Windows 98)\r\n"\
                          "Host: "
#define EXPLORER_HDR_50_E "\r\n"\
                          "Connection: Keep-Alive\r\n" \
                          "Extension: Security/Remote-Passphrase\r\n\r\n" 

#define HDR_SZ "\r\nContent-Length: "
#define HDR_FORM_CONTENT "\r\nContent-Type: application/x-www-form-urlencoded"


int BeBrowser( PCLIENT pc, int nType, char *byAddr, int nContent, BOOL bWWWEncoded,
                PSTR byBuffer )  
{
//   BYTE byAddr[256];
   BYTE byContent[12];
   int len = 0;

//   if( pa )
//      BuildAddress( pa, (PCHAR)byAddr );

   if( nContent > 0 )
      itoa( nContent, (char*)byContent, 10 );
   else
      byContent[0] = 0;

   switch( nType )
   {
   case RAW:
      if( pc )
         SendTCP( pc, (PBYTE)"\r\n", 2 );
      if( byBuffer )
         len += sprintf( byBuffer+len, "\r\n" );
      break;
   case PANTHER:
      if( pc )
      {
         SendTCP( pc, (PBYTE)PANTHER_HDR_S, sizeof(PANTHER_HDR_S)-1 );
         SendTCP( pc, (PBYTE)byAddr, strlen( (char*)byAddr ) );
         if( byContent[0] ) 
         {
            SendTCP( pc, (PBYTE)HDR_SZ, sizeof( HDR_SZ ) - 1 );
            SendTCP( pc, byContent, strlen( (char*)byContent ) );
         }
         if( bWWWEncoded )
            SendTCP( pc, (PBYTE)HDR_FORM_CONTENT, sizeof(HDR_FORM_CONTENT)-1 );
         SendTCP( pc, (PBYTE)PANTHER_HDR_E, sizeof(PANTHER_HDR_E)-1 );
      }
      if( byBuffer )
      {
         len += sprintf( byBuffer+len, "%s%s", PANTHER_HDR_S, byAddr );
   
         if( byContent[0] )
            len += sprintf( byBuffer+len, "%s%s", HDR_SZ, byContent );
         if( bWWWEncoded )
            len += sprintf( byBuffer+len, "%s", HDR_FORM_CONTENT );
         len += sprintf( byBuffer+len, "%s", PANTHER_HDR_E );
      }
      break;
   case NETSCAPE_45:
      if( pc )
      {
         SendTCP( pc, (PBYTE)NETSCAPE_HDR_45_S, sizeof( NETSCAPE_HDR_45_S )-1 );
         SendTCP( pc, (PBYTE)byAddr, strlen( (char*)byAddr ) );
         if( byContent[0] ) 
         {
            SendTCP( pc, (PBYTE)HDR_SZ, sizeof( HDR_SZ ) - 1 );
            SendTCP( pc, byContent, strlen( (char*)byContent ) );
         }
         if( bWWWEncoded )
            SendTCP( pc, (PBYTE)HDR_FORM_CONTENT, sizeof(HDR_FORM_CONTENT)-1 );
         SendTCP( pc, (PBYTE)NETSCAPE_HDR_45_E, sizeof( NETSCAPE_HDR_45_E )-1 );
      }
      if( byBuffer )
      {
         len += sprintf( byBuffer+len, "%s%s", NETSCAPE_HDR_45_S, byAddr );
         if( byContent[0] )
            len += sprintf( byBuffer+len, "%s%s", HDR_SZ, byContent );
         if( bWWWEncoded )
            len += sprintf( byBuffer+len, "%s", HDR_FORM_CONTENT );
         len += sprintf( byBuffer+len, "%s", NETSCAPE_HDR_45_E );
      }
      break;
   case EXPLORER_50:
      if( pc )
      {
         SendTCP( pc, (PBYTE)EXPLORER_HDR_50_S, sizeof( EXPLORER_HDR_50_S )-1 );
         SendTCP( pc, (PBYTE)byAddr, strlen( (char*)byAddr ) );
         if( byContent[0] ) 
         {
            SendTCP( pc, (PBYTE)HDR_SZ, sizeof( HDR_SZ ) - 1 );
            SendTCP( pc, byContent, strlen( (char*)byContent ) );
         }
         if( bWWWEncoded )
            SendTCP( pc, (PBYTE)HDR_FORM_CONTENT, sizeof(HDR_FORM_CONTENT)-1 );
         SendTCP( pc, (PBYTE)EXPLORER_HDR_50_E, sizeof( EXPLORER_HDR_50_E )-1 );
      }
      if( byBuffer )
      {
         len += sprintf( byBuffer+len, "%s%s", EXPLORER_HDR_50_S, byAddr );
         if( byContent[0] )
            len += sprintf( byBuffer+len, "%s%s", HDR_SZ, byContent );
         if( bWWWEncoded )
            len += sprintf( byBuffer+len, "%s", HDR_FORM_CONTENT );
         len += sprintf( byBuffer+len, "%s", EXPLORER_HDR_50_E );
      }

      break;
   }
   return len;
}

int IssueRequest( PCLIENT pc, int nMethod, char *pAddr, char *pRequest, char *pCGI )
{
   char byBuf[4096];
   int len, l;
   // unless it's raw - then /address:port/page....???
   switch( nMethod )
   {
   case METH_GET:
      len = 0;
      // unless it's raw - then /address:port/page....???
      if( pCGI )
         len += sprintf( byBuf+len, "GET %s?%s", pRequest, pCGI );
      else
         len += sprintf( byBuf+len, "GET %s", pRequest );
      len += BeBrowser( NULL, Browser, pAddr, -1, FALSE, byBuf+len );
      break;
   case METH_POST:
      len = 0; 
      if( !pCGI )
      {
         DebugBreak();
         l = 0;
      }
      else
         l = strlen( pCGI );
      len += sprintf( byBuf+len, "POST %s", pRequest );
      len += BeBrowser( NULL,  Browser, pAddr, l, TRUE, byBuf+len );
      len += sprintf( byBuf+len, "%s", pCGI );
      break;
   case METH_HEAD:
      len = 0; 
      len += sprintf( byBuf+len, "HEAD %s", pRequest );
      len += BeBrowser( NULL,  Browser, pAddr, -1, FALSE, byBuf+len );
      break;
   }
   Log( byBuf );
   Log( "\n------------\n" );
   SendTCP( pc, (PBYTE)byBuf, len );
   return 1;
}

void ProcessActive( void )
{
   // grab next address from active...
   PADDRESS pa;
   char *pHost, *pRequest, *pCGI;
   int nMethod;

   // parse address to see if it contains xxx://
   while( (pa = GetActive( &nMethod, &pHost, &pRequest, &pCGI ) ) )
   {
      if( nRequests >= nMaxRequests ) // too many to do...
         break; // get out now...
      {
         PCLIENT pc;
         if( !nRequestLimit ||
             nPosted++ < nRequestLimit )
         {
            do 
            {
               SetDlgItemInt( ghWndDlg, TXT_CONNECTIONS, nRequests, 0 );
               pc = OpenTCPClientAddrEx( GetNetworkAddress(pa), 
                                         ReadComplete,
                                         CloseCallback, 
                                         NULL );
               if( pc )
               {
                  int r;
                  InterlockedIncrement( (PLONG)&nRequests );
                  // store a NEW address to collect from page...
                  // when done - free this buffer...
                  // when a address is collected, get a new address for this...
                  SetNetworkLong( pc, 1, (DWORD)CreatePageAddress() ); 
                  SetNetworkLong( pc, 2, (DWORD)pa ); // keep parent in this one though...
                  SetNetworkLong( pc, 3, (DWORD)CreatePage( pHost, pRequest, pCGI ) );
                  SetNetworkLong( pc, 4, (DWORD)CreateForm(pa) );      
                  ReadComplete( pc, NULL, 0 ); // hmmm where did this go??

                  r = IssueRequest( pc, nMethod, pHost, pRequest, pCGI );// posts a GET request....
                  if( !r )
                  {
                     InterlockedDecrement( (PLONG)&nRequests );
                     goto DeQueue;
                  }
                  if( r == -1 )
                     goto Repeat;
                  break;
               }
               else
               {
                  OutputDebugString( "TCP Client Creation FAIL\n" );
                  Sleep( 20 ); // wait for a while for sockets to clear....
               }
            }
            while( !pc );
         }
      }
DeQueue:
      DeQueue(pa); // removed from active list.
Repeat:;
//      pa->QueueDone();     
   }

}


DWORD dwStartTime;
DWORD dwStartDone; // count of pages 30 minutes ago...
HINSTANCE hInst;
HWND hWndDialog;

BOOL CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch( uMsg )
   {
   case WM_INITDIALOG:
      ghWndDlg = hWnd;
      {
         FILE *pResume;
         pResume = fopen( "AddressLog.Completed", "rt" );
         if( pResume )
         {
            char byBuffer[4096];
            while( fgets( byBuffer, sizeof( byBuffer ), pResume ) )
            {
               PADDRESS pa;
               char *pEnd;
               pEnd = strchr( byBuffer, '\n' );
               if( pEnd )
                  *pEnd = 0;
               pa = CreateAddressURL( byBuffer + 3 );
               SetMethod( pa, atoi( byBuffer ), 0, NULL );
               ParseAddress( pa, NULL, TRUE );
               {
                  extern int nFinished;
                  nFinished++; 
               }
            }
            fclose( pResume );
         }
         {
            extern int nFinished;
            dwStartDone = nFinished;
         }
      }
      {
         FILE *pResume;
         pResume = fopen( "AddressLog.Resume", "rt" );
         if( pResume )
         {
            char byBuffer[4096];
            while( fgets( byBuffer, sizeof( byBuffer ), pResume ) )
            {
               PADDRESS pa;
               char *pEnd;
               pEnd = strchr( byBuffer, '\n' );
               if( pEnd )
                  *pEnd = 0;
               pa = CreateAddressURL( byBuffer + 3 );
               SetMethod( pa, atoi( byBuffer ), 0, NULL );
               ParseAddress( pa, NULL, TRUE );
               QueueActive(pa);
            }
            fclose( pResume );
            dwStartTime = GetTickCount();
         }
      }
      SetTimer( hWnd, 1000, 1000, 0 );
      SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
      SetDlgItemText( hWnd, EDT_URL, "http://www.greater.net" );
      SetDlgItemText( hWnd, EDT_FOLLOW_TEXT, "Display More Listings" );
      SetDlgItemText( hWnd, EDT_DIR, ".\\" );

      CheckDlgButton( hWnd, CHK_128, (nMaxRequests&0x80)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_64, (nMaxRequests&0x40)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_32, (nMaxRequests&0x20)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_16, (nMaxRequests&0x10)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_8, (nMaxRequests&0x8)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_4, (nMaxRequests&0x4)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_2, (nMaxRequests&0x2)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_1, (nMaxRequests&0x1)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_32kREQ, (nRequestLimit&0x8000)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_16kREQ, (nRequestLimit&0x4000)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_8kREQ, (nRequestLimit&0x2000)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_4kREQ, (nRequestLimit&0x1000)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_2kREQ, (nRequestLimit&0x800)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_1kREQ, (nRequestLimit&0x400)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_512REQ, (nRequestLimit&0x200)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_256REQ, (nRequestLimit&0x100)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_128REQ, (nRequestLimit&0x80)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_64REQ, (nRequestLimit&0x40)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_32REQ, (nRequestLimit&0x20)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_16REQ, (nRequestLimit&0x10)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_8REQ, (nRequestLimit&0x8)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_4REQ, (nRequestLimit&0x4)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_2REQ, (nRequestLimit&0x2)?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_1REQ, (nRequestLimit&0x1)?BST_CHECKED:BST_UNCHECKED );
      Browser = NETSCAPE_45;
      CheckDlgButton( hWnd, RDO_NETSCAPE45, BST_CHECKED );
      CheckDlgButton( hWnd, CHK_FORMS, BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_LINKS, BST_UNCHECKED );
      CheckDlgButton( hWnd, CHK_FOLLOW_TEXT, BST_UNCHECKED );
      bFollowForms = IsDlgButtonChecked( hWnd, CHK_FORMS );
      bFollowLinks = IsDlgButtonChecked( hWnd, CHK_LINKS );
      bFollowLinkText = IsDlgButtonChecked( hWnd, CHK_FOLLOW_TEXT );
      if( bFollowLinkText )
         GetDlgItemText( hWnd, EDT_FOLLOW_TEXT, pFollowText, sizeof( pFollowText ) );

      ProcessActive(); // only works if we had outstanding (resumed) requests...
      break;
   case WM_TIMER:
      {
         extern int nPending, nFinished;
         DWORD dwNow;
         dwNow = GetTickCount();
         if( (dwNow - dwStartTime) > ( 60 * 30 * 1000 ) )
         {
            dwStartTime = dwNow - ( 60 * 30 * 1000 );
            dwStartDone = nFinished - ((( (nFinished - dwStartDone )* 60000 ) / ( GetTickCount() - dwStartTime )) * (30));
         }
         SetDlgItemInt( hWnd, TXT_PENDING, nPending, TRUE );
         SetDlgItemInt( hWnd, TXT_COMPLETED, nFinished, TRUE );
         SetDlgItemInt( hWnd, TXT_RATE, ( (nFinished - dwStartDone )* 60000 ) / ( GetTickCount() - dwStartTime ), TRUE );
         {
            static int nDelay, nRequests, pPending, pFinished;
            if( pPending == nPending &&
                pFinished == nFinished )
            {
               nDelay++;
               if( nDelay > 60 )
               {
                  ProcessActive();  // deadlock!?
               }
            }
            else
               nDelay = 0;
            pPending = nPending;
            pFinished = nFinished;
         }
         
      }
      break;

   case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case RDO_RAW:
         Browser = RAW;
         break;
      case RDO_PANTHER:
         Browser = PANTHER;
         break;
      case RDO_NETSCAPE45:
         Browser = NETSCAPE_45;
         break;
      case RDO_EXPLORER50:
         Browser = EXPLORER_50;
         break;
      case BTN_ALL:
         DumpAddressTree(ANYADDRESS);
         break;
      case BTN_SITE:
         DumpSite( ANYADDRESS, 0 );
         break;
      case IDOK:
         {
            char URL[256];
            PADDRESS pa;
            dwStartTime = GetTickCount();
            GetDlgItemText( hWnd, EDT_URL, URL, sizeof(URL) );
            GetDlgItemText( hWnd, EDT_DIR, pDirectory, sizeof( pDirectory ) );
            /*
            ptemp = pDirectory;
            do {
               ptemp = strchr( ptemp, '\\' ) + 1;
               if( ptemp )
               {
                  if( !isdigit( *ptemp ) && !isalpha( *ptemp ) )
                     *ptemp = 0;
               }
            }while( ptemp && *ptemp );
            */
            Clear(ANYADDRESS); // erase all historical addresses...
            pa = MakeAddress( NULL, URL ); // add used to call processactive...
            SetMethod( pa, METH_GET, 0, NULL );
            QueueActive(pa);
            ProcessActive();
         }
         break;
      case CHK_128:
         IsDlgButtonChecked( hWnd, CHK_128 )? nMaxRequests |= 0x80:(nMaxRequests &= ~0x80);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_64:
         IsDlgButtonChecked( hWnd, CHK_64 )? nMaxRequests |= 0x40:(nMaxRequests &= ~0x40);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_32:
         IsDlgButtonChecked( hWnd, CHK_32 )? nMaxRequests |= 0x20:(nMaxRequests &= ~0x20);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_16:
         IsDlgButtonChecked( hWnd, CHK_16 )? nMaxRequests |= 0x10:(nMaxRequests &= ~0x10);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_8:
         IsDlgButtonChecked( hWnd, CHK_8 )? nMaxRequests |= 0x8:(nMaxRequests &= ~0x8);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_4:
         IsDlgButtonChecked( hWnd, CHK_4 )? nMaxRequests |= 0x4:(nMaxRequests &= ~0x4);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_2:
         IsDlgButtonChecked( hWnd, CHK_2 )? nMaxRequests |= 0x2:(nMaxRequests &= ~0x2);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_1:
         IsDlgButtonChecked( hWnd, CHK_1 )? nMaxRequests |= 0x1:(nMaxRequests &= ~0x1);
         SetDlgItemInt( hWnd, TXT_TOTAL, nMaxRequests, FALSE );
         break;
      case CHK_32kREQ:                              
         IsDlgButtonChecked( hWnd, CHK_32kREQ )? nRequestLimit |= 0x8000:(nRequestLimit &= ~0x8000);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_16kREQ:
         IsDlgButtonChecked( hWnd, CHK_16kREQ )? nRequestLimit |= 0x4000:(nRequestLimit &= ~0x4000);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_8kREQ:
         IsDlgButtonChecked( hWnd, CHK_8kREQ )? nRequestLimit |= 0x2000:(nRequestLimit &= ~0x2000);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_4kREQ:
         IsDlgButtonChecked( hWnd, CHK_4kREQ )? nRequestLimit |= 0x1000:(nRequestLimit &= ~0x1000);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_2kREQ:
         IsDlgButtonChecked( hWnd, CHK_2kREQ )? nRequestLimit |= 0x800:(nRequestLimit &= ~0x800);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_1kREQ:
         IsDlgButtonChecked( hWnd, CHK_1kREQ )? nRequestLimit |= 0x400:(nRequestLimit &= ~0x400);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_512REQ:
         IsDlgButtonChecked( hWnd, CHK_512REQ )? nRequestLimit |= 0x200:(nRequestLimit &= ~0x200);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_256REQ:
         IsDlgButtonChecked( hWnd, CHK_256REQ )? nRequestLimit |= 0x100:(nRequestLimit &= ~0x100);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_128REQ:                              
         IsDlgButtonChecked( hWnd, CHK_128REQ )? nRequestLimit |= 0x80:(nRequestLimit &= ~0x80);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_64REQ:
         IsDlgButtonChecked( hWnd, CHK_64REQ )? nRequestLimit |= 0x40:(nRequestLimit &= ~0x40);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_32REQ:
         IsDlgButtonChecked( hWnd, CHK_32REQ )? nRequestLimit |= 0x20:(nRequestLimit &= ~0x20);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_16REQ:
         IsDlgButtonChecked( hWnd, CHK_16REQ )? nRequestLimit |= 0x10:(nRequestLimit &= ~0x10);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_8REQ:
         IsDlgButtonChecked( hWnd, CHK_8REQ )? nRequestLimit |= 0x8:(nRequestLimit &= ~0x8);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_4REQ:
         IsDlgButtonChecked( hWnd, CHK_4REQ )? nRequestLimit |= 0x4:(nRequestLimit &= ~0x4);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_2REQ:
         IsDlgButtonChecked( hWnd, CHK_2REQ )? nRequestLimit |= 0x2:(nRequestLimit &= ~0x2);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case CHK_1REQ:
         IsDlgButtonChecked( hWnd, CHK_1REQ )? nRequestLimit |= 0x1:(nRequestLimit &= ~0x1);
         SetDlgItemInt( hWnd, TXT_REQUESTS, nRequestLimit, FALSE );
         break;
      case IDCANCEL:
         DestroyWindow( hWnd );
         //EndDialog( hWnd, 0 );
         ghWndDlg = NULL; // end this...
         break;
      case CHK_FORMS:
         bFollowForms = IsDlgButtonChecked( hWnd, CHK_FORMS );
         break;
      case CHK_LINKS:
         bFollowLinks = IsDlgButtonChecked( hWnd, CHK_LINKS );
         break;
      case CHK_FOLLOW_TEXT:
         bFollowLinkText = IsDlgButtonChecked( hWnd, CHK_FOLLOW_TEXT );
         GetDlgItemText( hWnd, EDT_FOLLOW_TEXT, pFollowText, sizeof( pFollowText ) );
         break;
      }
   }
   return FALSE;
}


/*
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow )
{
//   SYSTEMTIME st;
//   GetSystemTime( &st );
   fLog = fopen( "SpiderLog.TXT", "wt+" );
   bLimitToAddress = TRUE;
   NetworkWait( NULL, 256, 20 );
   return DialogBox( hInst, "SpiderStatus", NULL, DialogProc );
}
*/


int CPROC Spider( PSENTIENT ps, PTEXT param )
{
   if( NetworkWait( NULL, 256, 20 ) )
   {
      fLog = fopen( "SpiderLog.TXT", "wt+" );
      bLimitToAddress = TRUE;
      if( !ghWndDlg )
      {
         DialogBox( hInst,  "SpiderStatus", NULL, DialogProc );
   //      CreateDialog( hInst, "SpiderStatus", NULL, DialogProc );
      }
      else
      {
         DestroyWindow( ghWndDlg );
         ghWndDlg = NULL;
      }
   }
   return 0;
}

int APIENTRY DllMain( HINSTANCE hDLL, DWORD dwReason, void *pReserved )
{
   hInst = hDLL;
   return TRUE;
}

PUBLIC( char *, RegisterRoutines )( void )
{
   RegisterRoutine( "spider", "HTML spider dialog...", Spider );
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   UnregisterRoutine( "spider" );
}

// $Log: spider.c,v $
// Revision 1.11  2003/09/28 23:54:19  panther
// Updates to new client only network close.  Fix to portal script
//
// Revision 1.10  2003/07/28 09:07:39  panther
// Fix makefiles, fix cprocs on netlib interfaces... fix a couple badly formed functions
//
// Revision 1.9  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.8  2003/03/25 08:59:03  panther
// Added CVS logging
//
