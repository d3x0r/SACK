
#include <sharemem.h>
#include <sack_types.h>
#define PLUGIN_MODULE
#include "commands.h"
#include "plugin.h"
#include <libpq-fe.h>
#include <postgres_ext.h>


typedef struct database_tag{
   PTEXT  ConnectionDescription;
	PGconn *Connection;
	PGresult *lastresult;
   ExecStatusType status;
   PSENTIENT ps;
	struct {
		_32 bConnected;
      _32 bReconnecting;
	} flags;
} DATABASE, *PDATABASE;

typedef struct {
	int OID;
   PTEXT name;
} OIDText;

INDEX iPSQL;

FunctionProto  
   COMMAND,
   LISTDATA,
   QUERY,
   CLOSE,
   HELP,
   CREATE,
   STORE,
   RETRIEVE,
   SETTYPE;

#define ODBC_TF_DATE   0x80000000
#define ODBC_TF_NUMBER 0x40000000
#define ODBC_TF_TEXT   0x20000000

enum {
   METH_COMMAND
   , METH_LIST
   , METH_CREATE
   , METH_STORE
};

command_entry methods[]={
 {DEFTEXT("COMMAND"),2,7  ,DEFTEXT("Perform ODBC command"),COMMAND}
 ,{DEFTEXT("SHOW")   ,2,4  ,DEFTEXT("List data from database..."),LISTDATA}
 ,{DEFTEXT("CREATE") ,2,6  ,DEFTEXT("Create an empty object with vars..."), CREATE }
 ,{DEFTEXT("STORE") ,2,5  ,DEFTEXT("Store object with vars..."), STORE }
 ,{DEFTEXT("QUERY")   ,2,5  ,DEFTEXT("List data from database..."),QUERY}
 ,{DEFTEXT("RETRIEVE") ,2,8  ,DEFTEXT("Retrieve objects with vars..."), RETRIEVE }
 ,{DEFTEXT("SETTYPE") ,2,7  ,DEFTEXT("Create a 'date' variable..."), SETTYPE }

};


//	CONNECTION_OK,
//	CONNECTION_BAD,
//	/* Non-blocking mode only below here */
//
//	/*
//	 * The existence of these should never be relied upon - they should
//	 * only be used for user feedback or similar purposes.
//	 */
//	CONNECTION_STARTED,			/* Waiting for connection to be made.  */
//	CONNECTION_MADE,			/* Connection OK; waiting to send.	   */
//	CONNECTION_AWAITING_RESPONSE,		/* Waiting for a response from the
//										 * postmaster.		  */
//	CONNECTION_AUTH_OK,			/* Received authentication; waiting for
//								 * backend startup. */
//	CONNECTION_SETENV			/* Negotiating environment.    */

int ValidateConnection( PDATABASE database )
{
   int status;
	status = PQstatus( database->Connection );
	if( status == CONNECTION_BAD )
	{
		if( database->flags.bConnected )
		{
			DECLTEXT( msg, "Connection to database has failed." );
			EnqueLink( &database->ps->Command->Output, &msg );
			database->flags.bConnected = 0;
         database->flags.bReconnecting = 1;
         return 0;
		}
		else if( database->flags.bReconnecting )
		{
			DECLTEXT( msg, "Attempt to reconnect has failed. " );
			EnqueLink( &database->ps->Command->Output, &msg );
         return 0;
		}
		else
		{
			DECLTEXT( msg, "Attempt to connect failed : Connection follows" );
			EnqueLink( &database->ps->Command->Output, &msg );
         EnqueLink( &database->ps->Command->Output, SegDuplicate( database->ConnectionDescription ) );
         return 0;
		}
	}

	{
		if( !database->flags.bConnected )
		{
			if( database->flags.bReconnecting )
			{
				DECLTEXT( msg, "Reconnection completed." );
            EnqueLink( &database->ps->Command->Output, &msg );
            database->flags.bReconnecting = 1;
			}
         database->flags.bConnected = 1;
		}
	}
   return 1;
}


//---------------------------------------------------------------------------

PGresult *GetColumns( PDATABASE database, PTEXT whichtable )
{
	VARTEXT vt;
   PTEXT temp;
	PGresult *lastresult;
   ExecStatusType status;
	VarTextInit( &vt );
	vtprintf( &vt, "select * from %s where (1=0)", GetText( whichtable ) );
	temp = VarTextGet( &vt );
	Log1( "Doing: %s", GetText( temp ) );
	lastresult = PQexec( database->Connection, GetText( temp ) );
	status = PQresultStatus( lastresult );
   VarTextEmpty( &vt );
	if( status == PGRES_TUPLES_OK )
	{
		return lastresult;
	}
	PQclear( lastresult );
   return NULL;
}

//---------------------------------------------------------------------------

void CreateObject( PDATABASE database, PENTITY peContainer, PTEXT pName, PTEXT pTable )
{
   PENTITY pe;
	int ResultOffset;
   int cols, c;
   PGresult *lastresult;
	pe = CreateEntityIn( peContainer, pName );
   {
      DECLTEXT( msg, "_TABLE" );
      AddVariable( pe->pControlledBy
					  , pe
					  , (PTEXT)&msg
					  , pTable );
	}

	lastresult = GetColumns( database, pTable );
	if( lastresult )
	{
		cols = PQnfields( lastresult );
      Log1( "table has %d columns", cols );
		for( c = 0; c < cols; c++ )
		{
         Log1( "Adding Variable(%s)", PQfname( lastresult, c ) );
			AddVariable( pe->pControlledBy
						  , pe
						  , SegCreateFromText( PQfname( lastresult, c ) )
						  , NULL );
		}
		PQclear( database->lastresult );
      database->lastresult = NULL;
	}
	else
	{
		DECLTEXT( msg, "Failed to find table to create object from" );
      EnqueLink( &peContainer->pControlledBy->Command->Output, &msg );
	}

}

//---------------------------------------------------------------------------

int   CREATE( PSENTIENT ps, PTEXT parameters )
{
	PDATABASE database = (PDATABASE)GetLink( &ps->Current->pPlugin, iPSQL );
	PTEXT pName, pTable;
   if( (pName = GetParam( ps, &parameters ) )&&
       (pTable = GetParam( ps, &parameters )) )
	{
		PSENTIENT psIn = ps->pToldBy;
		if( !psIn )
			psIn = ps;
		CreateObject( database, psIn->Current, SegDuplicate( pName ), pTable );
   }
   else
   {
      DECLTEXT( msg, "create requires <object name> <from table>" );
      EnqueLink( &ps->Command->Output, (PTEXT)&msg );
   }
   return 0;
}


//---------------------------------------------------------------------------

int COMMAND(PSENTIENT ps, PTEXT params )
{
	PDATABASE database = (PDATABASE)GetLink( &ps->Current->pPlugin, iPSQL );
	if( database )
	{
		PTEXT temp = MacroDuplicate( ps, params );
		PTEXT command = BuildLine( temp );

		Log1( "Execute: %s", GetText( command ) );
		database->lastresult = PQexec( database->Connection, GetText( command ) );

		database->status = PQresultStatus( database->lastresult );

		switch( database->status )
		{
		case PGRES_COMMAND_OK:
			//{
			//	DECLTEXT( msg, "Command Success." );
			//	EnqueLink( &ps->Command->Output, &msg );
			//}
			break;
		case PGRES_EMPTY_QUERY:
			{
				DECLTEXT( msg, "Empty command - success." );
            EnqueLink( &ps->Command->Output, &msg );
			}
         break;
		case PGRES_TUPLES_OK:
			{
				DECLTEXT( msg, "Command Success(tuples)." );
            EnqueLink( &ps->Command->Output, &msg );
			}
			break;
		case PGRES_COPY_OUT:
			{
				DECLTEXT( msg, "Copy out (from server) data transfer started..." );
            EnqueLink( &ps->Command->Output, &msg );
			}
         break;
		case PGRES_COPY_IN:
			{
				DECLTEXT( msg, "Copy in (to server) data transfer started..." );
            EnqueLink( &ps->Command->Output, &msg );
			}
			break;
		case PGRES_BAD_RESPONSE:
			{
				DECLTEXT( msg, "Server responce garbled." );
            EnqueLink( &ps->Command->Output, &msg );
			}
			break;
		case PGRES_NONFATAL_ERROR:
			{
				DECLTEXT( msg, "Some non-fatal error occured - huh?" );
            EnqueLink( &ps->Command->Output, &msg );
			}
			break;
		case PGRES_FATAL_ERROR:
			{
				DECLTEXT( msg, "Some fatal error occured - how bad is bad?" );
            EnqueLink( &ps->Command->Output, &msg );
			}
          break;
		}
      if( database->status != PGRES_COMMAND_OK )
		{
			PTEXT extra;
			extra = SegCreateFromText( PQresultErrorMessage( database->lastresult ) );
			EnqueLink( &ps->Command->Output, extra );
		}
		PQclear( database->lastresult );
      database->lastresult = NULL;
		Release( command );
		Release( temp );
	}
	else
	{
	}
   return 0;
}

//---------------------------------------------------------------------------

int   LISTDATA(PSENTIENT ps, PTEXT parameters )
{
	PDATABASE database = (PDATABASE)GetLink( &ps->Current->pPlugin, iPSQL );

   {
      PTEXT pWhat;
      if( !database )
         return 0;
      if( !(pWhat = GetParam( ps, &parameters ) ) )
      {
         DECLTEXT( msg, "please specify what to list.  (table, ... )" );
         EnqueLink( &ps->Command->Output, &msg );
      }
      else if( TextLike( pWhat, "table" ) ) 
      {
         PTEXT pWhich;
         if( pWhich = GetParam( ps, &parameters ) )
			{
				PTEXT temp;
				VARTEXT vt;
            VarTextInit( &vt );
				database->lastresult = GetColumns( database, pWhich );
				if( database->lastresult )
				{
					int cols = PQnfields( database->lastresult ), c;
					if( PQntuples( database->lastresult ) )
					{
						DECLTEXT( msg, "Unexpected result from database - 1=0 is true?!" );
                  EnqueLink( &ps->Command->Output, &msg );
					}
					for( c = 0; c < cols; c++ )
					{
						PGresult *lastresult;
						ExecStatusType status;
                  PTEXT temp;
						//vtprintf( &vt, "select description from pg_description where (objoid=%d)", PQftype( database->lastresult, c ) );
						vtprintf( &vt, "select typname from pg_type where (oid=%d)", PQftype( database->lastresult, c ) );
                  temp = VarTextGet( &vt );
						lastresult = PQexec( database->Connection, GetText( temp ) );
                  status = PQresultStatus( lastresult );
						LineRelease( temp );
						if( status == PGRES_TUPLES_OK && PQntuples( lastresult ) )
							vtprintf( &vt, "%s is a(n) %s(%d)"
									  , PQfname( database->lastresult, c )
									  , PQgetvalue( lastresult, 0, 0 )
									  , PQfsize( database->lastresult, c ) );
						else
							vtprintf( &vt, "%s is type %d"
									  , PQfname( database->lastresult, c )
									  , PQftype( database->lastresult, c ) );
						EnqueLink( &ps->Command->Output, VarTextGet( &vt ) );
                  PQclear( lastresult );
					}
				}
				else
				{
					DECLTEXT( msg, "Failed to get columns from table..." );
					EnqueLink( &ps->Command->Output, &msg );
				}
            VarTextEmpty( &vt );

				PQclear( database->lastresult );
            database->lastresult = NULL;
         }
         else
			{
				database->lastresult = PQexec( database->Connection, "select * from pg_tables where (tableowner!='postgres')" );
				database->status = PQresultStatus( database->lastresult );

				if( database->status == PGRES_TUPLES_OK )
				{
					int rows = PQntuples( database->lastresult ), r;
					int tablename, owner;
               VARTEXT vt;
               VarTextInit( &vt );
					tablename = PQfnumber( database->lastresult, "tablename" );
					owner = PQfnumber( database->lastresult, "tableowner" );
					for( r = 0; r < rows; r++ )
					{
						vtprintf( &vt, "%s owned by %s", PQgetvalue( database->lastresult, r, tablename )
								  , PQgetvalue( database->lastresult, r, owner ) );
                  EnqueLink( &ps->Command->Output, VarTextGet( &vt ) );
					}
               VarTextEmpty( &vt );
				}

				PQclear( database->lastresult );
            database->lastresult = NULL;
			}
      }
	}
	return 0;
}

//---------------------------------------------------------------------------

int   QUERY(PSENTIENT ps, PTEXT params ) { return 0; };
int   CLOSE(PSENTIENT ps, PTEXT params ) { return 0; };
int   HELP(PSENTIENT ps, PTEXT params ) { return 0; };
int   STORE(PSENTIENT ps, PTEXT params ) { return 0; };
int   RETRIEVE(PSENTIENT ps, PTEXT params ) { return 0; };
int   SETTYPE(PSENTIENT ps, PTEXT params ) { return 0; };


//---------------------------------------------------------------------------

int Create( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	PDATABASE database;
	PTEXT address, database_name, user, password;
   VARTEXT conninfo;
	address = GetParam( ps, &parameters );
	if( !address )
	{
		DECLTEXT( msg, "Must specify a database address to connect to.  Also, database name, user, password(optional)." );
		EnqueLink( &ps->Command->Output, &msg );
      return -1;
	}

	database_name = GetParam( ps, &parameters );
 	if( !database_name )
	{
      DECLTEXT( msg, "Must specify the name of a database to connect to. Also username, password(optional)." );
		EnqueLink( &ps->Command->Output, &msg );
      return -1;
	}
	user = GetParam( ps, &parameters );
 	if( !user )
	{
      DECLTEXT( msg, "Must specify the name of a user to connect as. Also password(optional)." );
		EnqueLink( &ps->Command->Output, &msg );
      return -1;
	}
   password = GetParam( ps, &parameters );

	VarTextInit( &conninfo );
   if( password )
		vtprintf( &conninfo, "dbname=%s host=%s user=%s password=%s requiressl=true"
				     , GetText( database_name ), GetText( address ), GetText( user ), GetText( password ) );
   else
		vtprintf( &conninfo, "dbname=%s host=%s user=%s requiressl=true"
				     , GetText( database_name ), GetText( address ), GetText( user ) );
	database = Allocate( sizeof( DATABASE ) );
	MemSet( database, 0, sizeof( DATABASE ) );

	database->ConnectionDescription = VarTextGet( &conninfo );
   VarTextEmpty( &conninfo );
	database->Connection = PQconnectdb( GetText( database->ConnectionDescription ) );
	database->ps = ps; // temporarily use this - so we can get immediate results.
   // if the initial connect fails, abort this object's creation
	if( !ValidateConnection( database ) )
	{
		Release( database );
      return -1;
	}
   database->ps = CreateAwareness( pe );
   SetLink( &pe->pPlugin, iPSQL, database );

   AddMethod( pe, methods + METH_LIST );
   AddMethod( pe, methods + METH_COMMAND );
   AddMethod( pe, methods + METH_CREATE );
   //AddMethod( pe, methods + METH_STORE );
   if( ps->CurrentMacro )
      ps->CurrentMacro->state.flags.bSuccess = TRUE;
		WakeAThread( database->ps );

   return 0;
}



//---------------------------------------------------------------------------

PUBLIC( char *, RegisterRoutines )( void )
{
   //pExportedFunctions = pExportTable;

   //UpdateMinSignficants( commands, nCommands, NULL );
   RegisterObject( "database", "Generic Postgres Object... parameters determine database", Create );
   iPSQL = RegisterExtension();
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterObject( "postgresql" );
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


#ifdef THIS_IS_OLD_CODE_FOR_REFERENCE_ONLY

#include <windows.h>

#include <sharemem.h> // include first to avoid EXPORT redefinition
//#include <sql.h>
#include <sqlext.h>

#include <stdio.h>
#include <string.h>
#define PLUGIN_MODULE
#include "plugin.h"

#ifdef _LINUX
///src/msql/msql.c</tt>
///src/msql/relshow.c</tt>
//<li> <tt>./src/msql/msqldump.c</tt>
//<li> <tt>./src/msql/msqladmin.c</tt>
#endif
extern INDEX iODBC;
//extern int myTypeID;


static BYTE byResult[32768]; // largest single record from database (?)
static LONG ResultLen;
static PBYTE pResults[256]; // pointer to start of data in byResult Buffer.
static int  nResults;

//static BOOL bInterlock;  // lockout multi-thread entrace... may be irrelavent...

typedef struct odbc_handle_tag{
   HENV    env;  // odbc database access handle...
   HDBC    hdbc; // handle to database connection
   HSTMT   hstmt;
}ODBC, *PODBC;


typedef struct mydatapath_tag {
   DATAPATH common;
   PODBC    handle;
} MYDATAPATH, *PMYDATAPATH;


int   QUERY( PSENTIENT ps, PTEXT parameters )
{
	return 0;
}
int   RETRIEVE( PSENTIENT ps, PTEXT parameters )
{
	return 0;
}
int   SETTYPE( PSENTIENT ps, PTEXT parameters )
{
	return 0;
}


void DumpInfo( PLINKQUEUE *pplq, SQLSMALLINT type, SQLHANDLE handle )
{
   RETCODE rc;
   char statecode[6];
   char message[256];
   long  native;
   short  msglen;
   SQLSMALLINT  n;
   n = 1;
   do
   {
#if 1 || ( !defined( __LCC__ ) && !defined( __TURBOC__ ) )
      rc = SQLGetDiagRec( type 
                        , handle
                        , n++, statecode
                        , &native
                        , message, sizeof( message ), &msglen );
      if( rc == SQL_INVALID_HANDLE )
         break;
      if( rc != SQL_NO_DATA )
      {
      	VARTEXT vt;
      	VarTextInit( &vt );
         vtprintf( &vt, "(%5s)[%d]:%s", statecode, native, message );
         EnqueLink( pplq, VarTextGet( &vt ) );
         VarTextEmpty( &vt );
      }
#else
      {
         DECLTEXT( msg, "Cannot get diagnostic records under NT 3.51 LCC" );
          EnqueLink( pplq, &msg );
      }
#endif
   } while( rc != SQL_NO_DATA );
}



char * GetBound( PTEXT pText )
{
   if( !pText )
      return "(Bad Variable)";
   if( pText->flags & ( TF_LOWER | TF_UPPER | TF_EQUAL ) )
   {
      if( (pText->flags & ( TF_LOWER | TF_UPPER ) ) == ( TF_LOWER | TF_UPPER ) )
         return "<>";
      if( (pText->flags & ( TF_LOWER | TF_EQUAL ) ) == ( TF_LOWER | TF_EQUAL ) )
         return ">=";
      if( (pText->flags & ( TF_EQUAL | TF_UPPER ) ) == ( TF_EQUAL | TF_UPPER ) )
         return "<=";
      if( pText->flags & ( TF_LOWER ) )
         return ">";
      if( pText->flags & ( TF_UPPER ) ) 
         return "<";
   }

   return "=";
}

void BuildResultObjects( PODBC pODBC, PENTITY peSource )
{
   PENTITY pe;
   RETCODE rc;
   int bDuplicated = FALSE;
   INDEX idx;
   int ResultOffset;

   while( ( rc = SQLFetch( pODBC->hstmt ) ) == SQL_SUCCESS )
   {
      nResults = 0;
      ResultOffset = 0;
      idx = 0;
      bDuplicated = TRUE;
      pe = Duplicate( peSource );
      do
      {
         pResults[nResults] = byResult + ResultOffset;
         rc = SQLGetData( pODBC->hstmt
                        , (short)(1 + nResults++)
                        , SQL_C_CHAR
                        , byResult
                        , sizeof( byResult )
                        , &ResultLen );

         if( rc == SQL_SUCCESS || 
             rc == SQL_SUCCESS_WITH_INFO )
         {
            PTEXT pVar;
            pVar = pe->pVars->pNode[idx];
            if( ResultLen == SQL_NO_TOTAL ||  // -4
                ResultLen == SQL_NULL_DATA )  // -1 
            {
            }

            if( ResultLen > 0 )
            {
               if( idx < pe->pVars->Cnt )
               {
                  LineRelease( GetIndirect( NEXTLINE( pVar ) ) );
                  SetIndirect( NEXTLINE( pVar ), SegCreateFromText( byResult ) );
               }
               else
               {
                  DebugBreak();
               }
            }
            else
            {
               LineRelease( GetIndirect( NEXTLINE( pVar ) ) );
               SetIndirect( NEXTLINE( pVar ), 0 );
               pResults[nResults-1] = NULL;
            }
            idx++;
         }
         else
         {
            nResults--;  // one less result.
            pResults[nResults] = NULL;  // mark end of list...
         }
      } while( rc == SQL_SUCCESS );
   }
   if( bDuplicated )
      DestroyEntity( peSource );
}

/*
int HandleODBC( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp, command;
   PODBC pODBC = NULL;
   PLINKQUEUE *pplq;
   PMYDATAPATH pdp;
   int idx;
   int ResultOffset;

   pdp = (PMYDATAPATH)GetLink( ps->Current->pPlugin, myPlug );

   if( ps->CurrentMacro )
   {
      ps->CurrentMacro->state.flags.bSuccess = FALSE;
      if( pdp )
         pplq = pdp->common.ppInput;
   }
   else
      pplq = &ps->Command->Output;


   if( pdp && pdp->common.Type != myTypeID )
   {
      DECLTEXT( msg, "Data path is not available for ODBC use..." );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   if( command = GetParam( ps, &parameters ) ) 
   {
      RETCODE rc;
      idx = GetCommandIndex( commands, NUM_COMMANDS
                           , GetTextSize(command), GetText(command) );
      if( idx < 0 )
      {
         DECLTEXT( msg, "Unknown ODBC function specified." );
         EnqueLink( &ps->Command->Output, &msg );
         return 0;
      }

      if( pdp )
         pODBC = pdp->handle;
      else
         pODBC = NULL;

      if( pODBC && pODBC->hdbc )
      {
         rc = SQLAllocStmt( pODBC->hdbc
                          , &pODBC->hstmt );
         if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
         {
            DECLTEXT( msg, "Failed to open ODBC statement handle...." );
            EnqueLink( &ps->Command->Output, &msg );
            return 0;
         }
      }

      if( !pODBC && 
          commands[idx].function != _OPEN &&
          commands[idx].function != HELP )
      {
         DECLTEXT( msg, "ODBC Device is not open..." );
         EnqueLink( &ps->Command->Output, &msg );
         return 0;
      }


      switch( commands[idx].function )
      {
      case _OPEN:
      {
         PTEXT pDSN, pID, pPassword;

         if( !pODBC )
         {
            pdp = (PMYDATAPATH)CreateDataPath( sizeof( MYDATAPATH ) - sizeof( DATAPATH) );
            SetLink( ps->Current->pPlugin, myPlug, pdp );
            pdp->common.Type = myTypeID;
            pODBC = pdp->handle = (PODBC)Allocate( sizeof( ODBC) );
            MemSet( pODBC, 0, sizeof( ODBC ) );
         }
         if( !pODBC->env ) 
         {
            rc = SQLAllocEnv( &pODBC->env );
            if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
            {
               DECLTEXT( msg, "Failed to establish ODBC Environment Handle...." );
               EnqueLink( &ps->Command->Output, &msg );
               return 0;
            }
         }
         if( pODBC->hdbc )
         {
            DECLTEXT( msg, "ODBC Connection is already open...." );
            EnqueLink( &ps->Command->Output, &msg );
            return 0;
         }

         if( SQLAllocConnect( pODBC->env,
                              &pODBC->hdbc ) )
         {
            DECLTEXT( msg, "Failed to create ODBC Connection Handle...." );
            EnqueLink( &ps->Command->Output, &msg );
            goto clean_open0;
         }

         if( pDSN = GetParam( ps, &parameters ) )
         {  
            pID = GetParam( ps, &parameters );
            if( !(pPassword = GetParam( ps, &parameters ) ) )
            {
               DECLTEXT( msg, "Need to supply a password to open database us \" \" to specify none." );
               EnqueLink( &ps->Command->Output, &msg );
               goto clean_open1;
            }
            if( GetTextSize( pPassword ) == 1 &&
                GetText( pPassword )[0] == ' ' )
               pPassword->data.data[0] = 0;
            rc = SQLConnect( pODBC->hdbc
                           , GetText( pDSN ), SQL_NTS
                           , GetText( pID ), SQL_NTS
                           , GetText( pPassword ), SQL_NTS );
            if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
            {
               DECLTEXT( msg, "Failed to open ODBC Connection...." );
               EnqueLink( &ps->Command->Output, &msg );
               DumpInfo( pplq, SQL_HANDLE_DBC, pODBC->hdbc );
               goto clean_open1;
            }
            if( ps->CurrentMacro )
               ps->CurrentMacro->state.flags.bSuccess = TRUE;
         }
         else
         {
            DECLTEXT( msg, "Open reqires <database DSN> <user> <password>..." );
            EnqueLink( &ps->Command->Output, &msg );
            SQLFreeConnect( pODBC->hdbc );
            pODBC->hdbc = 0;
            SQLFreeEnv( pODBC->env );
            pODBC->env = 0;

            Release( pODBC );
            pODBC = NULL;

            pdp->handle = 0;
            pdp->common.Type = 0;
         }
      }
      break;
      case COMMAND:
      {
         PTEXT pCommand;
         pCommand = BuildLine( parameters );
         rc = SQLExecDirect( pODBC->hstmt
                           , GetText( pCommand ), SQL_NTS );
         LineRelease( pCommand );
         if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
         {
            DECLTEXT( msg, "ODBC Command excecution failed(1)...." );
            EnqueLink( &ps->Command->Output, &msg );
            DumpInfo( pplq, SQL_HANDLE_STMT, pODBC->hstmt );
         } 
      }
      break;
      case LISTDATA:
      {
         PTEXT pWhat;
         if( !pODBC )
            return 0;
         if( !(pWhat = GetParam( ps, &parameters ) ) )
         {
            DECLTEXT( msg, "please specify what to list.  (table, ... )" );
            EnqueLink( &ps->Command->Output, &msg );
         }
         else if( TextLike( pWhat, "table" ) ) 
         {
            PTEXT pWhat;
            if( pWhat = GetParam( ps, &parameters ) )
            {
               // SQLPrepare 
               // includes the table to list... therefore list the fields in the table.
               rc = SQLColumns( pODBC->hstmt
                              , NULL, 0
                              , NULL, 0
                              , GetText( pWhat ), SQL_NTS
                              , NULL, 0 );
               if( rc != SQL_SUCCESS_WITH_INFO &&
                   rc != SQL_SUCCESS )
               {
                  DumpInfo( pplq, SQL_HANDLE_STMT, pODBC->hstmt );
               }
               else if( rc == SQL_SUCCESS )
               {
                  while( ( rc = SQLFetch( pODBC->hstmt ) ) == SQL_SUCCESS )
                  {
                     nResults = 0;
                     ResultOffset = 0;
                     do
                     {
                        pResults[nResults] = byResult + ResultOffset;
                        rc = SQLGetData( pODBC->hstmt
                                       , (short)(1 + nResults++)
                                       , SQL_C_CHAR
                                       , byResult + ResultOffset
                                       , sizeof( byResult ) - ResultOffset
                                       , &ResultLen );
                        if( rc == SQL_SUCCESS || 
                            rc == SQL_SUCCESS_WITH_INFO )
                        {
                           if( ResultLen == SQL_NO_TOTAL ||  // -4
                               ResultLen == SQL_NULL_DATA )  // -1 
                           {
                           }

                           if( ResultLen > 0 )
                           {
                              byResult[ResultOffset + ResultLen] = 0;
                              ResultOffset += ResultLen + 1;
                           }
                           else
                           {
                              pResults[nResults-1] = NULL;
                           }
                        }
                        else
                        {
                           nResults--;  // one less result.
                           pResults[nResults] = NULL;  // mark end of list...
                        }
                     } while( rc == SQL_SUCCESS );
//                     byResult[ResultOffset] = 0;
                     EnqueLink( pplq
                              , SegAppend( SegCreateFromText( pResults[5] ),
                                           SegCreateFromText( pResults[3] ) ) );
                  }
                  EnqueLink( pplq, SegCreate(0) );
               }
            }
            else
            {
               rc = SQLTables( pODBC->hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0 );
               if( rc == SQL_SUCCESS_WITH_INFO )
               {
                  DECLTEXT( msg, "blah info back..." );
                  EnqueLink( &ps->Command->Output, &msg );
               }
               else if( rc != SQL_SUCCESS_WITH_INFO && rc != SQL_SUCCESS )
               {
                  DumpInfo( pplq, SQL_HANDLE_STMT, pODBC->hstmt );
               }
               else if( rc == SQL_SUCCESS )
               {
                  RETCODE rc;
                  do
                  {
                     rc = SQLFetch( pODBC->hstmt );
                     if( rc == SQL_SUCCESS ) 
                     {
                        RETCODE rc;
                        ResultOffset = 0;
                        nResults = 0;
                        do
                        {
                           pResults[nResults] = byResult + ResultOffset;
                           rc = SQLGetData( pODBC->hstmt
                                          , (short)(1 + nResults++)
                                          , SQL_C_CHAR
                                          , byResult + ResultOffset
                                          , sizeof( byResult ) - ResultOffset
                                          , &ResultLen );
                           if( rc == SQL_SUCCESS || 
                               rc == SQL_SUCCESS_WITH_INFO )
                           {
                              if( ResultLen == SQL_NO_TOTAL ||  // -4
                                  ResultLen == SQL_NULL_DATA )  // -1 
                              {
                              }

                              if( ResultLen > 0 )
                              {
                                 byResult[ResultOffset + ResultLen] = 0;
                                 ResultOffset += ResultLen + 1;
                              }
                              else
                              {
                                 pResults[nResults-1] = NULL;
                              }
                           }
                           else
                           {
                              nResults--;  // one less result.
                              pResults[nResults] = NULL;  // mark end of list...
                           }
                        }
                        while( rc == SQL_SUCCESS );
                        if( nResults > 4 )
                        {
                           if( !strcmp( pResults[3], "TABLE" ) )
                           {
                              EnqueLink( pplq
                                       , SegCreateFromText( pResults[2] ) );
                           }
                        }
                     }
                     else
                     {
                        DumpInfo( pplq, SQL_HANDLE_STMT, pODBC->hstmt );
                     }
                  }
                  while( rc == SQL_SUCCESS );
               }
               EnqueLink( pplq, SegCreate(0) );
               if( ps->CurrentMacro )
                  ps->CurrentMacro->state.flags.bSuccess = TRUE;
            }
         }
      }
      break;
      case CREATE:
         {
            PTEXT pName, pTable;
            if( (pName = GetParam( ps, &parameters ) )&&
                (pTable = GetParam( ps, &parameters )) )
            {
               rc = SQLColumns( pODBC->hstmt
                              , NULL, 0
                              , NULL, 0
                              , GetText( pTable ), SQL_NTS
                              , NULL, 0 );
               if( rc != SQL_SUCCESS_WITH_INFO &&
                   rc != SQL_SUCCESS )
               {
                  DumpInfo( pplq, SQL_HANDLE_STMT, pODBC->hstmt );
               }
               else if( rc == SQL_SUCCESS )
               {
                  SegGrab( pName ); // keep this name...
                  CreateObject( pODBC, ps->Current, pName, pTable );
               }
            }
            else
            {
               DECLTEXT( msg, "create requires <object name> <from table>..." );
               EnqueLink( &ps->Command->Output, (PTEXT)&msg );
            }
         }
         break;
      case STORE:
         {
            PTEXT pte;
            INDEX idx;
            PENTITY pe;
            PTEXT pVar, pTable = NULL;
            VARTEXT vt;
            PLINKSTACK pls = CreateLinkStack();
            VarTextInit( &vt );
            if( ( pte = GetParam( ps, &parameters ) ) )
            {
               pe = FindThing( ps->Current, FIND_VISIBLE, pte );
               if( !pe )
               {
                  DECLTEXT( msg, "Cannot see entity to store into database..." );
                  EnqueLink( &ps->Command->Output, &msg );
                  break;
               }
               FORALL( pe->pVars, idx, pVar )
               {
                  if( TextIs( pVar, "_TABLE" ) )
                     pTable = pVar;
                  else
                     PushLink( pls, pVar );
               }
               if( !pTable )
               {
                  DECLTEXT( msg, "Entity does not have a table of origin defined..." );
                  EnqueLink( &ps->Command->Output, &msg );
                  DeleteLinkStack( pls );
                  break;
               }
               vtprintf( &vt, "INSERT into %s ", GetText( NEXTLINE( pTable ) ) );
               LIST_FORALL( pls, idx, PTEXT, pVar )
               {
                  vtprintf( &vt, "%s%s"
                                    , (idx)?",":"("
                                    , GetText( pVar ) );
               }
               END_LISTFORALL();
               vtprintf( &vt, ") Values" );
               {
                  int wasnumber = FALSE;
                  LIST_FORALL( pls, idx, PTEXT, pVar )
                  {
                     PTEXT pVal;
                     pVal = GetIndirect( NEXTLINE( pVar ) );
                     if( pVal->flags & TF_QUOTE )
                     {
                        pVal->flags &= ~TF_QUOTE; // should be copy....
                        pVal = BuildLine( pVal );
                        pVal->flags |= TF_QUOTE;
                     }
                     else
                        pVal = BuildLine( pVal );
                     if( IsNumber( pVal ) )
                     {
                        vtprintf( &vt, "%s%s"
                                          , (idx)?(wasnumber)?",":"\',":"("
                                          , GetText( pVal ) );
                        wasnumber = TRUE;
                     }
                     else
                     {
                        vtprintf( &vt, "%s%s"
                                          , (idx)?(wasnumber)?",\'":"\',\'":"(\'"
                                          , GetText( pVal ) );
                        wasnumber = FALSE;
                     }
                     LineRelease( pVal );
                  }
                  END_LISTFORALL();
                  if( wasnumber )
                     vtprintf( &vt, ")" );
                  else
                     vtprintf( &vt, "')" );
               }
               {
	               PTEXT cmd = VarTextGet( &vt );
   	            rc = SQLExecDirect( pODBC->hstmt
      	                           , GetText( cmd ), SQL_NTS );

         	      if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
      	         {
   	               PTEXT pCommand;
	                  DECLTEXT( msg, "ODBC Command excecution failed(2)...." );
                  	EnqueLink( &ps->Command->Output, &msg );
               	   pCommand = SegDuplicate( cmd );
            	      DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
         	         EnqueLink( &ps->Command->Output, pCommand );
      	         } 
   	            else
	               {
                  	DECLTEXT( msg, "Object stored into database." );
               	   if( ps->CurrentMacro )
            	         ps->CurrentMacro->state.flags.bSuccess = TRUE;
         	         else
      	               EnqueLink( &ps->Command->Output, &msg );
   	               DestroyEntity( pe ); 
	               }
						LineRelease( cmd );
					}
               // get all variables...
               // get _TABLE variable...
               // dunno how to fetch variables really....
            }  
            DeleteLinkStack( pls );
         }
         break;
      case RETRIEVE:
         {
            if( temp = GetParam( ps, &parameters ) )
            {
               PENTITY pe;
               INDEX idx;
               PTEXT pVar;
	            VARTEXT vt;
               int didone = FALSE;
               VarTextInit( &vt );
               pe = FindThing( ps->Current, FIND_VISIBLE, temp );
               if( !pe )
               {
                  DECLTEXT( msg, "Cannot see entity to retrieve from..." );
                  EnqueLink( &ps->Command->Output, &msg );
                  break;
               }
               pVar = GetVariable( pe->pVars, "_TABLE" );
               if( !pVar )
               {
                  DECLTEXT( msg, "Object is not associated with a table..." );
                  EnqueLink( &ps->Command->Output, &msg );
                  break;
               }
               vtprintf( &vt, "Select * from %s"
               					, GetText( GetIndirect( pVar ) ) );
               FORALL( pe->pVars, idx, pVar )
               {
                  PTEXT pVal;
                  if( TextIs( pVar, "_TABLE" ) )
                     continue;
                  pVal = GetIndirect( NEXTLINE( pVar ) );
                  if( pVal )
                  {
                     if( didone )
                        vtprintf( &vt, "," );
                     else
                        vtprintf( &vt, " where (" );
                     {
                        PTEXT pValue;
                        PTEXT pInd;
                        pInd = NEXTLINE( pVar );
                        pValue = BuildLine( pVal );
                        //if( !IsNumber( pValue ) )
                        if( pInd->flags & ODBC_TF_DATE )
                           vtprintf( &vt, "%s%s#%s#",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        else if( pInd->flags & ODBC_TF_NUMBER )
                           vtprintf( &vt, "%s%s%s",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        else if( pInd->flags & ODBC_TF_TEXT )
                           vtprintf( &vt, "%s%s\'%s\'",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        else
                           vtprintf( &vt, "%s%s\'%s\'",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        //else
                        //   vtprintf( &vt, "%s%s%s",
                        //                     GetText( pVar ), GetBound( NEXTLINE(pVar) ), GetText( pValue ) );
                        LineRelease( pValue );
                     }
                     didone = TRUE;
                  }
               }
               if( didone )
                  vtprintf( &vt, ")" );
					{
						PTEXT cmd = VarTextGet( &vt );
	               rc = SQLExecDirect( pODBC->hstmt
   	                              , GetText( cmd ), SQL_NTS );
	               // assuming of course that we did a /odbc create record table

   	            if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
      	         {
         	         DECLTEXT( msg, "ODBC Command excecution failed(3)...." );
            	      EnqueLink( &ps->Command->Output, &msg );
               	   DumpInfo( pplq, SQL_HANDLE_STMT, pODBC->hstmt );
	               } 
   	            else if( rc == SQL_SUCCESS )
      	         {
         	         PENTITY pe;
            	      pe = FindThing( ps->Current, FIND_VISIBLE, temp );
               	   if( pe )
                  	{  // this is the prototype record to fetch... 
                     	// build the select query from it's variables...
	                     BuildResultObjects( pODBC, pe );
   	               }
      	            else
         	         {
            	         // of course output fail message....
               	   }
						}
						LineRelease( cmd );
               }
            }
            else
            {
               // output some sort of error message... 
            }
         }
         break;
      case QUERY:
         {
         }
         break;
      case CLOSE:
         if( pdp && pdp->common.Type )
         {
            if( pODBC )
            {
               pODBC->hstmt = 0;
               SQLDisconnect( pODBC->hdbc );
   clean_open1:
               SQLFreeConnect( pODBC->hdbc );
               pODBC->hdbc = 0;
   clean_open0:
               SQLFreeEnv( pODBC->env );
               pODBC->env = 0;
               pdp->common.Type = 0;
               Release( pODBC );
               pODBC = NULL;
               pdp->handle = 0;
               DestroyDataPath( (PDATAPATH)pdp );
               SetLink( ps->Current->pPlugin, myPlug, 0 );
            }
         }
         break;
      case HELP:
         {
            DECLTEXT( leader, " --- ODBC Builtin Commands ---" );
            EnqueLink( &ps->Command->Output, &leader );
            WriteCommandList( &ps->Command->Output, commands, NUM_COMMANDS, NULL );
         }
         break;
      case SETTYPE:
         {
            PTEXT pSave, pBoundType;
            int one_bound = FALSE;
            pSave = parameters;
            temp = GetParam( ps, &parameters );
            if( pSave == temp )
            {
               DECLTEXT( msg, "Invalid variable reference passed to SETTYPE." );
               EnqueLink( &ps->Command->Output, &msg );
               break;
            }
            if( !temp )
            {
               DECLTEXT( msg, "Must specify variable and type to set." );
               EnqueLink( &ps->Command->Output, &msg );
               break;
            }
            while( pBoundType = GetParam( ps, &parameters ) )
            {
               one_bound = TRUE;
               if( TextLike( pBoundType, "date" ) )
               {
                  temp->flags &= ~( ODBC_TF_DATE|ODBC_TF_NUMBER|ODBC_TF_TEXT );
                  temp->flags |= ODBC_TF_DATE;
               }
               else if( TextLike( pBoundType, "number" ) )
               {
                  temp->flags &= ~( ODBC_TF_DATE|ODBC_TF_NUMBER|ODBC_TF_TEXT );
                  temp->flags |= ODBC_TF_NUMBER;
               }
               else if( TextLike( pBoundType, "text" ) )
               {
                  temp->flags &= ~( ODBC_TF_DATE|ODBC_TF_NUMBER|ODBC_TF_TEXT );
                  temp->flags |= ODBC_TF_TEXT;
               }
            }
            if( !one_bound )
            {
               DECLTEXT( msg, "Must specify type to set...date, number, text" );
               EnqueLink( &ps->Command->Output, &msg );
               break;
            }

         }
         break;
      default:
         {
            DECLTEXT( msg, "ODBC Operation unknown....check /odbc HELP..." );
            EnqueLink( &ps->Command->Output, &msg );
         }
         break;
      }
   }
}
   else
   {
      DECLTEXT( msg, "ODBC command not found... please use /odbc HELP..." );
      EnqueLink( &ps->Command->Output, &msg );
   }
   // always clear old statement....
   if( pODBC && pODBC->hstmt )
   {
      SQLFreeStmt( pODBC->hstmt, SQL_CLOSE );
      pODBC->hstmt = NULL;
   }

   return 0;
}

*/

//---------------------------------------------------------------------------

int GetStatement( PSENTIENT ps, PODBC pODBC )
{
	RETCODE rc;
   if( pODBC && pODBC->hdbc )
   {
      rc = SQLAllocStmt( pODBC->hdbc
                       , &pODBC->hstmt );
      if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
      {
         DECLTEXT( msg, "Failed to open ODBC statement handle...." );
         EnqueLink( &ps->Command->Output, &msg );
         return 0;
      }
      return 1;
   }
   return 0;
}

//---------------------------------------------------------------------------

void DropStatement( PODBC pODBC )
{
   if( pODBC && pODBC->hstmt )
   {
      SQLFreeStmt( pODBC->hstmt, SQL_CLOSE );
      pODBC->hstmt = NULL;
   }
}

//---------------------------------------------------------------------------

int   STORE( PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
	RETCODE rc;
   PTEXT pte;
   INDEX idx;
   PENTITY pe;
   VARTEXT vt;
   PTEXT pVar, pTable = NULL;
   PLINKSTACK pls = CreateLinkStack();
   VarTextInit( &vt );
   if( ( pte = GetParam( ps, &parameters ) ) )
   {
   	int added;
      pe = FindThing( ps->Current, FIND_VISIBLE, pte );
      if( !pe )
      {
         DECLTEXT( msg, "Cannot see entity to store into database..." );
         EnqueLink( &ps->Command->Output, &msg );
      }
      LIST_FORALL( pe->pVars, idx, PTEXT, pVar )
      {
         if( TextIs( pVar, "_TABLE" ) )
            pTable = pVar;
         else
            PushLink( &pls, pVar );
      }
      LIST_ENDFORALL();
      if( !pTable )
      {
         DECLTEXT( msg, "Entity does not have a table of origin defined..." );
         EnqueLink( &ps->Command->Output, &msg );
         DeleteLinkStack( &pls );
         return 0;
      }
      vtprintf( &vt, "INSERT into %s ", GetText( NEXTLINE( pTable ) ) );
      added = 0;
      LIST_FORALL( pls, idx, PTEXT, pVar )
      {
      	if( GetIndirect( NEXTLINE( pVar ) ) )
      	{
	         vtprintf( &vt, "%s%s"
   	                          , (added)?",":"("
      	                       , GetText( pVar ) );
				added++;
			}
      }
      LIST_ENDFORALL();
      vtprintf( &vt, ") Values" );
      {
         int wasnumber = FALSE;
	      added = 0;
         LIST_FORALL( pls, idx, PTEXT, pVar )
         {
            PTEXT pVal, pVal2;
            if( !GetIndirect( NEXTLINE( pVar ) ) )
            	continue;
            pVal = TextDuplicate( GetIndirect( NEXTLINE( pVar ) ), FALSE );
            pVal2 = pVal;
            while( pVal )
            {
            	if( TextIs( pVal, "\'" ) )
            		SegInsert( SegDuplicate( pVal ), pVal );
            	pVal = NEXTLINE( pVal );
            }
            pVal = pVal2;
            pVal = BuildLine( pVal );
            LineRelease( pVal2 );
            if( IsNumber( pVal ) )
            {
               vtprintf( &vt, "%s%s"
                                 , (added)?(wasnumber)?",":"\',":"("
                                 , GetText( pVal ) );
               wasnumber = TRUE;
            }
            else
            {
               vtprintf( &vt, "%s%s"
                                 , (added)?(wasnumber)?",\'":"\',\'":"(\'"
                                 , GetText( pVal ) );
               wasnumber = FALSE;
            }
            LineRelease( pVal );
            added++;
         }
         LIST_ENDFORALL();
         if( wasnumber )
            vtprintf( &vt, ")" );
         else
            vtprintf( &vt, "')" );
      }
		//DebugBreak();
		{
			PTEXT cmd;
			cmd = VarTextGet( &vt );
   	   GetStatement( ps, pODBC );
      	rc = SQLExecDirect( pODBC->hstmt
         	               , GetText( cmd ), SQL_NTS );

	      if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
   	   {
      	   PTEXT pCommand;
         	DECLTEXT( msg, "ODBC Command excecution failed(2)...." );
	         EnqueLink( &ps->Command->Output, &msg );
      	   pCommand = SegDuplicate( cmd );
   	      DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
         	EnqueLink( &ps->Command->Output, pCommand );
	      }
   	   else
      	{
         	DECLTEXT( msg, "Object stored into database." );
	         if( ps->CurrentMacro )
   	         ps->CurrentMacro->state.flags.bSuccess = TRUE;
      	   else
         	   EnqueLink( &ps->Command->Output, &msg );
	         DestroyEntity( pe );
   	   }
      	// get all variables...
	      // get _TABLE variable...
   	   // dunno how to fetch variables really....
      	DropStatement( pODBC );
      	LineRelease( cmd );
	   }
	}
   DeleteLinkStack( &pls );
	return 0;
}


//---------------------------------------------------------------------------

int LISTDATA(PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
	RETCODE rc;
	GetStatement( ps, pODBC );
   {
      PTEXT pWhat;
      if( !pODBC )
         return 0;
      if( !(pWhat = GetParam( ps, &parameters ) ) )
      {
         DECLTEXT( msg, "please specify what to list.  (table, ... )" );
         EnqueLink( &ps->Command->Output, &msg );
      }
      else if( TextLike( pWhat, "table" ) ) 
      {
         PTEXT pWhich;
         if( pWhich = GetParam( ps, &parameters ) )
         {
            // SQLPrepare 
            // includes the table to list... therefore list the fields in the table.
            rc = SQLColumns( pODBC->hstmt
                           , NULL, 0
                           , NULL, 0
                           , GetText( pWhich ), SQL_NTS
                           , NULL, 0 );
            if( rc != SQL_SUCCESS_WITH_INFO &&
                rc != SQL_SUCCESS )
            {
               DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
            }
            else if( rc == SQL_SUCCESS )
            {
               while( ( rc = SQLFetch( pODBC->hstmt ) ) == SQL_SUCCESS )
               {
                  nResults = 0;
                  int ResultOffset = 0;
                  do
                  {
                     pResults[nResults] = byResult + ResultOffset;
                     rc = SQLGetData( pODBC->hstmt
                                    , (short)(1 + nResults++)
                                    , SQL_C_CHAR
                                    , byResult + ResultOffset
                                    , sizeof( byResult ) - ResultOffset
                                    , &ResultLen );
                     if( rc == SQL_SUCCESS || 
                         rc == SQL_SUCCESS_WITH_INFO )
                     {
                        if( ResultLen == SQL_NO_TOTAL ||  // -4
                            ResultLen == SQL_NULL_DATA )  // -1 
                        {
                        }

                        if( ResultLen > 0 )
                        {
                           byResult[ResultOffset + ResultLen] = 0;
                           ResultOffset += ResultLen + 1;
                        }
                        else
                        {
                           pResults[nResults-1] = NULL;
                        }
                     }
                     else
                     {
                        nResults--;  // one less result.
                        pResults[nResults] = NULL;  // mark end of list...
                     }
                  } while( rc == SQL_SUCCESS );
                  byResult[ResultOffset] = 0;
                  {
                  	PTEXT field;
	                  EnqueLink( &ps->Command->Output
   	                        , SegAppend( SegCreateFromText( pResults[5] ),
      	                                  field = SegCreateFromText( pResults[3] ) ) );
							field->format.position.offset.spaces = 1;
						}
               }
               EnqueLink( &ps->Command->Output, SegCreate(0) );
            }
         }
         else
         {
            rc = SQLTables( pODBC->hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0 );
            if( rc == SQL_SUCCESS_WITH_INFO )
            {
               DECLTEXT( msg, "blah info back..." );
               EnqueLink( &ps->Command->Output, &msg );
            }
            else if( rc != SQL_SUCCESS_WITH_INFO && rc != SQL_SUCCESS )
            {
               DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
            }
            else if( rc == SQL_SUCCESS )
            {
               RETCODE rc;
               do
               {
                  rc = SQLFetch( pODBC->hstmt );
                  if( rc == SQL_SUCCESS ) 
                  {
                     RETCODE rc;
                     int ResultOffset = 0;
                     nResults = 0;
                     do
                     {
                        pResults[nResults] = byResult + ResultOffset;
                        rc = SQLGetData( pODBC->hstmt
                                       , (short)(1 + nResults++)
                                       , SQL_C_CHAR
                                       , byResult + ResultOffset
                                       , sizeof( byResult ) - ResultOffset
                                       , &ResultLen );
                        if( rc == SQL_SUCCESS || 
                            rc == SQL_SUCCESS_WITH_INFO )
                        {
                           if( ResultLen == SQL_NO_TOTAL ||  // -4
                               ResultLen == SQL_NULL_DATA )  // -1 
                           {
                           }

                           if( ResultLen > 0 )
                           {
                              byResult[ResultOffset + ResultLen] = 0;
                              ResultOffset += ResultLen + 1;
                           }
                           else
                           {
                              pResults[nResults-1] = NULL;
                           }
                        }
                        else
                        {
                           nResults--;  // one less result.
                           pResults[nResults] = NULL;  // mark end of list...
                        }
                     }
                     while( rc == SQL_SUCCESS );
                     if( nResults > 4 )
                     {
                        if( !strcmp( pResults[3], "TABLE" ) )
                        {
                           EnqueLink( &ps->Command->Output
                                    , SegCreateFromText( pResults[2] ) );
                        }
                     }
                  }
                  else
                  {
                     DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
                  }
               }
               while( rc == SQL_SUCCESS );
            }
            EnqueLink( &ps->Command->Output, SegCreate(0) );
            if( ps->CurrentMacro )
               ps->CurrentMacro->state.flags.bSuccess = TRUE;
         }
      }
   }
   DropStatement( pODBC );
   return 0;
}

//---------------------------------------------------------------------------

int   COMMAND( PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
	RETCODE rc;
   PTEXT pCommand;
   {
   	DECLTEXT( msg, "Command to be issued!" );
   	EnqueLink( &ps->Command->Output, &msg );
   }
	GetStatement( ps, pODBC );
   
   pCommand = BuildLine( parameters );
   rc = SQLExecDirect( pODBC->hstmt
                     , GetText( pCommand ), SQL_NTS );
   LineRelease( pCommand );
   if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
   {
      DECLTEXT( msg, "ODBC Command excecution failed(1)...." );
      EnqueLink( &ps->Command->Output, &msg );
      DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
   } 
   else
   {
      if( ps->CurrentMacro )
      {
         ps->CurrentMacro->state.flags.bSuccess = TRUE;
      }
      else
      {
	      DECLTEXT( msg, "ODBC Command success." );
   	   EnqueLink( &ps->Command->Output, &msg );
		}
   }
   DropStatement( pODBC );
	return 0;
}

//---------------------------------------------------------------------------

void Destroy( PENTITY pe )
{
	PODBC pODBC = (PODBC)GetLink( &pe->pPlugin, iODBC );
	if( pODBC )
	{
		if( pODBC->hdbc )
		{
         pODBC->hstmt = 0;
         SQLDisconnect( pODBC->hdbc );
         SQLFreeConnect( pODBC->hdbc );
         pODBC->hdbc = 0;
      }
		if( pODBC->env )
		{
         SQLFreeEnv( pODBC->env );
         pODBC->env = 0;
		}
		Release( pODBC );
	}
	SetLink( &pe->pPlugin, iODBC, 0 );
}

//---------------------------------------------------------------------------

int Create( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	// init the ODBC object...
	// take parameters as if "OPEN"
	PSENTIENT ps2;
   PTEXT pDSN, pID, pPassword;
   PODBC pODBC = NULL;
   RETCODE rc;

   pODBC = Allocate( sizeof( ODBC ) );
   MemSet( pODBC, 0, sizeof( ODBC ) );
   SetLink( &pe->pPlugin, iODBC, pODBC );
   AddLink( &pe->pDestroy, Destroy );
   rc = SQLAllocEnv( &pODBC->env );
   if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
   {
      DECLTEXT( msg, "Failed to establish ODBC Environment Handle...." );
      EnqueLink( &ps->Command->Output, &msg );
      return -1;
   }

   if( SQLAllocConnect( pODBC->env,
                        &pODBC->hdbc ) )
   {
      DECLTEXT( msg, "Failed to create ODBC Connection Handle...." );
      EnqueLink( &ps->Command->Output, &msg );
      return -1;
   }

   if( pDSN = GetParam( ps, &parameters ) )
   {  
      pID = GetParam( ps, &parameters );
      /*
      if( !pID )
      {
      	DECLTEXT( msg, "Need to supply a user ID to open database as" );
         EnqueLink( &ps->Command->Output, &msg );
         return -1;
      }
      */
      pPassword = GetParam( ps, &parameters );
      /*
      if( !pPassword )
      {
         DECLTEXT( msg, "Need to supply a password to open database use \" \" to specify none." );
         EnqueLink( &ps->Command->Output, &msg );
         return -1;
      }
      */
      /*
      if( GetTextSize( pPassword ) == 1 &&
          GetText( pPassword )[0] == ' ' )
         pPassword->data.data[0] = 0;
      */
      /*
      rc = SQLConnect( pODBC->hdbc
                     , GetText( pDSN ), SQL_NTS
                     , GetText( pID ), SQL_NTS
                     , GetText( pPassword ), SQL_NTS 
                     );
      */
      {
      	PTEXT Connect1 = SegCreate( 256 );
      	Connect1->data.size =
      		sprintf( Connect1->data.data
      			, "FILEDSN=%s", GetText( pDSN ) );
      rc = SQLDriverConnect( pODBC->hdbc
                           , NULL // window handle - do not show dialogs
                           , GetText( Connect1 )
                           , GetTextSize( Connect1 )
                           , NULL
                           , 0
                           , NULL
                           , SQL_DRIVER_NOPROMPT );
		}
      if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
      {
         DECLTEXT( msg, "Failed to open ODBC Connection...." );
         EnqueLink( &ps->Command->Output, &msg );
         DumpInfo( &ps->Command->Output, SQL_HANDLE_DBC, pODBC->hdbc );
         return -1;
      }
	   ps2 = CreateAwareness( pe );
      AddMethod( pe, methods + METH_LIST );
      AddMethod( pe, methods + METH_COMMAND );
      AddMethod( pe, methods + METH_CREATE );
      AddMethod( pe, methods + METH_STORE );
      if( ps->CurrentMacro )
         ps->CurrentMacro->state.flags.bSuccess = TRUE;
	WakeAThread( ps2 );
   }
   else
   {
      DECLTEXT( msg, "Open reqires <database DSN> <user> <password>..." );
      EnqueLink( &ps->Command->Output, &msg );
      return -1;
   }

	return 0;
}

#endif
// $Log: postgresql.c,v $
// Revision 1.7  2003/03/25 08:59:02  panther
// Added CVS logging
//
