

PTRSZVAL CPROC OpenCompressor( PCAPTURE_DEVICE pDevice, _32 width, _32 height );
int CPROC CompressFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice );

PTRSZVAL CPROC OpenDecompressor( PCAPTURE_DEVICE pDevice, _32 width, _32 height );
int CPROC DecompressFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice );



PTRSZVAL CPROC OpenNetworkRender( char *name );
int CPROC RenderNetworkFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice );

PTRSZVAL CPROC OpenNetworkCapture( char *name );
int CPROC GetNetworkCapturedFrame( PTRSZVAL psv, PCAPTURE_DEVICE pDevice );

