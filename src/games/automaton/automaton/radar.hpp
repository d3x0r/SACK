
#ifndef RADAR_DEFINED
#define RADAR_DEFINED

#include <vectlib.h>
#include "object.hpp"  // lines, facets, objects...
class RADAR {
   TRANSFORM T;
    // scale( vForard, p )...
public: // output only.... 
//   RAY d;
   LINESEG Lines[1];
   LINESEGSET LinePool;
   OBJECT Visual;
   RCOORD speed; // max speed of rotation...
   // rotation accumulator for rotateabs()... rotate rel broken??
   RCOORD p; // direction of radar (relative to body )
public:
   RCOORD power; // max power of radar
   int Right, Left;  // movement left and/or right...
   int neural_p;     
   int distance; // distance of object;  ( 0 -> power )
   
public:
   RADAR( RCOORD distance, PCVECTOR o );
   RADAR( PCVECTOR o );
   bool Hits( RCOORD *distance, PRAY pr );
   void Scan( void );
//   void LineOfSite( P_3POINT pv );
//   void Origin( P_3POINT po );
   void Move( void );
   
   // put this radar object on another object
   inline void PutOn( POBJECT po ) { PutIn( &Visual, po ); }


};


#endif