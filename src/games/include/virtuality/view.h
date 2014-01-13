//#include <windows.h>
#include <vectlib.h>
#include <render.h>
#include <controls.h>

#include <virtuality/object.h>
	
#ifndef __VIRTUALITY_VIEW_MODULE__
#define __VIRTUALITY_VIEW_MODULE__
#define ZERO_PLANE_DISTANCE (0.0)  // offset for far away...
#define FIELD_OF_VIEW ( 600.0 )

typedef LOGICAL (*ViewMouseCallback)( PRENDERER hWnd, PCVECTOR vforward, 
                                                       PCVECTOR vright, 
                                                       PCVECTOR vup, 
                                                       PCVECTOR vorigin, int b );

// extended properties from View interface...
#define KEY_BUTTON1 0x10
#define KEY_BUTTON2 0x20
#define KEY_BUTTON3 0x40

enum {
   V_FORWARD = 0,
   V_RIGHT,
   V_LEFT,
	V_BACK,
	V_BACKWARD = V_BACK,
	V_BEHIND = V_BACK,
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
   Image hud_surface; // blah - I hate putting this here - you will abuse it.
   PSI_CONTROL pcVideo;
	
   //PTRANSFORM T; // current view matrix...
   //PTRANSFORM Tglobal; // MY transform plus current object transform
   //PTRANSFORM TDelta; // computed from current object plus TGlobal plus a delta(?)
	//PTRANSFORM Twork; // MY transform plus current object transform
   //PTRANSFORM Tcamera; // this is given from the display
   VECTOR r; // constant additional rotation from 'previous'...
   int Type;
   struct view_tag *Previous;  
	int nFracture;
	PCRITICALSECTION csUpdate;
} VIEW, *PVIEW;

void UpdateCursorPos( PVIEW pv, int x, int y);
void UpdateThisCursorPos( void );
int DoMouse( PVIEW pv );
VIRTUALITY_EXPORT PVIEW CreateView( ViewMouseCallback pMC, char *Title );

   // to accomidate dislocated views - need
   // to pass both the origin, and the rotation difference...
//   VIEW( PVECTOR pr, ViewMouseCallback pMC, char *Title, int sx, int sy ); // set view rotation...
VIRTUALITY_EXPORT PVIEW CreateViewEx( int Type, ViewMouseCallback pMC, TEXTCHAR *Title, int sx, int sy ); // set view rotation...

VIRTUALITY_EXPORT PVIEW CreateViewWithUpdateLockEx( int nType, ViewMouseCallback pMC, TEXTCHAR *Title, int sx, int sy, PCRITICALSECTION csUpdate );

void DestroyView(PVIEW pv);
void CPROC Update( PSI_CONTROL pv );
// pass NULL to render into current context.
VIRTUALITY_EXPORT void ShowObjects( void );
void ShowObject( POBJECT po );
void ShowObjectChildren( PTRANSFORM TCam, POBJECT po );

void MoveView( PVIEW pv, PCVECTOR v );

//void SetClip( RCOORD width, RCOORD height );
//void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c );
//void GetViewPoint( Image pImage, IMAGE_POINT presult, PVECTOR vo );
//void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt );




#endif
