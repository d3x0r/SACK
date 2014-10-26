#ifndef Z_ZVOXEL_CULLER_H
#define Z_ZVOXEL_CULLER_H

// #ifndef Z_ZTYPE_ZVECTOR3D_H
// #  include "ZType_ZVector3d.h"
// #endif

#ifndef Z_ZTYPES_H
#  include "ZTypes.h"
#endif

//#include "ZWorld.h"

class ZVoxelWorld;
class ZVoxelSector;

class ZVoxelCuller
{
protected:
   ZVoxelWorld &world;
public:
	ZVoxelCuller( ZVoxelWorld &world ): world( world )
	{
	}

	virtual void InitFaceCullData( ZVoxelSector *sector ) = 0;

   // if not internal, then is meant to cull the outside edges of the sector
	virtual void CullSector( ZVoxelSector *sector, bool internal ) = 0;
	//virtual void CullSectorInternal( ZVoxelSector *sector );
	//virtual void CullSectorEdges( ZVoxelSector *sector );

	virtual void CullSingleVoxel( ZVoxelSector *sector, int x, int y, int z ) = 0;

	virtual ULong getFaceCulling( ZVoxelSector *Sector, int offset ) = 0;
	virtual void setFaceCulling( ZVoxelSector *Sector, int offset, ULong value ) = 0;


};

#endif
