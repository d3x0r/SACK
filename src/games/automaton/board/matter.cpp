
#include "matter.hpp"

class MATTER_FACTORY:public IMATTER_FACTORY
{

public:
   MATTER_FACTOR();
   ~MATTER_FACTOR();
}



PIMATTER_FORMATTER CreateMatterFactory( void )
{
	PIMATTER_FACTORY pimf;
	pimf = new MATTER_FACTORY;
   return pimf;
}


