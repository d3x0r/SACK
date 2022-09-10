#include <stdhdrs.h>
#include <procreg.h>
#include <filesys.h>
#define SQLLIB_SOURCE
#ifdef USE_SQLITE_INTERFACE
#define USES_SQLITE_INTERFACE
#endif
#include "sqlstruc.h"

SQL_NAMESPACE

#ifdef __STATIC_GLOBALS__
extern struct pssql_global global_sqlstub_data;
#  define g (global_sqlstub_data)

#else
extern struct pssql_global *global_sqlstub_data;
#  define g (*global_sqlstub_data)
PRIORITY_PRELOAD( InitGlobalSqlUtil, GLOBAL_INIT_PRELOAD_PRIORITY )
{
	SimpleRegisterAndCreateGlobal( global_sqlstub_data );
}
#endif

#ifdef __cplusplus
using namespace sack::containers::BinaryTree;
using namespace sack::memory;
#endif

struct params
{
	CTEXTSTR name;
	PODBC odbc;
};

static int CPROC MyParmCmp( uintptr_t s1, uintptr_t s2 )
{
	struct params *p1 = (struct params*)s1;
	struct params *p2 = (struct params*)s2;
	if( p1->odbc == p2->odbc )
		return StrCaseCmp( p1->name, p2->name );
	else
		return 1;
}
static int CPROC MySqlUtilStrCmp( uintptr_t s1, uintptr_t s2 )
{
	return StrCaseCmp( (TEXTCHAR*)s1, (TEXTCHAR*)s2 );
}

PTREEROOT GetTableCache( PODBC odbc, CTEXTSTR tablename )
{
	//static PTREEROOT tables;
	PTREEROOT newcache;
	struct params parameters;
	parameters.odbc = odbc;
	parameters.name = tablename;
	//lprintf( "Looking for name cache of table %s", tablename );
	if( !g.tables )
	{
		g.tables = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
										 , MyParmCmp
										 , NULL );
	}
	if( !( newcache = (PTREEROOT)FindInBinaryTree( g.tables, (uintptr_t)&parameters ) ) )
	{
		struct params *saveparams = New( struct params );
		saveparams->name = StrDup( tablename );
		saveparams->odbc = odbc;
		AddBinaryNode( g.tables
						 , newcache = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
																	, MySqlUtilStrCmp
																	, NULL )
						 , (uintptr_t)saveparams );
	}
	return newcache;
}


INDEX GetIndexOfName(PODBC odbc, CTEXTSTR table,CTEXTSTR name)
{
	/* this resulting truncation warning is OK. */
	return (INDEX)(((uintptr_t)FindInBinaryTree( GetTableCache( odbc, table ), (uintptr_t)name ))-1);
}

CTEXTSTR GetKeyOfName(PODBC odbc, CTEXTSTR table,CTEXTSTR name)
{
	/* this resulting truncation warning is OK. */
	return (CTEXTSTR)FindInBinaryTree( GetTableCache( odbc, table ), (uintptr_t)name );
}

//---------------------------------------------------------------------------
#undef GetNameIndexExx
INDEX GetNameIndexExtended( PODBC odbc
														 , CTEXTSTR name
														 , CTEXTSTR table
														 , CTEXTSTR col
														 , CTEXTSTR namecol
														 , int bCreate
														 , int bQuote DBG_PASS )
{
	TEXTCHAR query[256];
	CTEXTSTR result = NULL;
	INDEX IDName = INVALID_INDEX;
	if( !table || !name )
		return INVALID_INDEX;
	if( bQuote ) // only check quotable values
		IDName = GetIndexOfName(odbc, table,name);
	if( IDName != INVALID_INDEX )
		return IDName;

	PushSQLQueryEx( odbc );

	tnprintf( query, sizeof( query ), "select %s from %s where `%s`=%s"
			  , col?col:"id"
			  , table
			  , namecol
			  , EscapeStringOpt( name, TRUE )
			  );
	if( SQLQueryEx( odbc, query, &result DBG_RELAY) && result )
	{
		IDName = (INDEX)IntCreateFromText( result );
		DebugBreak();
		PopODBCEx( odbc );
	}
	else if( bCreate )
	{
		tnprintf( query, sizeof( query ), "insert into %s (`%s`) values(%s)"
				  , table
				  ,namecol
				  ,EscapeString( name )
				  );
		if( !SQLCommandEx( odbc, query DBG_RELAY ) )
		{
			lprintf( "isert failed, how can we define name %s?", EscapeString( name ) );
			// inser failed...
		}
		else
		{
			// all is well.
			IDName = FetchLastInsertIDEx( odbc, table, col DBG_RELAY );
		}
		PopODBCEx( odbc );
	}
	else
		IDName = INVALID_INDEX;

	if( bQuote && ( IDName != INVALID_INDEX ) )
		AddBinaryNode( GetTableCache(odbc, table), (POINTER)(IDName+1), (uintptr_t)StrDup( name ) );
	return IDName;
}
//-----------------------------------------------------------------------

INDEX GetNameIndexExx( PODBC odbc, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS )
{
	return GetNameIndexExtended( odbc, name,table,col,namecol,bCreate,TRUE DBG_RELAY );
}


//-----------------------------------------------------------------------
CTEXTSTR FetchLastInsertKeyEx( PODBC odbc, CTEXTSTR table, CTEXTSTR col DBG_PASS )
{
	static TEXTSTR RecordID;
	CTEXTSTR result = NULL;
	if( !odbc )
		odbc = g.odbc;
	if( !odbc )
		return NULL;
	if( RecordID )
	{
		Release( RecordID );
		RecordID = NULL;
	}
	//lprintf( "getting last insert ID?" );
#ifdef POSGRES_BACKEND
	{
		TEXTCHAR query[256];
		sprintf( query, "select currval('%s_%s_seq')", table, col );
		if( SQLQueryEx( odbc, query, &result ) && result DBG_RELAY )
		{
			RecordID = StrDup( result );
			while( FetchSQLResult( odbc, &result ) );
		}
	}
#endif
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	// extended sqlite functions with LAST_INSERT_ID() so the following code would work alos.
	if( odbc->flags.bSQLite_native )
	{
		// can also be done with 'select last_insert_rowid()'
		RecordID = NewArray( TEXTCHAR, 32 );
		tnprintf( RecordID, 32, "%" _size_f, (INDEX)sqlite3_last_insert_rowid( odbc->db ) );
	}
#endif
#ifdef USE_ODBC
	PushSQLQueryEx( odbc );
	if( odbc->flags.bAccess )
	{
		if( SQLQueryEx( odbc, "select @@IDENTITY", &result DBG_RELAY ) && result )
			RecordID = StrDup( result );
	}
	else if( odbc->flags.bODBC )
	{
		if( SQLQueryEx( odbc, "select LAST_INSERT_ID()", &result DBG_RELAY ) && result )
		{
			RecordID = StrDup( result );
		}
	}
	PopODBCEx( odbc );
#endif
//
	return RecordID;
}

//-----------------------------------------------------------------------
INDEX FetchLastInsertIDEx( PODBC odbc, CTEXTSTR table, CTEXTSTR col DBG_PASS )
{
	CTEXTSTR result_key = FetchLastInsertKeyEx( odbc, table, col DBG_RELAY );
	INDEX result = result_key?(INDEX)IntCreateFromText( result_key ) : INVALID_INDEX;
	return result;
}


//-----------------------------------------------------------------------
CTEXTSTR  GetLastInsertKeyEx( CTEXTSTR table, CTEXTSTR col DBG_PASS )
{
	return FetchLastInsertKeyEx( NULL, table, col DBG_RELAY );
}

//-----------------------------------------------------------------------
 INDEX  GetLastInsertIDEx( CTEXTSTR table, CTEXTSTR col DBG_PASS )
{
	return FetchLastInsertIDEx( NULL, table, col DBG_RELAY );
}

//---------------------------------------------------------------------------

#undef EscapeBinary
#undef EscapeString

TEXTSTR EscapeSQLBinaryExx( PODBC odbc, CTEXTSTR blob, uintptr_t bloblen, uintptr_t *resultLen, LOGICAL bQuote DBG_PASS )
{
	int type_mysql = 1;
#if MYSQL_ODBC_CONNECTION_IS_BROKEN
	int first_failure = 1;
#endif
	CTEXTSTR pBlob = blob;
	TEXTCHAR *tmpnamebuf, *result;
	unsigned int n;
	int targetlen;

	if( odbc && ( odbc->flags.bSQLite
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
					 || odbc->flags.bSQLite_native
#endif
					) )
		type_mysql = 0;

	if( type_mysql )
	{
		n = 0;
		targetlen = 2;  // include the ' around it if required... otherwise concat fails.

		while( n < bloblen )
		{
#if MYSQL_ODBC_CONNECTION_IS_BROKEN
			if( (*pBlob) == '\x9f' || (*pBlob) == '\x9c' )
			{
				if( first_failure )
				{
					first_failure = 0;
					targetlen += 10; // concat("")
				}
				targetlen += 14;  // ",char(159),"
			}
#endif
			if( (*pBlob) == '\'' ||
				(*pBlob) == '\\' ||
				(*pBlob) == '\0' ||
				(*pBlob) == '\"' )
				targetlen++;
			n++;
		}

		n = 0;

		result = tmpnamebuf = (TEXTSTR)AllocateEx( ( sizeof( TEXTCHAR ) * ( targetlen + bloblen + 1 ) ) DBG_RELAY );

#if MYSQL_ODBC_CONNECTION_IS_BROKEN
		if( !first_failure )
			tmpnamebuf += tnprintf( tmpnamebuf, targetlen+bloblen, "concat('" );
		else
#endif
		{
			if( bQuote )
				(*tmpnamebuf++) = '\'';
		}
		while( n < bloblen )
		{
#if MYSQL_ODBC_CONNECTION_IS_BROKEN
			if( (*pBlob) == '\x9f' || (*pBlob) == '\x9c' )
				tmpnamebuf += tnprintf( tmpnamebuf, 15, "\',char(%d),\'", (unsigned char)(*pBlob) );
				else
#endif
			{
				if( (*pBlob) == '\'' ||
					(*pBlob) == '\\' ||
					(*pBlob) == '\0' ||
					(*pBlob) == '\"' )
					(*tmpnamebuf++) = '\\';
				if( (*pBlob) )
					(*tmpnamebuf++) = (*pBlob);
				else
					(*tmpnamebuf++) = '0';
			}
			n++;
		}

#if MYSQL_ODBC_CONNECTION_IS_BROKEN
		if( !first_failure )
		{
			(*tmpnamebuf++) = '\'';
			(*tmpnamebuf++) = ')';
		}
		else
#endif
		{
			if( bQuote )
				(*tmpnamebuf++) = '\'';
		}
		(*tmpnamebuf) = 0; // best terminate this thing.
	}
	else
	{
		n = 0;

		targetlen = 0;
		while( n < bloblen )
		{
			if( (*pBlob) == '\'' )
				targetlen++;
			if( (*pBlob) == '\0' )
				targetlen += 13;
			pBlob++;
			n++;
		}

		n = 0;
		pBlob = blob; // reset blob input
		result = tmpnamebuf = (TEXTSTR)AllocateEx( ( sizeof( TEXTCHAR ) * ( targetlen + bloblen + 3 ) ) DBG_RELAY );
		if( bQuote )
			( *tmpnamebuf++ ) = '\'';
		while( n < bloblen ) {
			if( (*pBlob) == 0 ) {
				( *tmpnamebuf++ ) = '\'';
				( *tmpnamebuf++ ) = '|';
				( *tmpnamebuf++ ) = '|';

				( *tmpnamebuf++ ) = 'C';
				( *tmpnamebuf++ ) = 'H';
				( *tmpnamebuf++ ) = 'A';
				( *tmpnamebuf++ ) = 'R';
				( *tmpnamebuf++ ) = '(';
				( *tmpnamebuf++ ) = '0';
				( *tmpnamebuf++ ) = ')';

				( *tmpnamebuf++ ) = '|';
				( *tmpnamebuf++ ) = '|';
				( *tmpnamebuf++ ) = '\'';
			}
			else {
				if( (*pBlob) == '\'' )
					( *tmpnamebuf++ ) = '\'';
				( *tmpnamebuf++ ) = (*pBlob);
			}
			n++;
			pBlob++;
		}
		if( bQuote )
			( *tmpnamebuf++ ) = '\'';
		( *tmpnamebuf ) = 0; // best terminate this thing.
	}
	if( resultLen )
		( *resultLen ) = tmpnamebuf - result;
	return result;
}
TEXTSTR EscapeSQLBinaryEx ( PODBC odbc, CTEXTSTR blob, uintptr_t bloblen DBG_PASS )
{
	return EscapeSQLBinaryExx( odbc, blob, bloblen, NULL, FALSE DBG_RELAY );
}
TEXTSTR EscapeBinaryEx ( CTEXTSTR blob, uintptr_t bloblen DBG_PASS )
{
	return EscapeSQLBinaryExx( NULL, blob, bloblen, NULL, FALSE DBG_RELAY );
}

TEXTCHAR * EscapeBinary ( CTEXTSTR blob, uintptr_t bloblen )
{
	return EscapeSQLBinaryExx( NULL, blob, bloblen, NULL, FALSE DBG_SRC );
}

//---------------------------------------------------------------------------

TEXTCHAR * EscapeSQLStringEx ( PODBC odbc, CTEXTSTR name DBG_PASS )
{
	if( !name ) return NULL;
	return EscapeSQLBinaryExx( odbc, name, strlen( name ), NULL, FALSE DBG_RELAY );
}

TEXTCHAR * EscapeStringEx ( CTEXTSTR name DBG_PASS )
{
	return EscapeSQLBinaryExx( NULL, name, (uint32_t)strlen( name ), NULL, FALSE DBG_RELAY );
}

TEXTCHAR * EscapeString ( CTEXTSTR name )
{
	return EscapeSQLBinaryExx( NULL, name, strlen( name ), NULL, FALSE DBG_SRC );
}

uint8_t hexbyte( TEXTCHAR *string )
{
	static TEXTCHAR hex[17] = "0123456789abcdef";
	static TEXTCHAR HEX[17] = "0123456789ABCDEF";
	TEXTCHAR *digit;
	uint8_t value = 0;

	digit = strchr( hex, string[0] );
	if( !digit )
	{
		digit = strchr( HEX, string[0] );
		if( digit )
		{
//cpg 19 Jan 2007 1>c:\work\sack\src\sqllib\sqlutil.c(187) : warning C4244: '=' : conversion from '__w64 int' to 'sack::uint8_t', possible loss of data
			value = (uint8_t)(digit - HEX);
		}
		else
			return 0;
	}
	else
	{
//cpg 19 Jan 2007 1>c:\work\sack\src\sqllib\sqlutil.c(194) : warning C4244: '=' : conversion from '__w64 int' to 'sack::uint8_t', possible loss of data
		value = (uint8_t)(digit - hex);
	}

	value *= 16;
	digit = strchr( hex, string[1] );
	if( !digit )
	{
		digit = strchr( HEX, string[1] );
		if( digit )
		{
//cpg 19 Jan 2007 1>c:\work\sack\src\sqllib\sqlutil.c(204) : warning C4244: '+=' : conversion from '__w64 int' to 'sack::uint8_t', possible loss of data
			value += (uint8_t)(digit - HEX);
		}
		else
			return 0;
	}
	else
	{
//cpg 19 Jan 2007 1>c:\work\sack\src\sqllib\sqlutil.c(211) : warning C4244: '+=' : conversion from '__w64 int' to 'sack::uint8_t', possible loss of data
		value += (uint8_t)(digit - hex);
	}
	return value;
}

TEXTSTR DeblobifyString( CTEXTSTR blob, TEXTSTR outbuf, size_t outbuflen  )
{

	TEXTCHAR *result;
	TEXTCHAR *x, *y;

	if( blob )
	{
		//lprintf("got blob %s", blob);
		if( !outbuf )
			outbuf = NewArray( TEXTCHAR, ( ( strlen( blob ) / 2 ) + 1 ) );
		result = outbuf;
		for( x=(TEXTSTR)blob, y = result;
			  x[0] && ((size_t)(y-outbuf) < outbuflen);
			  y++, x+=2 )
		{
			y[0] = hexbyte( x );
			y[1] = 0;
			//lprintf("y is %s", y);
		}
		//lprintf("returning %s", result );
		return result;
	}
	else
		lprintf( "Duh.  No Blob." );
	return NULL;
}

//---------------------------------------------------------------------------

TEXTSTR RevertEscapeBinary( CTEXTSTR blob, size_t *bloblen )
{
	TEXTCHAR *tmpnamebuf, *result;
	CTEXTSTR pBlob = blob;
	int n;
	int escape;
	int targetlen;
	n = 0;

	escape = 0;
	targetlen = 0;
	for( n = 0; (*pBlob); pBlob++, n++ )
	{
		if( !escape && ( (*pBlob) == '\\' ) )
			escape = 1;
		else if( escape )
		{
			if( (*pBlob) == '\\' ||
				(*pBlob) == '0' ||
				(*pBlob) =='\'' ||
				(*pBlob) == '\"' )
			{
				// targetlen is a subtraction for missing charactercount
				targetlen++;
			}
			escape = 0;
		}
	}
	if( bloblen )
	{
		(*bloblen) = n - targetlen;
	}

	escape = 0;
	result = tmpnamebuf = NewArray( TEXTCHAR, (*bloblen) ); //-V522
	for( n = 0; (*pBlob); n++ )
	{
		if( !escape && ( (*pBlob) == '\\' ) )
			escape = 1;
		else if( escape )
		{
			if( (*pBlob) == '\\' ||
				(*pBlob) =='\'' ||
				(*pBlob) == '\"' )
			{
			// targetlen is a subtraction for missing charactercount
				(*tmpnamebuf++) = (*pBlob);
			}
			else if( (*pBlob) == '0' )
				(*tmpnamebuf++) = 0;
			else
			{
				(*tmpnamebuf++) = '\\';
				(*tmpnamebuf++) = (*pBlob);
			}
			escape = 0;
		}
		else
			(*tmpnamebuf++) = (*pBlob);
	}

	(*tmpnamebuf) = 0; // best terminate this thing.
	return result;
}

//---------------------------------------------------------------------------

TEXTSTR RevertEscapeString( CTEXTSTR name )
{
	return RevertEscapeBinary( name, NULL );
}

//---------------------------------------------------------------------------

 INDEX  SQLReadNameTableExEx( PODBC odbc, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS )
{
			TEXTCHAR query[256];
			TEXTCHAR *tmp;
			CTEXTSTR result = NULL;
			INDEX IDName = INVALID_INDEX;
			if( !table || !name )
				return INVALID_INDEX;

			// look in internal cache first...
			IDName = GetIndexOfName( odbc,table,name);
			if( IDName != INVALID_INDEX )
				return IDName;

			PushSQLQueryEx( odbc );
			tmp = EscapeSQLStringEx( odbc, name DBG_RELAY );
			tnprintf( query, sizeof( query ), "select %s from %s where %s like \'%s\'", col?col:"id", table, namecol, tmp );
			Release( tmp );
			if( SQLQueryEx( odbc, query, &result DBG_RELAY) && result )
			{
				IDName = (INDEX)IntCreateFromText( result );
			}
			else if( bCreate )
			{
				TEXTSTR newval = EscapeSQLString( odbc, name );
				tnprintf( query, sizeof( query ), "insert into %s (%s) values( \'%s\' )", table, namecol, newval );
				if( !SQLCommandEx( odbc, query DBG_RELAY ) )
				{
					lprintf( "insert failed, how can we define name %s?", name );
					// inser failed...
				}
				else
				{
					// all is well.
					IDName = FetchLastInsertIDEx( odbc, table, col?col:"id" DBG_RELAY );
				}
				Release( newval );
			}
			else
				IDName = INVALID_INDEX;

			PopODBCEx(odbc);

			if( IDName != INVALID_INDEX )
			{
				// instead of strdup, consider here using SaveName from procreg?
				AddBinaryNode( GetTableCache(odbc,table), (POINTER)((uintptr_t)(IDName+1))
								 , (uintptr_t)StrDup( name ) );
			}
			return IDName;
}

 INDEX  ReadNameTableExEx( CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS )
{
	return SQLReadNameTableExEx( NULL, name, table,col,namecol,bCreate DBG_RELAY );
}

 TEXTSTR  SQLReadNameTableKeyExEx( PODBC odbc, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS )
 {
	 TEXTCHAR query[256];
	 TEXTCHAR *tmp;
	 CTEXTSTR result = NULL;
	 TEXTSTR IDName = NULL;
	 if( !table || !name )
		 return IDName;

	 // look in internal cache first...
	 IDName = (TEXTSTR)GetKeyOfName( odbc, table, name );
	 if( IDName != NULL )
		 return IDName;

	 PushSQLQueryEx( odbc );
	 tmp = EscapeSQLStringEx( odbc, name DBG_RELAY );
	 tnprintf( query, sizeof( query ), "select %s from %s where %s like \'%s\'", col ? col : "id", table, namecol, tmp );
	 Release( tmp );
	 if( SQLQueryEx( odbc, query, &result DBG_RELAY ) && result ) {
		 IDName = StrDup( result );
	 }
	 else if( bCreate ) {
		 TEXTSTR newval = EscapeSQLString( odbc, name );
		 tnprintf( query, sizeof( query ), "insert into %s (%s) values( \'%s\' )", table, namecol, newval );
		 if( !SQLCommandEx( odbc, query DBG_RELAY ) ) {
			 lprintf( "insert failed, how can we define name %s?", name );
			 // inser failed...
		 }
		 else {
			 // all is well.
			 IDName = (TEXTSTR)FetchLastInsertKeyEx( odbc, table, col ? col : "id" DBG_RELAY );
		 }
		 Release( newval );
	 }

	 PopODBCEx( odbc );

	 if( IDName ) {
		 // instead of strdup, consider here using SaveName from procreg?
		 AddBinaryNode( GetTableCache( odbc, table ), IDName 
			 , (uintptr_t)StrDup( name ) );
	 }
	 return IDName;
 }


//---------------------------------------------------------------------------

 INDEX  ReadNameTableEx( CTEXTSTR name, CTEXTSTR table, CTEXTSTR col DBG_PASS )
{
	return ReadNameTableExEx( name,table,col,"name",TRUE DBG_RELAY);
}

//---------------------------------------------------------------------------

 int  ReadFromNameTableEx ( INDEX id, CTEXTSTR table, CTEXTSTR id_colname, CTEXTSTR name_colname, CTEXTSTR *result DBG_PASS)
{
	TEXTCHAR query[256];
	if( !result || !table || id == INVALID_INDEX )
		return FALSE;
	// the tree locally cached is in NAME order, but the data is
	// the key, so we would have to scan the tree otherwise both directions
	// keyed so that we could get the name key from the ID data..
	tnprintf( query, sizeof( query ), "select %s from %s where %s=%" _size_f
			  , name_colname?name_colname:"name"
			  , table
			  , id_colname?id_colname:"id"
			  , id );
	if( !DoSQLQueryEx( query, result DBG_RELAY ) || !(*result) )
	{
		lprintf( "name ID(%" _size_f " as %s) was not found in %s.%s", id, id_colname?id_colname:"id", table, id_colname?id_colname:"id" );
		return FALSE;
	}
	else
	{
		//PopODBC();
	}
	return TRUE;
}

//---------------------------------------------------------------------------

 int  ReadFromNameTableExEx ( INDEX id, CTEXTSTR table, CTEXTSTR id_col, CTEXTSTR colname, CTEXTSTR *result DBG_PASS)
{
	TEXTCHAR query[256];
	if( !result || !table || id == INVALID_INDEX )
		return FALSE;
	// the tree locally cached is in NAME order, but the data is
	// the key, so we would have to scan the tree otherwise both directions
	// keyed so that we could get the name key from the ID data..
	tnprintf( query, sizeof( query ), "select %s from %s where %s=%" _size_f
			, colname
			  , table
			 , id_col?id_col:"id"
			  , id );
	if( !DoSQLQueryEx( query, result DBG_RELAY ) || !(*result) )
	{
		lprintf( "name ID(%" _size_fs ") was not found in %s.%s", id, table, colname?colname:"id" );
		return FALSE;
	}
	else
	{
		//PopODBC();
	}
	return TRUE;
}

//---------------------------------------------------------------------------

 int  SQLCreateTableEx ( PODBC odbc, CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, uint32_t options )
{
	//CTEXTSTR result;
	//TEXTCHAR query[256];
	if( !tablename )
		tablename = templatename;
	//if( odbc->flags.bSQLite_native )
	//	;
	//else if( odbc->flags.bAccess )
	//	tnprintf( query, 256, "DESCRIBE [%s]", tablename );
	//else
	//	tnprintf( query, 256, "DESCRIBE `%s`", tablename );
	//if( !SQLQuery( odbc, query, &result ) || !result || (options & (CTO_DROP|CTO_MATCH|CTO_MERGE)) )
	{
		TEXTCHAR sec_file[284];
		FILE *file;
		sec_file[0] = 0;
		file = sack_fopen( 0, filename, "rt" );
		if( !file )
		{
			if( !pathchr( filename ) )
			{
#ifndef HAVE_ENVIRONMENT
				CTEXTSTR path = OSALOT_GetEnvironmentVariable( "MY_LOAD_PATH" );
				tnprintf( sec_file, sizeof( sec_file ), "%s/%s", path, filename );
#else
				tnprintf( sec_file, sizeof( sec_file ), "%s", filename );
#endif
				file = sack_fopen( 0, sec_file, "rt" );
			}
		}
		if( file )
		{
			int done = FALSE;
			int gathering = FALSE;
			TEXTCHAR *buf;
			TEXTCHAR fgets_buf[1024];
			PVARTEXT pvt_cmd = VarTextCreate();
			INDEX nOfs = 0;
			if( !odbc->flags.bNoLogging )
				lprintf( "Opened %s to read for table %s(%s)", sec_file[0]?sec_file:filename, tablename,templatename );
			while( sack_fgets( fgets_buf, sizeof( fgets_buf ), file ) )
			{
				TEXTCHAR *p;
				buf = fgets_buf;
				p = buf + strlen( buf ) - 1;
				while( p[0] == ' ' || p[0] == '\t' || p[0] == '\n' || p[0] == '\r')
				{
					p[0] = 0;
					p--;
				}
				p = buf;
				while( p[0] )
				{
					if ( p[0] == '#' )
					{
						p[0] = 0;
						break;
					}
					p++;
				}
				p = buf;
				while( p[0] == ' ' || p[0] == '\t' )
					p++;
				if( p[0] )
				{
					// have content on the line...
					TEXTCHAR *end = p + strlen( p ) - 1;
					if( gathering && end[0] == ';' )
					{
						done = TRUE;
						end[0] = 0;
					}
					if( !gathering )
					{
						if( StrCaseCmpEx( p, "CREATE", 6 ) == 0 )
						{
							CTEXTSTR tabname;
							// cpg29dec2006  c:\work\sack\src\sqllib\sqlutil.c(498) : warning C4267: 'initializing' : conversion from 'size_t' to 'int', possible loss of data
							// cpg29dec2006                             int len = strlen( tablename );
							int len = (int)strlen( tablename );
							if( (tabname = StrStr( p, templatename )) &&
								(tabname[len] == '`' || tabname[len] ==' ' || tabname[len] =='\t' ) &&
								(tabname[-1] == '`' || tabname[-1] ==' ' || tabname[-1] =='\t'))
							{
								// need to gather, repace...
								TEXTCHAR line[1024];
								CTEXTSTR trailer;
								trailer = tabname;
								while( trailer[0] != '\'' &&
										trailer[0] != '`' &&
										trailer[0] != ' ' &&
										trailer[0] != '\t' )
									trailer++;
								tnprintf( line, sizeof( line ), "%*.*s%s%s"
										 , (int)(tabname - p), (int)(tabname - p), p
										 , templatename
										 , trailer
										 );
								StrCpyEx( buf, line, 1024 );
								gathering = TRUE;
							}
							else
							{

							}
						}
					}
					if( gathering )
					{
						nOfs += vtprintf( pvt_cmd, "%s ", p );
						if( done )
						{
							if( options & CTO_DROP )
							{
								TEXTCHAR buf[1024];
								tnprintf( buf, 1024, "Drop table %s", templatename );
								if( !SQLCommand( odbc, buf ) )
								{
									CTEXTSTR result;
									GetSQLError( &result );
									lprintf( "Failed to do drop: %s", result );
								}
							}
							// result is set with the first describe result
							// the matching done in CheckMySQLTable should
							// be done here, after parsing the line from the file (cmd)
							// into a TABLE structure.
							//DebugBreak();
							{
								PTABLE table;
								PTEXT cmd = VarTextGet( pvt_cmd );
								table = GetFieldsInSQL( GetText( cmd ), 1 );
								CheckODBCTable( odbc, table, options );
								DestroySQLTable( table );
								LineRelease( cmd );
							}
							break;
						}
					}
				}
#ifdef __cplusplus_cli
				Release( buf );
#endif
			}
			//lprintf( "Done with create..." );
			VarTextDestroy( &pvt_cmd );
			sack_fclose( file );
		}
		else
		{
			lprintf( "Unable to open templatefile: %s or %s/%s", filename
					 , OSALOT_GetEnvironmentVariable( "MY_LOAD_PATH" )
			        , filename );

		}
	}
	return TRUE;
}

//---------------------------------------------------------------------------

void DestroySQLTable( PTABLE table )
{
	int n;
	int m;
	if( !table )
		return;
// don't release tables created statically in C files...
	if( !table->flags.bDynamic )
		return;
	for( n = 0; n < table->fields.count; n++ )
	{
		Release( (POINTER)table->fields.field[n].name );
		Release( (POINTER)table->fields.field[n].type );
		Release( (POINTER)table->fields.field[n].extra );
		for( m = 0; table->fields.field[n].previous_names[m] && m < MAX_PREVIOUS_FIELD_NAMES; m++ )
		{
			Release( (POINTER)table->fields.field[n].previous_names[m] );
		}
	}
	for( n = 0; n < table->keys.count; n++ )
	{
		for( m = 0; table->keys.key[n].colnames[m] && m < MAX_KEY_COLUMNS; m++ )
		{
			Release( (POINTER)table->keys.key[n].colnames[m] );
		}
		Release( (POINTER)table->keys.key[n].name );
	}
	for( n = 0; n < table->constraints.count; n++ )
	{
		for( m = 0; table->constraints.constraint[n].colnames[m] && m < MAX_KEY_COLUMNS; m++ )
		{
			Release( (POINTER)table->constraints.constraint[n].colnames[m] );
		}
		for( m = 0; table->constraints.constraint[n].foriegn_colnames[m] && m < MAX_KEY_COLUMNS; m++ )
		{
			Release( (POINTER)table->constraints.constraint[n].foriegn_colnames[m] );
		}
		Release( (POINTER)table->constraints.constraint[n].name );
		Release( (POINTER)table->constraints.constraint[n].references );
	}
	Release( (POINTER)table->name );
	Release( (POINTER)table->constraints.constraint );
	Release( (POINTER)table->fields.field );
	Release( (POINTER)table->keys.key );
	Release( table );
}

void DumpSQLTable( PTABLE table )
{
	int n;
	int m;
	//if( 1 )
	//   return;
	// don't release tables created statically in C files...
	lprintf( "Table name: %s", table->name );
	for( n = 0; n < table->fields.count; n++ )
	{
		lprintf( "Column %d '%s' [%s] [%s]"
		        , n
				 ,( table->fields.field[n].name )
				, table->fields.field[n].type?table->fields.field[n].type:""
				 ,( table->fields.field[n].extra )
				 );
		for( m = 0; table->fields.field[n].previous_names[m] && m < MAX_PREVIOUS_FIELD_NAMES; m++ )
		{
			//Release( (POINTER)table->fields.field[n].previous_names[m] );
		}
	}
	for( n = 0; n < table->keys.count; n++ )
	{
		lprintf( "Key %s", table->keys.key[n].name?table->keys.key[n].name:"<NONAME>" );
		for( m = 0; table->keys.key[n].colnames[m] && m < MAX_KEY_COLUMNS; m++ )
		{
			lprintf( "Key part = %s"
					 , table->keys.key[n].colnames[m]
					 );
		}
	}
}


 int  CreateTableEx ( CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, uint32_t options )
{
	OpenSQL( DBG_VOIDSRC );
	if( g.odbc )
		return SQLCreateTableEx( g.odbc, filename, templatename, tablename, options );
	return FALSE;
}

#define FAILPARSE() do { if( ( start[0] < '0' ) || ( start[0] > '9' ) ) {  \
lprintf( "string fails date parsing... %s", timestring );                  \
return 0; } } while (0);

 int  ConvertDBTimeString ( CTEXTSTR timestring
                                        , CTEXTSTR *endtimestring
													 , int *pyr, int *pmo, int *pdy
													 , int *phr, int *pmn, int *psc )
{
	int mo,dy,yr;
	int hr = 0,mn = 0,sc = 0;
	CTEXTSTR start;
	start = timestring;
	if( !start )
	{
		if( pyr ) (*pyr) = 0;
		if( pmo ) (*pmo) = 0;
		if( pdy ) (*pdy) = 0;
		if( phr ) (*phr) = 0;
		if( pmn ) (*pmn) = 0;
		if( psc ) (*psc) = 0;
		return 0;
	}
	yr = 0;
	FAILPARSE();
	yr = (yr * 10) + (*start++) - '0';
	FAILPARSE();
	yr = (yr * 10) + (*start++) - '0';
	FAILPARSE();
	yr = (yr * 10) + (*start++) - '0';
	FAILPARSE();
	yr = (yr * 10) + (*start++) - '0';
	if( (*start) == '-' ) start++;
	mo = 0;
	FAILPARSE();
	mo = (mo * 10) + (*start++) - '0';
	FAILPARSE();
	mo = (mo * 10) + (*start++) - '0';
	if( (*start) == '-' ) start++;
	dy = 0;
	FAILPARSE();
	dy = (dy * 10) + (*start++) - '0';
	FAILPARSE();
	dy = (dy * 10) + (*start++) - '0';
	if( (*start) == ' ' )
	{
		start++;
		FAILPARSE();
		hr = (hr * 10) + (*start++) - '0';
		FAILPARSE();
		hr = (hr * 10) + (*start++) - '0';
		if( (*start) == ':' ) start++;
		FAILPARSE();
		mn = (mn * 10) + (*start++) - '0';
		FAILPARSE();
		mn = (mn * 10) + (*start++) - '0';
		if( (*start) == ':' ) start++;
		FAILPARSE();
		sc = (sc * 10) + (*start++) - '0';
		FAILPARSE();
		sc = (sc * 10) + (*start++) - '0';
	}
	if( endtimestring )
		*endtimestring = start;
	if( pyr )
		*pyr = yr;
	if( pmo )
		*pmo = mo;
	if( pdy )
		*pdy = dy;
	if( phr )
		*phr = hr;
	if( pmn )
		*pmn = mn;
	if( psc )
		*psc = sc;
	return 1;
}

LOGICAL CheckAccessODBCTable( PODBC odbc, PTABLE table, uint32_t options )
{
	CTEXTSTR *_fields = NULL;
	CTEXTSTR *fields;
	int columns;
	PVARTEXT pvtCreate = NULL;
	CTEXTSTR cmd = "select top 1 * from [%s]";
	int retry = 0;
retry:
	if( SQLRecordQueryf( odbc, &columns, NULL, &_fields, cmd, table->name ) )
	{
		int n;
		fields = NewArray( CTEXTSTR, columns );
		for( n = 0; n < columns; n++ )
			fields[n] = StrDup( _fields[n] );
		for( n = 0; n < columns; n++ )
		{
			int m;
			for( m = 0; m < table->fields.count; m++ )
			{
				if( StrCaseCmp( fields[n], table->fields.field[m].name ) == 0 )
					break;
			}
			if( m == table->fields.count )
			{
				// did not find this column in the definition drop it.
				{
					for( m = 0; m < table->fields.count; m++ )
					{
						int prev;
						for( prev = 0; table->fields.field[m].previous_names[prev]; prev++ )
						{
							if( strcmp( table->fields.field[m].previous_names[prev]
										 , fields[n] ) == 0 )
							{
								break;
							}
						}
						if( table->fields.field[m].previous_names[prev] )
							break;
					}
					if( m < table->fields.count )
					{
						// this column was known to be named something else
						// and we should do some extra fun stuff to preserve the data
						// In access rename is done with DROP and ADD column
						ReleaseODBC( odbc ); // release so that the alter statement may be done.
						SQLCommandf( odbc
									  , "alter table [%s] add column [%s] %s%s%s"
									  , table->name
									  , table->fields.field[m].name
									  , table->fields.field[m].type?table->fields.field[m].type:""
									  , table->fields.field[m].extra?" ":""
									  , table->fields.field[m].extra?table->fields.field[m].extra:""
									  );
						SQLCommandf( odbc, "update [%s] set [%s]=[%s]"
									  , table->name
									  , table->fields.field[m].name
									  , fields[n] );
					}
					ReleaseODBC( odbc ); // release all prior locks on the table...
					SQLCommandf( odbc
								  , "alter table [%s] drop column [%s]"
								  , table->name
								  , fields[n] );

					if( m < table->fields.count )
					{
						/* this field is now handled and done, forget it.*/
						Release( (void*)fields[n] );
						// okay we already added this one, so make it
						// match our definition... otherwise following code will
						// also attempt to add this...
						// but in the process of dropping we may NOT
						// drop a renamed column- but must instead preserve it's data
						fields[n] = StrDup( table->fields.field[m].name );
					}
				}
			}
		}

		for( n = 0; n < table->fields.count; n++ )
		{
			int m;
			for( m = 0; m < columns; m++ )
			{
				if( fields[m] )
					if( StrCaseCmp( fields[m], table->fields.field[n].name ) == 0 )
						break;
			}
			if( m == columns )
			{
				// did not find this defined column in the table, add it.
				PTEXT pt_cmd;
				if( !pvtCreate )
					pvtCreate = VarTextCreate();
				vtprintf( pvtCreate, "alter table [%s] add column [%s] %s%s%s"
						  , table->name
						  , table->fields.field[n].name
						  , table->fields.field[n].type?table->fields.field[n].type:""
						  , table->fields.field[n].extra?" ":""
						  , table->fields.field[n].extra?table->fields.field[n].extra:""
						  );
				pt_cmd = VarTextGet( pvtCreate );
				// close all prior statement handles so it's not locked
				// especially my own.
				PopODBCEx( odbc ); // release so that the alter statement may be done.
				SQLCommand( odbc, GetText( pt_cmd ) );
				LineRelease( pt_cmd );
			}
		}
		// release the duplicated fields...
		for( n = 0; n < columns; n++ )
			Release( (POINTER)fields[n] );
		Release( (POINTER)fields );
	}
	else
	{
		// table doesn't exist?
		CTEXTSTR error = NULL;
		FetchSQLError( odbc, &error );
		if( StrCmpEx( error, "(37000)", 7 ) == 0 )
		{
			// ODBC driver is old and does not support
			// 'TOP' command... please try again, using a less fancy
			// select... since it's file based, probably the data is not
			// all read, but one row at a time is read from the database.
			if( !retry )
			{
				cmd = "select * from [%s]";
				retry++;
				goto retry;
			}
		}
		if( StrCmpEx( error, "(S0002)", 7 ) == 0 )
		{
			PTEXT pt_cmd;
			int n;
			int first = 1;
			if( !pvtCreate )
				pvtCreate = VarTextCreate();
			vtprintf( pvtCreate, "create table [%s] (", table->name );
			for( n = 0; n < table->fields.count; n++ )
			{
				CTEXTSTR type;
				if( StrCaseCmpEx( table->fields.field[n].type, "varchar", 7 ) == 0 )
					type = "TEXT";
				else if( StrCaseCmpEx( table->fields.field[n].type, "tinyint", 7 ) == 0 )
					type = "INT";
				else if( StrCaseCmpEx( table->fields.field[n].type, "int(", 4 ) == 0 )
					type = "INT";
				else
				{
					if( table->fields.field[n].extra && StrStr( table->fields.field[n].extra, "auto_increment" ) )
						type = "COUNTER";
					else
						type = table->fields.field[n].type;
				}
				if( strchr( table->fields.field[n].name, ' ' ) )
				{
					vtprintf( pvtCreate, "%s[%s] %s%s%s"
							  , first?"":","
							  , table->fields.field[n].name
							  , type?type:""
							  , ""//table->fields.field[n].extra?" ":""
							  , ""//table->fields.field[n].extra?table->fields.field[n].extra:""
							  );
				}
				else
				{
					vtprintf( pvtCreate, "%s[%s] %s%s%s"
							  , first?"":","
							  , table->fields.field[n].name
							  , type?type:""
							  , ""//(strstr( table->fields.field[n].extra, "auto_increment" ))?"COUNTER":""
							  , ""//table->fields.field[n].extra?table->fields.field[n].extra:""
							  );
				}
				first = 0;
			}
			// not even sure where in the syntax key fields go...
			// does access actually have key fields?  or just things
			// called key fields
			//for( n = 0; n < table->keys.count; n++ )
			//{
				// for implementation see Check MYSQL
			//}
			vtprintf( pvtCreate, ")" );
			pt_cmd = VarTextGet( pvtCreate );
			SQLCommand( odbc, GetText( pt_cmd ) );
			LineRelease( pt_cmd );
		}
		else
		{
			lprintf( "error is : %s", error );
		}
	}
	if( pvtCreate )
		VarTextDestroy( &pvtCreate );
	return 1;
}

LOGICAL CPROC CheckMySQLODBCTable( PODBC odbc, PTABLE table, uint32_t options )
{
// when this gets to be implemented...
// the type "counter" needs to be interpreted as auto increment.
// also, the behavior for mysql auto increment (in extra fields )
// needs to be interpreted counter-intuitively for access databases..

	CTEXTSTR *fields = NULL;
	CTEXTSTR *result = NULL;
	FILE *f_odbc = NULL;
	int columns;
	int retry;
	int success;
	int buflen;
	PVARTEXT pvtCreate = NULL;
	int status = 1;
	TEXTCHAR *cmd;
	if( options & CTO_LOG_CHANGES )
	{
		f_odbc = sack_fopen( 0, "changes.sql", "at+" );
		if( !f_odbc )
			f_odbc = sack_fopen( 0, "changes.sql", "wt" );
	}
	cmd = NewArray( TEXTCHAR, 1024);
	buflen = 0;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
		buflen += tnprintf( cmd+buflen , 1024-buflen,"select tbl_name,sql from sqlite_master where type='table' and name='%s'"
								, table->name );
	else
#endif
		buflen += tnprintf( cmd+buflen , 1024-buflen,"show create table `%s`" ,table->name);
	if( buflen < 1024 )
		cmd[buflen] = 0;
	else
		cmd[1023] = 0;
	retry = 0;
retry:
	if( ( success = SQLRecordQueryf( odbc, &columns, &result, &fields, cmd, table->name ) )
		&& result )
			//    if( DoSQLQuery( cmd, &result ) && result )
	{
		int n;
		PTABLE pTestTable;
		//lprintf("Does this work or not?");
		pTestTable = GetFieldsInSQL( result[1] , 0 );
		//lprintf( "closing the odbc %p", odbc );
		SQLEndQuery( odbc );
		//lprintf(" ---------------Table to test-----------------------------------------" );
		//DumpSQLTable( pTestTable );
		//lprintf(" ---------------original table -----------------------------------------" );
		//DumpSQLTable( table );
		//lprintf(" -----------------end tables ---------------------------------------" );
		//lprintf(" . . . I guess so");
		if( pTestTable )
		{
			for( n = 0; n < pTestTable->fields.count; n++ )
			{
				int m;
				for( m = 0; m < table->fields.count; m++ )
				{
					if( StrCaseCmp( pTestTable->fields.field[n].name
								  , table->fields.field[m].name ) == 0 )
						break;
				}
				if( m == table->fields.count )
				{
				// did not find this column in the definition drop it.
					{
						int prev = 0;
						for( m = 0; m < table->fields.count; m++ )
						{
							for( prev = 0; table->fields.field[m].previous_names[prev]; prev++ )
							{
								if( strcmp( table->fields.field[m].previous_names[prev]
											 , pTestTable->fields.field[n].name ) == 0 )
								{
									break;
								}
							}
							if( table->fields.field[m].previous_names[prev] )
								break;
						}
						if( m < table->fields.count )
						{
						// this column was known to be named something else
						// and we should do some extra fun stuff to preserve the data
						// In access rename is done with DROP and ADD column
							if( options & CTO_DROP )
							{
								if( f_odbc )
									fprintf( f_odbc, "drop table `%s`;\n", table->name );
								else
									SQLCommandf( odbc, "drop table `%s`", table->name );
								goto do_create_table;
							}
							if( f_odbc )
							{
								fprintf( f_odbc
										 , "alter table `%s` add column `%s` %s%s%s;\n"
										 , table->name
										 , table->fields.field[m].name
										 , table->fields.field[m].type?table->fields.field[m].type:""
										 , table->fields.field[m].extra?" ":""
										 , table->fields.field[m].extra?table->fields.field[m].extra:""
										 );
								fprintf( f_odbc, "update `%s` set `%s`=`%s`;\n"
										 , table->name
										 , table->fields.field[m].name
										 , table->fields.field[m].previous_names[prev] );
							}
							else
							{
								SQLCommandf( odbc
											  , "alter table `%s` add column `%s` %s%s%s"
											  , table->name
											  , table->fields.field[m].name
											 , table->fields.field[m].type?table->fields.field[m].type:""
											  , table->fields.field[m].extra?" ":""
											  , table->fields.field[m].extra?table->fields.field[m].extra:""
											  );
								SQLCommandf( odbc, "update `%s` set `%s`=`%s`"
											  , table->name
											  , table->fields.field[m].name
											  , table->fields.field[m].previous_names[prev] );
							}
						}
						if( !( options & CTO_MERGE ) )
						{
							if( options & CTO_DROP )
							{
								if( f_odbc )
									fprintf( f_odbc, "drop table `%s`", table->name );
								else
									SQLCommandf( odbc, "drop table `%s`", table->name );
								goto do_create_table;
							}
							if( f_odbc )
								fprintf( f_odbc
											  , "alter table `%s` drop column `%s`;\n"
											  , table->name
											  , pTestTable->fields.field[n].name );
							else
								SQLCommandf( odbc
											  , "alter table `%s` drop column `%s`"
											  , table->name
											  , pTestTable->fields.field[n].name );

							if( m < table->fields.count )
							{
							/* this field is now handled and done, forget it.*/
								Release( (void*)pTestTable->fields.field[n].name );
							// okay we already added this one, so make it
							// match our definition... otherwise following code will
							// also attempt to add this...
							// but in the process of dropping we may NOT
							// drop a renamed column- but must instead preserve it's data
								pTestTable->fields.field[n].name = StrDup( table->fields.field[m].name );
							}
						}
					}
				}
			}

			for( n = 0; n < table->fields.count; n++ )
			{
				int m;
				for( m = 0; m < pTestTable->fields.count; m++ )
				{
				//                if( fields[m] )
					if( StrCaseCmp( pTestTable->fields.field[m].name, table->fields.field[n].name ) == 0 )
						break;
				}
				if( m == pTestTable->fields.count )
				{
				// did not find this defined column in the table, add it.
					PTEXT txt_cmd;
					if( options & CTO_DROP )
					{
						if( f_odbc )
							fprintf( f_odbc, "drop table `%s`", table->name );
						else
							SQLCommandf( odbc, "drop table `%s`", table->name );
						goto do_create_table;
					}
					if( !pvtCreate )
						pvtCreate = VarTextCreate();
					vtprintf( pvtCreate, "alter table `%s` add column `%s` %s%s%s"
							  , table->name
							  , table->fields.field[n].name
							  , table->fields.field[n].type?table->fields.field[n].type:""
							  , table->fields.field[n].extra?" ":""
							  , table->fields.field[n].extra?table->fields.field[n].extra:""
							  );
					txt_cmd = VarTextGet( pvtCreate );
					if( f_odbc )
						fprintf( f_odbc, "%s;\n", GetText( txt_cmd ) );
					else
						SQLCommand( odbc, GetText( txt_cmd ) );
					LineRelease( txt_cmd );
				}
			}
		}
		DestroySQLTable( pTestTable );
	}
	else
	{
		// table doesn't exist?
		if( success && !result )
		{
			// do_create_table results in a PopODBC, so this one is redundant.
			// and we don't want to pop before it gets popped below.
			//PopODBCEx(odbc);
			goto do_create_table;
		}
		if( !success )
		{
			CTEXTSTR error;
			error = NULL;
			FetchSQLError( odbc, &error );
			if( StrCmpEx( error, "(37000)", 7 ) == 0 )
			{
				// ODBC driver is old and does not support
				// 'TOP' command... please try again, using a less fancy
				// select... since it's file based, probably the data is not
				// all read, but one row at a time is read from the database.
				if( !retry )
				{
					StrCpyEx( cmd, "select * from `%s`", 1024 );
					retry++;
					PopODBCEx(odbc);
					goto retry;
				}
			}
			if( StrCmpEx( error, "(S0002)", 7 ) == 0 )
			{
				PTEXT txt_cmd;
				int n;
				int first;
				CTEXTSTR auto_increment_column;
			do_create_table:
				auto_increment_column = NULL;
				first = 1;
				if( !pvtCreate )
					pvtCreate = VarTextCreate();
				vtprintf( pvtCreate, "create table `%s` (", table->name );
				for( n = 0; n < table->fields.count; n++ )
				{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
					if( odbc->flags.bSQLite_native )
					{
						if( table->fields.field[n].extra
							&&  StrCaseStr( table->fields.field[n].extra, "auto_increment" ) )
						{
							if( auto_increment_column )
								lprintf( "SQLITE ERROR: Failure will happen - more than one auto_increment" );
							auto_increment_column = table->fields.field[n].name;
							vtprintf( pvtCreate, "%s`%s` %s%s"
									  , first?"":","
									  , table->fields.field[n].name
									  , "INTEGER" //table->fields.field[n].type
									  , " PRIMARY KEY"
									  );
						}
						else
						{
							CTEXTSTR unsigned_word;
							if(  table->fields.field[n].extra
								&& (unsigned_word=StrStr( table->fields.field[n].extra
								                        , "unsigned" )) )
							{
								TEXTSTR extra = StrDup( table->fields.field[n].extra );
								size_t len = StrLen( unsigned_word + 8 );
								// use same buffer allocated to write into...
								tnprintf( extra, strlen( table->fields.field[n].extra ), "%*.*s%*.*s"
								       , (int)(unsigned_word-table->fields.field[n].extra)
								       , (int)(unsigned_word-table->fields.field[n].extra)
									   , table->fields.field[n].extra
									   , (int)len 
									   , (int)len
									   , unsigned_word + 8
									   );
								vtprintf( pvtCreate, "%s`%s` %s %s"
										  , first?"":","
										  , table->fields.field[n].name
										  , table->fields.field[n].type? table->fields.field[n].type:""
										  , extra
										  );
								Release( extra );
							}
							else
								vtprintf( pvtCreate, "%s`%s` %s%s%s"
										  , first?"":","
										  , table->fields.field[n].name
										  , table->fields.field[n].type ? table->fields.field[n].type : ""
										  , table->fields.field[n].extra?" ":""
										  , table->fields.field[n].extra?table->fields.field[n].extra:""
										  );
						}
					}
					else
#endif
						vtprintf( pvtCreate, "%s`%s` %s%s%s"
								  , first?"":","
								  , table->fields.field[n].name
								  , table->fields.field[n].type ? table->fields.field[n].type : ""
								  , table->fields.field[n].extra?" ":""
								  , table->fields.field[n].extra?table->fields.field[n].extra:""
								  );

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
					if( odbc->flags.bSQLite_native )
					{
						int k;
						for( k = 0; k < table->keys.count; k++ )
						{
							if( table->keys.key[k].flags.bPrimary && !table->keys.key[k].colnames[1] )
							{
								if( StrCmp( table->keys.key[k].colnames[0], table->fields.field[n].name ) == 0 )
								{
									vtprintf( pvtCreate, " PRIMARY KEY" );
								}
							}
							if( table->keys.key[k].flags.bUnique && !table->keys.key[k].colnames[1] )
							{
								if( StrCmp( table->keys.key[k].colnames[0], table->fields.field[n].name ) == 0 )
								{
									vtprintf( pvtCreate, " UNIQUE" );
								}
							}
						}
					}
#endif

					first = 0;
				}
				for( n = 0; n < table->keys.count; n++ )
				{
					int col;
					int colfirst = 1;
					if( table->keys.key[n].flags.bPrimary )
					{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
						if( odbc->flags.bSQLite_native )
						{
							if( auto_increment_column )
							{
								if( table->keys.key[n].colnames[1] )
								{
									lprintf( "SQLITE ERROR: Complex PRIMARY KEY promoting to UNIQUE" );
									vtprintf( pvtCreate, "%sUNIQUE `primary` ("
											  , first?"":"," );
								}
								if( strcmp( auto_increment_column, table->keys.key[n].colnames[0] ) ) {
									lprintf( "SQLITE ERROR: auto_increment column was not the PRMIARY KEY" );
								} else
								{
									// ignore key
									continue;
								}
							}
							else
							{
								//vtprintf( pvtCreate, "%sPRIMARY KEY ("
								//		  , first?"":"," );
							}
						}
						else
#endif
						{
							vtprintf( pvtCreate, "%sPRIMARY KEY ("
									  , first?"":"," );
							for( col = 0; table->keys.key[n].colnames[col]; col++ )
							{
								if( !table->keys.key[n].colnames[col] )
									break;
								vtprintf( pvtCreate, "%s`%s`"
										  , colfirst?"":","
										  , table->keys.key[n].colnames[col]
										  );
								colfirst = 0;
							}
							vtprintf( pvtCreate, ")" );
						}
					}
					else
					{

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
						if( odbc->flags.bSQLite_native )
						{
							if( table->keys.key[n].flags.bUnique )
							{
								if( table->keys.key[n].colnames[1] )
								{
									int c;
									vtprintf( pvtCreate, "%sCONSTRAINT `%s` UNIQUE ("
											  , first?"":","
											  , table->keys.key[n].name
											  );
									for( c = 0; table->keys.key[n].colnames[c]; c++ )
										vtprintf( pvtCreate, "%s`%s`", (c==0)?"":",", table->keys.key[n].colnames[c] );
									vtprintf( pvtCreate, ")" );
                           if( table->keys.key[n].flags.uniqueResolution != UNIQRES_UNSET )
										vtprintf( pvtCreate, "ON CONFLICT %s"
													, table->keys.key[n].flags.uniqueResolution == UNIQRES_REPLACE ? "REPLACE"
													: table->keys.key[n].flags.uniqueResolution == UNIQRES_ABORT ? "ABORT"
													: table->keys.key[n].flags.uniqueResolution == UNIQRES_FAIL ? "FAIL"
													: table->keys.key[n].flags.uniqueResolution == UNIQRES_IGNORE ? "IGNORE"
													: table->keys.key[n].flags.uniqueResolution == UNIQRES_ROLLBACK ? "ROLLBACK"
													: "ABORT"
												  );
									first = 0;
								}
							}
						}
						else
#endif
						{
							vtprintf( pvtCreate, "%s%s`%s` %s%s ("
									  , first?"":","
									  , table->keys.key[n].flags.bUnique?"CONSTRAINT ":""
									  , table->keys.key[n].name
									  , table->keys.key[n].flags.bUnique?"UNIQUE ":""
							        , table->keys.key[n].flags.bUnique?"":"KEY " );
			lprintf( "!!! Create Table statement: %s", GetText( VarTextPeek( pvtCreate ) ) );
							for( col = 0; table->keys.key[n].colnames[col]; col++ )
							{
								if( !table->keys.key[n].colnames[col] )
									break;
								vtprintf( pvtCreate, "%s`%s`"
										  , colfirst?"":","
										  , table->keys.key[n].colnames[col]
										  );
								colfirst = 0;
							}
							vtprintf( pvtCreate, ")" );
							first = 0;

						}
					}
				}
				for( n = 0; n < table->constraints.count; n++ )
				{
					int col;
					int colfirst = 1;
					{
						{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
							if( odbc->flags.bSQLite_native )
								vtprintf( pvtCreate, "%s FOREIGN KEY  ("
										  , first?"":"," );
							else
#endif
								vtprintf( pvtCreate, "%sCONSTRAINT `%s` FOREIGN KEY ("
										  , first?"":","
										  , table->constraints.constraint[n].name );
							colfirst = 1;
							for( col = 0; table->constraints.constraint[n].colnames[col]; col++ )
							{
								if( !table->constraints.constraint[n].colnames[col] )
									break;
								vtprintf( pvtCreate, "%s`%s`"
										  , colfirst?"":","
										  , table->constraints.constraint[n].colnames[col]
										  );
								colfirst = 0;
							}
							vtprintf( pvtCreate, ") REFERENCES `%s`("
									  , table->constraints.constraint[n].references );

							colfirst = 1;
							for( col = 0; table->constraints.constraint[n].foriegn_colnames[col]; col++ )
							{
								if( !table->constraints.constraint[n].foriegn_colnames[col] )
									break;
								vtprintf( pvtCreate, "%s`%s`"
										  , colfirst?"":","
										  , table->constraints.constraint[n].foriegn_colnames[col]
										  );
								colfirst = 0;
							}
							vtprintf( pvtCreate, ")%s %s"

									  , table->constraints.constraint[n].flags.cascade_on_update?"ON UPDATE CASCADE"
										:table->constraints.constraint[n].flags.restrict_on_update?"ON UPDATE RESTRICT"
										:table->constraints.constraint[n].flags.setnull_on_update?"ON UPDATE SET NULL"
										:table->constraints.constraint[n].flags.setdefault_on_update?"ON UPDATE SET DEFAULT"
										:table->constraints.constraint[n].flags.noaction_on_update?"ON UPDATE NO ACTION"
										:""
									  , table->constraints.constraint[n].flags.cascade_on_delete?"ON DELETE CASCADE"
										:table->constraints.constraint[n].flags.restrict_on_delete?"ON DELETE RESTRICT"
										:table->constraints.constraint[n].flags.setnull_on_delete?"ON DELETE SET NULL"
										:table->constraints.constraint[n].flags.setdefault_on_delete?"ON DELETE SET DEFAULT"
										:table->constraints.constraint[n].flags.noaction_on_delete?"ON DELETE NO ACTION"
										:""
									  );
 							first = 0;
						}
					}
				}
				vtprintf( pvtCreate, ")" ) ; // closing paren of all columns...
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
				if( !odbc->flags.bSQLite_native )
#endif
				{
					/* these are not supported under sqlite backend*/
					if( table->type )
						vtprintf( pvtCreate, "TYPE=%s",table->type ) ;
					//else
					//	vtprintf( pvtCreate, "TYPE=MyISAM" ); // cpg 15 dec 2006
					if( table->comment )
						vtprintf( pvtCreate, " COMMENT=\'%s\'", table->comment );
				}
				//PopODBCEx(odbc);

				txt_cmd = VarTextPeek( pvtCreate );
				if( f_odbc )
					fprintf( f_odbc, "%s;\n", GetText( txt_cmd ) );
				else
					if( !SQLCommand( odbc, GetText( txt_cmd ) ) )
						status = 0;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
				if( odbc->flags.bSQLite_native ) {
					VarTextEmpty( pvtCreate );
					for( n = 0; n < table->keys.count; n++ ) {
						int col;
						int colfirst;
						colfirst = 1;
						if( !table->keys.key[n].flags.bPrimary) {
							vtprintf( pvtCreate, "CREATE %sINDEX '%s' ON '%s'("
								, table->keys.key[n].flags.bUnique ? "UNIQUE " : ""
								, table->keys.key[n].name
								, table->name );
							for( col = 0; table->keys.key[n].colnames[col]; col++ ) {
								if( !table->keys.key[n].colnames[col] )
									break;
								vtprintf( pvtCreate, "%s'%s'"
									, colfirst ? "" : ","
									, table->keys.key[n].colnames[col]
								);
								colfirst = 0;
							}
							vtprintf( pvtCreate, ")" );
							first = 0;

							if( !SQLCommand( odbc, GetText( VarTextPeek( pvtCreate ) ) ) )
								status = 0;
							VarTextEmpty( pvtCreate );
						}
					}
				}
#endif

			}
			else
			{
				lprintf( "error is : %s", error );
			}
		}
	}
	Release(cmd);
	if( pvtCreate )
		VarTextDestroy( &pvtCreate );
	if( f_odbc )
		fclose( f_odbc );
	return status;
}


LOGICAL CheckODBCTableEx( PODBC odbc, PTABLE table, uint32_t options DBG_PASS )
{
	if( !odbc )
	{
		OpenSQL( DBG_VOIDRELAY );
		odbc = g.odbc;
	}

	if( !odbc )
		return FALSE;


			  // should check some kinda flag on ODBC to see if it's MySQL or Access
			  //    DebugBreak();
	if( odbc->flags.bAccess )
		return CheckAccessODBCTable( odbc, table, options );
	else
		return CheckMySQLODBCTable( odbc, table, options );

}

#undef CheckODBCTable
LOGICAL CheckODBCTable( PODBC odbc, PTABLE table, uint32_t options )
{
	return CheckODBCTableEx( odbc, table, options DBG_SRC );
}


static void CreateNameTable( PODBC odbc, CTEXTSTR table_name )
{
	TEXTCHAR field1[256];
	TEXTCHAR field2[256];
	TABLE table;
	FIELD fields[2];
#ifdef __cplusplus
#else
	DB_KEY_DEF keys[1];
#endif
	tnprintf( field1, sizeof( field1 ), "%s_id", table_name );
#ifdef __cplusplus
	DB_KEY_DEF keys[1] = { required_key_def( TRUE, FALSE, NULL, field1 ) };
#endif
	table.name = table_name;
	table.fields.count = 2;
	table.fields.field = fields;
	table.keys.count = 1;
	table.keys.key = keys;
	table.type = NULL;
	table.comment = "Auto Created table.";
	fields[0].name = field1;
	fields[0].type = "int";
	fields[0].extra = "auto_increment";
	fields[0].previous_names[0] = NULL;
	tnprintf( field2, sizeof( field2 ), "%s_name", table_name );
	fields[1].name = field2;
	fields[1].type = "varchar(100)";
	fields[1].extra = NULL;
	fields[1].previous_names[0] = NULL;

#ifndef __cplusplus
	keys[0].name = NULL; // primary key needs no name
	keys[0].flags.bPrimary = 1;
	keys[0].flags.bUnique = 0;
	keys[0].colnames[0] = field1;
	keys[0].colnames[1] = NULL;
	keys[0].null = NULL;
#endif
	CheckODBCTable( odbc, &table, CTO_MERGE );
}


INDEX FetchSQLNameID( PODBC odbc, CTEXTSTR table_name, CTEXTSTR name )
{
	{
		CTEXTSTR result;
		int bTried = 0;
	retry:
		if( !SQLQueryf( odbc
						  , &result
						  , "select %s_id from %s where %s_name=\'%s\'"
						  , table_name
						  , table_name
						  , table_name
						  , name ) )
		{
			FetchSQLError( odbc, &result );
			if( ( StrCmpEx( result, "(S0022)", 7 ) == 0 ) ||
				( StrCmpEx( result, "(S0002)", 7 ) == 0 ) )
			{
				if( !bTried )
				{
					bTried = 1;
					CreateNameTable( odbc, table_name );
					goto retry;
				}
			}
		}
		else
		{
			if( !result )
			{
				if( !SQLCommandf( odbc, "insert into %s (%s_name)values(\'%s\')", table_name, table_name, name ) )
				{
					lprintf( "blah!" );
				}
				else
				{
					TEXTCHAR table_name_id[256];
					tnprintf( table_name_id, sizeof( table_name_id ), "%s_id", table_name );
					return FetchLastInsertID( odbc, table_name, table_name_id );
				}
			}
			else
			{
				return (INDEX)IntCreateFromText( result );
			}
		}
	}
	return INVALID_INDEX;
}

CTEXTSTR FetchSQLName( PODBC odbc, CTEXTSTR table_name, INDEX iName )
{
	{
		CTEXTSTR result;
		int bTried = 0;
	retry:
		if( !SQLQueryf( odbc
						  , &result
						  , "select %s_name from %s where %s_id=%lu"
						  , table_name
						  , table_name
						  , table_name
						  , iName ) )
		{
			FetchSQLError( odbc, &result );
			if( ( StrCmpEx( result, "(S0022)", 7 ) == 0 ) ||
				( StrCmpEx( result, "(S0002)", 7 ) == 0 ) )
			{
				if( !bTried )
				{
					bTried = 1;
					CreateNameTable( odbc, table_name );
					goto retry;
				}
			}
		}
		else
		{
			if( !result )
			{
				return NULL;
			}
			else
			{
				return StrDup( result );
			}
		}
	}
	return NULL;

}

INDEX GetSQLNameID( CTEXTSTR table_name, CTEXTSTR name )
{
	if( !g.odbc )
		OpenSQL( DBG_VOIDSRC );
	if( !g.odbc )
		return INVALID_INDEX;
	return FetchSQLNameID( g.odbc, table_name, name );
}
CTEXTSTR GetSQLName( CTEXTSTR table_name, INDEX iName )
{
	if( !g.odbc )
		OpenSQL( DBG_VOIDSRC );
	if( !g.odbc )
		return NULL;
	return FetchSQLName( g.odbc, table_name, iName );
}

LOGICAL BackupDatabase( PODBC source, PODBC dest )
{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( source->flags.bSQLite_native && dest->flags.bSQLite_native ) {
		sqlite3_backup *sb = sqlite3_backup_init( dest->db, "main", source->db, "main" );
		if( sb )
		{
			sqlite3_backup_step( sb, 1 );
			sqlite3_backup_step( sb, sqlite3_backup_remaining( sb ) );
			sqlite3_backup_finish( sb );
			return TRUE;
		}
	}
#endif
  	return FALSE;
}

SQL_NAMESPACE_END

