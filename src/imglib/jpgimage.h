/*
    Copyright (C) 1998 by Tor Andersson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef JPGIMAGE_H
#define JPGIMAGE_H

#include "image.h"

/**
 * the JPEG Image Loader.
 */
IMAGE_NAMESPACE
#ifdef __cplusplus 
namespace loader {
#endif

Image ImageJpgFile (uint8_t* buf, uint32_t size);
LOGICAL CPROC JpgImageFile( Image image, uint8_t **buf, size_t *size, int Q);

#ifdef __cplusplus 
}// namespace loader
#ifdef _D3D_DRIVER
using namespace sack::image::d3d::loader;
#elif defined( _D3D10_DRIVER )
using namespace sack::image::d3d10::loader;
#elif defined( _D3D11_DRIVER )
using namespace sack::image::d3d11::loader;
#endif

#endif
IMAGE_NAMESPACE_END

#endif //JPGIMAGE_H
// $Log: $
