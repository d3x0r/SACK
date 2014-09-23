#ifndef GLFRAME_DEFINED
#define GLFRAME_DEFINED
#define KEYBOARD_DEFINITION
#include <gl\gl.h>								// Header File For The OpenGL32 Library
#include <gl\glu.h>								// Header File For The GLu32 Library
#include <gl\glaux.h>								// Header File For The GLaux Library

#include "matrix.h"

typedef struct GLFrame_tag {
   TRANSFORM T;
   HGLRC		hRC;							// Permanent Rendering Context
   HDC		hDC;							// Private GDI Device Context
   int x, y; // upper left position of window...
   HWND		hWnd;							// Holds Our Window Handle

   BOOL	light;									// Lighting ON / OFF
   GLuint   filter;
   float aspect; // current aspect ratio....
   int	active;								// Window Active Flag Set To TRUE By Default
   int	fullscreen;							// Fullscreen Flag Set To Fullscreen Mode By Default
   int mouse_x, mouse_y, mouse_b;  // current state of mouse position and buttons..
   void (*MouseMethod)(DWORD dwUser, int x, int y, int b );
   DWORD dwMouseData;
   struct GLFrame_tag *next, *prior;
} GLFRAME, *PGLFRAME;

typedef struct keyboard_new_tag
{
#define NUM_KEYS 256  // one byte index... more than sufficient
   char keydown[NUM_KEYS];
   char keydouble[NUM_KEYS];
   unsigned int  keytime[NUM_KEYS];
   unsigned char key[NUM_KEYS];
} KEYBOARD2, *PKEYBOARD2;

#include "keydefs.h"

int IsKeyDown( unsigned char c );
BOOL KeyDown( unsigned char c );
BOOL KeyDouble( unsigned char c );

BOOL InitGLWindows( void );
void ExitGLWindows( void );

BOOL CreateGLWindow(PGLFRAME pglFrame, char* title, int x, int y, int width, int height, int bits, int fullscreenflag);
int DrawGLScene(PGLFRAME pglFrame);								// Here's Where We Do All The Drawing


#endif// $Log: glframe.h,v $
#endif// Revision 1.2  2003/03/25 08:59:01  panther
#endif// Added CVS logging
#endif//
