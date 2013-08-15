#ifndef BODY_CLASS_DEFINED
#define BODY_CLASS_DEFINED

#include "vectlib.h"

#include "weapons.hpp"
#include "radar.hpp"

#include "object.hpp"

#include "context.hpp" // bodies must include their context....

// this is POBJECT in a 3D world...

#define MAX_BOUNDS 5 // only really use 3 or 4 now...
// save/load counts on a constant structure here
typedef struct boundry_tag{
   int nBounds;
   _POINT BoundStart;  // offset from origin of first boundry...
   _POINT pBounds[MAX_BOUNDS];
   int Collide[MAX_BOUNDS];  // maybe a hardness of collision???
} BOUNDRY, *PBOUNDRY;


class BODY {
public:
   int Bounds[16];
private:
   int TurnRight, TurnLeft;
   int Forward;  
   int Backward;
   int Right;
   int Left;
//   RCOORD minx, maxx;
//   RCOORD miny, maxy;

   RCOORD MaxMoveSpeed, 
          MaxRotateSpeed;
public:  // expose this so that mouse can use translation....
   // POBJECT ->  3d form like cube, prism
   //    can also be a 2d form from bound information....
   //    2d bodies can exist on the surface of 3d bodies 
   //    and share the same limitations of their 3d counterpars
   //    in that they may exist simultaneously between 2 or more
   //    spaces at once... as the body spans from one to another
   //    but they must also 
   POBJECT pO;  // boundry form of body...
private:
//   RCOORD minz, maxz;

   int Up;
   int Down;

   int TurnUp, TurnDown;
   int RollRight, RollLeft;
public:

   BRAIN_STEM bs; // for purposes of window creation...
   BRAIN *Brain;

   int nRadar; // number of radars mounted
   RADAR *Radar[3];   

   int nWeapons;
   WEAPON *Weapon[4]; // max weapons pwe body...

private:
   void Init( BOOL b3D, POBJECT pForm ); // set most initial values...
   void AddBoundries( void );

   class BODY *pNext; // another body... circular queue...
   class BODY *Other( class BODY *pb );
public:
   // this is equivalent of Load....
   BODY( char *pFile, PCVECTOR vo, PCVECTOR vforward, PCVECTOR vright );
   BODY( POBJECT pobj, PCVECTOR vo, PCVECTOR vforward, PCVECTOR vright, char *name) ;
   BODY( int x, int y, int Form, char *name);
   ~BODY( void );

   void AddRadar( RCOORD power, PCVECTOR vo );
   void Save( char *pFile );
   void Update( void );
   bool BodyOn( PCVECTOR vforward, PCVECTOR pright, PCVECTOR vo );
   void Draw( void );  // screen coordinates??
   void DrawRadar( int which );
   void DrawBoundry( void );  // screen coordinates??
   void CollideWorld( ); // hits pInWorld only...
   void CollideOthers(); // hits other objects...
//   void Origin( P_3POINT pv );
   //inline void InWorld( POBJECT po ) {pInWorld = po;}
   inline void Set3dForm( POBJECT po ) { pO = po; }
};

#endif