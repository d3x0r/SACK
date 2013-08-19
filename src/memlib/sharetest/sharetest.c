
#include <windows.h>
#include <stdio.h>
//#include <commctrl.h>
#include "resource.h"
#include "sharemem.h"


void ODSEx( PBYTE s, LPSTR lpFile, int nLine )
{
   BYTE byBuffer[256];
   sprintf(byBuffer, WIDE("[%03X:%03X]%s(%d):%s"), 
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            lpFile,nLine, s);
   OutputDebugString ( byBuffer );
}

#define ODS(s) ODSEx(s,__FILE__,__LINE__)

#define MAX_I 128  // must be  apower of 2 and 3 (so far)

//#define MAX_THREADS 50
#define MAX_THREADS 1
typedef PBYTE PMEMORY[MAX_I];

PMEMORY pMem[MAX_THREADS];  // 64 threads of information...


void IsDone( PBYTE *pMem, char *pFile, int nLine )
#define IsDone() IsDone( pMem, __FILE__, __LINE__ );
{
   int i;
   for( i = 0; i < MAX_I; i++ )
      if( pMem[i] ) 
      {
         DebugBreak();
         ODSEx( WIDE("Memory Holder not Releaseed\n"), pFile, nLine );
         pMem[i] = 0;
      }

}

PBYTE *A1( PBYTE *pMem )
{
int i;
   ODS("Allocate 1 byte\n");
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      if( i < MAX_I-1)
         pMem[i+1] = (PBYTE)1;
      pMem[i] = Allocate(1);
   }
   return pMem;
}

PBYTE *A2( PBYTE *pMem )
{
int i;
   BYTE byBuffer[256];
   sprintf( byBuffer, WIDE("allocate (n*32) 0..%d\n"), MAX_I);
   ODS( byBuffer );
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      pMem[i] = Allocate( i * 32 );
   }
   return pMem;
}

PBYTE *A2i( PBYTE *pMem )
{
int i;
   BYTE byBuffer[256];
   int nSize;
   BOOL bFail = FALSE;
   sprintf( byBuffer, WIDE("allocate ((%d-n)*32) 0..%d\n"), MAX_I,MAX_I);
   ODS( byBuffer );
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      nSize = (MAX_I-i) *32;
      pMem[i] = Allocate( nSize );
   }
   return pMem;
}

PBYTE *A3( PBYTE *pMem )
{
int i;
   int s = 8137;
   BYTE byBuffer[256];
   sprintf( byBuffer, WIDE("allocate to fill memory\n"), MAX_I,MAX_I);
   ODS( byBuffer );
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      while( !( pMem[i] = Allocate( s )) )
      {
         s /= 2;
         if( !s ) 
            break;
      }
      if( !s )
      {
         BYTE byDebug[256];
         sprintf( byDebug, WIDE("Failed at %d...\n"), i );
         break;
      }
   }
   return pMem;
}

PBYTE *T1( PBYTE *pMem )
{
      ODS( WIDE("Starting FALR\n"));
      {
         int i;
         for( i = MAX_I-1; i >=0; i-- )
         {
            pMem[i] = Release( pMem[i] );
         }
      }
   IsDone();
   return pMem;
}

PBYTE *T2( PBYTE *pMem )
{
      ODS( WIDE("Starting FirstAllocFirstRelease\n"));
      {
         int i;
           for( i = 0; i < MAX_I; i++ )
         {
            pMem[i] = Release( pMem[i] );
         }
      }
   IsDone();
   return pMem;
}

PBYTE *T3( PBYTE *pMem )
{
      ODS( WIDE("Starting Stagger Deallocate\n"));
      {
      
         int i, r;
         for( i = 0; i < MAX_I; i++ )
         {
            r = i*2;
            if( r >= MAX_I ) 
               r -= (MAX_I - 1);
            pMem[r] = Release( pMem[r] );
         }
      }

   IsDone();
   return pMem;
}

PBYTE *T3i( PBYTE *pMem )
{
      ODS( WIDE("Starting Inverted Stagger Deallocate\n"));
      {
      
         int i, r;
         for( i = 0; i < MAX_I; i++ )
         {
            r = i*2;
            if( r >= MAX_I ) 
               r = (MAX_I*2 - 1 ) - r ;
            if( 0 )
            {
               BYTE byDebug[256];
               sprintf( byDebug, WIDE("Deallocate : %4d\n"), r );
               ODS( byDebug );
            }
            pMem[r] = Release( pMem[r] );
         }
      }
   IsDone();
   return pMem;
}

PBYTE *T3is( PBYTE *pMem )
{
      ODS( WIDE("Starting Inverted Sequential Stagger Deallocate\n"));
      {
         int i, r;
         for( i = 0; i < MAX_I; i++ )
         {
            if( i & 1 )
               r = MAX_I - i;
            else
              r = i;
            pMem[r] = Release( pMem[r] );
         }
      }
   IsDone();
   return pMem;
}

PBYTE *T4( PBYTE *pMem )
{
      ODS( WIDE("Starting 3-Stagger Deallocate\n"));
      {
         int i, r;
         for( i = 0; i < MAX_I; i++ )
         {
            r = i*3;
            while( r >= MAX_I )
            {
               r -= (MAX_I-1);
            }
            pMem[r] = Release( pMem[r] );
         }
      }
   IsDone();
   return pMem;
}

void DumpStats( void )
         {
            DWORD dwFree, dwUsed, dwChunks, dwFreeChunks;
            char msg[256];
            GetMemStats(&dwFree, &dwUsed, &dwChunks, &dwFreeChunks);
            sprintf( msg, WIDE("MemStat: Used: %d(%d) Free: %d(%d)")
                        , dwUsed, dwChunks - dwFreeChunks
                        , dwFree, dwFreeChunks );
            ODS( msg );
         }


PBYTE *T5( PBYTE *pMem )
{
      ODS( WIDE("Starting Defrag test\n"));
      DumpStats();
      {

         int i, r;
         for( i = 0; i < MAX_I/2; i++ )
         {
            r = i*2;
            if( r >= MAX_I )
               r -= (MAX_I - 1);
            pMem[r] = Release( pMem[r] );
         }
      DumpStats();
         for( i = 0; i < (MAX_I)/2; i++ )
         {
            r = i*2;
            Defragment( &pMem[r+1] );
         }
      DumpStats();
         for( ; i < MAX_I; i++ )
         {
            r = i*2;
            if( r >= MAX_I )
               r -= (MAX_I - 1);
            pMem[r] = Release( pMem[r] );
         }
      DumpStats();
      }
   IsDone();
   return pMem;
}


int nThreads;

void ThreadProc( PBYTE *pMem )
{
  int i;
   InterlockedIncrement( (PLONG)&nThreads );
   Sleep(500);  // wait for more threads to start up too..
   // initial state - have to do an allocate....
// Release( Allocate( 1 ) );
// DebugDumpMem();

   for( i = 0; i < 10; i++ )
   {
      T1  (A1(pMem));
//    Sleep(0);
      T2  (A1(pMem));
//    Sleep(0);
      T3  (A1(pMem));
//    Sleep(0);
      T3i (A1(pMem));
//    Sleep(0);
      T3is(A1(pMem));
//    Sleep(0);
      T4  (A1(pMem));
//    Sleep(0);
      T5  (A1(pMem));
//    Sleep(0);

      T1  (A2(pMem));
//    Sleep(0);
      T2  (A2(pMem));
//    Sleep(0);
      T3  (A2(pMem));
//    Sleep(0);
      T3i (A2(pMem));
//    Sleep(0);
      T3is(A2(pMem));
//    Sleep(0);
      T4  (A2(pMem));
//    Sleep(0);
      T5  (A1(pMem));
//    Sleep(0);

      T1  (A2i(pMem));
//    Sleep(0);
      T2  (A2i(pMem));
//    Sleep(0);
      T3  (A2i(pMem));
//    Sleep(0);
      T3i (A2i(pMem));
//    Sleep(0);
      T3is(A2i(pMem));
//    Sleep(0);
      T4  (A2i(pMem));
//    Sleep(0);
      T5  (A1(pMem));
//    Sleep(0);
   }
   InterlockedDecrement( (PLONG)&nThreads );
   ExitThread(0);
}

BOOL CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch( uMsg )
   {
   case WM_INITDIALOG:
      {
         DWORD dwThreadId;
         int i;

         i = 0;
         for( i = 0; i < MAX_THREADS; i++ )
         {
            CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, pMem[i], 0, &dwThreadId );
         }

      }
      SetTimer( hWnd, 100, 100, NULL );
      return TRUE;
   case WM_TIMER:
      {
         BYTE byMsg[256];
         sprintf( byMsg, WIDE("Thread: %d"), nThreads );
         SetDlgItemText( hWnd, TXT_MESSAGE, byMsg );
         if( !nThreads )
         {
            DWORD dwThreadId;
            int i;
            DebugDumpMem();
            nThreads = -1;
            /*
            i = 0;
            for( i = 0; i < MAX_THREADS; i++ )
            {
               CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, pMem[i], 0, &dwThreadId );
            }
            Sleep(1);
            */
         }
//       if( !nThreads )
//          DebugDumpMemEx( TRUE );
      }

      return TRUE;

   case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case IDOK:
      case IDCANCEL:
         EndDialog( hWnd, 0 );
      }
      break;

   case WM_DESTROY:
      return TRUE;
   }
   return FALSE;
}

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR pCmdLine, int nCmdShow )
{
// InitCommonControls();
   return DialogBox( hInst, WIDE("Test"), NULL, DialogProc );
}
// $Log: $
