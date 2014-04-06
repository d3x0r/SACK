

// pass just the file base (and path)
// the extensions will be applied dynamically becuase it's a file set.
struct grid_reader *GridReader_Open( CTEXTSTR filename_base );

// read a section of the file into real coords
// resluting stride will be equal to width specified.
RCOORD *GridReader_Read( struct grid_reader *reader, int x, int y, int w, int h );
