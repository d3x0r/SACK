
#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <logging.h>

#include "fractions.h"


#ifdef __cplusplus
	namespace sack { namespace math { namespace fraction {
#endif
//---------------------------------------------------------------------------

 int  sLogFraction ( TEXTCHAR *string, PFRACTION x )
{
	if( x->denominator < 0 )
	{
		if( x->numerator > -x->denominator )
			return tnprintf( string, 31, WIDE("-%d %d/%d")
						, x->numerator / (-x->denominator)
						, x->numerator % (-x->denominator), -x->denominator );
		else
			return tnprintf( string, 31, WIDE("-%d/%d"), x->numerator, -x->denominator );
	}
	else
	{
		if( x->numerator > x->denominator )
			return tnprintf( string, 31, WIDE("%d %d/%d")
						, x->numerator / x->denominator
						, x->numerator % x->denominator, x->denominator );
		else
			return tnprintf( string, 31, WIDE("%d/%d"), x->numerator, x->denominator );
	}
}

//---------------------------------------------------------------------------

 int  sLogCoords ( TEXTCHAR *string, PCOORDPAIR pcp )
{
	TEXTCHAR *start = string;
	string += tnprintf( string, 2*sizeof(TEXTCHAR), WIDE("(") );
	string += sLogFraction( string, &pcp->x );
	string += tnprintf( string, 2*sizeof(TEXTCHAR), WIDE(",") );
	string += sLogFraction( string, &pcp->y );
	string += tnprintf( string, 2*sizeof(TEXTCHAR), WIDE(")") );
	return (int)(string - start);
}

 void  LogCoords ( PCOORDPAIR pcp )
{
	TEXTCHAR buffer[256];
	TEXTCHAR *string = buffer;
	string += tnprintf( string, 2*sizeof(TEXTCHAR), WIDE("(") );
	string += sLogFraction( string, &pcp->x );
	string += tnprintf( string, 2*sizeof(TEXTCHAR), WIDE(",") );
	string += sLogFraction( string, &pcp->y );
	string += tnprintf( string, 2*sizeof(TEXTCHAR), WIDE(")") );
	Log( buffer );
}

//---------------------------------------------------------------------------

static void NormalizeFraction( PFRACTION f )
{
	int n;
	int target = min( f->numerator, f->denominator ) / 2;
	for( n = 2; n <target; n++ )
	{
		if( ( ( f->numerator % n) == 0 ) &&
		    ( ( f->denominator % n ) == 0 ) )
		{
			f->numerator /= n;
			f->denominator /= n;
			n = 1; // one cause we add one before looping again;
			target = min( f->numerator, f->denominator ) / 2;
			continue;
		}
	}
}

//---------------------------------------------------------------------------

 PFRACTION  AddFractions ( PFRACTION base, PFRACTION offset )
{
	if( !offset->numerator ) // 0 addition either way is same.
		return base;
	//LogFraction( base );
	//fprintf( log, WIDE(" + ") );
	//LogFraction( offset );
	//fprintf( log, WIDE(" = ") );
	if( base->denominator < 0 )
	{
		if( offset->denominator < 0 )
		{
			// result is MORE negative when adding them... this is good.
			if( offset->denominator == base->denominator )
			{
				base->numerator += offset->numerator;
			}
			else
			{
				// results in a positive value
				base->numerator = -( ( base->numerator * offset->denominator ) +
				                     ( offset->numerator * base->denominator ) );
				base->denominator *= offset->denominator;
				// need to retain it's original sign....
				base->denominator = -base->denominator;
			}
		}
		else
		{
			// base (probably small negative) - offset + (probably a BIG positive)
			if( offset->denominator == -base->denominator )
			{
				base->numerator -= offset->numerator;
			}
			else
			{
				// result is positive - which is original sign.
				base->numerator = -( ( base->numerator * offset->denominator ) +
									      ( offset->numerator * base->denominator ) );
				base->denominator *= -offset->denominator;
			}
		}
	}
	else
	{
		if( offset->denominator < 0 )
		{
			// correct - base positive, offset negative
			// results in a positive addition from the origin...
			// making it less negative and closer to the bottom/right
			if( offset->denominator == -base->denominator )
			{
				base->numerator = offset->numerator - base->numerator;
				base->denominator = -base->denominator;
			}
			else
			{
				base->numerator = ( base->numerator * offset->denominator ) +
									   ( offset->numerator * base->denominator );
				base->denominator *= offset->denominator;
			}
		}
		else
		{
			if( offset->denominator == base->denominator )
			{
				base->numerator += offset->numerator;
			}
			else
			{
            //lprintf( "%d %d %d %d = %d", base->numerator,
				//		  base->denominator,
				//		  offset->denominator, offset->numerator, ( base->numerator * offset->denominator ) +
				//						( offset->numerator * base->denominator ) );
				base->numerator = ( base->numerator * offset->denominator ) +
										( offset->numerator * base->denominator );
				base->denominator *= offset->denominator;
			}
		}
	}
	NormalizeFraction( base );
	return base;
	//LogFraction( base );
	//fprintf( log, WIDE("\n") );
}

//---------------------------------------------------------------------------

 PFRACTION  SubtractFractions ( PFRACTION base, PFRACTION offset )
{
	if( !offset->numerator ) // 0 addition either way is same.
		return base;
	//LogFraction( base );
	//fprintf( log, WIDE(" + ") );
	//LogFraction( offset );
	//fprintf( log, WIDE(" = ") );
	if( base->denominator < 0 )
	{
		if( offset->denominator < 0 )
		{
			// result is MORE negative when adding them... this is good.
			if( offset->denominator == base->denominator )
			{
				base->numerator -= offset->numerator;
			}
			else
			{
				// results in a positive value
				base->numerator = -( ( base->numerator * offset->denominator ) -
				                     ( offset->numerator * base->denominator ) );
				base->denominator *= offset->denominator;
				// need to retain it's original sign....
				base->denominator = -base->denominator;
			}
		}
		else
		{
			// base (probably small negative) - offset + (probably a BIG positive)
			if( offset->denominator == -base->denominator )
			{
				base->numerator += offset->numerator;
			}
			else
			{
				// result is positive - which is original sign.
				base->numerator = -( ( base->numerator * offset->denominator ) -
									      ( offset->numerator * base->denominator ) );
				base->denominator *= -offset->denominator;
			}
		}
	}
	else
	{
		if( offset->denominator < 0 )
		{
			// correct - base positive, offset negative
			// results in a positive addition from the origin...
			// making it less negative and closer to the bottom/right
			if( offset->denominator == -base->denominator )
			{
				base->numerator = offset->numerator + base->numerator;
				base->denominator = -base->denominator;
			}
			else
			{
				base->numerator = ( base->numerator * offset->denominator ) -
									   ( offset->numerator * base->denominator );
				base->denominator *= offset->denominator;
			}
		}
		else
		{
			if( offset->denominator == base->denominator )
			{
				base->numerator -= offset->numerator;
			}
			else
			{
				//lprintf( "%d %d %d %d = %d", base->numerator,
				//		  base->denominator,
				//		  offset->denominator, offset->numerator, ( base->numerator * offset->denominator ) +
				//						( offset->numerator * base->denominator ) );
				base->numerator = ( base->numerator * offset->denominator ) -
										( offset->numerator * base->denominator );
				base->denominator *= offset->denominator;
			}
		}
	}
	NormalizeFraction( base );
	return base;
	//LogFraction( base );
	//fprintf( log, WIDE("\n") );
}

//---------------------------------------------------------------------------

 void  AddCoords ( PCOORDPAIR base, PCOORDPAIR offset )
{
	AddFractions( &base->x, &offset->x );
	AddFractions( &base->y, &offset->y );
}

//---------------------------------------------------------------------------

 PFRACTION  ScaleFraction ( PFRACTION result, int32_t value, PFRACTION f )
{
	result->numerator = value * f->numerator;
	result->denominator = f->denominator;
	return result;
}


//---------------------------------------------------------------------------

 uint32_t  ScaleValue ( PFRACTION f, int32_t value )
{
	int32_t result = 0;
	if( f->denominator )
		result = ( value * f->numerator ) / f->denominator;
	return result;
}

//---------------------------------------------------------------------------

 uint32_t  InverseScaleValue ( PFRACTION f, int32_t value )
{
	int32_t result =0;
if( f->numerator )
	result = ( value * f->denominator ) / f->numerator;
	return result;
}

//---------------------------------------------------------------------------

 int32_t  ReduceFraction ( PFRACTION f )
{
	return ( f->numerator ) / f->denominator;
}

#ifdef __cplusplus
	}}}//	namespace sack { namespace math { namespace fraction {
#endif

//---------------------------------------------------------------------------
// $Log: fractions.c,v $
// Revision 1.6  2005/01/27 07:39:23  panther
// Linux cleaned.
//
// Revision 1.5  2004/09/03 14:43:47  d3x0r
// flexible frame reactions to font changes...
//
// Revision 1.4  2003/03/25 08:45:50  panther
// Added CVS logging tag
//
