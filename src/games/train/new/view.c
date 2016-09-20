#include <sharemem.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __LINUX__
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <gl/gl.h>
#include <gl/glu.h>
#endif

#include <timers.h>
#include "vidlib.h"

#include "object.h"
#include "view.h"
#include "key.h"

//#define WIREFRAME
//#define DRAW_AXIS
// #define PRINT_LINES //(slow slow slow)
#define VIEW_SIZE 400
//-------------------------------------------------
// OPTIONS AFFECTING DISPLAY ARE IN VIEW.H
//-------------------------------------------------

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
typedef struct {
	LOGICAL bEditing;
	LOGICAL bInit;
	POBJECT pEditObject;
	PFACET pFacet;
   PFACETSET pfs;
	int nFacetSet;
	int nFacet;
	int Invert;
	PTRANSFORM TEdit;
} EDIT_INFO, *PEDIT_INFO;

EDIT_INFO EditInfo;
#define GetEditFacetSet() EditInfo.pfs
#define GetEditFacet() EditInfo.pFacet


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
      pv->MouseMethod( pv->hVideo, 
                           mouse_vforward, 
                           mouse_vright, 
                           mouse_vup, 
                           mouse_origin, 
                           mouse_buttons);
}


void ComputePlaneRay( PRAY out )
{
	RAY in;
	SetPoint( in.n, VectorConst_Z );
	SetPoint( in.o, VectorConst_0 );
	ApplyR( EditInfo.TEdit, out, &in );
}

int CPROC ViewMouse( uintptr_t dwView, int32_t x, int32_t y, uint32_t b )
{
   VIEW *v = (VIEW*)dwView;
	int SetChanged;
		SetChanged = FALSE;	
      if( KeyDown(  v->hVideo, KEY_E ) )
      {
      	EditInfo.bEditing = !EditInfo.bEditing;
      	if( EditInfo.bEditing )
      	{
      		EditInfo.pEditObject = FirstObject.next;
      		EditInfo.nFacetSet = 0;
      		EditInfo.nFacet = 0;
      		if( !EditInfo.TEdit )
      			EditInfo.TEdit = CreateTransform();
      		SetChanged = TRUE;
      	}
      }
      if( EditInfo.bEditing )
      {
         if( KeyDown(  v->hVideo, KEY_O ) )
         {
         	// change editing object
         	EditInfo.pEditObject = NextLink( EditInfo.pEditObject );
         	if( !EditInfo.pEditObject )
         	   EditInfo.pEditObject = FirstObject.next;
         	EditInfo.nFacetSet = 0;
         	EditInfo.nFacet = 0;
         	SetChanged = TRUE;
         }
      
         if( KeyDown( v->hVideo, KEY_S ) )
         {
				EditInfo.nFacetSet++;
#if 0
         	if( EditInfo.nFacetSet >= EditInfo.pEditObject->objinfo->FacetSetPool.nUsedFacetSets )
         		EditInfo.nFacetSet = 0;	
				EditInfo.nFacet = 0;
#endif
        		SetChanged = TRUE;
         }
         if( KeyDown( v->hVideo, KEY_F ) )
         {
				EditInfo.nFacet++;
#if 0
         	if( EditInfo.nFacet >= EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[EditInfo.nFacetSet].nUsedFacets )
					EditInfo.nFacet = 0;
#endif
         	SetChanged = TRUE;
         }
         if( KeyDown( v->hVideo, KEY_I ) )
         {
         	EditInfo.Invert = !EditInfo.Invert;
         }
         if( KeyDown(  v->hVideo, KEY_N ) )
         {
         	int nfs, nf, nfp;
         	PFACET pf;
				//nfs = GetFacetSet( &EditInfo.pEditObject->objinfo );
#if 0
         	pf = GetEditFacet();
         	nf = AddPlaneToSet( EditInfo.pEditObject->objinfo, pf->d.o, pf->d.n, 1 );
         	nfp = GetFacetP( EditInfo.pEditObject->objinfo, nfs );
         	EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[nfs].pFacets[nfp].nFacet = 
         	   EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[EditInfo.nFacetSet].pFacets[EditInfo.nFacet].nFacet;
				EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[nfs].pFacets[nfp].bInvert = TRUE;
				{
				   int l;
					PLINESEGPSET *pplps = &pf->pLineSet;
               int lines = CountUsedInSet( LINESEGP, pf->pLineSet );
				   for( l = 0; l < lines; l++ )
				   {
						VECTOR n, ln;
                  PLINESEGP plsp = GetSetMember( LINESEGP, pplps, l );
                  PMYLINESEG line = GetSetMember( MYLINESEG, &EditInfo.pEditObject->objinfo.LinePool, plsp->nLine );
				   	if( plsp->nLine < 0 )
				   		continue;
						SetPoint( ln, line->r.n );
						if( plsp->bOrderFromTo  )
							Invert( ln );
						crossproduct( n, pf->d.n, ln );
				   	AddPlaneToSet( &EditInfo.pEditObject->objinfo, nfs
				   					  , line->r.o
				   					  , n, 1 );
				   }
				}
         	EditInfo.nFacetSet = nfs;
				EditInfo.nFacet = nf;
#endif
			}
#if 0
         if( SetChanged )
         {
				PFACET pf;
            PLINESEGP plsp = GetSetMember( LINESEGP, &pf->pLineSet, 0 );
            PMYLINESEG line = GetSetMember( MYLINESEG, &EditInfo.pEditObject->objinfo.LinePool, plsp->nLine );
         	pf = GetEditFacet();
		   	RotateTo( EditInfo.TEdit, pf->d.n
						  , line->r.n );
				TranslateV( EditInfo.TEdit, pf->d.o );
			}
#endif
		}
   mouse_buttons = ( mouse_buttons & 0xF0 ) | b;

   // should pass x and y to update cursor pos...
   UpdateCursorPos( v, x, y ); // this view.... this mouse eventing

		DoMouse( v );
return 1;
}

int bDump;


#define MODE_UNKNOWN 0
#define MODE_PERSP 1
#define MODE_ORTHO 2
int mode = MODE_UNKNOWN;

void BeginVisPersp( void )
{
	//if( mode != MODE_PERSP )
	{
		mode = MODE_PERSP;
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glLoadIdentity();									// Reset The Projection Matrix
		// Calculate The Aspect Ratio Of The Window
		gluPerspective(90.0f,1.0f,0.1f,30000.0f);
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		//glLoadIdentity();									// Reset The Modelview Matrix
	}
}


int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

   BeginVisPersp();
	return TRUE;										// Initialization Went OK
}


void CPROC _ShowObjects( uintptr_t dwView, PRENDERER pRenderer )
{

	VIEW *v = (VIEW*)dwView;
   lprintf( "Show everythign." );
	ShowObjects( v );
   lprintf( "done." );
	glFlush();
   lprintf( "Flushed." );
}

void CPROC TimerProc( uintptr_t psv )
{
static VECTOR KeySpeed, KeyRotation;
//   static PTRANSFORM SaveT;
static POBJECT pCurrent;
   MATRIX m;
   VIEW *View;

	ScanKeyboard( NULL, KeySpeed, KeyRotation );

   bDump = FALSE;


   if( KeyDown( pMainView->hVideo, KEY_SPACE ) )
	{
      exit(0);
     // mouse_buttons |= KEY_BUTTON1;
	}
   /*
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
    	if( !EditInfo.bEditing || IsKeyDown(  pMainView->hVideo, KEY_CONTROL ) )
    	{
			SetSpeed( pMainView->Tglobal, KeySpeed );
			SetRotation( pMainView->Tglobal, KeyRotation );
			Move(pMainView->Tglobal);  // relative rotation...
		}
      else if( !EditInfo.bEditing ) // editing without control key pressed
      {
      	KeySpeed[vRight] = 0;
      	KeySpeed[vUp] = 0;
      	KeyRotation[vForward] = 0;
      	if( Length( KeySpeed ) || Length( KeyRotation ) )
      	{
	      	SetSpeed( EditInfo.TEdit, KeySpeed );
   	   	SetRotation( EditInfo.TEdit, KeyRotation );
      		Move( EditInfo.TEdit );
	      	ComputePlaneRay( &GetEditFacet()->d );
   	   	IntersectPlanes( EditInfo.pEditObject->objinfo, TRUE );
			}
      }
	}

	View = pMainView; // start at main....

	while( View )
	{
		//Log( "VIEW BEGIN" );

		SetActiveGLDisplayView( View->hVideo, View->nFracture );
		if( !View->flags.bInited )
		{
			InitGL();
         View->flags.bInited = TRUE;
		}
		// set active GL viewport...
		glClear(GL_COLOR_BUFFER_BIT
				  | GL_DEPTH_BUFFER_BIT
				 );	// Clear Screen And Depth Buffer

		{
			VECTOR m, b;
			// rotate world into view coordinates... mouse is void(0) coordinates...

			UpdateThisCursorPos(); // no parameter version same x, y...
			//         View->DoMouse();

			//ApplyInverse( View->T, b, mouse_origin );
			//ApplyInverseRotation( View->T, m, mouse_vforward );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f );
			//ApplyInverseRotation( View->T, m, mouse_vright );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f00 );
			//ApplyInverseRotation( View->T, m, mouse_vup );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f0000 );
		}
		//ApplyInverseT( View->T, View->Twork, View->Tglobal );
		//ApplyT( View->T, View->Twork, View->Tglobal );
      //TranslateV( View->Twork, GetOrigin( View->Tglobal ));
		if( View->Type == V_FORWARD )
		{
         // this might be usefule information...
			//ShowTransform( View->Twork );
			//ShowTransform( View->Tglobal );
		}

		_ShowObjects( (uintptr_t)View, View->hVideo );

		if( EditInfo.bEditing )
		{
			char buf[256];
      	sprintf( buf, "Editing: O: %08x FS:%d F:%d"
      					, EditInfo.pEditObject
      					, EditInfo.nFacetSet
      					, EditInfo.nFacet );
			PutString( GetDisplayImage( View->hVideo )
						, 4, GetDisplayImage( View->hVideo )->height - 11
						, Color( 255,255,255 ), Color(0,0,0)
						, buf );
	      {
	   		PFACET pf;
	   		RAY rf;
	   		pf = GetEditFacet();
				ApplyR( View->Twork, &rf, &pf->d );
				DrawLine( GetDisplayImage( View->hVideo ), rf.o, rf.n, 0, 10, 0x3f5f9f );
   	   }
			
      }

		//glFlush();

		View = View->Previous;
	}
	SetActiveGLDisplay( NULL );
	UpdateDisplay( pMainView->hVideo );
	//Log( "VIEWS DONE" );
}


void CPROC CloseView( uintptr_t dwView )
{
   VIEW *V;
   V = (VIEW*)dwView;
   V->hVideo = (PRENDERER)NULL; // release from window side....
}


void RotateMatrix( int nType, PTRANSFORM T )
{
	switch( nType )
	{
	case V_FORWARD:
		break;
	case V_RIGHT:
#define MOVE_WORLD_ORIGIN
#ifdef MOVE_WORLD_ORIGIN
		RotateRight( T, vRight, vForward );
#else
		RotateRight( T, vForward, vRight );
#endif
		break;
	case V_LEFT:
#ifdef MOVE_WORLD_ORIGIN
		RotateRight( T, vForward, vRight );
#else
		RotateRight( T, vRight, vForward );
#endif
		break;
	case V_UP:
#ifdef MOVE_WORLD_ORIGIN
		RotateRight( T, vUp, vForward );
#else
		RotateRight( T, vForward, vUp );
#endif
		break;
	case V_DOWN:
#ifdef MOVE_WORLD_ORIGIN
		RotateRight( T, vForward, vUp );
#else
		RotateRight( T, vUp, vForward );
#endif
		break;
	case V_BACK:
#ifdef MOVE_WORLD_ORIGIN
		RotateRight( T, -1, -1 );
#else
		RotateRight( T, -1, -1 );
#endif
		break;
	}
}

PVIEW CreateViewEx( int nType, ViewMouseCallback pMC, char *Title, int sx, int sy )
{
   static HVIDEO hv;
   uint32_t width, height;
	PVIEW pv;
   uint32_t winsz;
   pv = Allocate( sizeof( VIEW ) );
   memset( pv, 0, sizeof( VIEW ) );
   pv->T = CreateTransform();
	pv->Twork = CreateTransform();
	pv->Type = nType;
	GetDisplaySize( &width, &height );
	if( width / 4 < height / 3 )
		winsz = width / 1/*4*/;
	else
		winsz = height / 1/*3*/;

   winsz = 512;  // 800x600 still fits this..
//   SetPoint( r, pr );

	//pv->hVideo = OpenDisplaySizedAt( 0, 600, 600, sx, sy  );
	if( !hv )
		hv = OpenDisplayAboveSizedAt( 0, winsz /** 4*/, winsz /** 3*/, 0, 0, NULL );
   pv->hVideo = hv;
	if( !( pv->nFracture = EnableOpenGLView( pv->hVideo, 0, 0, /*sx * winsz, sy * winsz, */winsz, winsz ) ) )
	//pv->hVideo = OpenDisplayAboveSizedAt( 0, winsz, winsz, sx * winsz, sy  * winsz, pMainView?pMainView->hVideo:NULL );
	//if( !( EnableOpenGL( pv->hVideo ) ) )
	{
		CloseDisplay( pv->hVideo );
      Release( pv );
		return NULL;
	}
	InitGL();
   if( !pMainView )
   {
		AddTimer(  50, TimerProc, 0 );
      pv->Tglobal = CreateTransform();
	}
   else
		pv->Tglobal = pMainView->Tglobal;

	SetRedrawHandler( pv->hVideo, _ShowObjects, (uintptr_t)pv );
	SetCloseHandler( pv->hVideo, CloseView, (uintptr_t)pv );
	SetMouseHandler( pv->hVideo, ViewMouse, (uintptr_t)pv );

   pv->MouseMethod = pMC;

	pv->Previous = pMainView;

	pMainView = pv;

   return pv;
}

PVIEW CreateView( ViewMouseCallback pMC, char *Title )
{           	
	return CreateViewEx( V_FORWARD, pMC, Title, 0, 0 );
}

void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c )
{
   VECTOR v1,v2;
	glBegin( GL_LINES );
	glColor4ubv( (unsigned char *)&c );
	glVertex3dv( add( v1, scale( v1, m, t1 ), p ) );
	glVertex3dv( add( v2, scale( v2, m, t2 ), p ) );
	glEnd();
}


void GetViewPoint( Image pImage, IMAGE_POINT presult, PVECTOR vo )
{
   //presult[0] = ProjectX( vo );
   //presult[1] = ProjectY( vo );
}

void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt )
{
   if( !vresult[2] )
      vresult[2] = 1.0f;  // dumb - but protects result...
   // use vresult Z for unprojection...
   vresult[0] = (pt[0] - (pImage->width/2)) * (vresult[2] * 2.0f) / ((RCOORD)pImage->width);
   vresult[1] = ((pImage->height/2) - pt[1] ) * (vresult[2] * 2.0f) / ((RCOORD)pImage->height);
}


uintptr_t CPROC RenderFacet( POBJECT po, PFACET pf )
{
	//POBJECT po = (POBJECT)psv;
	//PFACET pf = (PFACET)member;

	RAY rl[2], rvl;
	int t, p;
	POBJECT pi; // pIn Tree...

#if 0
	if( !pf->bDual )
	{
		RAY r;
		int invert;
		ApplyR( po->Ti, &r, &pf->d );
		invert = pf->bInvert;
		if( EditInfo.bEditing && EditInfo.Invert )
			invert = !invert;
		if( po->bInvert )
			invert = !invert;
		if( invert )
		{
			if( dotproduct( r.o, r.n ) > 0 )
			{
				// draw plane normal - option at some point....
				//DrawLine( GetDisplayImage( pv->hVideo ),
				//			 r.o, r.n, 0, 2, Color( 255, 0, 255 ) );
				//break;
			}
		}
		else
		{
			if( dotproduct( r.o, r.n ) < 0 )
			{
				// draw plane normal - option at some point....
				//DrawLine( GetDisplayImage( pv->hVideo ),
				//			 r.o, r.n, 0, 2, Color( 255, 0, 255 ) );
				//break;
			}
		}
	}
#endif
	//Log( "Begin facet..." );
	// now we know we can show this facet...
	{
		int l;
		int lstart = 0;
		int points;
		VECTOR pvPoints[10];
      VECTOR v;
		points = 10;
		GetPoints( pf, &points, pvPoints );
		glBegin( GL_POLYGON );
		//glColor3f(1.0f,1.0f,0.0f);			// Set The Color To Yellow
		glColor4ubv( (unsigned char *)&po->color );
		for( l = 0; l < points; l++ )
		{
			Apply( po->Ti, v, pvPoints[l] );
			//SetPoint( pvPoints[l], v );
			glVertex3dv( v );
		}
		glEnd();

#if defined WIREFRAME
		glBegin( GL_LINE_STRIP );
		glColor4ub( 255,255,255,255 );
		for( l = 0; l < points; l++ )
		{
			Apply( po->Ti, v, pvPoints[l] );
			glVertex3dv( v );
		}
		glEnd();
#endif
	}
	return 0;
}

void ShowObjectChildren( PVIEW pv, POBJECT po )
{
   POBJECT pc;
   if( !po )
		return;
   FORALLOBJ( po, pc )
   {
		{
			INDEX idx;
			PFACET facet;
			PLIST list = pc->objinfo->facets;
		LIST_FORALL( list, idx, PFACET, facet )
		{
			RenderFacet( pc, facet );
		}
		}
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

	{
		MATRIX m;
		static PTRANSFORM T, I;
		if( !T )
		{
			T = CreateTransform();
			I = CreateTransform(); // identity transform const
		}
		ApplyT      ( VectorConst_I, pv->Twork, pv->Tglobal );
		RotateMatrix( pv->Type, pv->Twork );
		//ApplyT      ( pv->Twork, T, po->Ti );
		GetGLMatrix ( pv->Twork, m );
		glLoadMatrixd( m );
	}
  //SetClip( (RCOORD)GetDisplayImage( pv->hVideo )->width,
  //         (RCOORD)GetDisplayImage( pv->hVideo )->height );
   ShowObjectChildren( pv, po );  // all children...
   //WriteToWindow( pv->hVideo, 0, 0, 0, 0 );
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
