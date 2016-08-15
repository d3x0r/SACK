
#ifndef DECOMPRESS_STRUCT_DEFINED
typedef uintptr_t PDECOMPRESS;
#endif


// pass address of compressor - after this pCompressor will be NULL.
void DestroyDecompressor( PDECOMPRESS *ppCompressor );

