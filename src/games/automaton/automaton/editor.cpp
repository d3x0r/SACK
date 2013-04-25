
extern "C" {
#include "vidlib.h"
};

#include "editor.hpp"
#include "view.hpp" // perhaps should just USE view code?

#define MNU_ADDLINE           1000
#define MNU_ADDPREDEF         1010
#define MNU_ADDPREDEF_LAST    1020

VECTOR vToScreen;

#define MOUSE_LOCK 7

void EDITOR::EditMouseCallback( int x, int y, int b )
{
   static int _x, _y, _right, _left;
   int right, left;
   right = b & MK_RBUTTON;
   left = b & MK_LBUTTON;


   if( bLocked )
   {
      ptMouseDel.x += x - ptMouse.x;
      ptMouseDel.y += y - ptMouse.y;
   }

   if( !(left + right + _left + _right) ) // just moving mouse..
   {
      if( bLocked )
      {
         if( ptMouseDel.x > MOUSE_LOCK || ptMouseDel.x < -MOUSE_LOCK ||
             ptMouseDel.y > MOUSE_LOCK || ptMouseDel.y < -MOUSE_LOCK )
         {
            bLocked = FALSE;
            SetCursorPos( ptUpperLeft.x + ptMouse.x + ptMouseDel.x, 
                          ptUpperLeft.y + ptMouse.y + ptMouseDel.y );
            return; // done....
         }
      }
      else
      {
         if( ( abs( ptO.x - x ) < MOUSE_LOCK ) &&
             ( abs( ptO.y - y ) < MOUSE_LOCK ) )
         {
            bLocked = TRUE;
            ptMouseDel.x = 0;
            ptMouseDel.y = 0;
            ptMouse.x = ptO.x;
            ptMouse.y = ptO.y;
         }
         else
         if( ( abs( ptN.x - x) < MOUSE_LOCK ) &&
             ( abs( ptN.y - y ) < MOUSE_LOCK ) )
         {
            bLocked = TRUE;
            ptMouseDel.x = 0;
            ptMouseDel.y = 0;
            ptMouse.x = ptN.x;
            ptMouse.y = ptN.y;
         }
      }

      if( bLocked ) // only reset mouse if not dragging??
      {
         SetCursorPos( ptUpperLeft.x + ptMouse.x, 
                       ptUpperLeft.y + ptMouse.y );
      }

   }

   if( bLocked )
   {
      x = ptMouse.x;
      y = ptMouse.y;
   }

//   x -= hVideo->pImage->width/2;
//   y = hVideo->pImage->height/2 - y;

   if( !right && _right )
   {
      POINT p;
      int nCmd;
      GetCursorPos( &p );
      nCmd = TrackPopupMenu( hMenu, 
                        TPM_CENTERALIGN 
								| TPM_TOPALIGN 
								| TPM_RIGHTBUTTON 
								| TPM_RETURNCMD 
								| TPM_NONOTIFY
								,p.x
								,p.y
                        ,0
								,NULL // hVideo->hWndOutput
								,NULL);
      switch( nCmd )
      {
      case MNU_ADDLINE:
         break;
      }
   }

   if( left && !_left ) // left click select...
   {
      if( x == ptO.x && y == ptO.y )
      {
         bDragOrigin = TRUE;
      } 
      else if( x == ptN.x && y == ptN.y )
      {
         bDragNormal = TRUE;
      }
   }
   else if( !left && _left ) // end click...
   {
      bDragOrigin = FALSE; // cancel all operations...
      bDragNormal = FALSE;
   }
   else 
   {
      if( bDragOrigin )
      {
         VECTOR vo;
         if( ptMouseDel.x || ptMouseDel.y )
         {
            ptMouse.x += ptMouseDel.x;
            ptMouseDel.x = 0;
            ptMouse.y += ptMouseDel.y;
            ptMouseDel.y = 0;
            vo[2] = 10.0f;   // should be -(distance on z)
            GetRealPoint( GetDisplayImage( hVideo ), vo, &ptMouse );
            TView.Apply( pCurrentLine->d.o, vo ); // apply inverse -> Apply...
            // update Intersecting lines......
            {
               PLINESEG pl;
               PLINESEGPSET plps;
               int l;
               plps = pCurrentFacet->pLineSet;
               pl = pCurrentLine->pTo;
               
               for( l = 0; l < plps->nUsedLines; l++ )
               {
                  pl = plps->pLines[l];
                  if( pl == pCurrentLine )
                     continue;

               }

            }
            EditResizeCallback(); // update screen...
         }
      } 
      else if( bDragNormal )
      {
         VECTOR vn;
         if( ptMouseDel.x || ptMouseDel.y )
         {
            ptMouse.x += ptMouseDel.x;
            ptMouseDel.x = 0;
            ptMouse.y += ptMouseDel.y;
            ptMouseDel.y = 0;
            vn[2] = 10.0f;  // should be -(distance on z)
            GetRealPoint( GetDisplayImage( hVideo ), vn, &ptMouse );
            sub( vn, vn, pCurrentLine->d.o );
            TView.Apply( pCurrentLine->d.n, vn ); // apply inverse -> Apply...
            // update Intersecting lines......
            EditResizeCallback(); // update screen...
         }
      }
   }

   _x = x;
   _y = y;
   _right = right;
   _left  = left;
}

#define MARK_SIZE 3
void EDITOR::MarkOrigin( Image pImage )
{
   do_line( pImage, ptO.x - MARK_SIZE,
                    ptO.y - MARK_SIZE,
                    ptO.x + MARK_SIZE, 
                    ptO.y - MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, ptO.x + MARK_SIZE,
                    ptO.y - MARK_SIZE,
                    ptO.x + MARK_SIZE, 
                    ptO.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, ptO.x + MARK_SIZE,
                    ptO.y + MARK_SIZE,
                    ptO.x - MARK_SIZE, 
                    ptO.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, ptO.x - MARK_SIZE,
                    ptO.y + MARK_SIZE,
                    ptO.x - MARK_SIZE, 
                    ptO.y - MARK_SIZE, Color( 200, 200, 200 ) );
}


void EDITOR::MarkSlope( Image pImage )
{
   do_line( pImage, ptN.x,
                    ptN.y - MARK_SIZE,
                    ptN.x + MARK_SIZE, 
                    ptN.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, ptN.x + MARK_SIZE,
                    ptN.y + MARK_SIZE,
                    ptN.x - MARK_SIZE, 
                    ptN.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, ptN.x - MARK_SIZE,
                    ptN.y + MARK_SIZE,
                    ptN.x, 
                    ptN.y - MARK_SIZE, Color( 200, 200, 200 ) );
}

void EDITOR::EditResizeCallback( void )
{
   PFACETSET pfs;
   PFACET pf;
   int f;

   PLINESEGPSET plps;
   PLINESEG pl;
   int l;
   // ShowCurrent();
   ClearDisplay( hVideo );
   {
      RECT r;
      //GetWindowRect( hVideo->hWndOutput, &r );
		r.left = 0;
		r.top = 0;
      ptUpperLeft.x = r.left //+ GetSystemMetrics( SM_CXBORDER ) 
                             + GetSystemMetrics( SM_CXFRAME );
      ptUpperLeft.y = r.top //+ GetSystemMetrics( SM_CYBORDER )
                            + GetSystemMetrics( SM_CYFRAME )
                            + GetSystemMetrics( SM_CYCAPTION );
   }
   SetClip( (RCOORD)GetDisplayImage( hVideo )->width, 
            (RCOORD)GetDisplayImage( hVideo )->height );
   pfs = pCurrent->pPlaneSet;
   for( f = 0; f < pfs->nUsedPlanes; f++ )
   {
      pf = pfs->pPlanes + f;
      plps = pf->pLineSet;
      for( l = 0; l < plps->nUsedLines; l++ )
      {
         RAY ld; // local directional...
         pl = plps->pLines[l];
         if( pl == pCurrentLine )
         {
            TView.ApplyInverse( &ld, &pl->d );
            DrawLine( GetDisplayImage( hVideo ), ld.o, ld.n, pl->dFrom-10, pl->dFrom, Color( 192, 192, 0 ) );
            DrawLine( GetDisplayImage( hVideo ), ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 150, 0 ) );
            DrawLine( GetDisplayImage( hVideo ), ld.o, ld.n, pl->dTo, pl->dTo+10, Color( 192, 192, 0 ) );
            GetViewPoint( GetDisplayImage( hVideo ), &ptO, ld.o );
            add( ld.o, ld.o, ld.n ); // add one slope to origin to get slope point
            GetViewPoint( GetDisplayImage( hVideo ), &ptN, ld.o );
            MarkOrigin( GetDisplayImage( hVideo ) );
            MarkSlope( GetDisplayImage( hVideo ) );
         }
         else
         if( pf = pCurrentFacet )
         {
            TView.ApplyInverse( &ld, &pl->d );
            DrawLine( GetDisplayImage( hVideo ), ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 0, 150 ) );
         }
         else
         {
            TView.ApplyInverse( &ld, &pl->d );
            DrawLine( GetDisplayImage( hVideo ), ld.o, ld.n, pl->dFrom, pl->dTo, Color( 92, 92, 92 ) );
         }
      }
   }
	UpdateDisplay( hVideo );
}
   
void EDITOR::EditCloseCallback( void )
{
   delete this;
//   PostQuitMessage();
}

static int _EditMouseCallback( PTRSZVAL dwUser, S_32 x, S_32 y, _32 b )
{
   class EDITOR *Editor = (class EDITOR*)dwUser;
   Editor->EditMouseCallback( x, y, b );
	return 0;
}

static void _EditResizeCallback( PTRSZVAL dwUser, PRENDERER self )
{
   class EDITOR *Editor = (class EDITOR*)dwUser;
   Editor->EditResizeCallback( );
}

static void _EditCloseCallback( PTRSZVAL dwUser )
{
   class EDITOR *Editor = (class EDITOR*)dwUser;
   Editor->EditCloseCallback(  );
}


EDITOR::EDITOR( void )
{
   hVideo = OpenDisplaySizedAt( 0, 0, 0, 0, 0 );
	SetMouseHandler( hVideo, _EditMouseCallback, (PTRSZVAL)this );
	SetRedrawHandler( hVideo, _EditResizeCallback, (PTRSZVAL)this );
	SetCloseHandler( hVideo, _EditCloseCallback, (PTRSZVAL)this );

   // menu for this instance....
   hMenu = CreatePopupMenu();
   AppendMenu( hMenu, MF_STRING, MNU_ADDPREDEF+0, "Add Square" );
   AppendMenu( hMenu, MF_STRING, MNU_ADDPREDEF+1, "Add Triangle" );
   AppendMenu( hMenu, MF_STRING, MNU_ADDLINE, "Add Line" );

   SetPoint( vToScreen, VectorConst_Z ); // facing into screen...
   Invert( vToScreen );    // facing out of screen...

   pCurrent = 
      pRoot = CreateObject();

   pCurrentFacet = AddPlane( pRoot->pPlaneSet, vToScreen, VectorConst_0, 0 );
   {
      VECTOR o, n;
      SetPoint( o, VectorConst_X );
      SetPoint( n, VectorConst_Y );
      AddLineToPlane( pCurrentLine = 
                      CreateLine( pRoot->pLinePool, o, n, -1, 1 ),
                      pCurrentFacet );
      Invert( n );
      Invert( o );
      AddLineToPlane( CreateLine( pRoot->pLinePool, o, n, -1, 1 ),
                      pCurrentFacet );

      SetPoint( o, VectorConst_Y );
      SetPoint( n, VectorConst_X );
      Invert( n ); // try an keep it counter clockwise (for now)
      AddLineToPlane( CreateLine( pRoot->pLinePool, o, n, -1, 1 ),
                      pCurrentFacet );

      Invert( o );
      Invert( n );
      AddLineToPlane( CreateLine( pRoot->pLinePool, o, n, -1, 1 ),
                      pCurrentFacet );
   }
   TView.clear();  
   TView.Translate( 0, 
                    0, 
                    (RCOORD)-10.0 );

   bLocked = FALSE;

   EditResizeCallback();
}

EDITOR::~EDITOR( void )
{
   CloseDisplay( hVideo );
   FreeObject( &pRoot );
}

