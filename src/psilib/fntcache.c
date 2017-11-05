#ifndef __STATIC__
#define _PSI_INCLUSION_
// this module should use the kind interface to get the global from imglib.
#define NO_FONT_GLOBAL_DECLARATION
#include "global.h"
#  ifdef __cplusplus
#    include "../src/imglib/fntcache.c"
#  else
#    include "../imglib/fntcache.c"
#  endif
#endif
