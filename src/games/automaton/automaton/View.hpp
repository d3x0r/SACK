#include <windows.h>
#include <vectlib.h>
#include "vidlib.h"

#define ZERO_PLANE_DISTANCE (0.0)  // offset for far away...
#define FIELD_OF_VIEW ( 600.0 )

typedef void (CALLBACK *ViewMouseCallback)( PRENDERER, PCVECTOR vforward, 
                                                       PCVECTOR vright, 
                                                       PCVECTOR vup, 
                                                       PCVECTOR vorigin, int b );

// extended properties from View interface...
#define KEY_BUTTON1 0x10
#define KEY_BUTTON2 0x20
#define KEY_BUTTON3 0x40

enum {
   V_FORWARD,
   V_RIGHT,
   V_LEFT,
   V_BACK,
   V_UP,
   V_DOWN
};

class VIEW {
//   HVIDEO hVideo;
   ViewMouseCallback MouseMethod;
public:  // used in TimerProc  update...
	PRENDERER hVideo; // debug mouse purposes....
	Image image;
   TRANSFORM T; // current view matrix...
   VECTOR r; // constant additional rotation from 'previous'...
   int Type;
   class VIEW *Previous;  
private:
public:
   void UpdateCursorPos( int x, int y);
   void DoMouse( void );
   VIEW( ViewMouseCallback pMC, char *Title );
   // to accomidate dislocated views - need
   // to pass both the origin, and the rotation difference...
//   VIEW( PVECTOR pr, ViewMouseCallback pMC, char *Title, int sx, int sy ); // set view rotation...
   VIEW( int Type, ViewMouseCallback pMC, char *Title, int sx, int sy ); // set view rotation...
   ~VIEW();
   void Update( void );
   void ShowObjects( void );
   void ShowObject( POBJECT po );
   void ShowObjectChildren( POBJECT po );
   inline void Move( PCVECTOR v ) { T.Translate( v ); }
};

void SetClip( RCOORD width, RCOORD height );
void DrawLine( Image pImage, VECTOR p, VECTOR m, RCOORD t1, RCOORD t2, CDATA c );
void GetViewPoint( Image pImage, POINT *presult, PVECTOR vo );
void GetRealPoint( Image pImage, PVECTOR vresult, POINT *pt );



   
