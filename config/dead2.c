

#include <stdio.h>
#include <stdhdrs.h>
#include <deadstart.h>

int APIENTRY DllMain( HINSTANCE hinst, int reason, void *x )
{
   //DebugBreak();
   //printf( "alsdfkj\n" );
}

PRELOAD(test1) 
{
  printf("blah\n");
}
