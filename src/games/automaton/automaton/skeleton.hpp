
#include "rotate.hpp" 
#include "object.hpp"

extern "C" {
#include "vidlib.h"
};

class EDITOR {
   HVIDEO hVideo;
   HMENU  hMenu;  // first one should have 'add line'
   POBJECT pCurrent;
   POBJECT pRoot;
public:
   void EditMouseCallback( int x, int y, int b );
   void EditResizeCallback( void );
   void EditCloseCallback( void );
   EDITOR(void);
   ~EDITOR(void);
};
