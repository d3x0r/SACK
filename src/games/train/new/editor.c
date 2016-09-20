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
   //HMENU  hMenu;  // first one should have 'add line'
   POBJECT pCurrent;
   PFACET pCurrentFacet;
   PLINESEG  pCurrentLine;
   IMAGE_POINT ptO, ptN;
   struct {
      int bLocked:1;  // mouse is locked to ptMouse...
      int bDragOrigin:1;
      int bDragNormal:1;
      int bDragWorld:1;
   };
   IMAGE_POINT ptMouse; // current point of mouse....
   IMAGE_POINT ptMouseDel; // accumulator of mouse delta (if locked)
   IMAGE_POINT ptUpperLeft; // upper left screen coordinate of window...
} EDITOR, *PEDITOR;

void CPROC EditResizeCallback( uintptr_t dwUser );


LOGICAL CPROC PageUpKey( uintptr_t psv, uint32_t keycode )
{
   PEDITOR pe = (PEDITOR)psv;
	TranslateRel( pe->TView, 0, 0, -1.0 );
	EditResizeCallback( (uintptr_t)pe );
	return TRUE;
}


LOGICAL CPROC PageDownKey( uintptr_t psv, uint32_t keycode )
{
	PEDITOR pe = (PEDITOR)psv;
	TranslateRel( pe->TView, 0, 0, 1.0 );
	EditResizeCallback( (uintptr_t)pe );
	return TRUE;
}

int CPROC EditMouseCallback( uintptr_t dwUser, int32_t x, int32_t y, uint32_t b )
{
   static int _x, _y, _right, _left;
   int right, left;
   PEDITOR pe = (PEDITOR)dwUser;
   right = b & MK_RBUTTON;
   left = b & MK_LBUTTON;

   //if( pe->bLocked )
   //{/
   //}
   pe->ptMouseDel[0] = x - pe->ptMouse[0];
   pe->ptMouseDel[1] = y - pe->ptMouse[1];
   pe->ptMouse[0] = x;
   pe->ptMouse[1] = y;

   if( !(left + right + _left + _right) ) // just moving mouse..
   {
      if( pe->bLocked )
      {
         if( pe->ptMouseDel[0] > MOUSE_LOCK || pe->ptMouseDel[0] < -MOUSE_LOCK ||
             pe->ptMouseDel[1] > MOUSE_LOCK || pe->ptMouseDel[1] < -MOUSE_LOCK )
         {
            pe->bLocked = FALSE;
            SetMousePosition( pe->hVideo, pe->ptMouse[0], pe->ptMouse[1] );
            return; // done....
         }
      }
      else
      {
         if( ( abs( pe->ptO[0] - x ) < MOUSE_LOCK ) &&
             ( abs( pe->ptO[1] - y ) < MOUSE_LOCK ) )
         {
            pe->bLocked = TRUE;
            pe->ptMouseDel[0] = 0;
            pe->ptMouseDel[1] = 0;
            pe->ptMouse[0] = pe->ptO[0];
            pe->ptMouse[1] = pe->ptO[1];
         }
         else
         if( ( abs( pe->ptN[0] - x) < MOUSE_LOCK ) &&
             ( abs( pe->ptN[1] - y ) < MOUSE_LOCK ) )
         {
            pe->bLocked = TRUE;
            pe->ptMouseDel[0] = 0;
            pe->ptMouseDel[1] = 0;
            pe->ptMouse[0] = pe->ptN[0];
            pe->ptMouse[1] = pe->ptN[1];
         }
      }

      if( pe->bLocked ) // only reset mouse if not dragging??
      {
	      pe->ptMouse[0] -= pe->ptMouseDel[0];
   	   pe->ptMouse[1] -= pe->ptMouseDel[1];
         SetMousePosition( pe->hVideo, pe->ptMouse[0],
                                  pe->ptMouse[1] );
      }

   }

   if( pe->bLocked )
   {
      x = pe->ptMouse[0] - pe->ptMouseDel[0];
      y = pe->ptMouse[1] - pe->ptMouseDel[0];
   }

   if( !right && _right )
   {
      IMAGE_POINT p;
		int nCmd;
#ifdef __WINDOWS__
      GetCursorPos( (POINT*)&p );
      nCmd = TrackPopupMenu( pe->hMenu, 
                        TPM_CENTERALIGN 
								| TPM_TOPALIGN 
								| TPM_RIGHTBUTTON 
								| TPM_RETURNCMD 
								| TPM_NONOTIFY
								,p[0]
								,p[1]
                        ,0
								,NULL // pe->hVideo->hWndOutput
								,NULL);
      switch( nCmd )
      {
      case MNU_ADDLINE:
         break;
		}
#endif
   }

   if( left && !_left ) // left click select...
   {
      if( x == pe->ptO[0] && y == pe->ptO[1] )
      {
         pe->bDragOrigin = TRUE;
      } 
      else if( x == pe->ptN[0] && y == pe->ptN[1] )
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
         if( pe->ptMouseDel[0] || pe->ptMouseDel[1] )
         {
            GetOriginV( pe->TView, vo );
            Invert( vo );
            GetRealPoint( GetDisplayImage( pe->hVideo ), vo, &pe->ptMouse );
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
            EditResizeCallback( (uintptr_t)pe ); // update screen...
         }
      } 
      else if( pe->bDragNormal )
      {
         VECTOR vn;
         if( pe->ptMouseDel[0] || pe->ptMouseDel[1] )
         {
            GetOriginV( pe->TView, vn );
            Invert( vn );
            GetRealPoint( GetDisplayImage( pe->hVideo ), vn, &pe->ptMouse );
            sub( vn, vn, pe->pCurrentLine->r.o );
            Apply( pe->TView, pe->pCurrentLine->r.n, vn ); // apply inverse -> Apply...
            // update Intersecting lines......
            EditResizeCallback( (uintptr_t)pe ); // update screen...
         }
      }
      else if( pe->bDragWorld )
      {
         VECTOR v1, v2;
         if( pe->ptMouseDel[0] || pe->ptMouseDel[1] )
         {
            GetOriginV( pe->TView, v1 );
            Invert( v1 );
            v2[vForward] = v1[vForward];
				pe->ptMouseDel[0] = pe->ptMouse[0] - pe->ptMouseDel[0];
				pe->ptMouseDel[1] = pe->ptMouse[1] - pe->ptMouseDel[1];

            GetRealPoint( GetDisplayImage( pe->hVideo ), v1, &pe->ptMouse );
            GetRealPoint( GetDisplayImage( pe->hVideo ), v2, &pe->ptMouseDel );
            sub( v1, v2, v1 );
            v1[vForward] = 0; 
            TranslateRelV( pe->TView, v1 );
            EditResizeCallback( (uintptr_t)pe ); // update screen...
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
   do_line( pImage, pe->ptO[0] - MARK_SIZE,
                    pe->ptO[1] - MARK_SIZE,
                    pe->ptO[0] + MARK_SIZE, 
                    pe->ptO[1] - MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptO[0] + MARK_SIZE,
                    pe->ptO[1] - MARK_SIZE,
                    pe->ptO[0] + MARK_SIZE, 
                    pe->ptO[1] + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptO[0] + MARK_SIZE,
                    pe->ptO[1] + MARK_SIZE,
                    pe->ptO[0] - MARK_SIZE, 
                    pe->ptO[1] + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptO[0] - MARK_SIZE,
                    pe->ptO[1] + MARK_SIZE,
                    pe->ptO[0] - MARK_SIZE, 
                    pe->ptO[1] - MARK_SIZE, Color( 200, 200, 200 ) );
}


void MarkSlope( PEDITOR pe, Image pImage )
{
   do_line( pImage, pe->ptN[0],
                    pe->ptN[1] - MARK_SIZE,
                    pe->ptN[0] + MARK_SIZE, 
                    pe->ptN[1] + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptN[0] + MARK_SIZE,
                    pe->ptN[1] + MARK_SIZE,
                    pe->ptN[0] - MARK_SIZE, 
                    pe->ptN[1] + MARK_SIZE, Color( 200, 200, 200 ) );
   do_line( pImage, pe->ptN[0] - MARK_SIZE,
                    pe->ptN[1] + MARK_SIZE,
                    pe->ptN[0], 
                    pe->ptN[1] - MARK_SIZE, Color( 200, 200, 200 ) );
}

// SetClip sets viewport x, y, 1 so that 1, 1, 1 is visible at outer corner
static RAY vc1 = { {0, 0, 1.0f}, { -1.0f, 0, 2.0f } },
           vc2 = { {0, 0, 1.0f}, { 1.0f, 0, 2.0f}}, 
           vc3 = { {0, 0, 1.0f}, { 0, -1.0f, 2.0f }},
           vc4 = { {0, 0, 1.0f}, { 0, 1.0f, 2.0f }}, 
           vc0 = { {0, 0, 1.0f}, { 0, 0, 1.0f }}; // allow microscopic closenesss....

#ifdef __WINDOWS__

static void SetClip( RCOORD width, RCOORD height )
{
   vc1.o[0] = width;
   vc2.o[0] = -width;
   vc3.o[1] = height;
   vc4.o[1] = -height;
}

#define ProjectX( v )  ((pImage->width/2)  + (int)( ( ((RCOORD)pImage->width)  * (v)[0] ) / ( (v)[2] * 2 ) ) )
#define ProjectY( v )  ((pImage->height/2) - (int)( ( ((RCOORD)pImage->height) * (v)[1] ) / ( (v)[2] * 2 ) ) )
static void GetViewPoint( Image pImage, IMAGE_POINT presult, PVECTOR vo )
{
   presult[0] = ProjectX( vo );
   presult[1] = ProjectY( vo );
}

static void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt )
{
   if( !vresult[2] )
      vresult[2] = 1.0f;  // dumb - but protects result...
   // use vresult Z for unprojection...
   vresult[0] = (pt[0] - (pImage->width/2)) * (vresult[2] * 2.0f) / ((RCOORD)pImage->width);
   vresult[1] = ((pImage->height/2) - pt[1] ) * (vresult[2] * 2.0f) / ((RCOORD)pImage->height);
}


static void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c )
{
   VECTOR v1,v2;
   add( v1, scale( v1, m, t1 ), p );
	add( v2, scale( v2, m, t2 ), p );
   /*
	glBegin( GL_LINES );
	glColor4ubv( (unsigned char *)&c );
	glVertex3dv( v1 );
	glVertex3dv( v2 );
	glEnd();
	glFlush();
   */
   //printf("end points = ");
   //PrintVector(v1);
   //PrintVector(v2);
   /*if( Clip( v1, v2, vc0.n, vc0.o ) &&
       Clip( v1, v2, vc1.n, vc1.o ) &&
       Clip( v1, v2, vc2.n, vc2.o ) &&
       Clip( v1, v2, vc3.n, vc3.o ) &&
		 Clip( v1, v2, vc4.n, vc4.o ) )
       */
	{

		do_line( pImage, ProjectX( v1 ), ProjectY( v1 ),
				  ProjectX( v2 ), ProjectY( v2 ), c );
	}
}
#endif


void CPROC EditResizeCallback( uintptr_t dwUser  )
{
   PEDITOR pe = (PEDITOR)dwUser;
   PFACETSET *ppfs;
   PFACET pf;
   int f;

   PLINESEGPSET *pplps;
   PMYLINESEGSET *ppls;
   PMYLINESEG pl;
   int l;
   // ShowCurrent();
   ClearImage( pImage );
   {
		//RECT r;
		GetDisplayPosition( pe->hVideo, pe->ptUpperLeft, pe->ptUpperLeft + 1, NULL, NULL );
      /*
      GetWindowRect( pe->hVideo->hWndOutput, &r );
      pe->ptUpperLeft[0] = r.left //+ GetSystemMetrics( SM_CXBORDER )
                             + GetSystemMetrics( SM_CXFRAME );
      pe->ptUpperLeft[1] = r.top + GetSystemMetrics( SM_CYBORDER )
                            + GetSystemMetrics( SM_CYFRAME )
									 + GetSystemMetrics( SM_CYCAPTION );
                            */
	}
#if 0
   SetClip( (RCOORD)GetDisplayImage( pe->hVideo )->width,
            (RCOORD)GetDisplayImage( pe->hVideo )->height );
   ppfs = pe->pCurrent->objinfo->ppFacetPool;
	ppls = GetFromSet( LINEPSEG, pe->pCurrent->objinfo->ppLinePool );
   //ForAllInSet( FACET, &pe->pCurrent->objinfo->ppFacetPool, DrawLineSegs );
   //for( f = 0; f < ; f++ )
	{
      int lines;
      //pf = GetFromSet( FACET, pfs->pFacets + f;
		pplps = &pf->pLineSet;
      lines = CountUsedInSet( LINESEGP, pf->pLineSet );
      for( l = 0; l < lines; l++ )
      {
			RAY ld; // local directional...
         PLINESEGP plsp = GetSetMember( LINESEGP, pplps, l );
         pl = GetSetMember( MYLINESEG, ppls, plsp->nLine );
         if( pl == pe->pCurrentLine )
         {
            ApplyInverseR( pe->TView, &ld, &pl->r );
            DrawLine( GetDisplayImage( pe->hVideo ), ld.o, ld.n, pl->dFrom-5, pl->dFrom, Color( 192, 192, 0 ) );
            DrawLine( GetDisplayImage( pe->hVideo ), ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 150, 0 ) );
            DrawLine( GetDisplayImage( pe->hVideo ), ld.o, ld.n, pl->dTo, pl->dTo+5, Color( 192, 192, 0 ) );
            GetViewPoint( GetDisplayImage( pe->hVideo ), &pe->ptO, ld.o );
            add( ld.o, ld.o, ld.n ); // add one slope to origin to get slope point
            GetViewPoint( GetDisplayImage( pe->hVideo ), &pe->ptN, ld.o );
            MarkOrigin( pe,  GetDisplayImage( pe->hVideo ) );
            MarkSlope( pe, GetDisplayImage( pe->hVideo ) );
         }
         else
         if( pf = pe->pCurrentFacet )
         {
            ApplyInverseR( pe->TView, &ld, &pl->r );
            DrawLine( GetDisplayImage( pe->hVideo ), ld.o, ld.n, pl->dFrom, pl->dTo, Color( 0, 0, 150 ) );
         }
         else
         {
            ApplyInverseR( pe->TView, &ld, &pl->r );
            DrawLine( GetDisplayImage( pe->hVideo ), ld.o, ld.n, pl->dFrom, pl->dTo, Color( 92, 92, 92 ) );
         }
      }
	}
#endif
   UpdateDisplay( pe->hVideo );
}
   



PEDITOR CreateEditor( POBJECT po )
{
	PEDITOR pe;
	pe = (PEDITOR)Allocate( sizeof( EDITOR ) );
	MemSet( pe, 0, sizeof( EDITOR ) );
	pe->TView = CreateTransform();
	pe->hVideo = OpenDisplaySizedAt( 0, -1, -1, -1, -1 );
	//InitVideo( "World Editor" );
   SetRedrawHandler( pe->hVideo, (void (CPROC*)(uintptr_t,PRENDERER))EditResizeCallback, (uintptr_t)pe );
	SetMouseHandler( pe->hVideo, EditMouseCallback, (uintptr_t)pe );
   BindEventToKey( pe->hVideo, KEY_PGUP, 0, PageUpKey, (uintptr_t)pe );
   BindEventToKey( pe->hVideo, KEY_PGDN, 0, PageDownKey, (uintptr_t)pe );
  // SetCloseHandler( pe->hVideo, EditCloseCallback, (uintptr_t)pe );

				  // menu for this instance....
#ifdef __WINDOWS__
   pe->hMenu = CreatePopupMenu();
   AppendMenu( pe->hMenu, MF_STRING, MNU_ADDPREDEF+0, "Add Square" );
   AppendMenu( pe->hMenu, MF_STRING, MNU_ADDPREDEF+1, "Add Triangle" );
   AppendMenu( pe->hMenu, MF_STRING, MNU_ADDLINE, "Add Line" );
#endif
   SetPoint( vToScreen, VectorConst_Z ); // facing into screen...
   Invert( vToScreen );    // facing out of screen...

   pe->pCurrent = po;
	pe->pCurrentFacet = NULL; //po->objinfo.FacetPool.pFacets + 0;
   pe->pCurrentLine = NULL; //po->objinfo.LinePool.pLines + 0;

   ClearTransform( pe->TView );
   Translate( pe->TView, 0, 
                    0, 
                    (RCOORD)-100.0 );

   pe->bLocked = FALSE;

	EditResizeCallback( (uintptr_t)pe);
   UpdateDisplay( pe->hVideo );
   return pe;
}

void DestroyEditor( PEDITOR pe )
{
   DestroyTransform( pe->TView );
   CloseDisplay( pe->hVideo );
   Release( pe );
}


