#include <imglib/fontstruct.h>

IMAGE_NAMESPACE

// can also return a reload of the image?
// returns internal image data; used to create the data if it doesn't exist on reload texture....
// mostly just a event called by draw routines to non IF_FLAG_FINAL_RENDER images
void CPROC MarkImageUpdated( Image child_image );
Image AllocateCharacterSpaceByFont( Image target_image, SFTFont font, PCHARACTER character );

void CPROC cplot( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c );
CDATA CPROC cgetpixel( ImageFile *pi, S_32 x, S_32 y );
void CPROC do_linec( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
void CPROC do_lineAlphac( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
void CPROC do_lineExVc( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( ImageFile*pif, S_32 x, S_32 y, int d ) );
void CPROC do_hlinec( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
void CPROC do_vlinec( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
void CPROC do_hlineAlphac( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
void CPROC do_vlineAlphac( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );


IMAGE_NAMESPACE_END
