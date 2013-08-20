#ifndef ALLEGRO_H
#define ALLEGRO_H

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
/******************************************************/
/************ Graphics and sprite routines ************/
/******************************************************/

#if !defined alleg_graphics_unused

//PSPRITE MakeSprite( char *pName );
//PSPRITE MakeSprite_Image( char *pName, ImageFile *pImage );
//PSPRITE MakeSprite_ImageFile( char *pName, char *fname );

//void rotate_sprite(ImageFile *dest, SPRITE *sprite, fixed angle);
//void rotate_scaled_sprite(ImageFile *dest, SPRITE *sprite, fixed angle, fixed scale);

//void BlotSprite( ImageFile *dest, SPRITE *ps );

#define SpriteImage(ps) (ps->image)

#endif

/***************************************/
/************ Math routines ************/
/***************************************/

#if !defined alleg_math_unused


//typedef int fixed;

#ifdef __cplusplus

}  /* end of extern "C" */


extern "C" {

#endif   /* ifdef __cplusplus */


typedef struct A_MATRIX            /* transformation matrix (fixed point) */
{
   fixed v[3][3];                /* scaling and rotation */
   fixed t[3];                   /* translation */
} A_MATRIX;


typedef struct A_MATRIX_f          /* transformation matrix (floating point) */
{
   float v[3][3];                /* scaling and rotation */
   float t[3];                   /* translation */
} A_MATRIX_f;


#ifdef __cplusplus__NOTHING

}  /* end of extern "C" */

/* overloaded functions for use with the fix class */


__INLINE__ void get_translation_matrix(A_MATRIX *m, fix x, fix y, fix z)
{ 
   get_translation_matrix(m, x.v, y.v, z.v);
}


__INLINE__ void get_scaling_matrix(A_MATRIX *m, fix x, fix y, fix z)
{ 
   get_scaling_matrix(m, x.v, y.v, z.v); 
}


__INLINE__ void get_x_rotate_matrix(A_MATRIX *m, fix r)
{ 
   get_x_rotate_matrix(m, r.v);
}


__INLINE__ void get_y_rotate_matrix(A_MATRIX *m, fix r)
{ 
   get_y_rotate_matrix(m, r.v);
}


__INLINE__ void get_z_rotate_matrix(A_MATRIX *m, fix r)
{ 
   get_z_rotate_matrix(m, r.v);
}


__INLINE__ void get_rotation_matrix(A_MATRIX *m, fix x, fix y, fix z)
{ 
   get_rotation_matrix(m, x.v, y.v, z.v);
}


__INLINE__ void get_align_matrix(A_MATRIX *m, fix xfront, fix yfront, fix zfront, fix xup, fix yup, fix zup)
{ 
   get_align_matrix(m, xfront.v, yfront.v, zfront.v, xup.v, yup.v, zup.v);
}


__INLINE__ void get_vector_rotation_matrix(A_MATRIX *m, fix x, fix y, fix z, fix a)
{ 
   get_vector_rotation_matrix(m, x.v, y.v, z.v, a.v);
}


__INLINE__ void get_transformation_matrix(A_MATRIX *m, fix scale, fix xrot, fix yrot, fix zrot, fix x, fix y, fix z)
{ 
   get_transformation_matrix(m, scale.v, xrot.v, yrot.v, zrot.v, x.v, y.v, z.v);
}


__INLINE__ void get_camera_matrix(A_MATRIX *m, fix x, fix y, fix z, fix xfront, fix yfront, fix zfront, fix xup, fix yup, fix zup, fix fov, fix aspect)
{ 
   get_camera_matrix(m, x.v, y.v, z.v, xfront.v, yfront.v, zfront.v, xup.v, yup.v, zup.v, fov.v, aspect.v);
}


__INLINE__ void qtranslate_matrix(A_MATRIX *m, fix x, fix y, fix z)
{
   qtranslate_matrix(m, x.v, y.v, z.v);
}


__INLINE__ void qscale_matrix(A_MATRIX *m, fix scale)
{
   qscale_matrix(m, scale.v);
}


__INLINE__ fix vector_length(fix x, fix y, fix z)
{ 
   fix t;
   t.v = vector_length(x.v, y.v, z.v);
   return t;
}


__INLINE__ void normalize_vector(fix *x, fix *y, fix *z)
{ 
   normalize_vector(&x->v, &y->v, &z->v);
}


__INLINE__ void cross_product(fix x1, fix y1, fix z1, fix x2, fix y2, fix z2, fix *xout, fix *yout, fix *zout)
{ 
   cross_product(x1.v, y1.v, z1.v, x2.v, y2.v, z2.v, &xout->v, &yout->v, &zout->v);
}


__INLINE__ fix dot_product(fix x1, fix y1, fix z1, fix x2, fix y2, fix z2)
{ 
   fix t;
   t.v = dot_product(x1.v, y1.v, z1.v, x2.v, y2.v, z2.v);
   return t;
}


__INLINE__ void apply_matrix(A_MATRIX *m, fix x, fix y, fix z, fix *xout, fix *yout, fix *zout)
{ 
   apply_matrix(m, x.v, y.v, z.v, &xout->v, &yout->v, &zout->v);
}

/*
__INLINE__ void persp_project(fix x, fix y, fix z, fix *xout, fix *yout)
{ 
#ifdef GCC
   persp_project(x.v, y.v, z.v, &xout->v, &yout->v);
#endif
}
*/

extern "C" {

#endif   /* ifdef __cplusplus */

#endif


#ifdef __cplusplus
}
#endif

#endif          /* ifndef ALLEGRO_H */


// $Log: sprite_local.h,v $
// Revision 1.6  2005/04/05 11:56:04  panther
// Adding sprite support - might have added an extra draw callback...
//
// Revision 1.5  2003/10/10 09:32:30  panther
// Pass 1 arm linux support
//
// Revision 1.4  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
