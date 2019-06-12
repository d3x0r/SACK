
#include "common.h"

void InitSRG( void ) {
    EM_ASM( (
        function SaltyRNG() {

        }

        Module.SACK.SaltyRNG = SaltyRNG;
    ) );
}