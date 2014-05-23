
#include "radar.hpp"
#include "neuron.h" // MAX_INPUT...

#include "plane.hpp" // intersection time should be mathlib?

RADAR::RADAR( RCOORD distance, PCVECTOR o )
{
   T.clear();


   power = distance;
   speed = (RCOORD)_5;  // used to be 0.10 and was close to this....
   p = (RCOORD)0.0;
   distance = 0;
   neural_p = 0;
   Right    = 0;
   Left     = 0;

   SetPoint( Lines[0].d.o, o ); // set origin on body...
   Lines[0].dFrom = 0;
   Lines[0].dTo = 1.0;  // power failure.... 100 % power...
   Lines[0].pStuff = NULL;
   Lines[0].pOtherStuff = NULL;
   Lines[0].bUsed = TRUE;
   Lines[0].bDraw = TRUE;  

   LinePool.pLines = Lines;
   LinePool.nLines = 1;
   LinePool.nUsedLines = 1;

   Visual.pPlaneSet = NULL;
   Visual.pLinePool = &LinePool;
   Visual.pNext = &Visual;
   Visual.pPrior = &Visual;
   Visual.pIn = NULL;
   Visual.pHolds = NULL;

   Move();  // finish update of compational variables...
}

RADAR::RADAR( PCVECTOR o ) {
   RADAR( (RCOORD)100, o );
}

bool RADAR::Hits( RCOORD *distance, PRAY pr ) // orogin, normal length of line.
{ 
   // return time which line hits wall...
   // time time on wall // if time is withing
   // timeof( pr.o - pr.n )... 0 to what??
//   RCOORD t1; // = distance...
   RCOORD t1, t2;
//   RAY dir;
   t1 = *distance;

   if( FindIntersectionTime( distance, Lines[0].d.n, Lines[0].d.o
                           , &t2, pr->n, pr->o ) )
   {
      if( t2 >= 1.0 || // don't collide ON bounds...
          t2 <= 0.0 ||
          *distance <= 0.0 ||
          *distance > t1 )
      {
         *distance=t1;
         return false;
      }
      // shorter distance.... returned IN distance...
      return true;
   }
   return false;

}


void RADAR::Scan( void )
{

  // for ( bodies in environemnt )
  // {
   //   if 
   // this should look at all other bodies
   // which exist within this environment
   // this means that the enviroment must
   // therefore have methods for accessing
   // bodys - and therefore their positions...
   // current origin/line of site is opon the body
   // we have to have the bodies position in order
   // that we may actually scan...
   // the other body must of course be translated....
   //}
}


void RADAR::Move( void ) // also Update...
{
   VECTOR vn;

   p += speed * ((RCOORD)(Right - Left) / (RCOORD)MAX_OUTPUT_VALUE);
   
   while( p < -M_PI )
      p += (RCOORD)(2 * M_PI);

   while( p > (RCOORD)(M_PI) )
      p -= (RCOORD)(2 * M_PI);

   T.RotateAbs( 0, 0, p );  // p untouched
   // RotateRoll( p ); // subset equation...(??)

   // this result is +/- 100 
   neural_p = (int)( (p * (RCOORD)MAX_INPUT_VALUE) / (RCOORD)(M_PI));

   scale( vn, VectorConst_Y, power );
   T.ApplyRotation( Lines[0].d.n, vn );
}
