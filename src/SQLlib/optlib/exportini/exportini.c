#include <stdhdrs.h>
#include <network.h>
#include <stdio.h>
#include <sack_types.h>
#include <sharemem.h>
#include <filesys.h>
#include <sqlgetoption.h>

TEXTCHAR *SystemPrefix;
PVARTEXT pvtIni;
PVARTEXT pvtSection;

#define BYTEOP_ENCODE(n)  ((((_16)(n))+20) & 0xFF)

void CRYPTOEncryptMemory( POINTER mem, size_t size)
{
    _32 i;
	 unsigned char *ptr = (unsigned char *)mem;
	 if( !mem )
	 {
		 return;
	 }
    for (i=0; i<size; i++)
	 {
        *ptr = BYTEOP_ENCODE(*ptr);
        ptr++;
    }
}

typedef struct list_fill_tag
{
	struct {
		_32 bSecondLevel : 1;
	} flags;
	int nLevel;
} LISTFILL, *PLISTFILL;

FILE *output;

int CPROC FillList( PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
{
   PLISTFILL plf = (PLISTFILL)psv;
	LISTFILL lf = *plf;
	lf.nLevel++;
   lf.flags.bSecondLevel = 1;
	lprintf( WIDE("%d - %s (%p)"), plf->nLevel, name, ID );
	{
		if( lf.nLevel == 1 )
		{
			if( pvtIni )
			{
				PTEXT pINI = VarTextGet( pvtIni );
				CRYPTOEncryptMemory( GetText( pINI ), GetTextSize( pINI ) );
				if( output )
				{
					fwrite( GetText(pINI), 1, GetTextSize( pINI ), output );
               LineRelease( pINI );
					fclose( output );
					output = NULL;
				}
				VarTextDestroy( &pvtIni );
			}
         // if it's this branch, then don't auto export it.
			if( StrCmp( name, WIDE("INI Store") ) == 0 )
            return TRUE;
			pvtIni = VarTextCreate();
			output = sack_fopen( 0, name, WIDE("wt") );
		}
		else if( lf.nLevel == 2 )
		{
			lprintf( WIDE("Section %s"), name );
			vtprintf( pvtSection, WIDE("%s"), name );
		}
		else if( lf.nLevel >= 3 )
		{
			POPTION_TREE_NODE ID_Value = GetOptionValueIndex( ID );
			if( ID_Value )
			{
				TEXTCHAR buffer[256];
				if( (int)GetOptionStringValue( ID_Value, buffer, sizeof( buffer ) ) > 0 )
				{
					if( VarTextPeek( pvtSection ) )
					{
						PTEXT section =VarTextGet( pvtSection );
						vtprintf( pvtIni, WIDE("[%s]\n"), GetText( section ) );
						LineRelease( section );
					}
					vtprintf( pvtIni, WIDE("%s=%s\n"), name, buffer );
				}
				else
				{
					vtprintf( pvtSection, WIDE("/%s"), name );
				}
			}
			else
			{
				vtprintf( pvtSection, WIDE("/%s"), name );
			}
		}
	}
  	EnumOptions( ID, FillList, (PTRSZVAL)&lf );
   //lprintf( WIDE("done with all children under this node.") );
   return TRUE;
}

int main( int argc, TEXTCHAR **argv )
{
	POPTION_TREE_NODE id_root = NULL;
	pvtSection = VarTextCreate();
	if( argc > 1 )
	{
		if( strcmp( argv[1], WIDE("-s") ) == 0 )
		{
			static TEXTCHAR tmp[256];
			if( argc > 2 )
			{
				SystemPrefix = argv[2];
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), WIDE("INI Store/%s"), GetSystemName() );
				SystemPrefix = tmp;
			}
			id_root = GetOptionIndex( id_root, WIDE("DEFAULT"), SystemPrefix, NULL );
			if( !id_root )
				return 0;
		}
	}

	{
		LISTFILL lf;
      MemSet( &lf, 0, sizeof( lf ) );
		EnumOptions( id_root, FillList, (PTRSZVAL)&lf );
	}
	{
		PTEXT pINI = VarTextGet( pvtIni );
		CRYPTOEncryptMemory( GetText( pINI ), GetTextSize( pINI ) );
		if( output )
		{
			fwrite( GetText(pINI), 1, GetTextSize( pINI ), output );
			LineRelease( pINI );
			fclose( output );
			output = NULL;
		}
	}
   return 0;
}

