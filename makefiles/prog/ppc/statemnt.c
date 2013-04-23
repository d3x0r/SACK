

#include "global.h"

typedef struct statement_tag
{
	struct {
		unsigned int bStartOfLine : 1;
	} flags;
	char file[__MAX_PATH__];
	int line;
	PTEXT content;
   struct statement_tag *next;
} STATEMENT, *PSTATEMENT;



static PSTATEMENT statement;

static PSTATEMENT current_statement;
static PTEXT current_word;

int CompleteStatement( void )
{
	// returns when the statement is complete...
	// there may be data laying about afterwards...
	if( !g.statement )
	{
		g.statment = Allocate( sizeof( STATEMENT ) );
		GetCurrentFileLine( g.statement->name, &g.statement->line );

	}
   return FALSE;
}

PTEXT GetInitializer( void )
{
   // collects a single initializer....
}


PTEXT GetToken( void )
{
   static PTEXT result;
   PTEXT word;
	if( !statement )
		return NULL;
   if( !current_statement )
		current_statement = statement;
	LineRelease( result );

}


