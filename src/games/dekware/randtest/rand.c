#include <stdio.h>
#include <stdlib.h>


#define MAX_RAND RAND_MAX

int main(void )
{
   int i;
   double x, y, dx, dy;

   x = (rand() < 10000?-1.0:1.0 )*(( rand()/10000 ) + ((double)rand())/(double)MAX_RAND);
   y = (rand() < 10000?-1.0:1.0 )*(( rand()/10000 ) + ((double)rand())/(double)MAX_RAND);
   for(i = 0; i < 40000; i++ )
   {
      printf( "%g %g\n", x, y );
	   dx = (rand() < 10000?-1.0:1.0 )*(( rand()/10000 ) + ((double)rand())/(double)MAX_RAND);
   	dy = (rand() < 10000?-1.0:1.0 )*(( rand()/10000 ) + ((double)rand())/(double)MAX_RAND);
      x += dx;
      y += dy;
   }

}
// $Log: rand.c,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
