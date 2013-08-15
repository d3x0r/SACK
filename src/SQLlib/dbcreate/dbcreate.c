/************************************************
 * dbcreate.c Cliff                             *
 * Modified Last: 5/15/2006                     *
 * this function parses a sql create script and *
 * Creates the Appropriate MySQL DataTables     *
 * The application then creates the appropriate *
 * header files and C files for a complete      *
 * dump or build				*
 ************************************************/
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <string.h>
#include <filesys.h>
#include <sack_types.h>
#include <typelib.h>
#include <logging.h>
#include <sharemem.h>
#include <network.h>
#include <stdhdrs.h>
#include <winsock.h>
#include <configscript.h>
#include <pssql.h>

extern int CreateDBHeaderFile( CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, _32 options );

//PTABLE GetFieldsInSQL( char *cmd)
void main(void)
{
    // read in a file of table names
    FILE *file;
    file = fopen( "x.MySQL", "rt" );
    if( file)
    {
		 char tablename[256];
       //SetAllocateDebug( TRUE );
		 SetSystemLoggingLevel( LOG_NOISE-1);

        SystemLogTime( SYSLOG_TIME_CPU|SYSLOG_TIME_DELTA );
        while( fgets( tablename, sizeof( tablename ), file ) )
        {
            // open a sql create table file
            tablename[strlen(tablename)-1] = 0; // squash the \n
            CreateTableEx( "debug.MySQL", tablename, tablename, CTO_MATCH );
				CreateDBHeaderFile( "debug.MySQL", tablename, tablename, CTO_MATCH );
				{
					_32 blocks, free, used, chunks, freechunks;
					GetMemStats( &free, &used, &chunks, &freechunks);
					xlprintf(LOG_NOISE-1)( "MemStats: free %ld used %ld  chunks %ld  freechunks %ld"
												, free, used, chunks, freechunks );
            //   DebugDumpMem();
				}
        }
    }
    else
    {
        lprintf("Attempt to open table names script failed!");
    }
}
