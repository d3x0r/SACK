
#ifndef DECOMPRESS_STRUCT_DEFINED
typedef PTRSZVAL PDECOMPRESS;
#endif


// pass address of compressor - after this pCompressor will be NULL.
void DestroyDecompressor( PDECOMPRESS *ppCompressor );

