#ifndef GETOPTION_SOURCE
#define GETOPTION_SOURCE
#endif
// we want access to GLOBAL from sqltub
#define SQLLIB_SOURCE
#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sharemem.h>
#include <filesys.h>
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

SQL_NAMESPACE
#ifdef __STATIC_GLOBALS__
#  define sg (global_sqlstub_data)
	extern struct pssql_global global_sqlstub_data;
#else
#  define sg (*global_sqlstub_data)
	extern struct pssql_global *global_sqlstub_data;
#endif
SQL_NAMESPACE_END
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


#ifdef __STATIC_GLOBALS__
#  define og (sack_global_option_data)
	extern OPTION_GLOBAL sack_global_option_data;
#else
#  define og (*sack_global_option_data)
	extern OPTION_GLOBAL *sack_global_option_data;
#endif


//------------------------------------------------------------------------

#define MKSTR(n,...) #__VA_ARGS__

#include "makeopts.mysql"
;

//------------------------------------------------------------------------

void FixCorruption( PODBC odbc ) {

}

//------------------------------------------------------------------------

CTEXTSTR New4ReadOptionNameTable( POPTION_TREE tree, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS )
{
			TEXTCHAR query[256];
			TEXTCHAR *tmp;
			CTEXTSTR result = NULL;
			CTEXTSTR IDName = NULL;
			if( !table || !name )
				return NULL;

			// look in internal cache first...
			IDName = GetKeyOfName(tree->odbc,table,name);
			if( IDName )
				return IDName;

			if( !tree->odbc )
				DebugBreak();

			PushSQLQueryEx( tree->odbc );
			tmp = EscapeSQLStringEx( tree->odbc, name DBG_RELAY );
			tnprintf( query, sizeof( query ), "select %s from %s where %s like '%s'", col?col:"id", table, namecol, tmp );
			Release( tmp );
			if( SQLQueryEx( tree->odbc, query, &result DBG_RELAY) && result )
			{
				IDName = SaveText( result );
				SQLEndQuery( tree->odbc ); // allow repurposing...
				PopODBCEx(tree->odbc);
			}
			else if( bCreate )
			{
				TEXTSTR newval;
				SQLEndQuery( tree->odbc );
				PopODBCEx(tree->odbc);
				OpenWriterEx( tree DBG_RELAY );
				newval = EscapeSQLString( tree->odbc_writer, name );
				tnprintf( query, sizeof( query ), "insert into %s (%s,%s) values( '%s','%s' )"
						  , table, col, namecol, IDName = GetSeqGUID(), newval );
				OpenWriterEx( tree DBG_RELAY );
				if( !SQLCommandEx( tree->odbc_writer, query DBG_RELAY ) )
				{
					CTEXTSTR error;
					FetchSQLError( tree->odbc_writer, &error );
					lprintf( "Error inserting option name: %s %s", "", error );
#ifdef DETAILED_LOGGING
					lprintf( "insert failed, how can we define name %s?", name );
#endif
					// inser failed...
				}
				else
				{
					// all is well.
				}
				Release( newval );
			}
			else {
				IDName = NULL;
				PopODBCEx(tree->odbc);
			}

			if( IDName )
			{
				// instead of strdup, consider here using SaveName from procreg?
				AddBinaryNode( GetTableCache(tree->odbc,table), (POINTER)IDName
								 , (uintptr_t)SaveText( name ) );
			}
			return IDName;
}


//------------------------------------------------------------------------

LOGICAL CPROC LogProcessNode( uintptr_t psvForeach, uintptr_t psvNodeData, int level )
{
	POPTION_TREE_NODE nodeval = (POPTION_TREE_NODE)psvNodeData;
	lprintf( "%d %s", level, nodeval->name );
	return TRUE;
}

void DumpFamilyTree( PFAMILYTREE tree )
{
	FamilyTreeForEach( tree, NULL, LogProcessNode, 0 );
}

//#define OPTION_ROOT_VALUE INVALID_INDEX
#define OPTION_ROOT_VALUE 0

POPTION_TREE_NODE New4GetOptionIndexExxx( PODBC odbc, POPTION_TREE tree, POPTION_TREE_NODE parent, const TEXTCHAR *system, const TEXTCHAR *program, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate, int bBypassParsing, int bIKnowItDoesntExist DBG_PASS )
//#define GetOptionIndex( f,b,v ) GetOptionIndexEx( OPTION_ROOT_VALUE, f, b, v, FALSE )
{
	POPTION_TREE_NODE node; // temp
	const TEXTCHAR **start = NULL;
	TEXTCHAR namebuf[256];
	TEXTCHAR query[256];
	const TEXTCHAR *p;
	CTEXTSTR *result = NULL;
	CTEXTSTR ID = NULL;

	// resets the search/browse cursor... not empty...
	FamilyTreeReset( &tree->option_tree );
	while( system || program || file || pBranch || pValue || start )
	{
#ifdef DETAILED_LOGGING
		lprintf( "Top of option loop" );
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
				lprintf( "Token parsing at FILE" );
#endif
				start = &file;
			}
			if( !start && pBranch )
			{
#ifdef DETAILED_LOGGING
				lprintf( "Token parsing at branch" );
#endif
				start = &pBranch;
			}
			if( !start && pValue )
			{
#ifdef DETAILED_LOGGING
				lprintf( "Token parsing at value" );
#endif
				start = &pValue;
			}
			if( !start || !(*start) ) continue;
		}
		p = bBypassParsing?NULL:pathchr( *start );
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
		if( strcmp( namebuf, "." ) == 0 )
			continue;
		// trim trailing spaces from option names.
		{
			int n = (int)(StrLen( namebuf ) - 1);
			while( n >= 0 && namebuf[n] == ' ' )
			{
				namebuf[n] = 0;
				n--;
			}
		}
#ifdef DETAILED_LOGGING
		lprintf( "Find [%s]", namebuf );
#endif
		//DumpFamilyTree( tree->option_tree );
		node = (POPTION_TREE_NODE)FamilyTreeFindChildEx( tree->option_tree, parent?parent->node:NULL, (uintptr_t)namebuf );
		if( node ) {
			parent = node;
			continue;
		}
		// else parent is ; and new node needs to be...
		{
			CTEXTSTR IDName = New4ReadOptionNameTable(tree,namebuf,OPTION4_NAME,"name_id","name",1 DBG_RELAY);
			if( !bIKnowItDoesntExist )
			{
				PushSQLQueryExEx(tree->odbc DBG_RELAY );
				tnprintf( query, sizeof( query )
						  , "select option_id from " OPTION4_MAP " where parent_option_id='%s' and name_id='%s'"
						  , parent?parent->guid:GuidZero()
						  , IDName );
			}
			//lprintf( "doing %s", query );
			if( bIKnowItDoesntExist || !SQLRecordQueryEx( tree->odbc, query, NULL, &result, NULL DBG_RELAY ) || !result )
			{
				if( bCreate )
				{
					// this is the only place where ID must be set explicit...
						// otherwise our root node creation failes if said root is gone.
					//lprintf( "New entry... create it..." );
					tnprintf( query, sizeof( query ), "Insert into " OPTION4_MAP "(`option_id`,`parent_option_id`,`name_id`) values ('%s','%s','%s')"
							  , ID = GetSeqGUID(), parent->guid, IDName ); //-V595
					OpenWriter( tree );
					if( SQLCommand( tree->odbc_writer, query ) )
					{
					}
					else
					{
						CTEXTSTR error;
						FetchSQLError( tree->odbc_writer, &error );
						lprintf( "Error inserting option: %s", error );
						ID = NULL;
					}
#ifdef DETAILED_LOGGING
					lprintf( "Created option root..." );
#endif
					//lprintf( "Adding new option to family tree... " );
					{
						POPTION_TREE_NODE new_node = GetFromSet( OPTION_TREE_NODE, &tree->nodes );
						//MemSet( new_node, 0, sizeof( struct sack_option_tree_family_node ) );
						new_node->guid = ID;
						new_node->flags.bHasValue = 0; // no value (yet?)
						new_node->name = SaveText( namebuf );
						new_node->value = NULL;
						new_node->node = FamilyTreeAddChild( &tree->option_tree, parent?parent->node:NULL, new_node, (uintptr_t)new_node->name );
						//lprintf( "New parent has been created in the tree... %p %s", new_node, new_node->guid );
						parent = new_node;
					}
					if( !bIKnowItDoesntExist )
						PopODBCEx( tree->odbc );
					continue; // get out of this loop, continue outer.
				}
				if( sg.flags.bLogOptionConnection )
					_lprintf(DBG_RELAY)( "Option node missing; and was not created='%s'", namebuf );

				if( !bIKnowItDoesntExist )
					PopODBCEx( tree->odbc );
				return NULL;
			}
			else
			{
				POPTION_TREE_NODE new_node = GetFromSet( OPTION_TREE_NODE, &tree->nodes );// New( struct sack_option_tree_family_node );
				//MemSet( new_node, 0, sizeof( struct sack_option_tree_family_node ) );
#ifdef DETAILED_LOGGING
				lprintf( "found the node which has the name specified..." );
#endif
				new_node->guid = SaveText( result[0] );
				new_node->flags.bHasValue = 0;
				new_node->name = SaveText( namebuf );
				new_node->value = NULL;
				new_node->node = FamilyTreeAddChild( &tree->option_tree, parent?parent->node:NULL, new_node, (uintptr_t)new_node->name );

				//lprintf( "New parent has been created in the tree...2 %p %s", new_node, new_node->guid );
				parent = new_node;
			}
			if( !bIKnowItDoesntExist )
				PopODBCEx( tree->odbc );
		}
	}
	return parent;
}

//------------------------------------------------------------------------

struct resultBuffer {
	TEXTCHAR *buffer;
	size_t buflen;
};

static void expandResultBuffer( struct resultBuffer *buf, size_t x ) {
	TEXTCHAR *newbuf = NewArray( TEXTCHAR, buf->buflen+x );
	MemCpy( newbuf, buf->buffer, buf->buflen );
	buf->buflen += x;
	if( buf->buffer ) Release( buf->buffer );
	buf->buffer = newbuf;
}

static struct resultBuffer plqBuffers[16];
static int nBuffer;

size_t New4GetOptionStringValue( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len DBG_PASS )
{
	TEXTCHAR query[256];
	CTEXTSTR *result = NULL;
	size_t result_len = 0;
	size_t query_len;
	size_t *result_lengths;
	struct resultBuffer *buf;
	PVARTEXT pvtResult = NULL;
	buf = &plqBuffers[nBuffer++];
	if( nBuffer >= 16 ) nBuffer = 0;
	
	if( optval->uncommited_write )
	{
		if( !optval->value ) {
			(*buffer) = NULL;
			return 0;
		}
		result_len = StrLen( optval->value ) + 1;
		if(result_len > buf->buflen)  expandResultBuffer( buf, result_len * 2 );
		StrCpyEx( buf->buffer, optval->value, result_len );
		//buf->buffer[result_len-1] = 0;
		(*buffer) = buf->buffer;
		(*len) = result_len-1;
		return result_len-1;
	}

#if 0
	tnprintf( query, sizeof( query ), "select override_value_id from " OPTION4_EXCEPTION " "
            "where ( apply_from<=now() or apply_from=0 )"
            "and ( apply_until>now() or apply_until=0 )"
            "and ( system_id=%d or system_id=0 )"
            "and option_id=%d "
           , og.SystemID
           , optval->guid );
	PushSQLQueryEx( odbc );
	for( SQLQuery( odbc, query, &result ); result; FetchSQLResult( odbc, &result ) )
	{
		_optval = optval;
		tmp_value_id = atol( result );
		if( (!optval->guid) )
			optval = _optval;
		SQLEndQuery( odbc );
	}
#endif

	PushSQLQueryEx( odbc );
	query_len = tnprintf( query, sizeof( query ), "select string from " OPTION4_VALUES " where option_id='%s' order by segment", optval->guid );
	// have to push here, the result of the prior is kept outstanding
	// if this was not pushed, the prior result would evaporate.
	(*buffer) = NULL;
	//lprintf( "do query for value string..." );
	result_len = (size_t)-1;
	if( optval->value )
		Release( (POINTER)optval->value );
	optval->value = NULL;
	optval->flags.bHasValue = 1;

	for( SQLRecordQueryLen( odbc, query, query_len, NULL, &result, &result_lengths, NULL ); result; FetchSQLRecord( odbc, &result ) )
	{
		if( !pvtResult )
			pvtResult = VarTextCreate();
		VarTextAddData( pvtResult, result[0], result_lengths[0] );
	}
	if( pvtResult )
	{
		PTEXT pResult = VarTextPeek( pvtResult );
		if( pResult ) {
			result_len = GetTextSize( pResult ) + 1;

			if( result_len > buf->buflen )  expandResultBuffer( buf, result_len );
			( *buffer ) = buf->buffer;
			( *len ) = result_len - 1;

			memcpy( ( *buffer ), GetText( pResult ), result_len );
			( *buffer )[result_len - 1] = 0;

			optval->value = DupCStrLen( *buffer, result_len - 1 );
		}
		//optval->value = StrDup( GetText( pResult ) );
		VarTextDestroy( &pvtResult );
	}
	PopODBCEx( odbc );
	return result_len-1;
}


int New4GetOptionBlobValueOdbc( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len )
{
	CTEXTSTR *result = NULL;
	size_t tmplen;
	if( !len )
		len = &tmplen;
	PushSQLQueryEx( odbc );
#ifdef DETAILED_LOGGING
	lprintf( "do query for value string..." );
#endif
	if( SQLRecordQueryf( odbc, NULL, &result, NULL
							  , "select `binary`,length(`binary`) from " OPTION4_BLOBS " where option_id='%s'"
							  , optval->guid ) )
	{
		int success = FALSE;
		//lprintf( " query succeeded...." );
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

LOGICAL New4CreateValue( POPTION_TREE tree, POPTION_TREE_NODE value, CTEXTSTR pValue )
{
	TEXTCHAR insert[256];
	CTEXTSTR result=NULL;
	TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc_writer, pValue, StrLen( pValue ), TRUE );
	LOGICAL retval = TRUE;
	size_t tmpOfs;
	if( value->value )
		Release( (POINTER)value->value );
	value->value = NULL;
	if( pValue == NULL )
	{
		tnprintf( insert, sizeof( insert ), "delete from " OPTION4_VALUES " where `option_id`='%s'"
				  , value->guid
				  );
	}
	else
	{
		size_t len = StrLen( pValue );
		size_t offset = 0;
		int segment = 0;
		size_t valLen;
		while( len > 95)
		{
			newval = EscapeSQLBinaryExx( tree->odbc_writer, pValue + offset, 95, &valLen, TRUE DBG_SRC );
			tmpOfs = tnprintf( insert, sizeof( insert ), "replace into " OPTION4_VALUES " (`option_id`,`string`,`segment` ) values ('%s',"
				, value->guid
			);
			memcpy( insert + tmpOfs, newval, valLen );
			tmpOfs += valLen;
			tmpOfs += tnprintf( insert + tmpOfs, sizeof( insert )-tmpOfs, ", %d)"
				, segment
			);

			if( SQLCommandExx( tree->odbc_writer, insert, tmpOfs DBG_SRC ) )
			{
				value->flags.bHasValue = 1;
			}
			else
			{
				FetchSQLError( tree->odbc_writer, &result );
				lprintf( "Insert value failed: %s", result );
				retval = FALSE;
			}
			offset += 95;
			len -= 95;
			segment++;
		}
		newval = EscapeSQLBinaryExx( tree->odbc_writer, pValue + offset, len, &valLen, TRUE DBG_SRC );
		tmpOfs = tnprintf( insert, sizeof( insert ), "replace into " OPTION4_VALUES " (`option_id`,`string`,`segment` ) values ('%s',"
				  , value->guid
				  );
		memcpy( insert + tmpOfs, newval, valLen );
		tmpOfs += valLen;
		tmpOfs += tnprintf( insert + tmpOfs, sizeof( insert ) - tmpOfs, ", %d)"
			, segment
		);
		if( SQLCommandExx( tree->odbc_writer, insert, tmpOfs DBG_SRC ) )
		{
		}
		tnprintf( insert, sizeof( insert ), "delete from " OPTION4_VALUES " where `option_id`='%s' and segment > %d"
				  , value->guid
				  , segment
				  );
	}
	// save the value that we last wrote; then we can get it without worrying about the commit state
	value->value = StrDup( pValue );
	value->uncommited_write = tree->odbc_writer;
	AddLink( &tree->uncommited, value );
	if( SQLCommand( tree->odbc_writer, insert ) )
	{
		value->flags.bHasValue = 1;
	}
	else
	{
		FetchSQLError( tree->odbc_writer, &result );
		lprintf( "Insert value failed: %s", result );
		retval = FALSE;
	}
	Release( newval );
	return retval;
}


int ResolveOptionName( POPTION_TREE options, CTEXTSTR parent_id, CTEXTSTR option_id, CTEXTSTR name_id, CTEXTSTR option_name, TEXTSTR output_buffer, size_t output_buffer_size )
{
	CTEXTSTR *results;
	if( StrCaseCmp( parent_id, GuidZero() ) == 0 )
	{
		return tnprintf( output_buffer, output_buffer_size, "%s", option_name );
	}
	PushSQLQueryEx( options->odbc ); 
	for( SQLRecordQueryf( options->odbc, NULL, &results, NULL
						, "select parent_option_id,option_id,name_id,name from " OPTION4_MAP " join " OPTION4_NAME " using(name_id) where option_id='%s'"
						, parent_id );
		results;
		FetchSQLRecord( options->odbc, &results ) )
	{
		int offset;
		offset = ResolveOptionName( options, results[0], results[1], results[2], results[3]
					, output_buffer, output_buffer_size );
		PopODBCEx( options->odbc ); 
		return offset + tnprintf( output_buffer + offset, output_buffer_size - offset, "/%s", option_name );
	}
	return 0;
}

void New4FindOptions( POPTION_TREE options, PLIST *result_list, CTEXTSTR name )
{
	CTEXTSTR *results;
	for( SQLRecordQueryf( options->odbc, NULL, &results, NULL
						, "select parent_option_id,option_id,name_id,name from " OPTION4_MAP " join " OPTION4_NAME " using(name_id) where name like '%s'"
						, name );
		results;
		FetchSQLRecord( options->odbc, &results ) )
	{
		TEXTCHAR option_name[256];
		ResolveOptionName( options, results[0], results[1], results[2], results[3], option_name, 256 );
		AddLink( result_list, StrDup( option_name ) );
	}
}

SACK_OPTION_NAMESPACE_END

