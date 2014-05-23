#ifndef SPRITE_LOCAL_INCLUDED
#define SPRITE_LOCAL_INCLUDED

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************/
/************ Some global stuff ************/
/*******************************************/

#include <stdlib.h>
#include <stddef.h>
//#include <errno.h>

#ifndef MIN
#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))
#endif

#ifndef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))
#endif

#ifndef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)
#endif

#ifndef __INLINE__
#if defined __LCC__ || defined GCC || defined __TURBOC__
#define __INLINE__ static
#else
#define __INLINE__ extern _inline
#endif
#endif

//typedef long fixed;

#ifdef _DEBUG
#define DBGARG(e) e,
#else
#define DBGARG(e)
#endif


#ifndef IMAGE_H
typedef struct sprite_tag
{
   Image image;
   // curx,y are kept for moving the sprite independantly
   int curx, cury;  // current x and current y for placement on image.
   int hotx, hoty;  // int of bitmap hotspot... centers cur on hot
   // should consider keeping the angle of rotation
   // and also should cosider keeping velocity/acceleration
   // but then limits would have to be kept also... so perhaps
   // the game module should keep such silly factors... but then couldn't
   // it also keep curx, cury ?  though hotx hoty is the actual
   // origin to rotate this image about, and to draw ON curx 0 cury 0
  // int orgx, orgy;  // rotated origin of bitmap. 
} SPRITE, *PSPRITE;
#endif

#define SpriteImage(ps) (ps->image)



#ifdef __cplusplus
}
#endif

#endif          /* ifndef SPRITE_LOCAL_INCLUDED */

