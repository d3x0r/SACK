#ifndef ZVOXEL_REF_DEFINED
#define ZVOXEL_REF_DEFINED

#include "ZVoxelSector.h"

class ZVoxelWorld;
class ZVoxelRef
{
public:
	ZVoxelSector * Sector;
	int Offset;
	int x, y, z;
	UShort VoxelType;
	ZVoxelWorld *World;
	ZVoxelTypeManager *VoxelTypeManager;
	ZVoxelRef( ZVoxelWorld *world, ZVoxelTypeManager *vtm, long x, long y, long z, UShort VoxelType, int offset )
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->Sector = Sector;
		this->Offset = offset;
		this->World = world;
		this->VoxelType = VoxelType;
		VoxelTypeManager = vtm;
	}
	static int ForEachVoxel( ZVoxelWorld * World, ZVoxelRef *v1, ZVoxelRef *v2, int (*f)(ZVoxelRef *v) );

};

#endif
