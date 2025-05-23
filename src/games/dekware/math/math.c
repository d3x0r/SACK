#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE

#include <plugin.h>
#include <math.h>

static int DoFunction(PSENTIENT  ps, PTEXT parameters, double (*f)(double) )
{
   PTEXT pAmount, temp, varname;
   varname = temp = GetParam( ps, &parameters );
   if( temp )
   {
		struct {
			uint32_t bIntNumber : 1;
		} flags;
		int64_t iNumber;
		double fNumber;
		int bInt;
		PTEXT pResult;
		{
			if( IsSegAnyNumber( &temp, &fNumber, &iNumber, &bInt ) )
			{
				flags.bIntNumber = bInt;
				{
					if( flags.bIntNumber )
					{
						pResult = SegCreateFrom_64( f( iNumber ) );
					}
					else
					{
						pResult = SegCreateFromFloat( f( fNumber ) );
					}
				}
			}
			else
			{
				DECLTEXT( msg, "Primary Operand is not a value." );
				EnqueLink( &ps->Command->Output, &msg );
				return FALSE;
			}
			if( varname->flags & TF_INDIRECT )
			{
				PTEXT pInd;
				pInd = GetIndirect( varname );
				SetIndirect( varname, pResult );
				LineRelease( pInd );
			}
			else
			{
				SegCopyFormat( pResult, varname );
				SegSubst( varname, pResult ); // self modifying code... :)
			}
		}
	}
}


static int HandleCommand( "Math", "sin", "usage: sin variable; result replaces variable" )(PSENTIENT ps,PTEXT parameters)
{
	DoFunction( ps, parameters, sin );
	return FALSE;
}

static int HandleCommand( "Math", "cos", "usage: cos variable; result replaces variable" )(PSENTIENT ps,PTEXT parameters)
{
	DoFunction( ps, parameters, cos );
	return FALSE;
}

static int HandleCommand( "Math", "tan", "usage: tan variable; result replaces variable" )(PSENTIENT ps,PTEXT parameters)
{
	DoFunction( ps, parameters, tan );
	return FALSE;
}

static int HandleCommand( "Math", "sqrt", "usage: sqrt variable; result replaces variable" )(PSENTIENT ps,PTEXT parameters)
{
	DoFunction( ps, parameters, sqrt );
	return FALSE;
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
