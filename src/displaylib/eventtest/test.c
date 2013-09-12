
#include <stdhdrs.h>
#include <timers.h>
#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <render.h>
#include <image.h>


static PRENDERER display;


int CPROC MouseEvent( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	char msg[256];
   _32 w, h;
	Image image = GetDisplayImage( display );
	snprintf( msg, sizeof( msg ), WIDE("Mouse: %ld,%ld  %08lx"), x, y, b );
	PutString( image, 5, 5, BASE_COLOR_WHITE, BASE_COLOR_BLUE, msg );
   GetStringSize( msg, &w, &h );
	UpdateDisplayPortion( display, 5, 5, 5 + w, 5 + h );
   return 1;
}



void CPROC KeyEvent( PTRSZVAL psv, _32 key )
{
	char msg[256];
   static int nLine;
	_32 w, h;
	Image image = GetDisplayImage( display );
	snprintf( msg, sizeof( msg ), WIDE("%04d Key: %08lx %s%s%s%s%s%s %d %d"), GetTickCount()%10000, key
			  , (key&KEY_PRESSED)?"down ":"up "
			  , (key&KEY_ALT_DOWN)?"alt ":" "
			  , (key&KEY_CONTROL_DOWN)?"ctrl ":" "
			  , (key&KEY_SHIFT_DOWN)?"shift ":" "
			  , ( key&KEY_ALPHA_LOCK_ON)?"alock ":" "
			  , (key&KEY_NUM_LOCK_ON)?"nlock ":" "
			  , KEY_REAL_CODE(key)
            , KEY_CODE(key)
			  );
	PutString( image, 5, ( 20 * nLine ) + 25, BASE_COLOR_WHITE, BASE_COLOR_BLUE, msg );
   GetStringSize( msg, &w, &h );
   UpdateDisplayPortion( display, 5, ( 20 * nLine ) + 25, 5 + w, ( 20 * nLine ) + 25 + h );
	nLine++;
	if( nLine == 10 )
      nLine = 0;
}

void CPROC DrawEvent( PTRSZVAL psv, PRENDERER renderer )
{
	char msg[256];
	_32 w, h;
	Image image = GetDisplayImage( display );
   ClearImageTo( image, BASE_COLOR_BLUE );
	snprintf( msg, sizeof( msg ), WIDE("Draw") );
	PutString( image, 5, 5, BASE_COLOR_WHITE, BASE_COLOR_BLUE, msg );
	GetStringSize( msg, &w, &h );
   UpdateDisplay( display );
	//UpdateDisplayPortion( display, 5, 5, 5 + w, 5 + h );
   return;

}


int main( void )
{
	display = OpenDisplaySizedAt( 0, 512, 384, 0, 0 );
	if( !display )
	{
      printf( WIDE("Failed to open rendering surface.") );
		return 1;
	}
   SetMouseHandler( display, MouseEvent, (PTRSZVAL)display );
	SetKeyboardHandler( display, KeyEvent, (PTRSZVAL)display );
	SetRedrawHandler( display, DrawEvent, (PTRSZVAL)display );
   UpdateDisplay( display );
	WakeableSleep( SLEEP_FOREVER );
   return 0;
}
