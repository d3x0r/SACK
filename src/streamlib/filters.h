

uintptr_t CPROC OpenCompressor( PCAPTURE_DEVICE pDevice, uint32_t width, uint32_t height );
int CPROC CompressFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice );

uintptr_t CPROC OpenDecompressor( PCAPTURE_DEVICE pDevice, uint32_t width, uint32_t height );
int CPROC DecompressFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice );



uintptr_t CPROC OpenNetworkRender( char *name );
int CPROC RenderNetworkFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice );

uintptr_t CPROC OpenNetworkCapture( char *name );
int CPROC GetNetworkCapturedFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice );

