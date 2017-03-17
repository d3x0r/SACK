#ifndef SYSTEM_SOURCE
#define SYSTEM_SOURCE
#endif

#include <sack_system.h>
#include <sharemem.h>

SACK_SYSTEM_NAMESPACE

SYSTEM_PROC( void, ParseIntoArgs )( TEXTCHAR *lpCmdLine, int *pArgc, TEXTCHAR ***pArgv )
{
	TEXTCHAR *args = lpCmdLine;
	TEXTCHAR  *p;
	TEXTCHAR **pp;
   TEXTCHAR argc; // result variable, count is a temp counter...
   TEXTCHAR **argv; // result variable, pp is a temp pointer
	TEXTCHAR quote = 0;
   int escape = 0;
	int count = 0;
	int lastchar;

	lastchar = ' '; // auto continue spaces...
	//lprintf( WIDE("Got args: %s"), args );
	p = args;
	while( p && p[0] )
	{
		//lprintf( WIDE("check character %c %c"), lastchar, p[0] );
		if( escape ) {
			if( p[0] == '\"' || p[0] == '\'' ) {
				escape = 0;
            count++;
			}
			else {
				escape = 0;
            count += 2;
			}
		}
		else if( p[0] == '\\' ) {
			escape = 1;
         count++;
		}
		else if( quote )
		{
			if( p[0] == quote )
			{
				count++;
				quote = 0;
				lastchar = ' ';
			}
		}
		else
		{
			if( p[0] == '\"' || p[0] == '\'' )
				quote = p[0];
			else
			{
				if( lastchar != ' ' && p[0] == ' ' ) // and there's a space
				{
					count++;
				}
				else if( lastchar == ' ' && p[0] != ' ' )
				{
				}
			}
			lastchar = p[0] ;
		}
		p++;
	}
	if( quote )
		count++; // complete this argument
	else if( p != args )
      count++;
	if( count )
	{
		TEXTCHAR *start;
		lastchar = ' '; // auto continue spaces...
      //lprintf( "Array is %d (+2?)", count );
		pp = argv = NewArray( TEXTCHAR*, count + 2 );
		argc = count - 2;
		p = args;
		quote = 0;
		count = 0;
		//pp[count++] = StrDup( pTask->pTask ); // setup arg to equal program (needed for linux stuff)
		start = NULL;
		while( p[0] )
		{
			//lprintf( WIDE("check character %c %c"), lastchar, p[0] );
			if( escape ) {
				escape = 0;
			}
			else if( p[0] == '\\' ) {
				escape = 1;
			}
			else if( quote )
			{
				if( !escape ) {
					if( !start )
						start = p;

					if( p[0] == quote )
					{
						p[0] = 0;
						pp[count++] = StrDup( start );
						p[0] = quote;
						quote = 0;
						start = NULL;
						lastchar = ' ';
					}
				}
			}
			else
			{
				if( !escape ) {
					if( p[0] == '\"' || p[0] == '\'' )
						quote = p[0];
					else
					{
						if( lastchar != ' ' && p[0] == ' ' ) // and there's a space
						{
							p[0] = 0;
							pp[count++] = StrDup( start );
							start = NULL;
							p[0] = ' ';
						}
						else if( lastchar == ' ' && p[0] != ' ' )
						{
							if( !start )
								start = p;
						}
					}
					lastchar = p[0] ;
				}
			}
			p++;
		}
		//lprintf( WIDE("Setting arg %d to %s"), count, start );
		if( start )
			pp[count++] = StrDup( start );
		pp[count] = NULL;

      if( pArgc )
			(*pArgc) = count;
      if( pArgv )
			(*pArgv) = argv;
	}
	else
	{
      if( pArgc )
			(*pArgc) = 0;
		if( pArgv )
		{
			(*pArgv) = NewArray( TEXTCHAR*, 1 );
			(*pArgv)[0] = NULL;
		}
	}
}

SACK_SYSTEM_NAMESPACE_END

