#define DEFINES_DEKWARE_INTERFACE
//[6~[6~#include <windows.h>
#include <stdhdrs.h>

#include <sharemem.h> // include first to avoid EXPORT redefinition
#include <pssql.h>

#include <stdio.h>
#include <string.h>
#define PLUGIN_MODULE
#include "plugin.h"

INDEX iODBC, iResult;
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

//static BOOL bInterlock;  // lockout multi-thread entrace... may be irrelavent...

typedef struct my_pluging_data {
	PLIST owned_by;
   INDEX uses;
	PODBC pODBC;
	// need to keep the recordset here for
	// access via parameters...

	// so /odbc/query result_name 'sql command'
	//   creates a single object of result_name,
	//   and it is created within the database...
	//   objects may then also be told to query, which effectively
	//    relates them within the related parent object

	//  /result_name/vars
	//  /result_name/next, prior, first, last - return success/failure for macro usage
	//  %(result_name)variable
	//  %(result_name)
	//  syntax %result_name.child.column works
	//  syntax /result_name/child/next
   //
	//     hmm -
	// okay so then... volatile variables are indexes into the current result set...


} OBJECT_DATA, *POBJECT_DATA;


typedef struct variable_data_tag {
   struct query_result_object_tag *pqro;
   INDEX iColumn; // which column index it is...
	volatile_variable_entry volatile_var;
} VARDATA, *PVARDATA;


typedef struct query_result_object_tag
{
	struct {
		_32 bFailed : 1; // also current_row == INVALID_INDEX
		_32 bFailedBegin : 1; // current_row == INVALID_INDEX cause seek before begin of set
		_32 bFailedEnd : 1; // current_row == INVALID_INDEX cause seek past last of set
	} flags;
   POBJECT_DATA pod; // odbc data that this came from...
   PENTITY pe;  // what entity was created for this data
// array of arrays of pointers to char
	PLIST result_set;
	CTEXTSTR *current_result;
   CTEXTSTR *fields; // have to have a place to put this... so keep it here...
//	struct {
//		int bLastBackwards : 1;
//	} flags;
	int cols; // how many entries are in a (current_result = *result_set)[0-cols]
	int rows; // how many arrays are in result_set[0-rows]
   PTEXT rows_text;
	int current_row;
   PTEXT last_query;
	PLIST vars; // list of volatile variables which we will destroy.. (and remove)
	// this list is indirectly the list of fields, which are only needed from a
   // script standpoint... so keeping the char **fields is still not needed here...
} QUERY_RESULT_OBJECT, *PQUERY_RESULT_OBJECT;


FunctionProto  
   COMMAND
   , LISTDATA
	, SETQUERY
	, QUERY
	, REQUERY
	, CreateQUERY
   , CLOSE
   , HELP
   , ODBC_STORE

	, FIRST_RECORD,LAST_RECORD,NEXT_RECORD,PRIOR_RECORD, ECHO_ERROR
, DUMP
   , SETTYPE;

#define ODBC_TF_DATE   0x80000000
#define ODBC_TF_NUMBER 0x40000000
#define ODBC_TF_TEXT   0x20000000

enum {
   METH_COMMAND
   , METH_LIST
	  , METH_STORE
	  , METH_CreateQUERY
	  , METH_REQUERY
     , METH_SETQUERY
	  , METH_FIRST_RECORD,METH_LAST_RECORD,METH_NEXT_RECORD,METH_PRIOR_RECORD, METH_ECHO_ERROR
	  , METH_DUMP
	  , METH_QUERY
};

#if 0
command_entry methods[]=
          { {DEFTEXT("cmd"),2,7  ,DEFTEXT("Perform ODBC command"),COMMAND}
			 , {DEFTEXT("SHOW")   ,2,4  ,DEFTEXT("List data from database..."),LISTDATA}
			 , {DEFTEXT("STORE") ,2,5  ,DEFTEXT("Store object with vars..."), ODBC_STORE }
			 , {DEFTEXT("QUERY")   ,2,5  ,DEFTEXT("Create result-set object (objname) (sql query...)"),CreateQUERY}
			 , {DEFTEXT("requery")   ,2,7  ,DEFTEXT("reget result-set object (sql query...)"),REQUERY}
			 , {DEFTEXT("setquery")   ,2,8  ,DEFTEXT("setup a query object (sql query...)"),SETQUERY}
			 , {DEFTEXT("first"), 1, 5, DEFTEXT( "go to first record" ), FIRST_RECORD }
			 , {DEFTEXT("last"), 1, 4, DEFTEXT( "go to last record" ), LAST_RECORD }
			 , {DEFTEXT("next"), 1, 4, DEFTEXT( "go to next record" ), NEXT_RECORD }
			 , {DEFTEXT("prior"), 1, 5, DEFTEXT( "go to prior record" ), PRIOR_RECORD }
			 , {DEFTEXT("error"), 1, 5, DEFTEXT( "echo the last error" ), ECHO_ERROR }
			 , {DEFTEXT("dump"), 1, 4, DEFTEXT( "dump all rows of current query result object" ), DUMP }
				 // change last_query, and get query set on current object...
			 , {DEFTEXT("DOQUERY")   ,2,7  ,DEFTEXT("get result-set object (sql query...)"),QUERY}

				 // ,{DEFTEXT("SETTYPE") ,2,7  ,DEFTEXT("Create a 'date' variable..."), SETTYPE }
				 // ,{DEFTEXT("next") ,2,7  ,DEFTEXT(""), SETTYPE }

			 };

PTEXT CPROC getsqlrows( PTRSZVAL psv, PENTITY pe, PTEXT *lastvalue );
PTEXT CPROC getsqlcols( PTRSZVAL psv, PENTITY pe, PTEXT *lastvalue );
#endif
#if 0
volatile_variable_entry vves[] = { { DEFTEXT( "rows" ), getsqlrows, NULL }
											, { DEFTEXT( "cols" ), getsqlcols, NULL }
};
#endif

//---------------------------------------------------------------------------

typedef char ***RESULT;


void CleanTempQueryData( PENTITY pe, PQUERY_RESULT_OBJECT pqro )
{
	{
		RESULT result;
		INDEX idx;
		PVARDATA var;
		LIST_FORALL( pqro->result_set, idx, RESULT, result )
		{
			int n;
			for( n = 0; n < pqro->cols; n++ )
				Release( result[n] );
			Release( result );
			SetLink( &pqro->result_set, idx, 0 );
		}
		pqro->current_result = NULL;
		pqro->rows = 0;
      pqro->cols = 0;
      DeleteList( &pqro->result_set );
      if( pqro->fields )
		{
			int n;
			for( n = 0; pqro->cols; n++ )
				Release( (char*)pqro->fields[n] );
			Release( pqro->fields );
			pqro->fields = NULL;
		}
		LIST_FORALL( pqro->vars, idx, PVARDATA, var )
		{
			//RemoveVolatileVariable( pe, &var->volatile_var );
			Release( var );
			SetLink( &pqro->vars, idx, 0 );
		}
      DeleteList( &pqro->vars );
	}
}

//---------------------------------------------------------------------------

void Destroy( PENTITY pe )
{
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &pe->pPlugin, iODBC );
	if( pod )
	{
      //if( pod->pODBC )
		// hmm... no close yet :)
      // CloseDatabase( pODBC );
		SetLink( &pe->pPlugin, iODBC, NULL );
	}
	{
		PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &pe->pPlugin, iResult );
		if( pqro )
		{
			CleanTempQueryData( pe, pqro );
			// lots of stuff to delete here...
			// all the result data stuff...
			// as try as hard as I can I cannot stop people from pulling
			// the entire database in a single query
			// hopefully I can termiante the odbc connection
         // - destroy runs in what thread?
			Release( pqro );
         SetLink( &pe->pPlugin, iResult, NULL );
		}
	}
   // also check for a iReuslt type - and destroy that data...
	SetLink( &pe->pPlugin, iODBC, 0 );
}


int NEXT_RECORD( PSENTIENT ps, PTEXT parameters )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	if( pqro )
	{
		if( pqro->current_row == INVALID_INDEX )
		{
         // failed for some other reason... cannot recover with this method
			if( !pqro->flags.bFailedBegin )
				return 0;

			// okay prior from failed end, will go to end, and go backwards
			// so we can hit, fail, bounce, etc...
			pqro->current_row = 0; // okay set begin, and go
			pqro->flags.bFailedBegin = FALSE;
			pqro->flags.bFailed = FALSE;
		}
      else
			pqro->current_row++;
		if( pqro->current_row >= pqro->rows )
		{
         pqro->flags.bFailedEnd = TRUE;
         pqro->flags.bFailed = TRUE;
			pqro->current_row = INVALID_INDEX;
			pqro->current_result = NULL;
		}
		else
		{
         pqro->current_result = (CTEXTSTR*)GetLink( &pqro->result_set, pqro->current_row );
			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
	}


	// I think once upon a time I thought about making this result
   // be the true/fall setting for macro success/failure...
	return 0;
}


int PRIOR_RECORD( PSENTIENT ps, PTEXT parameters )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	if( pqro )
	{
		if( pqro->current_row == INVALID_INDEX )
		{
         // failed for some other reason... cannot recover with this method
			if( !pqro->flags.bFailedEnd )
				return 0;

			// okay prior from failed end, will go to end, and go backwards
			// so we can hit, fail, bounce, etc...
			pqro->current_row = pqro->rows;
			pqro->flags.bFailedEnd = FALSE;
			pqro->flags.bFailed = FALSE;
		}

		if( pqro->current_row )
		{
			pqro->current_row--;
         pqro->current_result = (CTEXTSTR*)GetLink( &pqro->result_set, pqro->current_row );
			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
      else
		{
         pqro->flags.bFailedBegin = TRUE;
         pqro->flags.bFailed = TRUE;
			pqro->current_row = INVALID_INDEX;
			pqro->current_result = NULL;
		}
	}


	// I think once upon a time I thought about making this result
   // be the true/fall setting for macro success/failure...
	return 0;
}


int FIRST_RECORD( PSENTIENT ps, PTEXT parameters )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	if( pqro )
	{
      pqro->flags.bFailed = FALSE;
      pqro->flags.bFailedBegin = FALSE;
      pqro->flags.bFailedEnd = FALSE;
		pqro->current_row = 0;
		if( pqro->current_row >= pqro->rows )
		{
			pqro->current_row = INVALID_INDEX;
			pqro->flags.bFailed = TRUE;
			pqro->flags.bFailedBegin = TRUE;
			pqro->flags.bFailedEnd = TRUE;
			pqro->current_result = NULL;
         return 0;
		}
		else
		{
			pqro->current_result = (CTEXTSTR*)GetLink( &pqro->result_set, pqro->current_row );
			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
         return 1;
		}
	}


	// I think once upon a time I thought about making this result
   // be the true/fall setting for macro success/failure...
	return 0;
}


int LAST_RECORD( PSENTIENT ps, PTEXT parameters )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	if( pqro )
	{
      pqro->flags.bFailed = FALSE;
      pqro->flags.bFailedBegin = FALSE;
      pqro->flags.bFailedEnd = FALSE;
		pqro->current_row = pqro->rows;
		if( pqro->current_row )
		{
         pqro->current_row--;
         pqro->current_result = (CTEXTSTR*)GetLink( &pqro->result_set, pqro->current_row );
			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
		else
		{
			pqro->current_row = INVALID_INDEX;
			pqro->flags.bFailed = TRUE;
			pqro->flags.bFailedBegin = TRUE;
			pqro->flags.bFailedEnd = TRUE;
			pqro->current_result = NULL;
		}
	}


	// I think once upon a time I thought about making this result
   // be the true/fall setting for macro success/failure...
	return 0;
}

int DUMP( PSENTIENT ps, PTEXT parameters )
{
   INDEX row;
	INDEX idx;
   _32 *widths;
   // parameters are usless!
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	PVARTEXT pvt = VarTextCreate();

	widths = NewArray( _32, pqro->cols );
	MemSet( widths, 0, sizeof( _32 ) * pqro->cols );
	if( !pqro->fields )
	{
		S_MSG( ps, "Object has null data.");
      return 0;
	}
	for( idx = 0; idx < pqro->cols; idx++ )
	{
      int len;
		if( ( len = strlen( pqro->fields[idx] ) ) > widths[idx] )
         widths[idx] = len;
	}
	for( row = 0; row < pqro->rows; row++ )
	{
      CTEXTSTR *set = (CTEXTSTR*)GetLink( &pqro->result_set, row );
		for( idx = 0; idx < pqro->cols; idx++ )
		{
			int len;
			if( set[idx] && ( len = strlen( set[idx] ) ) > widths[idx] )
				widths[idx] = len;
		}
	}

   for( idx = 0; idx < pqro->cols; idx++ )
		vtprintf( pvt, "%s%*.*s", (!idx)?"":",", widths[idx],widths[idx],pqro->fields[idx] );
	EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
	for( row = 0; row < pqro->rows; row++ )
	{
      CTEXTSTR *set = (CTEXTSTR*)GetLink( &pqro->result_set, row );
		for( idx = 0; idx < pqro->cols; idx++ )
			vtprintf( pvt, "%s%*.*s", (!idx)?"":",",widths[idx],widths[idx],set[idx] );
		EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
	}
   VarTextDestroy( &pvt );
   return 0;
}

int ECHO_ERROR( PSENTIENT ps, PTEXT params )
{
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
	CTEXTSTR result = NULL;
   FetchSQLError( pod->pODBC, &result );
	EnqueLink( &ps->Command->Output, SegCreateFromText( result ) );
	return 0;
}

#if 0
int   SETTYPE( PSENTIENT ps, PTEXT parameters )
{
	return 0;
}
#endif


static PTEXT ObjectVolatileVariableGet( "sql", "rows", "Get number of rows from result query" )
//PTEXT getsqlrows
(  PENTITY pe, PTEXT *lastvalue )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &pe->pPlugin, iResult );
	if( pqro )
      return pqro->rows_text;
   return NULL;
}


static PTEXT ObjectVolatileVariableGet( "sql", "cols", "Get number of cols from result query" )
//PTEXT getsqlcols
( PENTITY pe, PTEXT *lastvalue )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &pe->pPlugin, iResult );
	if( pqro )
	{
		if( !(*lastvalue) )
         (*lastvalue) = SegCreateFromInt( pqro->cols );
		return (*lastvalue);
	}
   return NULL;
}


PTEXT CPROC getsqlvar( PENTITY pe, PTEXT *lastvalue )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &pe->pPlugin, iResult );
   PTRSZVAL psv = 0;
   // the psv parameter is falable.
   PVARDATA pvd = (PVARDATA)psv;
	if( pqro )
	{
      // just a slight sanity check...
		//if( pvd->pqro != pqro )
		{
			//lprintf( "Fatality we're a result from some other query result objecT?!" );
         //exit(0);
		}
		if( *lastvalue )
			Release( *lastvalue );
		(*lastvalue) = NULL;

		// other conditions may exist if this is NULL
		// one may care BOF EOF NULL, etc...
		// consider exporting some const variables also as well as these column names

		if( pqro->current_result )
			(*lastvalue) = SegCreateFromText( pqro->current_result[pvd->iColumn] );
	}

   return *lastvalue;
}


//---------------------------------------------------------------------------------------------

int CPROC QUERY( PSENTIENT ps, PTEXT parameters )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	if( pqro )
	{
		LineRelease( pqro->last_query );
		pqro->last_query = TextDuplicate( parameters, FALSE );
      return REQUERY( ps, parameters );
	}
	else
	{
		// hmm should remove this method from this entity, cause
      // it's not one of these things...
	}
   return 0;
}

//---------------------------------------------------------------------------------------------

int CPROC SETQUERY( PSENTIENT ps, PTEXT parameters )
{
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
	if( !pod && !pqro )
	{
		// to handle this we would need a mysql handle
		// fortunatly, our library has a default one...
      //  create a new pod on default sql? LOL
      lprintf( "Uhh how did we get this method on an object?" );
      return 0;
	}
	if( pqro )
	{
		LineRelease( pqro->last_query );
		pqro->last_query = TextDuplicate( parameters, FALSE );
      return 0;
	}
	else
	{
		// hmm should remove this method from this entity, cause
		// it's not one of these things...
      // well we have a pod - add the query to it...
		// should probably create a query state here...
		{
			PENTITY pe;
			PQUERY_RESULT_OBJECT pqro;
			pod->uses++;
			pqro = New( QUERY_RESULT_OBJECT );
			MemSet( pqro, 0, sizeof( QUERY_RESULT_OBJECT ) );
			pe = ps->Current;
			pqro->last_query = TextDuplicate( parameters, FALSE );
			pqro->pod = pod;
			// need to have a disembodied experience here on this object.
			// one should NEVER do this ! LOL
			// we have to do this, cause in the future this select query will
			// be executed as this object, and not as the database object that we
			// currently are.

			//SetLink( &pe->pPlugin, iODBC, pod );
			SetLink( &pe->pPlugin, iResult, pqro );
			AddLink( &pe->pDestroy, Destroy );

#if 0
			AddMethod( pe, methods + METH_FIRST_RECORD );
			AddMethod( pe, methods + METH_LAST_RECORD );
			AddMethod( pe, methods + METH_NEXT_RECORD );
			AddMethod( pe, methods + METH_PRIOR_RECORD );
			AddMethod( pe, methods + METH_DUMP );
			// do exactly the same query, re-resolving current variables
			AddMethod( pe, methods + METH_REQUERY );
			// change the query
			AddMethod( pe, methods + METH_QUERY );
#endif

			//AddVolatileVariable( pe,  vves, 0 );
			//AddVolatileVariable( pe,  vves+1, 0 );

			// override failure for PS
			// if success/fail cannot test whether there is data
			// at this point, but we will attempt to position the cursor
			// on valid data.
			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
	}
   return 0;
}

//---------------------------------------------------------------------------------------------

int CPROC CreateQUERY( PSENTIENT ps, PTEXT parameters )
{
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
   if( pod && pod->pODBC )
	{
		PTEXT pNewName = GetParam( ps, &parameters );
		if( pNewName )
		{
			// save the query expression
			PTEXT saved_line = TextDuplicate( parameters, FALSE );
			PTEXT tmp_query;
			PTEXT query;
         lprintf( "... %s", GetText( saved_line ) );
			{
				PENTITY pe;
				{
               PQUERY_RESULT_OBJECT pqro;
					INDEX idx;
               CTEXTSTR *current_result;
               PENTITY peTemp;
					pod->uses++;
					pqro = New( QUERY_RESULT_OBJECT );
					MemSet( pqro, 0, sizeof( QUERY_RESULT_OBJECT ) );
					//DebugBreak();
					pe = CreateEntityIn( ps->Current, TextDuplicate( pNewName, TRUE ) );
					pqro->last_query = saved_line;
               pqro->pod = pod;
					// need to have a disembodied experience here on this object.
					// one should NEVER do this ! LOL
					// we have to do this, cause in the future this select query will
					// be executed as this object, and not as the database object that we
               // currently are.
					peTemp = ps->Current;
               ps->Current = pe;
					tmp_query = MacroDuplicate( ps, saved_line );
               ps->Current = peTemp;
					query = BuildLine( tmp_query );
					LineRelease( tmp_query );
					{
                  lprintf( "Query is %s", GetText( query ) );
					}
					if( SQLRecordQuery( pod->pODBC
											, GetText( query )
											, &pqro->cols
											, &current_result
											, &pqro->fields ) )
					{
                  LineRelease( query );
						//pe = CreateEntityIn( ps->Current, TextDuplicate( pNewName, TRUE ) );
						if( !pe )
						{
							lprintf( "We're fucked." );
                     exit(0);
						}
                  lprintf( "Created an entity, have a query..." );
                  SetLink( &pe->pPlugin, iODBC, pod );
						SetLink( &pe->pPlugin, iResult, pqro );
						AddLink( &pe->pDestroy, Destroy );
						if( pqro->fields )
						{
							// steal a copy of these fields, cause they go away after another query.
							int n;
							CTEXTSTR *myfields = NewArray( CTEXTSTR, pqro->cols + 1 );
							for( n = 0; pqro->fields[n]; n++ )
								myfields[n] = StrDup( pqro->fields[n] );
							myfields[n] = NULL;
							pqro->fields = myfields;
						}
						for( ; current_result; FetchSQLRecord( pod->pODBC, &current_result ) )
						{
                     int n;
							pqro->current_result = NewArray( CTEXTSTR, pqro->cols );
							for( n = 0; n < pqro->cols; n++ )
                        pqro->current_result[n] = StrDup( current_result[n] );
							AddLink( &pqro->result_set, pqro->current_result );
                     pqro->rows++; // another result row available;
                     pqro->current_result = NULL; // clear this out, so fetch doesn't try to delete it.
						}
						pqro->rows_text = SegCreateFromInt( pqro->rows );
 						for( idx = 0; idx < pqro->cols; idx++ )
						{
							// create volatile varaible entry thing...
							PVARDATA pvd = New( VARDATA );
							MemSet( pvd, 0, sizeof( VARDATA ) );
							// add a new variable to query result_object for later deletion
							AddLink( &pqro->vars, pvd );
							pvd->iColumn = idx;
							// setup the volatile variable itself
							if( pqro->fields )
							{
								pvd->volatile_var.pName = pqro->fields[idx];
								//SetTextSize( &pvd->volatile_var.pName, strlen( pqro->fields[idx] ) );
								//pvd->volatile_var.name.flags = TF_STATIC; // don't delete this name.
								pvd->volatile_var.get = getsqlvar;
								//pvd->pqro = pqro;
                        lprintf( "NEED ADD VOLATILE VARIABLE - DYNAMIC!" );
						/////		AddVolatileVariable( pe, &pvd->volatile_var, (PTRSZVAL)pvd );
							}
						}
						{
#if 0
							AddMethod( pe, methods + METH_FIRST_RECORD );
							AddMethod( pe, methods + METH_LAST_RECORD );
							AddMethod( pe, methods + METH_NEXT_RECORD );
							AddMethod( pe, methods + METH_PRIOR_RECORD );
							AddMethod( pe, methods + METH_DUMP );
                     // do exactly the same query, re-resolving current variables
							AddMethod( pe, methods + METH_REQUERY );
                     // change the query
							AddMethod( pe, methods + METH_QUERY );
							AddVolatileVariable( pe,  vves, 0 );
							AddVolatileVariable( pe,  vves+1, 0 );
							{
								// also add a set of odbc command to this ...
                        // so that it can do things like create result records...
								AddMethod( pe, methods + METH_LIST );
								AddMethod( pe, methods + METH_COMMAND );
                        // create a new query result object...
								AddMethod( pe, methods + METH_CreateQUERY );
								AddMethod( pe, methods + METH_ECHO_ERROR );
							}
#endif
							//UnlockAwareness( CreateAwareness( pe ) ); // not really doing anything special with this...
						}
						{
                     PENTITY peTemp = ps->Current;
							ps->Current = pe;
							FIRST_RECORD( ps, NULL );
                     ps->Current = peTemp;
						}
						// override failure for PS
						// if success/fail cannot test whether there is data
						// at this point, but we will attempt to position the cursor
                  // on valid data.
						if( ps->CurrentMacro )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;

					}
					else
					{
                  DestroyEntity( pe );
						if( !ps->CurrentMacro )
						{
                     ECHO_ERROR( ps, NULL );
						}
						Release( pqro );
						LineRelease( query );
                  // the SQL error result should be available here...
					}
				}
			}
		}
	}
	return 0;
}

//---------------------------------------------------------------------------------------------

int CPROC REQUERY( PSENTIENT ps, PTEXT parameters )
{
	//POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
	PQUERY_RESULT_OBJECT pqro = (PQUERY_RESULT_OBJECT)GetLink( &ps->Current->pPlugin, iResult );
   if( pqro )
	{
		PTEXT tmp_query = MacroDuplicate( ps, pqro->last_query );
		PTEXT query = BuildLine( tmp_query );
		LineRelease( tmp_query );
		{
			lprintf( "REQuery is %s", GetText( query ) );
		}
		if( query )
		{
			PENTITY pe;
			{
				CTEXTSTR *current_result;
				INDEX idx;
				{
					RESULT result;
					INDEX idx;
					PVARDATA var;
					LIST_FORALL( pqro->result_set, idx, RESULT, result )
					{
						int n;
						for( n = 0; n < pqro->cols; n++ )
							Release( result[n] );
						Release( result );
						SetLink( &pqro->result_set, idx, 0 );
					}
					pqro->current_result = NULL;
					pqro->rows = 0;
					pqro->cols = 0;
				}
				//CleanTempQueryData( ps->Current, pqro );
				if( SQLRecordQuery( pqro->pod->pODBC, GetText( query )
										, &pqro->cols
										, &current_result
										, &pqro->fields ) )
				{
					LineRelease( query );
					pe = ps->Current;
               /*
					if( pqro->fields )
					{
						// steal a copy of these fields, cause they go away after another query.
						int n;
						CTEXTSTR *myfields = Allocate( sizeof( CTEXTSTR ) * pqro->cols + 1 );
						for( n = 0; pqro->fields[n]; n++ )
							myfields[n] = StrDup( pqro->fields[n] );
						myfields[n] = NULL;
						pqro->fields = myfields;
						}
                  */
					for( ; current_result; FetchSQLRecord( pqro->pod->pODBC, &current_result ) )
					{
						int n;
						pqro->current_result = NewArray( CTEXTSTR, pqro->cols );
						for( n = 0; n < pqro->cols; n++ )
							pqro->current_result[n] = StrDup( current_result[n] );
						AddLink( &pqro->result_set, pqro->current_result );
						pqro->rows++; // another result row available;
						pqro->current_result = NULL; // clear this out, so fetch doesn't try to delete it.
					}
					pqro->rows_text = SegCreateFromInt( pqro->rows );
               // columns for these fields already exist from the prior /query or /setquery
					//for( idx = 0; idx < pqro->cols; idx++ )
					{
						// create volatile varaible entry thing...
						//PVARDATA pvd = Allocate( sizeof( VARDATA ) );
						//MemSet( pvd, 0, sizeof( VARDATA ) );
						// add a new variable to query result_object for later deletion
						//AddLink( &pqro->vars, pvd );
						//pvd->iColumn = idx;
						// setup the volatile variable itself
						//if( pqro->fields )
						//{
						//	strcpy( GetText( (PTEXT)&pvd->volatile_var.name ), pqro->fields[idx] );
						//	SetTextSize( &pvd->volatile_var.name, strlen( pqro->fields[idx] ) );
						 //  pvd->volatile_var.name.flags = TF_STATIC; // don't delete this name.
						 //  pvd->volatile_var.get = getsqlvar;
							//pvd->pqro = pqro;
							//AddVolatileVariable( pe, &pvd->volatile_var, (PTRSZVAL)pvd );
					  // }
					}
					FIRST_RECORD( ps, NULL );
					if( ps->CurrentMacro )
						ps->CurrentMacro->state.flags.bSuccess = TRUE;
				}
				else
				{
					if( !ps->CurrentMacro )
					{
						ECHO_ERROR( ps, NULL );
					}
					Release( pqro );
					LineRelease( query );
					// the SQL error result should be available here...
				}
			}
		}
	}
	return 0;
}

//---------------------------------------------------------------------------

int CPROC ODBC_STORE( PSENTIENT ps, PTEXT parameters )
{
#if 0
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
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
#endif
	return 0;
}

//---------------------------------------------------------------------------

int CPROC LISTDATA( PSENTIENT ps, PTEXT parameters )
{
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
	PVARTEXT pvt = NULL;
   {
      PTEXT pWhat;
      if( !pod || !pod->pODBC )
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
				// includes the table to list... therefore list the fields in the table.
				// SQLShowTable( GetText( pWhich )
            // if( !) GetError
				{
					CTEXTSTR *pResult = NULL;
					int nColumns;
					char cmd[256];
					// specific method of mysql to do this...
					// there's another way - through the odbc API and the results
               // are different...
					snprintf( cmd, sizeof( cmd ), "show columns from %s", GetText( pWhich ) );
					if( SQLRecordQuery( pod->pODBC, cmd, &nColumns, &pResult, NULL ) )
						for( ;
							 pResult;
							  FetchSQLRecord( pod->pODBC, &pResult ) )
						{
							vtprintf( pvt, "%32s %s", pResult[0], pResult[1] );
							EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
						}
            }
         }
         else
			{
				CTEXTSTR *result = NULL;
				for( SQLRecordQuery( pod->pODBC, "show tables", NULL,&result, NULL );
					 result;
					 FetchSQLRecord( pod->pODBC, &result ) )
				{
					EnqueLink( &ps->Command->Output
								, SegCreateFromText( result[2] ) );
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
	if( pvt )
      VarTextDestroy( &pvt );
   return 0;
}

//---------------------------------------------------------------------------

int CPROC COMMAND( PSENTIENT ps, PTEXT parameters )
{
	POBJECT_DATA pod = (POBJECT_DATA)GetLink( &ps->Current->pPlugin, iODBC );
	PTEXT pCommand;
   /*
   {
   	DECLTEXT( msg, "Command to be issued!" );
   	EnqueLink( &ps->Command->Output, &msg );
		}
		*/
   PTEXT tmp = MacroDuplicate( ps, parameters );
	pCommand = BuildLine( tmp );
   LineRelease( tmp );
   if( !SQLCommand( pod->pODBC, GetText( pCommand ) ) )
	{
      CTEXTSTR error;
      DECLTEXT( msg, "ODBC Command excecution failed(1)...." );
		EnqueLink( &ps->Command->Output, &msg );
		FetchSQLError( pod->pODBC, &error );
		if( error )
		{
			EnqueLink( &ps->Command->Output, SegCreateFromText( error ) );
		}
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
   LineRelease( pCommand );
	return 0;
}


//---------------------------------------------------------------------------

int CPROC Create( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	// init the ODBC object...
	// take parameters as if "OPEN"
	POBJECT_DATA pod;
   PTEXT pDSN;
	pod = New( OBJECT_DATA );
   MemSet( pod, 0, sizeof( OBJECT_DATA ) );
	AddLink( &pe->pDestroy, Destroy );

   if( pDSN = GetParam( ps, &parameters ) )
   {  
		if( pod->pODBC = ConnectToDatabase( GetText( pDSN ) ) )
		{
			SetLink( &pe->pPlugin, iODBC, pod );
			//pID = GetParam( ps, &parameters );
			//pPassword = GetParam( ps, &parameters );
		}
      else
		{
         CTEXTSTR result;
			DECLTEXT( msg, "Failed to open ODBC Connection...." );
			EnqueLink( &ps->Command->Output, &msg );
			// no connection available.. guess NULL to FetchSQLError
         // also results in the same thing...
			GetSQLError( &result );
			EnqueLink( &ps->Command->Output, SegCreateFromText( result ) );
			return -1;
      }
		//ps2 = CreateAwareness( pe );
#if 0
      AddMethod( pe, methods + METH_LIST );
      AddMethod( pe, methods + METH_COMMAND );
      //AddMethod( pe, methods + METH_CREATE );
      //AddMethod( pe, methods + METH_STORE );
		AddMethod( pe, methods + METH_CreateQUERY );
		AddMethod( pe, methods + METH_ECHO_ERROR );
#endif
		//UnlockAwareness( ps2 );
   }
   else
   {
      DECLTEXT( msg, "Open reqires <database DSN> <user> <password>..." );
      EnqueLink( &ps->Command->Output, &msg );
      return -1;
   }

	return 0;
}


PUBLIC( char *, RegisterRoutines )( void )
{
   RegisterObject( "sql", "Generic ODBC Object... parameters determine database", Create );
   // not a real device type... but need the ID...
   //myTypeID = RegisterDevice( "odbc", "ODBC Database stuff", NULL );
	iODBC = RegisterExtension( "SQL" );
   // result data type...
   iResult = RegisterExtension( "SQL Result" );
	return DekVersion;
}


PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   UnregisterObject( "sql" );
   //UnregisterRoutine( "odbc" );
}


