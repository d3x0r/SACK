#define VECTOR_LIBRARY_SOURCE
#include <stdhdrs.h> // all for outputdebug string
//#include <winbase.h>
#include <procreg.h>
#include <math.h>
#include <sharemem.h>
#include <logging.h>

// assembly support is NOT currently...
// #define MSVC_ASSEMBLE 

//----------------------------------------------------------------

#include <vectlib.h>
#include "vecstruc.h"
//#include "vectlib.h"

VECTOR_NAMESPACE

#undef _0
#undef _X
#undef _Y
#undef _Z
static RCOORD time_scale = ONE;
static const _POINT __0 = {ZERO, ZERO, ZERO};
static const _POINT __X = { ONE, ZERO, ZERO};
static const _POINT __Y = {ZERO,  ONE, ZERO};
static const _POINT __Z = {ZERO, ZERO,  ONE};
#if (DIMENSIONS > 3 )
static const _POINT __W = {ZERO, ZERO, ZERO, ONE};
#endif
#if defined( __GNUC__  ) && defined( __cplusplus )
#ifdef __STATIC__
#define PRE_EXTERN
#else
#define PRE_EXTERN extern
#endif
#else
#define PRE_EXTERN
#endif
PRE_EXTERN MATHLIB_DEXPORT const PC_POINT VectorConst_0 = (PC_POINT)&__0[0];
PRE_EXTERN MATHLIB_DEXPORT const PC_POINT VectorConst_X = (PC_POINT)&__X[0];
PRE_EXTERN MATHLIB_DEXPORT const PC_POINT VectorConst_Y = (PC_POINT)&__Y[0];
PRE_EXTERN MATHLIB_DEXPORT const PC_POINT VectorConst_Z = (PC_POINT)&__Z[0];
#if (DIMENSIONS > 3 )
const PC_POINT _W = (PC_POINT)&__W;
#endif
const TRANSFORM __I = { { { 1, 0, 0, 0 }
								, { 0, 1, 0, 0 }
								, { 0, 0, 1, 0 }
								, { 0, 0, 0, 1 } }
							 , { 1, 1, 1 } // s
  //                    , NULL // motion
};
PRE_EXTERN MATHLIB_DEXPORT const PCTRANSFORM VectorConst_I = &__I;

static struct {
	struct {
		BIT_FIELD bRegisteredTransform : 1;
		BIT_FIELD bRegisteredMotion : 1;
	} flags;
} local_vectlib_data;
#define l local_vectlib_data

//----------------------------------------------------------------
#if defined( MSVC ) || defined( __WATCOMC__ )
#define INLINEFUNC(type, name, params) _inline type _##name params
#define REALFUNCT(type, name, params, callparams ) type name params { return _##name callparams; } 
#define REALFUNC( name, params, callparams ) void name params { _##name callparams; } 
#define DOFUNC(name) _##name
#elif defined( __LCC__ )
#define INLINEFUNC(type, name, params) inline type _INL_##name params
#define REALFUNC( name, params, callparams ) void name params { _INL_##name callparams; } 
#define REALFUNCT(type, name, params, callparams ) type name params { return _INL_##name callparams; }
#define DOFUNC(name) _INL_##name
#elif defined( __BORLANDC__ )
//#define INLINEFUNC(type, name, params) _inline type _INL_##name params
//#define REALFUNC( name, params, callparams ) void name params { _INL_##name callparams; } 
//#define REALFUNCT(type, name, params, callparams ) void name params { return _INL_##name callparams; }
//#define DOFUNC(name) _INL_##name
#define INLINEFUNC(type, name, params) type name params
#define REALFUNCT(type, name, params, callparams ) 
#define REALFUNC( name, params, callparams ) 
#define DOFUNC(name) name
#elif defined( __CYGWIN__ )
#define INLINEFUNC(type, name, params) inline type _INL_##name params
#define REALFUNC( name, params, callparams ) void name params { _INL_##name callparams; } 
#define REALFUNCT(type, name, params, callparams ) type name params { return _INL_##name callparams; }
#define DOFUNC(name) _INL_##name
#else
// no inline support at all....
#define INLINEFUNC(type, name, params)  type name params
#define REALFUNCT(type, name, params, callparams ) 
#define REALFUNC( name, params, callparams ) 
#define DOFUNC(name) name
#endif

#ifdef __BORLANDC__
#define SIN sin
#define COS cos
#else
#ifdef MAKE_RCOORD_SINGLE
#define sqrt (float)sqrt
#ifdef __WATCOMC__
#define SIN (float)sin
#define COS (float)cos
#else
#define SIN sinf
#define COS cosf
#endif
#else
#define sqrt sqrt
#define SIN sin
#define COS cos
#endif
#endif


INLINEFUNC( P_POINT, add, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2 ) )
{
   _1D( pr[0] = pv1[0] + pv2[0] );
   _2D( pr[1] = pv1[1] + pv2[1] );
   _3D( pr[2] = pv1[2] + pv2[2] );
   _4D( pr[3] = pv1[3] + pv2[3] );
   return pr;
}

REALFUNCT( P_POINT, add, ( PVECTOR pr, PCVECTOR pv1, PCVECTOR pv2 ), (pr, pv1, pv2 ) )

//----------------------------------------------------------------

INLINEFUNC( P_POINT, sub, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2 ) )
{
   _1D( pr[0] = pv1[0] - pv2[0] );
   _2D( pr[1] = pv1[1] - pv2[1] );
   _3D( pr[2] = pv1[2] - pv2[2] );
   _4D( pr[3] = pv1[3] - pv2[3] );
   return pr;
}

REALFUNCT( P_POINT, sub, ( PVECTOR pr, PCVECTOR pv1, PCVECTOR pv2 ), ( pr, pv1, pv2 ) )

//----------------------------------------------------------------

INLINEFUNC( P_POINT, scale, ( P_POINT pr, PC_POINT pv1, RCOORD k ) )
{
   _1D( pr[0] = pv1[0] * k );
   _2D( pr[1] = pv1[1] * k );
   _3D( pr[2] = pv1[2] * k );
   _4D( pr[3] = pv1[3] * k );
   return pr;
}

REALFUNCT( P_POINT, scale, ( PVECTOR pr, PCVECTOR pv1, RCOORD k ), (pr, pv1, k ) )

P_POINT Invert( P_POINT a )
{
	a[0] = -a[0];
	a[1]=-a[1];
	a[2]=-a[2];
	return a;
}


//----------------------------------------------------------------

INLINEFUNC( P_POINT, addscaled, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2, RCOORD k ) )
{
	_1D( pr[0] = pv1[0] + ( pv2[0] * k ) );
	_2D( pr[1] = pv1[1] + ( pv2[1] * k ) );
	_3D( pr[2] = pv1[2] + ( pv2[2] * k ) );
	_4D( pr[3] = pv1[3] + ( pv2[3] * k ) );
	return pr;
}

REALFUNCT( P_POINT, addscaled, ( P_POINT pr, PC_POINT pv1, PC_POINT pv2, RCOORD k ), (pr, pv1, pv2, k ) )
//----------------------------------------------------------------

INLINEFUNC( RCOORD, Length, ( PC_POINT v ) )
{
   return sqrt( v[0] * v[0] +
                v[1] * v[1]
                _3D( + v[2] * v[2] )
                _4D( + v[3] * v[3] ) );
}

REALFUNCT( RCOORD, Length, ( PCVECTOR pv ), (pv) )

//----------------------------------------------------------------

RCOORD Distance( PC_POINT v1, PC_POINT v2 )
{
   VECTOR v;
   DOFUNC(sub)( v, v1, v2 );
   return DOFUNC(Length)( v );
}

//----------------------------------------------------------------

 void normalize( P_POINT pv )
{
	RCOORD k = DOFUNC(Length)( pv );
   if( k != 0 )
		DOFUNC(scale)( pv, pv, ONE / k );
}

//----------------------------------------------------------------

 void crossproduct( P_POINT pr, PC_POINT pv1, PC_POINT pv2 )
{
   // this must be limited to 3D only, huh???
   // what if we are 4D?  how does this change??
  // evalutation of 4d matrix is 3 cross products of sub matriccii...
  pr[0] = pv2[2] * pv1[1] - pv2[1] * pv1[2]; //b2c1-c2b1
  pr[1] = pv2[0] * pv1[2] - pv2[2] * pv1[0]; //a2c1-c2a1 ( - determinaent )
  pr[2] = pv2[1] * pv1[0] - pv2[0] * pv1[1]; //b2a1-a2b1 
}

// hmmm
// 2 4 dimensional vectors would be insufficient to determine
// a single perpendicular vector... as it could lay  in a perpendular
// plane
// but - 2 planes intersecting must form a line of intersection
// 2 lines form a point of intersection
//----------------------------------------------------------------

RCOORD SinAngle( PC_POINT pv1, PC_POINT pv2 )
{
	_POINT r;
	RCOORD l;
	crossproduct( r, pv1, pv2 );
	l = DOFUNC(Length)( r ) / ( DOFUNC(Length)(pv1) * DOFUNC(Length)(pv2) );
	return l;
}

//----------------------------------------------------------------

RCOORD dotproduct( PC_POINT pv1, PC_POINT pv2 )
{
  return pv2[0] * pv1[0] +
  		   pv2[1] * pv1[1] +
  		   pv2[2] * pv1[2] ;
}

//----------------------------------------------------------------

// returns directed distance of OF in the direction of ON
RCOORD DirectedDistance( PC_POINT pvOn, PC_POINT pvOf )
{
	RCOORD l = DOFUNC(Length)(pvOn);
	if( l  )
		return dotproduct(  pvOn, pvOf ) / l; 	
	return 0;
}

//----------------------------------------------------------------
 void LogVector( char *lpName, VECTOR v )
#define LogVector(v) LogVector( #v, v )
{
   Log4( WIDE("Vector %s = <%lg, %lg, %lg>"),
            lpName, v[0], v[1], v[2] );
}

RCOORD CosAngle( PC_POINT pv1, PC_POINT pv2 )
{
	RCOORD l = DOFUNC(Length)( pv1 ) * DOFUNC(Length)( pv2 );
	if( l )
		return dotproduct( pv1, pv2 ) / l;
	return 0; // as good an angle as any...
}

//----------------------------------------------------------------

P_POINT project( P_POINT pr, PC_POINT onto, PC_POINT project )
{
	RCOORD dot;
	dot = dotproduct( onto, project ) / dotproduct( onto, onto );
	return DOFUNC(scale)( pr, onto, dot );
}

//----------------------------------------------------------------

void ClearTransform( PTRANSFORM pt )
{
	MemSet( pt, 0, sizeof( *pt ) );
	pt->m[0][0] = ONE;
	pt->m[1][1] = ONE;
	pt->m[2][2] = ONE;
	pt->m[3][3] = ONE;
	pt->s[0] = ONE;
	pt->s[1] = ONE;
	pt->s[2] = ONE;
};

//----------------------------------------------------------------

static void CPROC transform_created( void *data, PTRSZVAL size )
{
	PTRANSFORM pt = (PTRANSFORM)data;
	ClearTransform( pt );
}

PTRANSFORM CreateNamedTransform( CTEXTSTR name  )
{
	PTRANSFORM pt;
	if( name )
	{
		if( !l.flags.bRegisteredTransform )
		{
			l.flags.bRegisteredTransform = 1;
			RegisterDataType( WIDE( "SACK/vectlib" ), WIDE( "transform" ), sizeof( TRANSFORM ), transform_created, NULL );
		}
		pt = (PTRANSFORM)CreateRegisteredDataType( WIDE( "SACK/vectlib" ), WIDE( "transform" ), name );
	}
	else
	{
		pt = New( struct transform_tag );
		pt->motion = NULL;
		ClearTransform(pt);
	}
   return pt;
};

#undef CreateTransform
MATHLIB_EXPORT PTRANSFORM CreateTransform( void )
{
   return CreateNamedTransform( NULL );
}


//----------------------------------------------------------------


PTRANSFORM CreateTransformMotionEx( PTRANSFORM pt, int rocket )
{
	if( !pt->motion )
	{
		pt->motion = New( struct motion_frame_tag );
		MemSet( pt->motion, 0, sizeof( struct motion_frame_tag ) );
		pt->motion->rocket = rocket;
		pt->motion->speed_time_interval = 1000; // speed_time_interval
		pt->motion->rotation_time_interval = 1000;
	}
	return pt;
};

//----------------------------------------------------------------

PTRANSFORM CreateTransformMotion( PTRANSFORM pt )
{
   return CreateTransformMotionEx( pt, 0 );
}

//----------------------------------------------------------------

void DestroyTransform( PTRANSFORM pt )
{
	Release( pt );
}

//----------------------------------------------------------------

 void Scale( PTRANSFORM pt, RCOORD sx, RCOORD sy, RCOORD sz ) {
   pt->s[0] = sx;
   pt->s[1] = sy;
   pt->s[2] = sz;
}

//----------------------------------------------------------------

 void TranslateV( PTRANSFORM pt, PC_POINT t )
{
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
	SetPoint( pt->m[3], t );
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
}

//----------------------------------------------------------------

 void Translate( PTRANSFORM pt, RCOORD tx, RCOORD ty, RCOORD tz ) {
   pt->m[3][0] = tx;
   pt->m[3][1] = ty;
   pt->m[3][2] = tz;
}

//----------------------------------------------------------------

 void TranslateRelV( PTRANSFORM pt, PC_POINT t ) {
   DOFUNC(add)( pt->m[3], pt->m[3], t );
}

//----------------------------------------------------------------

void TranslateRel(PTRANSFORM pt, RCOORD tx, RCOORD ty, RCOORD tz) {
	VECTOR v;
	v[0] = tx;
	v[1] = ty;
   v[2] = tz;
   DOFUNC(add)( pt->m[3], pt->m[3], v );
}

//----------------------------------------------------------------


void RotateAround( PTRANSFORM pt, PC_POINT p, RCOORD amount )
{
	// P defines an axis around which the rotation portion of the matrix
	// is rotated by an amount.
	// coded from http://www.mines.edu/~gmurray/ArbitraryAxisRotation/ArbitraryAxisRotation.html
	// and http://www.siggraph.org/education/materials/HyperGraph/modeling/mod_tran/3drota.htm
   // and http://astronomy.swin.edu.au/~pbourke/geometry/rotate/
	TRANSFORM t;
	PTRANSFORM T = pt;
	RCOORD Len = Length( p );
	RCOORD Cos = COS(amount);
	RCOORD Sin = SIN(amount);
	RCOORD normal;
	t.s[0] = 1;
	t.s[1] = 1;
	t.s[2] = 1;
	// actually the only parts of the matrix resulting
	// will only be the rotation matrix, for which we are
	// building an absolute translation... which may be saved by
	// passing an identity filled transform... but anyhow...
	// the noise in the speed, accel, etc resulting from uninitialized
	// stack space being used for the transform this is building, matters
   // not at all.
	pt = &t;
	//SetPoint( _v, p );
	//normalize( _v );
#define u p[0]
#define v p[1]
#define up p[2]
	// okay this is rude and ugly, and could be optimized a bit
   // but we do have a short stack and 3 are already gone.
   normal = u*u+v*v+up*up;
	pt->m[0][0] = u * u + ( v * v + up * up ) * Cos
      / normal;
	pt->m[0][1] = u*v * ( 1-Cos ) - up * Len * Sin
      / normal;
	pt->m[0][2] = u*up*(1-Cos) + v*Len * Sin
      / normal;
	pt->m[1][0] = u*v*(1-Cos) + up*Len * Sin
      / normal;
	pt->m[1][1] = v*v + (u*u+up*up)*Cos
      / normal;
	pt->m[1][2] = v*up*(1-Cos)-u*Len*Sin
      / normal;
	pt->m[2][0] = u*up*(1-Cos)-v*Len*Sin
      / normal;
	pt->m[2][1] = v*up*(1-Cos)+u*Len*Sin
      / normal;
	pt->m[2][2] = up*up+(u*u + v*v)*Cos
      / normal;
	// oh yeah , be nice, and release these symbols...
	// V is such a common vector variable :)
#undef u
#undef v
#undef up

	ApplyRotationT( pt, T, T );
}

 void RotateAbsV( PTRANSFORM pt, PC_POINT dv ) {
   // set rotation matrix to coordinates specified.
   RCOORD rcos[3]; // cos(rx), cos(ry), cos(rz)
   RCOORD rcosf[3]; // cos[2]*cos[1], cos[2]*cos[0], cos[1]*cos[0]

   pt->m[1][0] = -(pt->m[0][1] = (float)SIN(dv[vForward]));
   pt->m[2][0] = -(pt->m[0][2] = (float)SIN(dv[vUp]));
   pt->m[2][1] = -(pt->m[1][2] = (float)SIN(dv[vRight]));
   pt->m[0][0] = //pt->s[0] *  // scale???? ookay...
      ( rcosf[0] = ( rcos[2] = (float)COS(dv[vForward]) ) 
                 * ( rcos[1] = (float)COS(dv[vUp]) ) );
   pt->m[1][1] = //pt->s[1] *
      ( rcosf[1] = ( rcos[2] ) * ( rcos[0] = (float)COS( dv[vRight] ) ) );
   pt->m[2][2] = //pt->s[2] *
      ( rcosf[2] = ( rcos[1] ) * ( rcos[0] ) );
}

//----------------------------------------------------------------

void RotateAbs( PTRANSFORM pt, RCOORD rx, RCOORD ry, RCOORD rz ) {
	_POINT p;
	p[0] = rx;
	p[1] = ry;
   p[2] = rz;
   RotateAbsV( pt, p );
}

//------------------------------------

INLINEFUNC( void, Rotate, ( RCOORD dAngle, P_POINT vaxis1, P_POINT vaxis2 ) )
{
   _POINT v1, v2;
   _POINT vsave;
   RCOORD dsin = (RCOORD)SIN( dAngle )
   	  , dcos = (RCOORD)COS( dAngle );
   MemCpy( vsave, vaxis1, sizeof( _POINT ) );
   DOFUNC(scale)( v1, vaxis1, dcos );
   DOFUNC(scale)( v2, vaxis2, dsin );
   DOFUNC(sub)( vaxis1, v1, v2 );
   DOFUNC(scale)( v2, vsave, dsin );
   DOFUNC(scale)( v1, vaxis2, dcos );
   DOFUNC(add)( vaxis2, v2, v1 );
}

//----------------------------------------------------------------

#define RotateYaw(m,a)        if(a) DOFUNC(Rotate)( a,m[vRight],m[vForward] );
#define RotatePitch(m,a)      if(a) DOFUNC(Rotate)( a,m[vForward],m[vUp] );
#define RotateRoll(m,a)       if(a) DOFUNC(Rotate)( a,m[vUp],m[vRight] );

void RotateRelV( PTRANSFORM pt, PC_POINT r )
{ // depends on Scale function....
   if( !pt->motion )
	   CreateTransformMotion( pt );
   switch( pt->motion->nTime++ )
   {
   case 0:
      RotateYaw   ( pt->m, r[vUp] );
      RotatePitch ( pt->m, r[vRight] );
      RotateRoll  ( pt->m, r[vForward] );
      break;
   case 1:
      RotateYaw   ( pt->m, r[vUp] );
      RotateRoll  ( pt->m, r[vForward] );
      RotatePitch ( pt->m, r[vRight] );
      break;
   case 2:
      RotatePitch ( pt->m, r[vRight] );
      RotateYaw   ( pt->m, r[vUp] );
      RotateRoll  ( pt->m, r[vForward] );
      break;
   case 3:
      RotatePitch ( pt->m, r[vRight] );
      RotateRoll  ( pt->m, r[vForward] );
      RotateYaw   ( pt->m, r[vUp] );
      break;
   case 4:
      RotateRoll  ( pt->m, r[vForward] );
      RotatePitch ( pt->m, r[vRight] );
      RotateYaw   ( pt->m, r[vUp] );
      break;
   default:
      pt->motion->nTime = 0;
      RotateRoll  ( pt->m, r[vForward] );
      RotateYaw   ( pt->m, r[vUp] );
      RotatePitch ( pt->m, r[vRight] );
      break;
   }
}

//----------------------------------------------------------------

 void RotateRel( PTRANSFORM pt, RCOORD x, RCOORD y, RCOORD z )
{
	_POINT p;
	p[0] = x;
	p[1] = y;
	p[2] = z;
	RotateRelV( pt, p );
}

//----------------------------------------------------------------

void RotateTo( PTRANSFORM pt, PCVECTOR vforward, PCVECTOR vright )
{
	SetPoint( pt->m[vForward], vforward );
	normalize( pt->m[vForward] );
	SetPoint( pt->m[vRight], vright );
	normalize( pt->m[vRight] );
	crossproduct( pt->m[vUp], pt->m[vForward], pt->m[vRight] );
}

//----------------------------------------------------------------

void RotateMast( PTRANSFORM pt, PCVECTOR vup )
{
	SetPoint( pt->m[vUp], vup );
	normalize( pt->m[vUp] );
	crossproduct( pt->m[vForward], pt->m[vUp], pt->m[vRight] );
	normalize( pt->m[vForward] );
	crossproduct( pt->m[vRight], pt->m[vForward], pt->m[vUp] );
}

//----------------------------------------------------------------

void RotateAroundMast( PTRANSFORM pt, RCOORD amount )
{
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
	{
		lprintf( WIDE( "blah" ) );
	}
#endif
	RotateYaw( pt->m, amount );
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
	{
		lprintf( WIDE( "blah" ) );
	}
#endif

}


//----------------------------------------------------------------

// Right as in Right Angle...
 void RotateRight( PTRANSFORM pt, int Axis1, int Axis2 )
{
   VECTOR v;
   if( Axis1 == -1 )
   {
      Invert( pt->m[vForward] );
      Invert( pt->m[vRight] );
   }
   else
   {
      SetPoint( v, pt->m[Axis1] );
      SetPoint( pt->m[Axis1], pt->m[Axis2] );
      Invert( v );
      SetPoint( pt->m[Axis2], v );
   }
}

//----------------------------------------------------------------

INLINEFUNC( void, ApplyInverseRotation, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) )
{
	#define i 0
	dest[i] = pt->m[i][vRight]   * src[vRight] +
             pt->m[i][vUp]      * src[vUp] +
             pt->m[i][vForward] * src[vForward];
	#undef i
	#define i 1
	dest[i] = pt->m[i][vRight]   * src[vRight] +
             pt->m[i][vUp]      * src[vUp] +
             pt->m[i][vForward] * src[vForward];
	#undef i
	#define i 2
	dest[i] = pt->m[i][vRight]   * src[vRight] +
             pt->m[i][vUp]      * src[vUp] +
             pt->m[i][vForward] * src[vForward];
	#undef i
}
REALFUNC( ApplyInverseRotation, (  PCTRANSFORM pt, P_POINT dest, PC_POINT src ), (pt, dest, src ) )

//----------------------------------------------------------------

INLINEFUNC( void, ApplyRotation, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) )
{
   #define i 0
   dest[i] = pt->s[i] * ( pt->m[vRight]  [i] * src[vRight] +
             pt->m[vUp]     [i] * src[vUp] +
             pt->m[vForward][i] * src[vForward] );
   #undef i
   #define i 1
   dest[i] = pt->s[i] * ( pt->m[vRight]  [i] * src[vRight] +
             pt->m[vUp]     [i] * src[vUp] +
             pt->m[vForward][i] * src[vForward] );
   #undef i
   #define i 2
   dest[i] = pt->s[i] * ( pt->m[vRight]  [i] * src[vRight] +
             pt->m[vUp]     [i] * src[vUp] +
             pt->m[vForward][i] * src[vForward] );
   #undef i
}
REALFUNC( ApplyRotation, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) , (pt, dest, src ) )

//----------------------------------------------------------------

void ApplyTranslation( PCTRANSFORM pt, P_POINT dest, PC_POINT src )
{
   DOFUNC(add)( dest, src, pt->m[3] );
}

//----------------------------------------------------------------
INLINEFUNC( void, ApplyInverseTranslation, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ) )
{
   DOFUNC(sub)( dest, pt->m[3], src );
}
REALFUNC( ApplyInverseTranslation, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ), (pt, dest, src ) )

//----------------------------------------------------------------

INLINEFUNC( void, ApplyInverse, ( PCTRANSFORM pt,  P_POINT dest, PC_POINT src ) )
{
   VECTOR v;
   DOFUNC(sub)( v, src, pt->m[3] );  // more then rotate....
   DOFUNC(ApplyInverseRotation)( pt, dest, v );
}
REALFUNC( ApplyInverse, ( PCTRANSFORM pt,  P_POINT dest, PC_POINT src ), (pt, dest, src ) )


//----------------------------------------------------------------

INLINEFUNC( void, Apply, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src )  )
{
   //VECTOR v;
   //DOFUNC(add)( v, src, pt->m[3] );
   DOFUNC(ApplyRotation)( pt, dest, src );
   DOFUNC(add)( dest, dest, pt->m[3] );
}
REALFUNC( Apply, ( PCTRANSFORM pt, P_POINT dest, PC_POINT src ), (pt, dest, src ) )
//----------------------------------------------------------------

 void ApplyR( PCTRANSFORM pt, PRAY prd, PRAY prs )
{
	DOFUNC(Apply)( pt, prd->o, prs->o );
	DOFUNC(ApplyRotation)( pt, prd->n, prs->n );
}

//----------------------------------------------------------------

void ApplyT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
	ClearTransform( &t );
	DOFUNC(ApplyRotation)( pt, t.m[0], pts->m[0] );
	DOFUNC(ApplyRotation)( pt, t.m[1], pts->m[1] );
	DOFUNC(ApplyRotation)( pt, t.m[2], pts->m[2] );
	{ 
		//VECTOR v;
		//// align our translation with souce transform
		//ApplyRotation( pts, v, pt->m[3] );
		//add( t.m[3], pts->m[3], v );
		DOFUNC(Apply)( pt, t.m[3], pts->m[3] );
	}
#ifdef _MSC_VER
	if( _isnan( t.m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
	MemCpy( ptd->m, t.m, sizeof( t.m ) );
}

//----------------------------------------------------------------

void ApplyCameraT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
	ClearTransform( &t );
	DOFUNC(ApplyRotation)( pt, t.m[0], pts->m[0] );
	DOFUNC(ApplyRotation)( pt, t.m[1], pts->m[1] );
	DOFUNC(ApplyRotation)( pt, t.m[2], pts->m[2] );
	{ 
		//VECTOR v;
		//// align our translation with souce transform
		//ApplyRotation( pts, v, pt->m[3] );
		//add( t.m[3], pts->m[3], v );
		DOFUNC(Apply)( pt, t.m[3], pts->m[3] );
	}
#ifdef _MSC_VER
	if( _isnan( t.m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
	MemCpy( ptd->m, t.m, sizeof( t.m ) );
}

//----------------------------------------------------------------

 void ApplyTranslationT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
	ClearTransform( &t );
	SetPoint( t.m[0], pts->m[0] );
	SetPoint( t.m[1], pts->m[1] );
	SetPoint( t.m[2], pts->m[2] );
	DOFUNC(Apply)(pt, t.m[3], pts->m[3] );
	MemCpy( ptd->m, t.m, sizeof( t.m ) );
}

//----------------------------------------------------------------

// may be called with the same transform for source and dest
// safely transforms such that the source is not destroyed until
// the value of dest is computed entirely, which is then set into dest.
 void ApplyRotationT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
	ClearTransform( &t );
	DOFUNC(ApplyRotation)( pt, t.m[0], pts->m[0] );
	DOFUNC(ApplyRotation)( pt, t.m[1], pts->m[1] );
	DOFUNC(ApplyRotation)( pt, t.m[2], pts->m[2] );
	SetPoint( t.m[3], pts->m[3] );
	MemCpy( ptd->m, t.m, sizeof( t.m ) );
}

//----------------------------------------------------------------

 void ApplyInverseR( PCTRANSFORM pt, PRAY prd, PRAY prs )
{
   DOFUNC(ApplyInverse)( pt, prd->o, prs->o );
   DOFUNC(ApplyInverseRotation)( pt, prd->n, prs->n );
}

//----------------------------------------------------------------

 void ApplyInverseT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
   ClearTransform( &t );
   DOFUNC(ApplyInverseRotation)( pt, t.m[0], pts->m[0] );
   DOFUNC(ApplyInverseRotation)( pt, t.m[1], pts->m[1] );
	DOFUNC(ApplyInverseRotation)( pt, t.m[2], pts->m[2] );
   DOFUNC(ApplyInverse)( pt, t.m[3], pts->m[3] );
   MemCpy( ptd->m, t.m, sizeof( t.m ) );
   MemCpy( ptd->s, t.s, sizeof( t.s ) );
}

//----------------------------------------------------------------

 void ApplyInverseTranslationT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
	ClearTransform( &t );
   SetPoint( t.m[0], pts->m[0] );
   SetPoint( t.m[1], pts->m[1] );
	SetPoint( t.m[2], pts->m[2] );
   DOFUNC(sub)( t.m[3], pt->m[3], pt->m[3] );  // more then rotate....
   MemCpy( ptd->m, t.m, sizeof( t.m ) );
}

//----------------------------------------------------------------

 void ApplyInverseRotationT( PCTRANSFORM pt, PTRANSFORM ptd, PCTRANSFORM pts )
{
	TRANSFORM t;
   ClearTransform( &t );
   DOFUNC(ApplyInverseRotation)( pt, t.m[0], pts->m[0] );
   DOFUNC(ApplyInverseRotation)( pt, t.m[1], pts->m[1] );
   DOFUNC(ApplyInverseRotation)( pt, t.m[2], pts->m[2] );
   SetPoint( t.m[3], pts->m[3] );
   MemCpy( ptd->m, t.m, sizeof( t.m ) );
}

//----------------------------------------------------------------

void MoveForward( PTRANSFORM pt, RCOORD distance )
{
	if( pt )
	{
			pt->m[3][0] += distance * pt->m[vForward][0];
			pt->m[3][1] += distance * pt->m[vForward][1];
			pt->m[3][2] += distance * pt->m[vForward][2];
	}
}
//----------------------------------------------------------------

void MoveRight( PTRANSFORM pt, RCOORD distance )
{
	if( pt )
	{
			pt->m[3][0] += distance * pt->m[vRight][0];
			pt->m[3][1] += distance * pt->m[vRight][1];
			pt->m[3][2] += distance * pt->m[vRight][2];
	}
}

//----------------------------------------------------------------

void MoveUp( PTRANSFORM pt, RCOORD distance )
{
	if( pt )
	{
			pt->m[3][0] += distance * pt->m[vUp][0];
			pt->m[3][1] += distance * pt->m[vUp][1];
			pt->m[3][2] += distance * pt->m[vUp][2];
	}
}

//----------------------------------------------------------------

 void Forward( PTRANSFORM pt, RCOORD distance )
{
	if( pt && pt->motion )
		pt->motion->speed[vForward] = distance;
}

//----------------------------------------------------------------

 void Up( PTRANSFORM pt, RCOORD distance )
{
	if( pt && pt->motion )
		pt->motion->speed[vUp] = distance;
}

//----------------------------------------------------------------

 void Right( PTRANSFORM pt, RCOORD distance )
{
	if( pt && pt->motion )
		pt->motion->speed[vRight] = distance;
}

//----------------------------------------------------------------

void AddTransformCallback( PTRANSFORM pt, MotionCallback callback, PTRSZVAL psv )
{
	if( pt && pt->motion )
	{
		INDEX idx;
		AddLink( &pt->motion->callbacks, callback );
		idx = FindLink( &pt->motion->callbacks, (POINTER)callback );
		SetLink( &pt->motion->userdata, idx, psv );
	}
}

//----------------------------------------------------------------

void InvokeCallbacks( PTRANSFORM pt )
{
	INDEX idx;
	MotionCallback callback;
	if( pt && pt->motion )
	{
		LIST_FORALL( pt->motion->callbacks, idx, MotionCallback, callback )
		{
			callback( (PTRSZVAL)GetLink( &pt->motion->userdata, idx ), pt );
#ifdef _MSC_VER
			if( _isnan( pt->m[0][0] ) )
				lprintf( WIDE( "blah" ) );
#endif
		}
	}
}

//----------------------------------------------------------------

LOGICAL Move( PTRANSFORM pt )
{
	LOGICAL moved = FALSE;
	// this matrix of course....
	// clock the matrix one cycle....
	// this means - add one speed vector 
	// and one rotation vector to the current
	// x-y-z-a-b-c position and orientation 
	// matrix...  this is later referenced
	// to trasform the remaining points
	// (application of this matrix)
	if( pt && pt->motion )
	{
		//VECTOR v;
		RCOORD speed_step;
		RCOORD rotation_step;
		if( !pt ) 
			return FALSE;
#ifdef _MSC_VER
		if( _isnan( pt->m[0][0] ) )
		{
			return FALSE;
			lprintf( WIDE( "blah" ) );
		}
#endif
	{
		{
			static _64 tick_freq_cpu;
			static _64 last_tick_cpu;
			static _32 tick_cpu;
			if( pt->motion->last_tick )
			{
				// how much time passed between then and no
				// and what's our target resolution?
				static _32 now;
				_32 delta = ( now = timeGetTime() ) - pt->motion->last_tick;
				if( !delta )
				{
					return FALSE;  // on 0 time elapse, don't try this... cpu scaling will mess this up.
#if HAVE_CHEAP_CPU_FREQUENCY
					if( !tick_freq_cpu )
						tick_freq_cpu = GetCPUFrequency();
					{
						
						static _64 now_cpu;
						_64 delta_cpu = ( last_tick_cpu - (now_cpu = GetCPUTick() ) )
							- ( ( now - tick_cpu ) * tick_freq_cpu );
						RCOORD delta2 = ( (RCOORD)delta_cpu * 1000 ) / ( (RCOORD)tick_freq_cpu * 33 );
						pt->time_scale = ONE / (RCOORD)( delta2 );
			//					//);
						tick_cpu = now;
						last_tick_cpu = now_cpu;
					}
#endif
					// err we have a cycle rate greater than 1ms 1000/sec?
				}
				else
				{
					speed_step = delta / pt->motion->speed_time_interval; // times 1000 so we get extra precision.
					rotation_step = delta / pt->motion->rotation_time_interval; // times 1000 so we get extra precision.
				}
				pt->motion->last_tick = now;
			}
			else
			{
				// don't move on the first tick.
#if CONSTANT_CPU_TICK
				tick_cpu = pt->motion->last_tick = timeGetTime();
				last_tick_cpu = GetCPUTick();
				pt->motion->time_scale = ONE;
#endif
				pt->motion->last_tick = timeGetTime();
				return FALSE;
			}
		}			
	}
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
	{
		VECTOR v;
		if( pt->motion->rocket )
		{
			moved = TRUE;
			scale( v, pt->motion->accel, speed_step );
			// add the scaled acceleration in the current direction of this
			pt->motion->speed[0] += v[0] * pt->m[0][0]
				+ v[1] * pt->m[1][0]
				+ v[2] * pt->m[2][0];
			pt->motion->speed[1] += v[0] * pt->m[0][1]
				+ v[1] * pt->m[1][1]
				+ v[2] * pt->m[2][1];
			pt->motion->speed[2] += v[0] * pt->m[0][2]
				+ v[1] * pt->m[1][2]
				+ v[2] * pt->m[2][2];
			addscaled( pt->m[3], pt->m[3], pt->motion->speed, speed_step );
		}
		else
		{
			if( pt->motion->speed[0] || pt->motion->speed[1] || pt->motion->speed[2]
				|| pt->motion->accel[0] || pt->motion->accel[1] || pt->motion->accel[2] )
			{
				moved = TRUE;
				addscaled( pt->motion->speed, pt->motion->speed, pt->motion->accel, speed_step );
				scale( v, pt->motion->speed, speed_step );
				//scale( v, v, pt->time_scale ); // velocity applied across this time
				pt->m[3][0] += v[0] * pt->m[0][0]
					+ v[1] * pt->m[1][0]
					+ v[2] * pt->m[2][0];
				pt->m[3][1] += v[0] * pt->m[0][1]
					+ v[1] * pt->m[1][1]
					+ v[2] * pt->m[2][1];
				pt->m[3][2] += v[0] * pt->m[0][2]
					+ v[1] * pt->m[1][2]
					+ v[2] * pt->m[2][2];
			}
		}
#ifdef _MSC_VER
		if( _isnan( pt->m[0][0] ) )
			lprintf( WIDE( "blah" ) );
#endif
		// include time scale for rotation also...
		if( pt->motion->rotation[0] || pt->motion->rotation[1] || pt->motion->rotation[2]
			|| pt->motion->rot_accel[0] || pt->motion->rot_accel[1] || pt->motion->rot_accel[2]
			)
		{
			VECTOR  r;
			//lprintf( WIDE(WIDE( "Time scale is not applied" )) );
			moved = TRUE;
			addscaled( pt->motion->rotation, pt->motion->rotation, pt->motion->rot_accel, rotation_step );
			scale( r, pt->motion->rotation, rotation_step );

			RotateRelV( pt, r );
#ifdef _MSC_VER
			if( _isnan( pt->m[0][0] ) )
				lprintf( WIDE( "blah" ) );
#endif
		}
#ifdef _MSC_VER
		if( _isnan( pt->m[0][0] ) )
			lprintf( WIDE( "blah" ) );
#endif
		InvokeCallbacks( pt );
#ifdef _MSC_VER
		if( _isnan( pt->m[0][0] ) )
			lprintf( WIDE( "blah" ) );
#endif
	}
	}
	return moved;
}

//----------------------------------------------------------------
#if 0
 void Unmove( PTRANSFORM pt )
{
   // this matrix of course....
   // clock the matrix one cycle....
   // this means - add one speed vector 
   // and one rotation vector to the current
   // x-y-z-a-b-c position and orientation 
   // matrix...  this is later referenced
   // to trasform the remaining points
	// (application of this matrix)
	VECTOR v;
	RCOORD speed_step;
   RCOORD rotation_step;
	if( !pt ) 
		return;
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
	{
		return;
		lprintf( WIDE( "blah" ) );
	}
#endif
	{
		{
			static _64 tick_freq_cpu;
			static _64 last_tick_cpu;
			static _32 tick_cpu;
			if( pt->last_tick )
			{
				// how much time passed between then and no
				// and what's our target resolution?
				static _32 now;
				_32 delta = ( now = timeGetTime() ) - pt->last_tick;
				if( !delta )
				{
					return FALSE;  // on 0 time elapse, don't try this... cpu scaling will mess this up.
#if HAVE_CHEAP_CPU_FREQUENCY
					if( !tick_freq_cpu )
						tick_freq_cpu = GetCPUFrequency();
					{
						
						static _64 now_cpu;
						_64 delta_cpu = ( last_tick_cpu - (now_cpu = GetCPUTick() ) )
							- ( ( now - tick_cpu ) * tick_freq_cpu );
						RCOORD delta2 = ( (RCOORD)delta_cpu * 1000 ) / ( (RCOORD)tick_freq_cpu * 33 );
						pt->time_scale = ONE / (RCOORD)( delta2 );
			//					//);
						tick_cpu = now;
						last_tick_cpu = now_cpu;
					}
#endif
					// err we have a cycle rate greater than 1ms 1000/sec?
				}
				else
				{
					speed_step = delta / pt->speed_time_interval; // times 1000 so we get extra precision.
					rotation_step = delta / pt->rotation_time_interval; // times 1000 so we get extra precision.
				}
            pt->prior_tick = pt->last_tick;
				pt->last_tick = now;
			}
			else
			{
				// don't move on the first tick.
#if CONSTANT_CPU_TICK
				tick_cpu = pt->last_tick = timeGetTime();
				last_tick_cpu = GetCPUTick();
				pt->time_scale = ONE;
#endif
				pt->last_tick = timeGetTime();
            pt->prior_tick = pt->last_tick - 1;
            return;
			}
		}			
	}
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
	//add( v, pt->speed, pt->accel );
	addscaled( pt->speed, pt->speed, pt->accel, -speed_step );
	scale( v, pt->speed, -speed_step );
	//scale( v, v, pt->time_scale ); // velocity applied across this time
   pt->m[3][0] += v[0] * pt->m[0][0]
                + v[1] * pt->m[1][0]
                + v[2] * pt->m[2][0];
   pt->m[3][1] += v[0] * pt->m[0][1]
                + v[1] * pt->m[1][1]
                + v[2] * pt->m[2][1];
   pt->m[3][2] += v[0] * pt->m[0][2]
                + v[1] * pt->m[1][2]
		+ v[2] * pt->m[2][2];
#ifdef _MSC_VER
   	if( _isnan( pt->m[0][0] ) )
			lprintf( WIDE( "blah" ) );
#endif
   // include time scale for rotation also...
	if( pt->rotation[0] || pt->rotation[1] || pt->rotation[2] )
	{
		VECTOR  r;
		//lprintf( WIDE(WIDE( "Time scale is not applied" )) );
		addscaled( pt->motion->rotation, pt->motion->rotation, pt->motion->rot_accel, -rotation_step );
		scale( r, pt->motion->rotation, -rotation_step );

		RotateRelV( pt, r );
#ifdef _MSC_VER
   	if( _isnan( pt->m[0][0] ) )
			lprintf( WIDE( "blah" ) );
#endif
	}
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
	InvokeCallbacks( pt );
#ifdef _MSC_VER
	if( _isnan( pt->m[0][0] ) )
		lprintf( WIDE( "blah" ) );
#endif
}
#endif
//----------------------------------------------------------------

P_POINT GetSpeed( PTRANSFORM pt, P_POINT s )
{
   SetPoint( s, pt->motion->speed );
   return s;
}

//----------------------------------------------------------------

PC_POINT  SetSpeed( PTRANSFORM pt, PC_POINT s )
{
	SetPoint( pt->motion->speed, s );
   return s;
}

//----------------------------------------------------------------

void SetTimeInterval( PTRANSFORM pt, RCOORD speed_interval, RCOORD rotation_interval )
{
   pt->motion->rotation_time_interval = rotation_interval; // application of motion uses this factor
   pt->motion->speed_time_interval = speed_interval; // application of motion uses this factor
}

//----------------------------------------------------------------
P_POINT  GetAccel( PTRANSFORM pt, P_POINT s )
{
   SetPoint( s, pt->motion->accel );
   return s;
}

//----------------------------------------------------------------

 PC_POINT  SetAccel( PTRANSFORM pt, PC_POINT s )
{
	SetPoint( pt->motion->accel, s );
	return s;
}

//----------------------------------------------------------------

 PC_POINT SetRotation( PTRANSFORM pt, PC_POINT r )
{
	SetPoint( pt->motion->rotation, r );
	return r;
}

//----------------------------------------------------------------

P_POINT GetRotation( PTRANSFORM pt, P_POINT r )
{
	SetPoint( r, pt->motion->rotation );
	return r;
}

//----------------------------------------------------------------

 PC_POINT SetRotationAccel( PTRANSFORM pt, PC_POINT r )
{
	SetPoint( pt->motion->rot_accel, r );
	return r;
}

//----------------------------------------------------------------

 void GetOriginV( PTRANSFORM pt, P_POINT o )
{
   SetPoint( o, pt->m[3] );
}

//----------------------------------------------------------------

 PC_POINT GetOrigin( PTRANSFORM pt  )
{
	if( pt )
		return pt->m[3];
	return NULL;
}

//----------------------------------------------------------------

 void GetAxisV( PTRANSFORM pt, P_POINT a, int n )
{
	SetPoint( a, pt->m[n] );
}

//----------------------------------------------------------------

 PC_POINT GetAxis( PTRANSFORM pt, int n )
{
	if( pt )
		return pt->m[n];
	return NULL;
}

//----------------------------------------------------------------

 void SetAxisV( PTRANSFORM pt, PC_POINT a, int n )
{
   SetPoint( pt->m[n], a );
}

//----------------------------------------------------------------

 void SetAxis( PTRANSFORM pt, RCOORD a, RCOORD b, RCOORD c, int n )
{
   SetPoint( pt->m[n], &a );
}

//----------------------------------------------------------------

void InvertTransform( PTRANSFORM pt )
{
	RCOORD tmp;
	int i, j;
	// confusing loops - but this will invert top row
	// to left row - reversable operation...
	// unsure if I need to change signs during this 
	// the matrix determinate should still be the same....???
	for( j = 0; j < 3; j++ )
	{
		for( i = j+1; i < 4; i++ )
		{
			tmp = pt->m[i][j];
			pt->m[i][j] = pt->m[j][i];
			pt->m[j][i] = tmp;
		}
	}
}

//----------------------------------------------------------------

void GetGLCameraMatrix( PTRANSFORM pt, PMATRIX out )
{
    // ugly but perhaps there will be some optimization if I
    // do this linear like... sure it's a lot of code, but at
    // least there's no work to loop and multiply...
    out[0][0] = pt->m[0][0];
    out[0][1] = pt->m[1][0];
    out[0][2] = -pt->m[2][0];
	 //out[0][3] = pt->m[3][0];
	 out[0][3] = pt->m[0][3];

    out[1][0] = pt->m[0][1];
    out[1][1] = pt->m[1][1];
    out[1][2] = -pt->m[2][1];
	 //out[1][3] = pt->m[3][1];
	 out[1][3] = pt->m[1][3];

	 // z was inverted of what it should have been...
    out[2][0] = pt->m[0][2];
    out[2][1] = pt->m[1][2];
    out[2][2] = -pt->m[2][2];
	 //out[2][3] = pt->m[3][2];
	 out[2][3] = pt->m[2][3];

    //out[3][0] = pt->m[0][3];
    //out[3][1] = pt->m[1][3];
    //out[3][2] = pt->m[2][3];
	 //out[3][3] = pt->m[3][3];
	 // okay apparently opengl applies
	 // this origin, and then rotates according to the
	 // above matrix... so I need to undo having the correct
    // bias on the translation.
	 //DOFUNC(ApplyInverseRotation)( pt, out[3], pt->m[3] );
    Invert( pt->m[2] );
    DOFUNC(ApplyInverseRotation)( pt, out[3], pt->m[3] );
	 Invert( pt->m[2] );
    Invert( out[3] );
    //ApplyRotation( pt, out[3], pt->m[3] );
    //out[3][0] = pt->m[3][0];
    //out[3][1] = pt->m[3][1];
    //out[3][2] = pt->m[3][2];
    out[3][3] = pt->m[3][3];



}

//----------------------------------------------------------------

void GetGLMatrix( PTRANSFORM pt, PMATRIX out )
{
	// ugly but perhaps there will be some optimization if I
	// do this linear like... sure it's a lot of code, but at
	// least there's no work to loop and multiply...
	out[0][0] = pt->m[0][0];
	out[0][1] = pt->m[1][0];
	out[0][2] = -pt->m[2][0];
	//out[0][3] = pt->m[3][0];
	out[0][3] = pt->m[0][3];

	out[1][0] = pt->m[0][1];
	out[1][1] = pt->m[1][1];
	out[1][2] = -pt->m[2][1];
	//out[1][3] = pt->m[3][1];
	out[1][3] = pt->m[1][3];

	// z was inverted of what it should have been...
	out[2][0] = pt->m[0][2];
	out[2][1] = pt->m[1][2];
	out[2][2] = -pt->m[2][2];
	//out[2][3] = pt->m[3][2];
	out[2][3] = pt->m[2][3];

	//out[3][0] = pt->m[0][3];
	//out[3][1] = pt->m[1][3];
	//out[3][2] = pt->m[2][3];
	//out[3][3] = pt->m[3][3];
	 // okay apparently opengl applies
	 // this origin, and then rotates according to the
	 // above matrix... so I need to undo having the correct
    // bias on the translation.
	 //DOFUNC(ApplyInverseRotation)( pt, out[3], pt->m[3] );
	Invert( pt->m[2] );
	DOFUNC(ApplyInverseRotation)( pt, out[3], pt->m[3] );
	Invert( pt->m[2] );
	Invert( out[3] );

	out[3][3] = pt->m[3][3];
}

//----------------------------------------------------------------

void SetGLMatrix( PMATRIX in, PTRANSFORM pt )
{
    // ugly but perhaps there will be some optimization if I
    // do this linear like... sure it's a lot of code, but at
    // least there's no work to loop and multiply...

    pt->m[0][0] =  in[0][0];
    pt->m[0][1] =  in[1][0];
    pt->m[0][2] =  in[2][0];
	pt->m[0][3] =  in[0][3];

    pt->m[1][0] =  in[0][1];
    pt->m[1][1] =  in[1][1];
    pt->m[1][2] =  in[2][1];
	 pt->m[1][3] =  in[1][3];

	 // z was inverted of what it should have been...
    pt->m[2][0] =  in[0][2];
    pt->m[2][1] =  in[1][2];
    pt->m[2][2] =  in[2][2];
	 pt->m[2][3] =  in[2][3];

	 pt->m[3][0] = in[3][0]; 
	 pt->m[3][1] = in[3][1]; 
	 pt->m[3][2] = in[3][2]; 
	 pt->m[3][3] = in[3][3]; 
    //DOFUNC(ApplyRotation)( pt, pt->m[3], in[3] );
	 // okay apparently opengl applies
	 // this origin, and then rotates according to the
	 // above matrix... so I need to undo having the correct
    // bias on the translation.
	 //DOFUNC(ApplyInverseRotation)( pt, in[3], pt->m[3] );
    //Invert( pt->m[2] );
    //DOFUNC(ApplyRotation)( pt, pt->m[3], in[3] );
	// Invert( pt->m[2] );
    //Invert( pt->m[3] );

    pt->m[3][3] = 1.0f;
}

void SetRotationMatrix( PTRANSFORM pt, RCOORD *quat )
{

   /*
   Nq = w^2 + x^2 + y^2 + z^2
if Nq > 0.0 then s = 2/Nq else s = 0.0
X = x*s; Y = y*s; Z = z*s
wX = w*X; wY = w*Y; wZ = w*Z
xX = x*X; xY = x*Y; xZ = x*Z
yY = y*Y; yZ = y*Z; zZ = z*Z
[ 1.0-(yY+zZ)       xY-wZ        xZ+wY  ]
[      xY+wZ   1.0-(xX+zZ)       yZ-wX  ]
[      xZ-wY        yZ+wX   1.0-(xX+yY) ]
*/

	pt->m[0][0] = 1 - 2 * quat[2] * quat[2] - 2 * quat[3] * quat[3];
	pt->m[0][1] = 2 * quat[1] * quat[2] - 2 * quat[3] * quat[0];
	pt->m[0][2] = (2 * quat[1] * quat[3] + 2 * quat[2] * quat[0]);
	pt->m[1][0] = 2 * quat[1] * quat[2] + 2 * quat[3] * quat[0];
	pt->m[1][1] = 1 - 2 * quat[1] * quat[1] - 2 * quat[3] * quat[3];
	pt->m[1][2] = (2 * quat[2] * quat[3] - 2 * quat[1] * quat[0]);
	pt->m[2][0] = 2 * quat[1] * quat[3] - 2 * quat[2] * quat[0];
	pt->m[2][1] = 2 * quat[2] * quat[3] + 2 * quat[1] * quat[0];
	pt->m[2][2] = (1 - 2 * quat[1] * quat[1] - 2 * quat[2] * quat[2]);


	//pt->m[0][0] = quat[0]*quat[0] + quat[1]*quat[1] - quat[2]*quat[2] - quat[3]*quat[3];
	//pt->m[0][1] = 2*quat[1]*quat[2] - 2 * quat[0] * quat[3];
	//pt->m[0][2] = 2*quat[1]*quat[3] + 2 * quat[0] * quat[2];
	//pt->m[1][0] = 2*quat[1]*quat[2] + 2 * quat[0] * quat[3];
	//pt->m[1][1] = quat[0]*quat[0] - quat[1]*quat[1] + quat[2]*quat[2] - quat[3]*quat[3];
	//pt->m[1][2] = 2*quat[2]*quat[3] - 2 * quat[0] * quat[1];	
	//pt->m[2][0] = 2*quat[1]*quat[3] - 2 * quat[0] * quat[2];
	//pt->m[2][1] = 2*quat[2]*quat[3] + 2 * quat[0] * quat[1];
	//pt->m[2][2] = quat[0]*quat[0] - quat[1]*quat[1] - quat[2]*quat[2] + quat[3]*quat[3];

	//  q^-1=q'/(q*q')
	// 
	// (w,x,y,z)^-1 = (w,-x,-y,-z)/((w,-x,-y,-z)*(w,x,y,z))
	//
}

void GetRotationMatrix( PTRANSFORM pt, RCOORD *quat )
{
//	t = Qxx+Qyy+Qzz (trace of Q)
//r = sqrt(1+t)
//w = 0.5*r
//x = copysign(0.5*sqrt(1+Qxx-Qyy-Qzz), Qzy-Qyz)
//y = copysign(0.5*sqrt(1-Qxx+Qyy-Qzz), Qxz-Qzx)
//z = copysign(0.5*sqrt(1-Qxx-Qyy+Qzz), Qyx-Qxy)
	/*
	where copysign(x,y) is x with the sign of y:
	copysign(x,y) = sign(y) |x|;

	*/
	/*
	t = Qxx+Qyy+Qzz
r = sqrt(1+t)
s = 0.5/r
w = 0.5*r
x = (Qzy-Qyz)*s
y = (Qxz-Qzx)*s
z = (Qyx-Qxy)*s
*/
	RCOORD t = pt->m[0][0] + pt->m[1][1] + pt->m[2][2];
	RCOORD r = sqrt(1+t);
	RCOORD s = ((RCOORD)0.5)/r;
	quat[0] = ((RCOORD)0.5)*r;
	quat[1] = (pt->m[2][1]-pt->m[1][2])*s;
	quat[2] = (pt->m[0][2]-pt->m[2][0])*s;
	quat[3] = (pt->m[1][0]-pt->m[0][1])*s;

}

#ifdef WORKING_REVERSE_ANGLE_CODE

	double ZMatrix::pitch( void )
	{
		if( ( m[2][2] > 0.707 ) )
			return atan2( m[2][1], m[2][2] ) * (360 / (2*3.14159)); 
		else if( ( m[2][2] < -0.707 ) )
			return atan2( m[2][1], -m[2][2] ) * (360 / (2*3.14159)); 
		else
			if( m[2][0] < 0 )
				return atan2( m[2][1], -m[2][0] ) * (360 / (2*3.14159)); 
			else
				return atan2( m[2][1], m[2][0] ) * (360 / (2*3.14159)); 


		return atan2( -m[2][1], sqrt( 1-m[2][1] * m[2][1]) ) * (360 / (2*3.14159)); 
		// sohcah toa  
		return asin( m[0][2] ) * (360 / (2*3.14159));
	}
	double ZMatrix::yaw( void )
	{
		if( m[2][1] < 0.707 && m[2][1] > -0.707 )
		{
			return atan2( m[2][0], m[2][2] ) * (360 / (2*3.14159)); 
		}
		else
		{
			if( m[1][2] < 0 )
				return atan2( m[1][0], m[1][2] ) * (360 / (2*3.14159)); 
			else
				return atan2( m[1][0], m[1][2] ) * (360 / (2*3.14159)); 
		}
		// sohcah toa  
		//return atan2( m[0][1], m[0][0] ) * (360 / (2*3.14159)); 
		return asin( m[0][2] ) * (360 / (2*3.14159));
	}
	double ZMatrix::roll( void )
	{
		// sohcah toa  
		if( ( m[0][0] > 0.707 ) )
			return atan2( m[0][1], m[0][0] ) * (360 / (2*3.14159)); 
		else if( ( m[0][0] < -0.707 ) )
			return atan2( m[0][1], -m[0][0] ) * (360 / (2*3.14159)); 
		else
			if( m[0][2] < 0 )
				return atan2( m[0][1], -m[0][2] ) * (360 / (2*3.14159)); 
			else
				return atan2( m[0][1], m[0][2] ) * (360 / (2*3.14159)); 

		return asin( m[0][1] ) * (360 / (2*3.14159));
	}


#endif

//----------------------------------------------------------------
#ifdef __BORLANDC__
#define PRINTF lprintf
#define SPRINTF sprintf
#else
#define PRINTF lprintf
#define SPRINTF(a,b,...) tnprintf(a,sizeof(a),b,##__VA_ARGS__)
#endif

#define DOUBLE_FORMAT  WIDE("%g")

void PrintVectorEx( CTEXTSTR lpName, PCVECTOR v DBG_PASS )
{
   _xlprintf( 1 DBG_RELAY )( WIDE("Vector  %s = <") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT,
            lpName, v[0], v[1], v[2], Length( v ) );
}
#undef PrintVector
void PrintVector( CTEXTSTR lpName, PCVECTOR v )
{
   PrintVectorEx( lpName, v DBG_SRC );
}

 void PrintVectorStdEx( CTEXTSTR lpName, VECTOR v DBG_PASS )
{
   TEXTCHAR byBuffer[256];
   tnprintf( byBuffer, sizeof( byBuffer ), WIDE("Vector  %s = <") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT WIDE("\n"),
            lpName, v[0], v[1], v[2], Length(v) );
   PRINTF( WIDE("%s"), byBuffer );
}

#undef PrintVectorStd
 void PrintVectorStd( CTEXTSTR lpName, VECTOR v )
{
   PrintVectorStdEx( lpName, v DBG_SRC );
}

#undef PrintMatrix
void PrintMatrix( CTEXTSTR lpName, MATRIX m )
#define PrintMatrix(m) PrintMatrix( #m, m )
{
   PrintMatrixEx( lpName, m DBG_SRC );
}

void PrintMatrixEx( CTEXTSTR lpName, MATRIX m DBG_PASS )
{
   _xlprintf( 1 DBG_RELAY )( WIDE("Vector  %s = <") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT,
            lpName, m[0][0], m[0][1], m[0][2], m[0][3], Length( m[0] ) );
   _xlprintf( 1 DBG_RELAY )( WIDE("Vector  %s = <") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT,
            lpName, m[1][0], m[1][1], m[1][2], m[1][3], Length( m[1] ) );
   _xlprintf( 1 DBG_RELAY )( WIDE("Vector  %s = <") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT,
            lpName, m[2][0], m[2][1], m[2][2], m[2][3], Length( m[2] ) );
   _xlprintf( 1 DBG_RELAY )( WIDE("Vector  %s = <") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE(", ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT,
            lpName, m[3][0], m[3][1], m[3][2], m[3][3], Length( m[3] ) );
   {
	   VECTOR v;
	   crossproduct( v, m[1], m[2] );
	   PrintVector( WIDE( "cross1" ), v );
	   crossproduct( v, m[2], m[0] );
	   PrintVector( WIDE( "cross2" ), v );
	   crossproduct( v, m[0], m[1] );
	   PrintVector( WIDE( "cross3" ), v );
   }
}


#undef ShowTransform
void ShowTransformEx( PTRANSFORM pt, char *header DBG_PASS )
{
   _xlprintf( 1 DBG_RELAY )( WIDE("transform %s"), header );
	_xlprintf( 1 DBG_RELAY )( WIDE("     -----------------"));
#define F4(name) _xlprintf( 1 DBG_RELAY )( _WIDE(#name) WIDE(" <") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT, pt->name[0], pt->name[1], pt->name[2], pt->name[3], Length( pt->name ) )
#define F(name) _xlprintf( 1 DBG_RELAY )( _WIDE(#name) WIDE(" <") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE("> ") DOUBLE_FORMAT, pt->name[0], pt->name[1], pt->name[2], Length( pt->name ) )
	if( pt->motion )
	{
		F(motion->speed);
		F(motion->rotation);
	}
   F4(m[0]);
   F4(m[1]);
   F4(m[2]);
   F4(m[3]);
//   F(rcosf);
   F(s);
}

void ShowTransform( PTRANSFORM pt, char *header )
{
	ShowTransformEx( pt, header DBG_SRC );
}

void showstd( PTRANSFORM pt, char *header )
{
	TEXTCHAR byMsg[256];
#undef F4
#undef F
#define F4(name) SPRINTF( byMsg, _WIDE(#name) WIDE(" <") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(">"), pt->name[0], pt->name[1], pt->name[2], pt->name[3] )
#define F(name) SPRINTF( byMsg, _WIDE(#name) WIDE(" <") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(" ") DOUBLE_FORMAT WIDE(">"), pt->name[0], pt->name[1], pt->name[2] )
   PRINTF( WIDE("%s"), header );
   PRINTF( WIDE("%s"), WIDE("     -----------------\n"));
   F(motion->speed);
   PRINTF( WIDE("%s"), byMsg );
   F(motion->rotation);
   PRINTF( WIDE("%s"), byMsg );
   F(m[0]);
   PRINTF( WIDE("%s"), byMsg );
   F(m[1]);
   PRINTF( WIDE("%s"), byMsg );
   F(m[2]);
   PRINTF( WIDE("%s"), byMsg );
   F(m[3]);
   PRINTF( WIDE("%s"), byMsg );
//   F(rcosf);
//   PRINTF( WIDE("%s"), byMsg );
   F(s);
   PRINTF( WIDE("%s"), byMsg );
}

void SaveTransform( PTRANSFORM pt, CTEXTSTR filename )
{
	FILE *file;
	Fopen( file, filename, WIDE("wb") );
	if( file )
	{
		fwrite( pt, 1, sizeof( *pt ), file );
		fclose( file );
	}

}
void LoadTransform( PTRANSFORM pt, CTEXTSTR filename )
{
	FILE *file;
	Fopen( file, filename, WIDE("rb" ) );
	if( file )
	{
		fread( pt, 1, sizeof( *pt ), file );
      pt->motion = NULL;
		fclose( file );
	}

}

void GetPointOnPlane( PRAY plane, PCVECTOR up, PCVECTOR size, PCVECTOR point )
{
	// plane is origin-normal specificatio of plane.
	// up is the direction of 'y' on the plane, right is the direction of 'x',
	// size  is the width/ehight of the plane from the origin
   // point is the x/y point (missing the z coordinate)
	VECTOR right;
	crossproduct( right, plane->n, up );
}

VECTOR_NAMESPACE_END

