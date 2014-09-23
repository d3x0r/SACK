//------------------------------------------

#define MAX_BOUNDS 4

// save/load counts on a constant structure here
typedef struct boundry_tag{
   int nBounds;
   _3POINT BoundStart;  // offset from origin of first boundry...
   _3POINT pBounds[MAX_BOUNDS];

   int Collide[MAX_BOUNDS];  // maybe a hardness of collision???

   _3POINT CurStart;
   _3POINT pCurBounds[MAX_BOUNDS];
} BOUNDRY, *PBOUNDRY;


BOUNDRY Forms[2] = { { 3, {-10, -10, 0 },
                       { {10, 30, 0 },
                         { 10, -30, 0 },
                         { -20, 0, 0 } } },
                     { 4, {10, 20, 0 },
                        { { 0, -40, 0 },
                         { -20, 0, 0 },
                         { 0, 40, 0 },
                         { 20, 0, 0 } } } 
                     };

