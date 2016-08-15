
IMAGE_NAMESPACE
#ifdef __cplusplus 
		namespace loader {
#endif


Image ImageBMPFile (uint8_t* ptr, uint32_t filesize);

#ifdef __cplusplus
		}// namespace loader
#endif
IMAGE_NAMESPACE_END

#ifdef __cplusplus
#ifdef _D3D_DRIVER
using namespace sack::image::d3d::loader;
#elif defined( _D3D10_DRIVER )
using namespace sack::image::d3d10::loader;
#elif defined( _D3D11_DRIVER )
using namespace sack::image::d3d11::loader;
#else
using namespace sack::image::loader;
#endif
#endif

// $Log: $
