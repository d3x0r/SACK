#include <stdio.h>

class ICLASS
{
public:
	int get( void );
   virtual int myget( void );
};

struct SCLASS
{
   int n;
};

class CLASS:public ICLASS, public SCLASS
{
	int get( void ) {
      return n;
	}

};

ICLASS* Create( void )
{
	CLASS *x = new CLASS;
   return (ICLASS*)x;
}

int main( void )
{
	ICLASS* x = Create();
   printf( "%d", x->get() );
}


