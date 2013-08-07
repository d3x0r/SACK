
#include <vcclr.h>
#include <stdhdrs.h>
#include <deadstart.h>
#include "sqlstruc.h"

using namespace System;

namespace sack {
	namespace sql {
		public ref class SQL
		{
			static int opens;
			PODBC odbc;
		public:
			enum class SQLMode
			{
				Access
				, ODBC
				, Sqlite
			};
			static SQL()
			{
			}

			SQL( String^ dsn )
			{
				InvokeDeadstart();
				pin_ptr<const WCHAR> _dsn = PtrToStringChars(dsn);
				CTEXTSTR __dsn = DupWideToText( _dsn );
				odbc = ConnectToDatabase( __dsn );
				opens++;
				lprintf( WIDE("Opening database %d"), opens );
			}

			~SQL()
			{
				CloseDatabase( odbc );
				opens--;
				lprintf( WIDE("Closing database %d remain"), opens );
			}

			SQLMode Mode()
			{
				if( odbc->flags.bAccess )
					return SQLMode::Access;
				if( odbc->flags.bSQLite )
					return SQLMode::Sqlite;
				return SQLMode::ODBC;
			}

			static array<String^>^ Query( String^ query )
			{
				pin_ptr<const WCHAR> _query = PtrToStringChars(query);
				CTEXTSTR __query = DupWideToText( _query );
				int result_count;
				CTEXTSTR *results;
				SQLRecordQuery( NULL, __query, &result_count, &results, NULL );
				array<String^>^ result = gcnew array<String^>(result_count);
				for( int n = 0; n < result_count; n++ )
				{
					result[n] = gcnew String( results[n] );
				}
				Deallocate( const WCHAR *, __query );
				return result;
			}

			array<String^>^ query( String^ query )
			{
				pin_ptr<const WCHAR> _query = PtrToStringChars(query);
				CTEXTSTR __query = DupWideToText( _query );
				int result_count;
				CTEXTSTR *results;
				SQLRecordQuery( odbc, __query, &result_count, &results, NULL );
				array<String^>^ result;
				if( !results )
				{
					result = gcnew array<String^>(0);
				}
				else
				{
					result = gcnew array<String^>(result_count);
					for( int n = 0; n < result_count; n++ )
					{
						result[n] = gcnew String( results[n] );
					}
				}
				Release( (POINTER)__query );
				return result;
			}
			static bool Command( String^ command )
			{
				pin_ptr<const WCHAR> _command = PtrToStringChars(command);
				CTEXTSTR __command = DupWideToText( _command );
				bool result = ( SQLCommand( NULL, __command ) != 0 );
				Release( (POINTER)__command );
				return result;
				
			}
			bool command( String^ command )
			{
				pin_ptr<const WCHAR> _command = PtrToStringChars(command);
				CTEXTSTR __command = DupWideToText( _command );
				bool result = ( SQLCommand( odbc, __command ) != 0 );
				Release( (POINTER)__command );
				return result;
				
			}
			void end_query( void )
			{
				PopODBCEx( odbc );
			}
			void check_table( String ^create_statement )
			{
				pin_ptr<const WCHAR> _command = PtrToStringChars(create_statement);
				CTEXTSTR __command = DupWideToText( _command );
				PTABLE table = GetFieldsInSQL( __command, 0 );
				CheckODBCTable( odbc, table, CTO_MERGE );
				Release( (POINTER)__command );
				DestroySQLTable( table );
			}
			int last_insert_id( void )
			{
				return FetchLastInsertID( odbc, NULL, NULL );
			}
		};
	}
}
