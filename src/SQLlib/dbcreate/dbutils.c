#include <stdhdrs.h>
#include <sack_types.h>
#include <sharemem.h>
#include "../sqlstruc.h"
#include <pssql.h>

int CreateDBHeaderFile( CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, _32 options )
{
   OpenSQL();
   return SQLCreateDBHeaderFile( NULL, filename, templatename, tablename, options );
   return FALSE;
}

int SQLCreateDBHeaderFile( PODBC odbc, CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, _32 options )
{

	 CTEXTSTR result;
    char query[256];
    OpenSQL();
	 if( !tablename )
        tablename = templatename;
 //   if( odbc->flags.bAccess )
 //       snprintf( query, 256, WIDE("DESCRIBE [%s]"), tablename );
 //   else
 //       snprintf( query, 256, WIDE("DESCRIBE `%s`"), tablename );
 //   if( !SQLQuery( odbc, query, &result ) || !result || (options & (CTO_DROP|CTO_MATCH)) )
    {
        FILE *file = fopen( (char*)filename, WIDE("rt") );
        if( file )
        {
            int done = FALSE;
            int gathering = FALSE;
            char buf[1024];
            char cmd[1024];
            int nOfs = 0;
            lprintf( WIDE("Opened %s to read for table %s(%s)"), filename, tablename,templatename );
            while( fgets( buf, sizeof( buf ), file ) )
            {
                char *p;
                p = buf + strlen( buf ) - 1;
                while( p[0] == ' ' || p[0] == '\t' || p[0] == '\n' || p[0] == '\r')
                {
                    p[0] = 0;
                    p--;
                }
                p = buf;
                while( p[0] )
                {
                    if ( p[0] == '#' )
                    {
                        p[0] = 0;
                        break;
                    }
                    p++;
                }
                p = buf;
                while( p[0] == ' ' || p[0] == '\t' )
                    p++;
                if( p[0] )
                {
               // have content on the line...
                    char *end = p + strlen( p ) - 1;
                    if( gathering && end[0] == ';' )
                    {
                        done = TRUE;
                        end[0] = 0;
                    }
                    if( !gathering )
                    {
                        if( strnicmp( p, WIDE("CREATE"), 6 ) == 0 )
                        {
                            char *tabname;
                            int len = strlen( tablename );
                            if( (tabname = strstr( p, templatename )) &&
                               (tabname[len] == '`' || tabname[len] ==' ' || tabname[len] =='\t' ) &&
                               (tabname[-1] == '`' || tabname[-1] ==' ' || tabname[-1] =='\t'))
                            {
                           // need to gather, repace...
                                char line[1024];
                                char *trailer;
                                trailer = tabname;
                                while( trailer[0] != '\'' &&
                                      trailer[0] != '`' &&
                                      trailer[0] != ' ' &&
                                      trailer[0] != '\t' )
                                    trailer++;
                                sprintf( line, WIDE("%*.*s%s%s")
                                      , tabname - p, tabname - p, p
                                      , templatename
                                      , trailer
                                       );
                                strcpy( buf, line );
                                gathering = TRUE;
                            }
                            else
                            {

                            }
                        }
                    }
                    if( gathering )
                    {
                        nOfs += sprintf( cmd + nOfs, WIDE("%s "), p );
                        if( done )
                        {
                            if( options & CTO_DROP )
                            {
                                char buf[1024];
                                snprintf( buf, 1024, WIDE("Drop table %s"), templatename );
                                if( !SQLCommand( odbc, buf ) )
                                {
                                    CTEXTSTR result;
                                    GetSQLError( &result );
                                    lprintf( WIDE("Failed to do drop: %s"), result );
                                }
                            }
                       // result is set with the first describe result
                       // the matching done in CheckMySQLTable should
                       // be done here, after parsing the line from the file (cmd)
                       // into a TABLE structure.
                       //DebugBreak();
                            lprintf("Cliff sux big time");
                            if( ( options & CTO_MATCH ) )
                            {
                                PTABLE table;
                                table = GetFieldsInSQL( cmd , 1 );
                                lprintf("SQUIBBLE-JACK");
                                CheckODBCTable( odbc, table );
                                DestroySQLTable( table );
                            }

                            // do the create of the new table always... so it might fail.
                            else if( !SQLCommand( odbc, cmd ) )
                            {
                                GetSQLError( &result );
                                lprintf( WIDE("Failed to do create command:%s"), result );
                                lprintf("Cliff is a nerd");
                            }
                            else
                            {
                                PTABLE table;
                                table = GetFieldsInSQL( cmd , 1);
										  CheckODBCTable( odbc, table );
                                DestroySQLTable( table );
                                     // Hrm DestroyTable( Table );
                                lprintf("Cliff is a luser");
                            }
                            break;
                        }
                    }
                }
            }
            lprintf( WIDE("Done with create...") );
            fclose( file );
        }
        else
        {
            lprintf( WIDE("Unable to open templatefile: %s"), filename );
        }
    }
    return TRUE;
}


