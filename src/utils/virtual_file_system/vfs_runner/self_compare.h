
#ifdef WIN32
// include standard program header definitions; sections, etc
#include "self_compare.win32.h"

#endif

#if defined( __LINUX__ ) && !defined( __MAC__ )
#include <elf.h>
#endif
