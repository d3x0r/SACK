// one day I'd like to make a multidimensional library
// but for now - 3D is sufficient - it can handle everything
// under 2D ignoring the Z axis... although it would be more
// efficient to do 2D implementation alone... 
// but without function overloading the names of all the functions
// become much too complex.. well perhaps - maybe I can
// make all the required functions with a suffix - and 
// supply defines to choose the default based on the dimension number
#ifndef ROTATE_DECLARATION
// vector multiple inclusion protection
#define ROTATE_DECLARATION

#if !defined(__STATIC__) && !defined(__LINUX__)
#  ifdef VECTOR_LIBRARY_SOURCE
#    define MATHLIB_EXPORT EXPORT_METHOD
#    if defined( __WATCOMC__ ) || defined( _MSC_VER ) && !defined( __clang__ )
// data requires an extra extern to generate the correct code *boggle*
#      define MATHLIB_DEXPORT extern EXPORT_METHOD
#    else
#      define MATHLIB_DEXPORT EXPORT_METHOD
#    endif
#  else
#    define MATHLIB_EXPORT IMPORT_METHOD
#    if ( defined( __WATCOMC__ ) || defined( _MSC_VER ) || defined( __GNUC__ ) ) && !defined( __ANDROID__ )
// data requires an extra extern to generate the correct code *boggle*
#      ifndef __cplusplus_cli
#        define MATHLIB_DEXPORT extern IMPORT_METHOD
#      else
#        define MATHLIB_DEXPORT extern IMPORT_METHOD
#      endif
#    else
#      define MATHLIB_DEXPORT IMPORT_METHOD
#    endif
#  endif
#else
#ifndef VECTOR_LIBRARY_SOURCE
#define MATHLIB_EXPORT extern
#define MATHLIB_DEXPORT extern
#else
#define MATHLIB_EXPORT
#define MATHLIB_DEXPORT
#endif
#endif

#define DIMENSIONS 3

#if( DIMENSIONS > 0 )
   #define vRight   0
   #define _1D(exp)  exp
   #if( DIMENSIONS > 1 )
	  #define vUp      1
	  #define _2D(exp)  exp
	  #if( DIMENSIONS > 2 )
		 #define vForward 2
		 #define _3D(exp)  exp
		 #if( DIMENSIONS > 3 )
			#define vIn      3  // 4th dimension 'IN'/'OUT' since projection is scaled 3d...
			#define _4D(exp)  exp
		 #else
			#define _4D(exp)
		 #endif
	  #else
		 #define _3D(exp)
		 #define _4D(exp)
	  #endif
   #else
	  #define _2D(exp)
	  #define _3D(exp)
	  #define _4D(exp)
   #endif
#else
   // print out a compiler message can't perform zero-D transformations...
#endif

#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER )
#  ifndef MAKE_RCOORD_SINGLE
#    define MAKE_RCOORD_SINGLE
#  endif
#endif

#ifdef __cplusplus
#  ifndef MAKE_RCOORD_SINGLE
#    define VECTOR_NAMESPACE SACK_NAMESPACE namespace math { namespace vector { namespace Double {
#    define _MATH_VECTOR_NAMESPACE namespace math { namespace vector { namespace Double {
#    define _VECTOR_NAMESPACE namespace vector { namespace Double {
#    define USE_VECTOR_NAMESPACE using namespace sack::math::vector::Double;
#  else
#    define VECTOR_NAMESPACE SACK_NAMESPACE namespace math { namespace vector { namespace Float {
#    define _MATH_VECTOR_NAMESPACE namespace math { namespace vector { Float {
#    define _VECTOR_NAMESPACE namespace vector { namespace Float {
#    define USE_VECTOR_NAMESPACE using namespace sack::math::vector::Float;
#  endif
#  define _MATH_NAMESPACE namespace math {
#  define VECTOR_NAMESPACE_END } } } SACK_NAMESPACE_END
#else
#  define VECTOR_NAMESPACE
#  define _MATH_VECTOR_NAMESPACE
#  define _MATH_NAMESPACE
#  define _VECTOR_NAMESPACE
#  define VECTOR_NAMESPACE_END
#  define USE_VECTOR_NAMESPACE 
#endif

#undef EXTERNAL_NAME
#undef VECTOR_METHOD

#ifdef MAKE_RCOORD_SINGLE
#  define VECTOR_METHOD(r,n,args) MATHLIB_EXPORT r n##f args
#  define EXTERNAL_NAME(n)  n##f
#else
#  define VECTOR_METHOD(r,n,args) MATHLIB_EXPORT r n##d args
#  define EXTERNAL_NAME(n)  n##d
#endif

#include <vectypes.h>

SACK_NAMESPACE
_MATH_NAMESPACE
/* Vector namespace contains methods for operating on vectors. Vectors
   are multi-dimensional scalar quantities, often used to
   represent coordinates and directions in space.

	PTRANSFORM can also be a MATRIX, however, instances of transforms should be
	created with CreateTransform and deleted with DeleteTransform.

	PTRANSFORM is an opaque type representing a transformation matrix. It has scaling 
	internally separate from the orientation and position matrix.
	

   */
#ifdef __cplusplus
	namespace vector {
#  ifndef MAKE_RCOORD_SINGLE
		namespace Double {
#  else
		namespace Float {
#  endif
#endif



/* A 4 dimensional point type. Contains 4 values. 
*  Rotation Vectors are saved as normalized axis-angle, with the angle as the 4th element.
*/
typedef RCOORD _POINT4[4];

/* A point type. Contains 3 values by default, library can
   handle 4 dimensional transformations(?)                  */
typedef RCOORD _POINT[DIMENSIONS];

/* pointer to a (DIMENSIONS) point. */
typedef RCOORD *P_POINT;
/* for consistency, a 4 dimensional point pointer. */
typedef RCOORD* P_POINT4;
/* pointer to a constant point. */
typedef const RCOORD *PC_POINT;
/* for consistency a pointer that should have 4 elements, pointer to a constant point. */
typedef const RCOORD* PC_POINT4;

/* A vector type. Contains 3 values by default, library can
   handle 4 dimensional transformations(?)                  */
typedef _POINT VECTOR;

/* A 4 dimensional vector type. Contains 4 values. 
* Rotation Vectors are saved as normalized axis-angle, with the angle as the 4th element.
*/
typedef _POINT4 VECTOR4;


/* pointer to a vector. */
typedef P_POINT PVECTOR;
/* pointer to a 4 dimensional vector. */
typedef P_POINT4 PVECTOR4;
/* pointer to a constant vector. */
typedef PC_POINT PCVECTOR;
/* pointer to a constant 4 dimensional vector. */
typedef PC_POINT4 PCVECTOR4;

/* <combine sack::math::vector::RAY@1>
   
   \ \                                 */
typedef struct vectlib_ray_type *PRAY;
/* <combine sack::math::vector::RAY@1>
   
   \ \                                 */
typedef struct vectlib_ray_type RAY;
/* A ray is a type that has an origin and a direction. (It is a
   pair of vectors actually)                                    */
struct vectlib_ray_type {
   _POINT o; // origin
   _POINT n; // normal
};

/* <combinewith sack::math::vector::lineseg_tag>
   
   \ \                                           */
typedef struct lineseg_tag  LINESEG;
/* <combine sack::math::vector::lineseg_tag>
   
   \ \                                       */
typedef struct lineseg_tag *PLINESEG;
/* This is a pure abstraction of a Line. It is used in the basis
   of 3d graphics.                                               */
struct lineseg_tag {
   /* a ray type that is the origin and slope of the line. */
	RAY r; 
	/* scalar along direction vector that indicates where the line
	   segment ends. (origin + (direction * dTo) ) = end           */
	/* scalar along direction vector that indicates where the line
	   segment ends. (origin + (direction * dTo) ) = start         */
	RCOORD dFrom, dTo;
};

/* <combine sack::math::vector::orthoarea_tag>
   
   \ \                                         */
typedef struct orthoarea_tag ORTHOAREA;
/* <combine sack::math::vector::orthoarea_tag>
   
   \ \                                         */
typedef struct orthoarea_tag *PORTHOAREA;
/* A representation of a rectangular 2 dimensional area. */
struct orthoarea_tag {
	/* x coorindate of a rectangular area. */
	/* y coordinate of a rectangular area. */
	RCOORD x, y;
	/* height (y + h = area end). height may be negative. */
	/* with (x + w = area end). with may be negative. */
	RCOORD w, h;
} ;

// relics from fixed point math dayz....
#define ZERO (0.0f)
/* Special symbol that is the unit quantity. */
#define ONE  (1.0f)

#ifndef M_PI
/* symbol to define a double precision value for PI if it
   doesn't exist in the compiler.                         */
#ifdef MAKE_RCOORD_SINGLE
#  define M_PI (3.1415926535f)
#else
#  define M_PI (3.1415926535)
#endif
#endif

/* a hard coded define that represents a 5 degree angle in
   radians.                                                */
#define _5  (RCOORD)((5.0/180.0)*M_PI )
/* a hard coded define that represents a 15 degree angle in
   radians.                                                 */
#define _15 (RCOORD)((15.0/180.0)*M_PI )
/* a hard coded define that represents a 30 degree angle in
   radians.                                                 */
#define _30 (RCOORD)((30.0/180.0)*M_PI )
/* a hard coded define that represents a 45 degree angle in
   radians.                                                 */
#define _45 (RCOORD)((45.0/180.0)*M_PI )

#define SetPoint( d, s ) ( (d)[0] = (s)[0], (d)[1]=(s)[1], (d)[2]=(s)[2] )
#define SetPoint4( d, s ) ( (d)[0] = (s)[0], (d)[1]=(s)[1], (d)[2]=(s)[2], (d)[3]=(s)[3] )
/* Inverts a vector. that is vector * -1. (a,b,c) = (-a,-b,-c)
   
   <b>Parameters</b>
															   */
VECTOR_METHOD( P_POINT, Invert, ( P_POINT a ) );

/* Macro which can be used to make a vector's direction be
   exactly opposite of what it is now.                     */
#define InvertVector( a ) ( a[0] = -a[0], a[1]=-a[1], a[2]=-a[2] )

/* Logs the vector and leader to log. the leader is called
   lpName, cause it was inteded to be used by just the vector
   name.
   Parameters
   lpName :  text leader to print before the vector. 
   v :       vector to log
   
   Example
   <code lang="c++">
   PrintVector( _X );
   // expands to
   PrintVectorEx( "_X", _X DBG_SRC );
   </code>                                                    */
VECTOR_METHOD( void, PrintVectorEx, ( CTEXTSTR lpName, PCVECTOR v DBG_PASS ) );
/* <combine sack::math::vector::PrintVectorEx@CTEXTSTR@PCVECTOR v>
   
   \ \                                                               */
#define PrintVector(v) PrintVectorEx( #v, v DBG_SRC )
/* Same as PrintVectorEx, but prints to standard output using
   printf.                                                    */
VECTOR_METHOD( void, PrintVectorStdEx, ( CTEXTSTR lpName, VECTOR v DBG_PASS ) );
/* <combine sack::math::vector::PrintVectorStdEx@CTEXTSTR@VECTOR v>
   
   \ \                                                                */
#define PrintVectorStd(v) PrintVectorStd( #v, v DBG_SRC )
/* Dumps to syslog a current matrix. Shows both matrix content,
   and the cross products between the matrix that cross1 should
   be row 0, cross2 should be row 1 and cross3 should be row2.
   Pass a text name to identify this matrix from others.
   Parameters
   lpName :    Name to write into the log.
   m :         the matrix to dump.
   DBG_PASS :  standard debug paramters
   
   Remarks
   A PTRANSFORM is not a MATRIX; there is a matrix in a
   transform, and is the first member, so a ptransform can be
   cast to a matrix and logged with this function.              */
VECTOR_METHOD( void, PrintMatrixEx, ( CTEXTSTR lpName, MATRIX m DBG_PASS ) );
/* <combine sack::math::vector::PrintMatrixEx@CTEXTSTR@MATRIX m>
   
   \ \                                                             */
#define PrintMatrix(m) PrintMatrixEx( #m, m DBG_SRC )

/* <combine sack::math::vector::TransformationMatrix>
   
   \ \                                                */
typedef struct transform_tag *PTRANSFORM;
/* <combine sack::math::vector::TransformationMatrix>
   
   \ \                                                */
typedef struct transform_tag 	TRANSFORM;
/* Pointer to a constant transform. */
typedef const TRANSFORM *PCTRANSFORM;
/* Constant pointer to a constant transform. For things like _I
   transformation which is the identity translation.            */
typedef const PCTRANSFORM *CPCTRANSFORM;

#define VectorConst_0 EXTERNAL_NAME(VectorConst_0)
#define VectorConst_X EXTERNAL_NAME(VectorConst_X)
#define VectorConst_Y EXTERNAL_NAME(VectorConst_Y)
#define VectorConst_Z EXTERNAL_NAME(VectorConst_Z)
#define VectorConst_I EXTERNAL_NAME(VectorConst_I)

#ifdef __cplusplus
#define VECTLIBCONST
#else
#define VECTLIBCONST const
#endif

//------ Constants for origin(0,0,0), and axii
#if !defined( VECTOR_LIBRARY_SOURCE ) || defined( VECTOR_LIBRARY_IS_EXTERNAL )
MATHLIB_DEXPORT VECTLIBCONST PC_POINT VectorConst_0;
/* Specifies the coordinate system's X axis direction. static
   constant.                                                  */
MATHLIB_DEXPORT VECTLIBCONST PC_POINT VectorConst_X;
/* Specifies the coordinate system's Y axis direction. static
   constant.                                                  */
MATHLIB_DEXPORT VECTLIBCONST PC_POINT VectorConst_Y;
/* Specifies the coordinate system's Z axis direction. static
   constant.                                                  */
MATHLIB_DEXPORT VECTLIBCONST PC_POINT VectorConst_Z;
/* This is a static constant identity matrix, which can be used
   to initialize a matrix transform (internally).               */
MATHLIB_DEXPORT VECTLIBCONST PCTRANSFORM VectorConst_I;
#define _0 ((PC_POINT)VectorConst_0)
#  ifndef _X
#    define _X ((PC_POINT)VectorConst_X)
#  else
#    warning _X previously defined, skipping definition for vectlib
#  endif
#define _Y ((PC_POINT)VectorConst_Y)
#define _Z ((PC_POINT)VectorConst_Z)
#define _I ((PC_POINT)VectorConst_I)
#endif

								   
/* compares two vectors to see if they are near each other. Boundary
   \conditions exist around 0, when the values are on opposite
   sides, but it's pretty good.                                      */
#define Near( a, b ) ( COMPARE(a[0],b[0]) && COMPARE( a[1], b[1] ) && COMPARE( a[2], b[2] ) )

/* Add two vectors together. (a1,b1,c1) + (a2,b2,c2) =
   (a1+a2,b1+b2,c1+c2)
   Remarks
   The result vector may be the same as one of the source
   vectors.                                               */
VECTOR_METHOD( P_POINT, add, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2 ) );
/* subtracts two vectors and stores the result in another
   vector.
   Remarks
   The result vector may be the same as one of the source
   vectors.                                               */
VECTOR_METHOD( P_POINT, sub, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2 ) );
/* Scales a vector by a scalar
   Parameters
   pr :   \result vector
   pv1 :  vector to scale
   k :    constant scalar to apply to vector
   
   Example
   <code lang="c#">
   VECTOR result;
   VECTOR start;
   SetPoint( start, _X );
   scale( result, start, 3 );
   </code>                                   */
VECTOR_METHOD( P_POINT, scale, ( P_POINT pr, PC_POINT pv1, RCOORD k ) );
/* Adds a vector scaled by a scalar to another vector, results
   in a third vector.
   
   
   Parameters
   pr :   pointer to a result vector
   pv1 :  pointer to vector 1
   pv2 :  pointer to vector 2
   k :    scalar quantity to apply to vector 2 when adding to
		  vector 1.
   
   Remarks
   The pointer to the result vector may be the same vector as
   vector 1 or vector 2.
   Example
   <code lang="c++">
   _POINT result;
   P_POINT v1 = _X;
   P_POINT v2 = _Y;
   RCOORD k = 1.414;
   addscaled( result, v1, v2, k );
   
   // result is ( 1, 1.414, 0 )
   </code>                                                     */
VECTOR_METHOD( P_POINT, addscaled, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2, RCOORD k ) );
/* Normalizes a non-zero vector. That is the resulting length of
   the vector is 1.0. Modifies the vector in place.              */
VECTOR_METHOD( P_POINT, normalize, ( P_POINT pv ) );
VECTOR_METHOD( P_POINT4, normalize4, ( P_POINT4 pv ) );
VECTOR_METHOD( P_POINT, crossproduct, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2 ) );
/* \Returns the sin of the angle between two vectors.
   Parameters
   pv1 :  one vector
   pv2 :  another vector
   
   Remarks
   If the length of either vector is 0, then a divide by zero
   exception will happen.                                     */
VECTOR_METHOD( RCOORD, SinAngle, ( PC_POINT pv1, PC_POINT pv2 ) );
/* \Returns the cos (cosine) of the angle between two vectors.
   Parameters
   pv1 :  one vector
   pv2 :  another vector
   
   Remarks
   If the length of either vector is 0, then a divide by zero
   exception will happen.                                      */
VECTOR_METHOD( RCOORD, CosAngle, ( PC_POINT pv1, PC_POINT pv2 ) );
VECTOR_METHOD( RCOORD, dotproduct, ( PC_POINT pv1, PC_POINT pv2 ) );
// result is the projection of project onto onto
VECTOR_METHOD( P_POINT, project, ( P_POINT pr, PC_POINT onto, PC_POINT project ) );
/* \Returns the scalar length of a vector. */
VECTOR_METHOD( RCOORD, Length, ( PC_POINT pv ) );
/* \Returns the distance between two points.
   
   
   Parameters
   v1 :  some point
   v2 :  another point
   
   Returns
   The distance between the two points.      */
VECTOR_METHOD( RCOORD, Distance, ( PC_POINT v1, PC_POINT v2 ) );
/* \Returns the distance a point is as projected on another
   vector. The result is the distance along that vector from the
   origin.
   Parameters
   pvOn :  Vector to project on
   pvOf :  vector to get projection length of.                   */
VECTOR_METHOD( RCOORD, DirectedDistance, ( PC_POINT pvOn, PC_POINT pvOf ) );

/* copies the value of a ray into another ray
   Parameters
   ray to set :   target value 
   ray to copy :  value to copy.
   
   Example
   <code>
   RAY ray;
   RAY ray2;
   // set ray to ray2
   SetRay( ray, ray2 );
   
   </code>                                    */
#define SetRay( pr1, pr2 ) { SetPoint( (pr1)->o, (pr2)->o ),  \
							 SetPoint( (pr1)->n, (pr2)->n ); }



		/* Allocates and initializes a new transform for the user.
		 if name is NULL, allocates an unnamed transform; otherwise
	   the transform is created in a known namespace that can be browsed.
		 */
VECTOR_METHOD( PTRANSFORM, CreateNamedTransform, ( CTEXTSTR name ) );
#define CreateTransform() CreateNamedTransform( NULL )
VECTOR_METHOD( PTRANSFORM, CreateTransformMotion, ( PTRANSFORM pt ) );
VECTOR_METHOD( PTRANSFORM, CreateTransformMotionEx, ( PTRANSFORM pt, int rocket ) );
VECTOR_METHOD( void, DestroyTransform     , ( PTRANSFORM pt ) );
/* Resets a transform back to initial conitions. */
VECTOR_METHOD( void, ClearTransform       , ( PTRANSFORM pt ) );
/* Badly named function.
   
   InvertTransform turns a transform sideways, that is takes
   axis-normal transforms and turns them for sending to other
   graphic systems.
   <code lang="c++">
   
   
	 \+-         -+
	 | 0   1   2 |
	 | 3   4   5 |
	 | 6   7   8 |
	 \+-         -+
   becomes
	 \+-         -+
	 | 0   3   6 |
	 | 1   4   7 |
	 | 2   5   8 |
	 \+-         -+
   
   
   Not entirely useful at all :)
   </code>                                                    */
VECTOR_METHOD( void, InvertTransform        , ( PTRANSFORM pt ) );
VECTOR_METHOD( void, Scale                 , ( PTRANSFORM pt, RCOORD sx, RCOORD sy, RCOORD sz ) );

VECTOR_METHOD( void, Translate             , ( PTRANSFORM pt, RCOORD tx, RCOORD ty, RCOORD tz ) );
VECTOR_METHOD( void, TranslateV            , ( PTRANSFORM pt, PC_POINT t ) );
VECTOR_METHOD( void, TranslateRel          , ( PTRANSFORM pt, RCOORD tx, RCOORD ty, RCOORD tz ) );
VECTOR_METHOD( void, TranslateRelV         , ( PTRANSFORM pt, PC_POINT t ) );


VECTOR_METHOD( void, RotateAbs, ( PTRANSFORM pt, RCOORD rx, RCOORD ry, RCOORD rz ) );
VECTOR_METHOD( void, RotateAbsV, ( PTRANSFORM pt, PC_POINT ) );
/* Updates the current rotation matrix of a transform by a
   relative amount. Amounts to rotate about the x, y and z axii
   are given in radians.
   Parameters
   pt :  transform to rotate
   rx :  amount around the x axis to rotate (pitch)(positive is
		 clockwise looking at the object from the right, axis up is
		 moved towards forward )
   ry :  amount around the y axis to rotate (yaw) (positive is
		 counter clockwise, moves right to forward)
   rz :  amount around the z axis to rotate (roll) (positive is
		 clockwise, moves up towards right )
   
   See Also
   RotateRelV                                                       */
VECTOR_METHOD( void, RotateRel, ( PTRANSFORM pt, RCOORD rx, RCOORD ry, RCOORD rz ) );
/* Update a transformation matrix by relative degress about the
   x axix, y axis and z axis.
   Parameters
   pt :  transform to update
   v :   vector containing x,y and z relative roll coordinate.  */
VECTOR_METHOD( void, RotateRelV, ( PTRANSFORM pt, PC_POINT ) );
/* Rotates a transform around some arbitrary axis. (any line may
   be used to rotate the transformation's rotation matrix)
   Parameters
   pt :      transform to update
   p :       P defines an axis around which the rotation portion
			 of the matrix is rotated by an amount. Can be any
			 arbitrary axis.
   amount :  an amount to rotate by.
   
   Note
   coded from
   http://www.mines.edu/~gmurray/ArbitraryAxisRotation/ArbitraryAxisRotation.html
   and
   http://www.siggraph.org/education/materials/HyperGraph/modeling/mod_tran/3drota.htm
   and http://astronomy.swin.edu.au/~pbourke/geometry/rotate/.
																					   */
VECTOR_METHOD( void, RotateAround, ( PTRANSFORM pt, PC_POINT p, RCOORD amount ) );
/* Sets the current 'up' axis of a transformation. The forward
   axis is adjusted so that it remains perpendicular to the mast
   axis vs the right axis. After the forward axis is updated,
   the right axis is adjusted to be perpendicular to up and
   forward.
   Parameters
   pt :   transform to set the up direction of
   vup :  new direction for 'up'
   
   Remarks
   RotateMast is based on the idea that your current frame is
   something like a boat. As the boat moves along a surface,
   it's version of 'up' may change depending on the landscape. This
   keeps up up. (Actually, the computation was used for an
   object running along the interior of a sphere, and this
   normalizes their 'up' to the center of the sphere.               */
VECTOR_METHOD( void, RotateMast, ( PTRANSFORM pt, PCVECTOR vup ) );
/* Rotates around the 'up' of the current rotation matrix. Same
   as a yaw rotation.
   Parameters
   pt :     transformation to rotate
   angle :  angle to rotate \- positive should be clockwise,
			looking from top down.                              */
VECTOR_METHOD( void, RotateAroundMast, ( PTRANSFORM pt, RCOORD angle ) );

/* Recovers a transformation state from a file.
   Parameters
   pt :        transform to read into
   filename :  filename with the transform in it. */
VECTOR_METHOD( void, LoadTransform, ( PTRANSFORM pt, CTEXTSTR filename ) );
/* Provides a way to save a matrix ( direct binary file dump)
   Parameters
   pt :        transform matrix to save
   filename :  \file to save the transformation in.           */
VECTOR_METHOD( void, SaveTransform, ( PTRANSFORM pt, CTEXTSTR filename ) );


VECTOR_METHOD( void, RotateTo, ( PTRANSFORM pt, PCVECTOR vforward, PCVECTOR vright ) );
VECTOR_METHOD( void, RotateRight, ( PTRANSFORM pt, int A1, int A2 ) );

VECTOR_METHOD( void, Apply           , ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) );
VECTOR_METHOD( void, ApplyR          , ( PCTRANSFORM pt, PRAY dest, PRAY src ) );
VECTOR_METHOD( void, ApplyT          , ( PCTRANSFORM pt, PTRANSFORM dest, PCTRANSFORM src ) );

/* row major multiplication
 ptd is destination
 pts is matrix to transform
 pt is matrix to transform pts with
 rows of pt apply to columns of pts
 */
VECTOR_METHOD( void, ApplyM          , ( PMatrix pt, PMatrix ptd, PMatrix pts ) );
/* column major multiplication
 ptd is destination
 pts is matrix to transform
 pt is matrix to transform pts with
 rows of pt apply to columns of pts

 */
VECTOR_METHOD( void, ApplyMcm        , ( PMatrix pt, PMatrix ptd, PMatrix pts ) );

// I know this was a result - unsure how it was implented...
//void ApplyT              (PTRANFORM pt, PTRANSFORM pt1, PTRANSFORM pt2 );

VECTOR_METHOD( void, ApplyInverse    , ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) );
VECTOR_METHOD( void, ApplyInverseR   , ( PCTRANSFORM pt, PRAY dest, PRAY src ) );
VECTOR_METHOD( void, ApplyInverseT   , ( PCTRANSFORM pt, PTRANSFORM dest, PCTRANSFORM src ) );
// again note there was a void ApplyInverseT

VECTOR_METHOD( void, ApplyRotation        , ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) );
VECTOR_METHOD( void, ApplyRotationT       , ( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts ) );

VECTOR_METHOD( void, ApplyInverseRotation , ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) );

VECTOR_METHOD( void, ApplyInverseRotationT, ( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts ) );

VECTOR_METHOD( void, ApplyTranslation     , ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) );
VECTOR_METHOD( void, ApplyTranslationT    , ( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts ) );

VECTOR_METHOD( void, ApplyInverseTranslation, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) );
VECTOR_METHOD( void, ApplyInverseTranslationT, ( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts ) );

// after Move() these callbacks are invoked.
typedef void (*MotionCallback)( uintptr_t, PTRANSFORM );
/* When Move is called on the transform, these callbacks are
   invoked so user code can get even update for motion.
   Parameters
   pt :        PTRANSFORM transform matrix to hook to
   callback :  user callback routine
   psv :       pointer size value data to be passed to user
			   callback routine.                             */
VECTOR_METHOD( void, AddTransformCallback, ( PTRANSFORM pt, MotionCallback callback, uintptr_t psv ) );

/* Set the speed vector used when Move is applied to a
   PTRANSFORM.
   Parameters
   pt :  transform to set the current speed of.
   s :   the speed vector to set.                      */
VECTOR_METHOD( PC_POINT, SetSpeed, ( PTRANSFORM pt, PC_POINT s ) );
VECTOR_METHOD( P_POINT, GetSpeed, ( PTRANSFORM pt, P_POINT s ) );
/* Sets the acceleration applied to the speed when Move is
   called.                                                 */
VECTOR_METHOD( PC_POINT, SetAccel, ( PTRANSFORM pt, PC_POINT s ) );
VECTOR_METHOD( P_POINT, GetAccel, ( PTRANSFORM pt, P_POINT s ) );
/* Sets the forward direction speed in a PTRANSFORM.
   Parameters
   pt :        Transform to set the right speed of.
   distance :  How far it should go in the next time interval. */
VECTOR_METHOD( void, Forward, ( PTRANSFORM pt, RCOORD distance ) );
VECTOR_METHOD( void, MoveForward, ( PTRANSFORM pt, RCOORD distance ) );
VECTOR_METHOD( void, MoveRight, ( PTRANSFORM pt, RCOORD distance ) );
VECTOR_METHOD( void, MoveUp, ( PTRANSFORM pt, RCOORD distance ) );
/* Sets the up direction speed in a PTRANSFORM.
   Parameters
   pt :        Transform to set the right speed of.
   distance :  How far it should go in the next time interval. */
VECTOR_METHOD( void, Up, ( PTRANSFORM pt, RCOORD distance ) );
/* Sets the right direction speed in a PTRANSFORM.
   Parameters
   pt :        Transform to set the right speed of.
   distance :  How far it should go in the next time interval. */
VECTOR_METHOD( void, Right, ( PTRANSFORM pt, RCOORD distance ) );
VECTOR_METHOD( PC_POINT, SetRotation, ( PTRANSFORM pt, PC_POINT r ) );
VECTOR_METHOD( P_POINT, GetRotation, ( PTRANSFORM pt, P_POINT r ) );
VECTOR_METHOD( PC_POINT, SetRotationAccel, ( PTRANSFORM pt, PC_POINT r ) );
/* Set how long it takes, in milliseconds, to move 1 unit of
   speed vector or rotate 1 unit of rotation vector when Move is
   called. Each matrix maintains a last tick. If many thousands
   of matrixes were used, probably a batch move could be
   implemented that would maintain tick counts for a group of
   matrixes... don't know how long it takes to compute move, but
   timeGetTime will slow it down a lot.
   
   
   Parameters
   pt :                 transform to set the time interval on.
   speed_interval :     what the time interval should be for
						speed.
   rotation_interval :  what the time interval should be for
						rotation.
   Remarks
   A default interval of 1000 is used. So it will take 1000
   milliseconds to move one unit of speed. This could be set to
   3600000 and then it would take one hour to move one unit of
   speed. (miles per hour)
   
   
   
   Rotation has its own interval that affects rotation the same
   way; If your rotation was set to roll 2*pi radians, then it
   would revolve one full rotation in the said time.
   
   
																 */
VECTOR_METHOD( void, SetTimeInterval, ( PTRANSFORM pt, RCOORD speed_interval, RCOORD rotation_interval ) );

/* Updates a transform by it's current speed and rotation
   assuming speed and rotation are specified in x per 1 second.
   Applies the fraction of time between now and the prior time
   move was called and scales speed and rotation by that when
   applying it.
   
   
   Parameters
   pt :  Pointer to a transform to update.
   
   See Also
   <link sack::math::vector::SetTimeInterval@PTRANSFORM@RCOORD@RCOORD, SetTimeInterval> */
VECTOR_METHOD( LOGICAL, Move, ( PTRANSFORM pt ) );
#if 0
	VECTOR_METHOD( void, Unmove, ( PTRANSFORM pt ) );
#endif

VECTOR_METHOD( void, showstdEx, ( PTRANSFORM pt, const char *header ) );  
VECTOR_METHOD( void, ShowTransformEx, ( PTRANSFORM pt, const char *header DBG_PASS ) );  
/* <combine sack::math::vector::ShowTransformEx@PTRANSFORM@char *header>
   
   \ \                                                                   */
#define ShowTransform( n ) ShowTransformEx( n, #n DBG_SRC )
VECTOR_METHOD( void, showstd, ( PTRANSFORM pt, const char *header ) );  


VECTOR_METHOD( void, GetOriginV, ( PTRANSFORM pt, P_POINT o ) ); 
VECTOR_METHOD( PC_POINT, GetOrigin, ( PTRANSFORM pt ) ); 

VECTOR_METHOD( void, GetAxisV, ( PTRANSFORM pt, P_POINT a, int n ) ); 
VECTOR_METHOD( PC_POINT, GetAxis, ( PTRANSFORM pt, int n ) ); 

VECTOR_METHOD( void, SetAxis, ( PTRANSFORM pt, RCOORD a, RCOORD b, RCOORD c, int n ) ); 
VECTOR_METHOD( void, SetAxisV, ( PTRANSFORM pt, PC_POINT a, int n ) ); 

// matrix is suitable to set as the first matrix for opengl rendering.
// it is an inverse application that uses the transform's origin as camera origin
// and the rotation matrix as what to look at.
VECTOR_METHOD( void, GetGLCameraMatrix, ( PTRANSFORM pt, PMATRIX out ) );

VECTOR_METHOD( void, GetGLMatrix, ( PTRANSFORM pt, PMATRIX out ) );
VECTOR_METHOD( void, SetGLMatrix, ( PMATRIX in, PTRANSFORM pt ) );

VECTOR_METHOD( void, SetRotationMatrix, ( PTRANSFORM pt, RCOORD *quat ) );
VECTOR_METHOD( void, GetRotationMatrix, ( PTRANSFORM pt, RCOORD *quat ) );
VECTOR_METHOD( RCOORD, IntersectLineWithPlane, (PCVECTOR Slope, PCVECTOR Origin,  // line m, b
	PCVECTOR n, PCVECTOR o,  // plane n, o
	RCOORD *time) );
VECTOR_METHOD( RCOORD, PointToPlaneT, (PCVECTOR n, PCVECTOR o, PCVECTOR p) );

/* convert basis matrix to rotation vector v */
VECTOR_METHOD( void, basis_lq, (PVECTOR4 v4, PMatrix basis) );

/* convert rotation vector v to quaternion q */
VECTOR_METHOD( void, lq_exp, (PVECTOR4 q, PCVECTOR4 v) );

/* convert rotation vector v to basismatrix  */
VECTOR_METHOD( PMatrix, lq_basis, ( PMatrix matrix, PCVECTOR4 v ) );

/* convert rotation vector v to basismatrix
   sets the origin, so the resulting matrix is a valid view matrix for Vulkan
 */
VECTOR_METHOD( PMatrix, lq_matrix, ( PMatrix matrix, PCVECTOR4 v, PCVECTOR position ) );

/* use a mouse sort of left-right/updown (yaw/pitch) input to update the orientation.
 This has no restrictions on the resulting output, and the rotation is already relative to the
 current viewpoint.
 put the output in out, and return out.
 */
VECTOR_METHOD( PVECTOR4, lq_free_look, ( PVECTOR4 out, PCVECTOR4 orientation, RCOORD pitch, RCOORD yaw, RCOORD roll ) );

/* use a mouse sort of left-right/updown (yaw/pitch) input to update the orientation.
 This restricts the roll, expecting you're on a level ground; whereas the free-look is more suited to space.
 put the output in out, and return out.
 k is the factor to adjust the roll by; 1.0 is immediate fix...
 */
VECTOR_METHOD( PVECTOR4, lq_level_look, ( PVECTOR4 out, PCVECTOR4 orientation, RCOORD pitch, RCOORD yaw, RCOORD k ) );

/* convert rotation vector v to colmajor (opengl) basismatrix
 this is probably redundant - native matrix is the correct direction already...

 */
VECTOR_METHOD( PMatrix, lq_gl_basis, ( PMatrix matrix, PCVECTOR4 v ) );

/* get a vector representing the up (y) direction from a rotation vector */
VECTOR_METHOD( PVECTOR, lq_up, ( PVECTOR out, PCVECTOR4 r ) );

/* get a vector representing the right (x) direction from a rotation vector */
VECTOR_METHOD( PVECTOR, lq_right, ( PVECTOR out, PCVECTOR4 r ) );

/* get a vector representing the forward (z) direction from a rotation vector */
VECTOR_METHOD( PVECTOR, lq_forward, ( PVECTOR out, PCVECTOR4 r ) );

/* get how much roll there is from the identity matrix orientation 
 * The raw rotation angles of the axis-angle vector has a complex interaction over 
 * 1 tick that results in an orientation, and it is that resulting orientation that
 * determines the roll, yaw, and pitch. So the values returned here are not
 * just the components of the axis-angle vector.
 */
VECTOR_METHOD( RCOORD, lq_roll, ( PCVECTOR4 r ) );

/* get how much yaw there is from the identity matrix orientation
 * The raw rotation angles of the axis-angle vector has a complex interaction over
 * 1 tick that results in an orientation, and it is that resulting orientation that
 * determines the roll, yaw, and pitch. So the values returned here are not
 * just the components of the axis-angle vector.
 * 
 * this function does attempt to return +/- 360 degrees, which is closer to the spin angles.
 */
VECTOR_METHOD( RCOORD, lq_yaw, ( PCVECTOR4 r ) );

/* get how much pitch there is from the identity matrix orientation
 * The raw rotation angles of the axis-angle vector has a complex interaction over
 * 1 tick that results in an orientation, and it is that resulting orientation that
 * determines the roll, yaw, and pitch. So the values returned here are not
 * just the components of the axis-angle vector.
 */
VECTOR_METHOD( RCOORD, lq_pitch, ( PCVECTOR4 r ) );

// apply the rotation A to R and store in OUT
VECTOR_METHOD( PVECTOR4, lq_applyRotation, ( PVECTOR4 out, PCVECTOR4 r, PCVECTOR4 a ) );
// rotate vector V around R and store in out  (out and V could be the same?)
VECTOR_METHOD( void, lq_apply, ( PVECTOR out, PCVECTOR4 r, PCVECTOR v ) );

VECTOR_METHOD( PVECTOR4, lq_normalize, ( PVECTOR4 out ) );
// cross product between two vectors; result as a rotation vector 
// this is the rotation that rotates one to the other.
VECTOR_METHOD( PVECTOR4, lq_cross, ( PVECTOR4 out, PCVECTOR a, PCVECTOR b ) );
VECTOR_METHOD( PVECTOR4, lq_set, ( PVECTOR4 out, RCOORD x, RCOORD y, RCOORD z, RCOORD angle ) );

/* sets rotation vector from pitch and roll (?) */
VECTOR_METHOD( PVECTOR4, lq_set_xy, ( PVECTOR4 out, RCOORD x, RCOORD z ) );
/* sets the rotataion vector, and normalizes it into axis&angle internal format */
VECTOR_METHOD( PVECTOR4, lq_set3, ( PVECTOR4 out, RCOORD x, RCOORD y, RCOORD z ) );
/* normalizes x,y,z axis part, and uses the specified angle */
VECTOR_METHOD( PVECTOR4, lq_set4, ( PVECTOR4 out, RCOORD x, RCOORD y, RCOORD z, RCOORD angle ) );
/* treats input as latitude and longitude on a sphere.
 Lat +/- 90  (+/-360)
 lng +/- 180 (+/-360)

 This will result in rotation coordinates that are unique for the specified over-latitude range.
 */
VECTOR_METHOD( PVECTOR4, lq_set_latlong, ( PVECTOR4 out, RCOORD lat, RCOORD lng ) );

#if ( !defined( VECTOR_LIBRARY_SOURCE ) && !defined( NO_AUTO_VECTLIB_NAMES ) ) || defined( NEED_VECTLIB_ALIASES )
#define add EXTERNAL_NAME(add)
#define sub EXTERNAL_NAME(sub)
#define scale EXTERNAL_NAME(scale)
#define Scale EXTERNAL_NAME(Scale)
#define Invert EXTERNAL_NAME(Invert)
#define GetOrigin EXTERNAL_NAME(GetOrigin)
#define GetOriginV EXTERNAL_NAME(GetOriginV)
#define GetAxis EXTERNAL_NAME(GetAxis)
#define GetAxisV EXTERNAL_NAME(GetAxisV)
#define GetGLCameraMatrix EXTERNAL_NAME(GetGLCameraMatrix)
#define ApplyInverse EXTERNAL_NAME(ApplyInverse)

#define Move EXTERNAL_NAME(Move)
#define MoveForward EXTERNAL_NAME(MoveForward)
#define MoveRight EXTERNAL_NAME(MoveRight)
#define MoveUp EXTERNAL_NAME(MoveUp)

#define Forward EXTERNAL_NAME(Forward)
#define Right EXTERNAL_NAME(Right)
#define Up EXTERNAL_NAME(Up)

#define PrintVectorEx EXTERNAL_NAME(PrintVectorEx)
#define PrintMatrixEx EXTERNAL_NAME(PrintMatrixEx)
#define ShowTransformEx EXTERNAL_NAME(ShowTransformEx)



#define addscaled EXTERNAL_NAME(addscaled)
#define Length EXTERNAL_NAME(Length)
#define PointToPlaneT EXTERNAL_NAME(PointToPlaneT)
#define normalize EXTERNAL_NAME(normalize)
#define normalize4 EXTERNAL_NAME(normalize4)
#define Translate EXTERNAL_NAME(Translate)
#define TranslateV EXTERNAL_NAME(TranslateV)
#define Apply EXTERNAL_NAME(Apply)
#define ApplyR EXTERNAL_NAME(ApplyR)
#define ApplyT EXTERNAL_NAME(ApplyT)
#define ApplyM EXTERNAL_NAME(ApplyM)
#define ApplyTranslation EXTERNAL_NAME(ApplyTranslation)
#define ApplyTranslationR EXTERNAL_NAME(ApplyTranslationR)
#define ApplyTranslationT EXTERNAL_NAME(ApplyTranslationT)
#define ApplyInverseRotation EXTERNAL_NAME(ApplyInverseRotation)
#define ApplyInverseR EXTERNAL_NAME(ApplyInverseR)
#define ApplyRotation EXTERNAL_NAME(ApplyRotation)
#define ApplyRotationT EXTERNAL_NAME(ApplyRotationT)
#define RotateAround EXTERNAL_NAME(RotateAround)
#define RotateRel EXTERNAL_NAME(RotateRel)
#define SetRotation EXTERNAL_NAME(SetRotation)
#define GetRotation EXTERNAL_NAME(GetRotation)
#define SetRotationAccel EXTERNAL_NAME(SetRotationAccel)
#define RotateRight EXTERNAL_NAME(RotateRight)
#define dotproduct EXTERNAL_NAME(dotproduct)
#define DestroyTransform EXTERNAL_NAME(DestroyTransform)
#define SinAngle EXTERNAL_NAME(SinAngle)
#define CosAngle EXTERNAL_NAME(CosAngle)
#define crossproduct EXTERNAL_NAME(crossproduct)
#define CreateTransformMotion EXTERNAL_NAME(CreateTransformMotion)
#define CreateTransformMotionEx EXTERNAL_NAME(CreateTransformMotionEx)
#define CreateNamedTransform EXTERNAL_NAME(CreateNamedTransform)
#define ClearTransform EXTERNAL_NAME(ClearTransform)
#define RotateTo EXTERNAL_NAME(RotateTo)
#define SetSpeed EXTERNAL_NAME(SetSpeed)
#define SetAccel EXTERNAL_NAME(SetAccel)
#define TranslateRel EXTERNAL_NAME( TranslateRel )
#define TranslateRelV EXTERNAL_NAME( TranslateRelV )
#define RotateAbs EXTERNAL_NAME(RotateAbs)
#define RotateAbsV EXTERNAL_NAME(RotateAbsV)
#define GetRotationMatrix EXTERNAL_NAME(GetRotationMatrix)
#define SetRotationMatrix EXTERNAL_NAME(SetRotationMatrix)

#define basis_lq EXTERNAL_NAME(basis_lq)
#define lq_exp EXTERNAL_NAME(lq_exp)
#define lq_basis EXTERNAL_NAME(lq_basis)
#define lq_gl_basis EXTERNAL_NAME(lq_gl_basis)
#define lq_up EXTERNAL_NAME(lq_up)
#define lq_right EXTERNAL_NAME(lq_right)
#define lq_forward EXTERNAL_NAME(lq_forward)
#define lq_roll EXTERNAL_NAME(lq_roll)
#define lq_yaw EXTERNAL_NAME(lq_yaw)
#define lq_pitch EXTERNAL_NAME(lq_pitch)
#define lq_normalize EXTERNAL_NAME(lq_normalize)
#define lq_cross EXTERNAL_NAME(lq_cross)
#define lq_set EXTERNAL_NAME(lq_set)
#define lq_set3 EXTERNAL_NAME(lq_set3)
#define lq_set4 EXTERNAL_NAME(lq_set4)
#define lq_set_xy EXTERNAL_NAME(lq_set_xy)
#define lq_set_latlong EXTERNAL_NAME(lq_set_latlong)
#define lq_applyRotation EXTERNAL_NAME(lq_applyRotation)
#define lq_apply EXTERNAL_NAME(lq_apply)
#define lq_matrix EXTERNAL_NAME(lq_matrix)
#define lq_free_look EXTERNAL_NAME(lq_free_look)
#define lq_level_look EXTERNAL_NAME(lq_level_look)
//#define lq_ EXTERNAL_NAME(lq_apply)

#endif

#ifdef __cplusplus
VECTOR_NAMESPACE_END
USE_VECTOR_NAMESPACE
#endif
#endif
// $Log: vectlib.h,v $
// Revision 1.13  2004/08/22 09:56:41  d3x0r
// checkpoint...
//
// Revision 1.12  2004/02/02 22:43:35  d3x0r
// Add lineseg type and orthoarea (min/max box)
//
// Revision 1.11  2004/01/11 23:24:15  panther
// Fix type warnings, conflicts, fix const issues
//
// Revision 1.10  2004/01/11 23:11:49  panther
// Fix const typings
//
// Revision 1.9  2004/01/11 23:10:38  panther
// Include keyboard to avoid windows errors
//
// Revision 1.8  2004/01/04 20:54:18  panther
// Use PCTRANSFORM for prototypes
//
// Revision 1.7  2003/12/29 08:10:18  panther
// Added more functions for applying transforms
//
// Revision 1.6  2003/11/22 23:27:11  panther
// Fix type passed to printvector
//
// Revision 1.5  2003/09/01 20:04:37  panther
// Added OpenGL Interface to windows video lib, Modified RCOORD comparison
//
// Revision 1.4  2003/03/25 08:38:11  panther
// Add logging
//
