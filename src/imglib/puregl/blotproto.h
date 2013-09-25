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
#define DOALPHA2( over, in, a ) (  (atmp=(a)),                                             \
(!(atmp))?(over):((atmp)>=255)?((in)| 0xFF000000UL):(               \
   (atmp2=256U-atmp),(atmp++),(aout = ((_32)AlphaTable[atmp][AlphaVal( over )]) << 24),                                \
   (_XXr = (((RedVal(in))   *(atmp)) + ((RedVal(over))  *((atmp2)))) >> 8 ),         \
   (_XXg = (((GreenVal(in)) *(atmp)) + ((GreenVal(over))*((atmp2)))) >> 8 ),            \
   (_XXb = (((BlueVal(in))  *(atmp)) + ((BlueVal(over)) *((atmp2)))) >> 8 ),         \
   (aout|(CLAMP(_XXr)<<16)|(CLAMP(_XXg)<<8)|(CLAMP(_XXb)))))                                            \
   

#define SHADEPIXEL(pixel, c ) ( ( ( ( ( (pixel)&0xFF ) * ((c) & 0xFF) ) >> 8 ) & 0xFF )                \
   				       | ( ( ( ( ( ((pixel)>>8)&0xFF ) * (((c)>>8) & 0xFF) ) >> 8 ) & 0xFF ) << 8 )     \
	   			       | ( ( ( ( ( ((pixel)>>16)&0xFF ) * (((c)>>16) & 0xFF) ) >> 8 ) & 0xFF ) << 16 ) )

// need to declere these local for multishade to work
//CDATA rout,gout,bout;
#ifdef _OPENGL_DRIVER
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
	((_32)ScalarAlphaTable[AlphaVal(pixel)][                                                                 \
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
	((_32)ScalarAlphaTable[AlphaVal(pixel)][                                                                 \
 	   ScalarAlphaTable[(RedVal(pixel)?AlphaVal(r):255)][ScalarAlphaTable[(BlueVal(pixel)?AlphaVal(b):255)][(GreenVal(pixel)?AlphaVal(g):255) ] ] ]) )
#endif

//-----------------------------------------------------------

void  CPROC cCopyPixelsT0( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs );   
void  CPROC cCopyPixelsT1( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs );   
void  CPROC cCopyPixelsTA( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent ); 
void  CPROC cCopyPixelsTImgA( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent ); 
void  CPROC cCopyPixelsTImgAI( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent ); 


void CPROC asmCopyPixelsT0( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs );   
void CPROC asmCopyPixelsT1( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs );   
void CPROC asmCopyPixelsTA( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent ); 
void CPROC asmCopyPixelsTImgA( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent ); 
void CPROC asmCopyPixelsTImgAI( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent ); 

void CPROC asmCopyPixelsT0MMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs );      
void CPROC asmCopyPixelsT1MMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs );      
void CPROC asmCopyPixelsTAMMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent );    
void CPROC asmCopyPixelsTImgAMMX( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent );    
void CPROC asmCopyPixelsTImgAIMMX( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent ); 

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

void CPROC cCopyPixelsShadedT0( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA shade );   
void CPROC cCopyPixelsShadedT1( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA shade );   
void CPROC cCopyPixelsShadedTA( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA shade ); 
void CPROC cCopyPixelsShadedTImgA( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA shade ); 
void CPROC cCopyPixelsShadedTImgAI( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA shade ); 

void CPROC asmCopyPixelsShadedT0( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA shade );   
void CPROC asmCopyPixelsShadedT1( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA shade );   
void CPROC asmCopyPixelsShadedTA( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA shade ); 
void CPROC asmCopyPixelsShadedTImgA( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA shade ); 
void CPROC asmCopyPixelsShadedTImgAI( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA shade ); 

void CPROC asmCopyPixelsShadedT0MMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs      
								  , CDATA shade );      
void CPROC asmCopyPixelsShadedT1MMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs      
								  , CDATA shade );      
void CPROC asmCopyPixelsShadedTAMMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent      
								  , CDATA shade );    
void CPROC asmCopyPixelsShadedTImgAMMX( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent      
								  , CDATA shade );    
void CPROC asmCopyPixelsShadedTImgAIMMX( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent      
								  , CDATA shade ); 

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

void CPROC cCopyPixelsMultiT0( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA rShade, CDATA gShade, CDATA bShade );   
void CPROC cCopyPixelsMultiT1( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA rShade, CDATA gShade, CDATA bShade );   
void CPROC cCopyPixelsMultiTA( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 
void CPROC cCopyPixelsMultiTImgA( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 
void CPROC cCopyPixelsMultiTImgAI( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 


void CPROC asmCopyPixelsMultiT0( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA rShade, CDATA gShade, CDATA bShade );   
void CPROC asmCopyPixelsMultiT1( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , CDATA rShade, CDATA gShade, CDATA bShade );   
void CPROC asmCopyPixelsMultiTA( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 
void CPROC asmCopyPixelsMultiTImgA( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 
void CPROC asmCopyPixelsMultiTImgAI( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi      
								  , _32 ws, _32 hs      
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 

void CPROC asmCopyPixelsMultiT0MMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs      
								  , CDATA rShade, CDATA gShade, CDATA bShade );      
void CPROC asmCopyPixelsMultiT1MMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs      
								  , CDATA rShade, CDATA gShade, CDATA bShade );      
void CPROC asmCopyPixelsMultiTAMMX( PCDATA po, PCDATA  pi       
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade );    
void CPROC asmCopyPixelsMultiTImgAMMX( PCDATA po, PCDATA  pi    
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade );    
void CPROC asmCopyPixelsMultiTImgAIMMX( PCDATA po, PCDATA  pi   
								  , _32 oo, _32 oi         
								  , _32 ws, _32 hs         
								  , _32 nTransparent      
								  , CDATA rShade, CDATA gShade, CDATA bShade ); 


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

BLOT_EXTERN void (CPROC*CopyPixelsT0)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsT0
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsT1)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsT1
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsTA)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsTA
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsTImgA)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsTImgA
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsTImgAI)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsTImgAI
#endif
								  ;

//-----------------------------------------------------------
//-----------------------------------------------------------

BLOT_EXTERN void (CPROC*CopyPixelsShadedT0)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , CDATA shade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsShadedT0
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsShadedT1)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , CDATA shade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsShadedT1
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsShadedTA)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent
								  , CDATA shade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsShadedTA
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsShadedTImgA)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent
								  , CDATA shade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsShadedTImgA
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsShadedTImgAI)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent
								  , CDATA shade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsShadedTImgAI
#endif
								  ;

//-----------------------------------------------------------
//-----------------------------------------------------------
//-----------------------------------------------------------

BLOT_EXTERN void (CPROC*CopyPixelsMultiT0)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , CDATA rShade, CDATA gShade, CDATA bShade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsMultiT0
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsMultiT1)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , CDATA rShade, CDATA gShade, CDATA bShade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsMultiT1
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsMultiTA)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent
								  , CDATA rShade, CDATA gShade, CDATA bShade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsMultiTA
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsMultiTImgA)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent
								  , CDATA rShade, CDATA gShade, CDATA bShade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsMultiTImgA
#endif
								  ;
BLOT_EXTERN void (CPROC*CopyPixelsMultiTImgAI)( PCDATA po, PCDATA  pi
								  , _32 oo, _32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent
								  , CDATA rShade, CDATA gShade, CDATA bShade ) 
#ifdef IMAGE_MAIN
								  = cCopyPixelsMultiTImgAI
#endif
								  ;


//-----------------------------------------------------------
//-----------------------------------------------------------
//-----------------------------------------------------------
#define SCALED_BLOT_WORK_PARAMS  PCDATA po, PCDATA  pi       \
						    , int i_errx, int i_erry          \
						    , _32 wd, _32 hd                \
				          , _32 dwd, _32 dhd                \
				          , _32 dws, _32 dhs                \
				          , _32 oo, _32 srcpwidth

void CPROC cBlotScaledT0( SCALED_BLOT_WORK_PARAMS );   
void CPROC cBlotScaledT1( SCALED_BLOT_WORK_PARAMS );   
void CPROC cBlotScaledTA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 
void CPROC cBlotScaledTImgA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 
void CPROC cBlotScaledTImgAI( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 


void CPROC asmBlotScaledT0( SCALED_BLOT_WORK_PARAMS
								  );   
void CPROC asmBlotScaledT1( SCALED_BLOT_WORK_PARAMS
								  );   
void CPROC asmBlotScaledTA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 
void CPROC asmBlotScaledTImgA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 
void CPROC asmBlotScaledTImgAI( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 

void CPROC asmBlotScaledT0MMX( SCALED_BLOT_WORK_PARAMS
								  );      
void CPROC asmBlotScaledT1MMX( SCALED_BLOT_WORK_PARAMS
								  );      
void CPROC asmBlotScaledTAMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent );    
void CPROC asmBlotScaledTImgAMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent );    
void CPROC asmBlotScaledTImgAIMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent ); 


BLOT_EXTERN void (CPROC*BlotScaledT0)( SCALED_BLOT_WORK_PARAMS )
#ifdef IMAGE_MAIN
								  = cBlotScaledT0
#endif
				          
				          ;   
BLOT_EXTERN void (CPROC*BlotScaledT1)( SCALED_BLOT_WORK_PARAMS )
#ifdef IMAGE_MAIN
								  = cBlotScaledT1
#endif
				             ;   
BLOT_EXTERN void (CPROC*BlotScaledTA)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent )
#ifdef IMAGE_MAIN
								  = cBlotScaledTA
#endif
								  ; 
BLOT_EXTERN void (CPROC*BlotScaledTImgA)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent )
#ifdef IMAGE_MAIN
								  = cBlotScaledTImgA
#endif
								  ; 
BLOT_EXTERN void (CPROC*BlotScaledTImgAI)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent )
#ifdef IMAGE_MAIN
								  = cBlotScaledTImgAI
#endif
								  ; 

//-----------------------------------------------------------
//-----------------------------------------------------------
//-----------------------------------------------------------


void CPROC cBlotScaledShadedT0( SCALED_BLOT_WORK_PARAMS
				          , CDATA color 
				          );   
void CPROC cBlotScaledShadedT1( SCALED_BLOT_WORK_PARAMS
				          , CDATA color 
				          );   
void CPROC cBlotScaledShadedTA( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent
				          , CDATA color 
				          ); 
void CPROC cBlotScaledShadedTImgA( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent
				          , CDATA color 
				          ); 
void CPROC cBlotScaledShadedTImgAI( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent
					       , CDATA color 
					       ); 


void CPROC asmBlotScaledShadedT0( SCALED_BLOT_WORK_PARAMS
								  , CDATA color
								  );   
void CPROC asmBlotScaledShadedT1( SCALED_BLOT_WORK_PARAMS
								  , CDATA color
								  );   
void CPROC asmBlotScaledShadedTA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  ); 
void CPROC asmBlotScaledShadedTImgA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  ); 
void CPROC asmBlotScaledShadedTImgAI( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  ); 

void CPROC asmBlotScaledShadedT0MMX( SCALED_BLOT_WORK_PARAMS
								  , CDATA color
								  );      
void CPROC asmBlotScaledShadedT1MMX( SCALED_BLOT_WORK_PARAMS
								  , CDATA color
								  );      
void CPROC asmBlotScaledShadedTAMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  );    
void CPROC asmBlotScaledShadedTImgAMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  );    
void CPROC asmBlotScaledShadedTImgAIMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  ); 


BLOT_EXTERN void (CPROC*BlotScaledShadedT0)( SCALED_BLOT_WORK_PARAMS
								  , CDATA color
				          )
#ifdef IMAGE_MAIN
								  = cBlotScaledShadedT0
#endif
				          
				          ;   
BLOT_EXTERN void (CPROC*BlotScaledShadedT1)( SCALED_BLOT_WORK_PARAMS
								  , CDATA color
				             )
#ifdef IMAGE_MAIN
								  = cBlotScaledShadedT1
#endif
				             ;   
BLOT_EXTERN void (CPROC*BlotScaledShadedTA)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  )
#ifdef IMAGE_MAIN
								  = cBlotScaledShadedTA
#endif
								  ; 
BLOT_EXTERN void (CPROC*BlotScaledShadedTImgA)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  )
#ifdef IMAGE_MAIN
								  = cBlotScaledShadedTImgA
#endif
								  ; 
BLOT_EXTERN void (CPROC*BlotScaledShadedTImgAI)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA color
								  )
#ifdef IMAGE_MAIN
								  = cBlotScaledShadedTImgAI
#endif
								  ; 

//-----------------------------------------------------------
//-----------------------------------------------------------
//-----------------------------------------------------------


void CPROC cBlotScaledMultiT0( SCALED_BLOT_WORK_PARAMS
				          , CDATA r, CDATA g, CDATA b 
				          );   
void CPROC cBlotScaledMultiT1( SCALED_BLOT_WORK_PARAMS
				          , CDATA r, CDATA g, CDATA b 
				          );   
void CPROC cBlotScaledMultiTA( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent
				          , CDATA r, CDATA g, CDATA b 
				          ); 
void CPROC cBlotScaledMultiTImgA( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent
				          , CDATA r, CDATA g, CDATA b 
				          ); 
void CPROC cBlotScaledMultiTImgAI( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent
					       , CDATA r, CDATA g, CDATA b 
					       ); 


void CPROC asmBlotScaledMultiT0( SCALED_BLOT_WORK_PARAMS
      
								  , CDATA r, CDATA g, CDATA b
								  );   
void CPROC asmBlotScaledMultiT1( SCALED_BLOT_WORK_PARAMS
								  , CDATA r, CDATA g, CDATA b
								  );   
void CPROC asmBlotScaledMultiTA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  ); 
void CPROC asmBlotScaledMultiTImgA( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  ); 
void CPROC asmBlotScaledMultiTImgAI( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  ); 

void CPROC asmBlotScaledMultiT0MMX( SCALED_BLOT_WORK_PARAMS
								  , CDATA r, CDATA g, CDATA b
								  );      
void CPROC asmBlotScaledMultiT1MMX( SCALED_BLOT_WORK_PARAMS
								  , CDATA r, CDATA g, CDATA b
								  );      
void CPROC asmBlotScaledMultiTAMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  );    
void CPROC asmBlotScaledMultiTImgAMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  );    
void CPROC asmBlotScaledMultiTImgAIMMX( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  ); 


BLOT_EXTERN void (CPROC*BlotScaledMultiT0)( SCALED_BLOT_WORK_PARAMS
								  , CDATA r, CDATA g, CDATA b
				          )
#ifdef IMAGE_MAIN
								  = cBlotScaledMultiT0
#endif
				          
				          ;   
BLOT_EXTERN void (CPROC*BlotScaledMultiT1)( SCALED_BLOT_WORK_PARAMS
								  , CDATA r, CDATA g, CDATA b
				             )
#ifdef IMAGE_MAIN
								  = cBlotScaledMultiT1
#endif
				             ;   
BLOT_EXTERN void (CPROC*BlotScaledMultiTA)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  )
#ifdef IMAGE_MAIN
								  = cBlotScaledMultiTA
#endif
								  ; 
BLOT_EXTERN void (CPROC*BlotScaledMultiTImgA)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  )
#ifdef IMAGE_MAIN
								  = cBlotScaledMultiTImgA
#endif
								  ; 
BLOT_EXTERN void (CPROC*BlotScaledMultiTImgAI)( SCALED_BLOT_WORK_PARAMS
								  , _32 nTransparent 
								  , CDATA r, CDATA g, CDATA b
								  )
#ifdef IMAGE_MAIN
								  = cBlotScaledMultiTImgAI
#endif
								  ; 


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
