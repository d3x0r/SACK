#include <stdio.h>
#include <sys/stat.h>


int main( void )
{
   int r;
   struct stat statbuf;
   r = stat( "thisdir", &statbuf );
   if( r < 0 )
   {
      printf( "stat failed..." );
   }
   else
      printf( "Stat success..." );
}
