
ASM_IMAGE_NAMESPACE
_32 DOALPHA( _32 over, _32 in, _8 a );
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE

void  CPROC cSetColorAlpha( PCDATA po, int oo, int w, int h, CDATA color );
void  CPROC cSetColor( PCDATA po, int oo, int w, int h, CDATA color );

Image GetInvertedImage( Image child_image );
Image GetShadedImage( Image child_image, CDATA red, CDATA green, CDATA blue );


IMAGE_NAMESPACE_END

