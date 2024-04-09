/* Defines events/interfaces for 3D render interfaces. 3d
   Mouse/keyboard support to render surfaces is provided here; Also
   provides generic interface between 3D rendering and
   application layer.                                               */
#ifndef INCLUDED__RENDER3D_EXTENSIONS_DEFINED__
#define INCLUDED__RENDER3D_EXTENSIONS_DEFINED__

#include <render.h>
#include <vectlib.h>

typedef struct render_3d_interface_tag
{
	RENDER_PROC_PTR( PTRANSFORM, GetRenderTransform)         ( PRENDERER );
	// returns TRUE if at least one of the points is inside the camera clip
	RENDER_PROC_PTR( LOGICAL, ClipPoints)         ( P_POINT points, int nPoints );
	RENDER_PROC_PTR( void, GetViewVolume )( PRAY *planes ); // array of 6 planes
	RENDER_PROC_PTR( void, SetRendererAnchorSpace )( PRENDERER display, int anchor ); // 0=world;1=local;2=view

#ifdef _D3D_DRIVER
	IDirect3DDevice9 *current_device;
#endif
#ifdef _D3D10_DRIVER
	ID3D10Device *current_device;
	ID3D10RenderTargetView *current_target;
	IDXGISwapChain *current_chain;
#endif
#ifdef _D3D11_DRIVER
	ID3D11Device *current_device;
	ID3D11DeviceContext *current_device_context;
	ID3D11RenderTargetView *current_target;
	IDXGISwapChain *current_chain;
#endif

#ifdef _VULKAN_DRIVER
	RENDER_PROC_PTR( struct cmdBuf *, getCommandBuffer )( void );
	RENDER_PROC_PTR( struct cmdBuf *, getGeometryBuffer )( void );
	RENDER_PROC_PTR( struct cmdBuf *, getVertexBuffer )( void );

#endif

} RENDER3D_INTERFACE, *PRENDER3D_INTERFACE;

#ifdef _VULKAN_DRIVER
RENDER_PROC( void, createCommandBuffers )( struct SwapChain* chain, VkCommandBuffer* buffers, uint32_t count, LOGICAL primary );
#endif

#if defined( _VULKAN_DRIVER )
#  define EXTRA_INIT_PARAM struct SwapChain* chain,
#  define EXTRA_INIT_ARG_TYPE struct SwapChain*,
#else
#  define EXTRA_INIT_PARAM
#  define EXTRA_INIT_ARG_TYPE 
#endif


#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER ) || defined( _D3D11_DRIVER )
#define g_d3d_device  (USE_RENDER3D_INTERFACE)->current_device
#define g_d3d_device_context  (USE_RENDER3D_INTERFACE)->current_device_context
#endif

// static uintptr_t OnInit3d( "Virtuality" )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
// each Init is called once for each display that is opened; the application recevies the reference
// of the transform used for the camera.  This may be used to clip objects that are out of scene.
// If you return 0 as the uintptr_t result, you will not get any further events.
//
// - (passes identity_depth as a reference to identity depty, because if the screen size changes, the identity depth also changes)
// identity_depth this is relative to how the display was opened.  identity is where there is a 1:1 relation
// between rendered image pixels rendered and physical display pixels.
// This may be arbitrarily overridden to be closer than normal in case the physical display dpi is too dense.
// identity depth should be used for rendering icons on objects.
#define OnInit3d(name) \
	DefineRegistryMethod("sack/render/puregl",Init3d,"init3d",name,"ExtraInit3d",uintptr_t,( EXTRA_INIT_PARAM PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect ),__LINE__)

// static void OnClose3d( "Virtuality" )(uintptr_t psvInit)
// handle event when the specified display context is closed.  The value passed is the result from Init3d();
// All resources relavent to the 3d engine should be released.  (shaders)  and statuses cleared so reinitialization can occur.
#define OnClose3d(name) \
	DefineRegistryMethod("sack/render/puregl",Close3d,"draw3d",name,"ExtraClose3d",void,(uintptr_t psvInit),__LINE__)

// static void OnResume3d( "Virtuality" )( void) { }
// On Resume is invoked when a significant time has passed and simulations should consider working from 'NOW'
// instead of the prior tick.  So basically the next 'update' should be a delta of 0; or that the delta should be from
// this point.  So maybe the draw is called first before the update in the case of resume.
// Android targets can invoke a pause state, from which resume will be required for smooth animations
// (excessive time steps will cause overflow conditions)
#define OnResume3d(name) \
	DefineRegistryMethod("sack/render/puregl",Resume3d,"draw3d",name,"Resume3d",void,(void),__LINE__)

// static LOGICAL OnUpdate3d( "Virtuality" )( uintptr_t psvInit, PTRANSFORM eye_transform );
// called when a new frame will be rendered.  Once per frame.  All others are called per-camera per-frame.
// can update the common viewpoint here, and it will propagate, otherwise the camera ends up resetting to
// this point, unless the named transformation matrix is loaded manually.
// it is passed the origin transformation view into the universe)
// since this is potentially one for multiple Init instances, cannot pass the instance this applies to, since it is all.
// the PTRANSFORM origin has just previously been updated with Move(), so it can have speed and acceleration applied
// return true/false to indicate a desire to draw this frame.  If this and nothing else changed, the frame will be skipped.
#define OnUpdate3d(name) \
	DefineRegistryMethod("sack/render/puregl",Update3d,"draw3d",name,"Update3d",LOGICAL,(PTRANSFORM origin ),__LINE__)

// static void OnFirstDraw3d( "Virtuality" )( uintptr_t psvInit );
// called the first time a camera draws itself;
// allows user code to load geometry into the camera... 
// it is passed the instance handle returned from Init3d
#define OnFirstDraw3d(name) \
	DefineRegistryMethod("sack/render/puregl",FirstDraw3d,"draw3d",name,"FirstDraw3d",void,(uintptr_t psvInit ),__LINE__)


// static void OnBeginDraw3d( "Virtuality" )( uintptr_t psvInit, PTRANSFORM camera )
// this is called once for each display that is opened, and for each OnInit3d that did not return 0.
// the psvInit is the init value returned, the mouse is the mouse as it is; it will be NULL if the mouse
// is not in the current display and we are just drawing.
// opportunity to override the camera position.
#define OnBeginDraw3d(name) \
	DefineRegistryMethod("sack/render/puregl",BeginDraw3d,"draw3d",name,"ExtraBeginDraw3d",void,(uintptr_t psvUser,PTRANSFORM camera),__LINE__)

// static void OnDraw3d( "Virtuality" )( uintptr_t psvInit )
// this is called once for each display that is opened, and for each OnInit3d that did not return 0.
// the psvInit is the init value returned.
#define OnDraw3d(name) \
	DefineRegistryMethod("sack/render/puregl",Draw3d,"draw3d",name,"ExtraDraw3d",void,(uintptr_t psvUser),__LINE__)

// static LOGICAL OnMouse3d( "Virtuality" )( uintptr_t psvInit, PRAY mouse, int32_t x, int32_t y, uint32_t b )
// this is a real mouse event that is in a display that you returned non 0 during Init3d.
// PRAY is the line represenging the point that the user has the mouse over at the moemnt.
// mouse buttons are passed. for mouse state (may also be key state)
// return FALSE if you did not use the mouse.
// return TRUE if you did, and therefore the event is used and noonne else should make two things happen...
#define OnMouse3d(name) \
	DefineRegistryMethod("sack/render/puregl",Mouse3d,"draw3d",name,"ExtraMouse3d",LOGICAL,(uintptr_t psvUser, PRAY mouse_ray, int32_t x, int32_t y, uint32_t b),__LINE__)

// static LOGICAL OnKey3d( "Virtuality" )( uintptr_t psvInit, uint32_t key )
// this is a real key event that is in a display that you returned non 0 during Init3d.
// return FALSE if you did not use the key.
// return TRUE if you did use the key, and therefore the event is used and noonne else should make two things happen...
#define OnKey3d(name) \
	DefineRegistryMethod("sack/render/puregl",Key3d,"draw3d",name,"ExtraKey3d",LOGICAL,(uintptr_t psvUser, uint32_t key),__LINE__)


#if !defined( FORCE_NO_INTERFACE ) && !defined( FORCE_NO_RENDER_INTERFACE )

#define REND3D_PROC_ALIAS(name) ((USE_RENDER_3D_INTERFACE)->_##name)

#  define GetRender3dInterface() (PRENDER3D_INTERFACE)GetInterface( "render.3d" )

#  define GetRenderTransform             REND3D_PROC_ALIAS(GetRenderTransform)
#  define ClipPoints             REND3D_PROC_ALIAS(ClipPoints)
#  define GetViewVolume             REND3D_PROC_ALIAS(GetViewVolume)
#endif

#endif // __RENDER3D_EXTENSIONS_DEFINED__
