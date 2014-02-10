
#include <vectlib.h>

#include "object.hpp"

typedef struct motion_tag
{
   int     nType;
   POBJECT pObject;
   struct  motion_tag *pNext, *pPrior;
} MOTION, *PMOTION;


PMOTION CreateMotion( POBJECT pObject, int nType );

enum {
   KEYBOARD,
   FOLLOW_PRIOR,  // internal scripting one.... moves at half the speed...
};

void FreeMotion( PMOTION pMotion );
void ApplyDynamics( void );
