#include "symbits.h"
typedef char CHARACTER, *PCHARACTER;
// Character: 8,12 .10.. (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_0 =
{  0,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_1 =
{  8,  8,  0, 0, {________,
                  _XXXXXX_,
                  XX____XX,
                  X______X,
                  X_X__X_X,
                  X______X,
                  X_XXXX_X,
                  X__XX__X,
                  XX____XX,
                  _XXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_2 =
{  8,  8,  0, 0, {________,
                  _XXXXXX_,
                  XXXXXXXX,
                  XXXXXXXX,
                  XX_XX_XX,
                  XXXXXXXX,
                  XX____XX,
                  XXX__XXX,
                  XXXXXXXX,
                  _XXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_3 =
{  7,  8,  0, 0, {________,
                  ________,
                  _X___X__,
                  XXX_XXX_,
                  XXXXXXX_,
                  XXXXXXX_,
                  XXXXXXX_,
                  _XXXXX__,
                  __XXX___,
                  ___X____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_4 =
{  7,  8,  0, 0, {________,
                  ___X____,
                  __XXX___,
                  _XXXXX__,
                  XXXXXXX_,
                  XXXXXXX_,
                  _XXXXX__,
                  __XXX___,
                  ___X____,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_5 =
{  8,  8,  0, 0, {________,
                  ___XX___,
                  __XXXX__,
                  __XXXX__,
                  XXXXXXXX,
                  XXX__XXX,
                  XXX__XXX,
                  ___XX___,
                  ___XX___,
                  _XXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_6 =
{  8,  8,  0, 0, {________,
                  ___XX___,
                  __XXXX__,
                  _XXXXXX_,
                  XXXXXXXX,
                  XXXXXXXX,
                  _XXXXXX_,
                  ___XX___,
                  ___XX___,
                  _XXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,6) to 6 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_7 =
{  6,  8,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  XXXXXX__,
                  XXXXXX__,
                  _XXXX___,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,10) to 6 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_8 =
{  6,  8,  1, 0, {XXXXXX__,
                  XXXXXX__,
                  XXXXXX__,
                  XXXXXX__,
                  X____X__,
                  ________,
                  ________,
                  X____X__,
                  XXXXXX__,
                  XXXXXX__, // <---- baseline
                  XXXXXX__,
                  XXXXXX__} };


// Character: 8,12 .10.. (1,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_9 =
{  6,  8,  1, 0, {________,
                  ________,
                  _XXXX___,
                  XXXXXX__,
                  XX__XX__,
                  X____X__,
                  X____X__,
                  XX__XX__,
                  XXXXXX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,10) to 6 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_10 =
{  6,  8,  1, 0, {XXXXXX__,
                  XXXXXX__,
                  X____X__,
                  ________,
                  __XX____,
                  _XXXX___,
                  _XXXX___,
                  __XX____,
                  ________,
                  X____X__, // <---- baseline
                  XXXXXX__,
                  XXXXXX__} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_11 =
{  7,  8,  0, 0, {________,
                  __XXXXX_,
                  ____XXX_,
                  __XXX_X_,
                  _XXX__X_,
                  XXXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_12 =
{  6,  8,  1, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  __XX____,
                  XXXXXX__,
                  __XX____,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_13 =
{  8,  8,  0, 0, {________,
                  ___XXXXX,
                  ___XX__X,
                  ___XX__X,
                  ___XXXXX,
                  ___XX___,
                  ___XX___,
                  _XXXX___,
                  XXXXX___,
                  _XXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_14 =
{  8,  8,  0, 0, {________,
                  _XXXXXXX,
                  _XX___XX,
                  _XXXXXXX,
                  _XX___XX,
                  _XX___XX,
                  _XX___XX,
                  _XX__XXX,
                  XXX__XXX,
                  XXX__XX_, // <---- baseline
                  XX______,
                  ________} };


// Character: 8,12 .10.. (0,8) to 8 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_15 =
{  8,  8,  0, 0, {________,
                  ________,
                  ___XX___,
                  XX_XX_XX,
                  _XXXXXX_,
                  XXX__XXX,
                  XXX__XXX,
                  _XXXXXX_,
                  XX_XX_XX,
                  ___XX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_16 =
{  7,  8,  0, 0, {________,
                  X_______,
                  XX______,
                  XXX_____,
                  XXXXX___,
                  XXXXXXX_,
                  XXXXX___,
                  XXX_____,
                  XX______,
                  X_______, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_17 =
{  7,  8,  0, 0, {________,
                  ______X_,
                  _____XX_,
                  ____XXX_,
                  __XXXXX_,
                  XXXXXXX_,
                  __XXXXX_,
                  ____XXX_,
                  _____XX_,
                  ______X_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_18 =
{  6,  8,  1, 0, {________,
                  __XX____,
                  _XXXX___,
                  XXXXXX__,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__,
                  _XXXX___,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_19 =
{  6,  8,  1, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  ________,
                  ________,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_20 =
{  8,  8,  0, 0, {________,
                  _XXXXXXX,
                  XX_XX_XX,
                  XX_XX_XX,
                  XX_XX_XX,
                  _XXXX_XX,
                  ___XX_XX,
                  ___XX_XX,
                  ___XX_XX,
                  ___XX_XX, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_21 =
{  8,  8,  0, 0, {________,
                  _XXXXXX_,
                  _XX___XX,
                  __XX____,
                  __XXXX__,
                  _XX__XX_,
                  _XX__XX_,
                  __XXXX__,
                  ____XX__,
                  XX___XX_, // <---- baseline
                  _XXXXXX_,
                  ________} };


// Character: 8,12 .10.. (0,3) to 7 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_22 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  XXXXXXX_,
                  XXXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_23 =
{  6,  8,  1, 0, {________,
                  __XX____,
                  _XXXX___,
                  XXXXXX__,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__,
                  _XXXX___,
                  __XX____, // <---- baseline
                  XXXXXX__,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_24 =
{  6,  8,  1, 0, {________,
                  __XX____,
                  _XXXX___,
                  XXXXXX__,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_25 =
{  6,  8,  1, 0, {________,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__,
                  _XXXX___,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,7) to 7 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_26 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ___XX___,
                  ____XX__,
                  XXXXXXX_,
                  ____XX__,
                  ___XX___,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,7) to 7 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_27 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  __XX____,
                  _XX_____,
                  XXXXXXX_,
                  _XX_____,
                  __XX____,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_28 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX______,
                  XX______,
                  XX______,
                  XXXXXXX_,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,7) to 8 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_29 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  __X__X__,
                  _XX__XX_,
                  XXXXXXXX,
                  _XX__XX_,
                  __X__X__,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_30 =
{  7,  8,  0, 0, {________,
                  ________,
                  ___X____,
                  ___X____,
                  __XXX___,
                  __XXX___,
                  _XXXXX__,
                  _XXXXX__,
                  XXXXXXX_,
                  XXXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_31 =
{  7,  8,  0, 0, {________,
                  ________,
                  XXXXXXX_,
                  XXXXXXX_,
                  _XXXXX__,
                  _XXXXX__,
                  __XXX___,
                  __XXX___,
                  ___X____,
                  ___X____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_32 =
{  0,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 4 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_33 =
{  4,  8,  1, 0, {________,
                  _XX_____,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  _XX_____,
                  _XX_____,
                  ________,
                  _XX_____,
                  _XX_____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_34 =
{  6,  8,  1, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _X__X___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_35 =
{  7,  8,  0, 0, {________,
                  _XX_XX__,
                  _XX_XX__,
                  XXXXXXX_,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  XXXXXXX_,
                  _XX_XX__,
                  _XX_XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_36 =
{  6,  8,  0, 0, {__XX____,
                  __XX____,
                  _XXXXX__,
                  XX______,
                  XX______,
                  _XXXX___,
                  ____XX__,
                  ____XX__,
                  XXXXX___,
                  __XX____, // <---- baseline
                  __XX____,
                  ________} };


// Character: 8,12 .10.. (0,7) to 6 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_37 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  XX___X__,
                  XX__XX__,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX__XX__,
                  X___XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_38 =
{  7,  8,  0, 0, {________,
                  _XXX____,
                  XX_XX___,
                  XX_XX___,
                  _XXX____,
                  XXXXX_X_,
                  XX_XXXX_,
                  XX__XX__,
                  XX_XXX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 3 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_39 =
{  3,  8,  1, 0, {________,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  XX______,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_40 =
{  5,  8,  1, 0, {________,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX______,
                  XX______,
                  XX______,
                  _XX_____,
                  __XX____,
                  ___XX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_41 =
{  5,  8,  1, 0, {________,
                  XX______,
                  _XX_____,
                  __XX____,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX______, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,7) to 8 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_42 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  _XX__XX_,
                  __XXXX__,
                  XXXXXXXX,
                  __XXXX__,
                  _XX__XX_,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,7) to 6 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_43 =
{  6,  8,  1, 0, {________,
                  ________,
                  ________,
                  __XX____,
                  __XX____,
                  XXXXXX__,
                  __XX____,
                  __XX____,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,2) to 4 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_44 =
{  4,  8,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  _XXX____,
                  _XXX____, // <---- baseline
                  XX______,
                  ________} };


// Character: 8,12 .10.. (0,5) to 7 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_45 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (2,2) to 3 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_46 =
{  3,  8,  2, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_47 =
{  7,  8,  0, 0, {________,
                  ________,
                  ______X_,
                  _____XX_,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX______,
                  X_______, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_48 =
{  7,  8,  0, 0, {________,
                  _XXXXX__,
                  XX___XX_,
                  XX__XXX_,
                  XX_XXXX_,
                  XX_X_XX_,
                  XXXX_XX_,
                  XXX__XX_,
                  XX___XX_,
                  _XXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_49 =
{  6,  8,  0, 0, {________,
                  ___X____,
                  __XX____,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_50 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX__XX__,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_51 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  ____XX__,
                  ____XX__,
                  __XXX___,
                  ____XX__,
                  ____XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_52 =
{  7,  8,  0, 0, {________,
                  ____XX__,
                  ___XXX__,
                  __XXXX__,
                  _XX_XX__,
                  XX__XX__,
                  XXXXXXX_,
                  ____XX__,
                  ____XX__,
                  ___XXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_53 =
{  6,  8,  0, 0, {________,
                  XXXXXX__,
                  XX______,
                  XX______,
                  XX______,
                  XXXXX___,
                  ____XX__,
                  ____XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_54 =
{  6,  8,  0, 0, {________,
                  __XXX___,
                  _XX_____,
                  XX______,
                  XX______,
                  XXXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_55 =
{  7,  8,  0, 0, {________,
                  XXXXXXX_,
                  XX___XX_,
                  XX___XX_,
                  _____XX_,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  __XX____,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_56 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XXX_XX__,
                  _XXXX___,
                  XX_XXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_57 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXXX__,
                  ___XX___,
                  ___XX___,
                  __XX____,
                  _XXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (2,7) to 3 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_58 =
{  3,  8,  2, 0, {________,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (2,7) to 3 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_59 =
{  3,  8,  2, 0, {________,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  _XX_____, // <---- baseline
                  XX______,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_60 =
{  6,  8,  0, 0, {________,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX______,
                  _XX_____,
                  __XX____,
                  ___XX___,
                  ____XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,6) to 6 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_61 =
{  6,  8,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXX__,
                  ________,
                  XXXXXX__,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_62 =
{  6,  8,  1, 0, {________,
                  XX______,
                  _XX_____,
                  __XX____,
                  ___XX___,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XX______, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_63 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  __XX____,
                  ________,
                  __XX____,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_64 =
{  7,  8,  0, 0, {________,
                  _XXXXX__,
                  XX___XX_,
                  XX___XX_,
                  XX_XXXX_,
                  XX_XXXX_,
                  XX_XXXX_,
                  XX______,
                  XX______,
                  _XXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_65 =
{  6,  8,  0, 0, {________,
                  __XX____,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XXXXXX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_66 =
{  7,  8,  0, 0, {________,
                  XXXXXX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XXXXX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_67 =
{  7,  8,  0, 0, {________,
                  __XXXX__,
                  _XX__XX_,
                  XX___XX_,
                  XX______,
                  XX______,
                  XX______,
                  XX___XX_,
                  _XX__XX_,
                  __XXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_68 =
{  7,  8,  0, 0, {________,
                  XXXXX___,
                  _XX_XX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX_XX__,
                  XXXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_69 =
{  7,  8,  0, 0, {________,
                  XXXXXXX_,
                  _XX___X_,
                  _XX_____,
                  _XX__X__,
                  _XXXXX__,
                  _XX__X__,
                  _XX_____,
                  _XX___X_,
                  XXXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_70 =
{  7,  8,  0, 0, {________,
                  XXXXXXX_,
                  _XX__XX_,
                  _XX___X_,
                  _XX__X__,
                  _XXXXX__,
                  _XX__X__,
                  _XX_____,
                  _XX_____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_71 =
{  7,  8,  0, 0, {________,
                  __XXXX__,
                  _XX__XX_,
                  XX___XX_,
                  XX______,
                  XX______,
                  XX__XXX_,
                  XX___XX_,
                  _XX__XX_,
                  __XXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_72 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XXXXXX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 4 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_73 =
{  4,  8,  1, 0, {________,
                  XXXX____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_74 =
{  7,  8,  0, 0, {________,
                  ___XXXX_,
                  ____XX__,
                  ____XX__,
                  ____XX__,
                  ____XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_75 =
{  7,  8,  0, 0, {________,
                  XXX__XX_,
                  _XX__XX_,
                  _XX_XX__,
                  _XX_XX__,
                  _XXXX___,
                  _XX_XX__,
                  _XX_XX__,
                  _XX__XX_,
                  XXX__XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_76 =
{  7,  8,  0, 0, {________,
                  XXXX____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX___X_,
                  _XX__XX_,
                  _XX__XX_,
                  XXXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_77 =
{  7,  8,  0, 0, {________,
                  XX___XX_,
                  XXX_XXX_,
                  XXXXXXX_,
                  XXXXXXX_,
                  XX_X_XX_,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_78 =
{  7,  8,  0, 0, {________,
                  XX___XX_,
                  XX___XX_,
                  XXX__XX_,
                  XXXX_XX_,
                  XXXXXXX_,
                  XX_XXXX_,
                  XX__XXX_,
                  XX___XX_,
                  XX___XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_79 =
{  7,  8,  0, 0, {________,
                  __XXX___,
                  _XX_XX__,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  _XX_XX__,
                  __XXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_80 =
{  7,  8,  0, 0, {________,
                  XXXXXX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XXXXX__,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_81 =
{  7,  8,  0, 0, {________,
                  __XXX___,
                  _XX_XX__,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX__XXX_,
                  XX_XXXX_,
                  _XXXXX__,
                  ____XX__, // <---- baseline
                  ___XXXX_,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_82 =
{  7,  8,  0, 0, {________,
                  XXXXXX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XXXXX__,
                  _XX_XX__,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_83 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX______,
                  _XXX____,
                  ___XX___,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_84 =
{  6,  8,  0, 0, {________,
                  XXXXXX__,
                  X_XX_X__,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_85 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_86 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_87 =
{  7,  8,  0, 0, {________,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX_X_XX_,
                  XX_X_XX_,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_88 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  __XX____,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_89 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  __XX____,
                  __XX____,
                  __XX____,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_90 =
{  7,  8,  0, 0, {________,
                  XXXXXXX_,
                  XX__XXX_,
                  X__XX___,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  _XX___X_,
                  XX___XX_,
                  XXXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (2,9) to 4 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_91 =
{  4,  8,  2, 0, {________,
                  XXXX____,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_92 =
{  7,  8,  0, 0, {________,
                  ________,
                  X_______,
                  XX______,
                  _XX_____,
                  __XX____,
                  ___XX___,
                  ____XX__,
                  _____XX_,
                  ______X_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (2,9) to 4 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_93 =
{  4,  8,  2, 0, {________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_94 =
{  7,  8,  0, 0, {___X____,
                  __XXX___,
                  _XX_XX__,
                  XX___XX_,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,0) to 8 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_95 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  XXXXXXXX,
                  ________} };


// Character: 8,12 .10.. (2,10) to 3 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_96 =
{  3,  8,  2, 0, {XX______,
                  XX______,
                  _XX_____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_97 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_98 =
{  7,  8,  0, 0, {________,
                  XXX_____,
                  _XX_____,
                  _XX_____,
                  _XXXXX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XX_XXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_99 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX______,
                  XX______,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_100 =
{  7,  8,  0, 0, {________,
                  ___XXX__,
                  ____XX__,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_101 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XXXXXX__,
                  XX______,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_102 =
{  6,  8,  0, 0, {________,
                  __XXX___,
                  _XX_XX__,
                  _XX_____,
                  _XX_____,
                  XXXXX___,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_103 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXX_XX_,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXXX__,
                  ____XX__, // <---- baseline
                  XX__XX__,
                  _XXXX___} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_104 =
{  7,  8,  0, 0, {________,
                  XXX_____,
                  _XX_____,
                  _XX_____,
                  _XX_XX__,
                  _XXX_XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_105 =
{  6,  8,  1, 0, {________,
                  __XX____,
                  __XX____,
                  ________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_106 =
{  6,  8,  0, 0, {________,
                  ____XX__,
                  ____XX__,
                  ________,
                  __XXXX__,
                  ____XX__,
                  ____XX__,
                  ____XX__,
                  ____XX__,
                  XX__XX__, // <---- baseline
                  XX__XX__,
                  _XXXX___} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_107 =
{  7,  8,  0, 0, {________,
                  XXX_____,
                  _XX_____,
                  _XX_____,
                  _XX__XX_,
                  _XX_XX__,
                  _XXXX___,
                  _XX_XX__,
                  _XX__XX_,
                  XXX__XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_108 =
{  6,  8,  1, 0, {________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_109 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXX__,
                  XX_X_XX_,
                  XX_X_XX_,
                  XX_X_XX_,
                  XX_X_XX_,
                  XX___XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_110 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_111 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_112 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX_XXX__,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XXXXX__, // <---- baseline
                  _XX_____,
                  XXXX____} };


// Character: 8,12 .10.. (0,6) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_113 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXX_XX_,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXXX__, // <---- baseline
                  ____XX__,
                  ___XXXX_} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_114 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXX_XX__,
                  _XX_XXX_,
                  _XXX_XX_,
                  _XX_____,
                  _XX_____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_115 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  _XX_____,
                  ___XX___,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_116 =
{  6,  8,  0, 0, {________,
                  ________,
                  __X_____,
                  _XX_____,
                  XXXXXX__,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  _XX_XX__,
                  __XXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_117 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_118 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_119 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX___XX_,
                  XX___XX_,
                  XX_X_XX_,
                  XX_X_XX_,
                  _XX_XX__,
                  _XX_XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_120 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX___XX_,
                  _XX_XX__,
                  __XXX___,
                  __XXX___,
                  _XX_XX__,
                  XX___XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_121 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  __XXXX__,
                  ____XX__, // <---- baseline
                  ___XX___,
                  XXXX____} };


// Character: 8,12 .10.. (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_122 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXX__,
                  X___XX__,
                  ___XX___,
                  _XX_____,
                  XX___X__,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_123 =
{  6,  8,  0, 0, {________,
                  ___XXX__,
                  __XX____,
                  __XX____,
                  _XX_____,
                  XX______,
                  _XX_____,
                  __XX____,
                  __XX____,
                  ___XXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,9) to 2 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_124 =
{  2,  8,  3, 0, {________,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  ________,
                  XX______,
                  XX______,
                  XX______,
                  XX______, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_125 =
{  6,  8,  0, 0, {________,
                  XXX_____,
                  __XX____,
                  __XX____,
                  ___XX___,
                  ____XX__,
                  ___XX___,
                  __XX____,
                  __XX____,
                  XXX_____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_126 =
{  8,  8,  0, 0, {________,
                  _XXX__XX,
                  XX_XX_X_,
                  XX__XXX_,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,7) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_127 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ___X____,
                  __XXX___,
                  _XX_XX__,
                  XX___XX_,
                  XX___XX_,
                  XXXXXXX_,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_128 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX______,
                  XX______,
                  XX______,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  __XX____,
                  XXXX____} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_129 =
{  7,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_130 =
{  6,  8,  0, 0, {____XX__,
                  ___XX___,
                  __XX____,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XXXXXX__,
                  XX______,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_131 =
{  7,  8,  0, 0, {__XX____,
                  _XXXX___,
                  XX__XX__,
                  ________,
                  _XXXX___,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_132 =
{  7,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  ________,
                  _XXXX___,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_133 =
{  7,  8,  0, 0, {XX______,
                  _XX_____,
                  __XX____,
                  ________,
                  _XXXX___,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_134 =
{  7,  8,  0, 0, {__XXX___,
                  _XX_XX__,
                  _XX_XX__,
                  __XXX___,
                  XXXXX___,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_135 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX______,
                  XX______,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  __XX____,
                  XXXX____} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_136 =
{  6,  8,  0, 0, {__XX____,
                  _XXXX___,
                  XX__XX__,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XXXXXX__,
                  XX______,
                  XX______,
                  _XXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_137 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XXXXXX__,
                  XX______,
                  XX______,
                  _XXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_138 =
{  6,  8,  0, 0, {XX______,
                  _XX_____,
                  __XX____,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XXXXXX__,
                  XX______,
                  XX______,
                  _XXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_139 =
{  6,  8,  1, 0, {________,
                  XX_XX___,
                  XX_XX___,
                  ________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_140 =
{  6,  8,  1, 0, {__X_____,
                  _XXX____,
                  XX_XX___,
                  ________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_141 =
{  6,  8,  1, 0, {XX______,
                  _XX_____,
                  __XX____,
                  ________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_142 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  ________,
                  __XX____,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XXXXXX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_143 =
{  6,  8,  0, 0, {_XXXX___,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XXXXXX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_144 =
{  6,  8,  0, 0, {____XX__,
                  ___XX___,
                  __XX____,
                  XXXXXX__,
                  XX___X__,
                  XX______,
                  XXXXX___,
                  XX______,
                  XX___X__,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_145 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  ___XX_XX,
                  _XXXXXXX,
                  XX_XX___,
                  XX_XX___,
                  XXX_XXXX, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_146 =
{  7,  8,  0, 0, {________,
                  __XXXXX_,
                  _XXXX___,
                  XX_XX___,
                  XX_XX___,
                  XXXXXXX_,
                  XX_XX___,
                  XX_XX___,
                  XX_XX___,
                  XX_XXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_147 =
{  6,  8,  0, 0, {__XX____,
                  _XXXX___,
                  XX__XX__,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_148 =
{  6,  8,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_149 =
{  6,  8,  0, 0, {XX______,
                  _XX_____,
                  __XX____,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_150 =
{  7,  8,  0, 0, {__XX____,
                  _XXXX___,
                  XX__XX__,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_151 =
{  7,  8,  0, 0, {XX______,
                  _XX_____,
                  __XX____,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_152 =
{  7,  8,  0, 0, {________,
                  _XX__XX_,
                  _XX__XX_,
                  ________,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  __XXXX__,
                  ____XX__, // <---- baseline
                  ___XX___,
                  XXXX____} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_153 =
{  6,  8,  0, 0, {XX__XX__,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_154 =
{  6,  8,  0, 0, {XX__XX__,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_155 =
{  6,  8,  0, 0, {________,
                  __XX____,
                  __XX____,
                  _XXXX___,
                  XX__XX__,
                  XX______,
                  XX______,
                  XX__XX__,
                  _XXXX___,
                  __XX____, // <---- baseline
                  __XX____,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_156 =
{  7,  8,  0, 0, {__XXXX__,
                  _XX__XX_,
                  _XX_____,
                  _XX_____,
                  _XX_____,
                  XXXXXX__,
                  _XX_____,
                  _XX_____,
                  XX______,
                  XXXXXXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_157 =
{  6,  8,  0, 0, {XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  XXXXXX__,
                  __XX____,
                  XXXXXX__,
                  __XX____,
                  __XX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_158 =
{  8,  8,  0, 0, {XXXX____,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____,
                  X___X___,
                  X__XXXX_,
                  X___XX__,
                  X___XX_X,
                  X____XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_159 =
{  8,  8,  0, 0, {____XXX_,
                  ___XX_XX,
                  ___XX___,
                  ___XX___,
                  _XXXXXX_,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XX_XX___,
                  _XXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_160 =
{  7,  8,  0, 0, {____XX__,
                  ___XX___,
                  __XX____,
                  ________,
                  _XXXX___,
                  ____XX__,
                  _XXXXX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_161 =
{  6,  8,  1, 0, {___XX___,
                  __XX____,
                  _XX_____,
                  ________,
                  XXXX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_162 =
{  6,  8,  0, 0, {____XX__,
                  ___XX___,
                  __XX____,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_163 =
{  7,  8,  0, 0, {____XX__,
                  ___XX___,
                  __XX____,
                  ________,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_164 =
{  7,  8,  0, 0, {________,
                  _XXX_XX_,
                  XX_XXX__,
                  ________,
                  XXXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_165 =
{  7,  8,  0, 0, {_XXX_XX_,
                  XX_XXX__,
                  ________,
                  XX___XX_,
                  XXX__XX_,
                  XXXX_XX_,
                  XX_XXXX_,
                  XX__XXX_,
                  XX___XX_,
                  XX___XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_166 =
{  7,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  _XXXXXX_,
                  ________,
                  XXXXXXX_,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_167 =
{  7,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  ________,
                  XXXXXXX_,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_168 =
{  6,  8,  0, 0, {________,
                  __XX____,
                  __XX____,
                  ________,
                  __XX____,
                  _XX_____,
                  XX______,
                  XX______,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,5) to 6 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_169 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXX__,
                  XX______,
                  XX______,
                  XX______,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,5) to 6 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_170 =
{  6,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXX__,
                  ____XX__,
                  ____XX__,
                  ____XX__,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_171 =
{  8,  8,  0, 0, {________,
                  _X____X_,
                  XX___XX_,
                  XX__XX__,
                  XX_XX___,
                  __XX____,
                  _XX_XXX_,
                  XX____XX,
                  X____XX_,
                  ____XX__, // <---- baseline
                  ___XXXXX,
                  ________} };


// Character: 8,12 .10.. (0,9) to 8 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_172 =
{  8,  8,  0, 0, {________,
                  _XX___XX,
                  XXX__XX_,
                  _XX_XX__,
                  _XXXX___,
                  __XX_XXX,
                  _XX_XXXX,
                  XX_XX_XX,
                  X_XX__XX,
                  __XXXXXX, // <---- baseline
                  ______XX,
                  ________} };


// Character: 8,12 .10.. (1,9) to 4 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_173 =
{  4,  8,  1, 0, {________,
                  _XX_____,
                  _XX_____,
                  ________,
                  _XX_____,
                  _XX_____,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  _XX_____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_174 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  __XX__XX,
                  _XX__XX_,
                  XX__XX__,
                  XX__XX__,
                  _XX__XX_,
                  __XX__XX, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_175 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XX__XX__,
                  _XX__XX_,
                  __XX__XX,
                  __XX__XX,
                  _XX__XX_,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_176 =
{  8,  8,  0, 0, {__X__X__,
                  X__X__X_,
                  _X__X__X,
                  __X__X__,
                  X__X__X_,
                  _X__X__X,
                  __X__X__,
                  X__X__X_,
                  _X__X__X,
                  __X__X__, // <---- baseline
                  X__X__X_,
                  _X__X__X} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_177 =
{  8,  8,  0, 0, {_X_X_X_X,
                  X_X_X_X_,
                  _X_X_X_X,
                  X_X_X_X_,
                  _X_X_X_X,
                  X_X_X_X_,
                  _X_X_X_X,
                  X_X_X_X_,
                  _X_X_X_X,
                  X_X_X_X_, // <---- baseline
                  _X_X_X_X,
                  X_X_X_X_} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_178 =
{  8,  8,  0, 0, {_XX_XX_X,
                  XX_XX_XX,
                  X_XX_XX_,
                  _XX_XX_X,
                  XX_XX_XX,
                  X_XX_XX_,
                  _XX_XX_X,
                  XX_XX_XX,
                  X_XX_XX_,
                  _XX_XX_X, // <---- baseline
                  XX_XX_XX,
                  X_XX_XX_} };


// Character: 8,12 .10.. (3,10) to 2 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_179 =
{  2,  8,  3, 0, {XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______, // <---- baseline
                  XX______,
                  XX______} };


// Character: 8,12 .10.. (0,10) to 5 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_180 =
{  5,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (0,10) to 5 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_181 =
{  5,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (0,10) to 7 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_182 =
{  7,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (0,5) to 7 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_183 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (0,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_184 =
{  5,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (0,10) to 7 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_185 =
{  7,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XX_,
                  _____XX_,
                  _____XX_,
                  XXX__XX_,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (1,10) to 6 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_186 =
{  6,  8,  1, 0, {XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  XX__XX__,
                  XX__XX__} };


// Character: 8,12 .10.. (0,6) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_187 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  _____XX_,
                  _____XX_,
                  XXX__XX_,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (0,10) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_188 =
{  7,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XX_,
                  _____XX_,
                  _____XX_,
                  XXXXXXX_,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_189 =
{  7,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXXXXXX_,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_190 =
{  5,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,5) to 5 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_191 =
{  5,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (3,10) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_192 =
{  5,  8,  3, 0, {XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_193 =
{  8,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXXXXX,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,5) to 8 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_194 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (3,10) to 5 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_195 =
{  5,  8,  3, 0, {XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XXXXX___,
                  XX______,
                  XX______,
                  XX______,
                  XX______, // <---- baseline
                  XX______,
                  XX______} };


// Character: 8,12 .10.. (0,5) to 8 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_196 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_197 =
{  8,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXXXXX,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (3,10) to 5 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_198 =
{  5,  8,  3, 0, {XX______,
                  XX______,
                  XX______,
                  XX______,
                  XXXXX___,
                  XX______,
                  XX______,
                  XXXXX___,
                  XX______,
                  XX______, // <---- baseline
                  XX______,
                  XX______} };


// Character: 8,12 .10.. (1,10) to 7 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_199 =
{  7,  8,  1, 0, {XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XXX_,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  XX__XX__,
                  XX__XX__} };


// Character: 8,12 .10.. (1,10) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_200 =
{  7,  8,  1, 0, {XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XXX_,
                  XX______,
                  XX______,
                  XXXXXXX_,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,6) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_201 =
{  7,  8,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  XX______,
                  XX______,
                  XX__XXX_,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  XX__XX__,
                  XX__XX__} };


// Character: 8,12 .10.. (0,10) to 8 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_202 =
{  8,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XXX,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 8 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_203 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________,
                  XXX__XXX,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (1,10) to 7 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_204 =
{  7,  8,  1, 0, {XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XXX_,
                  XX______,
                  XX______,
                  XX__XXX_,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  XX__XX__,
                  XX__XX__} };


// Character: 8,12 .10.. (0,6) to 8 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_205 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_206 =
{  8,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XXX,
                  ________,
                  ________,
                  XXX__XXX,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (0,10) to 8 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_207 =
{  8,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXXXXX,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_208 =
{  8,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXXXXXXX,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 8 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_209 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  ________,
                  ________,
                  XXXXXXXX,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (0,5) to 8 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_210 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (1,10) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_211 =
{  7,  8,  1, 0, {XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XXXXXXX_,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,10) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_212 =
{  5,  8,  3, 0, {XX______,
                  XX______,
                  XX______,
                  XX______,
                  XXXXX___,
                  XX______,
                  XX______,
                  XXXXX___,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_213 =
{  5,  8,  3, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  XX______,
                  XX______,
                  XXXXX___,
                  XX______,
                  XX______, // <---- baseline
                  XX______,
                  XX______} };


// Character: 8,12 .10.. (1,5) to 7 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_214 =
{  7,  8,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  XX__XX__,
                  XX__XX__} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_215 =
{  8,  8,  0, 0, {_XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  XXX__XXX,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_, // <---- baseline
                  _XX__XX_,
                  _XX__XX_} };


// Character: 8,12 .10.. (0,10) to 8 x 12
static struct{ char s, w, o, j; unsigned char data[12]; } _char_216 =
{  8,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXXXXX,
                  ________,
                  ________,
                  XXXXXXXX,
                  ___XX___,
                  ___XX___, // <---- baseline
                  ___XX___,
                  ___XX___} };


// Character: 8,12 .10.. (0,10) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_217 =
{  5,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,5) to 5 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_218 =
{  5,  8,  3, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  XX______,
                  XX______,
                  XX______,
                  XX______, // <---- baseline
                  XX______,
                  XX______} };


// Character: 8,12 .10.. (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_219 =
{  0,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,4) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_220 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXXX,
                  XXXXXXXX,
                  XXXXXXXX,
                  XXXXXXXX, // <---- baseline
                  XXXXXXXX,
                  XXXXXXXX} };


// Character: 8,12 .10.. (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_221 =
{  0,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_222 =
{  0,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,10) to 8 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_223 =
{  8,  8,  0, 0, {XXXXXXXX,
                  XXXXXXXX,
                  XXXXXXXX,
                  XXXXXXXX,
                  XXXXXXXX,
                  XXXXXXXX,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_224 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXX_XX_,
                  XX_XXXX_,
                  XX__XX__,
                  XX__XX__,
                  XX_XXXX_,
                  _XXX_XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_225 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX_XX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XXXXX___,
                  XX______, // <---- baseline
                  _XX_____,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_226 =
{  6,  8,  0, 0, {________,
                  XXXXXX__,
                  XX__XX__,
                  XX__XX__,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_227 =
{  7,  8,  0, 0, {________,
                  XXXXXXX_,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  _XX__XX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_228 =
{  6,  8,  0, 0, {________,
                  XXXXXX__,
                  XX___X__,
                  _XX__X__,
                  _XX_____,
                  __XX____,
                  _XX_____,
                  _XX__X__,
                  XX___X__,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_229 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XXXXXX_,
                  XX__X___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,6) to 8 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_230 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XX__XX_,
                  _XXXX_XX, // <---- baseline
                  _XX_____,
                  XX______} };


// Character: 8,12 .10.. (0,7) to 7 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_231 =
{  7,  8,  0, 0, {________,
                  ________,
                  ________,
                  _XXX_XX_,
                  XX_XXX__,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ____XXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_232 =
{  6,  8,  0, 0, {________,
                  XXXXXX__,
                  __XX____,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  __XX____,
                  XXXXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_233 =
{  6,  8,  0, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XXXXXX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_234 =
{  7,  8,  0, 0, {________,
                  _XXXXX__,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  XX___XX_,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  XXX_XXX_, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_235 =
{  6,  8,  0, 0, {________,
                  __XXXX__,
                  _XX_____,
                  __XX____,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,7) to 8 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_236 =
{  8,  8,  0, 0, {________,
                  ________,
                  ________,
                  _XXX_XX_,
                  XX_XX_XX,
                  XX_XX_XX,
                  XX_XX_XX,
                  _XX_XXX_,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 7 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_237 =
{  7,  8,  0, 0, {________,
                  ________,
                  _____XX_,
                  _XXXXX__,
                  XX_XXXX_,
                  XX_X_XX_,
                  XXXX_XX_,
                  _XXXXX__,
                  XX______,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_238 =
{  6,  8,  0, 0, {________,
                  __XXXX__,
                  _XX_____,
                  XX______,
                  XX______,
                  XXXXXX__,
                  XX______,
                  XX______,
                  _XX_____,
                  __XXXX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_239 =
{  6,  8,  0, 0, {________,
                  ________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 6 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_240 =
{  6,  8,  0, 0, {________,
                  ________,
                  XXXXXX__,
                  ________,
                  ________,
                  XXXXXX__,
                  ________,
                  ________,
                  XXXXXX__,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 6 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_241 =
{  6,  8,  0, 0, {________,
                  ________,
                  __XX____,
                  __XX____,
                  XXXXXX__,
                  __XX____,
                  __XX____,
                  ________,
                  XXXXXX__,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_242 =
{  6,  8,  0, 0, {________,
                  _XX_____,
                  __XX____,
                  ___XX___,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  ________,
                  XXXXXX__,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_243 =
{  6,  8,  0, 0, {________,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  _XX_____,
                  __XX____,
                  ___XX___,
                  ________,
                  XXXXXX__,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,8) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_244 =
{  5,  8,  3, 0, {________,
                  ________,
                  _XXX____,
                  XX_XX___,
                  XX_XX___,
                  XX______,
                  XX______,
                  XX______,
                  XX______,
                  XX______, // <---- baseline
                  XX______,
                  XX______} };


// Character: 8,12 .10.. (0,10) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_245 =
{  5,  8,  0, 0, {___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  ___XX___,
                  XX_XX___,
                  XX_XX___,
                  _XXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 6 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_246 =
{  6,  8,  0, 0, {________,
                  ________,
                  __XX____,
                  __XX____,
                  ________,
                  XXXXXX__,
                  ________,
                  __XX____,
                  __XX____,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,8) to 8 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_247 =
{  8,  8,  0, 0, {________,
                  ________,
                  _XXX__XX,
                  XX_XX_XX,
                  XX__XXX_,
                  ________,
                  _XXX__XX,
                  XX_XX_XX,
                  XX__XXX_,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 6 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_248 =
{  6,  8,  1, 0, {________,
                  _XXXX___,
                  XX__XX__,
                  XX__XX__,
                  XX__XX__,
                  _XXXX___,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,6) to 3 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_249 =
{  3,  8,  3, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (3,5) to 2 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_250 =
{  2,  8,  3, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XX______,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_251 =
{  7,  8,  1, 0, {________,
                  ____XXX_,
                  ____X___,
                  ____X___,
                  ____X___,
                  X___X___,
                  XX__X___,
                  _XX_X___,
                  __XXX___,
                  ___XX___, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,9) to 6 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_252 =
{  6,  8,  0, 0, {________,
                  XX_XX___,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  _XX_XX__,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (1,9) to 5 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_253 =
{  5,  8,  1, 0, {________,
                  XXXX____,
                  ___XX___,
                  __XX____,
                  _XX_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (2,8) to 4 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_254 =
{  4,  8,  2, 0, {________,
                  ________,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  XXXX____,
                  XXXX____, // <---- baseline
                  ________,
                  ________} };


// Character: 8,12 .10.. (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_255 =
{  0,  8,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________} };


struct { unsigned short height, chars;
         PCHARACTER character[256]; } Terminal12by8 = { 12, 256, {  (PCHARACTER)&_char_0
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

} };// $Log: $
