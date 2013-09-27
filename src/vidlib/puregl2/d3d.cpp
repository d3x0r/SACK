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
#include "win32.local.h"
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
			Render3d.current_device = hVideo->d3ddev;
			if( ! hVideo->d3ddev )
			{
				LeaveCriticalSec( &cs );
				return FALSE;
			}
			Render3d.current_device->BeginScene();    // begins the 3D scene
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
			Render3d.current_device->EndScene();    // ends the 3D scene

			Render3d.current_device->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
			Render3d.current_device = NULL;

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
#if 0
	{
		nFracture = hVideo->nFractures;
		if( !hVideo->pFractures || hVideo->nFractures == hVideo->nFracturesAvail )
		{
			hVideo->pFractures =
#ifdef __cplusplus
				(struct HVIDEO_tag::fracture_tag*)
#endif
				Allocate( sizeof( hVideo->pFractures[0] ) * ( hVideo->nFracturesAvail += 16 ) );
			MemSet( hVideo->pFractures + nFracture, 0, sizeof( hVideo->pFractures[0] ) * 16 );
		}
		hVideo->pFractures[nFracture].x = x;
		hVideo->pFractures[nFracture].y = y;
		hVideo->pFractures[nFracture].w = w;
		hVideo->pFractures[nFracture].h = h;
		{
		}
		if (!hVideo->pFractures[nFracture].hDCBitmap) // first time ONLY...
			hVideo->pFractures[nFracture].hDCBitmap = CreateCompatibleDC ((HDC)hVideo->hDCOutput);
		hVideo->pFractures[nFracture].hOldBitmap = SelectObject( (HDC)hVideo->pFractures[nFracture].hDCBitmap
																				 , hVideo->pFractures[nFracture].hBm);
		if( hVideo->flags.bReady && !hVideo->flags.bHidden && hVideo->pRedrawCallback )
		{
			struct {
				PVIDEO hVideo;
				int nFracture;
			} msg;
			msg.hVideo = hVideo;
			msg.nFracture = nFracture;
			//lprintf( WIDE("Sending redraw for fracture %d on vid %p"), nFracture, hVideo );
			//SendServiceEvent( local_vidlib.dwMsgBase + MSG_RedrawFractureMethod, &msg, sizeof( msg ) );
		}
	}
	hVideo->nFractures++;
#endif
	//lprintf( WIDE("And here I might want to update the video, hope someone else does for me.") );
	return nFracture + 1;
}



#define MODE_UNKNOWN 0
#define MODE_PERSP 1
#define MODE_ORTHO 2
static int mode = MODE_UNKNOWN;



static void BeginVisPersp( struct display_camera *camera )
{
	D3DXMATRIX  dmx;
	//D3DXMatrixPerspectiveLH( &dmx, 2, 2/camera->aspect, 1.0f, 30000.0f );
	D3DXMatrixPerspectiveFovLH( &dmx, 90.0/180.0*3.1415926, camera->aspect, 1.0f, 30000.0f );
	//D3DXMatrixPerspectiveLH( &dmx, camera->hVidCore->pWindowPos.cx, camera->hVidCore->pWindowPos.cy, 0.1f, 30000.0f );
	camera->hVidCore->d3ddev->SetTransform( D3DTS_PROJECTION, &dmx );

	{
		D3DXMATRIX dmx2;
		D3DXMatrixIdentity( &dmx2 );
		camera->hVidCore->d3ddev->SetTransform( D3DTS_VIEW, &dmx2 );
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
    hVideo->d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	//d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.hDeviceWindow = hVideo->hWndOutput;    // set the window to be used by Direct3D

    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;    // set the back buffer format to 32-bit
	d3dpp.BackBufferWidth = hVideo->pWindowPos.cx;    // set the width of the buffer
    d3dpp.BackBufferHeight = hVideo->pWindowPos.cy;    // set the height of the buffer

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
	return TRUE;
}

RENDER_NAMESPACE_END

#endif