#ifndef ZVOXEL_REF_DEFINED
#define ZVOXEL_REF_DEFINED

#include "ZVoxelSector.h"
#include "../ZVoxelExtension.h"
class ZVoxelWorld;
class ZVoxelRef
{
public:
	ZVoxelSector * Sector;
	int Offset;
	int x, y, z;
	int wx, wy, wz;
	UShort VoxelType;
	ZVoxelWorld *World;
	ZVoxelTypeManager *VoxelTypeManager;
	ZVoxelExtension *VoxelExtension;
	ZVoxelRef()
	{
	}
	/*
	ZVoxelRef( ZVoxelWorld *world, ZVoxelTypeManager *vtm, long x = 0, long y = 0, long z = 0, ZVoxelSector *Sector=NULL, UShort VoxelType = 0, int offset = 0 )
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
	*/
	static int ForEachVoxel( ZVoxelWorld * World, ZVoxelRef *v1, ZVoxelRef *v2, int (*f)(ZVoxelRef *v), bool not_zero );

};

#endif
