#define OPTION_MAIN_SOURCE
#ifndef GETOPTION_SOURCE
#define GETOPTION_SOURCE
#endif
#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sharemem.h>
#include <filesys.h>
#include <system.h>
#include <network.h>
#ifdef __WATCOMC__
#include <io.h> // unlink
#endif

#include <pssql.h>
#include <sqlgetoption.h>

#include "../sqlstruc.h"
// define this to show very verbose logging during creation and
// referencing of option tree...
//#define DETAILED_LOGGING

/*
 Dump Option table...
 SELECT oname2.name,oname.name,optionvalues.string,omap.*
 FROM `optionmap` as omap
 join optionname as oname on omap.name_id=oname.name_id
 left join optionvalues on omap.value_id=optionvalues.value_id
 left join optionmap as omap2 on omap2.node_id=omap.parent_node_id
 left join optionname as oname2 on omap2.name_id=oname2.name_id
*/

SACK_OPTION_NAMESPACE

#include "optlib.h"

typedef struct sack_option_global_tag OPTION_GLOBAL;


#define og (*sack_global_option_data)
extern OPTION_GLOBAL *sack_global_option_data;

//------------------------------------------------------------------------

#define MKSTR(n,...) #__VA_ARGS__

#include "makeopts.mysql"
;

//------------------------------------------------------------------------

//------------------------------------------------------------------------

//#define OPTION_ROOT_VALUE INVALID_INDEX
#define OPTION_ROOT_VALUE 0

POPTION_TREE_NODE NewGetOptionIndexExxx( PODBC odbc, POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate, int bIKnowItDoesntExist DBG_PASS )
//#define GetOptionIndex( f,b,v ) GetOptionIndexEx( OPTION_ROOT_VALUE, f, b, v, FALSE )
{
	const TEXTCHAR **start = NULL;
	TEXTCHAR namebuf[256];
	TEXTCHAR query[256];
	const TEXTCHAR *p;
	static const TEXTCHAR *_program = NULL;
	const TEXTCHAR *program = NULL;
	static const TEXTCHAR *_system = NULL;
	const TEXTCHAR *system = NULL;
	CTEXTSTR *result = NULL;
	INDEX ID;
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	//, IDName; // Name to lookup
	if( og.flags.bUseProgramDefault )
	{
		if( !_program )
			_program = GetProgramName();
		program = _program;
	}
	if( og.flags.bUseSystemDefault )
	{
		if( !_system )
			_system = GetSystemName();
		system = _system;
	}
	// resets the search/browse cursor... not empty...
	FamilyTreeReset( &tree->option_tree );
	while( system || program || file || pBranch || pValue || start )
	{
#ifdef DETAILED_LOGGING
		lprintf( WIDE("Top of option loop") );
#endif
		if( !start || !(*start) )
		{
			if( program )
				start = &program;
			if( !start && system )
				start = &system;

			if( !start && file )
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE("Token parsing at FILE") );
#endif
				start = &file;
			}
			if( !start && pBranch )
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE("Token parsing at branch") );
#endif
				start = &pBranch;
			}
			if( !start && pValue )
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE("Token parsing at value") );
#endif
	            start = &pValue;
			}
			if( !start || !(*start) ) continue;
		}
		p = pathchr( *start );
		if( p )
		{
			if( p-(*start) > 0 )
			{
				MemCpy( namebuf, (*start), (p - (*start)) * sizeof(TEXTCHAR) );
				namebuf[p-(*start)] = 0;
			}
			else
			{
				(*start) = p + 1;
				continue;
			}
			(*start) = p + 1;
		}
		else
		{
			StrCpyEx( namebuf, (*start), sizeof( namebuf )-1 );
			(*start) = NULL;
			start = NULL;
		}
			
		// remove references of 'here' during parsing.
		if( strcmp( namebuf, WIDE( "." ) ) == 0 )
			continue;

		{
			// double convert 'precistion loss 64bit gcc'
			POPTION_TREE_NODE node = (POPTION_TREE_NODE)FamilyTreeFindChild( tree->option_tree, (PTRSZVAL)namebuf );
			if( node )
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE("Which is found, and new parent ID result...%d"), node_id );
#endif
				parent = node;
				continue;
			}
		}

		{
			INDEX IDName = ReadOptionNameTable(tree,namebuf,OPTION_NAME,WIDE( "name_id" ),WIDE( "name" ),1 DBG_RELAY);

			if( !bIKnowItDoesntExist )
			{
				PushSQLQueryExEx(tree->odbc DBG_RELAY );
				snprintf( query, sizeof( query )
						  , WIDE( "select option_id from " )OPTION_MAP WIDE( " where parent_option_id=%ld and name_id=%d" )
						  , parent->id
						  , IDName );
			}
			//lprintf( WIDE( "doing %s" ), query );
			if( bIKnowItDoesntExist || !SQLRecordQueryEx( tree->odbc, query, NULL, &result, NULL DBG_RELAY ) || !result )
			{
				if( bCreate )
				{
					// this is the only place where ID must be set explicit...
						// otherwise our root node creation failes if said root is gone.
					//lprintf( WIDE( "New entry... create it..." ) );
						snprintf( query, sizeof( query ), WIDE( "Insert into " )OPTION_MAP WIDE( "(`parent_option_id`,`name_id`) values (%ld,%lu)" ), parent->id, IDName );
					OpenWriter( tree );
					if( SQLCommand( tree->odbc_writer, query ) )
					{
						ID = FetchLastInsertID( tree->odbc_writer, OPTION_MAP, WIDE("option_id") );
					}
					else
					{
						CTEXTSTR error;
						FetchSQLError( tree->odbc, &error );
#ifdef DETAILED_LOGGING
						lprintf( WIDE("Error inserting option: %s"), error );
#endif
					}
#ifdef DETAILED_LOGGING
					lprintf( WIDE("Created option root...") );
#endif
					//lprintf( WIDE("Adding new option to family tree... ") );
					{
						POPTION_TREE_NODE new_node = New( struct sack_option_tree_family_node );
						new_node->id = ID;
						new_node->value_id = INVALID_INDEX; // no value (yet?)
						new_node->name_id = IDName;
						new_node->value = NULL;
						FamilyTreeAddChild( &tree->option_tree, new_node, (PTRSZVAL)SaveText( namebuf ) );
						parent = new_node;
					}
					if( !bIKnowItDoesntExist )
						PopODBCEx( tree->odbc );
					continue; // get out of this loop, continue outer.
				}
#ifdef DETAILED_LOGGING
				_lprintf(DBG_RELAY)( WIDE("Option tree corrupt.  No option option_id=%ld"), ID );
#endif
				if( !bIKnowItDoesntExist )
					PopODBCEx( tree->odbc );
				return NULL;
			}
			else
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE("found the node which has the name specified...") );
#endif
				POPTION_TREE_NODE new_node = New( struct sack_option_tree_family_node );
				new_node->id = IndexCreateFromText( result[0] );
				new_node->value_id = INVALID_INDEX;
				new_node->name_id = IDName;
				new_node->value = NULL;
				FamilyTreeAddChild( &tree->option_tree, new_node, (PTRSZVAL)SaveText( namebuf ) );
				parent = new_node;
			}
			if( !bIKnowItDoesntExist )
				PopODBCEx( tree->odbc );
		}
	}
	return parent;
}

//------------------------------------------------------------------------

size_t NewGetOptionStringValue( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len DBG_PASS )
{
	TEXTCHAR query[256];
	CTEXTSTR result = NULL;
	size_t result_len = 0;

	len--;

	if( optval->value_id != INVALID_INDEX )
	{
		result_len = StrLen( optval->value );
		StrCpyEx( buffer, optval->value, min(len+1,result_len+1) );
		buffer[result_len = min(len,result_len)] = 0;
		return result_len;
	}

#if 0
	snprintf( query, sizeof( query ), WIDE( "select override_value_id from " )OPTION_EXCEPTION WIDE( " " )
            WIDE( "where ( apply_from<=now() or apply_from=0 )" )
            WIDE( "and ( apply_until>now() or apply_until=0 )" )
            WIDE( "and ( system_id=%d or system_id=0 )" )
            WIDE( "and option_id=%d " )
           , og.SystemID
           , optval->id );
	PushSQLQueryEx( odbc );
	for( SQLQuery( odbc, query, &result ); result; FetchSQLResult( odbc, &result ) )
	{
		_optval = optval;
		tmp_value_id = atol( result );
		if( (!optval->id) || ( optval->id == INVALID_INDEX ) )
			optval = _optval;
		SQLEndQuery( odbc );
	}
#endif

	PushSQLQueryEx( odbc );
	snprintf( query, sizeof( query ), WIDE( "select string from " )OPTION_VALUES WIDE( " where option_id=%ld" ), optval->id );
	// have to push here, the result of the prior is kept outstanding
	// if this was not pushed, the prior result would evaporate.
	buffer[0] = 0;
	//lprintf( WIDE("do query for value string...") );
	if( SQLQuery( odbc, query, &result ) )
	{
		//lprintf( WIDE(" query succeeded....") );
		if( result )
		{
			optval->value_id = optval->id;
			result_len = StrLen( result );
			StrCpyEx( buffer, result, min(len+1,result_len+1) );
			buffer[result_len = min(len,result_len)] = 0;
			optval->value = StrDup( buffer );
		}
		else
		{
			buffer[0] = 0;
			result_len = (size_t)-1;
			optval->value = NULL;
		}
		SQLEndQuery( odbc );
	}
	PopODBCEx( odbc );
	return result_len;
}


int NewGetOptionBlobValueOdbc( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len )
{
	CTEXTSTR *result = NULL;
	size_t tmplen;
	if( !len )
      len = &tmplen;
	PushSQLQueryEx( odbc );
#ifdef DETAILED_LOGGING
	lprintf( WIDE("do query for value string...") );
#endif
	if( SQLRecordQueryf( odbc, NULL, &result, NULL
							  , WIDE( "select `binary`,length(`binary`) from " )OPTION_BLOBS WIDE( " where option_id=%ld" )
							  , optval->id ) )
	{
      int success = FALSE;
		//lprintf( WIDE(" query succeeded....") );
		if( buffer && result && result[0] && result[1] )
		{
         success = TRUE;
			(*buffer) = NewArray( TEXTCHAR, (*len)=(size_t)IntCreateFromText( result[1] ));
			MemCpy( (*buffer), result[0], (*len) );
		}
		PopODBCEx( odbc );
		return success;
	}
	PopODBCEx( odbc );
	return FALSE;
}


//------------------------------------------------------------------------

LOGICAL NewCreateValue( POPTION_TREE tree, POPTION_TREE_NODE value, CTEXTSTR pValue )
{
	TEXTCHAR insert[256];
	CTEXTSTR result=NULL;
	TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc_writer, pValue, StrLen( pValue ), TRUE );
	LOGICAL retval = TRUE;
	if( pValue == NULL )
		snprintf( insert, sizeof( insert ), WIDE( "insert into " )OPTION_BLOBS WIDE( " (`option_id`,`binary` ) values (%lu,'')" )
				  , value->id
				  );
	else
		snprintf( insert, sizeof( insert ), WIDE( "insert into " )OPTION_VALUES WIDE( " (`option_id`,`string` ) values (%lu,%s)" )
				  , value->id
				  , pValue?newval:WIDE( "NULL" )
				  );

	// save the value that we last wrote; then we can get it without worrying about the commit state
	value->value = StrDup( pValue );
	OpenWriter( tree );
	if( SQLCommand( tree->odbc_writer, insert ) )
	{
		value->value_id = value->id;
	}
	else
	{
		FetchSQLError( tree->odbc_writer, &result );
		lprintf( WIDE("Insert value failed: %s"), result );
		retval = FALSE;
	}
	Release( newval );
	return retval;
}


SACK_OPTION_NAMESPACE_END

