#include <stdhdrs.h>
#include <stdio.h>
#include <sqlgetoption.h>

// Enumeration callback; lists one level under the node being enumerated.  Entries which carry a
// string value are printed as "name = value", branches are printed with a trailing '/' so they
// can be descended into with another -e.
static int CPROC EnumOption( uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
{
	TEXTCHAR *value = NULL;
	size_t valuelen = 0;
	(void)psv;
	(void)flags;
	if( !name || StrCmp( name, "." ) == 0 )
		return TRUE; // internal self-reference entry
	// GetOptionStringValue hands back an internal (rotating) buffer - do not release it.
	if( ID && (int)GetOptionStringValue( ID, &value, &valuelen ) > 0 && value )
		printf( "%s = %.*s\n", name, (int)valuelen, value );
	else
		printf( "%s/\n", name );
	return TRUE;
}

SaneWinMain( argc, argv )
{
	int argn = 1;
	int bRead = 0;
	int bEnum = 0;
	if( argn < argc && argv[argn][0] == '-' )
	{
		if( argv[argn][1] == 'd' ) { bRead = 1; argn++; }
		else if( argv[argn][1] == 'e' ) { bEnum = 1; argn++; }
	}
	// root_name and path are always needed; option only when not enumerating; value only on write.
	if( ( argc - argn ) < ( bEnum ? 2 : ( bRead ? 3 : 4 ) ) )
	{
		printf( "Usage: %s [-d|-e] [root_name] [path] [option] [value]\n", argv[0] );
		printf( "   -d reads the value instead of setting the value.\n" );
		printf( "   -e enumerates the options/branches at [path] (no [option] needed).\n" );
		printf( "   root_name = \"flashdrive.ini\" (if \"\" then option will go under DEFAULT)\n" );
		printf( "   path = \"some/path/long quoted path name\"\n" );
		printf( "          \"\"  enumerates from the profile root (under DEFAULT/<program>)\n" );
		printf( "          \"/\" enumerates from the absolute root (INI file level)\n" );
		printf( "   option = \"option name to set\"\n" );
		printf( "   value = \"value of the option\"\n" );
		return 0;
	}
	{
		CTEXTSTR root = argv[argn];
		CTEXTSTR path = argv[argn + 1];
		CTEXTSTR ini  = root[0] ? root : NULL;
		if( bEnum )
		{
			POPTION_TREE_NODE node = NULL; // NULL == the absolute root of the option tree
			if( path[0] == '/' )
			{
				// absolute: outside the DEFAULT/<program> level.  "/" alone is the root itself.
				if( path[1] )
					node = GetOptionIndex( NULL, ini ? ini : "DEFAULT", path, NULL );
			}
			else if( path[0] )
			{
				// relative: the same resolution a normal profile read uses, so this lists what
				// SACK_GetProfileString( path, ... ) would see.
				node = GetOptionIndexEx( NULL, ini, path, NULL, FALSE, FALSE DBG_SRC );
			}
			// else "" - profile root; NULL parent enumerates from the top.
			if( !node && path[0] && !( path[0] == '/' && !path[1] ) )
			{
				printf( "No such path: %s\n", path );
				return 0;
			}
			EnumOptions( node, EnumOption, 0 );
		}
		else if( bRead )
		{
			// No default is supplied and none is stored - a missing option reads as absent
			// rather than being created.
			TEXTCHAR *buffer = NULL;
			size_t buflen = 0;
			if( SACK_ReadPrivateProfileString( path, argv[argn + 2], &buffer, &buflen, ini ) )
			{
				printf( "%.*s\n", (int)buflen, buffer );
				Release( buffer );
			}
		}
		else
		{
			if( ini )
				SACK_WritePrivateProfileString( path, argv[argn + 2], argv[argn + 3], ini );
			else
				SACK_WriteProfileString( path, argv[argn + 2], argv[argn + 3] );
		}
	}
	return 0;
}
EndSaneWinMain()
