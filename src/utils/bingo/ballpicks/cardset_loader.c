
#include <stdhdrs.h>


void LoadCardset( char *name, int *card_count, char **result_faces )
{
	char *ext = strchr(name, '.' );
	char alt_name[128];
	char *result;
	int f = 0;
	DWORD dwSize = 0;
	snprintf( alt_name, sizeof( alt_name ), "%*.*s.raw", ext-name,ext-name,name );
	result = OpenSpace( NULL, alt_name, &dwSize );
	if( !result || !dwSize )
	{
		FILE *file = fopen( name, "rb" );
		if( file )
		{
			char card[12];
			uint32_t length;
			fseek( file, 0, SEEK_END );
			length = ftell( file );
			fseek( file, 0, SEEK_SET );
			result = NewArray( char, 25 * ( length / 12 ) );
			while( fread( card, 1, 12, file ) )
			{
				int d;
				int n = 0;
				for( d = 0; d < 25; d++ )
				{
					if( d != 12 )
					{
						result[f*25 + d] = ((card[n/2] >> ( (!( n & 1 )) * 4) )&0xF) + ( (d / 5)*15)+1;
						n++;
					}
					else
						result[f*25 + d] = 0;
				}
            f++;
			}
			fclose( file );
			file = fopen( alt_name, "wb" );
			if( file )
			{
				fwrite( result, f, 25, file );
				fclose( file );
			}
		}
	}
	else
		f = dwSize / 25;
	(*result_faces) = result;
	(*card_count) = f;
	//return result;
}

