/*
    Copyright (C) 1998 by Panther
  
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

#ifdef DO_GIF
#ifndef GIFIMAGE_H
#define GIFIMAGE_H

/**
 * An ImageFile subclass for reading GIF files.
 */
   IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif

ImageFile *ImageGifFile (_8 * buf, long size);
#ifdef __cplusplus
}//namespace loader {
#endif
IMAGE_NAMESPACE_END

#endif //GIFIMAGE_H
#endif //DO_GIF

// $Log: $
