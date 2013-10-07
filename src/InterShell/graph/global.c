#define GLOBAL_SOURCE
#include "linegraph.h"

PRELOAD( init_global )
{
   g.pImageInterface = GetImageInterface();
}

