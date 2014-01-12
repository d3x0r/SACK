#if 0
#include <stdhdrs.h>
#include <vidlib/vidstruc.h>
#include <render.h>

#ifdef __cplusplus

#undef Release
#undef Delete
/*
#define __in
#define __out
#define __in_opt
#define __out_opt
#define __inout
#define __inout_opt

#define __out_ecount(n)
#define __out_ecount_opt(n)
#define __out_bcount(n)
#define __out_bcount_opt(n)
#define __in_ecount(n)
#define __in_ecount_opt(n)
#define __in_bcount(n)
#define __in_bcount_opt(n)

#define UINT8 _8
#define UINT  _32
#include <d3d10.h>
*/
#include <d3d9.h>

struct directx_local {
	struct _dx9
	{
		LPDIRECT3D9 pD3D;
		LPDIRECT3DDEVICE9 pDevice;
	} dx9;
   /*
	struct _dx10
	{
		//HINSTANCE               hInst = NULL;
		//HWND                    hWnd = NULL;
		D3D10_DRIVER_TYPE       driverType;
		ID3D10Device*           pd3dDevice;
		IDXGISwapChain*         pSwapChain;
		ID3D10RenderTargetView* pRenderTargetView;
		} dx10;
		*/

} directx_local;
#define l directx_local

PRELOAD( InitLocal )
{
   //l.dx10.driverType = D3D10_DRIVER_TYPE_NULL;
}


bool InitD3D( PRENDERER hVideo )
{
   /*
   DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof(sd) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 640;
    sd.BufferDesc.Height = 480;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    if( FAILED( D3D10CreateDeviceAndSwapChain( NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
                     0, D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice ) ) )
    {
        return FALSE;
    }

    // Create a render target view
    ID3D10Texture2D *pBackBuffer;
    if( FAILED( g_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), (LPVOID*)&pBackBuffer ) ) )
        return FALSE;
    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return FALSE;
    g_pd3dDevice->OMSetRenderTargets( 1, &g_pRenderTargetView, NULL );
    */

    if((l.dx9.pD3D = Direct3DCreate9(D3D_SDK_VERSION))==NULL){
        return false;
    }
    D3DDISPLAYMODE d3ddm;
    l.dx9.pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    if(FAILED(l.dx9.pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
       hVideo->hWndOutput, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &l.dx9.pDevice))){
        return false;
    }
    return true;
} 

void RenderD3DScene(){
    l.dx9.pDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,1,0);
    l.dx9.pDevice->Present(NULL,NULL,NULL,NULL);
} 



void EnableDirectX( PRENDERER render )
{

}

#endif
#endif
