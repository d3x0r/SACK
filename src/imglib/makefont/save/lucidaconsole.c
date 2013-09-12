#include "symbits.h"
#include "fontstruct.h"

// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_0 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_1 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_2 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_3 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_4 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_5 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_6 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_7 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_8 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_9 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_10 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_11 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_12 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_13 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_14 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_15 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_16 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_17 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_18 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_19 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_20 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_21 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_22 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_23 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_24 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_25 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_26 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_27 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_28 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_29 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_30 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_31 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_32 =
{  0,  7,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,8) to 1 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_33 =
{  1,  7,  3, 0, {________,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  ________,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_34 =
{  4,  7,  1, 0, {X__X____,
                  X__X____,
                  X__X____,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_35 =
{  7,  7,  0, 0, {________,
                  ___X_X__,
                  ___X_X__,
                  _XXXXXX_,
                  __X_X___,
                  __X_X___,
                  XXXXXX__,
                  _X_X____,
                  _X_X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_36 =
{  5,  7,  1, 0, {__X_____,
                  _XXXX___,
                  X_X_____,
                  X_X_____,
                  _XX_____,
                  __XX____,
                  __X_X___,
                  __X_X___,
                  XXXX____, // <---- baseline
                  __X_____,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_37 =
{  7,  7,  0, 0, {________,
                  _XX___X_,
                  X__X_X__,
                  X__XX___,
                  _XXX____,
                  ___XXX__,
                  __XX__X_,
                  _X_X__X_,
                  X___XX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_38 =
{  7,  7,  0, 0, {________,
                  ___XXX__,
                  __X__X__,
                  __X_XX__,
                  __XX____,
                  XX_X__X_,
                  X___X_X_,
                  XX__XX__,
                  _XXXXXX_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,9) to 1 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_39 =
{  1,  7,  3, 0, {X_______,
                  X_______,
                  X_______,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,9) to 4 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_40 =
{  4,  7,  2, 0, {__XX____,
                  _XX_____,
                  _X______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  _X______, // <---- baseline
                  _XX_____,
                  __XX____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_41 =
{  4,  7,  1, 0, {X_______,
                  _XX_____,
                  __X_____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  __X_____, // <---- baseline
                  _XX_____,
                  XX______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_42 =
{  5,  7,  1, 0, {________,
                  __X_____,
                  XX_XX___,
                  _XX_____,
                  _X_X____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_43 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  ___X____,
                  ___X____,
                  XXXXXXX_,
                  ___X____,
                  ___X____,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,2) to 2 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_44 =
{  2,  7,  2, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XX______,
                  XX______, // <---- baseline
                  _X______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,4) to 5 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_45 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,2) to 2 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_46 =
{  2,  7,  2, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XX______,
                  XX______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_47 =
{  7,  7,  0, 0, {______X_,
                  _____X__,
                  _____X__,
                  ____X___,
                  ____X___,
                  ___X____,
                  __X_____,
                  __X_____,
                  _X______, // <---- baseline
                  _X______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_48 =
{  6,  7,  0, 0, {________,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_49 =
{  5,  7,  1, 0, {________,
                  __X_____,
                  XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_50 =
{  5,  7,  1, 0, {________,
                  XXXX____,
                  ____X___,
                  ____X___,
                  ___XX___,
                  __X_____,
                  _X______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 4 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_51 =
{  4,  7,  1, 0, {________,
                  XXXX____,
                  ___X____,
                  ___X____,
                  _XX_____,
                  ___X____,
                  ___X____,
                  ___X____,
                  XXX_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_52 =
{  6,  7,  0, 0, {________,
                  ____X___,
                  ___XX___,
                  __X_X___,
                  _X__X___,
                  X___X___,
                  XXXXXX__,
                  ____X___,
                  ____X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 4 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_53 =
{  4,  7,  1, 0, {________,
                  XXXX____,
                  X_______,
                  X_______,
                  XXX_____,
                  ___X____,
                  ___X____,
                  ___X____,
                  XXX_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_54 =
{  5,  7,  1, 0, {________,
                  __XXX___,
                  _X______,
                  X_______,
                  X_XX____,
                  XX__X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_55 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  ____X___,
                  ___X____,
                  __X_____,
                  _X______,
                  _X______,
                  X_______,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_56 =
{  5,  7,  1, 0, {________,
                  _XXX____,
                  X___X___,
                  X___X___,
                  _XXX____,
                  X__X____,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_57 =
{  5,  7,  1, 0, {________,
                  _XXX____,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXXX___,
                  ____X___,
                  ___X____,
                  XXX_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,6) to 2 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_58 =
{  2,  7,  3, 0, {________,
                  ________,
                  ________,
                  XX______,
                  XX______,
                  ________,
                  ________,
                  XX______,
                  XX______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,6) to 2 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_59 =
{  2,  7,  2, 0, {________,
                  ________,
                  ________,
                  XX______,
                  XX______,
                  ________,
                  ________,
                  XX______,
                  XX______, // <---- baseline
                  _X______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_60 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  ____X___,
                  __XX____,
                  _X______,
                  _X______,
                  __XX____,
                  ____X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,5) to 5 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_61 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  ________,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_62 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X_______,
                  _XX_____,
                  ___X____,
                  ___X____,
                  _XX_____,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_63 =
{  5,  7,  1, 0, {________,
                  XXXX____,
                  X___X___,
                  ____X___,
                  ___X____,
                  __X_____,
                  __X_____,
                  ________,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_64 =
{  7,  7,  0, 0, {________,
                  __XXX___,
                  _X___X__,
                  X__XXX__,
                  X_X__X__,
                  X_X_XX__,
                  X_XX_XX_,
                  _X______,
                  __XXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_65 =
{  7,  7,  0, 0, {________,
                  ___X____,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_66 =
{  5,  7,  1, 0, {________,
                  XXXX____,
                  X___X___,
                  X___X___,
                  XXXX____,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_67 =
{  6,  7,  0, 0, {________,
                  __XXXX__,
                  _X______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  _X______,
                  __XXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_68 =
{  6,  7,  0, 0, {________,
                  XXXX____,
                  X___X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  X___X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_69 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X_______,
                  X_______,
                  X_______,
                  XXXX____,
                  X_______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_70 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X_______,
                  X_______,
                  X_______,
                  XXXX____,
                  X_______,
                  X_______,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_71 =
{  6,  7,  0, 0, {________,
                  __XXXX__,
                  _X______,
                  X_______,
                  X_______,
                  X__XXX__,
                  X____X__,
                  _X___X__,
                  __XXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_72 =
{  5,  7,  1, 0, {________,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_73 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_74 =
{  5,  7,  0, 0, {________,
                  _XXXX___,
                  ____X___,
                  ____X___,
                  ____X___,
                  ____X___,
                  ____X___,
                  ____X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_75 =
{  6,  7,  1, 0, {________,
                  X___X___,
                  X__X____,
                  X_X_____,
                  XX______,
                  XX______,
                  X_X_____,
                  X__X____,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_76 =
{  5,  7,  1, 0, {________,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_77 =
{  6,  7,  0, 0, {________,
                  XX__XX__,
                  XX__XX__,
                  XX_X_X__,
                  X_XX_X__,
                  X_XX_X__,
                  X_X__X__,
                  X____X__,
                  X____X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_78 =
{  5,  7,  1, 0, {________,
                  X___X___,
                  XX__X___,
                  XX__X___,
                  X_X_X___,
                  X_X_X___,
                  X__XX___,
                  X__XX___,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_79 =
{  6,  7,  0, 0, {________,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_80 =
{  5,  7,  1, 0, {________,
                  XXXX____,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____,
                  X_______,
                  X_______,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_81 =
{  7,  7,  0, 0, {________,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  __XXX___, // <---- baseline
                  ____X___,
                  _____XX_,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_82 =
{  6,  7,  1, 0, {________,
                  XXXX____,
                  X___X___,
                  X___X___,
                  X__X____,
                  XXX_____,
                  X_X_____,
                  X__X____,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_83 =
{  5,  7,  1, 0, {________,
                  _XXXX___,
                  X_______,
                  X_______,
                  _XX_____,
                  ___X____,
                  ____X___,
                  ____X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_84 =
{  7,  7,  0, 0, {________,
                  XXXXXXX_,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_85 =
{  5,  7,  1, 0, {________,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_86 =
{  7,  7,  0, 0, {________,
                  X_____X_,
                  _X___X__,
                  _X___X__,
                  _X___X__,
                  __X_X___,
                  __X_X___,
                  __XXX___,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_87 =
{  7,  7,  0, 0, {________,
                  X_____X_,
                  X_____X_,
                  X__X__X_,
                  X__X__X_,
                  _XX_X_X_,
                  _XX_XX__,
                  _XX_XX__,
                  _X___X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_88 =
{  7,  7,  0, 0, {________,
                  X_____X_,
                  _X___X__,
                  __X_X___,
                  ___X____,
                  ___X____,
                  __X_X___,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_89 =
{  7,  7,  0, 0, {________,
                  X_____X_,
                  _X___X__,
                  __X_X___,
                  __X_X___,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_90 =
{  6,  7,  0, 0, {________,
                  XXXXXX__,
                  _____X__,
                  ____X___,
                  ___X____,
                  __X_____,
                  _X______,
                  X_______,
                  XXXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,9) to 4 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_91 =
{  4,  7,  3, 0, {XXXX____,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______, // <---- baseline
                  X_______,
                  XXXX____,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_92 =
{  7,  7,  0, 0, {X_______,
                  _X______,
                  _X______,
                  __X_____,
                  __X_____,
                  ___X____,
                  ____X___,
                  ____X___,
                  _____X__, // <---- baseline
                  _____X__,
                  ______X_,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_93 =
{  4,  7,  1, 0, {XXXX____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____, // <---- baseline
                  ___X____,
                  XXXX____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_94 =
{  5,  7,  1, 0, {__X_____,
                  __X_____,
                  _XX_____,
                  _X_X____,
                  _X_X____,
                  X__X____,
                  X___X___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,0) to 7 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_95 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  XXXXXXX_,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,9) to 2 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_96 =
{  2,  7,  2, 0, {X_______,
                  _X______,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_97 =
{  6,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_98 =
{  5,  7,  1, 0, {X_______,
                  X_______,
                  X_______,
                  X_XX____,
                  XX__X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_99 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXXX___,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_100 =
{  5,  7,  1, 0, {____X___,
                  ____X___,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X__XX___,
                  _XX_X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_101 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXX____,
                  X___X___,
                  XXXXX___,
                  X_______,
                  X_______,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_102 =
{  6,  7,  0, 0, {___XXX__,
                  __X_____,
                  __X_____,
                  XXXXXX__,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_103 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXXX___, // <---- baseline
                  ____X___,
                  _XXX____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_104 =
{  5,  7,  1, 0, {X_______,
                  X_______,
                  X_______,
                  X_XXX___,
                  XX__X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 3 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_105 =
{  3,  7,  1, 0, {__X_____,
                  ________,
                  ________,
                  XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_106 =
{  4,  7,  1, 0, {___X____,
                  ________,
                  ________,
                  XXXX____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____, // <---- baseline
                  ___X____,
                  XXX_____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_107 =
{  6,  7,  1, 0, {X_______,
                  X_______,
                  X_______,
                  X___X___,
                  X__X____,
                  XXX_____,
                  X_X_____,
                  X__X____,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 3 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_108 =
{  3,  7,  1, 0, {XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_109 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  X_XX_XX_,
                  XX_XX_X_,
                  X__X__X_,
                  X__X__X_,
                  X__X__X_,
                  X__X__X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_110 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X_XXX___,
                  XX__X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_111 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  _XXXX___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_112 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X_XX____,
                  XX__X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____, // <---- baseline
                  X_______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_113 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXXX___, // <---- baseline
                  ____X___,
                  ____X___,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_114 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X_XXX___,
                  XX__X___,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_115 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXXX___,
                  X_______,
                  XXX_____,
                  ___XX___,
                  ____X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_116 =
{  6,  7,  0, 0, {________,
                  __X_____,
                  __X_____,
                  XXXXXX__,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ___XXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_117 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X__XX___,
                  XXX_X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_118 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  X_____X_,
                  _X___X__,
                  _X___X__,
                  __X_X___,
                  __X_X___,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_119 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  X_____X_,
                  X__X__X_,
                  X_X_X_X_,
                  X_X_X_X_,
                  _XX_XX__,
                  _X___X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_120 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  X____X__,
                  _X__X___,
                  __XX____,
                  __XX____,
                  _X__X___,
                  X____X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_121 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  X____X__,
                  _X__X___,
                  _X__X___,
                  __XX____,
                  __XX____,
                  __X_____, // <---- baseline
                  __X_____,
                  XX______,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_122 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  XXXXXX__,
                  ____X___,
                  ___X____,
                  __X_____,
                  _X______,
                  XXXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_123 =
{  5,  7,  1, 0, {__XXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XX______,
                  __X_____,
                  __X_____,
                  __X_____, // <---- baseline
                  __X_____,
                  ___XX___,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,9) to 1 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_124 =
{  1,  7,  3, 0, {X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  X_______, // <---- baseline
                  X_______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_125 =
{  5,  7,  1, 0, {XX______,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ___XX___,
                  __X_____,
                  __X_____,
                  __X_____, // <---- baseline
                  __X_____,
                  XXX_____,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,4) to 7 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_126 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  _XXX__X_,
                  X__XXX__,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_127 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_128 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_129 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,2) to 2 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_130 =
{  2,  7,  3, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XX______,
                  XX______, // <---- baseline
                  _X______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_131 =
{  7,  7,  0, 0, {____XXX_,
                  ___X____,
                  ___X____,
                  __XXXX__,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____, // <---- baseline
                  ___X____,
                  XXX_____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,1) to 4 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_132 =
{  4,  7,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  X__X____, // <---- baseline
                  X__X____,
                  X__X____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,1) to 5 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_133 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  X_X_X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_134 =
{  5,  7,  1, 0, {________,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____, // <---- baseline
                  __X_____,
                  __X_____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_135 =
{  5,  7,  1, 0, {________,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  __X_____, // <---- baseline
                  __X_____,
                  __X_____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_136 =
{  4,  7,  1, 0, {_XX_____,
                  X__X____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_137 =
{  7,  7,  0, 0, {________,
                  _X__X___,
                  X_XX____,
                  X_XX____,
                  _XX_____,
                  __XX_X__,
                  _XX_X_X_,
                  _XX_X_X_,
                  X__X_X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_138 =
{  5,  7,  1, 0, {_X__X___,
                  _XXXX___,
                  X_______,
                  X_______,
                  _XX_____,
                  ___X____,
                  ____X___,
                  ____X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,6) to 3 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_139 =
{  3,  7,  2, 0, {________,
                  ________,
                  ________,
                  __X_____,
                  _X______,
                  X_______,
                  _X______,
                  __X_____,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_140 =
{  7,  7,  0, 0, {________,
                  _XXXXXX_,
                  X__X____,
                  X__X____,
                  X__XXX__,
                  X__X____,
                  X__X____,
                  X__X____,
                  _XXXXXX_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_141 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_142 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_143 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_144 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,9) to 2 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_145 =
{  2,  7,  2, 0, {_X______,
                  X_______,
                  XX______,
                  XX______,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,9) to 2 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_146 =
{  2,  7,  3, 0, {XX______,
                  XX______,
                  _X______,
                  X_______,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_147 =
{  4,  7,  1, 0, {X__X____,
                  X__X____,
                  X__X____,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_148 =
{  4,  7,  1, 0, {X__X____,
                  X__X____,
                  X__X____,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,5) to 3 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_149 =
{  3,  7,  2, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  XXX_____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,4) to 5 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_150 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,4) to 7 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_151 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXXXX_,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,9) to 4 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_152 =
{  4,  7,  2, 0, {_X_X____,
                  X_X_____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_153 =
{  6,  7,  0, 0, {________,
                  XXXX_X__,
                  _X_X_X__,
                  _X_XXX__,
                  _X_X_X__,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_154 =
{  5,  7,  1, 0, {_X__X___,
                  __XX____,
                  ________,
                  _XXXX___,
                  X_______,
                  XXX_____,
                  ___XX___,
                  ____X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,6) to 3 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_155 =
{  3,  7,  2, 0, {________,
                  ________,
                  ________,
                  X_______,
                  _X______,
                  __X_____,
                  _X______,
                  X_______,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_156 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  _XX_XX__,
                  X__X__X_,
                  X__XXXX_,
                  X__X____,
                  X__X____,
                  _XX_XXX_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_157 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_158 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_159 =
{  7,  7,  0, 0, {_X__X___,
                  X_____X_,
                  _X___X__,
                  __X_X___,
                  __X_X___,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,0) to 0 x 0
static struct{ char s, w, o, j; unsigned char data[12]; } _char_160 =
{  0,  7,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,6) to 1 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_161 =
{  1,  7,  3, 0, {________,
                  ________,
                  ________,
                  X_______,
                  ________,
                  X_______,
                  X_______,
                  X_______,
                  X_______, // <---- baseline
                  X_______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_162 =
{  5,  7,  1, 0, {________,
                  ___X____,
                  _XXXX___,
                  XX_X____,
                  X__X____,
                  X__X____,
                  XX_X____,
                  _XXXX___,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,8) to 4 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_163 =
{  4,  7,  2, 0, {________,
                  __XX____,
                  _X______,
                  _X______,
                  XXX_____,
                  _X______,
                  _X______,
                  X_______,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_164 =
{  7,  7,  0, 0, {________,
                  X_____X_,
                  _XXXXX__,
                  _X___X__,
                  _X___X__,
                  _X___X__,
                  _XXXXX__,
                  X_____X_,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_165 =
{  7,  7,  0, 0, {________,
                  X_____X_,
                  _X___X__,
                  __X_X___,
                  ___X____,
                  _XXXXX__,
                  ___X____,
                  _XXXXX__,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,9) to 1 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_166 =
{  1,  7,  3, 0, {X_______,
                  X_______,
                  X_______,
                  X_______,
                  ________,
                  ________,
                  ________,
                  X_______,
                  X_______, // <---- baseline
                  X_______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_167 =
{  5,  7,  1, 0, {________,
                  _XXXX___,
                  X_______,
                  XX______,
                  _XXX____,
                  X___X___,
                  XX__X___,
                  _XXX____,
                  ____X___, // <---- baseline
                  ____X___,
                  XXXX____,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 4 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_168 =
{  4,  7,  1, 0, {X__X____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_169 =
{  7,  7,  0, 0, {________,
                  __XXX___,
                  _X___X__,
                  X__XX_X_,
                  X_X___X_,
                  X_X___X_,
                  X__XX_X_,
                  _X___X__,
                  __XXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_170 =
{  5,  7,  1, 0, {________,
                  XXXX____,
                  ___X____,
                  _XXX____,
                  X__X____,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_171 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  __X_X___,
                  _X_X____,
                  X_X_____,
                  _X_X____,
                  __X_X___,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,5) to 6 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_172 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXXXXX__,
                  _____X__,
                  _____X__,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,4) to 5 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_173 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  XXXXX___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_174 =
{  6,  7,  0, 0, {________,
                  _XXXX___,
                  X____X__,
                  X_XX_X__,
                  X_XX_X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 1
static struct{ char s, w, o, j; unsigned char data[12]; } _char_175 =
{  7,  7,  0, 0, {XXXXXXX_,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,8) to 3 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_176 =
{  3,  7,  2, 0, {________,
                  _X______,
                  X_X_____,
                  _X______,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,7) to 5 x 7
static struct{ char s, w, o, j; unsigned char data[12]; } _char_177 =
{  5,  7,  1, 0, {________,
                  ________,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  ________,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,8) to 4 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_178 =
{  4,  7,  2, 0, {________,
                  XXXX____,
                  ___X____,
                  _XX_____,
                  XXXX____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,8) to 4 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_179 =
{  4,  7,  2, 0, {________,
                  XXXX____,
                  _XXX____,
                  ___X____,
                  XXXX____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,9) to 2 x 2
static struct{ char s, w, o, j; unsigned char data[12]; } _char_180 =
{  2,  7,  2, 0, {_X______,
                  X_______,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_181 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X__XX___,
                  XXX_X___, // <---- baseline
                  X_______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_182 =
{  5,  7,  1, 0, {________,
                  XXXXX___,
                  XXX_X___,
                  XXX_X___,
                  _XX_X___,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  __X_X___, // <---- baseline
                  __X_X___,
                  __X_X___,
                  ________} };
// Character: 7,12 .9.. 7,0 (2,5) to 3 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_183 =
{  3,  7,  2, 0, {________,
                  ________,
                  ________,
                  ________,
                  XXX_____,
                  XXX_____,
                  XXX_____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (3,0) to 2 x 3
static struct{ char s, w, o, j; unsigned char data[12]; } _char_184 =
{  2,  7,  3, 0, {________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________, // <---- baseline
                  X_______,
                  _X______,
                  XX______} };
// Character: 7,12 .9.. 7,0 (2,8) to 2 x 4
static struct{ char s, w, o, j; unsigned char data[12]; } _char_185 =
{  2,  7,  2, 0, {________,
                  XX______,
                  _X______,
                  _X______,
                  _X______,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_186 =
{  5,  7,  1, 0, {________,
                  _XXX____,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 5
static struct{ char s, w, o, j; unsigned char data[12]; } _char_187 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  X_X_____,
                  _X_X____,
                  __X_X___,
                  _X_X____,
                  X_X_____,
                  ________,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_188 =
{  7,  7,  0, 0, {________,
                  XX___X__,
                  _X__X___,
                  _X_X____,
                  _X_X_X__,
                  __X_XX__,
                  __XX_X__,
                  _X_XXXX_,
                  X____X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_189 =
{  7,  7,  0, 0, {________,
                  XX___X__,
                  _X__X___,
                  _X_X____,
                  _X_XXXX_,
                  __X___X_,
                  __X__X__,
                  _X__X___,
                  X___XXX_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_190 =
{  7,  7,  0, 0, {________,
                  XXX___X_,
                  XXX__X__,
                  __X_X___,
                  XXXX_X__,
                  ___XXX__,
                  __XX_X__,
                  _X_XXXX_,
                  X____X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_191 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  __X_____,
                  ________,
                  __X_____,
                  __X_____,
                  _X______,
                  X_______, // <---- baseline
                  X___X___,
                  _XXXX___,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_192 =
{  7,  7,  0, 0, {_X______,
                  __XX____,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_193 =
{  7,  7,  0, 0, {___X____,
                  __XX____,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_194 =
{  7,  7,  0, 0, {__XX____,
                  _X_XX___,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_195 =
{  7,  7,  0, 0, {___X_X__,
                  __XXX___,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_196 =
{  7,  7,  0, 0, {_X__X___,
                  ___X____,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_197 =
{  7,  7,  0, 0, {__XXX___,
                  ___X____,
                  __X_X___,
                  __X_X___,
                  __X_X___,
                  _X___X__,
                  _XXXXX__,
                  _X___X__,
                  X_____X_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 7 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_198 =
{  7,  7,  0, 0, {________,
                  ___XXXX_,
                  ___XX___,
                  __X_X___,
                  __X_X___,
                  _X__XX__,
                  _XXXX___,
                  _X__X___,
                  X___XXX_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_199 =
{  6,  7,  0, 0, {________,
                  __XXXX__,
                  _X______,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  _X______,
                  __XXXX__, // <---- baseline
                  ____X___,
                  _____X__,
                  ____XX__} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_200 =
{  5,  7,  1, 0, {_X______,
                  XXXXX___,
                  X_______,
                  X_______,
                  X_______,
                  XXXX____,
                  X_______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_201 =
{  5,  7,  1, 0, {___X____,
                  XXXXX___,
                  X_______,
                  X_______,
                  X_______,
                  XXXX____,
                  X_______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_202 =
{  5,  7,  1, 0, {_XX_____,
                  XXXXX___,
                  X_______,
                  X_______,
                  X_______,
                  XXXX____,
                  X_______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_203 =
{  5,  7,  1, 0, {X__X____,
                  XXXXX___,
                  X_______,
                  X_______,
                  X_______,
                  XXXX____,
                  X_______,
                  X_______,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_204 =
{  5,  7,  1, 0, {_X______,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_205 =
{  5,  7,  1, 0, {___X____,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_206 =
{  5,  7,  1, 0, {_XX_____,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_207 =
{  5,  7,  1, 0, {X__X____,
                  XXXXX___,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_208 =
{  6,  7,  0, 0, {________,
                  XXXX____,
                  X___X___,
                  X____X__,
                  XX___X__,
                  X____X__,
                  X____X__,
                  X___X___,
                  XXXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_209 =
{  5,  7,  1, 0, {__X_X___,
                  XX_XX___,
                  XX__X___,
                  XX__X___,
                  X_X_X___,
                  X_X_X___,
                  X__XX___,
                  X__XX___,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_210 =
{  6,  7,  0, 0, {__X_____,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_211 =
{  6,  7,  0, 0, {____X___,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_212 =
{  6,  7,  0, 0, {__XX____,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_213 =
{  6,  7,  0, 0, {___X_X__,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_214 =
{  6,  7,  0, 0, {_X__X___,
                  _XXXX___,
                  _X__X___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _X__X___,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_215 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  X____X__,
                  _X__X___,
                  __XX____,
                  __XX____,
                  _X__X___,
                  X____X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_216 =
{  6,  7,  0, 0, {________,
                  _XXXXX__,
                  _X__X___,
                  X___XX__,
                  X__X_X__,
                  X_X__X__,
                  XX___X__,
                  _X__X___,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_217 =
{  5,  7,  1, 0, {_X______,
                  X_X_X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_218 =
{  5,  7,  1, 0, {___X____,
                  X_X_X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_219 =
{  5,  7,  1, 0, {_XX_____,
                  X__XX___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_220 =
{  5,  7,  1, 0, {X__X____,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  _XXX____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 7 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_221 =
{  7,  7,  0, 0, {____X___,
                  X__X__X_,
                  _X___X__,
                  __X_X___,
                  __X_X___,
                  ___X____,
                  ___X____,
                  ___X____,
                  ___X____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_222 =
{  5,  7,  1, 0, {________,
                  X_______,
                  XXXX____,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____,
                  X_______,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_223 =
{  6,  7,  1, 0, {XXXX____,
                  X__X____,
                  X__X____,
                  X_X_____,
                  X_X_____,
                  X__X____,
                  X___X___,
                  X____X__,
                  X_XXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_224 =
{  6,  7,  1, 0, {_X______,
                  __X_____,
                  ________,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_225 =
{  6,  7,  1, 0, {___X____,
                  __X_____,
                  ________,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_226 =
{  6,  7,  1, 0, {__XX____,
                  _X__X___,
                  ________,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_227 =
{  6,  7,  1, 0, {__X_X___,
                  _X_X____,
                  ________,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_228 =
{  6,  7,  1, 0, {________,
                  _X_X____,
                  ________,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_229 =
{  6,  7,  1, 0, {__X_____,
                  _X_X____,
                  __X_____,
                  _XXX____,
                  ____X___,
                  _XXXX___,
                  X___X___,
                  X___X___,
                  _XXXXX__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 7 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_230 =
{  7,  7,  0, 0, {________,
                  ________,
                  ________,
                  XXX_XX__,
                  ___X__X_,
                  _XXXXXX_,
                  X__X____,
                  X__X____,
                  _XX_XXX_,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,6) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_231 =
{  5,  7,  1, 0, {________,
                  ________,
                  ________,
                  _XXXX___,
                  X_______,
                  X_______,
                  X_______,
                  X_______,
                  _XXXX___, // <---- baseline
                  __X_____,
                  ___X____,
                  __XX____} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_232 =
{  5,  7,  1, 0, {_X______,
                  __X_____,
                  ________,
                  _XXX____,
                  X___X___,
                  XXXXX___,
                  X_______,
                  X_______,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_233 =
{  5,  7,  1, 0, {___X____,
                  __X_____,
                  ________,
                  _XXX____,
                  X___X___,
                  XXXXX___,
                  X_______,
                  X_______,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_234 =
{  5,  7,  1, 0, {__XX____,
                  _X__X___,
                  ________,
                  _XXX____,
                  X___X___,
                  XXXXX___,
                  X_______,
                  X_______,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_235 =
{  5,  7,  1, 0, {________,
                  _X_X____,
                  ________,
                  _XXX____,
                  X___X___,
                  XXXXX___,
                  X_______,
                  X_______,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 3 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_236 =
{  3,  7,  1, 0, {_X______,
                  __X_____,
                  ________,
                  XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_237 =
{  5,  7,  1, 0, {___X____,
                  __X_____,
                  ________,
                  XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_238 =
{  5,  7,  1, 0, {__XX____,
                  _X__X___,
                  ________,
                  XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 4 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_239 =
{  4,  7,  1, 0, {________,
                  _X_X____,
                  ________,
                  XXX_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_240 =
{  6,  7,  0, 0, {XX_X____,
                  __XX____,
                  _X_XX___,
                  _XXXXX__,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_241 =
{  5,  7,  1, 0, {__X_X___,
                  _X_X____,
                  ________,
                  X_XXX___,
                  XX__X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_242 =
{  6,  7,  0, 0, {__X_____,
                  ___X____,
                  ________,
                  _XXXX___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_243 =
{  6,  7,  0, 0, {____X___,
                  ___X____,
                  ________,
                  _XXXX___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_244 =
{  6,  7,  0, 0, {___XX___,
                  __X__X__,
                  ________,
                  _XXXX___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_245 =
{  6,  7,  0, 0, {___X_X__,
                  __X_X___,
                  ________,
                  _XXXX___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_246 =
{  6,  7,  0, 0, {________,
                  __X_X___,
                  ________,
                  _XXXX___,
                  X____X__,
                  X____X__,
                  X____X__,
                  X____X__,
                  _XXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_247 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  __X_____,
                  ________,
                  XXXXXX__,
                  ________,
                  ________,
                  __X_____,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,6) to 6 x 6
static struct{ char s, w, o, j; unsigned char data[12]; } _char_248 =
{  6,  7,  0, 0, {________,
                  ________,
                  ________,
                  _XXXXX__,
                  X___XX__,
                  X__X_X__,
                  X_X__X__,
                  XX___X__,
                  XXXXX___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_249 =
{  6,  7,  0, 0, {_X______,
                  __X_____,
                  ________,
                  _X___X__,
                  _X___X__,
                  _X___X__,
                  _X___X__,
                  _X__XX__,
                  _XXX_X__,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_250 =
{  5,  7,  1, 0, {___X____,
                  __X_____,
                  ________,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X__XX___,
                  XXX_X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 9
static struct{ char s, w, o, j; unsigned char data[12]; } _char_251 =
{  5,  7,  1, 0, {__XX____,
                  _X__X___,
                  ________,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X__XX___,
                  XXX_X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,8) to 5 x 8
static struct{ char s, w, o, j; unsigned char data[12]; } _char_252 =
{  5,  7,  1, 0, {________,
                  _X_X____,
                  ________,
                  X___X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  X__XX___,
                  XXX_X___,
                  ________,
                  ________,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,9) to 6 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_253 =
{  6,  7,  0, 0, {____X___,
                  ___X____,
                  ________,
                  X____X__,
                  _X__X___,
                  _X__X___,
                  __XX____,
                  __XX____,
                  __X_____, // <---- baseline
                  __X_____,
                  XX______,
                  ________} };
// Character: 7,12 .9.. 7,0 (1,9) to 5 x 11
static struct{ char s, w, o, j; unsigned char data[12]; } _char_254 =
{  5,  7,  1, 0, {X_______,
                  X_______,
                  X_______,
                  X_XX____,
                  XX__X___,
                  X___X___,
                  X___X___,
                  X___X___,
                  XXXX____, // <---- baseline
                  X_______,
                  X_______,
                  ________} };
// Character: 7,12 .9.. 7,0 (0,8) to 6 x 10
static struct{ char s, w, o, j; unsigned char data[12]; } _char_255 =
{  6,  7,  0, 0, {________,
                  __X_X___,
                  ________,
                  X____X__,
                  _X__X___,
                  _X__X___,
                  __XX____,
                  __XX____,
                  __X_____, // <---- baseline
                  __X_____,
                  XX______,
                  ________} };
struct { unsigned short height, chars;
         PCHARACTERXT character[256]; } Lucida12by7 = { 12, 256, {  (PCHARACTERXT)&_char_0
 ,(PCHARACTERXT)&_char_1
 ,(PCHARACTERXT)&_char_2
 ,(PCHARACTERXT)&_char_3
 ,(PCHARACTERXT)&_char_4
 ,(PCHARACTERXT)&_char_5
 ,(PCHARACTERXT)&_char_6
 ,(PCHARACTERXT)&_char_7
 ,(PCHARACTERXT)&_char_8
 ,(PCHARACTERXT)&_char_9
 ,(PCHARACTERXT)&_char_10
 ,(PCHARACTERXT)&_char_11
 ,(PCHARACTERXT)&_char_12
 ,(PCHARACTERXT)&_char_13
 ,(PCHARACTERXT)&_char_14
 ,(PCHARACTERXT)&_char_15
 ,(PCHARACTERXT)&_char_16
 ,(PCHARACTERXT)&_char_17
 ,(PCHARACTERXT)&_char_18
 ,(PCHARACTERXT)&_char_19
 ,(PCHARACTERXT)&_char_20
 ,(PCHARACTERXT)&_char_21
 ,(PCHARACTERXT)&_char_22
 ,(PCHARACTERXT)&_char_23
 ,(PCHARACTERXT)&_char_24
 ,(PCHARACTERXT)&_char_25
 ,(PCHARACTERXT)&_char_26
 ,(PCHARACTERXT)&_char_27
 ,(PCHARACTERXT)&_char_28
 ,(PCHARACTERXT)&_char_29
 ,(PCHARACTERXT)&_char_30
 ,(PCHARACTERXT)&_char_31
 ,(PCHARACTERXT)&_char_32
 ,(PCHARACTERXT)&_char_33
 ,(PCHARACTERXT)&_char_34
 ,(PCHARACTERXT)&_char_35
 ,(PCHARACTERXT)&_char_36
 ,(PCHARACTERXT)&_char_37
 ,(PCHARACTERXT)&_char_38
 ,(PCHARACTERXT)&_char_39
 ,(PCHARACTERXT)&_char_40
 ,(PCHARACTERXT)&_char_41
 ,(PCHARACTERXT)&_char_42
 ,(PCHARACTERXT)&_char_43
 ,(PCHARACTERXT)&_char_44
 ,(PCHARACTERXT)&_char_45
 ,(PCHARACTERXT)&_char_46
 ,(PCHARACTERXT)&_char_47
 ,(PCHARACTERXT)&_char_48
 ,(PCHARACTERXT)&_char_49
 ,(PCHARACTERXT)&_char_50
 ,(PCHARACTERXT)&_char_51
 ,(PCHARACTERXT)&_char_52
 ,(PCHARACTERXT)&_char_53
 ,(PCHARACTERXT)&_char_54
 ,(PCHARACTERXT)&_char_55
 ,(PCHARACTERXT)&_char_56
 ,(PCHARACTERXT)&_char_57
 ,(PCHARACTERXT)&_char_58
 ,(PCHARACTERXT)&_char_59
 ,(PCHARACTERXT)&_char_60
 ,(PCHARACTERXT)&_char_61
 ,(PCHARACTERXT)&_char_62
 ,(PCHARACTERXT)&_char_63
 ,(PCHARACTERXT)&_char_64
 ,(PCHARACTERXT)&_char_65
 ,(PCHARACTERXT)&_char_66
 ,(PCHARACTERXT)&_char_67
 ,(PCHARACTERXT)&_char_68
 ,(PCHARACTERXT)&_char_69
 ,(PCHARACTERXT)&_char_70
 ,(PCHARACTERXT)&_char_71
 ,(PCHARACTERXT)&_char_72
 ,(PCHARACTERXT)&_char_73
 ,(PCHARACTERXT)&_char_74
 ,(PCHARACTERXT)&_char_75
 ,(PCHARACTERXT)&_char_76
 ,(PCHARACTERXT)&_char_77
 ,(PCHARACTERXT)&_char_78
 ,(PCHARACTERXT)&_char_79
 ,(PCHARACTERXT)&_char_80
 ,(PCHARACTERXT)&_char_81
 ,(PCHARACTERXT)&_char_82
 ,(PCHARACTERXT)&_char_83
 ,(PCHARACTERXT)&_char_84
 ,(PCHARACTERXT)&_char_85
 ,(PCHARACTERXT)&_char_86
 ,(PCHARACTERXT)&_char_87
 ,(PCHARACTERXT)&_char_88
 ,(PCHARACTERXT)&_char_89
 ,(PCHARACTERXT)&_char_90
 ,(PCHARACTERXT)&_char_91
 ,(PCHARACTERXT)&_char_92
 ,(PCHARACTERXT)&_char_93
 ,(PCHARACTERXT)&_char_94
 ,(PCHARACTERXT)&_char_95
 ,(PCHARACTERXT)&_char_96
 ,(PCHARACTERXT)&_char_97
 ,(PCHARACTERXT)&_char_98
 ,(PCHARACTERXT)&_char_99
 ,(PCHARACTERXT)&_char_100
 ,(PCHARACTERXT)&_char_101
 ,(PCHARACTERXT)&_char_102
 ,(PCHARACTERXT)&_char_103
 ,(PCHARACTERXT)&_char_104
 ,(PCHARACTERXT)&_char_105
 ,(PCHARACTERXT)&_char_106
 ,(PCHARACTERXT)&_char_107
 ,(PCHARACTERXT)&_char_108
 ,(PCHARACTERXT)&_char_109
 ,(PCHARACTERXT)&_char_110
 ,(PCHARACTERXT)&_char_111
 ,(PCHARACTERXT)&_char_112
 ,(PCHARACTERXT)&_char_113
 ,(PCHARACTERXT)&_char_114
 ,(PCHARACTERXT)&_char_115
 ,(PCHARACTERXT)&_char_116
 ,(PCHARACTERXT)&_char_117
 ,(PCHARACTERXT)&_char_118
 ,(PCHARACTERXT)&_char_119
 ,(PCHARACTERXT)&_char_120
 ,(PCHARACTERXT)&_char_121
 ,(PCHARACTERXT)&_char_122
 ,(PCHARACTERXT)&_char_123
 ,(PCHARACTERXT)&_char_124
 ,(PCHARACTERXT)&_char_125
 ,(PCHARACTERXT)&_char_126
 ,(PCHARACTERXT)&_char_127
 ,(PCHARACTERXT)&_char_128
 ,(PCHARACTERXT)&_char_129
 ,(PCHARACTERXT)&_char_130
 ,(PCHARACTERXT)&_char_131
 ,(PCHARACTERXT)&_char_132
 ,(PCHARACTERXT)&_char_133
 ,(PCHARACTERXT)&_char_134
 ,(PCHARACTERXT)&_char_135
 ,(PCHARACTERXT)&_char_136
 ,(PCHARACTERXT)&_char_137
 ,(PCHARACTERXT)&_char_138
 ,(PCHARACTERXT)&_char_139
 ,(PCHARACTERXT)&_char_140
 ,(PCHARACTERXT)&_char_141
 ,(PCHARACTERXT)&_char_142
 ,(PCHARACTERXT)&_char_143
 ,(PCHARACTERXT)&_char_144
 ,(PCHARACTERXT)&_char_145
 ,(PCHARACTERXT)&_char_146
 ,(PCHARACTERXT)&_char_147
 ,(PCHARACTERXT)&_char_148
 ,(PCHARACTERXT)&_char_149
 ,(PCHARACTERXT)&_char_150
 ,(PCHARACTERXT)&_char_151
 ,(PCHARACTERXT)&_char_152
 ,(PCHARACTERXT)&_char_153
 ,(PCHARACTERXT)&_char_154
 ,(PCHARACTERXT)&_char_155
 ,(PCHARACTERXT)&_char_156
 ,(PCHARACTERXT)&_char_157
 ,(PCHARACTERXT)&_char_158
 ,(PCHARACTERXT)&_char_159
 ,(PCHARACTERXT)&_char_160
 ,(PCHARACTERXT)&_char_161
 ,(PCHARACTERXT)&_char_162
 ,(PCHARACTERXT)&_char_163
 ,(PCHARACTERXT)&_char_164
 ,(PCHARACTERXT)&_char_165
 ,(PCHARACTERXT)&_char_166
 ,(PCHARACTERXT)&_char_167
 ,(PCHARACTERXT)&_char_168
 ,(PCHARACTERXT)&_char_169
 ,(PCHARACTERXT)&_char_170
 ,(PCHARACTERXT)&_char_171
 ,(PCHARACTERXT)&_char_172
 ,(PCHARACTERXT)&_char_173
 ,(PCHARACTERXT)&_char_174
 ,(PCHARACTERXT)&_char_175
 ,(PCHARACTERXT)&_char_176
 ,(PCHARACTERXT)&_char_177
 ,(PCHARACTERXT)&_char_178
 ,(PCHARACTERXT)&_char_179
 ,(PCHARACTERXT)&_char_180
 ,(PCHARACTERXT)&_char_181
 ,(PCHARACTERXT)&_char_182
 ,(PCHARACTERXT)&_char_183
 ,(PCHARACTERXT)&_char_184
 ,(PCHARACTERXT)&_char_185
 ,(PCHARACTERXT)&_char_186
 ,(PCHARACTERXT)&_char_187
 ,(PCHARACTERXT)&_char_188
 ,(PCHARACTERXT)&_char_189
 ,(PCHARACTERXT)&_char_190
 ,(PCHARACTERXT)&_char_191
 ,(PCHARACTERXT)&_char_192
 ,(PCHARACTERXT)&_char_193
 ,(PCHARACTERXT)&_char_194
 ,(PCHARACTERXT)&_char_195
 ,(PCHARACTERXT)&_char_196
 ,(PCHARACTERXT)&_char_197
 ,(PCHARACTERXT)&_char_198
 ,(PCHARACTERXT)&_char_199
 ,(PCHARACTERXT)&_char_200
 ,(PCHARACTERXT)&_char_201
 ,(PCHARACTERXT)&_char_202
 ,(PCHARACTERXT)&_char_203
 ,(PCHARACTERXT)&_char_204
 ,(PCHARACTERXT)&_char_205
 ,(PCHARACTERXT)&_char_206
 ,(PCHARACTERXT)&_char_207
 ,(PCHARACTERXT)&_char_208
 ,(PCHARACTERXT)&_char_209
 ,(PCHARACTERXT)&_char_210
 ,(PCHARACTERXT)&_char_211
 ,(PCHARACTERXT)&_char_212
 ,(PCHARACTERXT)&_char_213
 ,(PCHARACTERXT)&_char_214
 ,(PCHARACTERXT)&_char_215
 ,(PCHARACTERXT)&_char_216
 ,(PCHARACTERXT)&_char_217
 ,(PCHARACTERXT)&_char_218
 ,(PCHARACTERXT)&_char_219
 ,(PCHARACTERXT)&_char_220
 ,(PCHARACTERXT)&_char_221
 ,(PCHARACTERXT)&_char_222
 ,(PCHARACTERXT)&_char_223
 ,(PCHARACTERXT)&_char_224
 ,(PCHARACTERXT)&_char_225
 ,(PCHARACTERXT)&_char_226
 ,(PCHARACTERXT)&_char_227
 ,(PCHARACTERXT)&_char_228
 ,(PCHARACTERXT)&_char_229
 ,(PCHARACTERXT)&_char_230
 ,(PCHARACTERXT)&_char_231
 ,(PCHARACTERXT)&_char_232
 ,(PCHARACTERXT)&_char_233
 ,(PCHARACTERXT)&_char_234
 ,(PCHARACTERXT)&_char_235
 ,(PCHARACTERXT)&_char_236
 ,(PCHARACTERXT)&_char_237
 ,(PCHARACTERXT)&_char_238
 ,(PCHARACTERXT)&_char_239
 ,(PCHARACTERXT)&_char_240
 ,(PCHARACTERXT)&_char_241
 ,(PCHARACTERXT)&_char_242
 ,(PCHARACTERXT)&_char_243
 ,(PCHARACTERXT)&_char_244
 ,(PCHARACTERXT)&_char_245
 ,(PCHARACTERXT)&_char_246
 ,(PCHARACTERXT)&_char_247
 ,(PCHARACTERXT)&_char_248
 ,(PCHARACTERXT)&_char_249
 ,(PCHARACTERXT)&_char_250
 ,(PCHARACTERXT)&_char_251
 ,(PCHARACTERXT)&_char_252
 ,(PCHARACTERXT)&_char_253
 ,(PCHARACTERXT)&_char_254
 ,(PCHARACTERXT)&_char_255

} };// $Log: $
