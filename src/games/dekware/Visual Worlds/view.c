

#include "plugin.h"

#include <matrix.h>

typedef struct Location
{
   PTRANSFORM T;  // object's own translation matrix...
   PTRANSFORM Ti; // applied my transform with parent's...
} LOCATION, *PLOCATION;


int iTransform;
int iTransformApplied;

int SetView( PSENTIENT ps, PTEXT parameters )
{
}

int OpenViewPort( PSENTIENT ps, PTEXT parameters )
{

	return 0;
}

int CreateMotionFrame( PSENTIENT ps, PTEXT parameters )
{
	// creates an object with it's own translation matrix.

}

// $Log: view.c,v $
// Revision 1.2  2003/03/25 08:59:01  panther
// Added CVS logging
//
