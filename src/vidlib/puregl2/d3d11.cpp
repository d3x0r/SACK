/* reference code is http://www.rastertek.com/dx11tut03.html  */
/* see also http://msdn.microsoft.com/en-us/library/windows/desktop/hh437378(v=vs.85).aspx */


#define NO_UNICODE_C
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


int SetActiveD3DDisplayView( struct display_camera *hVideo, int nFracture )
{
	static CRITICALSECTION cs;
	static struct display_camera *_hVideo; // last display with a lock.
	if( hVideo )
	{
		if( nFracture )
		{
			nFracture -= 1;
		}
		else
		{
			_hVideo = hVideo;
			Render3d.current_device = hVideo->device;
			Render3d.current_device_context = hVideo->device_context;

			if( ! hVideo->device )
			{
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
			// Present as fast as possible.
			if( _hVideo->flags.vsync )
			{
				// Lock to screen refresh rate.
				_hVideo->swap_chain->Present(1, 0);
			}
			else
			{
				// Present as fast as possible.
				_hVideo->swap_chain->Present(0, 0);
			}
			_hVideo = NULL;
			//lprintf( "swapping buffer..." );
		}
	}
	return TRUE;
}

int SetActiveD3DDisplay( struct display_camera *hVideo )
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
	DirectX::XMMATRIX  dmx;
	/* init fProjection */
	//D3DXMatrixPerspectiveFovLH( &dmx, 90.0/180.0*3.1415926, camera->aspect, 1.0f, 30000.0f );
	MygluPerspective(90.0f,camera->aspect,1.0f,30000.0f);
	{
		int n;
		for( n = 0; n < 4; n++ )
			;//DirectX::XMStoreFloat4( dmx.r[n], l.fProjection[n] );
	}
	//D3DXMatrixPerspectiveLH( &dmx, camera->hVidCore->pWindowPos.cx, camera->hVidCore->pWindowPos.cy, 0.1f, 30000.0f );

	//camera->hVidCore->d3ddev->SetTransform( D3DTS_PROJECTION, &dmx );

	{
		DirectX::XMMATRIX dmx2;
		dmx2 = DirectX::XMMatrixIdentity();
	//	camera->hVidCore->d3ddev->SetTransform( D3DTS_VIEW, &dmx2 );
	}
}


int Init3D( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	// called as the first thing, when doing a render pass to this camera;
	// setup default parameters for state.

	BeginVisPersp( camera );

	if( !SetActiveD3DDisplay( camera ) )  // BeginScene()
		return FALSE;

	float pBackgroundColour[4] = { 0.0f, 0.2f, 0.5f, 0.2f };

	// Clear the back buffer.
	camera->device_context->ClearRenderTargetView( camera->render_target_view, pBackgroundColour);
    
	// Clear the depth buffer.
	camera->device_context->ClearDepthStencilView( camera->depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

	return TRUE;
}

void SetupPositionMatrix( struct display_camera *camera )
{
	// camera->origin_camera is valid eye position matrix
}

void EndActive3D( struct display_camera *camera ) // does appropriate EndActiveXXDisplay
{
	SetActiveD3DDisplay( NULL );
}


void DisableD3d( struct display_camera *camera )
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if( camera->swap_chain)
	{
		camera->swap_chain->SetFullscreenState(false, NULL);
	}

	if( camera->raster_state)
	{
		camera->raster_state->Release();
		camera->raster_state = 0;
	}

	if( camera->depth_stencil_view)
	{
		camera->depth_stencil_view->Release();
		camera->depth_stencil_view = 0;
	}

	if( camera->depth_stencil_state)
	{
		camera->depth_stencil_state->Release();
		camera->depth_stencil_state = 0;
	}

	if( camera->depth_stencil_buffer )
	{
		camera->depth_stencil_buffer->Release();
		camera->depth_stencil_buffer = 0;
	}

	if(camera->render_target_view)
	{
		camera->render_target_view->Release();
		camera->render_target_view = 0;
	}

	if( camera->device_context )
	{
		camera->device_context->Release();
		camera->device_context = 0;
	}

	if(camera->device)
	{
		camera->device->Release();
		camera->device = 0;
	}

	if(camera->swap_chain)
	{
		camera->swap_chain->Release();
		camera->swap_chain = 0;
	}
}


int InitDxDisplays( struct display_camera *camera )
{
	HRESULT result;

	if( l.adapters )
		return 1;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&l.dxgi_factory);

	if(FAILED(result))
	{
		return false;
	}
	unsigned int nAdapters = 0;
	struct dxgi_adapter *adapter;
	do
	{
		adapter = New( struct dxgi_adapter );
		adapter->ID = nAdapters;
		adapter->adapter_outputs = NULL;
		// Use the factory to create an adapter for the primary graphics interface (video card).
		result = l.dxgi_factory->EnumAdapters(nAdapters++, &adapter->adapter);
		if( result )
		{
			Deallocate( struct dxgi_adapter *, adapter );
			if( result != DXGI_ERROR_NOT_FOUND )
				lprintf( WIDE("Fatal no adapaters?  enumerated %d %x"), nAdapters, result );
			break;
		}
		else
		{
			adapter->adapter->GetDesc( &adapter->adapterDesc );
			lprintf( WIDE("Found adapter [%s]"), adapter->adapterDesc.Description );
			AddLink( &l.adapters, adapter );
		}
	}
	while( result != DXGI_ERROR_NOT_FOUND );

	INDEX idx;
	LIST_FORALL( l.adapters, idx, struct dxgi_adapter *, adapter )
	{
		struct dxgi_adapter_output *output;
		unsigned int nOutput = 0;
		do
		{
			output = New( struct dxgi_adapter_output );
			output->ID = nOutput;
			// Enumerate the primary adapter output (monitor).
			result = adapter->adapter->EnumOutputs(nOutput++, &output->adapterOutput);
			if(FAILED(result))
			{
				Deallocate( struct dxgi_adapter_output *, output );
				break;
			}
			else
			{
				output->adapterOutput->GetDesc( &output->adapterOutputDesc );
				lprintf( WIDE("Found output on [%s]%s"), adapter->adapterDesc.Description, output->adapterOutputDesc.DeviceName );
				AddLink( &adapter->adapter_outputs, output );
			}
		}while( result != DXGI_ERROR_NOT_FOUND );

		INDEX idx2;
		LIST_FORALL( adapter->adapter_outputs, idx2, struct dxgi_adapter_output *, output )
		{
			// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
			result = output->adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &output->numModes, NULL);
			if(FAILED(result))
			{
				return false;
			}

			// Create a list to hold all the possible display modes for this monitor/video card combination.
			output->displayModeList = new DXGI_MODE_DESC[output->numModes];
			if(!output->displayModeList)
			{
				return false;
			}

			// Now fill the display mode list structures.
			result = output->adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &output->numModes, output->displayModeList);
			if(FAILED(result))
			{
				return false;
			}

			// Now go through all the display modes and find the one that matches the screen width and height.
			// When a match is found store the numerator and denominator of the refresh rate for that monitor.
			for(int i=0; i<output->numModes; i++)
			{
				if(output->displayModeList[i].Width == (unsigned int)camera->w)
				{
					if(output->displayModeList[i].Height == (unsigned int)camera->h)
					{
						//numerator = displayModeList[i].RefreshRate.Numerator;
						//denominator = displayModeList[i].RefreshRate.Denominator;
					}
				}
			}
		}
	}
	return 1;
}

int EnableD3d( struct display_camera *camera )
{
	HRESULT result;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	InitDxDisplays( camera );
	
	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = camera->h;
	swapChainDesc.BufferDesc.Height = camera->w;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		// Set the refresh rate of the back buffer.
	if(camera->flags.vsync)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;//numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;//denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = camera->hWndInstance;
	swapChainDesc.Windowed = 1;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	result = D3D11CreateDeviceAndSwapChain( ((struct dxgi_adapter*)GetLink( &l.adapters, 0 ))->adapter
					, D3D_DRIVER_TYPE_HARDWARE
					, NULL
					, D3D11_CREATE_DEVICE_SINGLETHREADED                                 
					| D3D11_CREATE_DEVICE_BGRA_SUPPORT
					| D3D11_CREATE_DEVICE_DEBUGGABLE
					| D3D11_CREATE_DEVICE_DEBUG
					//| 
					, NULL // defaults to a list of all feature levels (allows shaders?!)
					, 0
					, D3D11_SDK_VERSION
					, &swapChainDesc
					, &camera->swap_chain
					, &camera->device
					, NULL
					, &camera->device_context);

	if( result )
	{
		result = D3D11CreateDeviceAndSwapChain(NULL
					, D3D_DRIVER_TYPE_REFERENCE //D3D_DRIVER_TYPE_HARDWARE
					, NULL
					, 0
					, NULL // defaults to a list of all feature levels (allows shaders?!)
					, 0
					, D3D11_SDK_VERSION
					, &swapChainDesc
					, &camera->swap_chain
					, &camera->device
					, NULL
					, &camera->device_context);
		if( result )
		{
			lprintf( WIDE("Failed to create device.") );
			return FALSE;
		}
	}

#if 0
   // dcomp is only available on windows 8, 8.1; windows 8.1 isn't available until 4 days from NOW.
	// 8.1 makes the interface perhaps desktop friendly; the shell has good features, but clumbsy 
	// legacy habit interface.
	camera->device->QueryInterface(&camera->pDXGIDevice);

	result  = DCompositionCreateDevice(camera->pDXGIDevice, 
                __uuidof(IDCompositionDevice), 
                reinterpret_cast<void **>(&camera->m_pDCompDevice));
#endif
#if BUILD_D2D_TARGET_SURFACE
	D3D11_TEXTURE2D_DESC description = {};
	description.ArraySize = 1;
	description.BindFlags =
		D3D11_BIND_RENDER_TARGET;
	description.Format =
		DXGI_FORMAT_B8G8R8A8_UNORM;
	description.Width = camera->w;
	description.Height = camera->h;
	description.MipLevels = 1;
	description.SampleDesc.Count = 1;
	description.MiscFlags =
		D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	camera->device->CreateTexture2D(
											 &description,
											 0, // no initial data
											 &camera->texture);


	camera->texture->QueryInterface(&camera->surface);

	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory),0,(void**)&l.d2d1_factory);

	IWICBitmap *bitmap;

	const D2D1_PIXEL_FORMAT format =
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
								D2D1_ALPHA_MODE_PREMULTIPLIED);

	const D2D1_RENDER_TARGET_PROPERTIES properties =
		D2D1::RenderTargetProperties(  D2D1_RENDER_TARGET_TYPE_DEFAULT,
									  format);


	lprintf( WIDE("don't know what to do with a target anyway (yet); set it as a render target I suppose.") );
	  	l.d2d1_factory->CreateDxgiSurfaceRenderTarget(
																 camera->surface,
																 &properties,
																 &camera->target);
#endif
	ID3D11Texture2D *pBackBuffer;
    camera->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if( pBackBuffer )
	{
		// use the back buffer address to create the render target
		camera->device->CreateRenderTargetView(pBackBuffer, NULL, &camera->render_target_view);
		pBackBuffer->Release();
		// set the render target as the back buffer
		camera->device_context->OMSetRenderTargets(1, &camera->render_target_view, camera->depth_stencil_view );
	}
	else
		lprintf( WIDE("Fatal gettigg backbuffer.") );


	D3D11_RASTERIZER_DESC rasterDesc;
	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;  // usually cull_back
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = false;  // usually enabled
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = camera->device->CreateRasterizerState(&rasterDesc, &camera->raster_state);
	if(FAILED(result))
	{
		return false;
	}

	// Now set the rasterizer state.
	camera->device_context->RSSetState(camera->raster_state);

#if 0
	// another bit of coe that looked like a shader resource dictionary or something

	// Shader-resource buffer
	D3D11_BUFFER_DESC bufferDesc = {0};
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.ByteWidth = 4 * 4 * 2;// 2 float4
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

	// [0, 3] = texCoords, [4, 7] = positions
	const float coords[8] = {
		0.0f, 0.0f, 1.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 0.5f
	};

	D3D11_SUBRESOURCE_DATA bufferData = {0};
	bufferData.pSysMem = coords;

	ID3D11Buffer *pBuffer;
	camera->device->CreateBuffer(&bufferDesc, &bufferData, &pBuffer);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC bufferSRVDesc;
	ZeroMemory(&bufferSRVDesc, sizeof(bufferSRVDesc));
	bufferSRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	bufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	bufferSRVDesc.Buffer.ElementWidth = 2;// 2 float4
	
	ID3D11ShaderResourceView *pBufferSRV;
	camera->device->CreateShaderResourceView(pBuffer, &bufferSRVDesc, &pBufferSRV);
#endif
	return TRUE;
}


int EnableD3dView( struct display_camera *camera, int x, int y, int w, int h )
{
	// enable a partial opengl area on a single window surface
	// actually turns out it's just a memory context anyhow...
	int nFracture;

	if( !camera->hVidCore->flags.bD3D )
	{
		if( !EnableD3d( camera ) )
         return 0;
	}
	//nFracture = CreatePartialDrawingSurface( hVideo, x, y, w, h );
	nFracture = 0;
	if( nFracture )
	{
		nFracture -= 1;
		return nFracture + 1;
	}
	return 0;
}




RENDER_NAMESPACE_END

#endif