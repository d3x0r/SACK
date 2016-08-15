/* Defines a simple FRACTION type. Fractions are useful for
   scaling one value to another. These operations are handles
   continously. so iterating a fraction like 13 denominations of
   100 will be smooth.                                           */

#ifndef FRACTIONS_DEFINED
/* Multiple inclusion protection symbol. */
#define FRACTIONS_DEFINED

#include <sack_types.h>

#ifdef __cplusplus
#  define _FRACTION_NAMESPACE namespace fraction {
#  define _FRACTION_NAMESPACE_END }
#  ifndef _MATH_NAMESPACE
#    define _MATH_NAMESPACE namespace math {
#  endif
#  define 	SACK_MATH_FRACTION_NAMESPACE_END } } }
#else
#  define _FRACTION_NAMESPACE
#  define _FRACTION_NAMESPACE_END
#  ifndef _MATH_NAMESPACE
#    define _MATH_NAMESPACE
#  endif
#  define 	SACK_MATH_FRACTION_NAMESPACE_END
#endif


SACK_NAMESPACE
	/* Namespace of custom math routines.  Contains operators
	 for Vectors and fractions. */
	_MATH_NAMESPACE
	/* Fraction namespace contains a PFRACTION type which is used to
   store integer fraction values. Provides for ration and
   proportion scaling. Can also represent fractions that contain
   a whole part and a fractional part (5 2/3 : five and
	two-thirds).                                                  */
	_FRACTION_NAMESPACE

/* Define the call type of the function. */
#define FRACTION_API CPROC
#  ifdef FRACTION_SOURCE
#    define FRACTION_PROC EXPORT_METHOD
#  else
/* Define the library linkage for a these functions. */
#    define FRACTION_PROC IMPORT_METHOD
#  endif


/* The faction type. Stores a fraction as integer
   numerator/denominator instead of a floating point scalar. */
/* Pointer to a <link sack::math::fraction::FRACTION, FRACTION>. */
/* The faction type. Stores a fraction as integer
   numerator/denominator instead of a floating point scalar. */
typedef struct fraction_tag {
	/* Numerator of the fraction. (This is the number on top of a
	   fraction.)                                                 */
	int numerator;
	/* Denominator of the fraction. (This is the number on bottom of
	   a fraction.) This specifies the denominations.                */
	int denominator;
} FRACTION, *PFRACTION;
#ifdef HAVE_ANONYMOUS_STRUCTURES
typedef struct coordpair_tag {
	union {
		FRACTION x;
		FRACTION width;
	};
	union {
		FRACTION y;
		FRACTION height;
	};
} COORDPAIR, *PCOORDPAIR;
#else
/* A coordinate pair is a 2 dimensional fraction expression. can
   be regarded as x, y or width,height. Each coordiante is a
   Fraction type.                                                */
typedef struct coordpair_tag {
       	/* The x part of the coordpair. */
       	FRACTION x;
       	/* The y part of the coordpair. */
       	FRACTION y;
} COORDPAIR, *PCOORDPAIR;

#endif

/* \ \ 
   Parameters
   fraction :     the fraction to set
   numerator :    numerator of the fraction
   demoninator :  denominator of the fraction */
#define SetFraction(f,n,d) ((((f).numerator=((int)(n)) ),((f).denominator=((int)(d)))),(f))
/* Sets the value of a FRACTION. This is passed as the whole
   number and the fraction.
   Parameters
   fraction :  the fraction to set
   w :         this is the whole number to set
   n :         numerator of remainder to set
   d :         denominator of fraction to set.
   
   Example
   Fraction f = 3 1/2;
   <code lang="c++">
   FRACTION f;
   SetFractionV( f, 3, 1, 2 );
   // the resulting fraction will be 7/2
   </code>                                                   */
#define SetFractionV(f,w,n,d) (  (d)? \
	((((f).numerator=((int)((n)*(w))) )  \
	,((f).denominator=((int)(d)))),(f))  \
	: 	((((f).numerator=((int)((w))) )  \
	,((f).denominator=((int)(1)))),(f))  \
)
/* \ \ 
   Parameters
   base :    origin point (content is modified by adding offset
             to it)
   offset :  offset point                                       */
FRACTION_PROC  void FRACTION_API  AddCoords ( PCOORDPAIR base, PCOORDPAIR offset );

/* Add one fraction to another.
   Parameters
   base :    This is the starting value, and recevies the result
             of (base+offset)
   offset :  This is the fraction to add to base.
   
   Returns
   base                                                          */
FRACTION_PROC  PFRACTION FRACTION_API  AddFractions ( PFRACTION base, PFRACTION offset );
/* Add one fraction to another.
   Parameters
   base :    This is the starting value, and recevies the result
             of (base+offset)
   offset :  This is the fraction to add to base.
   
   Returns
   base                                                          */
FRACTION_PROC  PFRACTION FRACTION_API  SubtractFractions ( PFRACTION base, PFRACTION offset );

/* NOT IMPLEMENTED */
FRACTION_PROC  PFRACTION FRACTION_API  MulFractions ( PFRACTION f, PFRACTION x );

/* Log a fraction into a string. */
FRACTION_PROC  int FRACTION_API  sLogFraction ( TEXTCHAR *string, PFRACTION x );
/* Unsafe log of a coordinate pair's value into a string. The
   string should be at least 69 characters long.
   Parameters
   string :  the string to print the fraction into
   pcp :     the coordinate pair to print                     */
FRACTION_PROC  int FRACTION_API  sLogCoords ( TEXTCHAR *string, PCOORDPAIR pcp );
/* Log coordpair to logfile. */
FRACTION_PROC  void FRACTION_API  LogCoords ( PCOORDPAIR pcp );

/* scales a fraction by a signed integer value.
   
   
   Parameters
   result\ :  pointer to a FRACTION to receive the result
   value :    the amount to be scaled
   f :        the fraction to multiply the value by
   
   Returns
   \result; the pointer the fraction to receive the result. */
FRACTION_PROC  PFRACTION FRACTION_API  ScaleFraction ( PFRACTION result, int32_t value, PFRACTION f );
/* Results in the integer part of the fraction. If the faction
   was 330/10 then the result would be 33.                     */
FRACTION_PROC  int32_t FRACTION_API  ReduceFraction ( PFRACTION f );

/* Scales a 32 bit integer value by a fraction. The result is
   the scaled value result.
   Parameters
   f :      pointer to the faction to multiply value by
   value :  the value to scale
   
   Returns
   The (value * f) integer value of.                          */
FRACTION_PROC  uint32_t FRACTION_API  ScaleValue ( PFRACTION f, int32_t value );
/* \ \ 
   Parameters
   f :      The fraction to scale the value by
   value :  the value to scale by (1/f)
   
   Returns
   the value of ( value * 1/ f )               */
FRACTION_PROC  uint32_t FRACTION_API  InverseScaleValue ( PFRACTION f, int32_t value );

	SACK_MATH_FRACTION_NAMESPACE_END

#ifdef __cplusplus
using namespace sack::math::fraction;
#endif

#endif

//---------------------------------------------------------------------------
// $Log: fractions.h,v $
// Revision 1.6  2004/09/03 14:43:40  d3x0r
// flexible frame reactions to font changes...
//
// Revision 1.5  2003/03/25 08:38:11  panther
// Add logging
//
// Revision 1.4  2003/01/27 09:45:03  panther
// Fix lack of anonymous structures
//
// Revision 1.3  2002/10/09 13:16:02  panther
// Support for linux shared memory mapping.
// Support for better linux compilation of configuration scripts...
// Timers library is now Threads AND Timers.
//
//
