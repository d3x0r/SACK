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

// This module will display an object on the screen
// or maybe just a polygon
// because an object may be a BSP tree which describes
// a level, or it may be an object definition.
// In either case a higher resolution screen should merely
// add detail, not be able to project more of the scene.
// The camera could also have a 'periferal' view along the
// edges of the scene viewed.


PRELOAD( InitLocal )
{
	g.pii = GetImageInterface();
	g.pri = GetDisplayInterface();
	g.pr3i = GetRender3dInterface();
	g.pi3i = GetImage3dInterface();
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
	ApplyR( (PCTRANSFORM)g.EditInfo.TEdit, out, &in );
}

static LOGICAL OnKey3d( WIDE("Virtuality") )( PTRSZVAL psvView, _32 key )
{
	VIEW *v = (VIEW*)psvView;
	int used = 0;
	int SetChanged;
		SetChanged = FALSE;	
	if( !IsKeyPressed( key ) )
		return FALSE;
	if( KeyDown(  NULL, KEY_E ) )
	{
		g.EditInfo.bEditing = !g.EditInfo.bEditing;
		if( g.EditInfo.bEditing )
		{
			g.EditInfo.pEditObject = (POBJECT)pFirstObject;
			g.EditInfo.nFacetSet = 0;
			g.EditInfo.nFacet = 0;
			if( !g.EditInfo.TEdit )
			{
				g.EditInfo.TEdit = CreateTransform();
				CreateTransformMotion( g.EditInfo.TEdit );
			}
			used = 1;
      	}
      }
      if( g.EditInfo.bEditing )
      {
		  // next object
         if( KeyDown(  NULL, KEY_O ) )
         {
         	// change editing object
         	g.EditInfo.pEditObject = NextLink( g.EditInfo.pEditObject );
         	if( !g.EditInfo.pEditObject )
         	   g.EditInfo.pEditObject = (POBJECT)pFirstObject;
         	g.EditInfo.nFacetSet = 0;
         	g.EditInfo.nFacet = 0;
			used = 1;
         	SetChanged = TRUE;
         }
      // next facet set 
         if( KeyDown( NULL, KEY_S ) )
         {
				g.EditInfo.nFacetSet++;
#if 0
         	if( g.EditInfo.nFacetSet >= g.EditInfo.pEditObject->objinfo->FacetSetPool.nUsedFacetSets )
         		g.EditInfo.nFacetSet = 0;	
				g.EditInfo.nFacet = 0;
#endif
				used = 1;
        		SetChanged = TRUE;
         }
		 // Next facet
         if( KeyDown( NULL, KEY_F ) )
         {
			g.EditInfo.nFacet++;
			g.EditInfo.pFacet = (PFACET)GetLink( &g.EditInfo.pEditObject->objinfo->facets,
				g.EditInfo.nFacet );
			if( !g.EditInfo.pFacet )
			{
				g.EditInfo.nFacet = 0;
				g.EditInfo.pFacet = (PFACET)GetLink( &g.EditInfo.pEditObject->objinfo->facets, g.EditInfo.nFacet );
			}

			if( g.EditInfo.pFacet )
			{
				VECTOR other_axis;
				TranslateV( g.EditInfo.TEdit, g.EditInfo.pFacet->d.o );
				other_axis[vForward] = g.EditInfo.pFacet->d.n[vForward];
				other_axis[vRight] = g.EditInfo.pFacet->d.n[vUp];
				other_axis[vUp] = -g.EditInfo.pFacet->d.n[vRight];
				RotateTo( g.EditInfo.TEdit, g.EditInfo.pFacet->d.n, other_axis );
			}
			used = 1;
		}
		//Invert Plane
		if( KeyDown( NULL, KEY_I ) )
		{
				g.EditInfo.Invert = !g.EditInfo.Invert;
				used = 1;
		}

		 // New Plane; this does more than it should....
		if( 0 &&  KeyDown(  NULL, KEY_N ) )
		{
			int nfs, nf;
			PFACET pf;
				//nfs = GetFacetSet( &g.EditInfo.pEditObject->objinfo );
			pf = GetEditFacet();
			nf = GetEditFacetIndex();
			if( !pf )
				return FALSE;
			pf->flags.bInvert = TRUE;

				{
					int l;
					PLINESEGPSET *pplps = &pf->pLineSet;
					int lines = CountUsedInSet( LINESEGP, pf->pLineSet );
					for( l = 0; l < lines; l++ )
					{
						VECTOR n, ln;
						PLINESEGP plsp = GetSetMember( LINESEGP, pplps, l );
						PMYLINESEG line = plsp->pLine;
						if( !line )
							continue;
						SetPoint( ln, line->l.r.n );
						if( plsp->bOrderFromTo  )
							Invert( ln );
						crossproduct( n, pf->d.n, ln );
				   		AddPlaneToSet( g.EditInfo.pEditObject->objinfo, line->l.r.o
				   					  , n, 1 );
				   }
				}
				//g.EditInfo.nFacetSet = nfs;
				g.EditInfo.nFacet = nf;
			}
#if 0
			if( SetChanged )
			{
				PFACET pf;
				PLINESEGP plsp = GetSetMember( LINESEGP, &pf->pLineSet, 0 );
				PMYLINESEG line = GetSetMember( MYLINESEG, &g.EditInfo.pEditObject->objinfo.LinePool, plsp->nLine );
				pf = GetEditFacet();
				RotateTo( g.EditInfo.TEdit, pf->d.n
						  , line->r.n );
				TranslateV( g.EditInfo.TEdit, pf->d.o );
			}
#endif
		}
	return used;
}

POBJECT TestMouseObject( POBJECT po, PRAY mouse, PFACET *face, PVECTOR vmin )
{
	LOGICAL status;
	POBJECT sub;
	POBJECT result = NULL;
	for( ; po; po = po->next )
	{
		if( po->pHolds )
		{
			sub = TestMouseObject( po->pHolds, mouse, face, vmin );
			if( sub )
				result = sub;
		}
		if( po->pHas )
		{
			sub = TestMouseObject( po->pHas, mouse, face, vmin );
			if( sub )
				result = sub;
		}
		status = ComputeRayIntersectObject( po, mouse, face, vmin );
		if( status )
		{
			result = po;
		}
	}
	return result;
}


static LOGICAL OnMouse3d( WIDE("Virtuality") )( PTRSZVAL psvView, PRAY mouse, _32 b )
//int CPROC ViewMouse( PTRSZVAL dwView, S_32 x, S_32 y, _32 b )
{
   VIEW *v = (VIEW*)psvView;
/*
			VECTOR m, b;
			// rotate world into view coordinates... mouse is void(0) coordinates...

			//ApplyInverse( View->T, b, mouse_origin );
			//ApplyInverseRotation( View->T, m, mouse_vforward );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f );
			//ApplyInverseRotation( View->T, m, mouse_vright );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f00 );
			//ApplyInverseRotation( View->T, m, mouse_vup );
			//DrawLine( GetDisplayImage( View->hVideo ), b, m, 0, 10, 0x7f0000 );
  */

	if( g.EditInfo.bEditing )
	{
		PFACET new_facet = NULL;
		VECTOR vmin;
		POBJECT new_object;
		new_object = TestMouseObject( g.pFirstRootObject, mouse, &new_facet, vmin );
		if( new_object )
		{
			g.EditInfo.pEditObject = new_object;
			g.EditInfo.pFacet = new_facet;
			{
				VECTOR other_axis;
				TranslateV( g.EditInfo.TEdit, g.EditInfo.pFacet->d.o );
				other_axis[vForward] = g.EditInfo.pFacet->d.n[vForward];
				other_axis[vRight] = g.EditInfo.pFacet->d.n[vUp];
				other_axis[vUp] = -g.EditInfo.pFacet->d.n[vRight];
				RotateTo( g.EditInfo.TEdit, g.EditInfo.pFacet->d.n, other_axis );
			}

		}
		return 1;
	}

	return 0;
}

int bDump;



int frames;
int time;

static void UpdateObject( POBJECT po )
{
	POBJECT pCurObj;
	if( !po )
		return;
	FORALLOBJ( po, pCurObj )
	{
		Move( pCurObj->Ti );
		if( pCurObj->pHolds )
		{
			//lprintf( WIDE("Show holds"));
			UpdateObject( pCurObj->pHolds );
		}
		if( pCurObj->pHas )
		{
			//lprintf( WIDE("Show has") );
			UpdateObject( pCurObj->pHas );
		}
   }
}

static void UpdateObjects( void )
{

	{
		POBJECT po = (POBJECT)pFirstObject; // some object..........
		while( po && ( po->pIn || po->pOn ) ) // go to TOP of tree...
		{
			if( po->pIn )
				po = po->pIn;
			else if( po->pOn )
				po = po->pOn;
		}
		UpdateObject( po );
	}
}

static LOGICAL OnUpdate3d( WIDE( "Virtuality" ) )( PTRANSFORM origin )
{
	{
		static VECTOR KeySpeed, KeyRotation;
		VECTOR ks, kr;
		if( !time )
			time = timeGetTime();

		// scan the keyboard, cause ... well ... it needs 
		// scaled keyspeed and acceleration ticks.
		ScanKeyboard( NULL, KeySpeed, KeyRotation );

		if( !g.EditInfo.bEditing || IsKeyDown(  NULL, KEY_CONTROL ) )
		{
			SetSpeed( origin, KeySpeed );
			SetRotation( origin, KeyRotation );
			Move(origin);  // relative rotation...

			//lprintf( WIDE("Updated main view transform.") );
			//ShowTransform(pMainView->Tglobal );
		}
		else if( g.EditInfo.bEditing ) // editing without control key pressed
		{
			if( g.EditInfo.pFacet )
			{
				ks[vRight] = 0;
				ks[vUp] = 0;
				kr[vForward] = 0;
				if( Length( KeyRotation ) || Length( KeySpeed ) )
				{
					SetSpeed( g.EditInfo.TEdit, KeySpeed );
					SetRotation( g.EditInfo.TEdit, KeyRotation );
					Move( g.EditInfo.TEdit );
					ComputePlaneRay( &g.EditInfo.pFacet->d );
					IntersectPlanes( g.EditInfo.pEditObject->objinfo, g.EditInfo.pEditObject->pIn?g.EditInfo.pEditObject->pIn->objinfo:NULL, TRUE );
				}
			}
		}
	}
	UpdateObjects();
	return TRUE;
}

static void OnDraw3d( WIDE("Virtuality") )( PTRSZVAL psvUnusedOne )
{
	// cannot count that the camera state is relative for how we want to show?

	ShowObjects(  );

		if( g.EditInfo.bEditing )
		{
			TEXTCHAR buf[256];
		  	snprintf( buf, 256, WIDE("Editing: O: %08x FS:%d F:%d")
      					, g.EditInfo.pEditObject
      					, g.EditInfo.nFacetSet
      					, g.EditInfo.nFacet );
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
				if( pf )
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







void CPROC CloseView( PTRSZVAL dwView )
{
   VIEW *V;
   V = (VIEW*)dwView;
   V->hVideo = (PRENDERER)NULL; // release from window side....
}


static PTRSZVAL OnInit3d( WIDE("Virtuality") )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	g.camera = camera;
	return (PTRSZVAL)1;
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

	if( !pf->flags.bDual )
	{
		int invert;
		ApplyR( po->Ti, &rvl, &pf->d );
		invert = pf->flags.bInvert;
		if( g.EditInfo.bEditing && g.EditInfo.Invert )
			invert = !invert;
		if( po->bInvert )
			invert = !invert;

		if( invert )
		{
			// invert is to show if is inside.
			// normal view is from the outside (not this)
			// projection is very wide because back planes can extend far
			if( dotproduct( rvl.n, GetAxis( g.camera, vForward ) ) < -0.507 )
			{
				// draw plane normal - option at some point....
				//DrawLine( GetDisplayImage( pv->hVideo ),
				//			 r.o, r.n, 0, 2, Color( 255, 0, 255 ) );
				return 0;
			}
		}
		else
		{
			// normal view is from the outside (this)
			// projection is very wide because back planes can extend far
			if( dotproduct( rvl.n, GetAxis( g.camera, vForward ) ) > 0.507 )
			{
				// draw plane normal - option at some point....
				//DrawLine( GetDisplayImage( pv->hVideo ),
				//			 r.o, r.n, 0, 2, Color( 255, 0, 255 ) );
				return 0;
			}
		}
	}

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
			if( g.EditInfo.bEditing )
			{
				if( po == g.EditInfo.pEditObject )
				{
					if( pf == g.EditInfo.pFacet )
					{
						// yellow?
						gl_color[0] = 0.81f;
						gl_color[1] = 0.61f;
						gl_color[2] = 0.10f;
						gl_color[3] = 1.0f;
					}
					else
					{
						// cyan
						gl_color[0] = 0.31f;
						gl_color[1] = 0.61f;
						gl_color[2] = 1.0f;
						gl_color[3] = 1.0f;
					}
				}
				else
				{
					// blue basically
					gl_color[0] = 0.01f;
					gl_color[1] = 0.31f;
					gl_color[2] = 1.0f;
					gl_color[3] = 1.0f;
				}
			}
			else
			{
				// white
				gl_color[0] = 1.0;
				gl_color[1] = 1.0;
				gl_color[2] = 1.0;
				gl_color[3] = 1.0;
			}
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

