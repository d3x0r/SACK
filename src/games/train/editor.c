
#include "global.h"

#include <image.h>
#include <vidlib.h>
#include <vectlib.h>

#include <sharemem.h>
//#include ""
//#include "view.hpp" // perhaps should just USE view code?
#include "view.h"


#define MNU_ADDLINE           1000
#define MNU_ADDPREDEF         1010
#define MNU_ADDPREDEF_LAST    1020

VECTOR vToScreen;

#define MOUSE_LOCK 7


typedef struct EDITOR_tag {
   PTRANSFORM TView;
   PRENDERER hVideo;
   HMENU  hMenu;  // first one should have 'add line'
   POBJECT pCurrent;
   PFACET pCurrentFacet;
   PLINESEG  pCurrentLine;
   POINT ptO, ptN;
   struct {
      int bLocked:1;  // mouse is locked to ptMouse...
      int bDragOrigin:1;
      int bDragNormal:1;
      int bDragWorld:1;
   };
   POINT ptMouse; // current point of mouse....
   POINT ptMouseDel; // accumulator of mouse delta (if locked)
   POINT ptUpperLeft; // upper left screen coordinate of window...
} EDITOR, *PEDITOR;

void EditResizeCallback( PTRSZVAL dwUser );


void EditMouseCallback( PTRSZVAL dwUser, int x, int y, int b )
{
   static int _x, _y, _right, _left;
   int right, left;
   PEDITOR pe = (PEDITOR)dwUser;
   right = b & MK_RBUTTON;
   left = b & MK_LBUTTON;

   if( IsKeyDown( pe->hVideo, KEY_PGUP ) )
   {
   	TranslateRel( pe->TView, 0, 0, -1.0 );
   	EditResizeCallback( (PTRSZVAL)pe );
   }
   else if( IsKeyDown( pe->hVideo, KEY_PGDN ) )
   {
   	TranslateRel( pe->TView, 0, 0, 1.0 );
   	EditResizeCallback( (PTRSZVAL)pe );
   }

   //if( pe->bLocked )
   //{/
   //}
   pe->ptMouseDel.x = x - pe->ptMouse.x;
   pe->ptMouseDel.y = y - pe->ptMouse.y;
   pe->ptMouse.x = x;
   pe->ptMouse.y = y;

   if( !(left + right + _left + _right) ) // just moving mouse..
   {
      if( pe->bLocked )
      {
         if( pe->ptMouseDel.x > MOUSE_LOCK || pe->ptMouseDel.x < -MOUSE_LOCK ||
             pe->ptMouseDel.y > MOUSE_LOCK || pe->ptMouseDel.y < -MOUSE_LOCK )
         {
            pe->bLocked = FALSE;
            SetMousePosition( pe->hVideo, pe->ptMouse.x, pe->ptMouse.y );
            return; // done....
         }
      }
      else
      {
         if( ( abs( pe->ptO.x - x ) < MOUSE_LOCK ) &&
             ( abs( pe->ptO.y - y ) < MOUSE_LOCK ) )
         {
            pe->bLocked = TRUE;
            pe->ptMouseDel.x = 0;
            pe->ptMouseDel.y = 0;
            pe->ptMouse.x = pe->ptO.x;
            pe->ptMouse.y = pe->ptO.y;
         }
         else
         if( ( abs( pe->ptN.x - x) < MOUSE_LOCK ) &&
             ( abs( pe->ptN.y - y ) < MOUSE_LOCK ) )
         {
            pe->bLocked = TRUE;
            pe->ptMouseDel.x = 0;
            pe->ptMouseDel.y = 0;
            pe->ptMouse.x = pe->ptN.x;
            pe->ptMouse.y = pe->ptN.y;
         }
      }

      if( pe->bLocked ) // only reset mouse if not dragging??
      {
	      pe->ptMouse.x -= pe->ptMouseDel.x;
   	   pe->ptMouse.y -= pe->ptMouseDel.y;
         SetMousePosition( pe->hVideo, pe->ptMouse.x,
                                  pe->ptMouse.y );
      }

   }

   if( pe->bLocked )
   {
      x = pe->ptMouse.x - pe->ptMouseDel.x;
      y = pe->ptMouse.y - pe->ptMouseDel.x;
   }

   if( !right && _right )
   {
      POINT p;
      int nCmd;
      GetCursorPos( &p );
      nCmd = TrackPopupMenu( pe->hMenu, 
                        TPM_CENTERALIGN 
								| TPM_TOPALIGN 
								| TPM_RIGHTBUTTON 
								| TPM_RETURNCMD 
								| TPM_NONOTIFY
								,p.x
								,p.y
                        ,0
								,GetNativeHandle( pe->hVideo )
								,NULL);
      switch( nCmd )
      {
      case MNU_ADDLINE:
         break;
      }
   }

   if( left && !_left ) // left click select...
   {
      if( x == pe->ptO.x && y == pe->ptO.y )
      {
         pe->bDragOrigin = TRUE;
      } 
      else if( x == pe->ptN.x && y == pe->ptN.y )
      {
         pe->bDragNormal = TRUE;
      }
      else
      {
      	pe->bDragWorld = TRUE;
      }	
   }
   else if( !left && _left ) // end click...
   {
      pe->bDragOrigin = FALSE; // cancel all operations...
      pe->bDragNormal = FALSE;
      pe->bDragWorld = FALSE;
   }
   else if( left && _left ) // probably dont want to do this if !left && !_left
   {
      if( pe->bDragOrigin )
      {
         VECTOR vo;
         if( pe->ptMouseDel.x || pe->ptMouseDel.y )
			{
            Image surface = GetDisplayImage( pe->hVideo );
            GetOriginV( pe->TView, vo );
            Invert( vo );
            GetRealPoint( surface, vo, &pe->ptMouse );
            Apply( pe->TView, pe->pCurrentLine->r.o, vo ); // apply inverse -> Apply...
            // update Intersecting lines......
            /*
            {
               PLINESEG pl;
               PLINESEGPSET plps;
               int l;
               plps = pe->pCurrentFacet->pLineSet;
               pl = pe->pCurrentLine->pTo;
               
               for( l = 0; l < plps->nUsedLines; l++ )
               {
                  pl = plps->pLines[l];
                  if( pl == pe->pCurrentLine )
                     continue;

               }

            }
            */
            EditResizeCallback( (PTRSZVAL)pe ); // update screen...
         }
      } 
      else if( pe->bDragNormal )
      {
         VECTOR vn;
         if( pe->ptMouseDel.x || pe->ptMouseDel.y )
			{
            Image pImage = GetDisplayImage( pe->hVideo );
            GetOriginV( pe->TView, vn );
            Invert( vn );
            GetRealPoint( pImage, vn, &pe->ptMouse );
            sub( vn, vn, pe->pCurrentLine->r.o );
            Apply( pe->TView, pe->pCurrentLine->r.n, vn ); // apply inverse -> Apply...
            // update Intersecting lines......
            EditResizeCallback( (PTRSZVAL)pe ); // update screen...
         }
      }
      else if( pe->bDragWorld )
      {
         VECTOR v1, v2;
         if( pe->ptMouseDel.x || pe->ptMouseDel.y )
			{
            Image pImage = GetDisplayImage( pe->hVideo );
            GetOriginV( pe->TView, v1 );
            Invert( v1 );
            v2[vForward] = v1[vForward];
				pe->ptMouseDel.x = pe->ptMouse.x - pe->ptMouseDel.x;
				pe->ptMouseDel.y = pe->ptMouse.y - pe->ptMouseDel.y;

            GetRealPoint( pImage, v1, &pe->ptMouse );
            GetRealPoint( pImage, v2, &pe->ptMouseDel );
            sub( v1, v2, v1 );
            v1[vForward] = 0; 
            TranslateRelV( pe->TView, v1 );
            EditResizeCallback( (PTRSZVAL)pe ); // update screen...
         }
      }

   }

   _x = x;
   _y = y;
   _right = right;
   _left  = left;
}

#define MARK_SIZE 3
void MarkOrigin( PEDITOR pe, Image pImage )
{
   do_line( pImage, pe->ptO.x - MARK_SIZE,
                    pe->ptO.y - MARK_SIZE,
                    pe->ptO.x + MARK_SIZE, 
                    pe->ptO.y - MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptO.x + MARK_SIZE,
                    pe->ptO.y - MARK_SIZE,
                    pe->ptO.x + MARK_SIZE, 
                    pe->ptO.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptO.x + MARK_SIZE,
                    pe->ptO.y + MARK_SIZE,
                    pe->ptO.x - MARK_SIZE, 
                    pe->ptO.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptO.x - MARK_SIZE,
                    pe->ptO.y + MARK_SIZE,
                    pe->ptO.x - MARK_SIZE, 
                    pe->ptO.y - MARK_SIZE, Color( 200, 200, 200 ) );
}


void MarkSlope( PEDITOR pe, Image pImage )
{
   do_line( pImage, pe->ptN.x,
                    pe->ptN.y - MARK_SIZE,
                    pe->ptN.x + MARK_SIZE, 
                    pe->ptN.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptN.x + MARK_SIZE,
                    pe->ptN.y + MARK_SIZE,
                    pe->ptN.x - MARK_SIZE, 
                    pe->ptN.y + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptN.x - MARK_SIZE,
                    pe->ptN.y + MARK_SIZE,
                    pe->ptN.x, 
                    pe->ptN.y - MARK_SIZE, Color( 200, 200, 200 ) );
}

void EditResizeCallback( PTRSZVAL dwUser  )
{
   PEDITOR pe = (PEDITOR)dwUser;
   PFACETSET pfs;
   PFACET pf;
   int f;

   PLINESEGPSET plps;
   PLINESEGSET pls;
   PLINESEG pl;
	int l;
   Image pImage = GetDisplayImage( pe->hVideo );
   // ShowCurrent();
   ClearImage( pImage );
   {
      RECT r;
      GetWindowRect( GetNativeHandle( pe->hVideo ), &r );
      pe->ptUpperLeft.x = r.left //+ GetSystemMetrics( SM_CXBORDER ) 
                             + GetSystemMetrics( SM_CXFRAME );
      pe->ptUpperLeft.y = r.top + GetSystemMetrics( SM_CYBORDER )
                            + GetSystemMetrics( SM_CYFRAME )
                            + GetSystemMetrics( SM_CYCAPTION );
   }
   SetClip( (RCOORD)pImage->width, 
            (RCOORD)pImage->height );
   pfs = &pe->pCurrent->objinfo.FacetPool;
   pls = &pe->pCurrent->objinfo.LinePool;
   for( f = 0; f < pfs->nUsedFacets; f++ )
   {
      pf = pfs->pFacets + f;
      plps = &pf->pLineSet;
      for( l = 0; l < plps->nUsedLines; l++ )
      {
         RAY ld; // local directional...
         pl = pls->pLines + plps->pLines[l].nLine;
         if( pl == pe->pCurrentLine )
         {
            ApplyInverseR( pe->TView, &ld, &pl->r );
            DrawLine( pImage, ld.o, ld.n, pl->dFrom-5, pl->dFrom, Color( 192, 192, 0 ) );
            DrawLine( pImage, ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 150, 0 ) );
            DrawLine( pImage, ld.o, ld.n, pl->dTo, pl->dTo+5, Color( 192, 192, 0 ) );
            GetViewPoint( pImage, &pe->ptO, ld.o );
            add( ld.o, ld.o, ld.n ); // add one slope to origin to get slope point
            GetViewPoint( pImage, &pe->ptN, ld.o );
            MarkOrigin( pe,  pImage );
            MarkSlope( pe, pImage );
         }
         else
         if( pf = pe->pCurrentFacet )
         {
            ApplyInverseR( pe->TView, &ld, &pl->r );
            DrawLine( pImage, ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 0, 150 ) );
         }
         else
         {
            ApplyInverseR( pe->TView, &ld, &pl->r );
            DrawLine( pImage, ld.o, ld.n, pl->dFrom, pl->dTo, Color( 92, 92, 92 ) );
         }
      }
   }
   UpdateDisplayPortion( pe->hVideo, 0, 0, 0, 0 );
}
   



PEDITOR CreateEditor( POBJECT po )
{
	PEDITOR pe;
	pe = (PEDITOR)Allocate( sizeof( EDITOR ) );
	MemSet( pe, 0, sizeof( EDITOR ) );
	pe->TView = CreateTransform();
   pe->hVideo = OpenDisplay( 0 );//"World Editor" );
   SetRedrawHandler( pe->hVideo, EditResizeCallback, (PTRSZVAL)pe );
   SetMouseHandler( pe->hVideo, EditMouseCallback, (PTRSZVAL)pe );
  // SetCloseHandler( pe->hVideo, EditCloseCallback, (PTRSZVAL)pe );

   // menu for this instance....
   pe->hMenu = CreatePopupMenu();
   AppendMenu( pe->hMenu, MF_STRING, MNU_ADDPREDEF+0, "Add Square" );
   AppendMenu( pe->hMenu, MF_STRING, MNU_ADDPREDEF+1, "Add Triangle" );
   AppendMenu( pe->hMenu, MF_STRING, MNU_ADDLINE, "Add Line" );

   SetPoint( vToScreen, VectorConst_Z ); // facing into screen...
   Invert( vToScreen );    // facing out of screen...

   pe->pCurrent = po;
   pe->pCurrentFacet = po->objinfo.FacetPool.pFacets + 0;
   pe->pCurrentLine = po->objinfo.LinePool.pLines + 0;

   ClearTransform( pe->TView );
   Translate( pe->TView, 0, 
                    0, 
                    (RCOORD)-100.0 );

   pe->bLocked = FALSE;

   EditResizeCallback( (PTRSZVAL)pe);
   return pe;
}

void DestroyEditor( PEDITOR pe )
{
   DestroyTransform( pe->TView );
   CloseDisplay( pe->hVideo );
   Release( pe );
}


