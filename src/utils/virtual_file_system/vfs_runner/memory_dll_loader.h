
POINTER ScanLoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, LOGICAL (CPROC*Callback)(CTEXTSTR library) );
POINTER LoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, LOGICAL (CPROC *Callback)(CTEXTSTR library) );
// this was moved to VFS core
//POINTER GetExtraData( POINTER block );
