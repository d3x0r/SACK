#include <stdhdrs.h>
#include <stdio.h>
#include <sack_types.h>
#include <network.h>
#include <sharemem.h>
#include <filesys.h>
#include <sqlgetoption.h>

TEXTCHAR *SystemPrefix;


void ProcessINIFile( CTEXTSTR filename, TEXTCHAR *pData, _32 nData )
{
	TEXTCHAR *section = NULL;
	TEXTCHAR *entry = NULL;
	//	int start_line = TRUE;
	int ftnsys = 0;
	int find_nextline = 0;
	TEXTCHAR *p = (TEXTSTR)pathrchr( (CTEXTSTR)filename );
	TEXTCHAR filebuf[256];
	if( p )
		filename = p + 1;

	if( SystemPrefix )
	{
		static TEXTCHAR tmpbuf[256];
		snprintf( tmpbuf, sizeof( tmpbuf ), WIDE("%s/%s"), SystemPrefix, filename );
		filename = tmpbuf;
	}

	if( StrCaseCmp( filename, WIDE("ftnsys.ini") ) == 0 )
      ftnsys = 1;

	while( nData && pData[0] )
	{
		if( find_nextline )
		{
			if( !pData[0] || pData[0] == '\n' )
			{
				find_nextline = 0;
			}
			pData++;
			nData--;
			continue;
		}

		switch( pData[0] )
		{
		case '[' :
			{
				TEXTCHAR *sec = pData + 1;
				while( sec[0] && sec[0] != ']' )
					sec++;
				if( sec[0] != ']' )
					break;
				sec[0] = 0;
				if( section ) Release( section );
  				section = StrDup( pData + 1 );
				nData -= ( sec - pData ) + 1;
				pData = sec + 1;
				find_nextline = 1;
			}
			break;
		case ';':
			find_nextline = 1;
			break;
		default:
			// collect as an option...
			{
				TEXTCHAR *optname = pData;
				TEXTCHAR *optval = optname;
				TEXTCHAR *optend;
				while( optval[0] && optval[0] != '=' )
				{
					if( optval[0] == '\r' || optval[0] == '\n' )
					{
						find_nextline = TRUE;
						break;
					}
					optval++;
				}
				if( optval[0] && optval[0] == '=' )
				{
					optval[0] = 0;
					optval++;
					while( optval[-1] == ' ' )
					{
						optval[-1] = 0;
						optval--;
					}

					if( entry ) Release( entry );
					entry = StrDup( optname );
					optend = optval;
					while( optend[0] && optend[0] != '\r' && optend[0] != '\n' )
						optend++;
					if( optend[0] )
					{
						optend[0] = 0;
						lprintf( WIDE("SQLWrite: (%s)[%s] %s=%s"), ftnsys?"NULL":filename, section,entry,optval );
						if( ftnsys )
							SACK_WriteProfileString( section, entry, optval );
						else
							SACK_WritePrivateProfileString( section, entry, optval, filename );
						//DebugDumpMem();
					}
					pData = optend + 1;
					nData -= ( optend - pData ) + 1;
				}
				find_nextline = 1;
			}
		}
	}
}

#define BYTEOP_DECODE(n)  ((((_16)(n))-20) & 0xFF)

void CRYPTODecryptMemory( POINTER mem, _32 size)
{
    _32 i;
	 unsigned char *ptr = (unsigned char *)mem;
	 if( !mem )
	 {
		 return;
	 }
    for (i=0; i<size; i++)
	 {
        *ptr = BYTEOP_DECODE(*ptr);
        ptr++;
    }
}


//void CPROC ReadINIFile( PTRSZVAL psv, char *filename, int flags )
void CPROC ReadINIFile( PTRSZVAL psv,  CTEXTSTR filename, int flags )
{
	FILE *handle;
	handle = sack_fopen( 0, filename, WIDE("rb") );
	printf( WIDE("Process file: %s\n"), filename );
   if( handle )
	{
      _32 nsize;
		fseek( handle, 0L, SEEK_END );
		nsize = ftell( handle );
		fseek( handle, 0L, SEEK_SET );
      if( nsize )
		{
			char *filedata = NewArray( char, nsize + 1);
         TEXTSTR unicode_data;
			filedata[nsize] = 0; // make sure it's null terminated :)
			/*allocate memory*/
			fread( filedata, 1, nsize, handle );
         if( filedata[0] != '[' )
				CRYPTODecryptMemory ( filedata, nsize );
#ifdef UNICODE
			unicode_data = DupCharToText( filedata );
			ProcessINIFile( filename, unicode_data, nsize );
#else
			ProcessINIFile( filename, filedata, nsize );
#endif
			Release( filedata );
	   }
		fclose( handle );
	}
}

int main( int argc, char **argv )
{
	void *info = NULL;
	TEXTCHAR path[256];
	int n;
	if( argc > 1 )
	{
		if( StrCmp( DupCharToText( argv[1] ), WIDE("-s") ) == 0 )
		{
			static TEXTCHAR tmp[256];
			if( argc > 2 )
			{
				snprintf( tmp, sizeof( tmp ), WIDE("/System Settings/%s"), argv[2] );
				argc--;
            argv++;
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), WIDE("/System Settings/%s"), GetSystemName() );
			}
			SystemPrefix = tmp;
		}
	}
	//CreateOptionDatabase();
   BeginBatchUpdate();
   for( n = 1; n < argc; n++ )
	{
		//cpg27dec2006  		char *mask, *rootpath;
      TEXTCHAR *mask, *rootpath;
		StrCpyEx( path, DupCharToText( argv[n] ), 256 );
		mask = (TEXTSTR)pathrchr( path );
		rootpath = path;
		if( mask ) { mask[0] = 0; mask++; }
		else { rootpath = WIDE("."); mask = path; }
		while( ScanFiles((CTEXTSTR)  rootpath, (CTEXTSTR) mask, &info, ReadINIFile, 0, 0 ) )
		{
			//_32 a,b,c,d;
			//GetMemStats( &a, &b, &c, &d );
			//lprintf( "stats: %d %d %d %d", a, b, c, d );
         //DebugDumpMem();
		}
	}
   EndBatchUpdate();
   return 0;//cpg27dec2006
}

//cpg27dec2006 c:\work\altanik\src\fut\getoption\importini\importini.c(145): Warning! W107: Missing return value for function 'main'
