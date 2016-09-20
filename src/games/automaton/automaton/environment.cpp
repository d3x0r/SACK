//#include <afxwin.h>
#include <windows.h>
#include <stdio.h>

#include <vectlib.h>
#include <timers.h>

extern "C" {
#include "vidlib.h"
#include "image.h"
};

#include "brain.hpp"
#include "bboard.hpp"

//------------------------
#include "object.hpp"
#include "view.hpp"
#include "shapes.hpp"

POBJECT pFlatWorld;
POBJECT pWorld;   // bodies are within world...

VIEW   *View[6];  // keep pointers for closing...

BOOL   b3D;
BOOL   bDebug;

//-------------------------------------------

 // display characteristics - cannot be 
// defined quite the way I was gonna do it
// but we should only have one world per program
// multiple boards though - and I'm not entirely
// sure whether they are there or not....duh ?
// how to wrap these correctly...

int nScale = 4;
HMENU hMenuZoom;
#define SCALE nScale

#define MNU_ADDBODY		1000
#define MNU_LASTBODY    1090
#define MNU_DELBODY		1091
#define MNU_LOAD        1092
#define MNU_SAVE        1093

#define MNU_ADDRADAR    1100
#define MNU_LASTRADAR   1190
#define MNU_DELRADAR    1191

#define MNU_ADDWEAPON    1100
#define MNU_LASTWEAPON   1190
#define MNU_DELWEAPON    1191

#define MNU_OPENBRAIN	1500 

enum {
   MNU_ZOOM = 1600,
   MNU_ZOOM2,   // 2
   MNU_ZOOM3,   // 4
   MNU_ZOOM4,   // 8
   MNU_ZOOM5,   //16
   MNU_ZOOM6,   //32
   MNU_ZOOM7,   //64
   MNU_ZOOM8,   //128
};

PRENDERER hVideo; // flat world window....

HMENU hMenuFlat;	   // main environment menu...
HMENU hMenuBodyFlat; 
HMENU hMenuSpace;
HMENU hMenuBodySpace; 

#define UNSCALE_X(x)  ( ( ((x)*4) / SCALE )) 
#define UNSCALE_Y(y)  ( ( ((y)*4) / SCALE ))

#define DISPLAY_X(x)  ( UNSCALE_X( (x) - display_x ))
#define DISPLAY_Y(y)  ( UNSCALE_Y( (y) - display_y ))

#define SCALE_X(x) ( (( x ) * SCALE ) / 4 )
#define SCALE_Y(y) ( (( y ) * SCALE ) / 4 )

#define REAL_X(x) (SCALE_X(x) + display_x)
#define REAL_Y(y) (SCALE_Y(y) + display_y)

int display_x = 10; // tranlation of display of plane of flatness....
int display_y = 10;

//---------------------------------------------
//------------------------------------------

static BOOL SelectExistingFile( HWND hParent, PSTR szFile)
{
   
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
   int x;
   szFile[0] = 0;
   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = "Bodies\0*.Body\0";
   ofn.nFilterIndex = 1;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
              | OFN_NOREADONLYRETURN;// Display the Open dialog box. 

   x = GetOpenFileName(&ofn);
   return x;
   
 //  return FALSE;
}

//------------------------------------------
static BOOL SelectNewFile( HWND hParent, PSTR szFile )
{
   
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
   szFile[0] = 0;
   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = "Bodies\0*.Body\0";
   ofn.nFilterIndex = 1;
   ofn.Flags = OFN_NOTESTFILECREATE
              | OFN_NOREADONLYRETURN ;// Display the Open dialog box. 

   return GetOpenFileName(&ofn);
}

//------------------------------------------
#include "body.hpp"

#define MAX_BODIES 100

	BODY *Body[MAX_BODIES];
	char szName[64];


   // laser direction... pointer 
BODY *OnBody( PCVECTOR vforward, //view slope
              PCVECTOR vright, // cross pointer...
              PCVECTOR vo  ) // view origin
{
	for( int i = 0; i < MAX_BODIES; i++ )
		if( Body[i]&&
			Body[i]->BodyOn( vforward,
                          vright,
                          vo ) )
		return Body[i];
	return NULL;
}

void CALLBACK EnvironmentMouse( PRENDERER display, 
                                    PCVECTOR vforward, 
                                    PCVECTOR vright, 
                                    PCVECTOR vup, 
                                    PCVECTOR vo, int b )
{
   BOOL bLeft;
   BOOL bRight;
	static BOOL _bLeft, _bRight;	  // goes up, goes down....
   static int _x, _y;
   int x, y, z;

   static BOOL bDragging;
   static BODY *pb;
   static POBJECT pInWorld;

   x = (int)vo[vRight];
   y = (int)vo[vUp];
   z = (int)vo[vForward];

   bLeft = (b & MK_LBUTTON) | (b&KEY_BUTTON1);
   bRight = (b & MK_RBUTTON) | (b&KEY_BUTTON2);

//	pb = OnBody( vforward, vright, vo ); // test - for body reactive highlight

   if( bRight )
   {
      if( !_bRight )
      {
         POBJECT po;
         pb       = NULL; 
         pInWorld = NULL;
         FORALL( pFlatWorld, &po )
            if( ObjectOn( po, vforward, vright, vo ) )
            {   
               pInWorld = po; // got at least this far....
               if( ( pb = OnBody( vforward, vright, vo ) ) )
                  break; // got a body - can leave now...
            }
      }
   }
   else
   {
      if( _bRight ) // catch full click cycle before adding body...
      {
		   int nCmd;

		   if( !pb )
		   {
            POINT p;
            if( pInWorld )
{
               GetCursorPos( &p );
               nCmd = TrackPopupMenu( (b3D)?hMenuSpace:hMenuFlat, 
                                 TPM_CENTERALIGN 
										   | TPM_TOPALIGN 
										   | TPM_RIGHTBUTTON 
										   | TPM_RETURNCMD 
										   | TPM_NONOTIFY
										   ,p.x
										   ,p.y
                                 ,0
										   ,NULL //hWnd
										   ,NULL);
               if( nCmd >= MNU_ZOOM && nCmd <= MNU_ZOOM+10 )
               {
                  int nOld;
                  nCmd -= MNU_ZOOM;
                  nOld = 0;
                  {
                     int rx, ry;

                     rx = x; 
                     ry = y; 
                     x = DISPLAY_X(x);
                     y = DISPLAY_Y(y);

                     while( nScale > 1 ) {
                        nOld++; 
                        nScale >>= 1;
                     }
                     CheckMenuItem( hMenuZoom, nOld, MF_UNCHECKED|MF_BYPOSITION );
                     CheckMenuItem( hMenuZoom, nCmd, MF_CHECKED|MF_BYPOSITION );

                     nScale = 1 << ( nCmd - MNU_ZOOM );

                     display_x += rx - REAL_X(x);
                     display_y += ry - REAL_Y(y);
                  }

               }
               else
			      switch( nCmd )
			      {
                  int i;
               case MNU_LOAD:
				      for( i = 0; i < MAX_BODIES; i++ ) {
					      if( !Body[i] ) {    
                        char byFile[256];
                        byFile[0] = 0;
                        if( SelectExistingFile( NULL /*hWnd*/, (char*)byFile ) ) {
                           if( !strstr( (char*)byFile, ".Body" ) )
                               strcat( (char*)byFile, ".Body" );
                           Body[i] = new BODY( (char*)byFile, vo, vforward, vright );
                        }
                        break;
                     }
                  }
                  break;

               {
                  POBJECT po;
			      case MNU_ADDBODY+0:
                  if(0)
                  {
                      if( b3D )
                         po = CreateScaledInstance( PyramidNormals, PYRAMID_SIDES, 20.0, vo, vforward, vright, vup );
                  }
                  if(0)
			      case MNU_ADDBODY+1:
                  {
                     if( b3D )
                     {
                         po = CreateScaledInstance( CubeNormals, CUBE_SIDES, 20.0, vo, vforward, vright, vup );
                     }
                  }

				       for( i = 0; i < MAX_BODIES; i++ )
				       {
					      if( !Body[i] )
					      {
                        if( !b3D )
                        {
                           // this created a POBJECT and
                           // puts it in current 'pFlatWorld
					            Body[i] = new BODY( x, y, 
								         	            nCmd - MNU_ADDBODY, 
										                 "BODY ###" );
					            break; // body DONE;
					         }
                        else
                        {
					            Body[i] = new BODY( po, vo, vforward, vright, "BODY ###" );
                           break; // body DONE;
                        }
					      }
                  }
               } // close of nasty variable enclosure (at wrong level)
                  break; // last break in switch....
               }
             //  break; // get out of FORALL flatworlds....
             }
          }
		    else // if (!pb) therefore pb = current body....
		    {
             POINT p;
             GetCursorPos( &p );
             nCmd = TrackPopupMenu( (b3D)?hMenuBodySpace:hMenuBodyFlat, 
                                 TPM_CENTERALIGN 
										   | TPM_TOPALIGN 
										   | TPM_RIGHTBUTTON 
										   | TPM_RETURNCMD 
										   | TPM_NONOTIFY
										   ,p.x
										   ,p.y
                                 ,0
										   ,NULL // hWnd
										   ,NULL);
			    switch( nCmd )
			    {
             case MNU_SAVE:
                if( pb )
				   {
                  char byFile[256];
                  if( SelectNewFile( NULL /*hWnd*/, (char*)byFile ) )
                  {
                     if( !strstr( (char*)byFile, ".Body" ) )
                         strcat( (char*)byFile, ".Body" );
                     pb->Save( (char*)byFile );
                  }
               }
               break;
             case MNU_ADDRADAR: // this 
                if( !b3D )
                {
                   VECTOR v;

                   // justify mouse cursor to body relative...
                   pb->pO->T.ApplyInverse( v, vo );
                   // radar should include 
                   // vforward, vRight (roation direction)
                   pb->AddRadar( 500.0F, v );
                }
                break;

			    case MNU_DELBODY:
				    {
					    int i;
					    for( i = 0; i < MAX_BODIES; i++ )
					    {
						    if( Body[i] == pb )
						    {
							    delete Body[i];
							    Body[i] = NULL;
						    }
					    }
				    }
				    break;

			    case MNU_OPENBRAIN:
                // what goes here I'm not sure...

                // this results in ShowWindow( BOARD );

				    break;
			    }
		    }
	    }
	}

   _x = x; _y = y;
   _bRight = bRight;
   _bLeft = bLeft;

}


//------------------------------------------

PFACET pCurrentView; // uhh.... :) 

void EnvironmentRefresh( DWORD unused, PRENDERER self ) // called when window resizes...
{
   int h, w;
	Image image = GetDisplayImage( hVideo );
//   int y, x;
   
   h = image->height;
   w = image->width;           

   ClearImage( image );
   
   {
      VECTOR v1, v2;  
      RAY r;
      int i;
      CDATA c;
      PLINESEGSET pls;
      PLINESEG pl;

      c = Color( 90, 120, 192 );

      pls = pFlatWorld->pLinePool;
      for( i = 0; i < pls->nUsedLines; i++ )
      {
         // unfortunatly we fucked with the
         // line definetion to include dFrom and dTo;
         // T.Apply( &r,  );
         pl = pls->pLines + i;
         SetRay( &r, &pl->d ); // zero align....
         scale( v1, r.n, pl->dFrom );
         scale( v2, r.n, pl->dTo );
         add( v1, v1, r.o );
         add( v2, v2, r.o );
         do_line( image
				,              (int)DISPLAY_X(v1[0]),
                           (int)DISPLAY_Y(v1[1]),
                           (int)DISPLAY_X(v2[0]), 
                           (int)DISPLAY_Y(v2[1]),
                           c );
      }
   }
   // find first one....
   for( int i = 0; i < MAX_BODIES; i++ )
   {
      if( Body[i] )
      {
         Body[i]->Draw(); // each body updates itself
      }
   }

   UpdateDisplay( hVideo );
}

//------------------------------------------
// keyboard extensions are not applied since
// this is direct from mouse - not from view...
int EnvironMouse_2( DWORD dwUnused, int32_t x, int32_t y, uint32_t b )
{
static BOOL _bLeft, _bRight;	  // goes up, goes down....
static int _x, _y;
static BOOL bDragging;

   BOOL bLeft  = b & MK_LBUTTON;
   BOOL bRight = b & MK_RBUTTON;
   
   if( bLeft )
   {
      if( !_bLeft ) 
      {
         bDragging = TRUE;
      }
      else // continued draggin....
      {
         if( bDragging )
         {
            display_x += SCALE_X( _x - x );
            display_y += SCALE_Y( _y - y );
            EnvironmentRefresh( 0 );
         }
      }
   }
   else
   {
      if( _bLeft ) // up edge....
      {
         if( bDragging )
            bDragging = FALSE;
      }
   }


   {
      VECTOR v;
      int bsave;

      v[vRight]   = (RCOORD)REAL_X(x);  // consider DISPLAY_X 
      v[vUp]      = (RCOORD)REAL_Y(y);  // consider UNSCALE_Y
      v[vForward] = 0.0;
      bsave = b3D;
      b3D = FALSE;
      EnvironmentMouse( hVideo, VectorConst_Z, VectorConst_X, VectorConst_Y, v, b );
      b3D = bsave;
   }

   _x = x; _y = y;
   _bRight = bRight;
   _bLeft = bLeft;

}

//------------------------------------------
unsigned char gbExitProgram;

void EnvironClose( DWORD dwUnused )
{
   int i;
   gbExitProgram = TRUE;
   for( i = 0; i < MAX_BODIES; i++ )
   {
      if( Body[i] ) 
      {
         delete Body[i];
         Body[i] = NULL;
      }
   }

//   if( b3D )
   {
      for( i = 0; i < 6; i++ )
      {
         if( View[i] )
         {
            delete( View[i] );
            View[i] = NULL;
         }
      }
   }
//   else
   {
      if( hVideo )
      {
         CloseDisplay( hVideo ); 
         hVideo = NULL;
      }
   }
}

//------------------------------------------


//extern "C" void BlotImageMultiShaded( HVIDEO hDest, ImageFile *pIF, int x, int y, CDATA r, CDATA g, CDATA b );
Image pTest;

void CPROC UpdateBodies( uintptr_t psv )
{
   int i;
   for( i = 0; i < MAX_BODIES; i++ )
      if( Body[i] )
      {
         Body[i]->Update();
      }

   if( hVideo )
      EnvironmentRefresh( 0 );

}

//------------------------------------------

void EnvironmmentInit()
{
   VECTOR vo, vn;
   InitializeBoard( );

//   if( b3D )
   {
      pWorld = CreateScaledInstance( CubeNormals, CUBE_SIDES, 1500.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
   //   SetObjectSolid( pWorld );  // consider object switch wire_frame/solid...
      SetObjectColor( pWorld, Color( 79, 135, 153 ) );  // purple boundry...
      InvertObject( pWorld ); // turn object inside out...
      // only reference is here?  no - bodies reference pWorld as extern

      if( bDebug )
      {
         int x, y, z;
         z = 0;
         for( x = -5; x < 5; x++ )
            for( y = -5; y < 5; y++ )
               for( z = -5; z < 5; z++ )
               {
                  VECTOR vo;
                  vo[0] = (RCOORD)(x * 30);
                  vo[1] = (RCOORD)(y * 30);
                  vo[2] = (RCOORD)(z * 30);
                  CreateScaledInstance( CubeNormals, CUBE_SIDES, 20.0, vo, VectorConst_0, VectorConst_0, VectorConst_0 );
               }
       }
   }

   if( bDebug )
      pTest = LoadImageFile( "images/multi-blob.gif" );

   hMenuFlat = CreatePopupMenu();
   AppendMenu( hMenuFlat, MF_STRING, MNU_ADDBODY, "Add Triangle" );
   AppendMenu( hMenuFlat, MF_STRING, MNU_ADDBODY+1, "Add Rectangle" );

   AppendMenu( hMenuFlat,MF_STRING|MF_POPUP, (DWORD)CreatePopupMenu(), "Zoom" );
   {
      hMenuZoom = GetSubMenu( hMenuFlat, 2 );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM, "x1/4" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+1, "x1/2" );
      AppendMenu( hMenuZoom,MF_STRING|MF_CHECKED, MNU_ZOOM+2, "x1" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+3, "x2" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+4, "x4" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+5, "x8" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+6, "x16" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+7, "x32" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+8, "x64" );
      AppendMenu( hMenuZoom,MF_STRING, MNU_ZOOM+9, "x128" );
   }

   AppendMenu( hMenuFlat, MF_STRING, MNU_LOAD, "Load Body" );

   hMenuSpace = CreatePopupMenu();
   AppendMenu( hMenuSpace, MF_STRING, MNU_ADDBODY+0, "Add Prism" );
   AppendMenu( hMenuSpace, MF_STRING, MNU_ADDBODY+1, "Add Cube" );
   AppendMenu( hMenuSpace, MF_STRING, MNU_LOAD, "Load Body" );

   hMenuBodyFlat = CreatePopupMenu();
   AppendMenu( hMenuBodyFlat, MF_STRING, MNU_DELBODY, "Delete &Body" );
   AppendMenu( hMenuBodyFlat, MF_STRING, MNU_OPENBRAIN, "Open Brain" );
   AppendMenu( hMenuBodyFlat, MF_STRING, MNU_ADDRADAR, "Add Radar" );
   AppendMenu( hMenuBodyFlat, MF_STRING, MNU_ADDWEAPON, "Add Weapon" );
   AppendMenu( hMenuBodyFlat, MF_STRING, MNU_SAVE, "Save Body" );

   hMenuBodySpace = CreatePopupMenu();
   AppendMenu( hMenuBodySpace, MF_STRING, MNU_DELBODY, "Delete &Body" );
   AppendMenu( hMenuBodySpace, MF_STRING, MNU_OPENBRAIN, "Open Brain" );
   AppendMenu( hMenuBodySpace, MF_STRING, MNU_ADDRADAR, "Add Radar" );
   AppendMenu( hMenuBodySpace, MF_STRING, MNU_ADDWEAPON, "Add Weapon" );
   AppendMenu( hMenuBodySpace, MF_STRING, MNU_SAVE, "Save Body" );

//   if( b3D )
   {
      View[5] = new VIEW( V_RIGHT, EnvironmentMouse, "View Right", 400, 200 );
      View[4] = new VIEW( V_LEFT, EnvironmentMouse, "View Left", 0, 200 );
      View[3] = new VIEW( V_BACK, EnvironmentMouse, "View Behind", 600, 200 );
      View[2] = new VIEW( V_DOWN, EnvironmentMouse, "View Down", 200, 400 );
      View[1] = new VIEW( V_UP, EnvironmentMouse, "View Up", 200, 0 );

      // this MUST be last - so that it is the PRIMARY view of manipulation
      View[0] = new VIEW( V_FORWARD, EnvironmentMouse, "View Forward", 200, 200 );
   }
//   else
   {
      // create viewing plane...
      scale( vn, VectorConst_Z, 1 );
      pFlatWorld = CreatePlane( VectorConst_0, vn, VectorConst_X, 1000, Color( 199, 128, 255 ) );
      PutIn( pWorld, pFlatWorld );

//      View[0] = new VIEW( V_FORWARD, EnvironmentMouse, "View Forward", 200, 200 );

      hVideo = OpenDisplaySizedAt( 0, 0, 0, 0, 0 );
		SetMouseHandler( hVideo, EnvironMouse_2, 0 );
		SetRedrawHandler( hVideo, EnvironmentRefresh, 0 );
		SetCloseHandler( hVideo, EnvironClose, 0 );
   }
   scale( vo, VectorConst_Z, -500 );
   View[0]->Move( vo );

   AddTimer( 40, UpdateBodies, 0 );
}

//------------------------------------------
#include "editor.hpp"
EDITOR Editor;  // create IMMEDIATELY

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow ) // void main( void )
{
   b3D = TRUE;
   //bDebug = FALSE;

   if( strstr( lpCmd, "-3" ) )
      b3D = TRUE; // command line...

   if( strstr( lpCmd, "-d" ) )  
      bDebug = TRUE; // if 3d makes 1331 cubes... 

//   EnvironmmentInit( );// bypass environment creation for a moment...

   {
      MSG Msg;
      while( GetMessage( &Msg, NULL, 0, 0 ) && 
             !gbExitProgram )
         DispatchMessage( &Msg );
   }
   EnvironClose( 0 ); // .....
   return 0;
}

