#ifndef WEAPONS_DEFINED
#define WEAPONS_DEFINED

#include <vectlib.h>

class WEAPON {
   TRANSFORM T;   
   int p; // power(range) of shot
   RCOORD speed; // max speed of rotation...

public:
   int Right, Left; // movment of gun Turret...
   int Fire;  // set to fire this weapon...

public:
   WEAPON() {
   }
   ~WEAPON() {
   }
};


#endif