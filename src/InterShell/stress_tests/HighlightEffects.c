#include <stdhdrs.h>
#include <network.h>

#define USES_MILK_INTERFACE
#define DEFINES_MILK_INTERFACE
#include "../milk_export.h"
#include "../milk_registry.h"

PCLIENT pc;

PLIST buttons;

int bHighlight;



void CPROC ReadStuff( PCLIENT pc, POINTER p, int size )
{
	if( !p )
	{
      p = Allocate( 256 );
	}
	else
	{
		INDEX idx;
		PMENU_BUTTON button;
      P_8 bytes = (P_8)p;
		int n;
		for( n = 0; n < size; n++ )
		{
			bHighlight = bytes[n] & 1;
         //MILK_DisablePageUpdate( TRUE );
			LIST_FORALL( buttons, idx, PMENU_BUTTON, button )
			{
				UpdateButton( button );
			}
			//MILK_DisablePageUpdate( FALSE );
		}
	}
   ReadTCP( pc, p, 256 );
}

void CPROC Connected( PCLIENT pcServer, PCLIENT pcNew )
{
	SetNetworkReadComplete( pcNew, ReadStuff );
}

PRELOAD( MilkButtonBlink_Network )
{
   pc = OpenTCPListenerEx( 17899, Connected );
}

OnShowControl( "Network Highlight(Stress3)" )( PTRSZVAL button )
{
	MILK_SetButtonHighlight( (PMENU_BUTTON)button, bHighlight );
}

OnKeyPressEvent( "Network Highlight(Stress3)" )( PTRSZVAL button )
{
   // uhmm whatever.
}

OnCreateMenuButton( "Network Highlight(Stress3)" )( PMENU_BUTTON button )
{
   AddLink( &buttons, button );
   return (PTRSZVAL)button;
}

