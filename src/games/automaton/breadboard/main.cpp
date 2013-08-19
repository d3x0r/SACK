#include <controls.h>
#include <logging.h>

#include "../brain/braintypes.h"
#include "../brain/brain.hpp"
#include "../brain/bboard.hpp"

typedef struct {
	BRAIN *Brain;
	PBRAIN_STEM BrainStem;
	BOARD *Board;
} GLOBAL;

GLOBAL g;

ANYVALUE a(VAL_NATIVE,(NATIVE)0)
					  ,b(VAL_NATIVE,(NATIVE)0)
						,c(VAL_NATIVE,(NATIVE)0);

#if 0
void BuildBrain( BRAIN *Brain )
{
   int i, j, max, t, trys, x, y;
   PNEURON pn[MAX_NEURONS];
   PBRAIN_STEM pbs;

   Brain->Lock();
   pbs = Brain->pBrainStem;

   x = 2;
   y = 2;
   do
   {
      max = ( rand() * MAX_NEURONS ) / RAND_MAX;
   } while( max < 25 );
   

   while( pbs )
   {
      j = pbs->nInputs;
      t = 0;
      for( i = 0; i < j; i++ )
      {
         pn[t+i] 
#ifdef VIS_BRAIN
                  = Brain->Board->AddInputToBoard( x, y, pbs, i ); 
#else
                  = Brain->GetNeuron();
#endif
         if( !pn[t+i]) 
            DebugBreak();
//         SetAsInput( Brain, pn[t+i], pbs, i );
         
         x += 4;
         if( x > 120 )
         {
            x = 2;
            x += 4;
         }
      }
      t += i;
      j = pbs->nOutputs;
      for( i = 0; i < j; i++ )
      {
         pn[t+i] 
#ifdef VIS_BRAIN
                  = Brain->Board->AddOutputToBoard( x, y, pbs, i ); 
#else
                  = Brain->GetNeuron();
#endif
         if( !pn[t+i]) 
            DebugBreak();
//         SetAsOutput( Brain, pn[t+i], pbs, i );
         x += 4;
         if( x > 120 )
         {
            x = 2;
            y += 4;
         }
      }
      t += i;
      pbs = pbs->group;
   }
   if( max > t )
      max -= t;
   for( i = 0; i < max; i++ )
   {
      pn[t+i] 
#ifdef VIS_BRAIN
              = Brain->Board->AddNeuron( x, y, NT_ANALOG );
#else
              = Brain->GetNeuron();
#endif
         if( !pn[t+i]) 
            DebugBreak();
      pn[t+i]->nThreshold.n = (( rand() * MAX_THRESHOLD * 2 ) / RAND_MAX) - MAX_THRESHOLD;
      
      x += 4;
      if( x > 120 )
      {
         x = 2;
         y += 4;
      }
   }
   t += i; 
   t--; // don't use VERY LAST
   j = ( rand() * 3 * t ) / RAND_MAX;
   for( i = 0; i < j; i++ )
   {
      PSYNAPSE ps;
      int dofrom = TRUE;
      ps = Brain->GetSynapse();
      ps->nType = ST_NORMAL;
      ps->nGain.n = ((rand() * 200)/RAND_MAX) - 100 ;
      for( trys = 0; trys < 15; trys++ )
      {
         static int ofsx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
         static int ofsy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
         int from, nfrom, to, nto;
         if( dofrom )
         {
            do
            {
               from = ( rand() * t) / RAND_MAX;
               nfrom = (rand() / ( (RAND_MAX+1) / 8 ) );
            }
            while( pn[from]->nType == NT_OUTPUT );
            if( Brain->LinkSynapseFrom( ps, pn[from], nfrom ) )
            {
#ifdef VIS_BRAIN
               Brain->Board->AddGate( 2 + ( from % 30 )*4 + ofsx[nfrom],
                                      2 + ( from / 30 )*4 + ofsy[nfrom] );
#endif
               dofrom = FALSE;
            }
         }
         if( !dofrom )
         {
            do 
            {
               to = ( rand() * t ) / RAND_MAX;
               nto = ( rand() / ( (RAND_MAX+1) / 8 ) );
            }
            while( pn[to]->nType == NT_INPUT );
            if( Brain->LinkSynapseTo( ps, pn[to], nto ) )
               break;
         }
      }
      if( trys == 15 )
      {
//         DebugBreak(); // hmm... strange
         if( !dofrom )
            Brain->UnLinkSynapseFrom( ps );
         Brain->ReleaseSynapse( ps );
      }
   }
   Brain->Unlock();
}
#endif

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCMd, int nCmdShow )
{
   MSG Msg;
  SetSystemLog( SYSLOG_FILE, stdout );
   SetAllocateLogging( TRUE );
   //Enable3DFX(TRUE);

   if( InitializeBoard() )
   {
      g.Brain = new BRAIN;
      // if brain is not mounted, components are not known to board ?
      g.Board = new BOARD;
		g.Board->MountBrain( g.Brain );
      //a.set( VAL_NATIVE, ((NATIVE)0) );
      //b.set( VAL_NATIVE, ((NATIVE)0) );
      //c.set( VAL_NATIVE, ((NATIVE)0) );
      g.Brain->AddComponent( 3, "component A"
                  ,INPUT, &a, "A"
                  ,INPUT, &b, "B"
                  ,OUTPUT, &c, "C" );
      g.Brain->AddComponent( 3, "component B"
                  ,INPUT, &a, "A"
                  ,INPUT, &b, "B"
                  ,OUTPUT, &c, "C" );
      g.Brain->AddComponent( 3, "component C"
                  ,INPUT, &a, "A"
                  ,INPUT, &b, "B"
                  ,OUTPUT, &c, "C" );
      // brains get boards to draw on immediatly...

      while( GetMessage( &Msg, NULL, 0, 0 ) )
      {
         DispatchMessage( &Msg );
      }
   }
   return 0;
}
