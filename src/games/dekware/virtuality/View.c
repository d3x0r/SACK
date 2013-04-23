#include <sharemem.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "vidlib.h"

#include "object.h"
#include "View.H"
#include "key.h"

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

void CALLBACK InitSpans( ImageFile *pImage )
{
   int y;
   if( !pImage )
      return;
   nHeight = pImage->height;
   nWidth = pImage->width;

   if( !pSpans )
   {
      pSpans = (PSPAN)Allocate( sizeof( SPAN ) * pImage->height );
   }
   MinSpan = nHeight;
   MaxSpan = 0;
   for( y = 0; y < nHeight; y++ )
   {
      pSpans[y].minx = (RCOORD)nWidth;
      pSpans[y].maxx = 0;
      //fprintf( stderr, "doing %d\n", y );
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

   void UpdateCursorPos( PVIEW pv, int x, int y )
   {
      VECTOR v;

      MouseIn = pv;

      GetAxisV( pv->T, mouse_vforward, vForward );
      GetAxisV( pv->T, mouse_vright, vRight );
      GetAxisV( pv->T, mouse_vup, vUp );

      // v forward should be offsetable by mouse...
      scale( v, GetAxis( pv->T, vForward ), 100.0 );
      add( mouse_origin, GetOrigin(pv->T), v ); // put it back in world...
   }

   void UpdateThisCursorPos( void )
   {
      VECTOR v;
      if( MouseIn )
      {
         GetAxisV( MouseIn->T, mouse_vforward, vForward );
         GetAxisV( MouseIn->T, mouse_vright, vRight );
         GetAxisV( MouseIn->T, mouse_vup, vUp );

         // v forward should be offsetable by mouse...
         scale( v, GetAxis( MouseIn->T, vForward ), 100.0 );
         add( mouse_origin, GetOrigin(MouseIn->T), v ); // put it back in world...
      }
   }


void DoMouse( PVIEW pv )
{

   // only good for direct ahead manipulation....

   // no deviation from forward....
   if( pv->MouseMethod )
      pv->MouseMethod( (HWND)GetNativeHandle(pv->hVideo), 
                           mouse_vforward, 
                           mouse_vright, 
                           mouse_vup, 
                           mouse_origin, 
                           mouse_buttons);
}


int CPROC ViewMouse( PTRSZVAL dwView, S_32 x, S_32 y, _32 b )
{
   VIEW *v = (VIEW*)dwView;
	
   mouse_buttons = ( mouse_buttons & 0xF0 ) | b;

   // should pass x and y to update cursor pos...
   UpdateCursorPos( v, x, y ); // this view.... this mouse eventing

		DoMouse( v );
return 1;
}

int bDump;

void CPROC _ShowObjects( PTRSZVAL dwView, PRENDERER hVideo )
{

  VIEW *v = (VIEW*)dwView;
  VIEW *View = (VIEW*)dwView;

      
      ClearDisplay( View->hVideo );

      {
         VECTOR m, b;

         // rotate world into view coordinates... mouse is void(0) coordinates...

         UpdateThisCursorPos(); // no parameter version same x, y...
//         View->DoMouse();

         ApplyInverse( View->T, b, mouse_origin );
         ApplyInverseRotation( View->T, m, mouse_vforward );
         DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f );
         ApplyInverseRotation( View->T, m, mouse_vright );
         DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f00 );
         ApplyInverseRotation( View->T, m, mouse_vup );
         DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f0000 );
      }
      ShowObjects( View );  // clears the board...

			UpdateDisplay( View->hVideo );


//ShowObjects( v );   
}

void CALLBACK TimerProc( struct HWND__ *hWnd,unsigned int _i1,unsigned int _i2,unsigned long _i3)
{
static VECTOR KeySpeed, KeyRotation;
extern POBJECT pFirstObject;
//   static PTRANSFORM SaveT;
   static POBJECT pCurrent;
   VIEW *View;

   if( pMainView )
   {
      ScanKeyboard( pMainView->hVideo, KeySpeed, KeyRotation );
   }

   bDump = FALSE;
   /*
   if( KeyDown( KEY_SPACE ) )
   {
      mouse_buttons |= KEY_BUTTON1;
   }
      else
         mouse_buttons &= ~KEY_BUTTON1;
   if( KeyDown( KEY_ESC ) )
   {
      mouse_buttons |= KEY_BUTTON2;
   }
      else
         mouse_buttons &= ~KEY_BUTTON2;
      if( KeyDown( KEY_SPACE ) )
   {
      mouse_buttons |= KEY_BUTTON3;
   }
      else
         mouse_buttons &= ~KEY_BUTTON3;

    if( KeyDown( KEY_D ) )
    {
       bDump = true;
    }
    
    if( KeyDown( KEY_N ) )
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

    if( KeyDown( KEY_P ) )
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
    */

    if( pCurrent )
       pMainView->T = pCurrent->T;
    else
    {
      SetSpeed( pMainView->T, KeySpeed );
      SetRotation( pMainView->T, KeyRotation );
      Move(pMainView->T);  // relative rotation...
    }
   
   View = pMainView; // start at main....

   while( View )
   {
      if( bDump )
      {
         char pHeader[64];
         sprintf( pHeader, "View Matrix [%d]", View->Type );
         //showstdEx( View->T, pHeader );
		}

      Redraw( View->hVideo );
/*
      
      ClearDisplay( View->hVideo );

      {
         VECTOR m, b;

         // rotate world into view coordinates... mouse is void(0) coordinates...

         UpdateThisCursorPos(); // no parameter version same x, y...
//         View->DoMouse();

         ApplyInverse( View->T, b, mouse_origin );
         ApplyInverseRotation( View->T, m, mouse_vforward );
         DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f );
         ApplyInverseRotation( View->T, m, mouse_vright );
         DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f00 );
         ApplyInverseRotation( View->T, m, mouse_vup );
         DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f0000 );
      }
      _ShowObjects( (PTRSZVAL)View, View->hVideo );  // clears the board...

			UpdateDisplayPortion( View->hVideo, 0, 0, 0, 0 );
			
         */

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
            RotateRight( View->T, vForward, vRight );
            break;
         case V_LEFT:
            RotateRight( View->T, vRight, vForward );
            break;
         case V_UP:
            RotateRight( View->T, vForward, vUp );
            break;
         case V_DOWN:   
            RotateRight( View->T, vUp, vForward );
            break;
         case V_BACK:
            RotateRight( View->T, -1, -1 );   
            break;
         }
      }
      else
         break;  // no more...
   }
}


void CPROC CloseView( PTRSZVAL dwView )
{
   VIEW *V;
   V = (VIEW*)dwView;
   V->hVideo = 0; // release from window side....
}


PVIEW CreateViewEx( int nType, ViewMouseCallback pMC, char *Title, int sx, int sy )
{
   PVIEW pv;
   pv = New( VIEW );
   memset( pv, 0, sizeof( VIEW ) );
   pv->T = CreateTransform();

   pv->Type = nType;
//   SetPoint( r, pr );

   pv->hVideo = OpenDisplaySizedAt( /*Title,*/ 0&DISPLAY_ATTRIBUTE_LAYERED, sx, sy, 200, 200 );
	SetRedrawHandler( pv->hVideo, _ShowObjects, (PTRSZVAL)pv );
	SetCloseHandler( pv->hVideo, CloseView, (PTRSZVAL)pv );
   SetMouseHandler( pv->hVideo, ViewMouse, (PTRSZVAL)pv );
	
   RestoreDisplay( pv->hVideo );
   pv->MouseMethod = pMC;

   InitSpans( GetDisplayImage( pv->hVideo ) ); // set nHeight, nWidth

   pv->Previous = pMainView;
   if( !pMainView )
	{
      lprintf( "Scheduled the timer... " );
      SetTimer(  NULL, 100, 200, TimerProc );
   }
   pMainView = pv;

   return pv;
}

PVIEW CreateView( ViewMouseCallback pMC, char *Title )
{           	
	return CreateViewEx( V_FORWARD, pMC, Title, 0, 0 );
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
int Clip( PVECTOR p1, PVECTOR p2, PVECTOR n, PVECTOR o )
{
   VECTOR m;
   RCOORD d1, d2; 

   d1 = PointToPlaneT( n, o, p1 );
   d2 = PointToPlaneT( n, o, p2 );
   if( d1 >= 0 && d2 >= 0 )
      return TRUE; // right ON the plane...
   if( d1 < 0 && d2 < 0 ) 
      return FALSE;

   if( d1 > 0 && d2 < 0 )
   {
      // d2 = distance below the plane...
      // and d2 is 0 or negative...
      sub( m, p1, p2 ); // slope from 2 to 1...
      scale( m, m, -d2 ); // portion of line below plane...
      // would be 1/(distance1 + distance2) since d2 is negative +(-d2)
      scale( m, m, 1/(d1-d2) );
      add( p2, p2, m );
      return TRUE;
   }
   if( d1 < 0 && d2 > 0 )
   {
      sub( m, p2, p1 ); // slope from 2 to 1...
      scale( m, m, -d1 );
      // would be 1/(distance1 + distance2) since d2 is negative +(-d2)
      scale( m, m, 1/(d2-d1) );
      add( p1, p1, m );
      return TRUE;
   }
   return FALSE;

}

void DrawLine( ImageFile *pImage, VECTOR p, VECTOR m, RCOORD t1, RCOORD t2, CDATA c )
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
//                                   ( x + (nWidth/2) ) ] = c;
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

void GetViewPoint( ImageFile *pImage, POINT *presult, PVECTOR vo )
{
   presult->x = ProjectX( vo );
   presult->y = ProjectY( vo );
}

void GetRealPoint( ImageFile *pImage, PVECTOR vresult, POINT *pt )
{
   if( !vresult[2] )
      vresult[2] = 1.0f;  // dumb - but protects result...
   // use vresult Z for unprojection...
   vresult[0] = (pt->x - (pImage->width/2)) * (vresult[2] * 2.0f) / ((RCOORD)pImage->width);
   vresult[1] = ((pImage->height/2) - pt->y ) * (vresult[2] * 2.0f) / ((RCOORD)pImage->height);
}

void ShowSpans( ImageFile *pImage )
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
    
      *(CDATA*)(pImage->image + ( y * nWidth ) + x) = pSpans[y].color;
    }
    l = 0; 
      }
   }
   InitSpans( pImage );
}

void ShowObject( PVIEW pv, POBJECT po )
{
//   PFACET pp;
//   PFACETSET pps;
   PLINESEG pl;
   int l;
   // for all objects that have been created.
   // draw the object on the screen.

   nHeight = GetDisplayImage( pv->hVideo )->height; // scale now...
   nWidth = GetDisplayImage( pv->hVideo )->width;

#ifdef WIREFRAME
#ifdef DRAW_AXIS
   {
      //Draw object Axis.
      VECTOR vOrigin, vo;
      VECTOR vAxis[3], va[3];
      POBJECT pi ; // pIn stuff....

      // same as translating (0,0,0)by this matrix..
      GetOriginV( po->T, vo );  
      GetAxisV( po->T, va[vRight], vRight );
      GetAxisV( po->T, va[vUp], vUp );
      GetAxisV( po->T, va[vForward], vForward );
      pi = po->pIn;

      while( pi ) {
         Apply( pi->T, vOrigin, vo );
         SetPoint( vo, vOrigin );
         ApplyRotation( pi->T, vAxis[0], va[0] );
         SetPoint( va[0], vAxis[0] );
         ApplyRotation( pi->T, vAxis[1], va[1] );
         SetPoint( va[1], vAxis[1] );
         ApplyRotation( pi->T, vAxis[2], va[2] );
         SetPoint( va[2], vAxis[2] );
         pi = pi->pIn;
      };
      // finally rotate into view space.... which is at MASter level...
       ApplyInverse( pv->T, vOrigin, vo );
       ApplyInverseRotation( pv->T, vAxis[0], va[0] );
       ApplyInverseRotation( pv->T, vAxis[1], va[1] );
       ApplyInverseRotation( pv->T, vAxis[2], va[2] );

       DrawLine( GetDisplayImage( pv->hVideo ), vOrigin, vAxis[0], 0, 100, 0x7f );
       DrawLine( GetDisplayImage( pv->hVideo ), vOrigin, vAxis[1], 0, 100, 0x7f00 );
       DrawLine( GetDisplayImage( pv->hVideo ), vOrigin, vAxis[2], 0, 100, 0x7f0000 );
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
         ApplyR( po->T, &rl[t], &pl->d );
         pi = po->pIn;
         while( pi ) {
            t = 1-t;
            ApplyR( pi->T, &rl[t], &rl[1-t]  );
            pi = pi->pIn;
         };
         ApplyInverseR( pv->T, &rvl, &rl[t] );
//               if( pl->bDraw || 1)
         {
            DrawLine( GetDisplayImage( pv->hVideo ), 
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
//     }
         }
      }
         ShowSpans();  // per polygon - show it.
#endif
}

void ShowObjectChildren( PVIEW pv, POBJECT po )
{
   POBJECT pc;
   if( !po )
      return;
   FORALLOBJ( po, pc )
   {
      ShowObject( pv, pc );
      if( pc->pHolds )
         ShowObjectChildren( pv, pc->pHolds );
      if( pc->pHas )
         ShowObjectChildren( pv, pc->pHas );
   }
}

void ShowObjects( PVIEW pv )
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
  SetClip( (RCOORD)GetDisplayImage( pv->hVideo )->width, 
           (RCOORD)GetDisplayImage( pv->hVideo )->height );
   ShowObjectChildren( pv, po );  // all children...
   UpdateDisplay( pv->hVideo );
}


void Update( PVIEW pv )
{
   if( pv->hVideo )
	{

      lprintf( "... should we clear here?" );
      Redraw( pv->hVideo );
	//ClearVideo( pv->hVideo ) ;
   }
}

void DestroyView( PVIEW pv )
{
   if( pv->hVideo )  // could already be closed...
      CloseDisplay( pv->hVideo );
   Release( pv );
   
}

void MoveView( PVIEW pv, PCVECTOR v )
{
   TranslateV( pv->T, v );
}
// $Log: View.c,v $
// Revision 1.4  2003/03/25 08:59:03  panther
// Added CVS logging
//
