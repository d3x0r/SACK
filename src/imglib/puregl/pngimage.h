
#ifdef DO_PNG
#ifndef PNGIMAGE_H
#define PNGIMAGE_H

/**
 * An ImageFile subclass for reading GIF files.
 */
IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif
ImageFile *ImagePngFile (_8 * buf, long size);
#ifdef __cplusplus
}
#endif
IMAGE_NAMESPACE_END

#endif //GIFIMAGE_H
#endif //DO_PNG


// $Log: $
