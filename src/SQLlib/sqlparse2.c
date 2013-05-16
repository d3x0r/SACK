#if 0

#include <sack_types.h>
#include <sharemem.h>
#include <pssql.h>

PTEXT GetPhrase( PTEXT *token )
{
	PTEXT result = NULL;
   PTEXT next;
   PTEXT tmp = (*token);
	if( GetText(tmp)[0] == '`' )
	{
		tmp = NEXTLINE( tmp );
		while( tmp && GetText(tmp)[0] != '`' )
		{
         next = NEXTLINE( tmp );
			SegGrab( tmp );
			result = SegAppend( result, tmp );
         tmp = next;
		}
	}
	else
		result = SegGrab( tmp );

   // set temp to first word after phrase end.
	tmp = NEXTLINE( tmp );
   (*token) = tmp;
   return result;
}

PFIELD GetColumn( PTEXT *token )
{
   // this should build a whole column structure
   PTEXT word = (*token);
   PTEXT next = NEXTLINE( word );
	PTEXT result = SegGrab( word );
	if( GetText( word )[0] == '(' )
	{
		do
		{
			next = NEXTLINE( word );
			result = SegAppend( result, SegGrab( word ) );
			word = next;
		} while(  0 );
	}
	(*token) = next;
   return field;
}

PTABLE BuildTableDefinitions( PTABLE table, PTEXT *token )
{
	// this has the option of destroying the table and returning a NULL

  // col_name type [NOT NULL | NULL] [DEFAULT default_value] [AUTO_INCREMENT]
  //          [[PRIMARY] KEY] [COMMENT 'string'] [reference_definition]
  //| PRIMARY KEY (index_col_name,...)
  //| KEY [index_name] (index_col_name,...)
  //| INDEX [index_name] (index_col_name,...)
  //| UNIQUE [INDEX] [index_name] (index_col_name,...)
  //| FULLTEXT [INDEX] [index_name] (index_col_name,...)
  //| [CONSTRAINT symbol] FOREIGN KEY [index_name] (index_col_name,...)
  //          [reference_definition]
  //| CHECK (expr)
	if( TextLike( (*token), "PRIMARY" ) )
	{
		if( TextLike( NEXTLINE(*token), "KEY" ) )
		{
			token = NEXTLINE( NEXTLINE( *token ) );

		}
	}
	else if( TextLike( (*token), "KEY" ) )
	{
		token = NEXTLINE( *token );

	}
	else if( TextLike( (*token), "INDEX" ) )
	{
		token = NEXTLINE( *token );
	}
	else if( TextLike( (*token), "UNIQUE" ) )
	{
		token = NEXTLINE( *token );
		if( TextLike( (*token), "INDEX" ) )
         token = NEXTLINE( *token );
	}
	else if( TextLike( (*token), "FULLTEXT" ) )
	{
		token = NEXTLINE( *token );
		if( TextLike( (*token), "INDEX" ) )
         token = NEXTLINE( *token );
	}
	else if( TextLike( (*token), "CONSTRAINT" ) )
	{
      lprintf( "No support for foreign key specifications / constraint specifications" );
		token = NEXTLINE( *token );
		if( TextLike( (*token), "INDEX" ) )
         token = NEXTLINE( *token );
	}
	else
	{
		PTEXT colname = GetPhrase( token );
		if( colname )
		{
         PFIELD field = GetColumn( token );
         AddLink( &columsn,  );
		}

	}


   return table;
}

PTABLE GetFieldsInSQL2Ex( char *create_string, int writestate DBG_PASS )
{
   PTEXT tmp = SegCreateFromText( create_string );
	PTEXT cmd = burst( tmp );
	PTEXT token;
   PTEXT phrase;
	PTABLE table = Allocate( sizeof( TABLE ) );
   MemSet( table, 0, sizeof( TABLE ) );
	LineRelease( tmp );
   token = cmd;
	if( !TextLike( token, "CREATE" ) )
	{
      lprintf( "Failed to match 'CREATE' for 'create table...'" );
      LineRelease( cmd );
      return NULL;
	}
	token = NEXTLINE( token );
	if( TextLike( token, "TEMPORARY" ) )
	{
		table->flags.bTemporary = 1;
      token = NEXTLINE( token );
	}
	if( !TextLike( token, "TABLE" ) )
	{
      lprintf( "Failed to match 'CREATE' for 'create table...'" );
      LineRelease( cmd );
      return NULL;
	}
	token = NEXTLINE( token );
	if( TextLike( token, "IF" ) )
	{
		token = NEXTLINE( token );
		if( TextLike( token, "NOT" ) )
		{
			token = NEXTLINE( token );
			if( TextLike( token, "EXISTS" ) )
			{
				table->flags.bIfNotExist = 1;
            token = NEXTLINE( token );
			}
			else
			{
				DestroySQLTable( table );
				lprintf( "Parse error during 'IF NOT EXIST' phrase of create table" );
				table = NULL;
			}
		}
		else
		{
			DestroySQLTable( table );
         table = NULL;
			lprintf( "Parse error during 'IF NOT EXIST' phrase of create table" );
		}
	}
	//for( ; token; token = NEXTLINE( token ) )
   if( table )
	{
		phrase = GetPhrase( &token );
		tmp = BuildLine( phrase );
		table->name = StrDup( GetText( tmp ) );
		LineRelease( tmp );
		LineRelease( phrase );

		if( GetText( token )[0] != '(' )
		{
			if( TextLike( token, "LIKE" ) )
			{
				PTEXT likename;
				// not realy an error, no parens required.
				token = NEXTLINE( token );
				likename = GetPhrase( &token );
				tmp = BuildLine( likename );
				table->create_like_table_name = StrDup( GetText( tmp ) );
				LineRelease( tmp );
            LineRelease( likename );
			}
			else
			{
				DestroySQLTable( table );
				table = NULL;
			}
         lprintf( "Failure to find open to describe columns for " );
		}
		else
		{
         token = NEXTLINE( token );
         table = BuildTableDefinitions( table, &token );
		}
	}
	LineRelease( cmd );
   return table;
}

#endif
