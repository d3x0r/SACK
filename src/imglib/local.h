#include <imglib/fontstruct.h>

IMAGE_NAMESPACE

// can also return a reload of the image?
// returns internal image data; used to create the data if it doesn't exist on reload texture....
// mostly just a event called by draw routines to non IF_FLAG_FINAL_RENDER images
void CPROC MarkImageUpdated( Image child_image );
Image AllocateCharacterSpaceByFont( Image target_image, SFTFont font, PCHARACTER character );

void CPROC cplot( ImageFile *pi, int32_t x, int32_t y, CDATA c );
void CPROC cplotraw( ImageFile *pi, int32_t x, int32_t y, CDATA c );
void CPROC cplotalpha( ImageFile *pi, int32_t x, int32_t y, CDATA c );
CDATA CPROC cgetpixel( ImageFile *pi, int32_t x, int32_t y );
void CPROC do_linec( ImageFile *pImage, int32_t x, int32_t y
                            , int32_t xto, int32_t yto, CDATA color );
void CPROC do_lineAlphac( ImageFile *pImage, int32_t x, int32_t y
                            , int32_t xto, int32_t yto, CDATA color );
void CPROC do_lineExVc( ImageFile *pImage, int32_t x, int32_t y
                            , int32_t xto, int32_t yto, CDATA color
                            , void (*func)( ImageFile*pif, int32_t x, int32_t y, int d ) );
void CPROC do_hlinec( ImageFile *pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color );
void CPROC do_vlinec( ImageFile *pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color );
void CPROC do_hlineAlphac( ImageFile *pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color );
void CPROC do_vlineAlphac( ImageFile *pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color );


IMAGE_NAMESPACE_END
