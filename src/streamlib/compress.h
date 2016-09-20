
#ifndef COMPRESS_STRUCT_DEFINED
typedef uintptr_t PCOMPRESS;
#endif

// pass address of compressor - after this pCompressor will be NULL.
void DestroyCompressor( PCOMPRESS *ppCompressor );


