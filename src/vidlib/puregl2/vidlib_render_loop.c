
#include <stdhdrs.h>
#include "local.h"




int IsVidThread( void )
{
   // used by opengl to allow selecting context.
	if( IsThisThread( l.actual_thread ) )
		return TRUE;
	return FALSE;
}

void Redraw( PVIDEO hVideo )
{
	if( hVideo )
		hVideo->flags.bUpdated = 1;
	else
		l.flags.bUpdateWanted = 1;
}


#define __glPi 3.14159265358979323846

void MygluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
#define m l.fProjection
    //GLfloat m[4][4];
    GLfloat sine, cotangent, deltaZ;
    GLfloat radians=(GLfloat)(fovy/2.0f*__glPi/180.0f);

    /*m[0][0] = 1.0f;*/ m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
    m[1][0] = 0.0f; /*m[1][1] = 1.0f;*/ m[1][2] = 0.0f; m[1][3] = 0.0f;
    m[2][0] = 0.0f; m[2][1] = 0.0f; /*m[2][2] = 1.0f; m[2][3] = 0.0f;*/
	 m[3][0] = 0.0f; m[3][1] = 0.0f; /*m[3][2] = 0.0f; m[3][3] = 1.0f;*/

    deltaZ=zFar-zNear;
    sine=(GLfloat)sin(radians);
    if ((deltaZ==0.0f) || (sine==0.0f) || (aspect==0.0f))
    {
        return;
    }
    cotangent=(GLfloat)(cos(radians)/sine);

    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1.0f;
    m[3][2] = -2.0f * zNear * zFar / deltaZ;
	 m[3][3] = 0;
#undef m
    //glMultMatrixf(&m[0][0]);
}


void WantRenderGL( void )
{
	INDEX idx;
	PRENDERER hVideo;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Render" );

	{
		PRENDERER other = NULL;
		PRENDERER hVideo;
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( other == hVideo )
				DebugBreak();
			other = hVideo;
			if( l.flags.bLogWrites )
				lprintf( WIDE("Have a video in stack...") );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogWrites )
					lprintf( WIDE("But it's not exposed...") );
				continue;
			}
			if( hVideo->flags.bUpdated )
			{
				// any one window with an update draws all.
				l.flags.bUpdateWanted = 1;
				break;
			}
		}
	}
}

void RenderGL( struct display_camera *camera )
{
	INDEX idx;
	PRENDERER hVideo;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Render" );


	// do OpenGL Frame

	InitGL( camera );
	//lprintf( "Called init for camera.." );
	{
		PRENDERER hVideo = camera->hVidCore;

		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			if( !reference->flags.did_first_draw )
			{
				first_draw = 1;
				reference->flags.did_first_draw = 1;
			}
			else
				first_draw = 0;

			// setup initial state, like every time so it's a known state?
			{
				// copy l.origin to the camera
				ApplyTranslationT( VectorConst_I, camera->origin_camera, l.origin );

				if( first_draw )
				{
					if( l.flags.bLogRenderTiming )
						lprintf( "Send first draw" );
					if( reference->FirstDraw3d )
						reference->FirstDraw3d( reference->psv );
				}
				if( reference->ExtraDraw3d )
					reference->ExtraDraw3d( reference->psv, camera->origin_camera );
			}
		}

		switch( camera->type )
		{
		case 0:
			RotateRight( camera->origin_camera, vRight, vForward );
			break;
		case 1:
			RotateRight( camera->origin_camera, vForward, vUp );
			break;
		case 2:
			break;
		case 3:
			RotateRight( camera->origin_camera, vUp, vForward );
			break;
		case 4:
			RotateRight( camera->origin_camera, vForward, vRight );
			break;
		case 5:
			RotateRight( camera->origin_camera, -1, -1 );
			break;
		}
	
		if( l.flags.bLogRenderTiming )
			lprintf( "Begin drawing from bottom up" );
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( l.flags.bLogRenderTiming )
				lprintf( WIDE("Have a video in stack... %p"), hVideo );
			hVideo->flags.bRendering = 1;
			if( hVideo->flags.bDestroy )
			{
				hVideo->flags.bRendering = 0;
				continue;
			}
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogWrites )
					lprintf( WIDE("But it's not exposed...") );
				hVideo->flags.bRendering = 0;
				continue;
			}

			hVideo->flags.bUpdated = 0;

			if( l.flags.bLogWrites )
				lprintf( WIDE("------ BEGIN A REAL DRAW -----------") );
#if 0
			glEnable( GL_DEPTH_TEST );
			// put out a black rectangle
			// should clear stensil buffer here so we can do remaining drawing only on polygon that's visible.
			ClearImageTo( hVideo->pImage, 0 );
#endif
			glDisable(GL_DEPTH_TEST);							// Enables Depth Testing
			if( hVideo->pRedrawCallback )
			{
				hVideo->pRedrawCallback( hVideo->dwRedrawData, (PRENDERER)hVideo );
			}

			// allow draw3d code to assume depth testing 
			glEnable( GL_DEPTH_TEST );
			hVideo->flags.bRendering = 0;
		}
		if( l.flags.bLogRenderTiming )
			lprintf( "Begin Render (plugins)" );

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				if( ref->Draw3d )
					ref->Draw3d( ref->psv );
			}
		}
		if( l.flags.bLogRenderTiming )
			lprintf( "End Render" );
	}
}





