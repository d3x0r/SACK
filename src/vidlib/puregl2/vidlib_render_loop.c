#define NO_UNICODE_C
#define FIX_RELEASE_COM_COLLISION


#include <stdhdrs.h>
#include "local.h"

RENDER_NAMESPACE



void ogl_Redraw( PVIDEO hVideo )
{
	if( hVideo )
		hVideo->flags.bUpdated = 1;
	else
	{
		l.flags.bUpdateWanted = 1;
		if( l.wake_callback )
			l.wake_callback();
	}
}



void MygluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
#define m l.fProjection
	GLfloat sine, cotangent, deltaZ;
	GLfloat radians=(GLfloat)(fovy/2.0f*M_PI/180.0f);

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
}


void WantRender3D( void )
{
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Want Render" );

	{
		//PRENDERER other = NULL;
		PRENDERER hVideo;
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			//other = hVideo;
			if( l.flags.bLogWrites )
				lprintf( "Have a video in stack..." );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogWrites )
					lprintf( "But it's not exposed..." );
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

void Render3D( struct display_camera *camera )
{
	INDEX idx;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Render" );

	if( camera->flags.first_draw )
	{
		struct plugin_reference *reference;
		//lprintf( "camera is in first_draw..." );
		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			//lprintf( "so reset plugin... is there one?" );
			reference->flags.did_first_draw = 0;
		}
	}

  	l.current_render_camera = camera;
	l.flags.bViewVolumeUpdated = 1;
#ifdef __3D__
	Init3D( camera );
#endif
	//lprintf( "Called init for camera.." );
	{
		PRENDERER hVideo = camera->hVidCore;
		ApplyTranslationT( VectorConst_I, camera->origin_camera, l.origin );

		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			//lprintf( "Check plugin on camera %p %p", reference, camera );
			if( !reference->flags.did_first_draw )
			{
				//lprintf( "first draw..." );
				first_draw = 1;
				reference->flags.did_first_draw = 1;
			}
			else
			{
				//lprintf( "skipping first draw" );
				first_draw = 0;
			}

			// setup initial state, like every time so it's a known state?
			{
				// copy l.origin to the camera

				if( first_draw )
				{
					if( l.flags.bLogRenderTiming )
						lprintf( "Send first draw" );
					if( reference->FirstDraw3d )
						reference->FirstDraw3d( reference->psv );
				}
				if( reference->ExtraDraw3d )
				{
					reference->ExtraDraw3d( reference->psv, camera->origin_camera );
				}
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
         // forward; this is default mode... no-op
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
		l.flags.bViewVolumeUpdated = 1;

#ifdef __3D__
		SetupPositionMatrix( camera );
#endif

		if( l.flags.bLogRenderTiming )
			lprintf( "Begin drawing from bottom up" );
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( l.flags.bLogRenderTiming )
				lprintf( "Have a video in stack... %p", hVideo );
			hVideo->flags.bRendering = 1;
			if( hVideo->flags.bDestroy )
			{
				hVideo->flags.bRendering = 0;
				continue;
			}
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogWrites )
					lprintf( "But it's not exposed..." );
				hVideo->flags.bRendering = 0;
				continue;
			}

			hVideo->flags.bUpdated = 0;

			if( l.flags.bLogWrites )
				lprintf( "------ BEGIN A REAL DRAW -----------" );

#ifdef _OPENGL_DRIVER
//#if 0
			glEnable( GL_DEPTH_TEST );
			// put out a black rectangle
			// should clear stensil buffer here so we can do remaining drawing only on polygon that's visible.
			ClearImageTo( hVideo->pImage, 0 );
//#endif
			glDisable(GL_DEPTH_TEST);							// Enables Depth Testing

#endif
#ifdef _D3D_DRIVER
#if 0
			Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 1 );
			ClearImageTo( hVideo->pImage, 0 );
#endif
			Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 0 );
#endif
#ifdef _D3D10_DRIVER
#if 0
			//Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 1 );
			//ClearImageTo( hVideo->pImage, 0 );
#endif
			//Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 0 );
#endif

			if( hVideo->pRedrawCallback )
			{
				hVideo->pRedrawCallback( hVideo->dwRedrawData, (PRENDERER)hVideo );
			}

#ifdef _OPENGL_DRIVER
			// allow draw3d code to assume depth testing 
			glEnable( GL_DEPTH_TEST );
#endif
#ifdef _D3D_DRIVER
			Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 1 );
#endif
#ifdef _D3D10_DRIVER
			//Render3d.current_device->SetRenderState( D3DRS_ZENABLE, 1 );
#endif
			{
				INDEX idx;
				PSPRITE_METHOD psm;
				LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
				{
					psm->RenderSprites( psm->psv, hVideo, 0, 0, 0, 0 );
				}
			}
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
	EndActive3D( camera );
}

void drawCamera( struct display_camera *camera )
{
   // skip the 'default' camera.
	// if plugins or want update, don't continue.
	if( !camera->plugins && !l.flags.bUpdateWanted )
	{
		if( l.flags.bLogRenderTiming )
			lprintf( "nothing to do..." );
		return;
	}
	if( !camera->hVidCore || !camera->hVidCore->flags.bReady )
	{
		if( l.flags.bLogRenderTiming )
			lprintf( "Open Camera..." );
		ogl_OpenCamera( camera );
	}
	if( !camera->hVidCore || !camera->hVidCore->flags.bReady )
	{
		if( l.flags.bLogRenderTiming )
			lprintf( "not ready to draw camera.... %p", camera->hVidCore );
		return;
	}
	if( camera->flags.first_draw )
	{
		struct plugin_reference *reference;
		INDEX idx;
		if( l.flags.bLogRenderTiming )
			lprintf( "camera is in first_draw..." );
		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			//lprintf( "so reset plugin... is there one?" );
			reference->flags.did_first_draw = 0;
		}
	}
	if( l.flags.bLogRenderTiming )
		lprintf( "Render camera %p", camera );
	// drawing may cause subsequent draws; so clear this first
	Render3D( camera );
	camera->flags.first_draw = 0;
}


LOGICAL ProcessGLDraw( LOGICAL draw_all )
{
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Draw Tick" );
	Move( l.origin );

	{
		INDEX idx;
		Update3dProc proc;
		if( l.flags.bLogRenderTiming )
			lprintf( "check plugin updates" );
		LIST_FORALL( l.update, idx, Update3dProc, proc )
		{
			//lprintf( "Calling 3d callback...%p", proc );
			if( proc( l.origin ) )
			{
				//lprintf( "update call indicates it would like to draw..." );
				l.flags.bUpdateWanted = TRUE;
			}
		}
	}

	// no reason to check this if an update is already wanted.
	if( !l.flags.bUpdateWanted )
	{
		if( l.flags.bLogRenderTiming )
			lprintf( "Check if a render surface wants to draw..." );
		// set l.flags.bUpdateWanted for window surfaces.
		WantRender3D();
	}
	else
		if( l.flags.bLogRenderTiming )
			lprintf( "Update is wanted already..." );

	if( draw_all || l.flags.bUpdateWanted )
	{
		struct display_camera *camera;
		INDEX idx;
		LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
		{
			// skip the 'default' camera.
			if( !idx )
				continue;
			if( l.flags.bLogRenderTiming )
				lprintf( "draw a camera %p", camera );
			drawCamera( camera );
		}
		l.flags.bUpdateWanted = 0;
	}
	return l.flags.bUpdateWanted || l.flags.bRotateLock;
}



RENDER_NAMESPACE_END
