
#ifndef BECAUSE_EVERYTHING_HAS_CONTEXT
#define BECAUSE_EVERYTHING_HAS_CONTEXT

#include "body.hpp"

class STONE; // forward declarations ARE possible??

class STEP
{
   LINESEG l;
   // include a possible translation matrix to apply....
   void (*BodyCrossedFrom)( class BODY *pb );
   void (*BodyCrossedTo)( class BODY *pb );

   // pPlane and pOtherPlane suffice....
//   class STONE *pFrom, *pTo; //
public:
   inline class STONE *Cross( class STONE *from ) { 
         if( (void*)from==l.pStuff ) 
            return (class STONE *)l.pOtherStuff; 
         return (class STONE *)l.pStuff;  }
   inline class STONE *Cross( class STONE *from,
                                class BODY *pb ) { 
         if( (void*)from==l.pStuff )
         {
            BodyCrossedTo( pb );
            return (class STONE *)l.pOtherStuff;
         }
         BodyCrossedFrom( pb );
         return (class STONE *)l.pStuff;
         }


   STEP( class STONE *ps, PCVECTOR pvo, PCVECTOR pvn, RCOORD from, RCOORD to )
   {
      SetPoint( l.d.n, pvn );
      SetPoint( l.d.o, pvo );
      l.dFrom = from;
      l.dTo = to;
      l.pStuff = (void*)ps; // point to stone this started on....
      l.pOtherStuff = NULL; // later we can mate it to others...
   }

   ~STEP();
};

class STONE { // as in stepping....
   STEP *pSteps[16]; // 16 possible steps.... one more than brainboard...
   int nSteps; // number of possible ways to get out(here)
public:
   inline void AddStep( PCVECTOR pvo, PCVECTOR pvn, RCOORD from, RCOORD to )
         { if( nSteps < 16 ) { pSteps[nSteps] = new STEP( this, pvo, pvn, from, to ); nSteps++; }}
   STONE(){ nSteps = 0; memset( pSteps, 0, sizeof( STEP*) * 16 ); }
   ~STONE(){ nSteps = 0; }
};


#endif
