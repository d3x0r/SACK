
#include <stdhdrs.h>
#include <ctype.h>
#include <logging.h>
#include <network.h>
#include <sharemem.h>


#define PLUGIN_MODULE
#include "plugin.h"

#include "field.h"

#undef OutputDebugString
#define OutputDebugString(s) (fputs(s,fLog), fflush(fLog))
extern FILE *fLog;

#include "address.h"


extern BOOL     bLimitToAddress;
static ADDRESS *pFirstActive, *pLastActive;
static ADDRESS *pFirstDone, *pLastDone;

static ADDRESS *pAddressRoot;  // tree for sorting....
static ADDRESS *pAddressStart; // start from dialog input...

static FILE *pLogActive
          , *pReadActive;  
static char LogText[4096];
static char ReadLog[4096];

int nPending, nFinished; // size of pFirstActive list (in theory)

void Init( PADDRESS pa )
{
   if( !pLogActive )
      pLogActive = fopen( "AddressLog.Q", "wt" );
   if( !pReadActive )
      pReadActive = fopen( "AddressLog.Q", "rt" );

   pa->nUsed   = 0;
   pa->nTextLen = 0;
   pa->Text[0] = 0;
   pa->URL[0]  = 0;
   pa->pNext   = NULL;
   pa->pPrior  = NULL;
   pa->pLess   = NULL;
   pa->pMore   = NULL;
   pa->pChild  = NULL; 
   pa->pParent = NULL;
   pa->pElder  = NULL;
   pa->pRelative = NULL; // #anchor page relative link....
   pa->AdrCount    = 0; // accumulator for address character matches...
   pa->CollectionType = -1;
   pa->pPath   = NULL;
   pa->pPage   = NULL; // this might NOT be set if pPath results to NULL in parse
   pa->pAddr   = NULL;
   pa->pPort   = NULL;
   pa->pExtension = NULL;
   pa->pCGI    = NULL;
   //pa->sa      = NULL;
   pa->nFields = 0;
   pa->FormData = NULL;
   pa->bMustCreate = FALSE;
}

PADDRESS CreatePageAddress(void)
{
   PADDRESS pa;
   pa = (PADDRESS)Allocate( sizeof( ADDRESS ) );
   Init(pa);
   return pa;
}

PADDRESS CreateAddressURL( char *_URL )
{
   PADDRESS pa;
   pa = (PADDRESS)Allocate( sizeof( ADDRESS ) );
   Init(pa);
   if( strlen( _URL ) > MAX_URL_LEN )
      OutputDebugString( "URL BUFFER OVERFLOW!" );
      
   strcpy( pa->URL, _URL );
   return pa;
}

void FreeAddress(PADDRESS pa) 
{
   if( pa->nFields && pa->pCGI )
      Release( pa->pCGI );
   //if( pa->sa )
   //   ReleaseAddress( pa->sa );
   Release( (PBYTE)pa );
}

SOCKADDR *GetNetworkAddress( PADDRESS pa )
{
   return &pa->sa;
}

int CGIcpy( char *pDest, char *pSrc )
{
   char *pd;
   pd = pDest;
   if( pSrc )
   {
      while( *pSrc )
      {
         if( *pSrc == ' ' )
         {
            *(pDest++) = '+';
         }
         else if( *pSrc == '+' )
         {
            *(pDest++) = '%';
            *(pDest++) = '2';
            *(pDest++) = 'B';
         }
         else if( *pSrc == '#' )
         {
            *(pDest++) = '%';
            *(pDest++) = '2';
            *(pDest++) = '3';
         }
         else if( *pSrc == '/' )
         {
            *(pDest++) = '%';
            *(pDest++) = '2';
            *(pDest++) = 'F';
         }
         else if( *pSrc == '%' ) 
         {
            *(pDest++) = '%';
            *(pDest++) = '2';
            *(pDest++) = '5';
         }
         else if( *pSrc == '&' )
         {
            *(pDest++) = '%';
            *(pDest++) = '2';
            *(pDest++) = '6';
         }
         else if( *pSrc == '?' )
         {
            *(pDest++) = '%';
            *(pDest++) = '3';
            *(pDest++) = 'F';
         }
         else if( *pSrc == ':' )
         {
            *(pDest++) = '%';
            *(pDest++) = '3';
            *(pDest++) = 'A';
         }
         else
            *(pDest++) = *(pSrc);
         pSrc++;
      }
   }
   return pDest - pd;
}

// returns length of request string built.
int BuildCGI( PADDRESS pa, char **ppc, int bPost, int nFields, FIELD *FormData )
{
   char *pc;
   int i, iFirstOption;
   BOOL bFindEnd, bUseFirst, bUseNext, bDoneOne, bStepped, bFoundSelect, bImage;

   bPost = FALSE; // post is actually Multi-part-form-encode...

   if( ppc && !*ppc )
   {
      pc = (char*)Allocate( 1024 );
      if( ppc )
         *ppc = pc;
   }
   else
      pc = *ppc;

   bFindEnd     = FALSE;
   bUseFirst    = FALSE;
   bUseNext     = FALSE;
   bDoneOne     = FALSE;
   bStepped     = FALSE;
   bFoundSelect = FALSE;
   bImage       = FALSE;
   iFirstOption = -1;
   for( i = 0; i < nFields; i++ )
      if( FormData[i].used )
         break;

   if( i == nFields )
      bStepped = TRUE; // might be ONLY step but first time form process steps.
   else
      bStepped = FALSE;

   for( i = 0; i < nFields; i++ )
   {
      if( FormData[i].bDisabled )
         continue;
      switch( FormData[i].nType )
      {
      case FT_SELECT:
         if( FormData[i].used )
            bUseNext = TRUE;
         else
         {
            bUseFirst = TRUE;
            bStepped = TRUE;
         }
         FormData[i].used = TRUE;
         if( bDoneOne )
            if( bPost )
               pc += CGIcpy( pc, "\r\n" );
            else
               *(pc++) = '&';
         pc += CGIcpy( pc, FormData[i].pName );
         *(pc++) = '=';
         break;
      case FT_OPTION:
         if( iFirstOption < 0 )
         {
            if( FormData[i].pValue )
            {
               if( strlen( FormData[i].pValue ) )
                  iFirstOption = i;
            }
            else
               DebugBreak();
         }

         if( !bFindEnd )
         {
            if( bUseFirst )
            {
               if( strlen( FormData[i].pValue ) )
                  pc += CGIcpy( pc, FormData[i].pValue );
               FormData[i].used = TRUE;
               bFindEnd = TRUE;
               bUseFirst = FALSE;
            }
            else
               if( bUseNext )
               {
                  if( FormData[i].used )
                  {
                     if( !bStepped )
                     {
                        FormData[i].used = FALSE;
                        bUseFirst = TRUE;
                        bStepped = TRUE;
                        bUseNext = FALSE;
                     }
                     else
                     {
                        pc += CGIcpy( pc, FormData[i].pValue );
                        bFindEnd = TRUE;
                     }
                  }
               }
            bDoneOne = TRUE;
         }
         break;
      case FT_SELECT_END:
         if( !bFindEnd ||  // not ready for end yet
             bUseFirst )  // was looking for an option
         {
            pc += CGIcpy( pc, FormData[iFirstOption].pValue );
            FormData[iFirstOption].used = TRUE;
            bStepped = FALSE; // allow next section to 'step'
         }
         bFindEnd = FALSE;
         bUseFirst = FALSE;
         bUseNext = FALSE;
         iFirstOption = -1;
         break;
      case FT_INPUT:
         FormData[i].used = TRUE; // set to make next pass with no option fields fail bStep

         if( !strnicmp( FormData[i].pType, "reset", 5 ) ) // never include "reset"
            break;
         if( !strnicmp( FormData[i].pType, "submit", 6 ) ) // don't include "submit" without "name"
         {
            break;
         }

         if( !strnicmp( FormData[i].pType, "text", 4 ) )
         {
//            MessageBox( NULL, "May Require user Input", "More?", MB_OK );
         }
         else if( !strnicmp( FormData[i].pType, "checkbox", 8 ) ) // radio buttons name=value (of selected button)
         {
            if( bDoneOne )
               if( bPost )
                  pc += CGIcpy( pc, "\r\n" );
               else
                  *(pc++) = '&';
            if( !strnicmp( FormData[i].pValue, "checked", 7 ) )
            {
               pc += CGIcpy( pc, FormData[i].pName );
               pc += CGIcpy( pc, "=checked" );
               break;
            }
            else
            {
               pc += CGIcpy( pc, FormData[i].pName );
               pc += CGIcpy( pc, "=unchecked" );
               break;
            }
         }else if( !strnicmp( FormData[i].pType, "image", 5 ) )
         {
            if( !bImage )
            {
               if( FormData[i].pName )
               {
                  if( strnicmp( FormData[i].pName, "submit", 6 ) )
                  {
                     // not sure if this should be included or not....
                     if( bDoneOne )
                        if( bPost )
                           pc += CGIcpy( pc, "\r\n" );
                        else
                           *(pc++) = '&';
                     pc += CGIcpy( pc, "x=1" ); // include position of image clicked...
                     if( bDoneOne )
                        if( bPost )
                           pc += CGIcpy( pc, "\r\n" );
                        else
                           *(pc++) = '&';
                     pc += CGIcpy( pc, "y=1" ); // include position of image clicked...
                     bImage = TRUE;
                     break;
                  }
               }
            }
         }else if( !strnicmp( FormData[i].pType, "radio", 5 ) ) 
         {
            Log( "Cannot Format RADIO buttons YET!\n" );
            break; // don't include radio buttons...
         }
         // default - ass this field...
         if( bDoneOne )
            if( bPost )
               pc += CGIcpy( pc, "\r\n" );
            else
               *(pc++) = '&';
         pc += CGIcpy( pc, FormData[i].pName );
         pc += CGIcpy( pc, "=" );
         if( FormData[i].pValue )
            pc += CGIcpy( pc, FormData[i].pValue );
         bDoneOne = TRUE;
         break;
      }
   }
   *pc = 0; // terminate buffer;
   if( !bStepped )
      return 0;
   return pc - *ppc;
}

int BuildRequest( PADDRESS pa, char *buf, BOOL bPost )
{
   int len = 0;
   if( pa->pPath )
      len += sprintf( buf+len, "/%s", pa->pPath );
   if( pa->pPage )
      len += sprintf( buf+len, "/%s", pa->pPage );
   if( pa->pExtension )
      len += sprintf( buf+len, ".%s", pa->pExtension );
   if( pa->pRelative )
      len += sprintf( buf+len, "#%s", pa->pRelative );

   if( pa->nFields ) 
   {
      if( !BuildCGI( pa, &(pa->pCGI), bPost, pa->nFields, pa->FormData ) ) // will not build if already exists..
         return 0;
   }

   if( !bPost )
   {
      if( pa->pCGI )
         len += sprintf( buf+len, "?%s", pa->pCGI );
   }

   if( !len )
   {
      len = 1;
      buf[0] = '/';
      buf[1] = 0;
   }
   return len;
}

int BuildAddress( PADDRESS pa, char *buf )
{
   int len = 0;
   if( pa )
   {
      if( pa->pAddr )
         len += sprintf( buf+len, "%s", pa->pAddr );
      if( pa->pPort )
         len += sprintf( buf+len, ":%s", pa->pPort );
   }
   return len;
}

BOOL SLogAddress( PADDRESS pa, char *buf )
{
   int l;
   if( pa )
   {
      buf += sprintf( buf, "http://" );
      buf += BuildAddress( pa, buf );
      l = BuildRequest( pa, buf, FALSE );
      if( !l )
         return FALSE;
   }
   return TRUE;
}

void LogAddress( PADDRESS pa  )
{
   char URL[MAX_URL_LEN];
   if( pa )
   {
      SLogAddress( pa, URL );
      Log1( "%s", URL );
      Log1( "\nText:%s\n", pa->Text );
   }
}

PADDRESS SortAddress( PADDRESS pa, PADDRESS pRoot )
{
   int dir;

   if( !pRoot )   
   {
      if( !pAddressRoot )
      {
         pAddressRoot = pa;
         return NULL;
      }
      pRoot = pAddressRoot;
   }

   dir = stricmp( pa->pAddr, pRoot->pAddr );
   if( dir == 0 )
   {
      // check server PORT??!?!?!?
      if( pa->pCGI && pRoot->pCGI )
      {
         if( pa->lCGI < pRoot->lCGI )
         {
            dir = -1;
            goto Mismatched;
         }
         if( pa->lCGI > pRoot->lCGI )
         {
            dir = 1;
            goto Mismatched;
         }
         dir = stricmp( pa->pCGI, pRoot->pCGI );
         if( dir != 0 )
            goto Mismatched;
      }
      else if( pa->pCGI && !pRoot->pCGI ) {
         dir = 1;
         goto Mismatched;
      } else if( !pa->pCGI && pRoot->pCGI ) {
         dir = -1;
         goto Mismatched;
      }

      if( pa->pPath && pRoot->pPath )
      {
         if( pa->lPath < pRoot->lPath )
         {
            dir = -1;
            goto Mismatched;
         }
         if( pa->lPath > pRoot->lPath )
         {
            dir = 1;
            goto Mismatched;
         }
         dir = stricmp( pa->pPath, pRoot->pPath );
         if( dir != 0 )
            goto Mismatched;
      } else if( pa->pPath && !pRoot->pPath ) {
         dir = 1;
         goto Mismatched;
      } else if( !pa->pPath && pRoot->pPath ) {
         dir = -1;
         goto Mismatched;
      }

      if( pa->pPage && pRoot->pPage )
      {
         if( pa->lPage < pRoot->lPage )
         {
            dir = -1;
            goto Mismatched;
         }
         if( pa->lPage > pRoot->lPage )
         {
            dir = 1;
            goto Mismatched;
         }
         dir = stricmp( pa->pPage, pRoot->pPage );
         if( dir != 0 )
            goto Mismatched;
      } else if( pa->pPage && !pRoot->pPage ) {
         dir = 1;
         goto Mismatched;
      } else if( !pa->pPage && pRoot->pPage ) {
         dir = -1;
         goto Mismatched;
      }

      if( pa->pExtension && pRoot->pExtension )
      {
         if( pa->lExtension < pRoot->lExtension )
         {
            dir = -1;
            goto Mismatched;
         }
         if( pa->lExtension > pRoot->lExtension )
         {
            dir = 1;
            goto Mismatched;
         }
         dir = stricmp( pa->pExtension, pRoot->pExtension );
         if( dir != 0 )
            goto Mismatched;
      } else if( pa->pExtension && !pRoot->pExtension ) {
         dir = 1;
         goto Mismatched;
      } else if( !pa->pExtension && pRoot->pExtension ) {
         dir = -1;
         goto Mismatched;
      }

      // and what if Page and not root->Page or
      // Path and not root->Path or viceversa?

      return pRoot; // matched???
   }
   else
   {
      if( bLimitToAddress )
      {
         // test IP address - same IP is as good as same address....
         if( memcmp( pa->sa.sa_data+2, 
                     pRoot->sa.sa_data+2, 4 ) )
            // may change IP but remain same name....
            if( stricmp( pa->pAddr, pRoot->pAddr ) )
               return (ADDRESS*)1; // return non-zero causes client to fail...
      }
   }
Mismatched:
   if( dir < 0 ) 
   {
      if( pRoot->pLess )
         return SortAddress( pa, pRoot->pLess );
      else
      {
         pRoot->pLess = pa;
         return NULL;
      }
   }
   if( pRoot->pMore )
      return SortAddress( pa, pRoot->pMore );
   else
   {
      pRoot->pMore = pa;
      return NULL;
   }
}

void QueueActive( PADDRESS pa  )
{
   int addr, cgi;
   if( pa )
   {
      nPending++;
      addr = BuildAddress( pa, LogText );
      addr += 1; // keep nul terminator
      cgi = BuildRequest( pa, LogText + addr, TRUE );

      fprintf( pLogActive, "%d, %s(%u.%u.%u.%u:%d), %s", 
                  GetMethod( pa ), 
                  LogText, 
                  (unsigned char)pa->sa.sa_data[2],
                  (unsigned char)pa->sa.sa_data[3],
                  (unsigned char)pa->sa.sa_data[4],
                  (unsigned char)pa->sa.sa_data[5],
                  ntohs( *(short*)pa->sa.sa_data ),
                  LogText+addr );
      if( pa->pCGI )
         fprintf( pLogActive, ", %s", pa->pCGI );
      fprintf( pLogActive, " : %s", pa->Text );
      fprintf( pLogActive, "\n" );
      fflush( pLogActive );
      
      pa->pPrior = pLastActive;
      if( pLastActive )
      {
         pLastActive->pNext = pa;
      }
      else
         pFirstActive = pa;
      pLastActive = pa;
   }
}

void QueueDone(PADDRESS pa )
{
   if( pa )
   {
      pa->pPrior = pLastDone;
      if( pLastDone )
      {
         pLastDone->pNext = pa;
      }
      else
         pFirstDone = pa;
      pLastDone = pa;
   }
}

PBYTE FindAddress( PADDRESS pa, PBYTE pBuffer, int BufLen )
{
   int c;
   // consider setting AdrCount to zero ........
   // probably just AFTER getting a valid address to return...
   for( c = 0; c < BufLen; c++ )
   {
      if( ( ( pa->CollectionType == 0 && ( pa->AdrCount != 2 && 
                                           pa->AdrCount != 10 )) ||
            ( pa->CollectionType == 1 && pa->AdrCount != 6 ) ||
            ( pa->CollectionType == -1 ) &&   
            ( pa->CollectionType != 2 ) ) &&  
          pBuffer[c] <= 32 )  // any 'whitespace'
         continue;  // next character please...
      switch( pa->CollectionType )
      {
      case -1:
         switch( pa->AdrCount )
         {
         case 0:
            if( pBuffer[c] == '<' )
               pa->AdrCount++;
            break;
         case 1:
            if( ( pBuffer[c] == 'A' || pBuffer[c] == 'a' ) )
            {
               pa->AdrCount++;
               pa->CollectionType = 0;
            }
            else if( ( pBuffer[c] == 'F' || pBuffer[c] == 'f' ) )
            {
               pa->AdrCount++;
               pa->CollectionType = 1;
            }
            else 
               pa->AdrCount = 0; // reset - NO MATCH
            break;

         default:
            OutputDebugString( "Addressing Parsing fell apart!(-1) \n " );
            pa->AdrCount = 0;
            break;
         }
         break;
      case 0:
         switch( pa->AdrCount )
         {
         case 2:
            if( pBuffer[c] <= 32 )  // any control character or space...
               pa->AdrCount++;
            else
               pa->AdrCount = 0;
            break;
         case 3:
            // match to hReF.... ( or spaces )
            if( pBuffer[c] == 'h' || pBuffer[c] == 'H' )
               pa->AdrCount++;
            else if( pBuffer[c] == '>' )
               pa->AdrCount = 0;
            break;
         case 4:
            if( pBuffer[c] == 'r' || pBuffer[c] == 'R' )
               pa->AdrCount++;
            else
               pa->AdrCount = 3;
            break;
         case 5:
            if( pBuffer[c] == 'e' || pBuffer[c] == 'E' )
               pa->AdrCount++;
            else
               pa->AdrCount = 3;
            break;
         case 6:
            if( pBuffer[c] == 'f' || pBuffer[c] == 'F' )
               pa->AdrCount++;
            else
               pa->AdrCount = 3;
            break;
         case 7:
            if( pBuffer[c] == '=' )
               pa->AdrCount++;
            else
               pa->AdrCount = 0;
            break;
         case 8:
            if( pBuffer[c] == '\"' )
            {
               pa->AdrCount++;
            }
            else
            {
               pa->AdrCount = 10;
               pa->URL[pa->nUsed++] = pBuffer[c];
            if( pa->nUsed > MAX_URL_LEN )
               DebugBreak();
            }
            break;
         case 9: // collectin URL now...
            if( pBuffer[c] == '\"' )
            {
               pa->AdrCount++;  // definatly done (start new again)
            }
            else
               pa->URL[pa->nUsed++] = pBuffer[c];
            break;
         case 10:
            if( pBuffer[c] == '>' )
            {
               pa->URL[pa->nUsed] = 0;
               pa->AdrCount = 1;  // collect until </a>
               pa->CollectionType = 2;
               break;
            }
            pa->URL[pa->nUsed++] = pBuffer[c];
            break;
         default:
            OutputDebugString( "Addressing Parsing fell apart!(0) \n " );
            pa->AdrCount = 0;
            break;
         }
         break;
      case 1:
         switch( pa->AdrCount )
         {
         case 2:
            // alternative might have been "form"
            if( pBuffer[c] == 'R' || pBuffer[c] == 'r' )
               pa->AdrCount++;
            else
               pa->AdrCount = 0;
            break;
         case 3:
            // match to hReF.... ( or spaces )
            if( pBuffer[c] == 'A' || pBuffer[c] == 'a' )
               pa->AdrCount++;
            else
               pa->AdrCount = 0;
            break;
         case 4:
            if( pBuffer[c] == 'M' || pBuffer[c] == 'm' )
               pa->AdrCount++;
            else
               pa->AdrCount = 0;
            break;
         case 5:
            if( pBuffer[c] == 'e' || pBuffer[c] == 'E' )
               pa->AdrCount++;
            else
               pa->AdrCount = 0;
            break;
         case 6: 
            if( pBuffer[c] <= 32 )  // any control character or space...
            {
               pa->AdrCount++;
            }
            else
               pa->AdrCount = 0;
            break;

         case 7:
            if( pBuffer[c] == 'S' || pBuffer[c] == 's' )
               pa->AdrCount++;
            else
               pa->AdrCount = 7;
            break;
         case 8:
            if( pBuffer[c] == 'R' || pBuffer[c] == 'r' )
               pa->AdrCount++;
            else
               pa->AdrCount = 7;
            break;
         case 9:
            if( pBuffer[c] == 'C' || pBuffer[c] == 'c' )
               pa->AdrCount++;
            else
               pa->AdrCount = 7;
            break;

         case 10:
            if( pBuffer[c] == '=' || pBuffer[c] == '=' )
               pa->AdrCount++;
            else
               pa->AdrCount = 7;
            break;
         case 11:
            if( pBuffer[c] == '\"' )
            {
               pa->AdrCount++;
            }
            else
               pa->AdrCount = 0;
            break;
         case 12: // collectin URL now...
            if( pBuffer[c] == '\"' )
            {
             //  pBuffer[c] = 0; // terminate the address string...
               pa->AdrCount = 0;  // definatly done (start new again)
//               pa->CollectionType = -1; // collect text until "</a>"
               pa->URL[pa->nUsed] = 0;
               return pBuffer+c; // return end of address to skip in buffer...
            }
            pa->URL[pa->nUsed++] = pBuffer[c];
            if( pa->nUsed > MAX_URL_LEN )
               DebugBreak();

            break;
         default:
            OutputDebugString( "Addressing Parsing fell apart!(1) \n " );
            pa->AdrCount = 0;
            break;
         }
         break;
      case 2:
         switch( pa->AdrCount )
         {
         case 1:
            if( pBuffer[c] == '<' )
               pa->AdrCount++;
            else
               pa->Text[pa->nTextLen++] = pBuffer[c];
            break;
         case 2:
            if( pBuffer[c] == '/' )
               pa->AdrCount++;
            else
               pa->AdrCount = 5;
            break;
         case 3:
            if( pBuffer[c] == 'a' || pBuffer[c] == 'A' )
            {
               pa->Text[pa->nTextLen] = 0; // terminate text string....
               pa->AdrCount++;
            }
            else
               pa->AdrCount = 5;
            break;
         case 4:
            if( pBuffer[c] == '>' || pBuffer[c] <= 32 )
            {
               pa->Text[pa->nTextLen] = 0;
               pa->AdrCount = 0; // resume text collection (maybe?)
               return pBuffer+c;
            }
            break;
         case 5:
            if( pBuffer[c] == '>'  )
            {
               pa->AdrCount = 1; // resume text collection (maybe?)
            }
            break;
         }
         break;
      }
      if( pa->nUsed > MAX_URL_LEN )
         DebugBreak();
      if( pa->nTextLen > 256 )
         DebugBreak();
      if( !pa->AdrCount ) // collection aborted or not... extra work sometimes...
        pa->CollectionType = -1;
   }
   return NULL; // fortunatly we accumulated the partial URL...
}

void ReduceAddress( char *pa ) // a fully qualified address - removed ".."
{
   PSTR ps, ps2;
   if( pa )
   {
      while( ps = strstr( pa, ".." ) )
      {
         if( ps[-1] == '/' )
         {
            ps[-1] = 0; // terminate at slash before perods...
            ps2 = strrchr( pa, '/' );
            strcpy( ps2, ps + 2 );
         }
         else
         {
            OutputDebugString( " ******* Unprefixed .. path... \n " );
         }
      }
   }
}

PADDRESS ParseAddress( PADDRESS pa, PADDRESS paParent, BOOL bSort )
{
   BOOL bFixed = FALSE;
   char *pTemp;
   if( paParent )
   {
      pa->pParent = paParent;
      pa->pElder = paParent->pChild;
      paParent->pChild = pa;
   }
ResolveAgain:
   // mailto:someone@somewhere.com....
   if( pa->URL[0] == '#' )
   {
      OutputDebugString( "Ignoring relative link to same page...\n" );
      FreeAddress( pa );
      return NULL;
   }

   pTemp = strchr( pa->URL, ':' );
   if( pTemp )
      if( !strnicmp( pa->URL, "mailto", 6 ) )
      {
         OutputDebugString( "Ignoring MAILTO : ");
         OutputDebugString( pa->URL );
         OutputDebugString( "\n" );
         FreeAddress( pa );
         return NULL;
      }else if( !strnicmp( pa->URL, "icq", 3 ) )
      {
         OutputDebugString( "Ignoring ICQ : ");
         OutputDebugString( pa->URL );
         OutputDebugString( "\n" );
         FreeAddress( pa );
         return NULL;
      }
   pa->pAddr = strstr( pa->URL, "//" );
   pa->wPort = 80;
   if( pa->pAddr ) 
   {
      // resolve something about what the prefix actually is...
      if( strnicmp( pa->URL, "http:", 5 ) &&
          strnicmp( pa->URL, "https:", 6 ) )
      {
         Log1( "%s Preceedant Address is not a web page address (http)...\n", pa->URL );
         FreeAddress( pa );
         return NULL;
      }
      pa->pAddr += 2; // skip double slash...
   }
   else
   {
      char TempURL[MAX_URL_LEN]; 
      if( bFixed )
      {
         OutputDebugString( "Fixed Address: " );
         OutputDebugString( pa->URL );
         OutputDebugString( "Still FAILED!\n" );
         FreeAddress( pa );
         return NULL;
      }
      if( paParent )
      {
         OutputDebugString( "Parent : " );
         OutputDebugString( paParent->pAddr );
         if( paParent->pPort )
         {
            OutputDebugString("[:]");
            OutputDebugString( paParent->pPort );
         }
         if( paParent->pPath )
         {
            OutputDebugString("[/]");
            OutputDebugString( paParent->pPath );
         }
         if( paParent->pPage )
         {
            OutputDebugString("[/]");
            OutputDebugString( paParent->pPage );
         }
         if( paParent->pExtension )
         {
            OutputDebugString("[.]");
            OutputDebugString( paParent->pExtension );
         }
         OutputDebugString( "\n" );

         if( pa->URL[0] == '/' )
         {
            int len;
            len = sprintf( TempURL, "http://" );
            len += BuildAddress( paParent, TempURL+len );
            len += sprintf( TempURL + len, "%s", pa->URL );
         }
         else
         if( !paParent->pPath )  // path - no Addr...
         {
            int len;
            len = sprintf( TempURL, "http://" );
            len +=  BuildAddress( paParent, TempURL+len );
            len += sprintf( TempURL + len, "/%s", pa->URL );
         }
         else
         {
            int len;
            len = sprintf( TempURL, "http://" );
            len += BuildAddress( paParent, TempURL+len );
            len += sprintf( TempURL + len, "/%s/", paParent->pPath );
            strcpy( TempURL+len, pa->URL );
         }
BeginRetry:
         ReduceAddress( TempURL );
         OutputDebugString( "Built : " );
         OutputDebugString( TempURL );
         OutputDebugString( "\n" );
         strcpy( pa->URL, TempURL );
         bFixed = TRUE;
         goto ResolveAgain;
      }
      else
      {

         sprintf( TempURL, "http://%s", pa->URL );
         goto BeginRetry;
      }
      pa->pAddr = pa->URL;
   }

   {
      pa->pPort = strchr( pa->pAddr, ':' );
      pa->pPath = strchr( pa->pAddr, '/' );
      if( pa->pPath && 
          pa->pPort > pa->pPath )
      {
         OutputDebugString( "Port address resolution will fail for" );
         OutputDebugString( pa->URL );
         OutputDebugString( "\n" );
         pa->pPort = NULL; // no port.
      }
      if( pa->pPort )
      {
         pa->pPort[0] = 0; // terminate address at ':'
         pa->pPort++; // move from : to first char...
         if( isdigit( *pa->pPort ) )
         {
            pa->wPort = (WORD)atoi( pa->pPort );
         }
         else
         {
            SERVENT *se;
            se = getservbyname( pa->pPort ,NULL );
            if( se )
            {
               pa->wPort = se->s_port;
            }
         }
      }
   }
   if( pa->pPath )
   {
      pa->pPath[0] = 0; // remove '/' 
      pa->pPath++; // and go to actual page address...
      pa->pPage = strrchr( pa->pPath, '/' );
      if( !pa->pPage )
      {
         pa->pPage = pa->pPath;
         pa->pPath = NULL;
      }
      else
      {
         pa->pPage[0] = 0;
         pa->pPage++;
      }
   }
   if( pa->pPage )
   {
      pa->pCGI = strchr( pa->pPage, '?' );
      if( pa->pCGI )
      {
         pa->pCGI[0] = 0;
         pa->pCGI++;
      }

      pa->pRelative = strchr( pa->pPage, '#' );
      if( pa->pRelative ) // if we get one of these pretty much gets ignored...
      {
         pa->pRelative[0] = 0;
         pa->pRelative++;
      }

      pa->pExtension = strrchr( pa->pPage, '.' );
      if( pa->pExtension )
      {
         pa->pExtension[0] = 0;
         pa->pExtension++;  
         if( !stricmp( pa->pExtension, "jpg" ) ||
             !stricmp( pa->pExtension, "gif" ) ||
             !stricmp( pa->pExtension, "wav" ) )
         {
            Log( "skipping image/sound link...\n" );
            FreeAddress( pa );
            return NULL;
         }
      }

   }
   
   if( !paParent || strcmp( pa->pAddr, paParent->pAddr ) )
   {
   	SOCKADDR *sa = CreateRemote( pa->pAddr, pa->wPort );;
      if( !sa )
      {
         Log1( "Address : %s", pa->pAddr );
         OutputDebugString( "  was NOT valid\n" );
         OutputDebugString( "Parent of this link: " );
         LogAddress(paParent);
         OutputDebugString( "\n" );
         FreeAddress( pa );
         return NULL;
      }
      pa->sa = *sa;
      ReleaseAddress( sa );
   }
   else
   {
      memcpy( &pa->sa, &paParent->sa, sizeof( SOCKADDR_IN ) );
   }

   if( pa->pAddr )
      pa->lAddr = strlen( pa->pAddr );

   if( pa->pPort )
      pa->lPort = strlen( pa->pPort );

   if( pa->pPath )
      pa->lPath = strlen( pa->pPath );

   if( pa->pPage )
      pa->lPage = strlen( pa->pPage );

   if( pa->pExtension )
      pa->lExtension = strlen( pa->pExtension );

   if( pa->pRelative )
      pa->lRelative = strlen( pa->pRelative );

   if( pa->pCGI )
      pa->lCGI = strlen( pa->pCGI );

   if( bSort )
   {
      if( SortAddress( pa, NULL ) ) // already have this address
      {
         if( !pa->bMustCreate )
         {
            FreeAddress( pa );
            return NULL; 
         }
      }
   }

   return pa;
}

PADDRESS AddAddress( PADDRESS pa, PADDRESS paParent )
{
   PADDRESS _this;
   if( pa )
      _this = ParseAddress( pa, paParent, TRUE);
   else 
      _this = NULL;

   if( _this )
   {
      QueueActive(pa);
   }
   return _this;
}

static SOCKADDR sa;
static ADDRESS CurrentActive;

PADDRESS GetActive( int *nMethod, char **pAddr, char **pRequest, char **pCGI )
{
   char *pComma, *pEnd;
   int l;
   if( pFirstActive )
   {
      l = BuildAddress( pFirstActive, ReadLog );
      *nMethod = GetMethod( pFirstActive );
      BuildRequest( pFirstActive, ReadLog + l + 1, (*nMethod == METH_POST) );
      *pAddr = ReadLog;
      *pRequest = ReadLog + l + 1;
      *pCGI = pFirstActive->pCGI;
   }
   return pFirstActive;

   CurrentActive.sa = sa;
   if( fgets( ReadLog, sizeof(ReadLog), pReadActive ) )
   {
      pComma = strchr( ReadLog, ',' );
      if( !pComma )
         return NULL;
      *pComma = 0;
      *nMethod = atoi( ReadLog );
      pComma += 2;
      *pAddr = pComma;
      pComma = strchr( pComma, '(' );
      *pComma = 0;
      pComma++;
      pEnd = strchr( pComma, ':' );
      *pEnd = 0;
      pEnd++;
      sa.sa_family = 2;
      *(long*)(sa.sa_data + 2) = inet_addr( pComma );
      *(short*)(sa.sa_data ) = htons( (short)atoi( pEnd ) );

      pComma = strchr( pComma, ',' );
      if( !pComma )
         return NULL;
      *pComma = 0;
      pComma += 2;
      *pRequest = pComma;
      
      pComma = strchr( pComma, ',' );
      if( !pComma )
      {
         *pCGI = NULL;
      }
      else
      {
         *pComma = 0;
         pComma += 2;
         *pCGI = pComma;
      }
      return &CurrentActive;
   }
   return NULL;
}

void DeQueue( PADDRESS pa )
{
   nPending--;
   nFinished++; 
   if( pFirstActive )
      pFirstActive = pFirstActive->pNext;
   if( !pFirstActive )
      pLastActive = NULL;
//   pFirstActive = pa->pNext;
}

void DumpAddressTree( PADDRESS pa )
{
   PADDRESS _this;
   if( !pa )
      return;
   if( pa == ANYADDRESS )
      _this = pAddressRoot;
   else 
      _this = pa;
   if( _this )
   {
      if( _this->pLess )
         DumpAddressTree(_this->pLess);
      if( _this->pMore )
         DumpAddressTree(_this->pMore);
      LogAddress(_this);
      OutputDebugString( "\n" );
   }
}

void DumpSite( PADDRESS pa, int level )
{
   int i;
   PADDRESS _this;
   if( pa == ANYADDRESS )
      _this = pAddressStart;
   else
      _this = pa;

   if( !_this )
      return;

   for( i = 0; i < level; i++ )
      OutputDebugString( " | - " );
   LogAddress(_this);
   OutputDebugString( "\n" );
   if( _this->pChild )
      DumpSite( _this->pChild, level+1 );
   if( _this->pElder )
      DumpSite( _this->pElder, level );
}


void Clear( PADDRESS pa  )
{
   if( !pa )
      return;

   if( pa == ANYADDRESS )
   {
      if( pAddressRoot )
      {
         Clear(pAddressRoot->pMore);
         Clear(pAddressRoot->pLess);
         FreeAddress( pAddressRoot );
         pAddressRoot = NULL;
      }
      return;
   }
   FreeAddress( pa );
}

void SetMethod( PADDRESS pa, int nMethod, int nFields, struct field_tag *pFields )
{
   if( pa )
   {
      pa->nMethod = nMethod;
      pa->FormData = pFields;
      pa->nFields = nFields;
   }
}

int GetMethod( PADDRESS pa ) { return pa->nMethod; }

int GetCGI( PADDRESS pa, char **ppCGI )
{
   *ppCGI = pa->pCGI;
   return strlen( pa->pCGI );
}

PADDRESS MakeAddress( PADDRESS paParent, char *_URL )
{
   pAddressStart = CreateAddressURL(_URL);
   //pAddressStart->bMustCreate = TRUE;
   return ParseAddress( pAddressStart, paParent, TRUE );
}


// $Log: address.c,v $
// Revision 1.4  2003/03/25 08:59:03  panther
// Added CVS logging
//
