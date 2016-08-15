#include <stdio.h>
#include <sharemem.h>
#include <filesys.h>
#include <procreg.h>


void CPROC ParseINI( uintptr_t psv, char *name, int flags )
{
	FILE *in = fopen( name, WIDE("rt") );
	if( in )
	{
		char line[1024];
      char section[128];
		char *filename = (char*)pathrchr( name );
		if( filename )
			filename++;
		else
         filename = name;
		while( fgets( line, sizeof( line ), in ) )
		{
			int len = strlen( line );
			if( line[len-1] == '\n' )
			{
				line[len-1] = 0;
				len--;
			}
			if( line[0] == '[' )
			{
				char *end = strchr( line, ']' );
				if( end )
				{
					end[0] = 0;
               strcpy( section, line + 1 );
				}
			}
			else if( line[0] == ';' )
			{
            // commented line...
			}
			else
			{
				char *val = strchr( line, '=' );
				if( val )
				{
               char *trim = val;
               char classname[1024];
					val[0] = 0;
					val++;
               while( val[0] == ' ' || val[0] == '\t' ) val++;
					trim--;
					while( trim[0] == ' ' || trim[0] == '\t' ) trim--;
					trim++;
					trim[0] = 0;
					// double slashes for section name...
               // filename will be without a slash, and so is INI...
					{
						char *p = section;
						while( p[0] )
						{
							if( p[0] == '/' || p[0] == '\\' )
							{
								char *c = p;
								while( c[0] ) c++;
                        c++;
								while( c != p )
								{
									c[0] = c[-1];
                           c--;
								}
								p++;
							}
                     p++;
						}
					}
               sprintf( classname, WIDE("INI/%s/%s"), filename, section );
               RegisterValue( classname, line, val );
				}
			}
		}
      fclose( in );
	}
}


int main( void )
{
	void *info = NULL;
	uint32_t free, used, chuncks, freechunks;
   SetSystemLog( SYSLOG_FILE, stdout );
	printf( WIDE("Lets see... ini files...") );
	while( ScanFiles( WIDE("."), WIDE("*.ini"), &info, ParseINI, 0, 0 ) );
	GetMemStats( &free, &used, &chuncks, &freechunks );
	printf( WIDE("Memory result : free:%ld used:%ld chuncks:%ld freechunks:%ld\n")
			, free, used, chuncks, freechunks );
	DumpRegisteredNames();
	SaveTree();
	LoadTree();
	DumpRegisteredNames();
   return 0;
}
