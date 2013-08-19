

#include <stdhdrs.h>
#include <sackcomm.h>

#include "usbpulse100drvr.h"


PRELOAD( InitElan )
{
	TEXTCHAR entry[256];
	TEXTCHAR com_port[32];

	int id = 0;
	for( id = 0; ; id++ )
	{
		snprintf( entry, 256,  "ELAN/Driver/Device %d", id+1 );
		SACK_GetProfileString( GetProgramName(), entry, id?"":"COM1", com_port, sizeof( com_port ) );
		if( !com_port[0] )
         break;
	}

}

