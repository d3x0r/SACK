/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *  Define operations on Images
 *    Image image = MakeImageFile(width,height);
 *    UnmakeImageFile( image );
 *    ... and draw on it inbetween I suppose...
 *  consult doc/image.html
 *
 */

#ifdef IMAGE_MAIN
#define BLOT_EXTERN
#else
#define BLOT_EXTERN extern
#endif

ASM_IMAGE_NAMESPACE
extern unsigned char AlphaTable[256][256];
extern unsigned char ScalarAlphaTable[256][256];
_32 DOALPHA( _32 over, _32 in, _8 a );
ASM_IMAGE_NAMESPACE_END
 
#ifdef __cplusplus
namespace sack {
	namespace image {
		extern "C" {
#endif


#define CLAMP(n) (((n)>255)?(255):(n))
#ifdef NEED_ALPHA2
static _32 _XXr, _XXg, _XXb, aout, atmp, atmp2;
#endif
#ifdef USE_OPENGL_COMPAT_COLORS
#define DOALPHA2( over, in, a ) (  (atmp=(a)),                                             \
(!(atmp))?(over):((atmp)>=255)?((in)| 0xFF000000UL):(               \
   (atmp2=256U-atmp),(atmp++),(aout = ((_32)AlphaTable[atmp][AlphaVal( over )]) << 24),                                \
   (_XXr = (((RedVal(in))   *(atmp)) + ((RedVal(over))  *((atmp2)))) >> 8 ),         \
   (_XXg = (((GreenVal(in)) *(atmp)) + ((GreenVal(over))*((atmp2)))) >> 8 ),            \
   (_XXb = (((BlueVal(in))  *(atmp)) + ((BlueVal(over)) *((atmp2)))) >> 8 ),         \
   (aout|(CLAMP(_XXb)<<16)|(CLAMP(_XXg)<<8)|(CLAMP(_XXr)))))
#else
#define DOALPHA2( over, in, a ) (  (atmp=(a)),                                             \
(!(atmp))?(over):((atmp)>=255)?((in)| 0xFF000000UL):(               \
   (atmp2=256U-atmp),(atmp++),(aout = ((_32)AlphaTable[atmp][AlphaVal( over )]) << 24),                                \
   (_XXr = (((RedVal(in))   *(atmp)) + ((RedVal(over))  *((atmp2)))) >> 8 ),         \
   (_XXg = (((GreenVal(in)) *(atmp)) + ((GreenVal(over))*((atmp2)))) >> 8 ),            \
   (_XXb = (((BlueVal(in))  *(atmp)) + ((BlueVal(over)) *((atmp2)))) >> 8 ),         \
   (aout|(CLAMP(_XXr)<<16)|(CLAMP(_XXg)<<8)|(CLAMP(_XXb)))))
#endif

#define SHADEPIXEL(pixel, c ) ( ( ( ( ( (pixel)&0xFF ) * ((c) & 0xFF) ) >> 8 ) & 0xFF )                \
   				       | ( ( ( ( ( ((pixel)>>8)&0xFF ) * (((c)>>8) & 0xFF) ) >> 8 ) & 0xFF ) << 8 )     \
	   			       | ( ( ( ( ( ((pixel)>>16)&0xFF ) * (((c)>>16) & 0xFF) ) >> 8 ) & 0xFF ) << 16 ) \
					   | ( (pixel) & 0xFF000000 ) )

#define INVERTPIXEL(pixel ) ( ( 255 - ( (pixel)&0xFF ) )           \
   				       | ( ( 255 - ( ((pixel)>>8)&0xFF ) ) << 8 )   \
	   			       | ( ( 255 - ( ((pixel)>>16)&0xFF ) ) << 16 ) \
                      | ( (pixel) & 0xFF000000 ) )

// need to declere these local for multishade to work
//CDATA rout,gout,bout;
#ifdef USE_OPENGL_COMPAT_COLORS
#define MULTISHADEPIXEL( pixel,r,g,b) 	AColor(             \
			   ((rout = ( ( ( ( BlueVal(pixel) ) * (RedVal(b)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( GreenVal(pixel) ) * (RedVal(g)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( RedVal(pixel) ) * (RedVal(r)+1) ) >> 8 ) & 0xFF )),\
				(( rout > 255 ) ?255:rout)),\
  				((gout = ( ( ( (BlueVal(pixel) ) * (GreenVal(b)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( (GreenVal(pixel )) * (GreenVal(g)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( (RedVal(pixel) ) * (GreenVal(r)+1) ) >> 8 ) & 0xFF )),\
  				(( gout > 255 )?255:gout )),\
				((bout = ( ( ( ( BlueVal(pixel) ) * (BlueVal(b)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( GreenVal(pixel) ) * (BlueVal(g)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( RedVal(pixel) ) * (BlueVal(r)+1) ) >> 8 ) & 0xFF )),\
  				( ( bout > 255 )?255:bout ) ),                                       \
	((_32)AlphaTable[AlphaVal(pixel)][                                                                 \
 	   ScalarAlphaTable[(RedVal(pixel)?AlphaVal(r):255)][ScalarAlphaTable[(( ((pixel)>>16)&0xFF )?AlphaVal(b):255)][(( ((pixel)>>8)&0xFF )?AlphaVal(g):255) ] ] ] ) )
#else
#define MULTISHADEPIXEL( pixel,r,g,b) 	AColor(             \
			   ((rout = ( ( ( ( (pixel) & 0xFF ) * ((((b)>>16) & 0xFF)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( ((pixel)>>8)&0xFF ) * ((((g)>>16) & 0xFF)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( ((pixel)>>16)&0xFF ) * ((((r)>>16) & 0xFF)+1) ) >> 8 ) & 0xFF )),\
				(( rout > 255 ) ?255:rout)),\
  				((gout = ( ( ( ( (pixel) & 0xFF ) * ((((b)>>8) & 0xFF)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( ((pixel)>>8)&0xFF ) * ((((g)>>8) & 0xFF)+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( ((pixel)>>16)&0xFF ) * ((((r)>>8) & 0xFF)+1) ) >> 8 ) & 0xFF )),\
  				(( gout > 255 )?255:gout )),\
				((bout = ( ( ( ( (pixel) & 0xFF ) * (( (b) & 0xFF )+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( ((pixel)>>8)&0xFF ) * (( (g) & 0xFF )+1) ) >> 8 ) & 0xFF )\
				       + ( ( ( ( ((pixel)>>16)&0xFF ) * (( (r) & 0xFF )+1) ) >> 8 ) & 0xFF )),\
  				( ( bout > 255 )?255:bout ) ), \
	((_32)AlphaTable[AlphaVal(pixel)][                                                                 \
 	   ScalarAlphaTable[(RedVal(pixel)?AlphaVal(r):255)][ScalarAlphaTable[(BlueVal(pixel)?AlphaVal(b):255)][(GreenVal(pixel)?AlphaVal(g):255) ] ] ]) )
#endif

//-----------------------------------------------------------
//-----------------------------------------------------------
//-----------------------------------------------------------
#define SCALED_BLOT_WORK_PARAMS  PCDATA po, PCDATA  pi       \
						    , int i_errx, int i_erry          \
						    , _32 wd, _32 hd                \
				          , _32 dwd, _32 dhd                \
				          , _32 dws, _32 dhs                \
				          , S_32 oo, S_32 srcpwidth


#ifdef __cplusplus
} // extern "C"
}//namespace sack {
}//	namespace image {
#endif
// $Log: blotproto.h,v $
// Revision 1.7  2003/08/20 13:59:13  panther
// Okay looks like the C layer blotscaled works...
//
// Revision 1.6  2003/08/20 08:07:12  panther
// some fixes to blot scaled... fixed to makefiles test projects... fixes to export containters lib funcs
//
// Revision 1.5  2003/07/25 00:08:31  panther
// Fixeup all copyies, scaled and direct for watcom
//
// Revision 1.4  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
