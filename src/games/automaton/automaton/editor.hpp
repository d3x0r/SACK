
#include <vectlib.h>
#include "object.hpp"

extern "C" {
#include "vidlib.h"
};

class EDITOR {
   TRANSFORM TView;
   PRENDERER hVideo;
   HMENU  hMenu;  // first one should have 'add line'
   POBJECT pCurrent;
   POBJECT pRoot;
   PFACET pCurrentFacet;
   PLINESEG  pCurrentLine;
   POINT ptO, ptN;
   struct {
      int bLocked:1;  // mouse is locked to ptMouse...
      int bDragOrigin:1;
      int bDragNormal:1;
   };
   POINT ptMouse; // current point of mouse....
   POINT ptMouseDel; // accumulator of mouse delta (if locked)
   POINT ptUpperLeft; // upper left screen coordinate of window...
private:
   void MarkOrigin( Image pImage ); // relies on ptO being set...
   void MarkSlope( Image pImage ); // relies on ptN being set...
public:
   void EditMouseCallback( int x, int y, int b );
   void EditResizeCallback( void );
   void EditCloseCallback( void );
   EDITOR(void);
   ~EDITOR(void);
};
