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
/*
 * ZVoxelType_Aroma.cpp
 *
 *  Created on: 4 févr. 2013
 *      Author: laurent
 */

#ifndef Z_VOXELTYPE_AROMA_H
#  include "ZVoxelType_Aroma.h"
#endif

#ifndef Z_ZVOXELEXTENSION_AROMA_H
#  include "ZVoxelExtension_Aroma.h"
#endif
#include "ZVoxelReactor.h"

ZVoxelExtension * ZVoxelType_Aroma::CreateVoxelExtension(bool IsLoadingPhase)
{
  ZVoxelExtension_Aroma * NewVoxelExtension = 0;

  NewVoxelExtension = new ZVoxelExtension_Aroma;
  return (NewVoxelExtension);
}

bool ZVoxelType_Aroma::React( const ZVoxelRef &self, double tick )
{
	ZVoxelExtension_Aroma *instance = (ZVoxelExtension_Aroma *)self.VoxelExtension;

	instance->time_since_spawn += tick;
	if( instance->time_since_spawn > 12000 )
	{
		self.World->SetVoxel_WithCullingUpdate(self.wx, self.wy, self.wz, 0, ZVoxelSector::CHANGE_UNIMPORTANT);
		return false;
	}
    // Eau qui coule. Flowing water.
    {
    ZVoxelSector * St[27];
    UShort * Vp[32];
    UShort  Vp2;
    ULong SecondaryOffset[27];
    ULong PrefSecondaryOffset[6][5];
    ULong i,vCount,j, vPrefCount;
    bool  DirEn[6];
    bool  PrefDirEn[6][5];
    //register Long cx,cy,cz;

	  Long Sx,Sy,Sz;
	  //ZVoxelSector * SectorTable[64];
	  //Sx = self.Sector->Pos_x - 1;
      //Sy = self.Sector->Pos_y - 1;
      //Sz = self.Sector->Pos_z - 1;
	  ZVoxelReactor::GetVoxelRefs( self, St, SecondaryOffset );
	if( 0 )	  
	  {
		  int n;
		  lprintf( "-------- Secondary Offset Translation ----------" );
		  for( n = 0; n < 27; n++ )
			  lprintf( " %08x   %d  %d  %d", SecondaryOffset[n]
						, (SecondaryOffset[n]>>ZVOXELBLOCSHIFT_Y ) & ZVOXELBLOCMASK_X
						, (SecondaryOffset[n]>>0 ) & ZVOXELBLOCMASK_Y
						, (SecondaryOffset[n]>>(ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X) ) & ZVOXELBLOCMASK_Z );
	  }
	  memset( PrefDirEn, 0, sizeof( PrefDirEn ) );

        for(i=0,vCount=0,vPrefCount=0;i<6;i++)
        {
			j = i ^ 1;

			Vp[i] = &St[i+1]->Data[ SecondaryOffset[i+1] ].Data;
			if (VoxelTypeManager->VoxelTable[*Vp[i]]->Is_CanBeReplacedBy_Water) {vCount++; DirEn[i]=true;}
			else DirEn[i]=false;

			if( *Vp[i] == self.VoxelType )
			for( int k = 0; k < 5; k++ )
			{
				PrefSecondaryOffset[j][k] = SecondaryOffset[ZVoxelReactor::x6_opposing_escape[j][k]];
				Vp2 = St[ZVoxelReactor::x6_opposing_escape[j][k]]->Data[ PrefSecondaryOffset[j][k] ].Data;
				if ( VoxelTypeManager->VoxelTable[Vp2]->Is_CanBeReplacedBy_Water) {vPrefCount++; PrefDirEn[j][k]=true;}
			}
        }

        if (vPrefCount>0 )
        {
			j = SRG_GetEntropy( ZVoxelReactor::Random2, 5, 0 );

			j = (j % vPrefCount) +1;
			for (i=0;i<6;i++)
			{
				int k;
				for( k = 0; k < 5; k++ )
				{
					if (PrefDirEn[i][k]) j--;
					if (!j)
					{
						self.World->SetVoxel_WithCullingUpdate(self.wx + ZVoxelReactor::xbp6_opposing_escape[i][k].x-1
							, self.wy + ZVoxelReactor::xbp6_opposing_escape[i][k].y-1
							, self.wz + ZVoxelReactor::xbp6_opposing_escape[i][k].z-1
							, self.VoxelType, ZVoxelSector::CHANGE_UNIMPORTANT);
						self.World->SetVoxel_WithCullingUpdate(self.wx, self.wy, self.wz, 0, ZVoxelSector::CHANGE_UNIMPORTANT);
						St[ZVoxelReactor::x6_opposing_escape[i][k]]->ModifTracker.Set(PrefSecondaryOffset[i][k]);
						break;
					}
				}
				if( k < 5 )
					break;
			}
        }
        else if (vCount>0 && vCount < 6)
        {
			j = SRG_GetEntropy( ZVoxelReactor::Random2, 5, 0 );

			j = (j % vCount) +1;
			for (i=0;i<6;i++)
			{
				if (DirEn[i]) j--;
				if (!j)
				{
				self.World->SetVoxel_WithCullingUpdate(self.wx + ZVoxelReactor::xbp6_opposing[i].x-1, self.wy + ZVoxelReactor::xbp6_opposing[i].y-1, self.wz + ZVoxelReactor::xbp6_opposing[i].z-1, self.VoxelType, ZVoxelSector::CHANGE_UNIMPORTANT);
				self.World->SetVoxel_WithCullingUpdate(self.wx, self.wy, self.wz, 0, ZVoxelSector::CHANGE_UNIMPORTANT);
				St[i+1]->ModifTracker.Set(SecondaryOffset[i+1]);
				break;
				}
			}
        }
    }
	return true;

	//lprintf( "Ground react at %g", tick );
}


