

#include <stdhdrs.h>
#include <filesys.h>

void CPROC ProcessFile( PTRSZVAL psvUser, CTEXTSTR name, int flags )
{
	static TEXTCHAR cmd[512];
	snprintf( cmd, sizeof( cmd ), "wstrip %s", name );
#ifdef __cplusplus
	::
#endif
		system( cmd );
}

int main( void )
{
   POINTER data = NULL;
	while( ScanFiles( ".", "*.dll|*.isp|*.isom|*.exe|*.nex|*.core", &data, ProcessFile, SFF_SUBCURSE, 0 ) );
   return 0;
}
