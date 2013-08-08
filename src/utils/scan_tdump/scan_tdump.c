#include <stdio.h>

#include <sack_types.h>  // for purposes of WIDE()

char names[2560][16];
int nNames;

char modules[512][16];
int nModules;

struct {
	int nModule;
	char name[64];
   int number;
} exports[12000];
int nExports;

struct {
	int nModule;
	char name[16];
}refs[3000];
int nRefs;


int main( void )
{
	char buf[256];

	while( fgets( buf, 256, stdin ) )
	{
		char *p = buf;
		int len = strlen( p );
		while( p[len-1] <= 32 ) len--;
		p[len] = 0;

		while( p[0] == ' ' ) p++;

		if( strncmp( p, WIDE("Module Name: '"), 14 ) == 0 )
		{
         char *end;
			p+=14;
			end = p;
			while( end[0] != '\'' )
				end++;
         end[0] = 0;
			{
				int n;
				for( n = 0; n < nModules; n++ )
				{
					if( stricmp( modules[n], p ) == 0 )
                  break;
				}
				if( n == nModules )
				{
					printf( WIDE("Module: %s(%d)\n"), p, nModules );
					strcpy( modules[nModules], p );
               nModules++;
				}

			}

		}
		if( strncmp( p, WIDE("Name: "), 6 )== 0 )
		{
         char *name, *ord;
			p += 6;
			name = p;
			ord = name;

			while( ord[0] != ' ' ) ord++;
			ord[0] = 0; // terminate the name.
			ord++;
			while( ord[0] == ' ' ) ord++;
			if( strncmp( ord, WIDE("Entry:"), 6 ) == 0 )
			{
				// sucess...
				ord += 6;
				while( ord[0] == ' ' ) ord++;
				{
					int n;
					for( n = 0; n < nExports; n++ )
					{
						if( ( exports[n].nModule == nModules-1 ) &&
							( stricmp( exports[n].name, name ) == 0 ) )
                     break;
					}
					if( n == nExports )
					{
						strcpy( exports[n].name, name );
						exports[n].nModule = nModules-1;
                  if( ord[0] == '0' )
							sscanf( ord, WIDE("%X"), &exports[n].number  );
                  else
							exports[n].number = atoi( ord );
						printf( WIDE("Export: %s=%d\n"), exports[n].name,exports[n].number );
                  nExports++;
					}
				}
			}
		}

		if( strncmp( p, WIDE("PTR"), 3 )== 0 )
		{
			while( p[0] != 'h' ) p++;
			p++;
			while( p[0] == ' ' || p[0] == '\t' ) p++;
			if( strnicmp( p, WIDE("kernel"), 6 ) == 0 )
            continue;
			if( strnicmp( p, WIDE("gdi."), 4 ) == 0 )
            continue;
			if( strnicmp( p, WIDE("user."), 5 ) == 0 )
            continue;
			if( strnicmp( p, WIDE("VER."), 4 ) == 0 )
            continue;
			if( strnicmp( p, WIDE("winsock."), 8 ) == 0 )
            continue;
			if( strnicmp( p, WIDE("0002h:"), 6 ) == 0 )
            continue;
			if( strnicmp( p, WIDE("0001h:"), 6 ) == 0 )
            continue;
			{
				int n;
				for( n = 0; n < nRefs; n++ )
				{
					if( stricmp( refs[n].name, p ) == 0 )
                  break;
				}
				if( n == nRefs )
				{
					printf( WIDE("Reference: %s\n"), p );
               refs[nRefs].nModule = nModules-1;
					strcpy( refs[nRefs].name, p );
               nRefs++;
				}

			}
		}

      if(0)
		if( strncmp( p, WIDE("Module  "), 6 ) == 0 )
		{
         int n = 0;
			p+=6;
			while( p[0] && p[0] != ':' )
			{
				if( p[0] >= '0' && p[0] <= '9' )
               n = n*10 +( p[0] - '0');
				p++;
			}
			if( n && p[0] )
			{
				p++;
            while( p[0] == ' ' )p++;
				if( strncmp( WIDE("kernel"), p, 6 ) &&
					strncmp( WIDE("gdi"), p, 3) &&
					strncmp( WIDE("user"), p, 4) )
				{
					{
						int n;
						for( n = 0; n < nNames; n++ )
						{
							if( stricmp( names[n], p ) == 0 )
								break;
						}
						if( n == nNames )
						{
							printf( WIDE("copy %%SOURCEPATH%%\\%s.dll\n"), p );
							strcpy( names[n], p );
                     nNames++;
						}
					}
				}
			}


		}
	}

	{
		int n;
		for( n = 0; n < nRefs; n++ )
		{
			char tmp[64];
			char *dot;
         sprintf( tmp, refs[n].name );
			dot = tmp;
			while( dot[0] && dot[0] != '.' ) dot++;
			if( dot[0] )
			{
				int m, o;
				int nRef = atoi( dot + 1);
            dot[0] = 0;
				for( m = 0; m < nModules; m++ )
				{
					if( stricmp( modules[m], tmp ) == 0 )
                  break;
				}
				if( m == nModules )
				{
					printf( WIDE("FATAL: FAILED TO FIND %s\n"), tmp );
				}
				for( o = 0; o < nExports; o++ )
				{
					if( exports[o].nModule == m &&
						exports[o].number == nRef )
					{
                  break;
					}
				}
				if( o == nExports )
				{
					printf( WIDE("Failed to resolve %s in %s\n"), refs[n].name, modules[refs[n].nModule] );
				}
			}
		}
	}

	printf( WIDE("exports: %d of 12000\n"), nExports );
printf( WIDE("refs: %d of 3000\n"), nRefs );
printf( WIDE("modules: %d of 512\n"), nModules );
		 printf( WIDE("names: %d of 2560\n"), nNames );
}

