#include <stdhdrs.h>
#include <stdio.h>
#include <pssql.h>
#include <sharemem.h>

void Usage( int bFull )
{
	if( bFull )
		fprintf( stderr, WIDE("usage: %s [-f <script file>] [DSN or Sqlite db file] [-n(o headers)]\n"), GetProgramName() );

	fprintf( stderr, WIDE("Command must start with '?', '!' or '='.\n") );
	fprintf( stderr, WIDE("?<query> results go to the current output, or the screen if non specified before\n") );
	fprintf( stderr, WIDE("!<command> - issues command to \n") );
	fprintf( stderr, WIDE("=<table>@<DSN><,DSN>,...> setup output to these DSN's for next command or query result\n") );
	fprintf( stderr, WIDE("?!<query> results go to the current output (with replace instead of insert), or the screen if non specified before\n") );
	fprintf( stderr, WIDE("\\q on a new line - quit\n") );
}

void CPROC LogSQLStates( CTEXTSTR message )
{
	lprintf( WIDE("%s"), message );
}
void CPROC ShowSQLStates( CTEXTSTR message )
{
	fprintf( stderr, WIDE("%s\n"), message );
}

int main( int argc, char **argv )
{
	FILE *input = stdin;
	PVARTEXT pvt_cmd;
	TEXTCHAR readbuf[4096];
	TEXTCHAR *buf;
	int offset = 0;
	int no_headers = 0;
	PODBC default_odbc = NULL;
	CTEXTSTR select_into;
	PLIST output = NULL;
	PLIST outputs = NULL;
	int arg_ofs = 0;
	SQLSetFeedbackHandler( ShowSQLStates );
	//SetAllocateDebug( TRUE );
	//SetAllocateLogging( TRUE );
	if( argc < 2 )
		Usage( 1 );
	else
	{
		while( argv[1+arg_ofs] && ( argv[1+arg_ofs][0] == '-' ) )
		{
			TEXTSTR tmp;
			switch( argv[1+arg_ofs][1] )
			{
			case 'n':
				no_headers = 1;
				break;
			case 'f':
				arg_ofs++;
				input = sack_fopen( 0, tmp = DupCharToText( argv[1+arg_ofs] ), WIDE("rt") );
				if( input )
					SQLSetFeedbackHandler( LogSQLStates );
				break;
			}
			if( tmp )
			{
				Deallocate( TEXTSTR, tmp );
				tmp = NULL;
			}
			arg_ofs++;
		}
		if( argv[1+arg_ofs] )
			default_odbc = ConnectToDatabase( DupCharToText( argv[1 + arg_ofs] ) );
	}
	SetHeapUnit( 4096 * 1024 ); // 4 megs expansion if needed...
	pvt_cmd = VarTextCreateExx( 10000, 50000 );


	while( (buf = readbuf), fgets( readbuf + offset
					, sizeof( readbuf ) - offset
					, input ) )
	{
		CTEXTSTR *result = NULL;
		size_t len;
		while( buf[0] == WIDE(' ') || buf[0] == WIDE('\t') )
         buf++;
		len = strlen( buf );
		if( buf[0] == WIDE('#') )
			continue;
		if( ( len > 0 ) && buf[len-1] == WIDE('\n') )
		{
			len--;
			buf[len] = 0;
		}

		if( strcmp( buf, WIDE("\\q") ) == 0 )
			break;

		if( !buf[0] && VarTextLength( pvt_cmd ) == 0 )
			continue;

		if( ( len > 0 ) && buf[len-1] == WIDE('\\') )
		{
			buf[len-1] = 0;
			len--;
			vtprintf( pvt_cmd, WIDE("%s"), buf );
			offset = 0;
			//offset = (len - 1); // read over the slash
			continue;
		}
		else
		{
			if( len > 0 )
				vtprintf( pvt_cmd, WIDE("%s"), buf );
			offset = 0;
		}

		buf = GetText( VarTextPeek( pvt_cmd ) );

		if( buf[0] == WIDE('?') )
		{
			int fields;
			int replace = 0;
			int ofs = 1;
			CTEXTSTR *columns;
			TEXTSTR *_columns;
			PVARTEXT pvt = NULL;
			if( buf[1] == WIDE('!') )
			{
				replace = 1;
				ofs = 2;
			}
			if( output )
			{
				if( !select_into || !select_into[0] )
				{
					printf( WIDE("Table name was invalid to insert into on the destination side...\'%s\'"), select_into );
					VarTextEmpty( pvt_cmd );
					continue;
				}
				pvt = VarTextCreateExx( 10000, 50000 );
			}
			if( SQLRecordQuery( default_odbc, buf + ofs, &fields, &result, &columns ) )
			{
				int count = 0;
				int first = 1;
				_columns = NewArray( TEXTSTR, fields );
				if( !no_headers )
				{
					{
						int n;
						for( n = 0; n < fields; n++ )
						{
							_columns[n] = StrDup( columns[n] );
							if( !pvt )
								fprintf( stdout, WIDE("%s%s"), n?WIDE(","):WIDE(""), columns[n] );
						}
					}
					if( !pvt )
						fprintf( stdout, WIDE("\n") );
				}
				for( ; result; FetchSQLRecord( default_odbc, &result ) )
				{
					if( pvt && first )
					{
						vtprintf( pvt, WIDE("%s into `%s` ("), replace?WIDE("replace"):WIDE("insert ignore"), select_into );
						{
							int first = 1;
							int n;
							for( n = 0; n < fields; n++ )
							{
								vtprintf( pvt, WIDE("%s`%s`"), first?WIDE(""):WIDE(","), _columns[n] );
								first = 0;
							}
						}
						vtprintf( pvt, WIDE(") values ") );
					}
					if( pvt )
					{
						vtprintf( pvt, WIDE("%s("), first?WIDE(""):WIDE(",") );
						{
							int first = 1; // private first, sorry :) parse that, Visual studio can.
							int n;
							for( n = 0; n < fields; n++ )
							{
								TEXTSTR tmp;
								vtprintf( pvt, WIDE("%s%s")
										  , first?WIDE(""):WIDE(",")
										  , result[n]?(tmp=EscapeStringOpt( result[n], TRUE)):((tmp=NULL),WIDE("NULL"))
										  );
								Release( tmp );
								first = 0;
							}
						}
						vtprintf( pvt, WIDE(")") );
					}
					else
					{
						int n;
						int first = 1;
						for( n = 0; n < fields; n++ )
						{

							fprintf( stdout, WIDE("%s%s"), first?WIDE(""):WIDE(","),result[n]?result[n]:WIDE("NULL") );
							first = 0;
						}
						fprintf( stdout, WIDE("\n") );
					}
					first = 0;
					count++;
					if( ( VarTextLength( pvt ) ) > 100000 )
					{
						PTEXT cmd;
						first = 1; // reset first to rebuild the beginning of the insert.
						printf( WIDE("Flushing at 100k characters...%d records\n"), count );
						if( pvt )
						{
							cmd = VarTextGet( pvt );
							if( cmd )
							{
								INDEX idx;
								PODBC odbc;
								LIST_FORALL( output, idx, PODBC, odbc )
								{
									if( !SQLCommand( odbc, GetText( cmd ) ) )
										printf( WIDE("Failed command to:%s\n"), (CTEXTSTR)GetLink( &outputs, idx ) );
								}
								LineRelease( cmd );
							}
						}
					}
				}
				if( !no_headers )
				{
					int n;
					for( n = 0; n < fields; n++ )
					{
						Release( _columns[n] );
					}
				}
				Release( _columns );
			}
			else
			{
				CTEXTSTR result;
				FetchSQLError( default_odbc, &result );
				if( result )
					fprintf( stderr, WIDE("%s\n"), result );
				else
					fprintf( stderr, WIDE("Failed, no error result.\n") );
			}
			if( pvt )
			{
				PTEXT cmd;
				printf( WIDE("Flushing command\n") );
				cmd = VarTextGet( pvt );
				if( cmd )
				{
					INDEX idx;
					PODBC odbc;
					LIST_FORALL( output, idx, PODBC, odbc )
					{
						printf( WIDE("Flushing command to %s\n"), (CTEXTSTR)GetLink( &outputs, idx ) );
						if( !SQLCommand( odbc, GetText( cmd ) ) )
						{
							CTEXTSTR error;
							FetchSQLError( odbc, &error );
							printf( WIDE("%s\n"), error );
						}
					}
					LineRelease( cmd );
				}
			}

			if( pvt )
			{
				VarTextDestroy( &pvt );
			}
			if( output )
			{
				INDEX idx;
				PODBC odbc;
				LIST_FORALL( output, idx, PODBC, odbc )
				{
					TEXTSTR output_name = (TEXTSTR)GetLink( &outputs, idx );
					Release( output_name );
					CloseDatabase( odbc );
				}
				DeleteList( &outputs );
				DeleteList( &output );
			}
		}
		else if( buf[0] == WIDE('!') )
		{
			if( output )
			{
				PODBC odbc;
				INDEX idx;
				LIST_FORALL( output, idx, PODBC, odbc )
				{
					printf( WIDE("Issue command to: %s\n"), (CTEXTSTR)GetLink( &outputs, idx ) );
					if( !SQLCommand( odbc, buf +1 ) )
					{
						CTEXTSTR result;
						FetchSQLError( odbc, &result );
						fprintf( stderr, WIDE("%s\n"), result );
					}
				}
				if( output )
				{
					INDEX idx;
					PODBC odbc;
					LIST_FORALL( output, idx, PODBC, odbc )
					{
						TEXTSTR output_name = (TEXTSTR)GetLink( &outputs, idx );
						Release( output_name );
						CloseDatabase( odbc );
					}
					DeleteList( &outputs );
					DeleteList( &output );
				}
				printf( WIDE("Ok.\n") );
			}
			else
			{
				if( !SQLCommand( default_odbc, buf +1 ) )
				{
					CTEXTSTR result;
					FetchSQLError( default_odbc, &result );
					fprintf( stderr, WIDE("%s\n"), result );
				}
				else
					fprintf( stderr, WIDE("Ok.\n") );
			}
		}
		else if( buf[0] == WIDE('=') )
		{
			TEXTCHAR *start = buf + 1;
			TEXTCHAR *end;
			end = strchr( start, WIDE('@') );
			if( !end )
			{
				printf( WIDE("Must specify table@dsn,dsn,dsn... no @ found on :%s\n"), buf );
			}
			else
			{
				end[0] = 0;
				end++;
				select_into = StrDup( start );
				while( ( start = end ) != 0 )
				{
					PODBC odbc;
					end = strchr( start, WIDE(',') );
					if( end )
					{
						end[0] = 0;
						end++;
					}

					AddLink( &output, odbc = ConnectToDatabase( start ) );
					if( IsSQLOpen( odbc ) )
					{
						printf( WIDE("Connected to: %s\n"), start );
						AddLink( &outputs, StrDup( start ) );
					}
					else
					{
						CTEXTSTR error;
						FetchSQLError( odbc, &error );
						printf( WIDE("Failed connect to:%s [%s]\n"), start, error );
					}
				}

			}
		}
		else
		{
			fprintf( stderr, WIDE("%s"), buf );
			Usage( 0 );
		}
		VarTextEmpty( pvt_cmd );
	}
	{
		//uint32_t a,b,c,d;
		//DebugDumpMem();
	}
	return 0;
}


