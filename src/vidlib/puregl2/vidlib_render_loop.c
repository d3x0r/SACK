#define NO_UNICODE_C
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
#include "local.h"

RENDER_NAMESPACE


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
#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER )
    m[2][2] = (zFar + zNear) / deltaZ;
    m[2][3] = 1.0f;
    m[3][2] = -1.0f * zNear * zFar / deltaZ;
#else
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1.0f;
	 m[3][2] = -2.0f * zNear * zFar / deltaZ;
#endif
	 m[3][3] = 0;
#undef m
    //glMultMatrixf(&m[0][0]);
}


void WantRender3D( void )
{
	struct plugin_reference *reference;
	if( l.flags.bLogRenderTiming )
		lprintf( WIDE("Begin Render") );

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

void Render3D( struct display_camera *camera )
{
	INDEX idx;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( WIDE("Begin Render") );


	// do OpenGL Frame
#ifdef _OPENGL_DRIVER
	SetActiveGLDisplay( camera->hVidCore );
	InitGL( camera );
#endif
#ifdef _D3D_DRIVER
	if( !SetActiveD3DDisplay( camera->hVidCore ) )  // BeginScene()
		return;

	InitD3D( camera );
	Render3d.current_device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER
											, D3DCOLOR_XRGB(0, 40, 100)
											, 1.0f
											, 0);
#endif
#ifdef _D3D10_DRIVER
	if( !SetActiveD3DDisplay( camera->hVidCore ) )  // BeginScene()
		return;

	InitD3D( camera );
	float pBackgroundColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	Render3d.current_device->ClearRenderTargetView( Render3d.current_target, pBackgroundColour);
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
						lprintf( WIDE("Send first draw") );
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
#ifdef _D3D_DRIVER
		{
			PC_POINT tmp = GetAxis( camera->origin_camera, 0 );
			{
				D3DXMATRIX out;
				D3DXVECTOR3 eye(tmp[12], tmp[13], tmp[14]);
				D3DXVECTOR3 at(tmp[8], tmp[9], tmp[10]);
				D3DXVECTOR3 up(tmp[4], tmp[5], tmp[6] );
				at += eye;
				D3DXMatrixLookAtLH(&out, &eye, &at, &up);
				Render3d.current_device->SetTransform( D3DTS_WORLD, &out );
			}
		}
         /* some kinda init; no? */
		Render3d.current_device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
		Render3d.current_device->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
		Render3d.current_device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		Render3d.current_device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		Render3d.current_device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);

		Render3d.current_device->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFFF );
		Render3d.current_device->SetRenderState( D3DRS_LIGHTING,FALSE);
		Render3d.current_device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
		Render3d.current_device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
#endif

#ifdef _D3D10_DRIVER
		{
			PC_POINT tmp = GetAxis( camera->origin_camera, 0 );
			{
				D3DXMATRIX out;
				D3DXVECTOR3 eye(tmp[12], tmp[13], tmp[14]);
				D3DXVECTOR3 at(tmp[8], tmp[9], tmp[10]);
				D3DXVECTOR3 up(tmp[4], tmp[5], tmp[6] );
				at += eye;
				//D3DXMatrixLookAtLH(&out, &eye, &at, &up);
				//Remder3d.current_device->SetTransform( D3DTS_WORLD, &out );
			}
		}

		static ID3D10BlendState* g_pBlendState = NULL;
		if( g_pBlendState )
		{
 
			D3D10_BLEND_DESC BlendState;
			ZeroMemory(&BlendState, sizeof(D3D10_BLEND_DESC));
 
			BlendState.BlendEnable[0] = TRUE;
			BlendState.SrcBlend = D3D10_BLEND_SRC_ALPHA;
			BlendState.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
			BlendState.BlendOp = D3D10_BLEND_OP_ADD;
			BlendState.SrcBlendAlpha = D3D10_BLEND_ZERO;
			BlendState.DestBlendAlpha = D3D10_BLEND_ZERO;
			BlendState.BlendOpAlpha = D3D10_BLEND_OP_ADD;
			BlendState.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
 
			Render3d.current_device->CreateBlendState(&BlendState, &g_pBlendState);
		}


      /* some kinda init; no? */
		Render3d.current_device->OMSetBlendState( g_pBlendState, 0, 0xffffffff);
#endif

#ifdef _D3D11_DRIVER
		{
			PC_POINT tmp = GetAxis( camera->origin_camera, 0 );
			{
				//D3DXMATRIX out;
				//D3DXVECTOR3 eye(tmp[12], tmp[13], tmp[14]);
				//D3DXVECTOR3 at(tmp[8], tmp[9], tmp[10]);
				//D3DXVECTOR3 up(tmp[4], tmp[5], tmp[6] );
				//at += eye;
				//D3DXMatrixLookAtLH(&out, &eye, &at, &up);
				//Remder3d.current_device->SetTransform( D3DTS_WORLD, &out );
			}
		}

		static ID3D11BlendState* g_pBlendState = NULL;
		if( g_pBlendState )
		{
#if 0
			D3D11_BLEND_DESC BlendState;
			ZeroMemory(&BlendState, sizeof(D3D11_BLEND_DESC));
 
			BlendState.BlendEnable[0] = TRUE;
			BlendState.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			BlendState.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			BlendState.BlendOp = D3D11_BLEND_OP_ADD;
			BlendState.SrcBlendAlpha = D3D11_BLEND_ZERO;
			BlendState.DestBlendAlpha = D3D11_BLEND_ZERO;
			BlendState.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			BlendState.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
 
			Render3d.current_device->CreateBlendState(&BlendState, &g_pBlendState);
#endif
		}


      /* some kinda init; no? */
	  // Render3d.current_device->OMSetBlendState( g_pBlendState, 0, 0xffffffff);
#endif

		if( l.flags.bLogRenderTiming )
			lprintf( WIDE("Begin drawing from bottom up") );
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

#ifdef _OPENGL_DRIVER

#if 0
			glEnable( GL_DEPTH_TEST );
			// put out a black rectangle
			// should clear stensil buffer here so we can do remaining drawing only on polygon that's visible.
			ClearImageTo( hVideo->pImage, 0 );
#endif
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
			lprintf( WIDE("Begin Render (plugins)") );

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
			lprintf( WIDE("End Render") );
	}
#ifdef _OPENGL_DRIVER
	SetActiveGLDisplay( NULL );
#endif
#ifdef _D3D_DRIVER
	SetActiveD3DDisplay( NULL ); // EndScene
#endif
#ifdef _D3D10_DRIVER
	Render3d.current_chain->Present(0, 0);
#endif
}




RENDER_NAMESPACE_END
