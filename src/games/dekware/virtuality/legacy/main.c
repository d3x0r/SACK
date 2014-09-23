#include <stdhdrs.h>
#include "autovers.h"
//#define PLUGIN_MODULE
#include "plugin.h"

#include "view.h"

int gbExitProgram;

PVIEW View[6];

void CALLBACK ViewMouse( HWND hWnd, 
                                    PCVECTOR vforward, 
                                    PCVECTOR vright, 
                                    PCVECTOR vup, 
                                    PCVECTOR vo, int b )
{
}



#define CUBE_SIDES 6
BASIC_PLANE CubeNormals[] = { { {0,0,0}, {    0,  0.5,    0 }/*, 1, 62 */},
                    { {0,0,0}, { -0.5,    0,    0 }/*, 1, 126 */},
                    { {0,0,0}, {  0.5,    0,    0 }/*, 1, 190 */},
                    { {0,0,0}, {    0,    0, -0.5 }/*, 1, 244 */},
                    { {0,0,0}, {    0,    0,  0.5 }/*, 1, 31 */},
                    { {0,0,0}, {    0, -0.5,    0 }/*, 1, 94 */},
                                                  
                    { {0,0,0}, {  0.22, 0.22,  0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.22,-0.22,  0.4}/*, 1, 46 */},
                    { {0,0,0}, { -0.22, 0.22,  0.4}/*, 1, 168 */},
                    { {0,0,0}, { -0.22,-0.22,  0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.22, 0.22, -0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.22,-0.22, -0.4}/*, 1, 46 */},
                    { {0,0,0}, { -0.22, 0.22, -0.4}/*, 1, 168 */},
                    { {0,0,0}, { -0.22,-0.22, -0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.43, 0.43,  0}/*, 1, 46 */},
                    { {0,0,0}, {  0.43,-0.43,  0}/*, 1, 46 */},
                    { {0,0,0}, { -0.43, 0.43,  0}/*, 1, 168 */},
                    { {0,0,0}, { -0.43,-0.43,  0}/*, 1, 46 */},
                    { {0,0,0}, {  0.43, 0, 0.43}/*, 1, 46 */},
                    { {0,0,0}, {  0.43, 0, -0.43}/*, 1, 46 */},
                    { {0,0,0}, { -0.43, 0, 0.43}/*, 1, 168 */},
                    { {0,0,0}, { -0.43, 0, -0.43}/*, 1, 46 */},

// offset to origin
                    { {0,0.5,0}, { 0, -0.1, 0}/*, -1, 126*/},
                    { {0.5,0.5,0}, { -0.1, 0, 0}/*, -1,  126*/},
                    { {-0.5,0.5,0}, { 0.1, 0, 0}/*, -1,  126*/},
                    { {0,0.5,0.5}, { 0, 0, -0.1}/*, -1,  126*/},
                    { {0,0.5,-0.5}, { 0, 0, 0.1}/*, -1,  126*/}

                  };

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShow )
{
   MSG msg;

   {
      POBJECT pWorld;
      pWorld = CreateScaledInstance( CubeNormals, CUBE_SIDES, 1500.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
      SetObjectColor( pWorld, Color( 79, 135, 153 ) );  // purple boundry...
      InvertObject( pWorld ); // turn object inside out...
   }
   //   View[5] = CreateViewEx( V_RIGHT, ViewMouse, "View Right", 400, 200 );
   //   View[4] = CreateViewEx( V_LEFT, ViewMouse, "View Left", 0, 200 );
   //   View[3] = CreateViewEx( V_BACK, ViewMouse, "View Behind", 600, 200 );
   //   View[2] = CreateViewEx( V_DOWN, ViewMouse, "View Down", 200, 400 );
   //   View[1] = CreateViewEx( V_UP, ViewMouse, "View Up", 200, 0 );

      // this MUST be last - so that it is the PRIMARY view of manipulation
		View[0] = CreateViewEx( V_FORWARD, ViewMouse, "View Forward", 200, 200 );
	{
		_POINT rotation;
		rotation[0] = 0.1;
		rotation[1] = 0.05;
		rotation[2] = 0.03;
      SetRotation( View[0]->T, rotation );
	   ShowTransform( View[0]->T);
	}

   //while( GetMessage( &msg, NULL, 0, 0 ) )
   //   DispatchMessage( &msg );   

   return 0;
}

int LoadBody( char *pName );

PUBLIC( char *, RegisterRoutines )( void )
{
	//pExportedFunctions = pExportTable;
	LoadBody( "body.shp" );
   WinMain( 0, 0, NULL, 0 );
   return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   
}

//#ifdef __CYGWIN__
// or - "-e _mainCRTStartup" to your link line in your Makefile.

//WinMainCRTStartup() { mainCRTStartup(); }
//#endif
// $Log: main.c,v $
// Revision 1.4  2003/03/25 08:59:03  panther
// Added CVS logging
//
