#ifndef WALL_STRUCTURE_DEFINED
#define WALL_STRUCTURE_DEFINED

#include <worldstrucs.h>

typedef struct wall_file_tag_v1 {
	WALLFLAGS flags;
	int nSector;
	int nLine;
	int nSectorInto; // these two elements will probably merge...
	int nWallInto;
	int nWallStart, nWallEnd;
} FILEWALLV1;

typedef struct wall_file_tag_v2 {
	WALLFLAGS flags;
	int nSector;
	int nLine;
	int nWallInto;
	int nWallStart, nWallEnd;
} FILEWALLV2;


#endif
