
#include <controls.h>
#include <keypad.h>


int main( void )
{
   int okay = 0, done = 0;
	PCOMMON frame = CreateFrame( WIDE("Test Keypads..."), 0, 0, 1024, 768, BORDER_NORMAL, NULL );
   MakeKeypad( frame, 10, 10, 90, 120, 0, 0 );
   MakeKeypad( frame, 115, 10, 90, 120, 0, KEYPAD_FLAG_DISPLAY );
   MakeKeypad( frame, 215, 10, 90, 120, 0, KEYPAD_FLAG_ENTRY );
   MakeKeypad( frame, 10, 150, 800, 200, 0, KEYPAD_FLAG_ALPHANUM );
   MakeKeypad( frame, 10, 355, 800, 200, 0, KEYPAD_FLAG_ALPHANUM|KEYPAD_FLAG_DISPLAY );
	AddCommonButtons( frame, &okay, &done );
   DisplayFrame( frame );
	CommonWait( frame );

}

