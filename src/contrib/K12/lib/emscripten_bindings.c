#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "KangarooTwelve.h"

KangarooTwelve_Instance * NewKangarooTwelve( void ) {
	return malloc( sizeof(  KangarooTwelve_Instance ) );
}

int KangarooTwelve_IsSqueezing( KangarooTwelve_Instance *pk12i ) {
	if( pk12i ) return pk12i->phase == SQUEEZING;
	return 0;
}

int KangarooTwelve_IsAbsorbing( KangarooTwelve_Instance *pk12i ) {
	if( pk12i ) return pk12i->phase == ABSORBING;
	return 0;
}
int KangarooTwelve_phase( KangarooTwelve_Instance *pk12i ) {
	printf( "PHASE:%d", pk12i->phase );
	if( pk12i ) return pk12i->phase;
	return 0;
}
