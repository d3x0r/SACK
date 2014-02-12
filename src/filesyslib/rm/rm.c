#include <windows.h>
#include <stdio.h>
#include <filesys.h>

int abort_remove;
int recurse = 1;
int force = 1;
int verbose;
char *current_mask;
int delete_level;
void DeleteAFile( PTRSZVAL psv, char *name, int flags )
{
   if( abort_remove )
      return;
   if( flags & SFF_DRIVE )
   {
      fprintf( stdout, WIDE("Not allowed to delete drives! aborting!\n") );
      abort_remove = 1;
      return;
   }
	delete_level++;
   if( verbose || !force )
      fprintf( stdout, WIDE("Going to delete %s %s")
               , (flags & SFF_DIRECTORY)?"path":"file"
               , name );
   if( !force )
   {
      fprintf( stdout, WIDE("[n]?\n") );
   }
   else if( verbose )
	      fprintf( stdout, WIDE("\n") );

   if( flags & SFF_DIRECTORY )
   {
      POINTER info=NULL;
      if( ( delete_level == 1 &&
            CompareMask( current_mask, pathrchr( name ) + 1, FALSE ) ) 
          ||( delete_level > 1 ) )
      {
	      while( ScanFiles( name, NULL, &info, DeleteAFile
   				         , recurse?(SFF_SUBCURSE|SFF_DIRECTORIES):0
                        , 0 ) );
	      if( !RemoveDirectory( name ) )
   	      fprintf( stderr, WIDE("Failed to remove directory: %s %d\n")
   	      			, name, GetLastError() );
	   }
   }
   else
   {
      if( !DeleteFile( name ) )
         fprintf( stderr, WIDE("Failed to delete file: %s\n"), name );
   }
	delete_level--;
}

int main( int argc, char **argv )
{
   int n;
   POINTER info = NULL;
   for( n = 1; n < argc; n++ )
   {
      if( argv[n][0] == '-' )
      {
         int c;
         for( c = 1; argv[n][c]; c++ )
         {
            switch( argv[n][c] )
            {
            case 'r':
            case 'R':
               recurse = 1;
               break;
            case 'i':
               force = 0;
               break;
            case 'f':
               force = 1;
               break;
            case 'v':
               verbose = 1;
               break;
            }
         }
         // skip options.
         //recurse = 1; // deletes directories also....
         //force = 1;
      }
      else
      {
      	char *mask;
      	char tmp[256];
      	char *path;

      	sprintf( tmp, argv[n] );
      	mask = pathrchr( tmp );
			if( mask )
			{
				mask[0] = 0; // terminate path part.
				mask++;
				path = tmp;
			}
			else
			{
				mask = tmp;
				path = ".";
			}
      	current_mask = mask;
      	
         while( ScanFiles( path, mask, &info, DeleteAFile
                           , recurse?(SFF_SUBCURSE|SFF_DIRECTORIES):0
                           , 0 ) );
      }
   }
   return abort_remove;
}


