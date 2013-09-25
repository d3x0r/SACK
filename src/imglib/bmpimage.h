
IMAGE_NAMESPACE
#ifdef __cplusplus 
		namespace loader {
#endif


Image ImageBMPFile (_8* ptr, _32 filesize);

#ifdef __cplusplus
		}// namespace loader
#endif
IMAGE_NAMESPACE_END

#ifdef __cplusplus
#ifdef _D3D_DRIVER
using namespace sack::image::d3d::loader;
#else
using namespace sack::image::loader;
#endif
#endif

// $Log: $
