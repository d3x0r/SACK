#include <sack_types.h>
#include <pssql.h>
#include "sqlstruc.h"
SQL_NAMESPACE

#undef DoSQLCommandf
#undef DoSQLQueryf
#undef DoSQLRecordQueryf
#undef SQLCommandf
#undef SQLQueryf
#undef SQLRecordQueryf
#undef SQLRecordQueryf_v2

#if defined( _DEBUG ) || defined( _DEBUG_INFO )
#define WRAP(a,b,c)  CTEXTSTR _FILE_##b = _WIDE(__FILE__); int _LINE_##b; __f_##b __##b(DBG_VOIDPASS)  { _FILE_##b = pFile; _LINE_##b = nLine; return b; }
#define DBG_ARGS(n)  , _FILE_##n, _LINE_##n
#else
#define WRAP(a,b,c)  __f_##b __##b(void)  { return b; }
#define DBG_ARGS(n) 
#endif

WRAP( int, DoSQLCommandf, ( CTEXTSTR fmt, ... ) )

WRAP( int, DoSQLQueryf, ( CTEXTSTR *result, CTEXTSTR fmt, ... ) )

WRAP( int, DoSQLRecordQueryf, ( int *nResults, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... ) )

WRAP( int, SQLCommandf, ( PODBC odbc, CTEXTSTR fmt, ... ) )

WRAP( int, SQLQueryf, ( PODBC odbc, CTEXTSTR *result, CTEXTSTR fmt, ... ) )

WRAP( int, SQLRecordQueryf, ( PODBC odbc, int *nResults, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... ) )
WRAP( int, SQLRecordQueryf_v2, ( PODBC odbc, int *nResults, CTEXTSTR **result, CTEXTSTR **fields, size_t **fieldLengths, CTEXTSTR fmt, ... ) )


int DoSQLCommandf( CTEXTSTR fmt, ... )
{
	int result;
	PTEXT cmd;
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	cmd = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	result = DoSQLCommandEx( GetText( cmd ) DBG_ARGS(DoSQLCommandf) );
	LineRelease( cmd );
	return result;
}

int DoSQLQueryf( CTEXTSTR *result, CTEXTSTR fmt, ... )
{
	int result_code;
	PTEXT cmd;
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	cmd = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	result_code = DoSQLQueryEx( (CTEXTSTR)GetText( cmd ), result DBG_ARGS(DoSQLQueryf) );
	LineRelease( cmd );
	return result_code;
}

int DoSQLRecordQueryf( int *nResults, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... )
{
	int result_code;
	PTEXT cmd;
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	cmd = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	result_code = SQLRecordQueryEx( NULL, GetText( cmd ), nResults, result, fields DBG_ARGS(DoSQLRecordQueryf) );
	LineRelease( cmd );
	return result_code;
}

int SQLCommandf( PODBC odbc, CTEXTSTR fmt, ... )
{
	if( !odbc->flags.bClosed ) {
		int result;
		PTEXT cmd;
		PVARTEXT pvt = VarTextCreateExx( 4096, 16384 * 16 );
		va_list args;
		va_start( args, fmt );
		vvtprintf( pvt, fmt, args );
		cmd = VarTextGet( pvt );
		if( cmd )
		{
			VarTextDestroy( &pvt );
			result = SQLCommandEx( odbc, GetText( cmd ) DBG_ARGS( SQLCommandf ) );
			LineRelease( cmd );
		}
		else {
			result = 0;
			lprintf( WIDE( "ERROR: Sql format failed: %s" ), fmt );
		}
		return result;
	}
	return FALSE;
}

int SQLQueryf( PODBC odbc, CTEXTSTR *result, CTEXTSTR fmt, ... )
{
	int result_code;
	PTEXT cmd;
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	cmd = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	result_code = SQLQueryEx( odbc, (CTEXTSTR)GetText( cmd ), result DBG_ARGS(SQLQueryf) );
	LineRelease( cmd );
	return result_code;
}

int SQLRecordQueryf( PODBC odbc, int *nResults, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... )
{
	int result_code;
	PTEXT cmd;
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	cmd = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	result_code = SQLRecordQueryEx( odbc, GetText( cmd ), nResults, result,fields DBG_ARGS(SQLRecordQueryf) );
	LineRelease( cmd );
	return result_code;
}

int SQLRecordQueryf_v2( PODBC odbc, int *nResults, CTEXTSTR **result, size_t **resultLengths, CTEXTSTR **fields, CTEXTSTR fmt, ... )
{
	int result_code;
	PTEXT cmd;
	PVARTEXT pvt = VarTextCreate();
	va_list args;
	va_start( args, fmt );
	vvtprintf( pvt, fmt, args );
	cmd = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	result_code = SQLRecordQuery_v4( odbc, GetText( cmd ), GetTextSize( cmd ), nResults, result, resultLengths,fields,NULL  DBG_ARGS(SQLRecordQueryf_v2) );
	LineRelease( cmd );
	return result_code;
}


SQL_NAMESPACE_END
