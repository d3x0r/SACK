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

Image ImageJpgFile (_8* buf, _32 size);

#ifdef __cplusplus 
}// namespace loader
#endif
IMAGE_NAMESPACE_END

#endif //JPGIMAGE_H
// $Log: $
