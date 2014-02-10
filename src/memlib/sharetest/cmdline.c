
#include <stdhdrs.h>
#include <stdio.h>
#include "sharemem.h"
#include "logging.h"

void ODSEx( TEXTSTR s, TEXTSTR lpFile, int nLine )
{
   TEXTCHAR byBuffer[256];
   sprintf(byBuffer, 
#ifdef _WIN32
   			"[%03X:%03X]"
#endif
   			"%s(%d):%s", 
#ifdef _WIN32
            GetCurrentProcessId(),

            GetCurrentThreadId(),
#endif
            lpFile,nLine, s);
   Log( byBuffer );
}

#define ODS(s) ODSEx(s,__FILE__,__LINE__)

#define MAX_I 128  // must be  apower of 2 and 3 (so far)

//#define MAX_THREADS 50
#define MAX_THREADS 1
typedef POINTER PMEMORY[MAX_I];

PMEMORY pMem[MAX_THREADS];  // 64 threads of information...


void IsDone( POINTER *pMem, char *pFile, int nLine )
#define IsDone() IsDone( pMem, __FILE__, __LINE__ );
{
   int i;
   for( i = 0; i < MAX_I; i++ )
      if( pMem[i] ) 
      {
#ifdef _WIN32
         DebugBreak();
#endif
         ODSEx( WIDE("Memory Holder not Releaseed"), pFile, nLine );
         pMem[i] = 0;
      }

}

POINTER *A1( POINTER *pMem )
{
int i;
   ODS("Allocate 1 TEXTCHAR");
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      if( i < MAX_I-1)
         pMem[i+1] = (POINTER)1;
      pMem[i] = Allocate(1);
   }
   return pMem;
}

POINTER *A2( POINTER *pMem )
{
int i;
   TEXTCHAR byBuffer[256];
   sprintf( byBuffer, WIDE("allocate (n*32) 0..%d"), MAX_I);
   ODS( byBuffer );
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      pMem[i] = Allocate( i * 32 );
   }
   return pMem;
}

POINTER *A2i( POINTER *pMem )
{
int i;
   TEXTCHAR byBuffer[256];
   int nSize;
   BOOL bFail = FALSE;
   sprintf( byBuffer, WIDE("allocate ((%d-n)*32) 0..%d"), MAX_I,MAX_I);
   ODS( byBuffer );
   IsDone();
   for( i = 0; i < MAX_I; i++ )
   {
      nSize = (MAX_I-i) *32;
      pMem[i] = Allocate( nSize );
   }
   return pMem;
}

POINTER *A3( POINTER *pMem )
{
int i;
   int s = 8137;
   TEXTCHAR byBuffer[256];
   sprintf( byBuffer, WIDE("allocate to fill memory"), MAX_I,MAX_I);
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
         TEXTCHAR byDebug[256];
         sprintf( byDebug, WIDE("Failed at %d..."), i );
         break;
      }
   }
   return pMem;
}

POINTER *T1( POINTER *pMem )
{
      ODS( WIDE("Starting FALR"));
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

POINTER *T2( POINTER *pMem )
{
      ODS( WIDE("Starting FirstAllocFirstRelease"));
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

POINTER *T3( POINTER *pMem )
{
      ODS( WIDE("Starting Stagger Deallocate"));
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

POINTER *T3i( POINTER *pMem )
{
      ODS( WIDE("Starting Inverted Stagger Deallocate"));
      {
      
         int i, r;
         for( i = 0; i < MAX_I; i++ )
         {
            r = i*2;
            if( r >= MAX_I ) 
               r = (MAX_I*2 - 1 ) - r ;
            if( 0 )
            {
               TEXTCHAR byDebug[256];
               sprintf( byDebug, WIDE("Deallocate : %4d"), r );
               ODS( byDebug );
            }
            pMem[r] = Release( pMem[r] );
         }
      }
   IsDone();
   return pMem;
}

POINTER *T3is( POINTER *pMem )
{
      ODS( WIDE("Starting Inverted Sequential Stagger Deallocate"));
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

POINTER *T4( POINTER *pMem )
{
      ODS( WIDE("Starting 3-Stagger Deallocate"));
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
            _32 dwFree, dwUsed, dwChunks, dwFreeChunks;
            char msg[256];
            GetMemStats(&dwFree, &dwUsed, &dwChunks, &dwFreeChunks);
            sprintf( msg, WIDE("MemStat: Used: %d(%d) Free: %d(%d)")
                        , dwUsed, dwChunks - dwFreeChunks
                        , dwFree, dwFreeChunks );
            ODS( msg );
         }


POINTER *T5( POINTER *pMem )
{
      ODS( WIDE("Starting Defrag test"));
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

void ThreadProc( POINTER *pMem )
{
  int i;
//   InterlockedIncrement( (PLONG)&nThreads );
//   Sleep(500);  // wait for more threads to start up too..
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
//   InterlockedDecrement( (PLONG)&nThreads );
//   ExitThread(0);
}




int main( void )
{
	SetSystemLog( SYSLOG_FILE, fopen( WIDE("CmdLine.log"), WIDE("wt") ) );
	printf( WIDE("Doing Test...") );
	ThreadProc( pMem[0] );
}
// $Log: $
