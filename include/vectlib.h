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
#    if defined( __WATCOMC__ ) || defined( _MSC_VER )
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

#if defined( _D3D_DRIVER ) || ( !defined( __cplusplus ) && defined( __ANDROID__ ) )
#define MAKE_RCOORD_SINGLE
#endif

#ifdef __cplusplus
#ifndef MAKE_RCOORD_SINGLE
#define VECTOR_NAMESPACE SACK_NAMESPACE namespace math { namespace vector { namespace Double {
#define _MATH_VECTOR_NAMESPACE namespace math { namespace vector { namespace Double {
#define _VECTOR_NAMESPACE namespace vector { namespace Double {
#define USE_VECTOR_NAMESPACE using namespace sack::math::vector::Double;
#else
#define VECTOR_NAMESPACE SACK_NAMESPACE namespace math { namespace vector { namespace Float {
#define _MATH_VECTOR_NAMESPACE namespace math { namespace vector { Float {
#define _VECTOR_NAMESPACE namespace vector { namespace Float {
#define USE_VECTOR_NAMESPACE using namespace sack::math::vector::Float;
#endif
#define _MATH_NAMESPACE namespace math {
#define VECTOR_NAMESPACE_END } } } SACK_NAMESPACE_END
#ifdef SACK_BAG_EXPORTS

#ifndef MAKE_RCOORD_SINGLE
#define USE_VECTOR_NAMESPACE using namespace sack::math::vector::Double;
#else
#ifndef USE_VECTOR_NAMESPACE 
#define USE_VECTOR_NAMESPACE using namespace sack::math::vector;
#endif
#endif
#endif
#else
#define VECTOR_NAMESPACE
#define _MATH_VECTOR_NAMESPACE
#define _MATH_NAMESPACE
#define _VECTOR_NAMESPACE
#define VECTOR_NAMESPACE_END
#define USE_VECTOR_NAMESPACE 
#endif


#include <vectypes.h>

SACK_NAMESPACE
	_MATH_NAMESPACE
	/* Vector namespace contains methods for operating on vectors. Vectors
	   are multi-dimensional scalar quantities, often used to
	   represent coordinates and directions in space.                      */
   _VECTOR_NAMESPACE

//#include "../src/vectlib/vecstruc.h"



typedef RCOORD _POINT[DIMENSIONS];
/* pointer to a point. */
typedef RCOORD *P_POINT;
/* pointer to a constant point. */
typedef const RCOORD *PC_POINT;

/* A vector type. Contains 3 values by default, library can
   handle 4 dimensional transformations(?)                  */
typedef _POINT VECTOR;
/* pointer to a vector. */
typedef P_POINT PVECTOR;
/* pointer to a constant vector. */
typedef PC_POINT PCVECTOR;

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
#define M_PI (3.1415926535)
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

// should end up layering this macro based on DIMENSIONS
#define SetPoint( d, s ) ( (d)[0] = (s)[0], (d)[1]=(s)[1], (d)[2]=(s)[2] )
/* Inverts a vector. that is vector * -1. (a,b,c) = (-a,-b,-c)
   
   <b>Parameters</b>
                                                               */
MATHLIB_EXPORT P_POINT Invert( P_POINT a );
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
MATHLIB_EXPORT void PrintVectorEx( CTEXTSTR lpName, PCVECTOR v DBG_PASS );
/* <combine sack::math::vector::PrintVectorEx@CTEXTSTR@PCVECTOR v>
   
   \ \                                                               */
#define PrintVector(v) PrintVectorEx( WIDE(#v), v DBG_SRC )
/* Same as PrintVectorEx, but prints to standard output using
   printf.                                                    */
MATHLIB_EXPORT void PrintVectorStdEx( CTEXTSTR lpName, VECTOR v DBG_PASS );
/* <combine sack::math::vector::PrintVectorStdEx@CTEXTSTR@VECTOR v>
   
   \ \                                                                */
#define PrintVectorStd(v) PrintVectorStd( WIDE(#v), v DBG_SRC )
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
MATHLIB_EXPORT void PrintMatrixEx( CTEXTSTR lpName, MATRIX m DBG_PASS );
/* <combine sack::math::vector::PrintMatrixEx@CTEXTSTR@MATRIX m>
   
   \ \                                                             */
#define PrintMatrix(m) PrintMatrixEx( WIDE(#m), m DBG_SRC )

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

//------ Constants for origin(0,0,0), and axii
#ifndef VECTOR_LIBRARY_SOURCE
MATHLIB_DEXPORT const PC_POINT VectorConst_0;
/* Specifies the coordinate system's X axis direction. static
   constant.                                                  */
MATHLIB_DEXPORT const PC_POINT VectorConst_X;
/* Specifies the coordinate system's Y axis direction. static
   constant.                                                  */
MATHLIB_DEXPORT const PC_POINT VectorConst_Y;
/* Specifies the coordinate system's Z axis direction. static
   constant.                                                  */
MATHLIB_DEXPORT const PC_POINT VectorConst_Z;
/* This is a static constant identity matrix, which can be used
   to initialize a matrix transform (internally).               */
MATHLIB_DEXPORT const PCTRANSFORM VectorConst_I;
#define _0 ((PC_POINT)VectorConst_0)
#define _X ((PC_POINT)VectorConst_X)
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
MATHLIB_EXPORT P_POINT add( P_POINT pr, PC_POINT pv1, PC_POINT pv2 );
/* subtracts two vectors and stores the result in another
   vector.
   Remarks
   The result vector may be the same as one of the source
   vectors.                                               */
MATHLIB_EXPORT P_POINT sub( P_POINT pr, PC_POINT pv1, PC_POINT pv2 );
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
MATHLIB_EXPORT P_POINT scale( P_POINT pr, PC_POINT pv1, RCOORD k );
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
MATHLIB_EXPORT P_POINT addscaled( P_POINT pr, PC_POINT pv1, PC_POINT pv2, RCOORD k );
/* Normalizes a non-zero vector. That is the resulting length of
   the vector is 1.0. Modifies the vector in place.              */
MATHLIB_EXPORT void normalize( P_POINT pv );
MATHLIB_EXPORT void crossproduct( P_POINT pr, PC_POINT pv1, PC_POINT pv2 );
/* \Returns the sin of the angle between two vectors.
   Parameters
   pv1 :  one vector
   pv2 :  another vector
   
   Remarks
   If the length of either vector is 0, then a divide by zero
   exception will happen.                                     */
MATHLIB_EXPORT RCOORD SinAngle( PC_POINT pv1, PC_POINT pv2 );
/* \Returns the cos (cosine) of the angle between two vectors.
   Parameters
   pv1 :  one vector
   pv2 :  another vector
   
   Remarks
   If the length of either vector is 0, then a divide by zero
   exception will happen.                                      */
MATHLIB_EXPORT RCOORD CosAngle( PC_POINT pv1, PC_POINT pv2 );
MATHLIB_EXPORT RCOORD dotproduct( PC_POINT pv1, PC_POINT pv2 );
// result is the projection of project onto onto
MATHLIB_EXPORT P_POINT project( P_POINT pr, PC_POINT onto, PC_POINT project );
/* \Returns the scalar length of a vector. */
MATHLIB_EXPORT RCOORD Length( PC_POINT pv );
/* \Returns the distance between two points.
   
   
   Parameters
   v1 :  some point
   v2 :  another point
   
   Returns
   The distance between the two points.      */
MATHLIB_EXPORT RCOORD Distance( PC_POINT v1, PC_POINT v2 );
/* \Returns the distance a point is as projected on another
   vector. The result is the distance along that vector from the
   origin.
   Parameters
   pvOn :  Vector to project on
   pvOf :  vector to get projection length of.                   */
MATHLIB_EXPORT RCOORD DirectedDistance( PC_POINT pvOn, PC_POINT pvOf );

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
MATHLIB_EXPORT PTRANSFORM CreateNamedTransform ( CTEXTSTR name );
#define CreateTransform() CreateNamedTransform( NULL )
MATHLIB_EXPORT PTRANSFORM CreateTransformMotion( PTRANSFORM pt );
MATHLIB_EXPORT PTRANSFORM CreateTransformMotionEx( PTRANSFORM pt, int rocket );
MATHLIB_EXPORT void DestroyTransform      ( PTRANSFORM pt );
/* Resets a transform back to initial conitions. */
MATHLIB_EXPORT void ClearTransform        ( PTRANSFORM pt );
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
MATHLIB_EXPORT void InvertTransform        ( PTRANSFORM pt );
MATHLIB_EXPORT void Scale                 ( PTRANSFORM pt, RCOORD sx, RCOORD sy, RCOORD sz );

MATHLIB_EXPORT void Translate             ( PTRANSFORM pt, RCOORD tx, RCOORD ty, RCOORD tz );
MATHLIB_EXPORT void TranslateV            ( PTRANSFORM pt, PC_POINT t );
MATHLIB_EXPORT void TranslateRel          ( PTRANSFORM pt, RCOORD tx, RCOORD ty, RCOORD tz );
MATHLIB_EXPORT void TranslateRelV         ( PTRANSFORM pt, PC_POINT t );


MATHLIB_EXPORT void RotateAbs( PTRANSFORM pt, RCOORD rx, RCOORD ry, RCOORD rz );
MATHLIB_EXPORT void RotateAbsV( PTRANSFORM pt, PC_POINT );
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
MATHLIB_EXPORT void RotateRel( PTRANSFORM pt, RCOORD rx, RCOORD ry, RCOORD rz );
/* Update a transformation matrix by relative degress about the
   x axix, y axis and z axis.
   Parameters
   pt :  transform to update
   v :   vector containing x,y and z relative roll coordinate.  */
MATHLIB_EXPORT void RotateRelV( PTRANSFORM pt, PC_POINT );
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
MATHLIB_EXPORT void RotateAround( PTRANSFORM pt, PC_POINT p, RCOORD amount );
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
MATHLIB_EXPORT void RotateMast( PTRANSFORM pt, PCVECTOR vup );
/* Rotates around the 'up' of the current rotation matrix. Same
   as a yaw rotation.
   Parameters
   pt :     transformation to rotate
   angle :  angle to rotate \- positive should be clockwise,
            looking from top down.                              */
MATHLIB_EXPORT void RotateAroundMast( PTRANSFORM pt, RCOORD angle );

/* Recovers a transformation state from a file.
   Parameters
   pt :        transform to read into
   filename :  filename with the transform in it. */
MATHLIB_EXPORT void LoadTransform( PTRANSFORM pt, CTEXTSTR filename );
/* Provides a way to save a matrix ( direct binary file dump)
   Parameters
   pt :        transform matrix to save
   filename :  \file to save the transformation in.           */
MATHLIB_EXPORT void SaveTransform( PTRANSFORM pt, CTEXTSTR filename );


MATHLIB_EXPORT void RotateTo( PTRANSFORM pt, PCVECTOR vforward, PCVECTOR vright );
MATHLIB_EXPORT void RotateRight( PTRANSFORM pt, int A1, int A2 );

MATHLIB_EXPORT void Apply           ( PCTRANSFORM pt, P_POINT dest, PC_POINT src );
MATHLIB_EXPORT void ApplyR          ( PCTRANSFORM pt, PRAY dest, PRAY src );
MATHLIB_EXPORT void ApplyT          ( PCTRANSFORM pt, PTRANSFORM dest, PTRANSFORM src );
// I know this was a result - unsure how it was implented...
//void ApplyT              (PTRANFORM pt, PTRANSFORM pt1, PTRANSFORM pt2 );

MATHLIB_EXPORT void ApplyInverse    ( PCTRANSFORM pt, P_POINT dest, PC_POINT src );
MATHLIB_EXPORT void ApplyInverseR   ( PCTRANSFORM pt, PRAY dest, PRAY src );
MATHLIB_EXPORT void ApplyInverseT   ( PCTRANSFORM pt, PTRANSFORM dest, PTRANSFORM src );
// again note there was a void ApplyInverseT

MATHLIB_EXPORT void ApplyRotation        ( PCTRANSFORM pt, P_POINT dest, PC_POINT src );
MATHLIB_EXPORT void ApplyRotationT       ( PCTRANSFORM pt, PTRANSFORM ptd, PTRANSFORM pts );

MATHLIB_EXPORT void ApplyInverseRotation ( PCTRANSFORM pt, P_POINT dest, PC_POINT src );

MATHLIB_EXPORT void ApplyInverseRotationT( PCTRANSFORM pt, PTRANSFORM ptd, PTRANSFORM pts );

MATHLIB_EXPORT void ApplyTranslation     ( PCTRANSFORM pt, P_POINT dest, PC_POINT src );
MATHLIB_EXPORT void ApplyTranslationT    ( PCTRANSFORM pt, PTRANSFORM ptd, PTRANSFORM pts );

MATHLIB_EXPORT void ApplyInverseTranslation( PCTRANSFORM pt, P_POINT dest, PC_POINT src );
MATHLIB_EXPORT void ApplyInverseTranslationT( PCTRANSFORM pt, PTRANSFORM ptd, PTRANSFORM pts );

// after Move() these callbacks are invoked.
typedef void (*MotionCallback)( PTRSZVAL, PTRANSFORM );
/* When Move is called on the transform, these callbacks are
   invoked so user code can get even update for motion.
   Parameters
   pt :        PTRANSFORM transform matrix to hook to
   callback :  user callback routine
   psv :       pointer size value data to be passed to user
               callback routine.                             */
MATHLIB_EXPORT void AddTransformCallback( PTRANSFORM pt, MotionCallback callback, PTRSZVAL psv );

/* Set the speed vector used when Move is applied to a
   PTRANSFORM.
   Parameters
   pt :  transform to set the current speed of.
   s :   the speed vector to set.                      */
MATHLIB_EXPORT PC_POINT SetSpeed( PTRANSFORM pt, PC_POINT s );
MATHLIB_EXPORT P_POINT GetSpeed( PTRANSFORM pt, P_POINT s );
/* Sets the acceleration applied to the speed when Move is
   called.                                                 */
MATHLIB_EXPORT PC_POINT SetAccel( PTRANSFORM pt, PC_POINT s );
MATHLIB_EXPORT P_POINT GetAccel( PTRANSFORM pt, P_POINT s );
/* Sets the forward direction speed in a PTRANSFORM.
   Parameters
   pt :        Transform to set the right speed of.
   distance :  How far it should go in the next time interval. */
MATHLIB_EXPORT void Forward( PTRANSFORM pt, RCOORD distance );
MATHLIB_EXPORT void MoveForward( PTRANSFORM pt, RCOORD distance );
MATHLIB_EXPORT void MoveRight( PTRANSFORM pt, RCOORD distance );
MATHLIB_EXPORT void MoveUp( PTRANSFORM pt, RCOORD distance );
/* Sets the up direction speed in a PTRANSFORM.
   Parameters
   pt :        Transform to set the right speed of.
   distance :  How far it should go in the next time interval. */
MATHLIB_EXPORT void Up( PTRANSFORM pt, RCOORD distance );
/* Sets the right direction speed in a PTRANSFORM.
   Parameters
   pt :        Transform to set the right speed of.
   distance :  How far it should go in the next time interval. */
MATHLIB_EXPORT void Right( PTRANSFORM pt, RCOORD distance );
MATHLIB_EXPORT PC_POINT SetRotation( PTRANSFORM pt, PC_POINT r );
MATHLIB_EXPORT P_POINT GetRotation( PTRANSFORM pt, P_POINT r );
MATHLIB_EXPORT PC_POINT SetRotationAccel( PTRANSFORM pt, PC_POINT r );
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
MATHLIB_EXPORT void SetTimeInterval( PTRANSFORM pt, RCOORD speed_interval, RCOORD rotation_interval );

/* Updates a transform by it's current speed and rotation
   assuming speed and rotation are specified in x per 1 second.
   Applies the fraction of time between now and the prior time
   move was called and scales speed and rotation by that when
   applying it.
   
   
   Parameters
   pt :  Pointer to a transform to update.
   
   See Also
   <link sack::math::vector::SetTimeInterval@PTRANSFORM@RCOORD@RCOORD, SetTimeInterval> */
MATHLIB_EXPORT void Move( PTRANSFORM pt );
#if 0
	MATHLIB_EXPORT void Unmove( PTRANSFORM pt );
#endif

MATHLIB_EXPORT void showstdEx( PTRANSFORM pt, char *header );  
MATHLIB_EXPORT void ShowTransformEx( PTRANSFORM pt, char *header DBG_PASS );  
/* <combine sack::math::vector::ShowTransformEx@PTRANSFORM@char *header>
   
   \ \                                                                   */
#define ShowTransform( n ) ShowTransformEx( n, #n DBG_SRC )
MATHLIB_EXPORT void showstd( PTRANSFORM pt, char *header );  


MATHLIB_EXPORT void GetOriginV( PTRANSFORM pt, P_POINT o ); 
MATHLIB_EXPORT PC_POINT GetOrigin( PTRANSFORM pt ); 

MATHLIB_EXPORT void GetAxisV( PTRANSFORM pt, P_POINT a, int n ); 
MATHLIB_EXPORT PC_POINT GetAxis( PTRANSFORM pt, int n ); 

MATHLIB_EXPORT void SetAxis( PTRANSFORM pt, RCOORD a, RCOORD b, RCOORD c, int n ); 
MATHLIB_EXPORT void SetAxisV( PTRANSFORM pt, PC_POINT a, int n ); 

// matrix is suitable to set as the first matrix for opengl rendering.
// it is an inverse application that uses the transform's origin as camera origin
// and the rotation matrix as what to look at.
MATHLIB_EXPORT void GetGLCameraMatrix( PTRANSFORM pt, PMATRIX out );

MATHLIB_EXPORT void GetGLMatrix( PTRANSFORM pt, PMATRIX out );
MATHLIB_EXPORT void SetGLMatrix( PMATRIX in, PTRANSFORM pt );

MATHLIB_EXPORT void SetRotationMatrix( PTRANSFORM pt, RCOORD *quat );
MATHLIB_EXPORT void GetRotationMatrix( PTRANSFORM pt, RCOORD *quat );



#ifdef __cplusplus
#if 0
#include "../src/vectlib/vectstruc.h"
class TransformationMatrix {
private:
#ifndef TRANSFORM_STRUCTURE
	//char data[32*4 + 4];

//   // requires [4][4] for use with opengl
//   MATRIX m;       // s*rcos[0][0]*rcos[0][1] sin sin   (0)
//                   // sin s*rcos[1][0]*rcos[1][1] sin   (0)
//                   // sin sin s*rcos[2][0]*rcos[2][0]   (0)
//                   // tx  ty  tz                        (1)
//
//   RCOORD s[3];
//        // [x][0] [x][1] = partials... [x][2] = multiplied value.
//
//   RCOORD speed[3]; // speed right, up, forward
//   RCOORD rotation[3]; // pitch, yaw, roll delta
//   int nTime; // rotation stepping for consistant rotation
	struct transform_tag data;
#else
	struct transform_tag data;
#endif
public:
	TransformationMatrix() {
  // 	clear();
	}
	~TransformationMatrix() {} // no result.
	inline void Translate( PC_POINT o ) 
   {	TranslateV( &data, o );	}
	inline void TranslateRel( PC_POINT o ) 
	{	TranslateRelV( &data, o );	}
	inline void Translate( RCOORD x, RCOORD y, RCOORD z ) 
	{	TranslateV( &data, &x );	}
	inline void _RotateTo( PC_POINT forward, PC_POINT right )
	{	//extern void RotateTo( PTRANSFORM pt, PCVECTOR vforward, PCVECTOR vright );
		RotateTo( &data, forward, right );
	}
	inline void RotateRel( RCOORD up, RCOORD right, RCOORD forward )
	{	
		RotateRelV( &data, &up );
	}
	inline void RotateRel( PC_POINT p )
	{	
		RotateRelV( &data, p );
	}
#if 0
	inline void Apply( P_POINT dest, PC_POINT src )
	{	//void Apply( PTRANSFORM pt, P_POINT dest, PC_POINT src );
		Apply( &data, dest, src );
	}
	inline void ApplyRotation( P_POINT dest, PC_POINT src )
	{	//void ApplyRotation( PTRANSFORM pt, P_POINT dest, PC_POINT src );
		ApplyRotation( &data, dest, src );
	}
	inline void Apply( PRAY dest, PRAY src )
	{	//void ApplyR( PTRANSFORM pt, PRAY dest, PRAY src );
		ApplyR( &data, dest, src );
	}								
	inline void ApplyInverse( P_POINT dest, PC_POINT src )
	{ // void ApplyInverse( PTRANSFORM pt, P_POINT dest, PC_POINT src );
		ApplyInverse( &data, dest, src );
	}
	inline void ApplyInverse( PRAY dest, PRAY src )
	{	void ApplyInverseR( PTRANSFORM pt, PRAY dest, PRAY src );
		ApplyInverseR( &data, dest, src );
	}
	inline void ApplyInverseRotation( P_POINT dest, PC_POINT src )
	{	void ApplyInverseRotation( PTRANSFORM pt, P_POINT dest, PC_POINT src );
		ApplyInverseRotation( &data, dest, src );
	}
	inline void SetSpeed( RCOORD x, RCOORD y, RCOORD z )
	{	sack::math::vector::SetSpeed( &data, &x );
	}
	inline void SetSpeed( PC_POINT p )
	{	sack::math::vector::SetSpeed( &data, p );
	}
	inline void SetRotation( PC_POINT p )
	{	sack::math::vector::SetRotation( &data, p );
	}
	inline void Move( void )
	{
		void Move( PTRANSFORM pt );
		sack::math::vector::Move( &data );
	}
	inline void Unmove( void )
	{
		void Unmove( PTRANSFORM pt );
		sack::math::vector::Unmove( &data );
	}
	inline void clear( void )
	{
		sack::math::vector::ClearTransform( &data );
	}
	inline void RotateAbs( RCOORD x, RCOORD y, RCOORD z )
	{
		sack::math::vector::RotateAbsV( (PTRANSFORM)&data, &x );
	}
#endif
	inline void _GetAxis( P_POINT result, int n )
	{
		GetAxisV( (PTRANSFORM)&data, result, n );
	}
	inline PC_POINT _GetAxis( int n )
	{
		return GetAxis( (PTRANSFORM)&data, n);
	}
	PC_POINT _GetOrigin( void )
	{	return GetOrigin( (PTRANSFORM)&data );
	}
	void _GetOrigin( P_POINT p )
	{	GetOriginV( &data, p );
	}
	void show( char *header )
	{
		ShowTransformEx( &data, header DBG_SRC );
	}
	void _RotateRight( int a, int b )
	{
		RotateRight( (PTRANSFORM)&data, a, b );
	}
};
#endif
#endif
VECTOR_NAMESPACE_END
#ifndef USE_VECTOR_NAMESPACE
	#pragma note(" see I was here" )
#endif
USE_VECTOR_NAMESPACE
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
