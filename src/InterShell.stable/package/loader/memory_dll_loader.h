

POINTER ScanLoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, LOGICAL (CPROC*Callback)(CTEXTSTR library) );
POINTER LoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, LOGICAL (CPROC *Callback)(CTEXTSTR library) );
POINTER GetExtraData( POINTER block );
