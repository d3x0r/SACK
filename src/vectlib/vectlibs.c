
#ifdef MAKE_RCOORD_SINGLE
#  define WAS_RCOORD_SINGLE
#else
#  undef WAS_RCOORD_SINGLE
#endif

#undef MAKE_RCOORD_SINGLE

#include "vectlib.c"

#undef VECTOR_TYPES_DEFINED
#undef ROTATE_DECLARATION
#define MAKE_RCOORD_SINGLE

#include "vectlib.c"

#if !defined( WAS_RCOORD_SINGLE )
#  undef MAKE_RCOORD_SINGLE
#endif
