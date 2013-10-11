#ifndef UNDER_CE
#define NEED_REAL_IMAGE_STRUCTURE

#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
//#include <windows.h>    // Header File For Windows
#include <sharemem.h>
#include <timers.h>
#include <logging.h>
#include <vectlib.h>



#include <imglib/imagestruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>
#include <render3d.h>
//#define LOG_OPENGL_CONTEXT
//#define ENABLE_ON_DC_LAYERED hDCFakeWindow
//#define ENABLE_ON_DC_LAYERED hDCFakeWindow
#define ENABLE_ON_DC_NORMAL hDCOutput

//#define ENABLE_ON_DC hDCBitmap
//#define ENABLE_ON_DC hDCOutput
//hDCBitmap

#include "local.h"
RENDER_NAMESPACE



void KillGLWindow( void )
{
   // do something here...
}

extern RENDER3D_INTERFACE Render3d;


int SetActiveD3DDisplayView( PVIDEO hVideo, int nFracture )
{
	static CRITICALSECTION cs;
	static PVIDEO _hVideo; // last display with a lock.
	if( hVideo )
	{
		EnterCriticalSec( &cs );
		EnterCriticalSec( &hVideo->cs );
		if( nFracture )
		{
			nFracture -= 1;
		}
		else
		{
			_hVideo = hVideo;
			Render3d.current_device = hVideo->camera->device;
			if( ! hVideo->camera->device )
			{
				LeaveCriticalSec( &cs );
				return FALSE;
			}
		}
	}
	else
	{
		if( _hVideo )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( "Prior GL Context being released." );
#endif
			//lprintf( "swapping buffer..." );
			//Render3d.current_chain->Present(0, 0);
			//Render3d.current_device = NULL;
		}
		LeaveCriticalSec( &cs );
	}
	return TRUE;
}

int SetActiveD3DDisplay( PVIDEO hVideo )
{
   return SetActiveD3DDisplayView( hVideo, 0 );
}

//----------------------------------------------------------------------------

static int CreatePartialDrawingSurface (PVIDEO hVideo, int x, int y, int w, int h )
{
	int nFracture = -1;
	// can use handle from memory allocation level.....
	if (!hVideo)         // wait......
		return FALSE;
	//lprintf( WIDE("And here I might want to update the video, hope someone else does for me.") );
	return nFracture + 1;
}



#define MODE_UNKNOWN 0
#define MODE_PERSP 1
#define MODE_ORTHO 2
static int mode = MODE_UNKNOWN;


void MygluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);

static void BeginVisPersp( struct display_camera *camera )
{
	D3DXMATRIX  dmx;
	/* init fProjection */
	//D3DXMatrixPerspectiveFovLH( &dmx, 90.0/180.0*3.1415926, camera->aspect, 1.0f, 30000.0f );
	MygluPerspective(90.0f,camera->aspect,1.0f,30000.0f);
	{
		int n;
		for( n = 0; n < 16; n++ )
			dmx.m[0][n] = l.fProjection[0][n];
	}
	//D3DXMatrixPerspectiveLH( &dmx, camera->hVidCore->pWindowPos.cx, camera->hVidCore->pWindowPos.cy, 0.1f, 30000.0f );

	//camera->hVidCore->d3ddev->SetTransform( D3DTS_PROJECTION, &dmx );

	{
		D3DXMATRIX dmx2;
		D3DXMatrixIdentity( &dmx2 );
	//	camera->hVidCore->d3ddev->SetTransform( D3DTS_VIEW, &dmx2 );
	}
}


int InitD3D( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	if( !camera->flags.init )
	{
		BeginVisPersp( camera );
		lprintf( WIDE("First GL Init Done.") );
		camera->flags.init = 1;
	}
	return TRUE;										// Initialization Went OK
}



RENDER_PROC( int, EnableOpenD3DView )( PVIDEO hVideo, int x, int y, int w, int h )
{
	// enable a partial opengl area on a single window surface
	// actually turns out it's just a memory context anyhow...
	int nFracture;

	if( !hVideo->flags.bD3D )
	{
		if( !EnableD3D( hVideo ) )
         return 0;
	}
	nFracture = CreatePartialDrawingSurface( hVideo, x, y, w, h );
	if( nFracture )
	{
		nFracture -= 1;
		return nFracture + 1;
	}
	return 0;
}

int EnableD3D( PVIDEO hVideo )
{
	struct display_camera *camera  = hVideo->camera;
	D3D10CreateDevice1(
										  0, // adapter
										  D3D10_DRIVER_TYPE_HARDWARE,
										  0, // reserved
										  D3D10_CREATE_DEVICE_BGRA_SUPPORT,
										  D3D10_FEATURE_LEVEL_10_0,
										  D3D10_1_SDK_VERSION,
										  &camera->device);

	D3D10_TEXTURE2D_DESC description = {};
	description.ArraySize = 1;
	description.BindFlags =
		D3D10_BIND_RENDER_TARGET;
	description.Format =
		DXGI_FORMAT_B8G8R8A8_UNORM;
	description.Width = camera->w;
	description.Height = camera->h;
	description.MipLevels = 1;
	description.SampleDesc.Count = 1;
	description.MiscFlags =
		D3D10_RESOURCE_MISC_GDI_COMPATIBLE;

	camera->device->CreateTexture2D(
											 &description,
											 0, // no initial data
											 &camera->texture);


	camera->texture->QueryInterface(&camera->surface);
	CComPtr<ID2D1Factory> factory;
	factory.CoCreateInstance(CLSID_WICImagingFactory);


	IWICBitmap *bitmap;

	const D2D1_PIXEL_FORMAT format =
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
								D2D1_ALPHA_MODE_PREMULTIPLIED);

	const D2D1_RENDER_TARGET_PROPERTIES properties =
		D2D1::RenderTargetProperties(
											  D2D1_RENDER_TARGET_TYPE_DEFAULT,
											  format);


	const D2D1_RENDER_TARGET_PROPERTIES properties2 =
		D2D1::RenderTargetProperties(
											  D2D1_RENDER_TARGET_TYPE_DEFAULT,
											  format,
											  0.0f, // default dpi
											  0.0f, // default dpi
											  D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);


	factory->CreateDxgiSurfaceRenderTarget(
																 camera->surface,
																 &properties,
																 &camera->target);






#if _OLD_D3D
    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	//d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.hDeviceWindow = camera->hWndOutput;    // set the window to be used by Direct3D

    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;    // set the back buffer format to 32-bit
	d3dpp.BackBufferWidth = camera->pWindowPos.cx;    // set the width of the buffer
    d3dpp.BackBufferHeight = camera->pWindowPos.cy;    // set the height of the buffer

	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // create a device class using this information and information from the d3dpp stuct
    hVideo->d3d->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hVideo->hWndOutput,
					  D3DCREATE_HARDWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &hVideo->d3ddev);

	hVideo->flags.bD3D = 1;
	LeaveCriticalSec( &hVideo->cs );
#endif
	return TRUE;
}

RENDER_NAMESPACE_END

#endif