
#include <stdhdrs.h>
#include <deadstart.h>

#include "../widgets/keypad/keypad/keypad.h"
#include <intershell_registry.h>
#include "../widgets/include/banner.h"

PRELOAD( InitCalculator )
{
	CreateKeypadType( "Calculator Pad" );

}

static void CPROC Easter1( uintptr_t psv )
{
	BannerMessage( "Enabled Sequence 1" );
}

static void CPROC Easter2( uintptr_t psv )
{
	BannerMessage( "Enabled Sequence 2" );
}


static void OnFinishAllInit( "Calculator" )( void )
{
	PLIST keypads = NULL;
	PSI_CONTROL keypad;
	INDEX idx;
	GetKeypadsOfType( &keypads, "Calculator Pad" );
	LIST_FORALL( keypads, idx, PSI_CONTROL, keypad )
	{
		Keypad_AddMagicKeySequence( keypad, "314159", Easter1, 0 );
		Keypad_AddMagicKeySequence( keypad, "\\C\\C0\\C\\C0\\C", Easter2, 0 );

	}
}


#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
