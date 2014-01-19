
#include <stdhdrs.h>
#include <deadstart.h>

#include "../widgets/keypad/keypad/keypad.h"
#include <InterShell_registry.h>
#include "../widgets/include/banner.h"

PRELOAD( InitCalculator )
{
	CreateKeypadType( WIDE("Calculator Pad") );

}

static void CPROC Easter1( PTRSZVAL psv )
{
	BannerMessage( WIDE("Enabled Sequence 1") );
}

static void CPROC Easter2( PTRSZVAL psv )
{
	BannerMessage( WIDE("Enabled Sequence 2") );
}


OnFinishAllInit( WIDE("Calculator") )( void )
{
	PLIST keypads = NULL;
	PSI_CONTROL keypad;
	INDEX idx;
	GetKeypadsOfType( &keypads, WIDE("Calculator Pad") );
	LIST_FORALL( keypads, idx, PSI_CONTROL, keypad )
	{
		Keypad_AddMagicKeySequence( keypad, WIDE("314159"), Easter1, 0 );
		Keypad_AddMagicKeySequence( keypad, WIDE("\\C\\C0\\C\\C0\\C"), Easter2, 0 );

	}
}


#if __WATCOMC__ < 1291
PUBLIC( void, ExportThis )( void )
{
}
#endif
