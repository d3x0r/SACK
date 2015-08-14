package net

/*
#include <network.h>

int PTRSZVAL_sz = sizeof( PTRSZVAL );

void ListenNotify_cgo( PCLIENT listener, PCLIENT newClient )
{
    ListenCallback( GetNetworkLong( listener, 0 ), newClient );
}
*/
import "C"

