/************************************************
 * SQLParse Jim and Cliff                       *
 * Modified Last: 2/25/2006                     *
 * this function parses a sql create script and *
 * Creates the Appropriate MySQL DataTables     *
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

//PTABLE GetFieldsInSQL( char *cmd)
void main(void)
{
    // read in a file of table names
    FILE *file;
    file = fopen( "x.MySQL", "rt" );
    if( file)
    {
        char tablename[256];

        while( fgets( tablename, sizeof( tablename ), file ) )
        {
            // open a sql create table file
            tablename[strlen(tablename)-1] = 0; // squash the \n
            CreateTableEx( "mkdb.mysql", tablename, tablename, CTO_MATCH );
        }
    }
    else
    {
        lprintf("Attempt to open table names script failed!");
    }
}

