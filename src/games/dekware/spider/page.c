#include <stdhdrs.h>
#include <network.h>
#include <logging.h>

#include <sharemem.h>
#define PLUGIN_MODULE
#include "plugin.h"

#undef OutputDebugString
#define OutputDebugString(s) (fputs(s,fLog), fflush(fLog))
extern FILE *fLog;

#include "address.h"
#include "page.h"
extern char pDirectory[];
static char pAddress[4096];
PPAGE CreatePage( char *pAddr, char *pRequest, char *pCGI ) 
{
   char pFilename[256];
   SYSTEMTIME st;
   PPAGE pp;
   DWORD dwWritten;
   pp = (PPAGE)Allocate( sizeof(PAGE) );
   GetLocalTime( &st );
   sprintf( pFilename, "%s\\PageData-%04d%02d%02d-%02d%02d%02d%03d.html",
                        pDirectory,
                        st.wYear, st.wMonth, st.wDay, st.wHour
                        , st.wMinute, st.wSecond, st.wMilliseconds );
   pp->hSave = CreateFile( pFilename, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL );
   WriteFile( pp->hSave, "<!-- ", 5, &dwWritten, NULL );
   WriteFile( pp->hSave, pAddr, strlen( pAddr ), &dwWritten, NULL );
   WriteFile( pp->hSave, pRequest, strlen( pRequest ), &dwWritten, NULL );
   if( pCGI )
   {
      WriteFile( pp->hSave, "?", 1, &dwWritten, NULL );
      WriteFile( pp->hSave, pCGI, strlen( pCGI ), &dwWritten, NULL );
   }
   WriteFile( pp->hSave, " -->\n", 5, &dwWritten, NULL );
   pp->HdrSepCount = 0;
   pp->PageLength  = 0;
   pp->PageRead    = 0;
   return pp;
}

void FreePage(PPAGE pp) 
{
   if( pp->hSave != INVALID_HANDLE_VALUE )
      if( pp->hSave != (void*)0xDDDDDDDD ) // help?!
         CloseHandle( pp->hSave );
   Release( (PBYTE)pp );
};


PBYTE Begin( PPAGE pp, PBYTE pBuffer, int BufLen )
{
// if this sequence spans buffers, we will not work....
   int c;
   c = 0;
   if( pp->HdrSepCount < 4 )
   {
      for( ; c < BufLen && pp->HdrSepCount < 4; c++  )
      {
         if( !( pp->HdrSepCount & 1 ) && pBuffer[c] == '\r' )
         {
            pp->HdrSepCount++;
            continue;
         }
         if( ( pp->HdrSepCount & 1 ) && pBuffer[c] == '\n' )
         {
            pp->HdrSepCount++;
            if( !StrCaseCmpEx( "Content-Length", (char*)(pBuffer + c + 1), 14 ) )
            {
               pp->PageLength = atoi( (char*)(pBuffer + c + 16) );
            }
            continue;
         }
         pp->HdrSepCount = 0; // reset now...
      }
   }
   BufLen -= c;
   pp->PageRead += BufLen;
   if( pp->HdrSepCount == 4 )
   {
      DWORD dwWritten;
      if( pp->hSave != INVALID_HANDLE_VALUE )
      {
         WriteFile( pp->hSave, pBuffer+c, BufLen, &dwWritten, NULL );
         if( dwWritten != BufLen )
         {
            Log2( "Failed to write information to file! %s(%d)", __FILE__, __LINE__ );
         }
      }
      return pBuffer + c;
   }
   return NULL;
}

BOOL Done( PPAGE pp )
{
   if( pp->PageLength )
   {
      if( pp->PageRead == pp->PageLength )
      {
         FreePage( pp );
         return TRUE;
      }
      if( pp->PageRead > pp->PageLength )
         OutputDebugString( "Page Read OVERFLOW!\n" );
   }
   return FALSE;
}

// $Log: page.c,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
