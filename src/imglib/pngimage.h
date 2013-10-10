
#ifdef DO_PNG
#ifndef PNGIMAGE_H
#define PNGIMAGE_H

/**
 * An ImageFile subclass for reading PNG files.
 */
IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif
   // decompress a buffer into an image...
	ImageFile *ImagePngFile (_8 * buf, size_t size);
	// compress image into a buffer
	LOGICAL PngImageFile ( Image pImage, _8 ** buf, size_t *size);
#ifdef __cplusplus
}
#endif
IMAGE_NAMESPACE_END

#endif //GIFIMAGE_H
#endif //DO_PNG

#ifdef __cplusplus
#ifdef _D3D_DRIVER
using namespace sack::image::d3d::loader;
#else
using namespace sack::image::loader;
#endif
#endif

// $Log: $
