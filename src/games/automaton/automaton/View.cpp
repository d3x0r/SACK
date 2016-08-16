#include <malloc.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <timers.h>

extern "C" {
#include "vidlib.h"
#include "keybrd.h"
}

#include "object.hpp"
#include "View.Hpp"
#include "key.hpp"

#define WIREFRAME
#define DRAW_AXIS
// #define PRINT_LINES //(slow slow slow)

//-------------------------------------------------
// OPTIONS AFFECTING DISPLAY ARE IN VIEW.H
//-------------------------------------------------

typedef struct span_tag
{
   RCOORD minx, maxx;
   // texture projection information
   CDATA color;
} SPAN, *PSPAN;

PSPAN pSpans;

int MinSpan, MaxSpan; // top and bottom span used this time.

VIEW *pMainView;

// This module will display an object on the screen
// or maybe just a polygon
// because an object may be a BSP tree which describes
// a level, or it may be an object definition.
// In either case a higher resolution screen should merely
// add detail, not be able to project more of the scene.
// The camera could also have a 'periferal' view along the
// edges of the scene viewed.

//PALETTE p;  // Pallete defined in vidlib.h.......

int nHeight, nWidth;

void CALLBACK InitSpans( Image pImage )
{
   int y;
   if( !pImage )
      return;
   nHeight = pImage->height;
   nWidth = pImage->width;

   if( !pSpans )
   {
#ifdef LOG_ALLOC
      printf("malloc(SPANS)\n");
#endif
      pSpans = (PSPAN)malloc( sizeof( SPAN ) * pImage->height );
   }
   MinSpan = nHeight;
   MaxSpan = 0;
   for( y = 0; y < nHeight; y++ )
   {
      pSpans[y].minx = (RCOORD)nWidth;
      pSpans[y].maxx = 0;
      fprintf( stderr, "doing %d\n", y );
      pSpans[y].color = 0x7f7f7f;  // MaxSpan is suitable zero here.
   }

}

 static VECTOR mouse_vforward,  // complete tranlations...
          mouse_vright, 
          mouse_vup, 
          mouse_origin;
 static int mouse_buttons;
#define KEY_BUTTON1 0x10
#define KEY_BUTTON2 0x20
#define KEY_BUTTON3 0x40

      static VIEW *MouseIn;

   void VIEW::UpdateCursorPos( int x, int y )
   {
      VECTOR v;

      MouseIn = this;

      T.GetAxis( mouse_vforward, vForward );
      T.GetAxis( mouse_vright, vRight );
      T.GetAxis( mouse_vup, vUp );

      // v forward should be offsetable by mouse...
      scale( v, T.GetAxis( vForward ), 100.0 );
      add( mouse_origin, T.GetOrigin(), v ); // put it back in world...
   }

   void UpdateCursorPos( void )
   {
      VECTOR v;
      if( MouseIn )
      {
         MouseIn->T.GetAxis( mouse_vforward, vForward );
         MouseIn->T.GetAxis( mouse_vright, vRight );
         MouseIn->T.GetAxis( mouse_vup, vUp );

         // v forward should be offsetable by mouse...
         scale( v, MouseIn->T.GetAxis( vForward ), 100.0 );
         add( mouse_origin, MouseIn->T.GetOrigin(), v ); // put it back in world...
      }
   }


void VIEW::DoMouse( void )
{

   // only good for direct ahead manipulation....

   // no deviation from forward....

   MouseMethod( hVideo,
					 mouse_vforward, 
					 mouse_vright, 
                mouse_vup, 
                mouse_origin, 
                mouse_buttons);
}


int ViewMouse( uintptr_t dwView, int32_t x, int32_t y, uint32_t b )
{
   VIEW *v = (VIEW*)dwView;

   mouse_buttons = ( mouse_buttons & 0xF0 ) | b;

   // should pass x and y to update cursor pos...
   v->UpdateCursorPos( x, y ); // this view.... this mouse eventing

   v->DoMouse( );
	return 0;
}

bool bDump;

void _ShowObjects( uintptr_t dwView, PRENDERER self )
{
  VIEW *v = (VIEW*)dwView;
  v->ShowObjects( );   
}

void CPROC TimerProc( uintptr_t psv )
{
extern VECTOR KeySpeed, KeyRotation;
extern POBJECT pFirstObject;
   static TRANSFORM SaveT;
   static POBJECT pCurrent;
   VIEW *View = (VIEW*)psv;

   ScanKeyboard( View->hVideo );
   bDump = false;
   if( KeyDown( View->hVideo, KEY_SPACE ) )
   {
      mouse_buttons |= KEY_BUTTON1;
   }
      else
         mouse_buttons &= ~KEY_BUTTON1;
   if( KeyDown( View->hVideo, KEY_ESC ) )
   {
      mouse_buttons |= KEY_BUTTON2;
   }
      else
         mouse_buttons &= ~KEY_BUTTON2;
      if( KeyDown( View->hVideo, KEY_SPACE ) )
   {
      mouse_buttons |= KEY_BUTTON3;
   }
      else
         mouse_buttons &= ~KEY_BUTTON3;

    if( KeyDown( View->hVideo, KEY_D ) )
    {
       bDump = true;
    }
    
    if( KeyDown( View->hVideo, KEY_N ) )
    {
       if( !pCurrent )
       {
          SaveT = pMainView->T;
          pCurrent = pFirstObject;
       }
       else
          pCurrent = pCurrent->pNext;

       if( !pCurrent )
          pMainView->T = SaveT;
    }

    if( KeyDown( View->hVideo, KEY_P ) )
    {
      if( !pCurrent )
      {
         SaveT = pMainView->T;
         pCurrent = pFirstObject;
         while( pCurrent && pCurrent->pNext )
            pCurrent = pCurrent->pNext;
      }
      else
         pCurrent = pCurrent->pPrior;

      if( !pCurrent )
         pMainView->T = SaveT;
    }
    if( pCurrent )
       pMainView->T = pCurrent->T;
    else
    {
      pMainView->T.SetSpeed( KeySpeed );
      pMainView->T.SetRotation( KeyRotation );
      pMainView->T.Move();  // relative rotation...
      }

   View = pMainView; // start at main....

   while( View )
   {
      if( bDump )
      {
         char pHeader[64];
         sprintf( pHeader, "View Matrix [%d]", View->Type );
         View->T.show( pHeader );
      }
      
      ClearDisplay( View->hVideo );

      {
         VECTOR m, b;
			Image image = GetDisplayImage( View->hVideo );
         // rotate world into view coordinates... mouse is void(0) coordinates...

         UpdateCursorPos(); // no parameter version same x, y...
//         View->DoMouse();

         View->T.ApplyInverse( b, mouse_origin );
         View->T.ApplyInverseRotation( m, mouse_vforward );
         DrawLine( image, b, m, 0, 10, 0x7f );
         View->T.ApplyInverseRotation( m, mouse_vright );
         DrawLine( image, b, m, 0, 10, 0x7f00 );
         View->T.ApplyInverseRotation( m, mouse_vup );
         DrawLine( image, b, m, 0, 10, 0x7f0000 );
      }
      _ShowObjects( (uint32_t)View, View->hVideo );  // clears the board...
		UpdateDisplay( View->hVideo );


      if( View->Previous )
      {
         View = View->Previous;
         View->T = pMainView->T; // get current translation (rep movsd)
         switch( View->Type )
         {
         case V_FORWARD:
            MessageBox( NULL, "View Heirarchy is not correct", "Design Flaw", MB_OK );
            break;
         case V_RIGHT:
            View->T.RotateRight( vForward, vRight );
            break;
         case V_LEFT:
            View->T.RotateRight( vRight, vForward );
            break;
         case V_UP:
            View->T.RotateRight( vForward, vUp );
            break;
         case V_DOWN:   
            View->T.RotateRight( vUp, vForward );
            break;
         case V_BACK:
            View->T.RotateRight( -1, -1 );   
            break;
         }
      }
      else
         break;  // no more...
   }
}

VIEW::VIEW( ViewMouseCallback pMC, char *Title )
{
   T.clear();
   hVideo = OpenDisplaySizedAt( 0, 200, 200, 
                              CW_USEDEFAULT, CW_USEDEFAULT );	
	SetRedrawHandler( hVideo, _ShowObjects, (uintptr_t)this );
   MouseMethod = pMC;
	SetMouseHandler( hVideo, ViewMouse, (uintptr_t)this );
   
   InitSpans( GetDisplayImage( hVideo ) ); // set nHeight, nWidth

   Previous = pMainView;
   if( !pMainView )
   {
		AddTimer( 80, TimerProc, (uintptr_t)this );
   }
   pMainView = this;
}

void CloseView( uint32_t dwView )
{
   VIEW *V;
   V = (VIEW*)dwView;
   V->hVideo = NULL; // release from window side....
}

VIEW::VIEW( int nType, ViewMouseCallback pMC, char *Title, int sx, int sy )
{
   T.clear();
   Type = nType;
//   SetPoint( r, pr );

   hVideo = OpenDisplaySizedAt( 0, 200, 200, sx, sy );
	SetRedrawHandler( hVideo, _ShowObjects, (uintptr_t)this );
	SetCloseHandler( hVideo, CloseView, (uintptr_t)this );
	SetMouseHandler( hVideo, ViewMouse, (uintptr_t)this );
   MouseMethod = pMC;

   InitSpans( GetDisplayImage( hVideo ) ); // set nHeight, nWidth

   Previous = pMainView;
   if( !pMainView )
   {
      AddTimer(  200, TimerProc, (uintptr_t)this );
   }
   pMainView = this;
}

void AddSpan( int Y, int X, CDATA color )
{
   PSPAN ps;
   // please call this with buffer coordinates 0->width , 0->height

   if( X > nWidth )
      X = nWidth - 1;  // truncate to screen.
   if( X < 0 )
      X = 0;

   if( Y > MaxSpan )
      MaxSpan = Y;
   if( Y < MinSpan )
      MinSpan = Y;
   ps = pSpans + Y;
   if( X < ps->minx )
      ps->minx = (RCOORD)X;
   if( X > ps->maxx )
      ps->maxx = (RCOORD)X;
   ps->color = color;
}

// SetClip sets viewport x, y, 1 so that 1, 1, 1 is visible at outer corner
static RAY vc1 = { {0, 0, 1.0f}, { -1.0f, 0, 2.0f } },
           vc2 = { {0, 0, 1.0f}, { 1.0f, 0, 2.0f}}, 
           vc3 = { {0, 0, 1.0f}, { 0, -1.0f, 2.0f }},
           vc4 = { {0, 0, 1.0f}, { 0, 1.0f, 2.0f }}, 
           vc0 = { {0, 0, 1.0f}, { 0, 0, 1.0f }}; // allow microscopic closenesss....

void SetClip( RCOORD width, RCOORD height )
{
   vc1.o[0] = width;
   vc2.o[0] = -width;
   vc3.o[1] = height;
   vc4.o[1] = -height;
   
}

   // clip is a destructive function.... original points destroyed
bool Clip( PVECTOR p1, PVECTOR p2, PVECTOR n, PVECTOR o )
{
   VECTOR m;
   RCOORD d1, d2; 

   d1 = PointToPlaneT( n, o, p1 );
   d2 = PointToPlaneT( n, o, p2 );
   if( d1 >= 0 && d2 >= 0 )
      return true; // right ON the plane...
   if( d1 < 0 && d2 < 0 ) 
      return false;

   if( d1 > 0 && d2 < 0 )
   {
      // d2 = distance below the plane...
      // and d2 is 0 or negative...
      sub( m, p1, p2 ); // slope from 2 to 1...
      scale( m, m, -d2 ); // portion of line below plane...
      // would be 1/(distance1 + distance2) since d2 is negative +(-d2)
      scale( m, m, 1/(d1-d2) );
      add( p2, p2, m );
      return true;
   }
   if( d1 < 0 && d2 > 0 )
   {
      sub( m, p2, p1 ); // slope from 2 to 1...
      scale( m, m, -d1 );
      // would be 1/(distance1 + distance2) since d2 is negative +(-d2)
      scale( m, m, 1/(d2-d1) );
      add( p1, p1, m );
      return true;
   }
   return false;

}

void DrawLine( Image pImage, VECTOR p, VECTOR m, RCOORD t1, RCOORD t2, CDATA c )
{
   int px = 9999;
   
//   int x, y, tx;

//   int StartScreenY, EndScreenY;
//   int StartScreenX, EndScreenX;
#undef t
//   RCOORD dt, t;
//   CDATA *pScanline;
   
   VECTOR v1,v2;

#define ProjectX( v )  ((pImage->width/2)  + (int)( ( ((RCOORD)pImage->width)  * (v)[0] ) / ( (v)[2] * 2 ) ) )
#define ProjectY( v )  ((pImage->height/2) - (int)( ( ((RCOORD)pImage->height) * (v)[1] ) / ( (v)[2] * 2 ) ) )

#ifdef PRINT_LINES
   printf("Point, Slope = " );
   PrintVector( p );
   PrintVector( m );
#endif
   scale( v1, m, t1 );
   add  ( v1, v1, p );
   scale( v2, m, t2 );
   add  ( v2, v2, p );

#ifdef PRINT_LINES
   printf("end points = ");
   PrintVector(v1);
   PrintVector(v2);
#endif
   // non perspective correct display method - quick and DIRTY

   // clip is a destructive function.... original points destroyed
   if( Clip( v1, v2, vc0.n, vc0.o ) &&
       Clip( v1, v2, vc1.n, vc1.o ) &&
       Clip( v1, v2, vc2.n, vc2.o ) &&
       Clip( v1, v2, vc3.n, vc3.o ) &&
       Clip( v1, v2, vc4.n, vc4.o ) )
   {
      do_line( pImage, (int)ProjectX(v1), (int)ProjectY(v1),
                            (int)ProjectX(v2), (int)ProjectY(v2), c );
   }

   
#ifdef BLKAJSDLKJAG
   StartScreenY = ProjectY(v1);
   EndScreenY = ProjectY(v2);

   if( StartScreenY == EndScreenY )
   {
      StartScreenX = ProjectX(v1);
      EndScreenX = ProjectX(v2);

      if( StartScreenX > EndScreenX )
      {
         x = StartScreenX;
         StartScreenX = EndScreenX;
         EndScreenX = x;
      }

      if( StartScreenY >= -(nHeight/2) && StartScreenY < (nHeight/2) )
      {
#ifdef WIREFRAME
         pScanline = (CDATA*)(pImage->image + (-StartScreenY/*+(nHeight/2)*/)*nWidth 
                     /*+StartScreenX */ /*+(nWidth/2)*/);
//         asm( // set a row of color...
//              "rep\n"
//              "stosb\n"
//              : : "a"(c), "c"(EndScreenX - StartScreenX), "D"(pScanline) );
         y = StartScreenY;
         for( x = StartScreenX; x < EndScreenX; x++ )
            pScanline[x] = c;
//            hVid->pImage->image[ ( -y+(nHeight/2)) * nWidth +
//                      			    ( x + (nWidth/2) ) ] = c;
#else
         AddSpan( -StartScreenY + (nHeight/2),
                  StartScreenX + (nWidth/2),
                  c );
         AddSpan( -StartScreenY + (nHeight/2),
                  EndScreenX + (nWidth/2),
                  c );
#endif
      }
      return;

   }


   t = t1;
   if( StartScreenY > EndScreenY )
   {
      y = StartScreenY;
      StartScreenY = EndScreenY;
      EndScreenY = y;
      t = t2;
      t2 = t1;
      t1 = t;
   }
   dt = ( t2 - t1 ) / (EndScreenY - StartScreenY);
   if( EndScreenY > nHeight/2 ||
       StartScreenY < -(nHeight/2) )
   {
      return; // line is NOT on screen vertically.
   }
   for( y = StartScreenY; y <= EndScreenY; y++ )
   {
      VECTOR v;
      v[0] = p[0] + m[0] * t;
      v[2] = p[2] + m[2] * t;
      x = ProjectX(v);
      t += dt;
      if( y < (nHeight/2) && y >= -(nHeight/2) )
      {
#ifdef WIREFRAME
         if( x >= (nWidth/2) )
         {
//            printf(" x past screen...");
            x = nWidth / 2; // also terminate loop...
         }
         else if( x < -(nWidth/2) )
         {
//            printf(" x before screen" );
            x = -(nWidth/2); // also terminate loop...
         }
         pScanline = (CDATA*)(pImage->image + ( ( -y /*+ ( nHeight / 2 ) */) * nWidth ) /*+ (nWidth/2)*/);
         if( px == 9999 )
         {
            px = x;
         }
         if( px < x )
         {
//            asm( // set a row of color...
//                 "rep\n"
//                 "stosb\n"
//                 : :  "a"(c), "c"(x-px), "D"(pScanline + (px + 1)) );
    //        memset( pScanline + (px+1), c, x - px );
            for( tx = (px); tx <= x; tx++ )  // (px+1) alternative?
               pScanline[tx] = c;
//               pImage->image[ ( -y+(nHeight/2)) * nWidth +
//                             ( tx + (nWidth/2) ) ] = c;
         }
         else if( px > x )
         {
//            asm( // set a row of color...
//                 "rep\n"
//                 "stosb\n"
//                 : :  "a"(c), "c"(px-x), "D"(pScanline + x) );
    //        memset( pScanline + (x), c, px - x );
            for( tx = x; tx <= (px); tx++ )  // (px-1) alternative?
               pScanline[tx] = c;
//               pImage->image[ ( -y+(nHeight/2)) * nWidth +
//                             ( tx + (nWidth/2) ) ] = c;
         }
         else // px and x are the same...
         {
            pScanline[ x ] = c;
         }
         px = x;
#else
         AddSpan( -y + (nHeight/2), x + (nWidth/2), c );
#endif
      }
   }
#endif
}

void GetViewPoint( Image pImage, POINT *presult, PVECTOR vo )
{
   presult->x = ProjectX( vo );
   presult->y = ProjectY( vo );
}

void GetRealPoint( Image pImage, PVECTOR vresult, POINT *pt )
{
   if( !vresult[2] )
      vresult[2] = 1.0f;  // dumb - but protects result...
   // use vresult Z for unprojection...
   vresult[0] = (pt->x - (pImage->width/2)) * (vresult[2] * 2.0f) / ((RCOORD)pImage->width);
   vresult[1] = ((pImage->height/2) - pt->y ) * (vresult[2] * 2.0f) / ((RCOORD)pImage->height);
}

void ShowSpans( Image pImage )
{
   int l, x, y;
   for( y = MinSpan; y < MaxSpan; y++ )
   {
      l = (int)(pSpans[y].maxx - pSpans[y].minx);
      if( l > 0 )
      {
         x = (int)(pSpans[y].minx);
         while( l-- )
	 {
		plot( pImage, x, y, pSpans[y].color );
	 }
	 l = 0; 
      }
   }
   InitSpans( pImage );
}

void VIEW::ShowObject( POBJECT po )
{
//   PFACET pp;
//   PFACETSET pps;
   PLINESEG pl;
   int l;
	Image image = GetDisplayImage( hVideo );
   // for all objects that have been created.
   // draw the object on the screen.

   nHeight = image->height; // scale now...
   nWidth = image->width;

#ifdef WIREFRAME
#ifdef DRAW_AXIS
   {
      //Draw object Axis.
      VECTOR vOrigin, vo;
      VECTOR vAxis[3], va[3];
      POBJECT pi ; // pIn stuff....

      // same as translating (0,0,0)by this matrix..
      po->T.GetOrigin( vo );  
      po->T.GetAxis( va[vRight], vRight );
      po->T.GetAxis( va[vUp], vUp );
      po->T.GetAxis( va[vForward], vForward );
      pi = po->pIn;

      while( pi ) {
         pi->T.Apply( vOrigin, vo );
         SetPoint( vo, vOrigin );
         pi->T.ApplyRotation( vAxis[0], va[0] );
         SetPoint( va[0], vAxis[0] );
         pi->T.ApplyRotation( vAxis[1], va[1] );
         SetPoint( va[1], vAxis[1] );
         pi->T.ApplyRotation( vAxis[2], va[2] );
         SetPoint( va[2], vAxis[2] );
         pi = pi->pIn;
      };
      // finally rotate into view space.... which is at MASter level...
       T.ApplyInverse( vOrigin, vo );
       T.ApplyInverseRotation( vAxis[0], va[0] );
       T.ApplyInverseRotation( vAxis[1], va[1] );
       T.ApplyInverseRotation( vAxis[2], va[2] );

       DrawLine( image, vOrigin, vAxis[0], 0, 100, 0x7f );
       DrawLine( image, vOrigin, vAxis[1], 0, 100, 0x7f00 );
       DrawLine( image, vOrigin, vAxis[2], 0, 100, 0x7f0000 );
   }
#endif
#endif

#ifdef WIREFRAME
   {
      PLINESEGSET pls;
      pls = po->pLinePool; // for all lines ONLY... not planes too..
      for( l = 0; l < pls->nUsedLines; l++ )
      {
         RAY rl[2], rvl;
         int t;
         POBJECT pi; // pIn Tree...
         pl = pls->pLines + l;
         if( !pl->bUsed )
            continue;

         t = 0;
         po->T.Apply( &rl[t], &pl->d );
         pi = po->pIn;
         while( pi ) {
            t = 1-t;
            pi->T.Apply( &rl[t], &rl[1-t]  );
            pi = pi->pIn;
         };
         T.ApplyInverse( &rvl, &rl[t] );

//               if( pl->bDraw || 1)
         {
            DrawLine( image, 
                      rvl.o,
                      rvl.n,
                      pl->dFrom,
                      pl->dTo,
                      Color( 255, 255, 255 ) );
         }
      }
   }
#endif
#ifndef WIREFRAME
   pps = po->pPlaneSet;
   for( p = 0; p < pps->nUsedPlanes; p++ )
   {
      pp = pps->pPlanes + p;
      // compute plane visibility after translation...

      if( pp->bDraw )
      {
         Apply( po->pr, pp->o, pp->io );
         ApplyRotation( po->pr, pp->n, pp->in );
        // visiblity is like the view origin to the plane's origin (of course visible)
        // and then the angle of that and the normal of the plane...

        // visibility could also be determined by geometry -
        // example - bsp trees produce visible polygons from a
        // specific sector.
        // also - descent levels produce visible planes by bounding
        // the world...
        // free space polygon transformations need some degree of
        // intelligence on computing visible faces. could of course
        // follow lines which bind 2 planes... any boundry which is not
        // visible does not have to be processed... if the
        // first plane to check is not visible, then all plane boundings
        // need to be checked for visibility until one is found.
        // if the object is not at all visible - the extent box of the
        // object is beyond the clipping frustrum, its planes do not
        // need to be processed.
//        if( 0 /*cheese factor 5*/ )
//         {
//            double percentz;
            // my view's origin... tv = pp->o - v->o
            // my view's slope = pp->s projected on tv
            // if < 0.10 don't draw... 0.10 is an epsilon of error though...
/*
            percentz = pp->n[2] / sqrt( ( pp->n[0] * pp->n[0] ) +
                                        ( pp->n[1] * pp->n[1] ) +
                                        ( pp->n[2] * pp->n[2] ) );
            if( percentz > -0.05 )  // more than 5% FACING me  ( -Z faces me )
*/
//               { // for loop here...}
//		 }
         }
      }
         ShowSpans();  // per polygon - show it.
#endif
}

void VIEW::ShowObjectChildren( POBJECT po )
{
   POBJECT pc;
   if( !po )
      return;
   FORALL( po, &pc )
   {
      ShowObject( pc );
      if( pc->pHolds )
         ShowObjectChildren( pc->pHolds );
      if( pc->pHas )
         ShowObjectChildren( pc->pHas );
   }
}

void VIEW::ShowObjects( void )
{
   POBJECT po;
   po = pFirstObject; // some object..........
   while( po && ( po->pIn || po->pOn ) ) // go to TOP of tree...
   {
      if( po->pIn )
         po = po->pIn;
      else if( po->pOn )
         po = po->pOn;
   }
   SetClip( (RCOORD)GetDisplayImage( hVideo )->width, 
           (RCOORD)GetDisplayImage( hVideo )->height );
   ShowObjectChildren( po );  // all children...
	UpdateDisplay( hVideo );
}


void VIEW::Update( void )
{
   if( hVideo )
   {
		UpdateDisplay( hVideo );
	   ClearDisplay( hVideo ) ;
   }
}

VIEW::~VIEW( void )
{
   if( hVideo )  // could already be closed...
	{
      CloseDisplay( hVideo );
		hVideo = NULL;
	}
}
