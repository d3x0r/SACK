#include <stdhdrs.h>
#include <sharemem.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "global.h"

#include <gl/gl.h>
#include <gl/glu.h>

#include <timers.h>
#include "vidlib.h"

#include "object.h"
#include "view.h"
#include "key.h"

#define WIREFRAME
#define DRAW_AXIS
// #define PRINT_LINES //(slow slow slow)

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
	int nFacetSet;
	int nFacet;
	int Invert;
	PTRANSFORM TEdit;
} EDIT_INFO, *PEDIT_INFO;

EDIT_INFO EditInfo;
#define GetEditFacetSet() (EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets + EditInfo.nFacetSet )
#define GetEditFacet() (EditInfo.pEditObject->objinfo.FacetPool.pFacets + GetEditFacetSet()->pFacets[EditInfo.nFacet].nFacet )


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

int CPROC ViewMouse( PTRSZVAL dwView, S_32 x, S_32 y, _32 b )
{
	VIEW *v = (VIEW*)dwView;
	int SetChanged;
		SetChanged = FALSE;	
      if( KeyDown(  v->hVideo, KEY_E ) )
      {
      	EditInfo.bEditing = !EditInfo.bEditing;
      	if( EditInfo.bEditing )
      	{
      		EditInfo.pEditObject = pFirstObject;
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
         	EditInfo.pEditObject = EditInfo.pEditObject->pNext;
         	if( !EditInfo.pEditObject )
         	   EditInfo.pEditObject = pFirstObject;
         	EditInfo.nFacetSet = 0;
         	EditInfo.nFacet = 0;
         	SetChanged = TRUE;
         }
      
         if( KeyDown( v->hVideo, KEY_S ) )
         {
         	EditInfo.nFacetSet++;
         	if( EditInfo.nFacetSet >= EditInfo.pEditObject->objinfo.FacetSetPool.nUsedFacetSets )
         		EditInfo.nFacetSet = 0;	
         	EditInfo.nFacet = 0;
        		SetChanged = TRUE;
         }
         if( KeyDown( v->hVideo, KEY_F ) )
         {
         	EditInfo.nFacet++;
         	if( EditInfo.nFacet >= EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[EditInfo.nFacetSet].nUsedFacets )
         		EditInfo.nFacet = 0;
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
         	nfs = GetFacetSet( &EditInfo.pEditObject->objinfo );
         	pf = GetEditFacet();
         	nf = AddPlaneToSet( &EditInfo.pEditObject->objinfo, nfs, pf->d.o, pf->d.n, 1 );
         	nfp = GetFacetP( &EditInfo.pEditObject->objinfo, nfs );
         	EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[nfs].pFacets[nfp].nFacet = 
         	   EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[EditInfo.nFacetSet].pFacets[EditInfo.nFacet].nFacet;
				EditInfo.pEditObject->objinfo.FacetSetPool.pFacetSets[nfs].pFacets[nfp].bInvert = TRUE;
				{
				   int l;
				   PLINESEGPSET plps = &pf->pLineSet;
				   for( l = 0; l < plps->nUsedLines; l++ )
				   {
				   	VECTOR n, ln;
				   	if( plps->pLines[l].nLine < 0 )
				   		continue;
						SetPoint( ln, EditInfo.pEditObject->objinfo.LinePool.pLines[plps->pLines[l].nLine].d.n );
						if( plps->pLines[l].bOrderFromTo  )
							Invert( ln );
						crossproduct( n, pf->d.n, ln );
				   	AddPlaneToSet( &EditInfo.pEditObject->objinfo, nfs
				   					  , EditInfo.pEditObject->objinfo.LinePool.pLines[plps->pLines[l].nLine].d.o
				   					  , n, 1 );
				   }
				}
         	EditInfo.nFacetSet = nfs;
         	EditInfo.nFacet = nf;
         }
         if( SetChanged )
         {
         	PFACET pf;
         	pf = GetEditFacet();
		   	RotateTo( EditInfo.TEdit, pf->d.n
		   				, EditInfo.pEditObject->objinfo.LinePool.pLines[pf->pLineSet.pLines[0].nLine].d.n );
				TranslateV( EditInfo.TEdit, pf->d.o );
			}
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
		gluPerspective(90.0f,1.0f,0.1f,300.0f);
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		glLoadIdentity();									// Reset The Modelview Matrix
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


void CPROC _ShowObjects( PTRSZVAL dwView, PRENDERER pRenderer )
{

	VIEW *v = (VIEW*)dwView;

	ShowObjects( v );

	if( EditInfo.bEditing )
	{
		char buf[256];
		snprintf( buf, sizeof( buf ), "Editing: O: %p FS:%d F:%d"
      				, EditInfo.pEditObject
      				, EditInfo.nFacetSet
      				, EditInfo.nFacet );
		PutString( GetDisplayImage( v->hVideo )
					, 4, GetDisplayImage( v->hVideo )->height - 11
					, Color( 255,255,255 ), Color(0,0,0)
					, buf );
		{
			PFACET pf;
			RAY rf;
	   		pf = GetEditFacet();
			ApplyR( v->Twork, &rf, &pf->d );
			DrawLine( GetDisplayImage( v->hVideo ), rf.o, rf.n, 0, 10, 0x3f5f9f );
   		}
	}
}

char *viewname[] = { "FORWARD", "RIGHT", "LEFT","BACK","UP","DOWN" };

void CPROC TimerProc( PTRSZVAL psv )
{
static VECTOR KeySpeed, KeyRotation;
extern POBJECT pFirstObject;
//   static PTRANSFORM SaveT;
static POBJECT pCurrent;
   MATRIX m;
   VIEW *View;

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

	ScanKeyboard( NULL, KeySpeed, KeyRotation );
	if( !EditInfo.bEditing || IsKeyDown(  View->hVideo, KEY_CONTROL ) )
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
			IntersectPlanes( &EditInfo.pEditObject->objinfo,  EditInfo.nFacetSet, TRUE );
		}
	}


	View = pMainView; // start at main....

	while( View )
	{
		Redraw( View->hVideo );
#if 0
		if( HasFocus( View->hVideo ) )
		{
			if( pCurrent )
				View->T = pCurrent->T;
			else
			{
			}
		}
		// need a full apply here - the origin is relative to the
		// direction viewed, and therefore needs to be translated appropriately
      // the orientation of the matrix also needs to rotate according to the view.
		ApplyT( View->T, View->Twork, View->Tglobal );
		Log1( "Transforms for view : %s", viewname[View->Type] );
      ShowTransform( View->T );
      ShowTransform( View->Tglobal );
		ShowTransform( View->Twork );

		Log( "VIEW BEGIN" );

		{
			VECTOR m, b;
			// rotate world into view coordinates... mouse is void(0) coordinates...

			//UpdateThisCursorPos(); // no parameter version same x, y...
			//         View->DoMouse();

			//ApplyInverse( View->T, b, mouse_origin );
			//ApplyInverseRotation( View->T, m, mouse_vforward );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f );
			//ApplyInverseRotation( View->T, m, mouse_vright );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f00 );
			//ApplyInverseRotation( View->T, m, mouse_vup );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f0000 );
		}

		_ShowObjects( (PTRSZVAL)View, View->hVideo );

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
#endif
		//SetActiveGLDisplay( NULL );
		View = View->Previous;
	}
	//Log( "VIEWS DONE" );
}


void CPROC CloseView( PTRSZVAL dwView )
{
   VIEW *V;
   V = (VIEW*)dwView;
   V->hVideo = (PRENDERER)NULL; // release from window side....
}

PVIEW CreateViewEx( int nType, ViewMouseCallback pMC, char *Title, int sx, int sy )
{
	PVIEW pv;
	pv = Allocate( sizeof( VIEW ) );
	MemSet( pv, 0, sizeof( VIEW ) );
	pv->T = CreateTransform();
	pv->Twork = CreateTransform();
	pv->Type = nType;

	//pv->hVideo = OpenDisplaySizedAt( 0, 600, 600, sx, sy  );
	pv->hVideo = OpenDisplaySizedAt( 0, 256, 256, sx, sy  );
	if( !EnableOpenGL( pv->hVideo ) )
	{
		CloseDisplay( pv->hVideo );
		Release( pv );
		return NULL;
	}
	//InitGL();
	SetRedrawHandler( pv->hVideo, _ShowObjects, (PTRSZVAL)pv );
	SetCloseHandler( pv->hVideo, CloseView, (PTRSZVAL)pv );
	SetMouseHandler( pv->hVideo, ViewMouse, (PTRSZVAL)pv );

	pv->MouseMethod = pMC;

	pv->Previous = pMainView;
	if( !pMainView )
	{
		AddTimer(  50, TimerProc, 0 );
		pv->Tglobal = CreateTransform();
		CreateTransformMotion( pv->Tglobal );
	}
	else
		pv->Tglobal = pMainView->Tglobal;
	pMainView = pv;
	switch( nType )
	{
	case V_FORWARD:
		break;
	case V_RIGHT:
		RotateRight( pv->T, vForward, vRight );
		break;
	case V_LEFT:
		RotateRight( pv->T, vRight, vForward );
		break;
	case V_UP:
		RotateRight( pv->T, vForward, vUp );
		break;
	case V_DOWN:
		RotateRight( pv->T, vUp, vForward );
		break;
	case V_BACK:
		RotateRight( pv->T, -1, -1 );
		break;
	}
	// show it the first time.
	RestoreDisplay( pv->hVideo );

	return pv;
}

PVIEW CreateView( ViewMouseCallback pMC, char *Title )
{           	
	return CreateViewEx( V_FORWARD, pMC, Title, 0, 0 );
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

#define ProjectX( v )  ((pImage->width/2)  + (int)( ( ((RCOORD)pImage->width)  * (v)[0] ) / ( (v)[2] * 2 ) ) )
#define ProjectY( v )  ((pImage->height/2) + (int)( ( ((RCOORD)pImage->height) * (v)[1] ) / ( (v)[2] * 2 ) ) )
void DrawLine( Image pImage, PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c )
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
   if( Clip( v1, v2, vc0.n, vc0.o ) &&
       Clip( v1, v2, vc1.n, vc1.o ) &&
       Clip( v1, v2, vc2.n, vc2.o ) &&
       Clip( v1, v2, vc3.n, vc3.o ) &&
       Clip( v1, v2, vc4.n, vc4.o ) )
	{

		do_line( pImage, ProjectX( v1 ), ProjectY( v1 ),
				  ProjectX( v2 ), ProjectY( v2 ), c );
	}
}


void GetViewPoint( Image pImage, IMAGE_POINT presult, PVECTOR vo )
{
	presult[0] = ProjectX( vo );
	presult[1] = ProjectY( vo );
}

void GetRealPoint( Image pImage, PVECTOR vresult, IMAGE_POINT pt )
{
	if( !vresult[2] )
		vresult[2] = 1.0f;  // dumb - but protects result...
	// use vresult Z for unprojection...
	vresult[0] = (pt[0] - (pImage->width/2)) * (vresult[2] * 2.0f) / ((RCOORD)pImage->width);
	vresult[1] = ((pImage->height/2) - pt[1] ) * (vresult[2] * 2.0f) / ((RCOORD)pImage->height);
}

void ShowObject( PVIEW pv, POBJECT po )
{
	PG_LINESEG pl;
	int l;
	static PTRANSFORM T;
	if( !T )
	{
		T = CreateTransform();
	}

	ApplyT( pv->T, pv->Twork, pv->Tglobal );

	// for all objects that have been created.
	// draw the object on the screen.

	// apply Twork ( Tglobal + T (view direction) ) to object
	// (move object into view space)
	ApplyInverseT( pv->Twork, T, po->T );
	//ApplyInverseTranslationT( pv->Tglobal, T, po->Ti );
	//ShowTransform( po->T );
	//ShowTransform( T );
	// draw axis
	{
		RAY r;
		VECTOR v;
#define AXIS_TRANSFORM T
		GetOriginV( T, r.o );
		GetAxisV( T, r.n, 0 );
		DrawLine( GetDisplayImage( pv->hVideo ), r.o, r.n, 0, 1, Color( 0, 0, 0x7f ) );
		GetAxisV( T, r.n, 1 );
		DrawLine( GetDisplayImage( pv->hVideo ), r.o, r.n, 0, 1, Color( 0, 0x7f, 0 ) );
		GetAxisV( T, r.n, 2 );
		DrawLine( GetDisplayImage( pv->hVideo ), r.o, r.n, 0, 1, Color( 0x7f, 0, 0 ) );
   }
   // draw object...
   {
		PLINESEGSET pls;
		PLINESEGPSET plps;
		PFACETSETSET pfss;
		PFACETPSET pfps;
		PFACETSET pfs;
		int nfp;
		pls = &po->objinfo.LinePool; // for all lines ONLY... not planes too..
		pfss = &po->objinfo.FacetSetPool;
		pfs = &po->objinfo.FacetPool;
		//Log( "Begin facet..." );
		for( nfp = 0; nfp < pfss->nUsedFacetSets; nfp++ )
		{
			PFACETP pfp;
			PFACET pf;
			int nfs;
			pfps = pfss->pFacetSets + nfp;
			for( nfs = 0; nfs < pfps->nUsedFacets; nfs++ )
			{
				pfp = pfps->pFacets + nfs;
				pf = pfs->pFacets + pfp->nFacet;
				for( l = 0; l < pls->nUsedLines; l++ )
				{
					RAY rl[2], rvl;
					pl = pls->pLines + l;
					ApplyR( T, &rvl, &pl->d );

					DrawLine( GetDisplayImage( pv->hVideo ),
                      rvl.o,
                      rvl.n,
                      pl->dFrom,
                      pl->dTo,
                      Color( 255, 255, 255 ) );
				}
				// now we know we can show this facet...
#if 0
				{
					int points;
					VECTOR pvPoints[10];
               points = 10;
               GetPoints( pls, pf, &points, pvPoints );

					for( l = 0; l < points; l++ )
					{
                  Apply( T, pvPoints[l], pvPoints[l] );
					}

               /*
					glBegin( GL_POLYGON );
					//glColor3f(1.0f,1.0f,0.0f);			// Set The Color To Yellow
					glColor4ubv( (unsigned char *)&po->color );
				   for( l = 0; l < points; l++ )
					{
						glVertex3dv( pvPoints[l] );
					}
					glEnd();
               */
					glBegin( GL_LINE_STRIP );
					glColor4ub( 255,255,255,255 );
				   for( l = 0; l < points; l++ )
					{
						glVertex3dv( pvPoints[l] );
					}
					glEnd();
				}
#endif
			}
		}
   }
}


void ShowObjectChildren( PVIEW pv, POBJECT po, POBJECT parent )
{
	POBJECT pc;
	if( !po )
		return;
	FORALLOBJ( po, pc )
	{
		if( parent )
			ApplyT( parent->T, pc->T, pc->Ti );
		else
			ApplyT( VectorConst_I, pc->T, pc->Ti );
		ShowObject( pv, pc );
		if( pc->pHolds )
			ShowObjectChildren( pv, pc->pHolds, pc );
		if( pc->pHas )
			ShowObjectChildren( pv, pc->pHas, pc );
	}
}

void ShowObjects( PVIEW pv )
{
	POBJECT po;
	po = pFirstObject; // some object..........
	lprintf( "First object is %p", po );
	while( po && ( po->pIn || po->pOn ) ) // go to TOP of tree...
	{
		if( po->pIn )
			po = po->pIn;
		else if( po->pOn )
			po = po->pOn;
	}
	ShowObjectChildren( pv, po, NULL );  // all children...
}



void DestroyView( PVIEW pv )
{
	if( pv->hVideo )  // could already be closed...
		CloseDisplay( pv->hVideo );
	Release( pv );
   
}

void MoveView( PVIEW pv, PCVECTOR v )
{
	TranslateV( pv->Tglobal, v );
	ApplyT( pv->T, pv->Twork, pv->Tglobal );
}
