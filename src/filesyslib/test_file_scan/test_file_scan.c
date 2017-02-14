
#include <stdhdrs.h>
#include <filesys.h>

static void CPROC DumpScan( uintptr_t user, CTEXTSTR name, int flags ) {
	printf( "%s %s %s\n", name, (flags&SFF_DIRECTORY)?"<DIR>":"", (flags&SFF_DRIVE)?"<DRIVE>":"" );
}

static void usage( argc, argv ) {
	printf( "%s -s -d -n -i -p -v[VFS mount name] [path]\n", argv[0] );
	printf( "    Parses path and separates the leading path from the filemask\n" );
	printf( "    then scans the specified path with the specified mask.  If no path part\n" );
   printf( "    then uses current directory and scans for the file mask\n" );
	printf( "    -s   Recurse into subdirectories\n" );
	printf( "    -d   Return directory names also\n" );
	printf( "    -n   Name only - don't return the base path as part of the name\n" );
	printf( "    -i   Ignore case when matching mask name\n" );
	printf( "    -p   Subpath only; return partial path from the base name to current\n" );
   printf( "    -v   specifies a virtual filesystem mount name to use instead of native filesystem\n" );
}

SaneWinMain( argc, argv ) {
	int argofs = 1;
	int flags = 0;
   char *mount;
	if( argc < 2 ) {
		usage( argc, argv );
      return 0;
	}
	{
		while( argofs < argc ) {
			if( argv[argofs][0] == '-' ) {
				switch( argv[argofs][1] ) {
				case 's': flags |= SFF_SUBCURESE; break;
				case 'd': flags |= SFF_DIRECTORIES; break;
				case 'n': flags |= SFF_NAMEONLY; break;
				case 'i': flags |= SFF_IGNORECASE; break;
				case 'p': flags |= SFF_SUBPATHONLY; break;
				case 'v': if( argv[argofs][2] ) { mount = argv[argofs] + 2; break; }
				else { argofs++; mount = argv[argofs] };
            break;
				}
            argofs++;
			}
			else
            break;
		}
	}
   if( argofs < argc )
	{
      void *info = NULL;
		char *path = argv[argofs];
		char *mask = (char*)pathrchr( (char*)path );
		struct file_system_mounted_interface *fsmi = mount?sack_get_mounted_filesystem( mount ):NULL;
		if( !mask )
			while( ScanFilesEx( NULL, path, &info, DumpScan, flags, 0, 0, fsmi ) );
		else {
			mask[0] = 0; mask++;
			while( ScanFilesEx( path, mask, &info, DumpScan, flags, 0, 0, fsmi ) );

		}
	} else {
      printf( "Error, failed to find path/file name to process" );
	}
}
EndSaneWinMain()
