//[6~[6~#include <windows.h>
#include <stdhdrs.h>

#include <sharemem.h> // include first to avoid EXPORT redefinition
#ifdef _MSC_VER
#include <sql.h>
#endif
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
INDEX iODBC;
//extern int myTypeID;


// some ideas for volatile variables
//  table fields
//  result fields
//  types
//  - maybe make each variable available
//    from a table volatile?   probably more overhead that way...
//    however, methods such as next/prior/first/last could be added to
//    change the state of these variables.
//  result count (rows)
//

static TEXTCHAR byResult[32768]; // largest single record from database (?)
static uint32_t ResultLen;
static TEXTSTR pResults[256]; // pointer to start of data in byResult Buffer.
static int  nResults;

//static BOOL bInterlock;  // lockout multi-thread entrace... may be irrelavent...

typedef struct odbc_handle_tag{
#ifdef _MSC_VER
   SQLHENV    env;  // odbc database access handle...
   SQLHDBC    hdbc; // handle to database connection
   SQLHSTMT hstmt;
#else
   HENV    env;  // odbc database access handle...
   HDBC    hdbc; // handle to database connection
   HSTMT   hstmt;
#endif
}ODBC, *PODBC;


typedef struct mydatapath_tag {
   DATAPATH common;
   PODBC    handle;
} MYDATAPATH, *PMYDATAPATH;

FunctionProto  
   COMMAND,
   LISTDATA,
   QUERY,
   CLOSE,
   HELP,
   ODBC_CREATE,
   ODBC_STORE,
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
	  , METH_QUERY
	  , METH_RETRIEVE
	  , METH_SETTYPE
     , METH_NEXT
};

command_entry methods[]={
 {DEFTEXT("COMMAND"),2,7  ,DEFTEXT("Perform ODBC command"),COMMAND}
 ,{DEFTEXT("SHOW")   ,2,4  ,DEFTEXT("List data from database..."),LISTDATA}
 ,{DEFTEXT("CREATE") ,2,6  ,DEFTEXT("Create an empty object with vars..."), ODBC_CREATE }
 ,{DEFTEXT("STORE") ,2,5  ,DEFTEXT("Store object with vars..."), ODBC_STORE }
 ,{DEFTEXT("QUERY")   ,2,5  ,DEFTEXT("List data from database..."),QUERY}
 ,{DEFTEXT("RETRIEVE") ,2,8  ,DEFTEXT("Retrieve objects with vars..."), RETRIEVE }
 ,{DEFTEXT("SETTYPE") ,2,7  ,DEFTEXT("Create a 'date' variable..."), SETTYPE }
 ,{DEFTEXT("next") ,2,7  ,DEFTEXT(""), SETTYPE }

};

int   QUERY( PSENTIENT ps, PTEXT parameters )
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
      if( rc == SQL_ERROR || rc == SQL_INVALID_HANDLE )
         break;
      if( rc != SQL_NO_DATA )
      {
      	PVARTEXT vt;
      	vt = VarTextCreate();
         vtprintf( vt, "(%5s)[%d]:%s", statecode, native, message );
         EnqueLink( pplq, VarTextGet( vt ) );
         VarTextDestroy( &vt );
      }
#else
      {
         DECLTEXT( msg, "Cannot get diagnostic records under NT 3.51 LCC" );
          EnqueLink( pplq, &msg );
      }
#endif
   } while( rc != SQL_NO_DATA );
}

void CreateObject( PODBC pODBC, PENTITY peContainer, PTEXT pName, PTEXT pTable )
{
   PENTITY pe;
   RETCODE rc;
   int ResultOffset;
//   while( ( rc = SQL
	SQLSMALLINT nCols;
	SQLNumResultCols(pODBC->hstmt, &nCols );
   pe = CreateEntityIn( peContainer, pName );
   while( ( rc = SQLFetch( pODBC->hstmt ) ) == SQL_SUCCESS )
	{
      nResults = 0;
      ResultOffset = 0;
      while( nResults < nCols )
      {
         pResults[nResults] = byResult + ResultOffset;
         rc = SQLGetData( pODBC->hstmt
                        , (short)(1 + nResults++)
                        , SQL_C_CHAR
                        , byResult + ResultOffset
                        , sizeof( byResult ) - ResultOffset
								, (long*)&ResultLen );
			lprintf( "SQL Col Result %d %d: %s", nResults, ResultOffset, byResult + ResultOffset );
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
		//while( rc == SQL_SUCCESS );
      lprintf( "Adding a variable to thing... %s", pResults[3] );
      AddVariable( pe->pControlledBy
              , pe,  SegCreateFromText( pResults[3] ), NULL );
   }

   {
		DECLTEXT( msg, "_TABLE" );
   lprintf( "Adding table variable... %s", GetText(pTable) );
      AddVariable( pe->pControlledBy
                 , pe, (PTEXT)&msg, pTable );
   }

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
                        , (long*)&ResultLen );

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
int   RETRIEVE( PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
   if( pODBC )
	{
      RETCODE rc;
		PENTITY pe;
		INDEX idx;
		PTEXT pVar;
		PVARTEXT pvt;
		int didone = FALSE;
		pe = FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
			//FindThing( ps->Current, FIND_VISIBLE, temp );
		if( !pe )
		{
			DECLTEXT( msg, "Cannot see entity to retrieve from or no entity specified" );
			EnqueLink( &ps->Command->Output, &msg );
			return 1;
		}
		pvt = VarTextCreate();
		LIST_FORALL( pe->pVars, idx, PTEXT, pVar )
		{
			if( TextIs( pVar, "_TABLE" ) )
            break;
		}
		//pVar = GetVariable( pe->pVars, "_TABLE" );
		if( !pVar )
		{
			DECLTEXT( msg, "Object is not associated with a table..." );
			EnqueLink( &ps->Command->Output, &msg );
			return 1;
		}
		vtprintf( pvt, "Select * from %s"
				  , GetText( GetIndirect( pVar ) ) );
		LIST_FORALL( pe->pVars, idx, PTEXT, pVar )
		{
			PTEXT pVal;
			if( TextIs( pVar, "_TABLE" ) )
			{
				continue;
			}
			pVal = GetIndirect( NEXTLINE( pVar ) );
			if( pVal )
			{
				if( didone )
					vtprintf( pvt, "," );
				else
					vtprintf( pvt, " where (" );
				{
					PTEXT pValue;
					PTEXT pInd;
					pInd = NEXTLINE( pVar );
					pValue = BuildLine( pVal );
					//if( !IsNumber( pValue ) )
					if( pInd->flags & ODBC_TF_DATE )
						vtprintf( pvt, "%s%s#%s#",
									GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
					else if( pInd->flags & ODBC_TF_NUMBER )
						vtprintf( pvt, "%s%s%s",
									GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
					else if( pInd->flags & ODBC_TF_TEXT )
						vtprintf( pvt, "%s%s\'%s\'",
									GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
					else
						vtprintf( pvt, "%s%s\'%s\'",
									GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
					//else
					//   vtprintf( vt, "%s%s%s",
					//                     GetText( pVar ), GetBound( NEXTLINE(pVar) ), GetText( pValue ) );
					LineRelease( pValue );
				}
				didone = TRUE;
			}
		}
		if( didone )
			vtprintf( pvt, ")" );
		{
			PTEXT cmd = VarTextGet( pvt );
			rc = SQLExecDirect( pODBC->hstmt
									, GetText( cmd ), SQL_NTS );
         EnqueLink( &ps->Command->Output, cmd );
			// assuming of course that we did a /odbc create record table

			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
			{
				DECLTEXT( msg, "ODBC Command execution failed(3)...." );
				EnqueLink( &ps->Command->Output, &msg );
				DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
			}
			else if( rc == SQL_SUCCESS )
			{
				BuildResultObjects( pODBC, pe );
			}
			//LineRelease( cmd );
		}
      VarTextDestroy( &pvt );
	}
	else
	{
		// output some sort of error message...
	}
	return 0;
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

   PODBC pODBC = NULL;
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
            VarTextInit( pvt );
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
               vtprintf( vt, "INSERT into %s ", GetText( NEXTLINE( pTable ) ) );
               LIST_FORALL( pls, idx, PTEXT, pVar )
               {
                  vtprintf( vt, "%s%s"
                                    , (idx)?",":"("
                                    , GetText( pVar ) );
               }
               END_LISTFORALL();
               vtprintf( vt, ") Values" );
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
                        vtprintf( vt, "%s%s"
                                          , (idx)?(wasnumber)?",":"\',":"("
                                          , GetText( pVal ) );
                        wasnumber = TRUE;
                     }
                     else
                     {
                        vtprintf( vt, "%s%s"
                                          , (idx)?(wasnumber)?",\'":"\',\'":"(\'"
                                          , GetText( pVal ) );
                        wasnumber = FALSE;
                     }
                     LineRelease( pVal );
                  }
                  END_LISTFORALL();
                  if( wasnumber )
                     vtprintf( vt, ")" );
                  else
                     vtprintf( vt, "')" );
               }
               {
	               PTEXT cmd = VarTextGet( pvt );
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
               VarTextInit( pvt );
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
               vtprintf( vt, "Select * from %s"
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
                        vtprintf( vt, "," );
                     else
                        vtprintf( vt, " where (" );
                     {
                        PTEXT pValue;
                        PTEXT pInd;
                        pInd = NEXTLINE( pVar );
                        pValue = BuildLine( pVal );
                        //if( !IsNumber( pValue ) )
                        if( pInd->flags & ODBC_TF_DATE )
                           vtprintf( vt, "%s%s#%s#",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        else if( pInd->flags & ODBC_TF_NUMBER )
                           vtprintf( vt, "%s%s%s",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        else if( pInd->flags & ODBC_TF_TEXT )
                           vtprintf( vt, "%s%s\'%s\'",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        else
                           vtprintf( vt, "%s%s\'%s\'",
                                             GetText( pVar ), GetBound( pInd ), GetText( pValue ) );
                        //else
                        //   vtprintf( vt, "%s%s%s",
                        //                     GetText( pVar ), GetBound( NEXTLINE(pVar) ), GetText( pValue ) );
                        LineRelease( pValue );
                     }
                     didone = TRUE;
                  }
               }
               if( didone )
                  vtprintf( vt, ")" );
					{
						PTEXT cmd = VarTextGet( pvt );
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

int   ODBC_STORE( PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
	RETCODE rc;
   INDEX idx;
   PENTITY pe;
   PVARTEXT vt;
   PTEXT pVar, pTable = NULL;
   PLINKSTACK pls = CreateLinkStack();
   vt = VarTextCreate();
   if( pe = FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL ) )
   {
   	int added;
      LIST_FORALL( pe->pVars, idx, PTEXT, pVar )
      {
         if( TextIs( pVar, "_TABLE" ) )
            pTable = pVar;
         else
            PushLink( &pls, pVar );
      }
      if( !pTable )
      {
         DECLTEXT( msg, "Entity does not have a table of origin defined..." );
         EnqueLink( &ps->Command->Output, &msg );
         DeleteLinkStack( &pls );
         return 0;
      }
      vtprintf( vt, "INSERT into %s ", GetText( NEXTLINE( pTable ) ) );
      added = 0;
      LIST_FORALL( pls, idx, PTEXT, pVar )
      {
      	if( GetIndirect( NEXTLINE( pVar ) ) )
      	{
	         vtprintf( vt, "%s%s"
   	                          , (added)?",":"("
      	                       , GetText( pVar ) );
				added++;
			}
      }
      vtprintf( vt, ") Values" );
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
            //while( pVal )
            {
            	//if( TextIs( pVal, "\'" ) )
            	//	SegInsert( SegDuplicate( pVal ), pVal );
            	//pVal = NEXTLINE( pVal );
            }
            pVal = pVal2;
            pVal = BuildLine( pVal );
            LineRelease( pVal2 );
				if( IsNumber( pVal )
					// if it contains a bracket, then it's probably
               // a well encoded sql function.
				  || strchr( GetText( pVal ), '(' ) )
            {
               vtprintf( vt, "%s%s"
                                 , (added)?(wasnumber)?",":"\',":"("
                                 , GetText( pVal ) );
               wasnumber = TRUE;
            }
            else
            {
               vtprintf( vt, "%s%s"
                                 , (added)?(wasnumber)?",\'":"\',\'":"(\'"
                                 , GetText( pVal ) );
               wasnumber = FALSE;
            }
            LineRelease( pVal );
            added++;
         }
         if( wasnumber )
            vtprintf( vt, ")" );
         else
            vtprintf( vt, "')" );
      }
		//DebugBreak();
		{
			PTEXT cmd;
			cmd = VarTextGet( vt );
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
   else
      {
         DECLTEXT( msg, "Cannot see entity to store into database..." );
         EnqueLink( &ps->Command->Output, &msg );
      }
	DeleteLinkStack( &pls );
   VarTextDestroy( &vt );
	return 0;
}

//---------------------------------------------------------------------------

int   ODBC_CREATE( PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
	RETCODE rc;
   
   PTEXT pName, pTable;
   GetStatement( ps, pODBC );
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
         DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
      }
      else if( rc == SQL_SUCCESS )
      {
      	PSENTIENT psIn = ps->pToldBy;
      	if( !psIn )
      		psIn = ps;
         CreateObject( pODBC, psIn->Current, SegDuplicate( pName ), pTable );
      }
   }
   else
   {
      DECLTEXT( msg, "create requires <object name> <from table>..." );
      EnqueLink( &ps->Command->Output, (PTEXT)&msg );
   }
   
   DropStatement( pODBC );
   return 0;
}

//---------------------------------------------------------------------------

int LISTDATA(PSENTIENT ps, PTEXT parameters )
{
	PODBC pODBC = (PODBC)GetLink( &ps->Current->pPlugin, iODBC );
	PVARTEXT pvt = NULL;
	RETCODE rc;
	GetStatement( ps, pODBC );
   {
      PTEXT pWhat;
      if( !pODBC )
         return 0;
		pvt = VarTextCreate();
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
                  int ResultOffset = 0;
					   SQLSMALLINT columns;
                  nResults = 0;
						if( SQLNumResultCols(pODBC->hstmt, &columns ) != SQL_SUCCESS )
						{
							DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
						}
                  else do
						{
                     pResults[nResults] = byResult + ResultOffset;
                     rc = SQLGetData( pODBC->hstmt
                                    , (short)(1 + nResults++)
                                    , SQL_C_CHAR
                                    , byResult + ResultOffset
                                    , sizeof( byResult ) - ResultOffset
                                    , (long*)&ResultLen );
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
						} while( rc == SQL_SUCCESS &&
								  nResults < columns
								 );
						if( nResults >= columns )
						{
						   pResults[nResults] = NULL;  // mark end of list...
						}
                  byResult[ResultOffset] = 0;
                  {
						   vtprintf( pvt, "%15s %s(%s)", pResults[3], pResults[5], pResults[6] );
							EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
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
                     SQLSMALLINT columns;
                     nResults = 0;
							if( SQLNumResultCols(pODBC->hstmt, &columns ) != SQL_SUCCESS )
							{
								DumpInfo( &ps->Command->Output, SQL_HANDLE_STMT, pODBC->hstmt );
							}
							else do
                     {
                        pResults[nResults] = byResult + ResultOffset;
                        rc = SQLGetData( pODBC->hstmt
                                       , (short)(1 + nResults++)
                                       , SQL_C_CHAR
                                       , byResult + ResultOffset
                                       , sizeof( byResult ) - ResultOffset
                                       , (long*)&ResultLen );
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
							while( rc == SQL_SUCCESS && nResults < columns );
							if( nResults >= columns )
                        pResults[nResults] = NULL;
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
            if( ps->CurrentMacro )
               ps->CurrentMacro->state.flags.bSuccess = TRUE;
         }
		}
		else
		{
         S_MSG( ps, "Unrecognized command... (table is only command)" );
		}
   }
	DropStatement( pODBC );
	if( pvt )
      VarTextDestroy( &pvt );
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

void CPROC Destroy( PENTITY pe )
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

int CPROC Create( PSENTIENT ps, PENTITY pe, PTEXT parameters )
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
      pPassword = GetParam( ps, &parameters );
      {
      	PTEXT Connect1 = SegCreate( 256 );
      	Connect1->data.size =
      		sprintf( Connect1->data.data
						, "DSN=%s%s%s%s%s"
						, GetText( pDSN )
                  , pID?";UID=":""
						, pID?GetText( pID ):""
						, pPassword?";PWD=":""
						, pPassword?GetText( pPassword ):""
						 );
         Log1( "Using \"%s\" to connect..", GetText( Connect1 ) );
      rc = SQLDriverConnect( pODBC->hdbc
                           , NULL // window handle - do not show dialogs
                           , GetText( Connect1 )
                           , (SQLSMALLINT)GetTextSize( Connect1 )
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
	   //ps2 = CreateAwareness( pe );
      AddMethod( pe, methods + METH_LIST );
      AddMethod( pe, methods + METH_COMMAND );
      AddMethod( pe, methods + METH_CREATE );
      AddMethod( pe, methods + METH_STORE );
      AddMethod( pe, methods + METH_RETRIEVE );
      if( ps->CurrentMacro )
         ps->CurrentMacro->state.flags.bSuccess = TRUE;
   }
   else
   {
      DECLTEXT( msg, "Open reqires <database DSN> <user> <password>..." );
      EnqueLink( &ps->Command->Output, &msg );
      return -1;
   }

	return 0;
}


INDEX iODBC;

PUBLIC( char *, RegisterRoutines )( void )
{
   //pExportedFunctions = pExportTable;

   //UpdateMinSignficants( commands, nCommands, NULL );
   RegisterObject( "odbc", "Generic ODBC Object... parameters determine database", Create );
   //RegisterRoutine( "odbc", "Microsoft SQL Interface Commands (/odbc help)", HandleODBC );
   // not a real device type... but need the ID...
   //myTypeID = RegisterDevice( "odbc", "ODBC Database stuff", NULL );
   iODBC = RegisterExtension( "odbc" );
	return DekVersion;
}


PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   UnregisterObject( "odbc" );
   //UnregisterRoutine( "odbc" );
}


// $Log: odbc.c,v $
// Revision 1.22  2005/04/20 06:20:24  d3x0r
// Okay a massive refit to 'FindThing' To behave like GetParam(SubstToken) to handle count.object references better.
//
// Revision 1.21  2005/02/23 11:38:59  d3x0r
// Modifications/improvements to get MSVC to build.
//
// Revision 1.20  2005/02/22 12:28:48  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.19  2005/01/18 02:47:01  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.18  2005/01/17 09:01:16  d3x0r
// checkpoint ...
//
// Revision 1.17  2004/04/06 03:47:17  d3x0r
// Build object correctly... psh - guess it just needed logging to stimulate compile?
//
// Revision 1.16  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.15  2003/11/08 00:09:41  panther
// fixes for VarText abstraction
//
// Revision 1.14  2003/10/27 17:37:18  panther
// Remove some commented out code
//
// Revision 1.13  2003/10/14 23:11:45  panther
// Fix columns variable type
//
// Revision 1.12  2003/10/01 14:47:43  panther
// Update to use colmn count instead of faulting last column
//
// Revision 1.11  2003/10/01 12:14:07  panther
// Default to DSN instead of filedsn... remove password/database
//
// Revision 1.10  2003/03/25 08:59:02  panther
// Added CVS logging
//
