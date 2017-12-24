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
extern struct pssql_global *global_sqlstub_data;
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


#define og (*sack_global_option_data)
extern OPTION_GLOBAL *sack_global_option_data;

//------------------------------------------------------------------------

#define MKSTR(n,...) #__VA_ARGS__

#include "makeopts.mysql"
;

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
			tnprintf( query, sizeof( query ), WIDE("select %s from %s where %s like '%s'"), col?col:WIDE("id"), table, namecol, tmp );
			Release( tmp );
			if( SQLQueryEx( tree->odbc, query, &result DBG_RELAY) && result )
			{
				IDName = SaveText( result );
				SQLEndQuery( tree->odbc );
			}
			else if( bCreate )
			{
				TEXTSTR newval = EscapeSQLString( tree->odbc, name );
				tnprintf( query, sizeof( query ), WIDE("insert into %s (%s,%s) values( '%s','%s' )")
						  , table, col, namecol, IDName = GetSeqGUID(), newval );
				OpenWriterEx( tree DBG_RELAY );
				if( !SQLCommandEx( tree->odbc_writer, query DBG_RELAY ) )
				{
#ifdef DETAILED_LOGGING
					lprintf( WIDE("insert failed, how can we define name %s?"), name );
#endif
					// inser failed...
				}
				else
				{
					// all is well.
				}
				Release( newval );
			}
			else
				IDName = NULL;

			PopODBCEx(tree->odbc);

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
	lprintf( WIDE("%d %s"), level, nodeval->name );
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
		if( strcmp( namebuf, WIDE( "." ) ) == 0 )
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
			CTEXTSTR IDName = New4ReadOptionNameTable(tree,namebuf,OPTION4_NAME,WIDE( "name_id" ),WIDE( "name" ),1 DBG_RELAY);
			if( !bIKnowItDoesntExist )
			{
				PushSQLQueryExEx(tree->odbc DBG_RELAY );
				tnprintf( query, sizeof( query )
						  , WIDE( "select option_id from " )OPTION4_MAP WIDE( " where parent_option_id='%s' and name_id='%s'" )
						  , parent?parent->guid:GuidZero()
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
					tnprintf( query, sizeof( query ), WIDE( "Insert into " )OPTION4_MAP WIDE( "(`option_id`,`parent_option_id`,`name_id`) values ('%s','%s','%s')" )
							  , ID = GetSeqGUID(), parent->guid, IDName );
					OpenWriter( tree );
					if( SQLCommand( tree->odbc_writer, query ) )
					{
					}
					else
					{
						CTEXTSTR error;
						FetchSQLError( tree->odbc, &error );
#ifdef DETAILED_LOGGING
						lprintf( WIDE("Error inserting option: %s"), error );
#endif
						ID = NULL;
					}
#ifdef DETAILED_LOGGING
					lprintf( WIDE("Created option root...") );
#endif
					//lprintf( WIDE("Adding new option to family tree... ") );
					{
						POPTION_TREE_NODE new_node = GetFromSet( OPTION_TREE_NODE, &tree->nodes );
						//MemSet( new_node, 0, sizeof( struct sack_option_tree_family_node ) );
						new_node->guid = ID;
						new_node->value_guid = NULL; // no value (yet?)
						new_node->name_guid = IDName;
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
				if( global_sqlstub_data->flags.bLogOptionConnection )
					_lprintf(DBG_RELAY)( WIDE("Option node missing; and was not created='%s'"), namebuf );

				if( !bIKnowItDoesntExist )
					PopODBCEx( tree->odbc );
				return NULL;
			}
			else
			{
				POPTION_TREE_NODE new_node = GetFromSet( OPTION_TREE_NODE, &tree->nodes );// New( struct sack_option_tree_family_node );
				//MemSet( new_node, 0, sizeof( struct sack_option_tree_family_node ) );
#ifdef DETAILED_LOGGING
				lprintf( WIDE("found the node which has the name specified...") );
#endif
				new_node->guid = SaveText( result[0] );
				new_node->value_guid = NULL;
				new_node->name_guid = IDName;
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
	CTEXTSTR result = NULL;
	size_t result_len = 0;
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
		StrCpyEx( buf->buffer, optval->value, result_len+1 );
		buf->buffer[result_len-1] = 0;
		(*buffer) = buf->buffer;
		(*len) = result_len-1;
		return result_len-1;
	}

#if 0
	tnprintf( query, sizeof( query ), WIDE( "select override_value_id from " )OPTION4_EXCEPTION WIDE( " " )
            WIDE( "where ( apply_from<=now() or apply_from=0 )" )
            WIDE( "and ( apply_until>now() or apply_until=0 )" )
            WIDE( "and ( system_id=%d or system_id=0 )" )
            WIDE( "and option_id=%d " )
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
	tnprintf( query, sizeof( query ), WIDE( "select string from " )OPTION4_VALUES WIDE( " where option_id='%s' order by segment" ), optval->guid );
	// have to push here, the result of the prior is kept outstanding
	// if this was not pushed, the prior result would evaporate.
	(*buffer) = NULL;
	//lprintf( WIDE("do query for value string...") );
	result_len = (size_t)-1;
	optval->value = NULL;
	optval->value_guid = optval->guid;

	for( SQLQuery( odbc, query, &result ); result; FetchSQLResult( odbc, &result ) )
	{
		if( !pvtResult )
			pvtResult = VarTextCreate();
		vtprintf( pvtResult, WIDE("%s"), result );

		//lprintf( WIDE(" query succeeded....") );
	}
	if( pvtResult )
	{
		PTEXT pResult = VarTextGet( pvtResult );
		result_len = GetTextSize( pResult ) + 1;

		if( result_len > buf->buflen )  expandResultBuffer( buf, result_len * 2 );
		(*buffer) = buf->buffer;
		(*len) = result_len-1;

		StrCpyEx( (*buffer), GetText( pResult ), result_len );
		(*buffer)[result_len-1] = 0;
		//optval->value = StrDup( GetText( pResult ) );
		LineRelease( pResult );
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
	lprintf( WIDE("do query for value string...") );
#endif
	if( SQLRecordQueryf( odbc, NULL, &result, NULL
							  , WIDE( "select `binary`,length(`binary`) from " )OPTION4_BLOBS WIDE( " where option_id='%s'" )
							  , optval->guid ) )
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

LOGICAL New4CreateValue( POPTION_TREE tree, POPTION_TREE_NODE value, CTEXTSTR pValue )
{
	TEXTCHAR insert[256];
	CTEXTSTR result=NULL;
	TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc_writer, pValue, StrLen( pValue ), TRUE );
	LOGICAL retval = TRUE;

	if( pValue == NULL )
	{
		tnprintf( insert, sizeof( insert ), WIDE( "delete from " )OPTION4_VALUES WIDE( " where `option_id`='%s'" )
				  , value->guid
				  );
		value->value = NULL;
	}
	else
	{
		size_t len = StrLen( pValue );
		size_t offset = 0;
		int segment = 0;
		while( len > 95)
		{
			newval = EscapeSQLBinaryOpt( tree->odbc_writer, pValue + offset, 95, TRUE );
			tnprintf( insert, sizeof( insert ), WIDE( "replace into " )OPTION4_VALUES WIDE( " (`option_id`,`string`,`segment` ) values ('%s',%s,%d)" )
					  , value->guid
					  , newval
					  , segment
					  );
			if( SQLCommand( tree->odbc_writer, insert ) )
			{
				value->value_guid = value->guid;
			}
			else
			{
				FetchSQLError( tree->odbc_writer, &result );
				lprintf( WIDE("Insert value failed: %s"), result );
				retval = FALSE;
			}
			offset += 95;
			len -= 95;
			segment++;
		}
		newval = EscapeSQLBinaryOpt( tree->odbc_writer, pValue + offset, len, TRUE );
		tnprintf( insert, sizeof( insert ), WIDE( "replace into " )OPTION4_VALUES WIDE( " (`option_id`,`string`,`segment` ) values ('%s',%*.*s,%d)" )
				  , value->guid
				  , len, len
				  , newval
				  , segment
				  );
		if( SQLCommand( tree->odbc_writer, insert ) )
		{
		}
		tnprintf( insert, sizeof( insert ), WIDE( "delete from " )OPTION4_VALUES WIDE( " where `option_id`='%s' and segment > %d" )
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
		value->value_guid = value->guid;
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


int ResolveOptionName( POPTION_TREE options, CTEXTSTR parent_id, CTEXTSTR option_id, CTEXTSTR name_id, CTEXTSTR option_name, TEXTSTR output_buffer, size_t output_buffer_size )
{
	CTEXTSTR *results;
	if( StrCaseCmp( parent_id, GuidZero() ) == 0 )
	{
		return tnprintf( output_buffer, output_buffer_size, WIDE("%s"), option_name );
	}
	PushSQLQueryEx( options->odbc ); 
	for( SQLRecordQueryf( options->odbc, NULL, &results, NULL
						, WIDE("select parent_option_id,option_id,name_id,name from ") OPTION4_MAP WIDE( " join ") OPTION4_NAME WIDE(" using(name_id) where option_id='%s'" )
						, parent_id );
		results;
		FetchSQLRecord( options->odbc, &results ) )
	{
		int offset;
		offset = ResolveOptionName( options, results[0], results[1], results[2], results[3]
					, output_buffer, output_buffer_size );
		PopODBCEx( options->odbc ); 
		return offset + tnprintf( output_buffer + offset, output_buffer_size - offset, WIDE("/%s"), option_name );
	}
	return 0;
}

void New4FindOptions( POPTION_TREE options, PLIST *result_list, CTEXTSTR name )
{
	CTEXTSTR *results;
	for( SQLRecordQueryf( options->odbc, NULL, &results, NULL
						, WIDE("select parent_option_id,option_id,name_id,name from ") OPTION4_MAP WIDE( " join ") OPTION4_NAME WIDE(" using(name_id) where name like '%s'" )
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

