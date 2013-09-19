/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 * A static, reliable, internal font approx 12 high and 8 wide.
 *  Any NULL SFTFont used will result in the usage of this font.
 * 
 * 
 *  consult doc/image.html
 *
 */


#include <vectlib.h>
#undef _X
#define BITS_GREY2
#include "symbits.h"
//#include <imglib/fontstruct.h>
#ifdef __cplusplus 
namespace sack {
namespace image {
#endif
	typedef struct font_tag *PFONT ;
#ifdef __cplusplus 
	namespace default_font {
#endif

#ifdef __3D__
#define EXTRA_STRUCT  struct ImageFile_tag *cell; RCOORD x1, x2, y1, y2; struct font_char_tag *next_in_line;
#define EXTRA_INIT  0,0,0,0,0,0,
#else
#define EXTRA_STRUCT  
#define EXTRA_INIT  
#endif

typedef char CHARACTER, *PCHARACTER;
static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_0 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_1 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_2 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_3 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_4 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_5 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_6 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_7 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_8 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_9 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_10 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_11 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_12 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_13 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_14 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_15 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_16 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_17 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_18 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_19 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_20 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_21 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_22 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_23 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_24 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_25 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_26 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_27 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_28 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_29 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_30 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_31 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; } _char_32 =
{  0,  8,  0, 0,  0,  0};


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[9]; } _char_33 =
{  2,  8,  3, 0,  9,  1, EXTRA_INIT { 
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  ____,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[6]; } _char_34 =
{  6,  8,  1, 0, 10,  8, EXTRA_INIT { 
                  oXo_,XO__,
                  _X__,Xo__,
                  _X__,Xo__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_35 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___o,o_O_,
                  ___O,o_X_,
                  ___X,_oO_,
                  _XXX,XXXX,
                  __OO,_X__,
                  __Oo,_X__,
                  OXXX,XXXo,
                  __X_,Oo__,
                  _oO_,X___,
                  _Oo_,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[22]; } _char_36 =
{  6,  8,  1, 0, 10,  0, EXTRA_INIT { 
                  __oX,____,
                  _oXX,XX__,
                  _XOX,____,
                  _XoX,____,
                  _OXX,____,
                  __OX,O___,
                  __oX,XO__,
                  __oX,oX__,
                  o_oX,OX__,
                  oXXX,X___, // <---- baseline
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[30]; } _char_37 =
{  9,  8, -1, 0, 10,  1, EXTRA_INIT { 
                  _oXX,o__O,O___,
                  _XoO,X_oX,____,
                  oX_o,X_Xo,____,
                  _XoO,XXO_,____,
                  _oXX,OX__,____,
                  ____,XoXX,o___,
                  ___X,OOOo,X___,
                  __OX,_XOo,X___,
                  _oX_,_OOo,X___,
                  _Xo_,__XX,o___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[30]; } _char_38 =
{  9,  8, -1, 0, 10,  1, EXTRA_INIT { 
                  ___o,XXo_,____,
                  ___X,ooX_,____,
                  __oX,_oX_,____,
                  ___X,OXo_,____,
                  __oX,Xo__,____,
                  _OXo,Xo_o,X___,
                  _X__,oX_o,X___,
                  oX__,_OXX,o___,
                  _XX_,__XX,____,
                  __OX,XXoX,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[4]; } _char_39 =
{  2,  8,  3, 0, 10,  7, EXTRA_INIT { 
                  XX__,
                  OX__,
                  OX__,
                  oO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_40 =
{  6,  8,  2, 0, 10, -1, EXTRA_INIT { 
                  ____,OO__,
                  __oX,o___,
                  _oX_,____,
                  _XO_,____,
                  _Xo_,____,
                  oX__,____,
                  oX__,____,
                  _Xo_,____,
                  _XO_,____,
                  __X_,____, // <---- baseline
                  __oX,o___,
                  ____,OO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_41 =
{  6,  8,  0, 0, 10, -1, EXTRA_INIT { 
                  oXO_,____,
                  __OX,____,
                  ___o,X___,
                  ____,XO__,
                  ____,oX__,
                  ____,oX__,
                  ____,oX__,
                  ____,oX__,
                  ____,XO__,
                  ___o,X___, // <---- baseline
                  __OX,____,
                  _Oo_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_42 =
{  6,  8,  1, 0,  9,  5, EXTRA_INIT { 
                  __OO,____,
                  o_OO,_o__,
                  OX__,XO__,
                  _ooo,o___,
                  _Xoo,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_43 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XXX_,
                  __oX,____,
                  __oX,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[4]; } _char_44 =
{  2,  8,  2, 0,  2, -1, EXTRA_INIT { 
                  oX__,
                  oX__, // <---- baseline
                  _X__,
                  _O__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_45 =
{  5,  8,  1, 0,  4,  4, EXTRA_INIT { 
                  XXXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_46 =
{  2,  8,  2, 0,  2,  1, EXTRA_INIT { 
                  oX__,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_47 =
{  7,  8,  0, 0, 10, -1, EXTRA_INIT { 
                  ____,_OO_,
                  ____,_X__,
                  ____,Oo__,
                  ____,X___,
                  ___O,o___,
                  ___X,____,
                  __Oo,____,
                  __X_,____,
                  _Oo_,____,
                  _X__,____, // <---- baseline
                  Oo__,____,
                  X___,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_48 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OX,Xo__,
                  _oX_,_Xo_,
                  _Xo_,_OO_,
                  _X__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _Xo_,_OO_,
                  _oX_,_Xo_,
                  __OX,Xo__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_49 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  ___o,____,
                  OXXX,____,
                  o_oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_50 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _OXX,O___,
                  _o__,XX__,
                  ____,oX__,
                  ____,oX__,
                  ____,XO__,
                  ___X,O___,
                  __OO,____,
                  _OX_,____,
                  OX__,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_51 =
{  5,  8,  2, 0, 10,  1, EXTRA_INIT { 
                  XXXX,o___,
                  o__O,X___,
                  ___o,X___,
                  ___X,O___,
                  oXXO,____,
                  ___X,O___,
                  ___o,X___,
                  ___o,X___,
                  ___X,O___,
                  XXXO,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_52 =
{  8,  8,  0, 0,  9,  1, EXTRA_INIT { 
                  ____,OX__,
                  ___o,XX__,
                  ___X,oX__,
                  __Oo,oX__,
                  _oO_,oX__,
                  _X__,oX__,
                  oXXX,XXXO,
                  ____,oX__,
                  ____,oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_53 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  oXXX,XO__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,o___,
                  ___o,XO__,
                  ____,oX__,
                  ____,oX__,
                  ____,XO__,
                  _XXX,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_54 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __oX,XXo_,
                  __Xo,__o_,
                  _XO_,____,
                  _Xo_,____,
                  oXoX,XO__,
                  oXO_,_XX_,
                  _X__,_oX_,
                  _X__,_oX_,
                  _OO_,_XO_,
                  __OX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_55 =
{  6,  8,  1, 0,  9,  1, EXTRA_INIT { 
                  XXXX,XX__,
                  ____,OO__,
                  ____,X___,
                  ___X,o___,
                  __oX,____,
                  __Xo,____,
                  _OX_,____,
                  _Xo_,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_56 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OX,XXo_,
                  _Xo_,_OX_,
                  oX__,_oX_,
                  _XX_,_Xo_,
                  __XX,Xo__,
                  _OXo,XXo_,
                  _X__,_XX_,
                  oX__,_oX_,
                  _XO_,_OX_,
                  __XX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_57 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OX,Xo__,
                  _XO_,_Xo_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _XO_,_XX_,
                  __XX,OoX_,
                  ____,_oX_,
                  ____,_XO_,
                  ____,oX__,
                  __XX,Xo__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[7]; } _char_58 =
{  2,  8,  2, 0,  7,  1, EXTRA_INIT { 
                  oX__,
                  oX__,
                  ____,
                  ____,
                  ____,
                  oX__,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[9]; } _char_59 =
{  3,  8,  2, 0,  7, -1, EXTRA_INIT { 
                  oXX_,
                  oXX_,
                  ____,
                  ____,
                  ____,
                  oXX_,
                  oXX_, // <---- baseline
                  _oX_,
                  _Xo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_60 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  ____,__o_,
                  ____,OXO_,
                  __OX,O___,
                  OXO_,____,
                  _oXO,____,
                  ___o,XXo_,
                  ____,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[6]; } _char_61 =
{  7,  8,  1, 0,  5,  3, EXTRA_INIT { 
                  XXXX,XXX_,
                  ____,____,
                  XXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_62 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  o___,____,
                  OXO_,____,
                  __OX,O___,
                  ____,OXO_,
                  ___O,Xo__,
                  oXXo,____,
                  Xo__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_63 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  _XXX,XO__,
                  oX__,_OX_,
                  oX__,_oX_,
                  ____,_OO_,
                  ____,OX__,
                  ___X,O___,
                  ___X,____,
                  ____,____,
                  ____,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[30]; } _char_64 =
{  9,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___O,XXX_,____,
                  __XO,__OX,____,
                  _OX_,OXXX,____,
                  _XoO,O_oX,____,
                  _XoX,o_oX,____,
                  oXoX,__XX,____,
                  _XoX,ooOX,____,
                  _XOO,XOoX,X___,
                  _oXo,____,____,
                  __oX,XXo_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_65 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  ___O,X___,
                  ___X,Xo__,
                  __Oo,oX__,
                  __X_,_Xo_,
                  _oO_,_OO_,
                  _XXX,XXX_,
                  oX__,__XO,
                  Oo__,__oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_66 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,Xo__,
                  oX__,oX__,
                  oX__,OX__,
                  oXXX,X___,
                  oX__,oXO_,
                  oX__,_oX_,
                  oX__,_OX_,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_67 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  __oO,XXXo,
                  _oXo,____,
                  _Xo_,____,
                  _X__,____,
                  oX__,____,
                  _XO_,____,
                  _oXO,____,
                  __oO,XXXo} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_68 =
{  7,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oXXX,Xo__,
                  oX__,oXO_,
                  oX__,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_OX_,
                  oX__,oXo_,
                  oXXX,Xo__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_69 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XXO_,
                  oX__,____,
                  oX__,____,
                  oXXX,XX__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_70 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XXX_,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XX__,
                  oX__,____,
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_71 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  __OX,XXX_,
                  oXo_,__o_,
                  XO__,____,
                  X___,____,
                  X___,XXX_,
                  XO__,_oX_,
                  oXO_,_oX_,
                  __OX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_72 =
{  7,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oXXX,XXX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_73 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  XXXX,XX__,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_74 =
{  5,  8,  2, 0,  8,  1, EXTRA_INIT { 
                  oXXX,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___O,X___,
                  XXXO,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_75 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oX__,_OO_,
                  oX__,OO__,
                  oX_O,O___,
                  oXOX,____,
                  oXoX,o___,
                  oX_o,Xo__,
                  oX__,oXo_,
                  oX__,_oXo} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_76 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_77 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oXX_,__XX,
                  oXX_,_oXX,
                  oXoO,_OoX,
                  oX_X,_XoX,
                  oX_O,OooX,
                  oX_o,X_oX,
                  oX__,__oX,
                  oX__,__oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_78 =
{  7,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oXO_,_oX_,
                  oXX_,_oX_,
                  oXOO,_oX_,
                  oX_X,ooX_,
                  oX_o,XoX_,
                  oX__,XOX_,
                  oX__,oXX_,
                  oX__,_OX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_79 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  __OX,XXo_,
                  _OX_,__Xo,
                  _Xo_,__OX,
                  oX__,__oX,
                  oX__,__oX,
                  _Xo_,__OX,
                  _OX_,__Xo,
                  __OX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_80 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XXo_,
                  oX__,_OX_,
                  oX__,_oX_,
                  oX__,_XO_,
                  oXXX,XO__,
                  oX__,____,
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_81 =
{  8,  8,  0, 0,  8, -1, EXTRA_INIT { 
                  __OX,XXo_,
                  _OX_,__Xo,
                  _Xo_,__OX,
                  oX__,__oX,
                  oX__,__oX,
                  _Xo_,__OX,
                  _OX_,__Xo,
                  __OX,XXo_, // <---- baseline
                  ____,_OXO,
                  ____,___O} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_82 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,Xo__,
                  oX__,OX__,
                  oX__,oX__,
                  oX__,XO__,
                  oXXX,O___,
                  oX_o,X___,
                  oX__,OX__,
                  oX__,_XO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_83 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,Xo__,
                  Xo__,____,
                  Xo__,____,
                  OXXO,____,
                  __oO,XO__,
                  ____,oX__,
                  o___,OX__,
                  OXXX,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_84 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  XXXX,XXXX,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_85 =
{  7,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_86 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  OO__,___X,
                  oX__,__OO,
                  _XO_,__X_,
                  _oX_,_OO_,
                  __Xo,_X__,
                  __OX,oX__,
                  ___X,Xo__,
                  ___X,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_87 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  Xo__,___X,
                  Oo__,___X,
                  oO_O,X_oO,
                  oX_X,X_oO,
                  _X_O,OoOo,
                  _XoO,oOO_,
                  _XXo,_XX_,
                  _OX_,_XX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_88 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oXo_,__OO,
                  _oX_,_OO_,
                  __OX,oX__,
                  ___X,X___,
                  ___X,Xo__,
                  __Xo,oX__,
                  _OO_,_OX_,
                  OX__,__XX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_89 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  Xo__,__X_,
                  OX__,_Xo_,
                  _XO_,OO__,
                  __XO,X___,
                  __oX,o___,
                  __oX,____,
                  __oX,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_90 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  XXXX,XXX_,
                  ____,_Xo_,
                  ____,XO__,
                  ___X,O___,
                  __OO,____,
                  _OO_,____,
                  OO__,____,
                  XXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_91 =
{  5,  8,  2, 0, 10, -1, EXTRA_INIT { 
                  oXXX,X___,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____, // <---- baseline
                  oX__,____,
                  oXXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_92 =
{  7,  8,  1, 0, 10, -1, EXTRA_INIT { 
                  X___,____,
                  Oo__,____,
                  _X__,____,
                  _Oo_,____,
                  __X_,____,
                  __Oo,____,
                  ___X,____,
                  ___O,o___,
                  ____,X___,
                  ____,Oo__, // <---- baseline
                  ____,_X__,
                  ____,_OO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_93 =
{  5,  8,  0, 0, 10, -1, EXTRA_INIT { 
                  oXXX,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___, // <---- baseline
                  ___o,X___,
                  oXXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_94 =
{  7,  8,  1, 0,  9,  3, EXTRA_INIT { 
                  __oO,____,
                  __XX,____,
                  _oXO,O___,
                  _Xo_,X___,
                  _X__,Oo__,
                  Oo__,_X__,
                  X___,_Oo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_95 =
{  8,  8,  0, 0,  0,  0, EXTRA_INIT { 
                  XXXX,XXXX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[1]; } _char_96 =
{  2,  8,  3, 0, 10, 10, EXTRA_INIT { 
                  oO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_97 =
{  6,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  OXXX,o___,
                  o__O,X___,
                  ___o,X___,
                  _OXX,X___,
                  Xo_o,X___,
                  Xo_O,X___,
                  OXXo,OX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_98 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXoX,XX__,
                  oXO_,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oXO_,_XO_,
                  oXoX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_99 =
{  7,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  __OX,XXO_,
                  _OX_,____,
                  _Xo_,____,
                  oX__,____,
                  _Xo_,____,
                  _XX_,____,
                  __OX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_100 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ____,_oX_,
                  ____,_oX_,
                  ____,_oX_,
                  __OX,XOX_,
                  _XO_,_OX_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,OoX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_101 =
{  7,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  __OX,XO__,
                  _OO_,_OO_,
                  _X__,_oX_,
                  oXXX,XXX_,
                  _X__,____,
                  _XO_,____,
                  __OX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_102 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __oX,XXX_,
                  __Xo,____,
                  _oX_,____,
                  XXXX,XXX_,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_103 =
{  7,  8,  0, 0,  7, -1, EXTRA_INIT { 
                  __OX,XOX_,
                  _XO_,_OX_,
                  _Xo_,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,XoX_, // <---- baseline
                  ____,_OO_,
                  _oXX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_104 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXoO,XXo_,
                  oXX_,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_105 =
{  4,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __oX,
                  ____,
                  ____,
                  XXXX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_106 =
{  5,  8,  1, 0, 10, -1, EXTRA_INIT { 
                  ___o,X___,
                  ____,____,
                  ____,____,
                  _XXX,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___,
                  ___o,X___, // <---- baseline
                  ___O,X___,
                  XXXX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_107 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,oX__,
                  oX__,Xo__,
                  oXoX,o___,
                  oXOX,____,
                  oX_X,X___,
                  oX__,XX__,
                  oX__,_XX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_108 =
{  4,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  XXXX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_109 =
{  8,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  oXOX,OoXO,
                  oXOo,XOoX,
                  oX_o,X_oX,
                  oX_o,X_oX,
                  oX_o,X_oX,
                  oX_o,X_oX,
                  oX_o,X_oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_110 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  oXoO,XXo_,
                  oXX_,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_111 =
{  7,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  __OX,XO__,
                  _XO_,_XO_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XO_,
                  __OX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_112 =
{  7,  8,  0, 0,  7, -1, EXTRA_INIT { 
                  oXoX,XX__,
                  oXO_,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oXO_,_XO_,
                  oXOX,XO__, // <---- baseline
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_113 =
{  7,  8,  0, 0,  7, -1, EXTRA_INIT { 
                  __OX,XoX_,
                  _XO_,_OX_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,OoX_, // <---- baseline
                  ____,_oX_,
                  ____,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_114 =
{  6,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  oXoX,XX__,
                  oXX_,oX__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_115 =
{  6,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  _oXX,XX__,
                  _Xo_,____,
                  _XO_,____,
                  __OX,Xo__,
                  ____,OX__,
                  o___,OX__,
                  OXXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_116 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  _oX_,____,
                  XXXX,XX__,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____,
                  __Xo,____,
                  __OX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_117 =
{  7,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _Xo_,oXX_,
                  _OXX,OoX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_118 =
{  8,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  oX__,__OO,
                  _Xo_,__X_,
                  _OX_,_oO_,
                  __Xo,_Xo_,
                  __OO,oX__,
                  ___X,Xo__,
                  ___X,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_119 =
{  8,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  Xo__,___X,
                  OO_O,X__X,
                  oO_X,X_oO,
                  oX_O,oooo,
                  _Xoo,_OO_,
                  _XX_,_XX_,
                  _OX_,_OX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_120 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  XO__,_X__,
                  _Xo_,Xo__,
                  _oXX,O___,
                  __XX,____,
                  _oXX,X___,
                  _X__,XO__,
                  Xo__,oXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_121 =
{  7,  8,  1, 0,  7, -1, EXTRA_INIT { 
                  X___,_oO_,
                  OO__,_X__,
                  _X__,oO__,
                  _OO_,X___,
                  __XO,O___,
                  __OX,____,
                  __oX,____, // <---- baseline
                  __X_,____,
                  XXo_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_122 =
{  6,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  XXXX,XX__,
                  ____,XO__,
                  ___X,O___,
                  __OO,____,
                  _OX_,____,
                  OX__,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_123 =
{  7,  8,  1, 0, 10, -1, EXTRA_INIT { 
                  ___O,XXo_,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __OX,____,
                  oXXo,____,
                  __OX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____, // <---- baseline
                  ___X,o___,
                  ___o,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[12]; } _char_124 =
{  2,  8,  3, 0, 10, -1, EXTRA_INIT { 
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__, // <---- baseline
                  oX__,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_125 =
{  6,  8,  1, 0, 10, -1, EXTRA_INIT { 
                  OXXo,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  ___X,o___,
                  ___o,XX__,
                  ___X,o___,
                  __oX,____,
                  __oX,____,
                  __oX,____, // <---- baseline
                  __OX,____,
                  OXXo,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[4]; } _char_126 =
{  7,  8,  1, 0,  4,  3, EXTRA_INIT { 
                  OXXo,_oX_,
                  X__O,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_127 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_128 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_129 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_130 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_131 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_132 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_133 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_134 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_135 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_136 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_137 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_138 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_139 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_140 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_141 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_142 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_143 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_144 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_145 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_146 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_147 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_148 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_149 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_150 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_151 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_152 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_153 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_154 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_155 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_156 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_157 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_158 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_159 =
{  6,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oX__,oX__,
                  oXXX,XX__} };


static struct{ char s, w, o, j; short a, d; } _char_160 =
{  0,  8,  0, 0,  0,  0};


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[9]; } _char_161 =
{  2,  8,  3, 0,  7, -1, EXTRA_INIT { 
                  oX__,
                  ____,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__, // <---- baseline
                  oX__,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_162 =
{  7,  8,  1, 0,  9,  1, EXTRA_INIT { 
                  ___o,X___,
                  __OX,XXO_,
                  _OXo,X___,
                  _X_o,X___,
                  oX_o,X___,
                  _Xoo,X___,
                  _OXo,X___,
                  __OX,XXO_,
                  ___o,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_163 =
{  5,  8,  2, 0, 10,  1, EXTRA_INIT { 
                  __oX,X___,
                  __Xo,____,
                  _oX_,____,
                  _oX_,____,
                  _oX_,____,
                  XXXX,____,
                  _oX_,____,
                  _oX_,____,
                  _Xo_,____,
                  XXXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_164 =
{  7,  8,  1, 0,  8,  2, EXTRA_INIT { 
                  o___,__o_,
                  oXOX,OX__,
                  _Xo_,OX__,
                  oX__,oX__,
                  _XO_,OX__,
                  oXOX,OXo_,
                  o___,__o_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_165 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  Oo__,__O_,
                  oX__,_OO_,
                  _XO_,oX__,
                  _oXo,X___,
                  __OX,O___,
                  _XXX,XX__,
                  __oX,____,
                  _XXX,XX__,
                  __oX,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[12]; } _char_166 =
{  2,  8,  3, 0, 10, -1, EXTRA_INIT { 
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  oX__,
                  ____,
                  ____,
                  oX__,
                  oX__,
                  oX__, // <---- baseline
                  oX__,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_167 =
{  5,  8,  2, 0, 10, -1, EXTRA_INIT { 
                  oXXX,O___,
                  Xo__,____,
                  Xo__,____,
                  OXO_,____,
                  OOXX,o___,
                  X__O,X___,
                  XO_o,X___,
                  oXXX,o___,
                  __oX,O___,
                  ___o,X___, // <---- baseline
                  o__O,X___,
                  OXXO,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_168 =
{  5,  8,  1, 0, 10, 10, EXTRA_INIT { 
                  oX_o,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_169 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __oX,XXo_,
                  _oXo,__Xo,
                  _XOO,XXXX,
                  _XXO,__oX,
                  oXX_,___X,
                  oXX_,___X,
                  _XXO,__oX,
                  _XOO,XXXX,
                  _OX_,__Xo,
                  __OX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_170 =
{  5,  8,  2, 0,  9,  5, EXTRA_INIT { 
                  XXXO,____,
                  o_oX,____,
                  oXXX,____,
                  XooX,____,
                  XXOX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_171 =
{  6,  8,  1, 0,  6,  2, EXTRA_INIT { 
                  __X_,_X__,
                  _Xo_,X___,
                  XO_X,o___,
                  oX_o,X___,
                  _oX_,oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[6]; } _char_172 =
{  8,  8,  0, 0,  5,  3, EXTRA_INIT { 
                  oXXX,XXXX,
                  ____,__oX,
                  ____,__oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_173 =
{  5,  8,  1, 0,  4,  4, EXTRA_INIT { 
                  XXXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[12]; } _char_174 =
{  6,  8,  1, 0,  9,  4, EXTRA_INIT { 
                  _OXX,O___,
                  XXXX,XO__,
                  XXXo,oX__,
                  XXOo,oX__,
                  XXoX,XO__,
                  _OXX,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_175 =
{  8,  8,  0, 0, 10, 10, EXTRA_INIT { 
                  XXXX,XXXX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[3]; } _char_176 =
{  3,  8,  3, 0, 10,  8, EXTRA_INIT { 
                  OXO_,
                  XOX_,
                  OXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_177 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  __oX,____,
                  __oX,____,
                  XXXX,XXX_,
                  __oX,____,
                  __oX,____,
                  ____,____,
                  XXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[5]; } _char_178 =
{  4,  8,  2, 0, 10,  6, EXTRA_INIT { 
                  OXXO,
                  __oX,
                  _oXo,
                  OO__,
                  XXXX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[5]; } _char_179 =
{  4,  8,  2, 0,  9,  5, EXTRA_INIT { 
                  XXXO,
                  __OX,
                  OXXo,
                  __OX,
                  XXXO} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[1]; } _char_180 =
{  2,  8,  3, 0, 10, 10, EXTRA_INIT { 
                  OO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_181 =
{  8,  8,  1, 0,  7, -1, EXTRA_INIT { 
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oXO_,_XX_,
                  oXOX,XoXo, // <---- baseline
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_182 =
{  6,  8,  1, 0,  9, -2, EXTRA_INIT { 
                  _XXX,XX__,
                  OXXX,oX__,
                  XXXX,oX__,
                  OXXX,oX__,
                  _oXX,oX__,
                  __oX,oX__,
                  __oX,oX__,
                  __oX,oX__,
                  __oX,oX__, // <---- baseline
                  __oX,oX__,
                  __oX,oX__,
                  ___o,_o__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_183 =
{  2,  8,  3, 0,  5,  4, EXTRA_INIT { 
                  oX__,
                  oX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[2]; } _char_184 =
{  2,  8,  4, 0,  0, -1, EXTRA_INIT { 
                  Oo__,
                  XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[5]; } _char_185 =
{  3,  8,  2, 0, 10,  6, EXTRA_INIT { 
                  oXX_,
                  _oX_,
                  _oX_,
                  _oX_,
                  _oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_186 =
{  6,  8,  1, 0,  9,  5, EXTRA_INIT { 
                  _oXX,X___,
                  _Xo_,OX__,
                  oX__,oX__,
                  _Xo_,OX__,
                  _oXX,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_187 =
{  6,  8,  1, 0,  6,  2, EXTRA_INIT { 
                  Oo_X,o___,
                  _Xo_,Xo__,
                  __Xo,_X__,
                  _OO_,XO__,
                  OO_O,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_188 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  oo__,_o__,
                  oX__,oX__,
                  oX__,Xo__,
                  oX_o,X___,
                  oX_X,o___,
                  _oOO,_oO_,
                  __X_,oOX_,
                  _OOo,ooX_,
                  _X_O,OXXO,
                  OO__,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_189 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  oO__,_O__,
                  oX__,oX__,
                  oX__,Xo__,
                  oX_O,X___,
                  oX_X,____,
                  __OO,XXXo,
                  __X_,__oX,
                  _Oo_,_oXo,
                  oX__,OX__,
                  Xo__,XXXX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[30]; } _char_190 =
{  9,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  OXXO,___O,o___,
                  __oX,__oO,____,
                  _XX_,__X_,____,
                  __OX,_X__,____,
                  OXXO,Oo__,____,
                  ___o,X_XX,____,
                  ___X,_OoX,____,
                  __Xo,OooX,____,
                  _OO_,OOXX,O___,
                  oX__,__oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_191 =
{  6,  8,  1, 0,  7, -1, EXTRA_INIT { 
                  __oX,____,
                  ____,____,
                  __oX,____,
                  __OX,____,
                  _oX_,____,
                  OX__,____,
                  X___,____, // <---- baseline
                  Xo__,oX__,
                  oXXX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_192 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __oO,o___,
                  ____,____,
                  ___O,X___,
                  ___X,Xo__,
                  __Oo,oX__,
                  __X_,_Xo_,
                  _oO_,_OO_,
                  _XXX,XXX_,
                  oX__,__XO,
                  Oo__,__oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_193 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___o,Oo__,
                  ____,____,
                  ___O,X___,
                  ___X,Xo__,
                  __Oo,oX__,
                  __X_,_Xo_,
                  _oO_,_OO_,
                  _XXX,XXX_,
                  oX__,__XO,
                  Oo__,__oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_194 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OO,OO__,
                  ____,____,
                  ___O,X___,
                  ___X,Xo__,
                  __Oo,oX__,
                  __X_,_Xo_,
                  _oO_,_OO_,
                  _XXX,XXX_,
                  oX__,__XO,
                  Oo__,__oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_195 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __Xo,XO__,
                  ____,____,
                  ___O,O___,
                  ___X,X___,
                  __Oo,Xo__,
                  __X_,OX__,
                  _OX_,oXo_,
                  _XXX,XXX_,
                  oO__,__Xo,
                  X___,__OX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_196 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  ___X,O___,
                  ___X,X___,
                  __Oo,OO__,
                  __X_,oX__,
                  _OO_,_Xo_,
                  _XXX,XXX_,
                  oO__,__Xo,
                  X___,__OO} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_197 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___X,O___,
                  ___X,X___,
                  ____,o___,
                  ___O,O___,
                  ___o,oo__,
                  __O_,_X__,
                  _oO_,_Xo_,
                  _XXX,XXX_,
                  oO__,__X_,
                  X___,__OO} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_198 =
{  9,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  ____,XXXX,____,
                  ___o,XX__,____,
                  ___X,OX__,____,
                  __oO,oX__,____,
                  __X_,oXXX,____,
                  _oXX,XX__,____,
                  _X__,oX__,____,
                  oX__,oXXX,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_199 =
{  8,  8,  0, 0,  8, -1, EXTRA_INIT { 
                  __oO,XXXo,
                  _oXo,___o,
                  _Xo_,____,
                  _X__,____,
                  oX__,____,
                  _XO_,____,
                  _oXO,____,
                  __oO,XXXo, // <---- baseline
                  ____,XO__,
                  ____,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_200 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __oO,o___,
                  ____,____,
                  oXXX,XXO_,
                  oX__,____,
                  oX__,____,
                  oXXX,XX__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_201 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  ___o,Oo__,
                  ____,____,
                  oXXX,XXO_,
                  oX__,____,
                  oX__,____,
                  oXXX,XX__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_202 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __OO,OO__,
                  ____,____,
                  oXXX,XXO_,
                  oX__,____,
                  oX__,____,
                  oXXX,XX__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_203 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  oXXX,XXO_,
                  oX__,____,
                  oX__,____,
                  oXXX,XX__,
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXXX,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_204 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __OO,____,
                  ____,____,
                  XXXX,XX__,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_205 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  ___O,o___,
                  ____,____,
                  XXXX,XX__,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_206 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _oOO,O___,
                  ____,____,
                  XXXX,XX__,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_207 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  oX__,X___,
                  ____,____,
                  XXXX,XX__,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  XXXX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_208 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  oXXX,XOo_,
                  oX__,_oXo,
                  oX__,__OX,
                  XXXX,__oX,
                  oX__,__oX,
                  oX__,__OX,
                  oX__,_oXo,
                  oXXX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_209 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __Xo,XO__,
                  ____,____,
                  oXo_,_oX_,
                  oXX_,_oX_,
                  oXXO,_oX_,
                  oOoX,_oX_,
                  oO_O,XoX_,
                  oO__,XOX_,
                  oO__,oXX_,
                  oO__,_XX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_210 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___O,O___,
                  ____,____,
                  __OX,XXo_,
                  _OX_,__Xo,
                  _Xo_,__OX,
                  oX__,__oX,
                  oX__,__oX,
                  _Xo_,__OX,
                  _OX_,__Xo,
                  __OX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_211 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ____,OO__,
                  ____,____,
                  __OX,XXo_,
                  _OX_,__Xo,
                  _Xo_,__OX,
                  oX__,__oX,
                  oX__,__oX,
                  _Xo_,__OX,
                  _OX_,__Xo,
                  __OX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_212 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __oO,OO__,
                  ____,____,
                  __OX,XXo_,
                  _OX_,__Xo,
                  _Xo_,__OX,
                  oX__,__oX,
                  oX__,__oX,
                  _Xo_,__OX,
                  _OX_,__Xo,
                  __OX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[30]; } _char_213 =
{  9,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __Xo,OXO_,____,
                  ____,____,____,
                  __oX,XXO_,____,
                  _OO_,__XO,____,
                  _X__,___X,o___,
                  oO__,___X,o___,
                  oO__,___X,o___,
                  _X__,___X,o___,
                  _OO_,__XO,____,
                  __oX,XXO_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[30]; } _char_214 =
{  9,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  _oX_,_oX_,____,
                  ____,____,____,
                  __oX,XXO_,____,
                  _OO_,__XO,____,
                  _X__,___X,o___,
                  oO__,___X,o___,
                  oO__,___X,o___,
                  _X__,___X,o___,
                  _OO_,__XO,____,
                  __oX,XXO_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_215 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  O___,_oo_,
                  OO__,oX__,
                  _OOo,X___,
                  __XX,____,
                  _OXO,O___,
                  OX__,OX__,
                  O___,_oo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_216 =
{  8,  8,  0, 0,  8,  1, EXTRA_INIT { 
                  __OX,XXOO,
                  _OX_,_oXo,
                  _Xo_,_XOX,
                  oX__,OooX,
                  oX_O,o_oX,
                  _XOO,__OX,
                  _OX_,__Xo,
                  _XOX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_217 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __oO,o___,
                  ____,____,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_218 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___o,Oo__,
                  ____,____,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_219 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OO,OO__,
                  ____,____,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XX_,
                  __XX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_220 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  oO__,_oX_,
                  oO__,_oX_,
                  oO__,_oX_,
                  oO__,_oX_,
                  oO__,_oX_,
                  _X__,_oX_,
                  _Xo_,_XX_,
                  __XX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_221 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  ___O,O___,
                  ____,____,
                  Xo__,__X_,
                  OX__,_Xo_,
                  _XO_,OO__,
                  __XO,X___,
                  __oX,o___,
                  __oX,____,
                  __oX,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[16]; } _char_222 =
{  7,  8,  1, 0,  8,  1, EXTRA_INIT { 
                  oX__,____,
                  oXXX,XXo_,
                  oX__,_OX_,
                  oX__,_oX_,
                  oX__,_XO_,
                  oXXX,XO__,
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_223 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _OXX,O___,
                  _Xo_,Xo__,
                  oX__,Xo__,
                  oX_O,O___,
                  oXoX,____,
                  oX_X,Xo__,
                  oX__,OXo_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oXoX,XXo_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_224 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _OO_,____,
                  __O_,____,
                  ____,____,
                  OXXX,o___,
                  o__O,X___,
                  ___o,X___,
                  _OXX,X___,
                  Xo_o,X___,
                  Xo_O,X___,
                  OXXo,OX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_225 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  ___X,o___,
                  __Oo,____,
                  ____,____,
                  OXXX,o___,
                  o__O,X___,
                  ___o,X___,
                  _OXX,X___,
                  Xo_o,X___,
                  Xo_O,X___,
                  OXXo,OX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_226 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _oXO,____,
                  _XoO,O___,
                  ____,____,
                  OXXX,o___,
                  o__O,X___,
                  ___o,X___,
                  _OXX,X___,
                  Xo_o,X___,
                  Xo_O,X___,
                  OXXo,OX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_227 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _OOo,X___,
                  _XoX,O___,
                  ____,____,
                  OXXX,o___,
                  o__o,X___,
                  ___o,X___,
                  _OXX,X___,
                  Xo_o,X___,
                  Xo_O,X___,
                  OXXo,OX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_228 =
{  6,  8,  1, 0,  9,  1, EXTRA_INIT { 
                  _X_o,X___,
                  ____,____,
                  OXXX,o___,
                  o__O,X___,
                  ___o,X___,
                  oXXX,X___,
                  Xo_o,X___,
                  Xo_O,X___,
                  OXXo,OX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_229 =
{  8,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __OX,O___,
                  __XO,X___,
                  __OX,O___,
                  _OXX,XO__,
                  ____,oXo_,
                  ____,_XO_,
                  _OXX,XXO_,
                  XO__,_XO_,
                  Xo__,oXO_,
                  OXXX,ooXO} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_230 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  XXXO,XXo_,
                  __oX,ooX_,
                  __oX,_oX_,
                  oXXX,XXX_,
                  XooX,____,
                  X_oX,o___,
                  OXOo,XXX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_231 =
{  7,  8,  0, 0,  7, -1, EXTRA_INIT { 
                  __OX,XXO_,
                  _OX_,____,
                  _Xo_,____,
                  oX__,____,
                  _Xo_,____,
                  _XX_,____,
                  __OX,XXO_, // <---- baseline
                  ___X,O___,
                  ___X,X___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_232 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OX,____,
                  ___o,O___,
                  ____,____,
                  __OX,XO__,
                  _OO_,_OO_,
                  _X__,_oX_,
                  oXXX,XXX_,
                  _X__,____,
                  _XO_,____,
                  __OX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_233 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ____,OO__,
                  ___o,O___,
                  ____,____,
                  __OX,XO__,
                  _OO_,_OO_,
                  _X__,_oX_,
                  oXXX,XXX_,
                  _X__,____,
                  _XO_,____,
                  __OX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_234 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___O,Oo__,
                  __X_,_X__,
                  ____,____,
                  __OX,XO__,
                  _OO_,_OO_,
                  _X__,_oX_,
                  oXXX,XXX_,
                  _X__,____,
                  _XO_,____,
                  __OX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_235 =
{  7,  8,  0, 0,  9,  1, EXTRA_INIT { 
                  __oX,oX__,
                  ____,____,
                  __OX,XO__,
                  _OO_,_OO_,
                  _X__,_oX_,
                  oXXX,XXX_,
                  _X__,____,
                  _XO_,____,
                  __OX,XXO_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[10]; } _char_236 =
{  4,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _oX_,
                  ___X,
                  ____,
                  XXXX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX,
                  __oX} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_237 =
{  5,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  ___O,O___,
                  __oO,____,
                  ____,____,
                  XXXX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_238 =
{  6,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  __OX,O___,
                  _OO_,Xo__,
                  ____,____,
                  XXXX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_239 =
{  6,  8,  1, 0,  9,  1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  OXXX,O___,
                  ___X,O___,
                  ___X,O___,
                  ___X,O___,
                  ___X,O___,
                  ___X,O___,
                  ___X,O___} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_240 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  oXXo,O___,
                  __XX,O___,
                  __o_,XO__,
                  __OX,XXo_,
                  _XO_,_XX_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _Xo_,_oX_,
                  _XO_,_XO_,
                  __OX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_241 =
{  7,  8,  1, 0, 10,  1, EXTRA_INIT { 
                  _OXo,oX__,
                  _XoO,XO__,
                  ____,____,
                  oXoX,XXo_,
                  oXX_,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_242 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OO,____,
                  ___O,o___,
                  ____,____,
                  __OX,XO__,
                  _XO_,_XO_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XO_,
                  __OX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_243 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ____,Xo__,
                  ___O,o___,
                  ____,____,
                  __OX,XO__,
                  _XO_,_XO_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XO_,
                  __OX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_244 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ___O,O___,
                  __O_,_O__,
                  ____,____,
                  __OX,XO__,
                  _XO_,_XO_,
                  _X__,_oX_,
                  oX__,_oX_,
                  _X__,_oX_,
                  _XO_,_XO_,
                  __OX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_245 =
{  8,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __OO,oX__,
                  __Xo,XO__,
                  ____,____,
                  __OX,XO__,
                  _Xo_,_OX_,
                  _X__,__Xo,
                  oO__,__Xo,
                  _X__,__Xo,
                  _Xo_,_OX_,
                  __OX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_246 =
{  8,  8,  0, 0,  9,  1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  __OX,XO__,
                  _Xo_,_OX_,
                  _X__,__Xo,
                  oO__,__Xo,
                  _X__,__Xo,
                  _Xo_,_OX_,
                  __OX,XX__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_247 =
{  7,  8,  1, 0,  7,  1, EXTRA_INIT { 
                  __oX,____,
                  ____,____,
                  ____,____,
                  XXXX,XXX_,
                  ____,____,
                  ____,____,
                  __oX,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[14]; } _char_248 =
{  7,  8,  0, 0,  7,  1, EXTRA_INIT { 
                  __OX,XOO_,
                  _XO_,_XO_,
                  _X__,OOX_,
                  oX_O,ooX_,
                  _XOO,_oX_,
                  _XX_,_XO_,
                  _XOX,XO__} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_249 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __XO,____,
                  ___O,O___,
                  ____,____,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _Xo_,oXX_,
                  _OXX,OoX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_250 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  ____,XO__,
                  ___O,O___,
                  ____,____,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _Xo_,oXX_,
                  _OXX,OoX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[20]; } _char_251 =
{  7,  8,  0, 0, 10,  1, EXTRA_INIT { 
                  __oX,X___,
                  _oXo,oX__,
                  ____,____,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  _Xo_,oXX_,
                  _OXX,OoX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[18]; } _char_252 =
{  7,  8,  0, 0,  9,  1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  oO__,_oX_,
                  oO__,_oX_,
                  oO__,_oX_,
                  oO__,_oX_,
                  oO__,_oX_,
                  _X__,oXX_,
                  _OXX,OoX_} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_253 =
{  7,  8,  1, 0, 10, -1, EXTRA_INIT { 
                  ___O,o___,
                  ____,____,
                  ____,____,
                  X___,_oO_,
                  OO__,_X__,
                  _X__,oO__,
                  _OO_,X___,
                  __XO,O___,
                  __OX,____,
                  __oX,____, // <---- baseline
                  __X_,____,
                  XXo_,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_254 =
{  7,  8,  0, 0, 10, -1, EXTRA_INIT { 
                  oX__,____,
                  oX__,____,
                  oX__,____,
                  oXoX,XX__,
                  oXO_,_OX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oX__,_oX_,
                  oXO_,_XO_,
                  oXOX,XO__, // <---- baseline
                  oX__,____,
                  oX__,____} };


static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[24]; } _char_255 =
{  8,  8,  0, 0, 10, -1, EXTRA_INIT { 
                  _oX_,oX__,
                  ____,____,
                  ____,____,
                  oO__,__OO,
                  _X__,_oX_,
                  _OO_,_XO_,
                  __X_,oX__,
                  __OO,XO__,
                  ___X,X___,
                  ___X,O___, // <---- baseline
                  __OX,____,
                  oXXo,____} };



struct { unsigned short height, baseline, chars; unsigned char flags, junk;
         const TEXTCHAR *fontname;
         PCHARACTER character[256]; } 
#ifdef __cplusplus
         ___LucidaConsole13by8 
#else
         _LucidaConsole13by8 
#endif
         = { 
13, 10, 256, 1, 0, WIDE("LucidaConsoleRegularfixed14By13"), {  (PCHARACTER)&_char_0
 ,(PCHARACTER)&_char_1
 ,(PCHARACTER)&_char_2
 ,(PCHARACTER)&_char_3
 ,(PCHARACTER)&_char_4
 ,(PCHARACTER)&_char_5
 ,(PCHARACTER)&_char_6
 ,(PCHARACTER)&_char_7
 ,(PCHARACTER)&_char_8
 ,(PCHARACTER)&_char_9
 ,(PCHARACTER)&_char_10
 ,(PCHARACTER)&_char_11
 ,(PCHARACTER)&_char_12
 ,(PCHARACTER)&_char_13
 ,(PCHARACTER)&_char_14
 ,(PCHARACTER)&_char_15
 ,(PCHARACTER)&_char_16
 ,(PCHARACTER)&_char_17
 ,(PCHARACTER)&_char_18
 ,(PCHARACTER)&_char_19
 ,(PCHARACTER)&_char_20
 ,(PCHARACTER)&_char_21
 ,(PCHARACTER)&_char_22
 ,(PCHARACTER)&_char_23
 ,(PCHARACTER)&_char_24
 ,(PCHARACTER)&_char_25
 ,(PCHARACTER)&_char_26
 ,(PCHARACTER)&_char_27
 ,(PCHARACTER)&_char_28
 ,(PCHARACTER)&_char_29
 ,(PCHARACTER)&_char_30
 ,(PCHARACTER)&_char_31
 ,(PCHARACTER)&_char_32
 ,(PCHARACTER)&_char_33
 ,(PCHARACTER)&_char_34
 ,(PCHARACTER)&_char_35
 ,(PCHARACTER)&_char_36
 ,(PCHARACTER)&_char_37
 ,(PCHARACTER)&_char_38
 ,(PCHARACTER)&_char_39
 ,(PCHARACTER)&_char_40
 ,(PCHARACTER)&_char_41
 ,(PCHARACTER)&_char_42
 ,(PCHARACTER)&_char_43
 ,(PCHARACTER)&_char_44
 ,(PCHARACTER)&_char_45
 ,(PCHARACTER)&_char_46
 ,(PCHARACTER)&_char_47
 ,(PCHARACTER)&_char_48
 ,(PCHARACTER)&_char_49
 ,(PCHARACTER)&_char_50
 ,(PCHARACTER)&_char_51
 ,(PCHARACTER)&_char_52
 ,(PCHARACTER)&_char_53
 ,(PCHARACTER)&_char_54
 ,(PCHARACTER)&_char_55
 ,(PCHARACTER)&_char_56
 ,(PCHARACTER)&_char_57
 ,(PCHARACTER)&_char_58
 ,(PCHARACTER)&_char_59
 ,(PCHARACTER)&_char_60
 ,(PCHARACTER)&_char_61
 ,(PCHARACTER)&_char_62
 ,(PCHARACTER)&_char_63
 ,(PCHARACTER)&_char_64
 ,(PCHARACTER)&_char_65
 ,(PCHARACTER)&_char_66
 ,(PCHARACTER)&_char_67
 ,(PCHARACTER)&_char_68
 ,(PCHARACTER)&_char_69
 ,(PCHARACTER)&_char_70
 ,(PCHARACTER)&_char_71
 ,(PCHARACTER)&_char_72
 ,(PCHARACTER)&_char_73
 ,(PCHARACTER)&_char_74
 ,(PCHARACTER)&_char_75
 ,(PCHARACTER)&_char_76
 ,(PCHARACTER)&_char_77
 ,(PCHARACTER)&_char_78
 ,(PCHARACTER)&_char_79
 ,(PCHARACTER)&_char_80
 ,(PCHARACTER)&_char_81
 ,(PCHARACTER)&_char_82
 ,(PCHARACTER)&_char_83
 ,(PCHARACTER)&_char_84
 ,(PCHARACTER)&_char_85
 ,(PCHARACTER)&_char_86
 ,(PCHARACTER)&_char_87
 ,(PCHARACTER)&_char_88
 ,(PCHARACTER)&_char_89
 ,(PCHARACTER)&_char_90
 ,(PCHARACTER)&_char_91
 ,(PCHARACTER)&_char_92
 ,(PCHARACTER)&_char_93
 ,(PCHARACTER)&_char_94
 ,(PCHARACTER)&_char_95
 ,(PCHARACTER)&_char_96
 ,(PCHARACTER)&_char_97
 ,(PCHARACTER)&_char_98
 ,(PCHARACTER)&_char_99
 ,(PCHARACTER)&_char_100
 ,(PCHARACTER)&_char_101
 ,(PCHARACTER)&_char_102
 ,(PCHARACTER)&_char_103
 ,(PCHARACTER)&_char_104
 ,(PCHARACTER)&_char_105
 ,(PCHARACTER)&_char_106
 ,(PCHARACTER)&_char_107
 ,(PCHARACTER)&_char_108
 ,(PCHARACTER)&_char_109
 ,(PCHARACTER)&_char_110
 ,(PCHARACTER)&_char_111
 ,(PCHARACTER)&_char_112
 ,(PCHARACTER)&_char_113
 ,(PCHARACTER)&_char_114
 ,(PCHARACTER)&_char_115
 ,(PCHARACTER)&_char_116
 ,(PCHARACTER)&_char_117
 ,(PCHARACTER)&_char_118
 ,(PCHARACTER)&_char_119
 ,(PCHARACTER)&_char_120
 ,(PCHARACTER)&_char_121
 ,(PCHARACTER)&_char_122
 ,(PCHARACTER)&_char_123
 ,(PCHARACTER)&_char_124
 ,(PCHARACTER)&_char_125
 ,(PCHARACTER)&_char_126
 ,(PCHARACTER)&_char_127
 ,(PCHARACTER)&_char_128
 ,(PCHARACTER)&_char_129
 ,(PCHARACTER)&_char_130
 ,(PCHARACTER)&_char_131
 ,(PCHARACTER)&_char_132
 ,(PCHARACTER)&_char_133
 ,(PCHARACTER)&_char_134
 ,(PCHARACTER)&_char_135
 ,(PCHARACTER)&_char_136
 ,(PCHARACTER)&_char_137
 ,(PCHARACTER)&_char_138
 ,(PCHARACTER)&_char_139
 ,(PCHARACTER)&_char_140
 ,(PCHARACTER)&_char_141
 ,(PCHARACTER)&_char_142
 ,(PCHARACTER)&_char_143
 ,(PCHARACTER)&_char_144
 ,(PCHARACTER)&_char_145
 ,(PCHARACTER)&_char_146
 ,(PCHARACTER)&_char_147
 ,(PCHARACTER)&_char_148
 ,(PCHARACTER)&_char_149
 ,(PCHARACTER)&_char_150
 ,(PCHARACTER)&_char_151
 ,(PCHARACTER)&_char_152
 ,(PCHARACTER)&_char_153
 ,(PCHARACTER)&_char_154
 ,(PCHARACTER)&_char_155
 ,(PCHARACTER)&_char_156
 ,(PCHARACTER)&_char_157
 ,(PCHARACTER)&_char_158
 ,(PCHARACTER)&_char_159
 ,(PCHARACTER)&_char_160
 ,(PCHARACTER)&_char_161
 ,(PCHARACTER)&_char_162
 ,(PCHARACTER)&_char_163
 ,(PCHARACTER)&_char_164
 ,(PCHARACTER)&_char_165
 ,(PCHARACTER)&_char_166
 ,(PCHARACTER)&_char_167
 ,(PCHARACTER)&_char_168
 ,(PCHARACTER)&_char_169
 ,(PCHARACTER)&_char_170
 ,(PCHARACTER)&_char_171
 ,(PCHARACTER)&_char_172
 ,(PCHARACTER)&_char_173
 ,(PCHARACTER)&_char_174
 ,(PCHARACTER)&_char_175
 ,(PCHARACTER)&_char_176
 ,(PCHARACTER)&_char_177
 ,(PCHARACTER)&_char_178
 ,(PCHARACTER)&_char_179
 ,(PCHARACTER)&_char_180
 ,(PCHARACTER)&_char_181
 ,(PCHARACTER)&_char_182
 ,(PCHARACTER)&_char_183
 ,(PCHARACTER)&_char_184
 ,(PCHARACTER)&_char_185
 ,(PCHARACTER)&_char_186
 ,(PCHARACTER)&_char_187
 ,(PCHARACTER)&_char_188
 ,(PCHARACTER)&_char_189
 ,(PCHARACTER)&_char_190
 ,(PCHARACTER)&_char_191
 ,(PCHARACTER)&_char_192
 ,(PCHARACTER)&_char_193
 ,(PCHARACTER)&_char_194
 ,(PCHARACTER)&_char_195
 ,(PCHARACTER)&_char_196
 ,(PCHARACTER)&_char_197
 ,(PCHARACTER)&_char_198
 ,(PCHARACTER)&_char_199
 ,(PCHARACTER)&_char_200
 ,(PCHARACTER)&_char_201
 ,(PCHARACTER)&_char_202
 ,(PCHARACTER)&_char_203
 ,(PCHARACTER)&_char_204
 ,(PCHARACTER)&_char_205
 ,(PCHARACTER)&_char_206
 ,(PCHARACTER)&_char_207
 ,(PCHARACTER)&_char_208
 ,(PCHARACTER)&_char_209
 ,(PCHARACTER)&_char_210
 ,(PCHARACTER)&_char_211
 ,(PCHARACTER)&_char_212
 ,(PCHARACTER)&_char_213
 ,(PCHARACTER)&_char_214
 ,(PCHARACTER)&_char_215
 ,(PCHARACTER)&_char_216
 ,(PCHARACTER)&_char_217
 ,(PCHARACTER)&_char_218
 ,(PCHARACTER)&_char_219
 ,(PCHARACTER)&_char_220
 ,(PCHARACTER)&_char_221
 ,(PCHARACTER)&_char_222
 ,(PCHARACTER)&_char_223
 ,(PCHARACTER)&_char_224
 ,(PCHARACTER)&_char_225
 ,(PCHARACTER)&_char_226
 ,(PCHARACTER)&_char_227
 ,(PCHARACTER)&_char_228
 ,(PCHARACTER)&_char_229
 ,(PCHARACTER)&_char_230
 ,(PCHARACTER)&_char_231
 ,(PCHARACTER)&_char_232
 ,(PCHARACTER)&_char_233
 ,(PCHARACTER)&_char_234
 ,(PCHARACTER)&_char_235
 ,(PCHARACTER)&_char_236
 ,(PCHARACTER)&_char_237
 ,(PCHARACTER)&_char_238
 ,(PCHARACTER)&_char_239
 ,(PCHARACTER)&_char_240
 ,(PCHARACTER)&_char_241
 ,(PCHARACTER)&_char_242
 ,(PCHARACTER)&_char_243
 ,(PCHARACTER)&_char_244
 ,(PCHARACTER)&_char_245
 ,(PCHARACTER)&_char_246
 ,(PCHARACTER)&_char_247
 ,(PCHARACTER)&_char_248
 ,(PCHARACTER)&_char_249
 ,(PCHARACTER)&_char_250
 ,(PCHARACTER)&_char_251
 ,(PCHARACTER)&_char_252
 ,(PCHARACTER)&_char_253
 ,(PCHARACTER)&_char_254
 ,(PCHARACTER)&_char_255

} };

#ifdef __cplusplus
PFONT __LucidaConsole13by8 = (PFONT)&___LucidaConsole13by8;
#endif

#ifdef __cplusplus
 }; // default_font namespace
}//namespace sack {
}//namespace image {
#endif
