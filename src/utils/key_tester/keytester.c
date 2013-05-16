//#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <render.h>


int CPROC Keyproc( PTRSZVAL psv, _32 keycode )
{
	char buffer[128];
   _32 newtime = timeGetTime();;
   static _32 time;
	int ofs = 0;
	ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "%05d : ", newtime - time );
   time = newtime;
	if( keycode & KEY_PRESSED )
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "press " );
   else
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "noprs " );

	if( keycode & KEY_ALT_DOWN )
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "alt " );
   else
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "    " );
	if( keycode & KEY_CONTROL_DOWN )
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "ctl " );
   else
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "    " );
	if( keycode & KEY_SHIFT_DOWN )
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "sft " );
   else
		ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "    " );
	ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "%d ", KEY_CODE( keycode ) );
	ofs += snprintf( buffer + ofs, sizeof( buffer ) - ofs, "%d ", KEY_REAL_CODE( keycode ) );
	fprintf( stdout, "%s\n", buffer );
   return 0;
}

int main( void )
{
	PRENDERER renderer = OpenDisplay( 0 );
   SetSystemLog( SYSLOG_FILE, stdout );
   SetKeyboardHandler( renderer, Keyproc, 0 );
   UpdateDisplay( renderer );


	while( 1 )
	{
      WakeableSleep( 10000 );
	}
   return 0;
}
