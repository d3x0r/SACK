//#include <windows.h>
#include <vectlib.h>
#include <render.h>

#include "object.h"
	
#ifndef __VIRTUALITY_VIEW_MODULE__
#define __VIRTUALITY_VIEW_MODULE__
#define ZERO_PLANE_DISTANCE (0.0)  // offset for far away...
#define FIELD_OF_VIEW ( 600.0 )

typedef void (*ViewMouseCallback)( PRENDERER hWnd, PCVECTOR vforward, 
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

typedef struct view_tag
{
	struct {
		_32 bInited : 1;
	} flags;
   ViewMouseCallback MouseMethod;
   // used in TimerProc  update...
   PRENDERER hVideo; // debug mouse purposes....

   PTRANSFORM T; // current view matrix...
   PTRANSFORM Tglobal; // MY transform plus current object transform
   PTRANSFORM Twork; // MY transform plus current object transform
   VECTOR r; // constant additional rotation from 'previous'...
   int Type;
   struct view_tag *Previous;  
	int nFracture;
} VIEW, *PVIEW;

void UpdateCursorPos( PVIEW pv, int x, int y);
void UpdateThisCursorPos( void );
void DoMouse( PVIEW pv );
VIRTUALITY_EXPORT PVIEW CreateView( ViewMouseCallback pMC, char *Title );

   // to accomidate dislocated views - need
   // to pass both the origin, and the rotation difference...
//   VIEW( PVECTOR pr, ViewMouseCallback pMC, char *Title, int sx, int sy ); // set view rotation...
VIRTUALITY_EXPORT PVIEW CreateViewEx( int Type, ViewMouseCallback pMC, TEXTCHAR *Title, int sx, int sy ); // set view rotation...
void DestroyView(PVIEW pv);
void Update( PVIEW pv );
void ShowObjects( PVIEW pv );
void ShowObject( PVIEW pv, POBJECT po );
void ShowObjectChildren( PVIEW pv, POBJECT po );

void MoveView( PVIEW pv, PCVECTOR v );

void SetClip( RCOORD width, RCOORD height );
void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c );
void GetViewPoint( Image pImage, IMAGE_POINT presult, PVECTOR vo );
void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt );




#endif
