#define USE_RENDER3D_INTERFACE g.pr3i
#define USE_IMAGE_3D_INTERFACE g.pi3i
#include <stdhdrs.h>

//#define USE_RENDER_INTERFACE l.pri

#define VIEW_MAIN
#include "global.h"

#include <psi.h>

#include <virtuality.h>



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


static struct local_view_data
{
	PVIEW view;
} local_view_data;
#define l local_view_data

 static VECTOR mouse_vforward,  // complete tranlations...
          mouse_vright, 
          mouse_vup, 
          mouse_origin;
 static int mouse_buttons;
#define KEY_BUTTON1 0x10
#define KEY_BUTTON2 0x20
#define KEY_BUTTON3 0x40

      static VIEW *MouseIn;

	  PRELOAD( InitLocal )
	  {
		  g.pii = GetImageInterface();
		  g.pri = GetDisplayInterface();
			g.pr3i = GetRender3dInterface();
			g.pi3i = GetImage3dInterface();
	  }

   void UpdateCursorPos( PVIEW pv, int x, int y )
   {
      VECTOR v;

      MouseIn = pv;

      //GetAxisV( pv->T, mouse_vforward, vForward );
      //GetAxisV( pv->T, mouse_vright, vRight );
      //GetAxisV( pv->T, mouse_vup, vUp );

      // v forward should be offsetable by mouse...
      //scale( v, GetAxis( pv->T, vForward ), 100.0 );
      //add( mouse_origin, GetOrigin(pv->T), v ); // put it back in world...
   }

   void UpdateThisCursorPos( void )
   {
      VECTOR v;
      if( MouseIn )
      {
         //GetAxisV( MouseIn->T, mouse_vforward, vForward );
         //GetAxisV( MouseIn->T, mouse_vright, vRight );
         //GetAxisV( MouseIn->T, mouse_vup, vUp );

         // v forward should be offsetable by mouse...
         //scale( v, GetAxis( MouseIn->T, vForward ), 100.0 );
         //add( mouse_origin, GetOrigin(MouseIn->T), v ); // put it back in world...
      }
   }


int DoMouse( PVIEW pv )
{

	// only good for direct ahead manipulation....

	// no deviation from forward....
	if( pv->MouseMethod )
		return pv->MouseMethod( pv->hVideo, 
                           mouse_vforward, 
                           mouse_vright, 
                           mouse_vup, 
                           mouse_origin, 
                           mouse_buttons);
	return 0;
}

void DrawLine( PCVECTOR p, PCVECTOR m, RCOORD t1, RCOORD t2, CDATA c )
{
	VECTOR v1,v2;
	glBegin( GL_LINES );
	glColor4ubv( (unsigned char *)&c );
	glVertex3fv( add( v1, scale( v1, m, t1 ), p ) );
	glVertex3fv( add( v2, scale( v2, m, t2 ), p ) );
	glEnd();
}

void ComputePlaneRay( PRAY out )
{
	RAY in;
	SetPoint( in.n, VectorConst_Z );
	SetPoint( in.o, VectorConst_0 );
	ApplyR( (PCTRANSFORM)EditInfo.TEdit, out, &in );
}

static LOGICAL OnMouse3d( WIDE("Virtuality") )( PTRSZVAL psvView, PRAY mouse, _32 b )
//int CPROC ViewMouse( PTRSZVAL dwView, S_32 x, S_32 y, _32 b )
{
   VIEW *v = (VIEW*)psvView;
	int SetChanged;
		SetChanged = FALSE;	


/*
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
  */


      if( KeyDown(  v->hVideo, KEY_E ) )
      {
      	EditInfo.bEditing = !EditInfo.bEditing;
      	if( EditInfo.bEditing )
      	{
      		EditInfo.pEditObject = (POBJECT)pFirstObject;
      		EditInfo.nFacetSet = 0;
      		EditInfo.nFacet = 0;
				if( !EditInfo.TEdit )
				{
					EditInfo.TEdit = CreateTransform();
				}
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
         	   EditInfo.pEditObject = (POBJECT)pFirstObject;
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
   //UpdateCursorPos( v, x, y ); // this view.... this mouse eventing

	if(	DoMouse( v ) )
		return 1;
	return 0;
}

int bDump;



int frames;
int time;

static void OnDraw3d( WIDE("Virtuality") )( PTRSZVAL psvView )
{
   PVIEW pv = (PVIEW)psvView;
	//lprintf( WIDE("Init GL") );

   //lprintf( "Begin Frame." );
   // cannot count that the camera state is relative for how we want to show?

	//lprintf( WIDE("Show everythign.") );
	ShowObjects(  );
	//lprintf( WIDE("done.") );
	//glFlush();
	//lprintf( WIDE("Flushed.") );
		if( EditInfo.bEditing )
		{
			TEXTCHAR buf[256];
		  	snprintf( buf, 256, WIDE("Editing: O: %08x FS:%d F:%d")
      					, EditInfo.pEditObject
      					, EditInfo.nFacetSet
      					, EditInfo.nFacet );
			/*
			PutString( GetDisplayImage( v->hVideo )
						, 4, GetDisplayImage( v->hVideo )->height - 11
						, Color( 255,255,255 ), Color(0,0,0)
						, buf );
			*/

			{
	   			PFACET pf;
	   			RAY rf;
	   			pf = GetEditFacet();
				DrawLine( pf->d.o, pf->d.n, 0, 10, 0x3f5f9f );
			}
			
		}

		//glFlush();
		//SetActiveGLDisplay( NULL );
		if( timeGetTime() != time && ( (frames %30 ) == 0 ))
		{
			//Image pImage = GetDisplayImage( v->hVideo );
			TEXTCHAR buf[256];
			snprintf( buf, 256, WIDE("fps : %d  (x10)")
      					, frames * 10000 /( timeGetTime() - time )
					 );
			lprintf( WIDE("%s"), buf );
			/*
			PutString( pImage
						, 4, 4
						, Color( 255,255,255 ), Color(0,0,0)
						, buf );
						*/
			
      }
      frames++;
		if( frames > 200 )
		{
			time = timeGetTime();
			frames = 0;
		}
   //lprintf( "End Frame." );

}




void CPROC TimerProc2( PTRSZVAL psv )
{
	static VECTOR KeySpeed, KeyRotation;
	VECTOR ks, kr;
	static int skip;
	//   static PTRANSFORM SaveT;
	static POBJECT pCurrent;
	//MATRIX m;
	if( !time )
		time = timeGetTime();

	// scan the keyboard, cause ... well ... it needs 
	// scaled keyspeed and acceleration ticks.
	ScanKeyboard( NULL, KeySpeed, KeyRotation );

	skip++;
	if( skip < 1 )
		return;
	skip = 0;
#if 0
	if( pMainView )
	{
		if( !EditInfo.bEditing || IsKeyDown(  pMainView->hVideo, KEY_CONTROL ) )
		{
			SetSpeed( pMainView->Tglobal, KeySpeed );
			SetRotation( pMainView->Tglobal, KeyRotation );
			Move(pMainView->Tglobal);  // relative rotation...
			//lprintf( WIDE("Updated main view transform.") );
			//ShowTransform(pMainView->Tglobal );
		}
		else if( !EditInfo.bEditing ) // editing without control key pressed
		{
			ks[vRight] = 0;
			ks[vUp] = 0;
			kr[vForward] = 0;
			if( Length( KeyRotation ) || Length( KeySpeed ) )
			{
				SetSpeed( EditInfo.TEdit, KeySpeed );
				SetRotation( EditInfo.TEdit, KeyRotation );
				Move( EditInfo.TEdit );
				ComputePlaneRay( &GetEditFacet()->d );
				IntersectPlanes( EditInfo.pEditObject->objinfo, TRUE );
			}
		}
	}
#endif
}




void CPROC Update( PSI_CONTROL psv )
{
	static POBJECT pCurrent;
	if( pMainView && KeyDown( pMainView->hVideo, KEY_SPACE ) )
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

   // override mainview transform with current object's persepctive

	{
		Log( WIDE("VIEW UPDATE") );


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
	}
}

void CPROC TimerProc( PTRSZVAL psv )
{
   PVIEW View;
	for( View = pMainView;
		  View;
		  View = View->Previous
		)
	{
		//lprintf( WIDE("Tick frame refresh (smuge)") );
		//UpdateDisplay( View->hVideo );
		//SmudgeCommon( View->pcVideo );
	}
}


void CPROC CloseView( PTRSZVAL dwView )
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

static PTRSZVAL OnInit3d( WIDE("Virtuality") )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	PVIEW view = New( VIEW );
	memset( view, 0, sizeof( VIEW ) );
	//view->T = CreateTransform();
	//view->Twork = CreateTransform();
	view->Type = 0;
	if( !pMainView )
	{
		//AddTimer(  10, TimerProc2, 0 );
		//view->Tglobal = CreateTransform();
		//CreateTransformMotion( view->Tglobal );
	}
	//else
	//	view->Tglobal = pMainView->Tglobal;
	if( !l.view )
		l.view = view;
	//view->Tcamera = camera;
	return (PTRSZVAL)view;
}

PVIEW CreateViewWithUpdateLockEx( int nType, ViewMouseCallback pMC, TEXTCHAR *Title, int sx, int sy, PCRITICALSECTION csUpdate )
{
	if( l.view )
		return l.view;
	{
		PRENDERER hv;
		static PLIST hvs;
		_32 width, height;
		PVIEW pv;
		_32 winsz;
		pv = New( VIEW );
		memset( pv, 0, sizeof( VIEW ) );
		pv->csUpdate = csUpdate;
		//pv->T = CreateTransform();
		//pv->Twork = CreateTransform();
		pv->Type = nType;
		if( !RequiresDrawAll() )
		{
			GetDisplaySize( &width, &height );
			if( width / 4 < height / 3 )
				winsz = width / 4;
			else
				winsz = height / 3;

			//winsz = 512;  // 800x600 still fits this..
			//   SetPoint( r, pr );

			//pv->hVideo = OpenDisplaySizedAt( 0, 600, 600, sx, sy  );
			//if( !hv )
			//hv = OpenDisplayAboveSizedAt( 0, 500, 500, -250, -250, NULL );
			hv = OpenDisplayAboveSizedAt( 0, 500, 500, 0, 0, NULL );
			AddLink( &hvs, hv );
			//pv->pcVideo = CreateFrameFromRenderer( WIDE("View Window"), BORDER_NONE, hv );
			pv->hVideo = hv;

			if( !RequiresDrawAll() )
			{
            lprintf( WIDE("using non 3d driver (probably); needed EnableOpenGL") );
				//pv->nFracture = EnableOpenGL( pv->hVideo );
			}
		}
		///if( !( pv->nFracture = EnableOpenGLView( pv->hVideo, 0, 0, /*sx * winsz, sy * winsz, */winsz, winsz ) ) )

		//pv->hVideo = OpenDisplayAboveSizedAt( 0, winsz, winsz, sx * winsz, sy  * winsz, pMainView?pMainView->hVideo:NULL );
		//if( !( EnableOpenGL( pv->hVideo ) ) )
		if(0)
		{
			CloseDisplay( pv->hVideo );
			Release( pv );
			return NULL;
		}


		if( !pMainView )
		{
			//AddTimer(  250, TimerProc, 0 );
			// scan keyboard and move my frame routine...
		//	AddTimer(  10, TimerProc2, 0 );
		//	pv->Tglobal = CreateTransform();
		//	CreateTransformMotion( pv->Tglobal );
		}
		//else
		//	pv->Tglobal = pMainView->Tglobal;

		//SetRedrawHandler( pv->hVideo, _ShowObjects, (PTRSZVAL)pv );
		//SetCloseHandler( pv->hVideo, CloseView, (PTRSZVAL)pv );
		//SetMouseHandler( pv->hVideo, ViewMouse, (PTRSZVAL)pv );

		pv->MouseMethod = pMC;

		pv->Previous = pMainView;

		pMainView = pv;
		// NULL is OK if it's 3d plugin
		RestoreDisplay( pv->hVideo );
		l.view = pv;
		return pv;
	}
}

PVIEW CreateViewEx( int nType, ViewMouseCallback pMC, TEXTCHAR *Title, int sx, int sy )
{
	return CreateViewWithUpdateLockEx( nType, pMC, Title, sx, sy, NULL );
}

PVIEW CreateView( ViewMouseCallback pMC, TEXTCHAR *Title )
{           	
	return CreateViewEx( V_FORWARD, pMC, Title, 0, 0 );
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

PTRSZVAL CPROC RenderFacet(  POBJECT po
						   , PFACET pf );

void RenderOpenFacet( POBJECT po, PFACET pf )
{
	static POBJECT view_volume;
	static PFACET the_plane;
	static PFACET volume_facets[6];
	PRAY planes;
	int n;
	if( !view_volume )
	{
		view_volume = CreateObject();
		the_plane = AddPlaneToSet( view_volume->objinfo, pf->d.o, pf->d.n, 0 );
		GetViewVolume( &planes );
		for( n = 0; n < 6; n++ )
		{
			volume_facets[n] = AddPlaneToSet( view_volume->objinfo, planes[n].o, planes[n].n, 1 );
			volume_facets[n]->flags.bClipOnly = 1;
		}
	}
	else
	{
		SetRay( &the_plane->d, &pf->d );
		GetViewVolume( &planes );
		for( n = 0; n < 6; n++ )
		{
         SetRay( &volume_facets[n]->d, &planes[n] );
		}
	}
	the_plane->color = SetAlphaValue( pf->color, GetAlphaValue( pf->color ) * 0.25 );
	the_plane->image = pf->image;
	IntersectObjectPlanes( view_volume );
	RenderFacet( view_volume, the_plane );
//      CreateLine(  NULL, planes[n].o, planes[n].n,
}

PTRSZVAL CPROC RenderFacet(  POBJECT po
						   , PFACET pf )
{
	//POBJECT po = (POBJECT)psv;
	//PFACET pf = (PFACET)member;

	RAY rl[2];
	RAY rvl;
	int t, p;
	POBJECT pi; // pIn Tree...
	if( !pf->pLineSet )
	{
		RenderOpenFacet( po, pf );
		return 0;
	}
//#if 0
	if( !pf->flags.bDual )
	{
		RAY r, r1;
		int invert;
		//Invert( r.o );
		ApplyR( po->Ti, &r1, &pf->d );
		//ApplyInverseR( VectorConst_I /*T_camera*/, &r, &r1 ); 
		//Invert( r.o );

		//ApplyRotation( T_camera, r.n, pf->d.n );
		//ApplyTranslation( T_camera, r.o, pf->d.o );
		//r.o[1] = -r.o[1]; // it's like z is backwards.
		//r.o[0] = -r.o[0]; // it's like z is backwards.
		//lprintf( WIDE(" ------- facet normal, origin ") );
		//PrintVector( pf->d.o );
		//PrintVector( pf->d.n );
		//ShowTransform( T_camera );
		//lprintf( WIDE(" --- translated origin, normal...") );
		//PrintVector( r1.o );
		//PrintVector( r1.n );
		//PrintVector( r.o );
		//PrintVector( r.n );
		//lprintf( WIDE(" Corss will be %g"), dotproduct( r.o, r.n ) );
		//ApplyR( po->Ti, &r, &pf->d );
		invert = pf->flags.bInvert;
		if( EditInfo.bEditing && EditInfo.Invert )
			invert = !invert;
		if( po->bInvert )
			invert = !invert;
if( 0 )
{
	// draw line from object to world origiin
	// was a useful debug at one point... can still
	// be used for mast tracking.
		glBegin( GL_LINE_STRIP );
		glColor4ub( 255,255,255,255 );
		//glVertex3fv( GetOrigin( T_camera ) );
		glVertex3fv( GetOrigin( po->Ti ) );
		glVertex3fv( VectorConst_0 );//GetOrigin( T_camera ) );	
		glColor4ub( 255,0,0,255 );
		glVertex3fv( VectorConst_X );//GetOrigin( T_camera ) );	
		glColor4ub( 255,0,255,255 );
		glVertex3fv( VectorConst_Y );//GetOrigin( T_camera ) );	
		glColor4ub( 255,255,0,255 );
		glVertex3fv( VectorConst_Z );//GetOrigin( T_camera ) );	
		glEnd();
}
		if( invert )
		{
			PrintVector( r.n );
			PrintVector( r.o );
			if( dotproduct( r.o, r.n ) > 0 )
			{
				// draw plane normal - option at some point....
				//DrawLine( GetDisplayImage( pv->hVideo ),
				//			 r.o, r.n, 0, 2, Color( 255, 0, 255 ) );
				return 0;
			}
		}
		else
		{
			//PrintVector( r.o );
			//PrintVector( r.n );
			if( dotproduct( r.o, r.n ) < 0 )
			{
				// draw plane normal - option at some point....
				//DrawLine( GetDisplayImage( pv->hVideo ),
				//			 r.o, r.n, 0, 2, Color( 255, 0, 255 ) );
				return 0;
			}
		}
	}
//#endif
	//Log( WIDE("Begin facet...") );
	// now we know we can show this facet...
   if( !pf->flags.bShared )
	{
		int l;
		int lstart = 0;
		int points;
		VECTOR pvPoints[10];
		int normals;
		VECTOR pvNormals[20]; // normals are 2x the points....there's an approach normal and a leave normal.
		VECTOR *v;
		float gl_color[4];
		points = 10;
		normals = 20;
		GetPoints( pf, &points, pvPoints );
		//GetNormals( pf, &normals, pvNormals );
		if(points) // solid fill first...
		{
				float *image_v;
			//glBegin( GL_POLYGON );
			//lprintf( WIDE("glpolygon..") );
			//glColor3f(1.0f,1.0f,0.0f);
			// Set The Color To Yellow
			if( pf->color )
			{
				 gl_color[0] = RedVal( pf->color ) / 255.0f;
				gl_color[1] = GreenVal( pf->color ) / 255.0f;
				gl_color[2] = BlueVal( pf->color ) / 255.0f;
				gl_color[3] = AlphaVal( pf->color ) / 255.0f;	
			}
			else
			{
				 gl_color[0] = RedVal( po->color ) / 255.0f;
				gl_color[1] = GreenVal( po->color ) / 255.0f;
				gl_color[2] = BlueVal( po->color ) / 255.0f;
				gl_color[3] = AlphaVal( po->color ) / 255.0f;	
			}
			v = NewArray( VECTOR, points );

			glColor4ubv( (unsigned char *)&gl_color );
			for( l = 0; l < points; l++ )
			{
				//if( l < normals )
				{
					//Apply( (PCTRANSFORM)po->Ti, v, pvNormals[l] );
					//glNormal3fv( v );
				}

				Apply( (PCTRANSFORM)po->Ti, v[l], pvPoints[l] );
				//SetPoint( pvPoints[l], v );				
				//glVertex3fv( v );
			}
			//glEnd();
			if( pf->image )
			{
				image_v = NewArray( float, 2*points );
				for( l = 0; l < points; l++ )
				{
					image_v[l*2+0] = v[l][0] / pf->image->width;
					image_v[l*2+1] = v[l][2] / pf->image->height;
				}
				lprintf( "Texture with %d points", points );
				ImageEnableShader( ImageGetShader( "Simple Texture", NULL ), v, ReloadTexture( pf->image, 4 ), image_v );
			}
			else
			{
				ImageEnableShader( ImageGetShader( "Simple Shader", NULL ), v, gl_color );
			}
			if( points == 4 )
				glDrawArrays( GL_QUADS, 0, points );
			else
				glDrawArrays( GL_POLYGON, 0, points );
			if( pf->image )
				Deallocate( float *, image_v );
			Release( v );

		}
		//#if defined WIREFRAME
		// do wireframe white line along edge....
		if( points )
		{
			//glBegin( GL_LINE_STRIP );
			//glColor4ub( 255,255,255,255 );
			v = NewArray( VECTOR, points+1 );
			for( l = 0; l < points; l++ )
			{
				//lprintf( WIDE("Vertex..") );
            //PrintVector(pvPoints[l] );
				Apply( (PCTRANSFORM)po->Ti, v[l], pvPoints[l] );
				//glVertex3fv( v[l] );
			}
			Apply( (PCTRANSFORM)po->Ti, v[l], pvPoints[0] );
			//glVertex3fv( v );
			//glEnd();
			gl_color[0] = 1.0;
			gl_color[1] = 1.0;
			gl_color[2] = 1.0;
			gl_color[3] = 1.0;
			ImageEnableShader( ImageGetShader( "Simple Shader", NULL ), v, gl_color);
			glDrawArrays( GL_LINE_STRIP, 0, points+1 );
			Release( v );
		}
#ifdef REPRESENT_OBJECT_FACET_NORMALS
		{
			VECTOR v;
			VECTOR v2;
			glBegin( GL_LINE_STRIP );
			glColor4ub( 255,0,0,255 );
			Apply( (PCTRANSFORM)po->Ti, v, pf->d.o );
			glVertex3fv( v );
			ApplyRotation( (PCTRANSFORM)po->Ti, v2, pf->d.n );
			addscaled( v, v, v2, 20 );
			glVertex3fv( v );
			glEnd();
		}
#endif
		//#endif
	}
return 0;
}

static int EvalExcept( int n )
{
	switch( n )
	{
	case 		STATUS_ACCESS_VIOLATION:
		lprintf( WIDE("Access violation - OpenGL layer at this moment..") );
		return EXCEPTION_EXECUTE_HANDLER;
	default:
		lprintf( WIDE("Filter unknown : %d"), n );

		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}
void ShowObjectChildren( POBJECT po )
{
   POBJECT pCurObj;
   if( !po )
		return;
   FORALLOBJ( po, pCurObj )
   {
		{
			INDEX idx;
			PFACET facet;
			//TRANSFORM T_tmp;
			PLIST list = pCurObj->objinfo->facets;
			//ClearTransform( &T_tmp );
			//ApplyInverseT( T_camera, &T_tmp, po->Ti );
			//ApplyTranslationT( T_camera, &T_tmp, po->Ti );
			//ApplyRotationT( T_camera, &T_tmp, po->Ti );
			//PrintVector( GetOrigin( &T_tmp ) );
			//lprintf( WIDE("Render object %p"), pCurObj );
			LIST_FORALL( list, idx, PFACET, facet )
			{
				//lprintf( WIDE("Render facet %d(%p)"), idx, facet );
				//DebugBreak();
#ifndef __cplusplus
#ifdef MSC_VER
				__try 
				{
#endif
#endif
						RenderFacet( pCurObj, facet );
#ifndef __cplusplus
#ifdef MSC_VER
				}
				__except( EvalExcept( GetExceptionCode() ) )
				{
					lprintf( WIDE(" ...") );
				}
#endif
#endif
			}
		}
		if( pCurObj->pHolds )
		{
			//lprintf( WIDE("Show holds"));
			ShowObjectChildren( pCurObj->pHolds );
		}
		if( pCurObj->pHas )
		{
			//lprintf( WIDE("Show has") );
			ShowObjectChildren( pCurObj->pHas );
		}
	}
}

void ShowObjects( )
{
	MATRIX m;
	POBJECT po;
#ifndef __cplusplus
#ifdef MSC_VER
	__try
	{
#endif
#endif
		/*
		if( pv->csUpdate )
		{
			lprintf( WIDE("Grab update section..") );
			EnterCriticalSec( pv->csUpdate );
			lprintf( WIDE("got it.") );
		}
		*/
		if(0)
		{
			float lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};    //red diffuse <==> object has a red color everywhere
			float lightAmbient[] = {1.0f, 1.0f, 1.0f, 1.0f};    //yellow ambient <==> yellow color where light hit directly the object's surface
			float lightPosition[]= {0.0f, 0.0f, -7.0f, 1.0f};
			
				//Ambient light component
				glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
			//Diffuse light component
			glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
				//Light position
				glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
			
				//Enable the first light and the lighting mode
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
}
   po = (POBJECT)pFirstObject; // some object..........
   while( po && ( po->pIn || po->pOn ) ) // go to TOP of tree...
   {
      if( po->pIn )
         po = po->pIn;
      else if( po->pOn )
         po = po->pOn;
	}

	ShowObjectChildren( po );  // all children...
#ifndef __cplusplus
#ifdef MSC_VER
	}
	  					__except( EvalExcept( GetExceptionCode() ) )
					{
						lprintf( WIDE(" ...") );
						;
					}
#endif
#endif
	//WriteToWindow( pv->hVideo, 0, 0, 0, 0 );
	//lprintf( WIDE("...") );
	//if( pv->csUpdate )
	//	LeaveCriticalSec( pv->csUpdate );
}



void DestroyView( PVIEW pv )
{
	if( pv )
	{
		if( pv->hVideo )  // could already be closed...
			CloseDisplay( pv->hVideo );
		Release( pv );   
	}
}

void MoveView( PVIEW pv, PCVECTOR v )
{
	//TranslateV( pv->T, v );
}
