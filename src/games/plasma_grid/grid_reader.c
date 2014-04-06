#include <stdhdrs.h>
#include <filesys.h>
#include <vectlib.h>

struct grid_reader
{
	CTEXTSTR filename;

	POINTER file_data;
	size_t file_size;
	size_t file_stride;
	size_t bytes;
};

struct grid_reader *GridReader_Open( CTEXTSTR filename_base )
{
	TEXTCHAR filename[256];
	struct grid_reader *reader = New( struct grid_reader );
	reader->filename = StrDup( filename_base );

	tnprintf( filename, 256, WIDE( "%s.gri" ), reader->filename );
	reader->file_size = 0;
	reader->file_data = OpenSpace( NULL, filename, &reader->file_size );

	/* need to read other files .grd and .vrt */
	reader->file_stride = 13896;
	reader->bytes = 2;
	
	/* reader->file_rows = 3012; */

	return reader;
}

RCOORD *GridReader_Read( struct grid_reader *reader, int x, int y, int w, int h )
{
	RCOORD *result = NewArray( RCOORD, w*h );
	S_16 value;
	RCOORD *tmp;
	int row, col;
	int min = 99999;
	int max = 0;

	if( x < 0 )
		x = 0;
	if( y < 0 ) 
		y = 0;
	if( (y+h) > 2992 )
		h = 2992 - y;
	for( col = x; col < (x+w); col++ )
		for( row = y; row < (y+h); row++ )
		{
			value = ((PS_16)reader->file_data)[ col + row * reader->file_stride/2 ];
			if( value == -9999 )
				value = 0;
			result[ ( col - x ) + ( row - y ) * w ] = (RCOORD)value;
			if( value < min )
				min = value;
			if( value > max )
				max = value;
		}
		lprintf( "min max %d,%d", min, max );
	tmp  = result;
	for( row = 0; row < (+h); row++ )
		for( col = 0; col < (+w); col++ )
		{
			(*tmp) = ( (*tmp) - min ) / ( max - min );
			tmp++;
		}
	return result;
}


