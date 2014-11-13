/*
 * This file is part of Blackvoxel.
 *
 * Copyright 2010-2014 Laurent Thiebaut & Olivia Merle
 *
 * Blackvoxel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Blackvoxel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef Z_ZRENDER_SMOOTH_H
#define Z_ZRENDER_SMOOTH_H

//#ifndef Z_ZRENDER_BASIC_H
//#  include "ZRender_Basic.h"
//#endif

#ifndef Z_ZWORLD_H
#  include "ZWorld.h"
#endif

#ifndef Z_ZVOXELTYPE_H
#  include "ZVoxelType.h"
#endif

#ifndef Z_ZTEXTUREMANAGER_H
#  include "ZTextureManager.h"
#endif

#ifndef Z_ZSECTORSPHERE_H
#include "ZSectorSphere.h"
#endif

#ifndef Z_ZRENDER_SORTER_H
#  include "ZRender_Sorter.h"
#endif

#include "ZActor_Player.h"

#include "ZRender_Interface.h"

extern GLuint TextureName[1024];

class ZGame;

class ZVoxelCuller_Smooth: public ZVoxelCuller
{
public:
	ZVoxelCuller_Smooth( ZVoxelWorld *world ) : ZVoxelCuller( world )
	{
	}
	 void InitFaceCullData( ZVoxelSector *sector );

   // if not internal, then is meant to cull the outside edges of the sector
	 void CullSector( ZVoxelSector *sector, bool internal, int interesting_faces );
	// void CullSectorInternal( ZVoxelSector *sector );
	// void CullSectorEdges( ZVoxelSector *sector );

	 void CullSingleVoxel( ZVoxelSector *sector, ULong offset );
	 void CullSingleVoxel( int x, int y, int z );
 	ULong getFaceCulling( ZVoxelSector *Sector, int offset );
	void setFaceCulling( ZVoxelSector *Sector, int offset, ULong value );
	bool Decompress_RLE(ZVoxelSector *Sector,  void * Stream);
	void Compress_RLE(ZVoxelSector *Sector,  void  * Stream);

};

class ZRender_Smooth: public ZRender_Interface
{
public:
    ZRender_Smooth( ZVoxelWorld *world )
	{
      voxelCuller = new ZVoxelCuller_Smooth( world );
      hRenderRadius = 1;  // 8
      vRenderRadius = 1;  // 3
      World = 0;
      VoxelTypeManager = 0;
      TextureManager = 0;
      Stat_RenderDrawFaces = 0;
      Stat_FaceTop = 0;
      Stat_FaceFront = 0;
      Stat_FaceRight = 0;
      Stat_FaceLeft = 0;
      Stat_FaceBottom = 0;
      Stat_FaceBack = 0;
      PointedVoxel = 0;
      Camera = 0;
      GameEnv = 0;
      Stat_RefreshWaitingSectorCount = 0;
      BvProp_CrossHairType = 1;
      BvProp_DisplayCrossHair     = true;
      BvProp_DisplayVoxelSelector = true;
      ViewportResolution.x = 1920;
      ViewportResolution.y = 1200;
      VerticalFOV = 63.597825649;
      FocusDistance = 50.0;
      PixelAspectRatio = 1.0;
      Optimisation_FCullingFactor = 1.0;

      Frustum_V = 0.0;
      Frustum_H = 0.0;
      Aspect_Ratio = 0.0;
      Frustum_CullingLimit = 0.0;
    }
private:
	void EmitFaces				( ZVoxelType ** VoxelTypeTable, UShort &VoxelType, UShort &prevVoxelType, ULong info
							  , Long x, Long y, Long z
							  , Long Sector_Display_x, Long Sector_Display_y, Long Sector_Display_z );
    void MakeSectorRenderingData(ZVoxelSector * Sector);
    void MakeSectorRenderingData_Sorted(ZVoxelSector * Sector);
public:
    void Render( bool use_external_matrix );
	 ZVoxelCuller *GetCuller( void )
	 {
       return voxelCuller;
	 }
};



#endif
