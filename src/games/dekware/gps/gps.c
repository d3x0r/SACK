
#include <space.h>
#include <commands.h>
#include <vectlib.h>

typedef struct GPS_Data GPSData;
struct GPS_Data {
	_POINT position;
	_POINT speed;
	_POINT rotation;

};

static struct {
   INDEX iGPS;
} l;



static int ObjectMethod( "GPS", "Distance", "I dunno some sort of command" )( PSENTIENT ps, PTEXT parameters )
{

   return 0;
}


#if 0
static int ObjectVariable( "GPS", "position", "no desc" )( PTRSZVAL psv, PENTITY pe, LOGICAL bSet, PTEXT *last_value )
{
}

#endif
static int OnCreateObject( "GPS", "Positioning Data for objects, maybe some methods to move and change postion" )( PSENTIENT ps, PENTITY pe, PTEXT paramaters )
{
	GPSData *gps = (GPSData *)GetLink( &pe->pPlugin, l.iGPS );
	if( !gps )
	{
		gps = New( GPSData );
		SetLink( &pe->pPlugin, l.iGPS, gps );
	}


   // 0 on success, else on error
   return 0;
}

PUBLIC( char *, RegisterRoutines )( void )
{
   l.iGPS = RegisterExtension( "GPS" );
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
}

