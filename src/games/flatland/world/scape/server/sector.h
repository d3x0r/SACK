#ifndef SECTOR_DEFINED
#define SECTOR_DEFINED
#include <vectlib.h>
#include <sack_types.h>

#define WORLD_TYPES_DEFINED

#include <worldstrucs.h>

typedef struct sectorfilev3_tag {
	SECTORFLAGS flags;
	int nwall;
	RAY r;
} FILESECTORV3;

typedef struct sectorfilev4_tag {
	int nID;
	SECTORFLAGS flags;
	int nwall;
	RAY r;
} FILESECTORV4;

typedef struct sectorfilev5_tag {
	int nName;
	SECTORFLAGS flags;
	int nwall;
	RAY r;
} FILESECTORV5;

typedef struct sectorfilev8_tag {
	int nName;
	SECTORFLAGS flags;
	int nwall;
	RAY r;
	int nTexture;
} FILESECTORV8;


                     
/*****************
// this is a wall loop - copy and insert wall check code...
	PWALL pStart, pCur;
	int priorend = TRUE;

	pCur = pStart = sector->wall;
	do
	{
		// code goes here....
		if( priorend )
		{
			priorend = pCur->flags.wall_start_end;
			pCur = pCur->wall_at_start;
		}
		else
		{
			priorend = pCur->flags.wall_end_end;
			pCur = pCur->wall_at_end;
		}
	}while( pCur != pStart );
***************/
void DeleteSectors( INDEX iWorld );
void DeleteWalls( INDEX iWorld );
void DeleteLines( INDEX iWorld );
WORLD_PROC( INDEX, GetWallLine )( INDEX iWorld, INDEX iWall );
WORLD_PROC( INDEX, GetNextWall )( INDEX iWorld, INDEX iWall, int *priorend );
WORLD_PROC( INDEX, GetFirstWall )( INDEX iWorld, INDEX iSector, int *priorend );
PTRSZVAL CPROC ServerDestroyLine( INDEX iLine, INDEX iWorld );

INDEX AddConnectedSector( INDEX iWorld, INDEX iWall, RCOORD offset );
int MoveWalls( INDEX iWorld, int nWalls, INDEX *WallList, P_POINT del, int bLockSlope );

INDEX CreateWall( INDEX iWorld
				 , INDEX iSector
				 // pStart == wall intended to be at the start part of
				 // this line...
				 // bFromStartEnd is a flag whether
				 // this new is at the end or start of the indicated line
				 , INDEX iStart, int bFromStartEnd
				 // pEnd == wall intended to be at the 'end' of this
				 // line...
				 // bFromEndEnd is a flag whether
				 // this new is at the end or start of the indicated line
				 , INDEX iEnd,   int bFromEndEnd
				 , _POINT o, _POINT n );
void ComputeSectorOrigin( INDEX iWorld, INDEX iSector );

int MoveSectors( INDEX iWorld, int nSectors,INDEX *pSectors, P_POINT del );

// return TRUE to allow the update
// return FALSE to invalidate the update... too many
// intersections... or lines that get deleted...
int UpdateMatingLines( INDEX iWorld, INDEX iWall
					  , int bLockSlope, int bErrorOK );


#endif
