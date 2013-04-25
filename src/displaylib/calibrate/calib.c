#define USE_RENDER_INTERFACE MyDisplayInterface

#include <stdio.h>
#include <render.h>
#include <stdlib.h>
#include <configscript.h>

static PRENDER_INTERFACE MyDisplayInterface;
static FILE *out;

PTRSZVAL CPROC SetMinX( PTRSZVAL psv, va_list args )
{
	PARAM( args, _64, val );
	char newval[32];
	snprintf( newval, sizeof( newval ), WIDE("%ld"), val );
	setenv( WIDE("SDL_ELO_MIN_X"), newval, TRUE );
   lprintf( WIDE("MINX = %Ld"), val );
   return psv;
}
PTRSZVAL CPROC SetMinY( PTRSZVAL psv, va_list args )
{
   PARAM( args, _64, val );
	char newval[32];
	snprintf( newval, sizeof( newval ), WIDE("%ld"), val );
   setenv( WIDE("SDL_ELO_MIN_Y"), newval, TRUE );
   lprintf( WIDE("MINY = %Ld"), val );
   return psv;
}
PTRSZVAL CPROC SetMaxX( PTRSZVAL psv, va_list args )
{
   PARAM( args, _64, val );
	char newval[32];
	snprintf( newval, sizeof( newval ), WIDE("%ld"), val );
   setenv( WIDE("SDL_ELO_MAX_X"), newval, TRUE );
   lprintf( WIDE("MAXX = %Ld"), val );
   return psv;
}
PTRSZVAL CPROC SetMaxY( PTRSZVAL psv, va_list args )
{
   PARAM( args, _64, val );
	char newval[32];
	snprintf( newval, sizeof( newval ), WIDE("%ld"), val );
   setenv( WIDE("SDL_ELO_MAX_Y"), newval, TRUE );
   lprintf( WIDE("MAXY = %Ld"), val );
   return psv;
}

PTRSZVAL CPROC WriteMinX( PTRSZVAL psv, va_list args )
{
   fprintf( out, WIDE("option \")MinX\" \"%s\"\n", getenv( WIDE("SDL_ELO_MIN_X") ) );
   return psv;
}
PTRSZVAL CPROC WriteMinY( PTRSZVAL psv, va_list args )
{
   fprintf( out, WIDE("option \")MinY\" \"%s\"\n", getenv( WIDE("SDL_ELO_MIN_Y") ) );
   return psv;
}
PTRSZVAL CPROC WriteMaxX( PTRSZVAL psv, va_list args )
{
   fprintf( out, WIDE("option \")MaxX\" \"%s\"\n", getenv( WIDE("SDL_ELO_MAX_X") ) );
   return psv;
}
PTRSZVAL CPROC WriteMaxY( PTRSZVAL psv, va_list args )
{
   fprintf( out, WIDE("option \")MaxY\" \"%s\"\n", getenv( WIDE("SDL_ELO_MAX_Y") ) );
   return psv;
}

int ReadXConfig( char *filename )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
   AddConfigurationMethod( pch, WIDE("option \")MinX\" \"%i\"", SetMinX );
   AddConfigurationMethod( pch, WIDE("option \")MinY\" \"%i\"", SetMinY );
   AddConfigurationMethod( pch, WIDE("option \")MaxX\" \"%i\"", SetMaxX );
	AddConfigurationMethod( pch, WIDE("option \")MaxY\" \"%i\"", SetMaxY );
	if( !ProcessConfigurationFile( pch, filename, 0 ) )
	{
		printf( WIDE("Failed to process config file \'%s\'"), filename );
      return 0;
	}
	DestroyConfigurationHandler( pch );
   return 1;
}

PTRSZVAL CPROC WriteLine( PTRSZVAL psv, char *line )
{
   if( line )
		fprintf( out, WIDE("%s\n"), line );
	else
      fprintf( out, WIDE("\n") );
}

void WriteXConfig( char *filename )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
   out = fopen( WIDE("tmp.x.config"), WIDE("wt") );
   AddConfigurationMethod( pch, WIDE("option \")MinX\" \"%i\"", WriteMinX );
   AddConfigurationMethod( pch, WIDE("option \")MinY\" \"%i\"", WriteMinY );
   AddConfigurationMethod( pch, WIDE("option \")MaxX\" \"%i\"", WriteMaxX );
	AddConfigurationMethod( pch, WIDE("option \")MaxY\" \"%i\"", WriteMaxY );
   SetConfigurationUnhandled( pch, WriteLine );
	if( !ProcessConfigurationFile( pch, filename, 0 ) )
	{
      printf( WIDE("Failed to process config file \'%s\'"), filename );
	}
	DestroyConfigurationHandler( pch );
   fclose( out );
	unlink( filename );
	if( rename( WIDE("tmp.x.config"), filename ) )
      perror( WIDE("blah") );
}


int main( int argc, char **argv )
{
   FILE * pFile;

   char *SDL_Min_X;
   char *SDL_Min_Y;
   char *SDL_Max_X;
   char *SDL_Max_Y;
   char *SDL_Mouse_Driver;
   MyDisplayInterface = GetDisplayInterface();
	if( argc < 2 )
	{
		printf( WIDE("Usage: %s <X configfile>\n"), argv[0] );
		printf( WIDE("   If no config file is specified, assume using frame buffer\n") );
		printf( WIDE("   and, the environemnt variables SDL_MOUSEDRV, SDL_ELO_M[IN/AX]_[X/Y]\n") );
	}
	else
	{
		if( !ReadXConfig( argv[1] ) )
		{
         lprintf( WIDE("Invalid configuration file %s"), argv[1] );
         return 0;
		}
	}

	// open some display to make sure the display engine is up, and is available
   // to control the display...
	OpenDisplaySizedAt( 0, 0, 0, 0, 0 );
   

		BeginCalibration( 4 );
   if( argc > 1 )
		WriteXConfig( argv[1] );
	

   SDL_Mouse_Driver = getenv("SDL_MOUSEDRV");
//   lprintf("SDL_Mouse_Driver: %s", SDL_Mouse_Driver);
   
   SDL_Min_X = getenv("SDL_ELO_MIN_X");
//   lprintf("SDL_Min_X: %s", SDL_Min_X);

   SDL_Min_Y = getenv("SDL_ELO_MIN_Y");
//   lprintf("SDL_Min_Y: %s", SDL_Min_Y);
 
   SDL_Max_X = getenv("SDL_ELO_MAX_X");
//   lprintf("SDL_Max_X: %s", SDL_Max_X);

   SDL_Max_Y = getenv("SDL_ELO_MAX_Y");
//   lprintf("SDL_Max_Y: %s", SDL_Max_Y);

   pFile = fopen ("ELO.Config","w");
   fprintf (pFile, WIDE("elo min %s,%s\nelo max %s,%s\n"),
            SDL_Min_X, SDL_Min_Y, SDL_Max_X, SDL_Max_Y);
	fclose (pFile);
				if( argc > 1 )
				{
					printf( WIDE("Please update \'%s\' values to %s,%s %s,%s\n")
                      , argv[1]
							, SDL_Min_X, SDL_Min_Y, SDL_Max_X, SDL_Max_Y);
				}


	return 0;
}
