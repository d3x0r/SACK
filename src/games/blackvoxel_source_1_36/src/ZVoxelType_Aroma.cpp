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
	ZVoxelRef before_self = self;
	   // mark that I have processed... for recursive purposes...
    self.Sector->ModifTracker.Set(self.Offset);
	instance->time_since_spawn += tick;

	if( instance->time_since_spawn > 120000 )
	{
		self.World->SetVoxel_WithCullingUpdate(self.Sector, self.Offset, 0, ZVoxelSector::CHANGE_UNIMPORTANT, false);
		return false;
	}
    // Eau qui coule. Flowing water.
    {
    ZVoxelSector * St[27];
    UShort * Vp[32];
    UShort  * Vp2[6][5];
    ULong SecondaryOffset[27];
    ULong PrefSecondaryOffset[6][5];
    ULong i,vCount,j, vPrefCount;
    bool  DirEn[6];
    bool  PrefDirEn[6][5];
    //register Long cx,cy,cz;
	/*
	if( ((self.Offset>>ZVOXELBLOCSHIFT_Y)&ZVOXELBLOCMASK_X) != self.x ||
		(self.Offset & ZVOXELBLOCMASK_Y) != self.y ||
		(self.Offset >> ( ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X)) != self.z )
		DebugBreak();
	*/
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
			if( !St[i+1] )
			{
				Vp[i] = NULL;
				DirEn[i]=false;
				continue;
			}
			Vp[i] = &St[i+1]->Data.Data[ SecondaryOffset[i+1] ];
#if defined( CAN_RECURSE )
			if( self.VoxelTypeManager->ActiveTable->Get(*Vp[i]) 
				&& !St[i+1]->ModifTracker.Get(SecondaryOffset[i+1] ) )
			{
				before_self.x = ( before_self.wx = self.wx + ZVoxelReactor::xbp6_opposing[i].x -1 ) & ZVOXELBLOCMASK_X;
				before_self.y = ( before_self.wy = self.wy + ZVoxelReactor::xbp6_opposing[i].y -1) & ZVOXELBLOCMASK_Y;
				before_self.z = ( before_self.wz = self.wz + ZVoxelReactor::xbp6_opposing[i].z -1) & ZVOXELBLOCMASK_Z;
				before_self.VoxelType = *Vp[i];
				before_self.Sector = St[i+1];
				before_self.Offset = SecondaryOffset[i+1];
				//lprintf( "precompute %3d %d,%d,%d %d", before_self.VoxelType, before_self.wx, before_self.wy, before_self.wz, i + 1 );
				self.VoxelTypeManager->VoxelTable[*Vp[i]]->React( before_self, tick );
			}
#endif
			if( *Vp[i] == self.VoxelType )
			for( int k = 0; k < 5; k++ )
			{
				int check_index = ZVoxelReactor::x6_opposing_escape[j][k];
				PrefSecondaryOffset[j][k] = SecondaryOffset[check_index];
				Vp2[j][k] = &St[check_index]->Data.Data[ SecondaryOffset[check_index] ];
#if defined( CAN_RECURSE )
				if( self.VoxelTypeManager->ActiveTable->Get(*Vp2[j][k]) 
					&& !St[check_index]->ModifTracker.Get(SecondaryOffset[check_index] ) )
				{
					before_self.x = ( before_self.wx = self.wx + ZVoxelReactor::xbp6_opposing_escape[j][k].x -1) & ZVOXELBLOCMASK_X;
					before_self.y = ( before_self.wy = self.wy + ZVoxelReactor::xbp6_opposing_escape[j][k].y -1) & ZVOXELBLOCMASK_Y;
					before_self.z = ( before_self.wz = self.wz + ZVoxelReactor::xbp6_opposing_escape[j][k].z -1) & ZVOXELBLOCMASK_Z;
					before_self.VoxelType = *Vp2[j][k];
					before_self.Sector = St[check_index];
					before_self.Offset = SecondaryOffset[check_index];
					//lprintf( "precompute %3d %d,%d,%d %d", before_self.VoxelType, before_self.wx, before_self.wy, before_self.wz, check_index );
					self.VoxelTypeManager->VoxelTable[*Vp2[j][k]]->React( before_self, tick );
					// reset directions.... 
				}
#endif
			}
			else for( int k = 0; k < 5; k++ )
				Vp2[j][k] = NULL;

		}
        for(i=0,vCount=0,vPrefCount=0;i<6;i++)
        {
			j = i ^ 1;
			// this should be moved over the test above..
			// but the bug is that the sector gets processed with worng cooreds?
			//lprintf( "check is %d=%d", i, *Vp[i] );
			if (Vp[i] && VoxelTypeManager->VoxelTable[*Vp[i]]->Is_CanBeReplacedBy_Water) {vCount++; DirEn[i]=true;}
			else DirEn[i]=false;

			for( int k = 0; k < 5; k++ )
			{
				//if( Vp2[j][k] )	lprintf( "check %d,%d = %d",  j, k, *Vp2[j][k] );
				if ( Vp2[j][k] && VoxelTypeManager->VoxelTable[*Vp2[j][k]]->Is_CanBeReplacedBy_Water) {vPrefCount++; PrefDirEn[j][k]=true;}
			}
        }

        if (vPrefCount>0 )
        {
			j = SRG_GetEntropy( ZVoxelReactor::Random2, 5, 0 );
			j = (j % vPrefCount) +1;
			//lprintf( "j is %d of %d", j, vPrefCount );
			for (i=0;i<6;i++)
			{
				int k;
				for( k = 0; k < 5; k++ )
				{
					if (PrefDirEn[i][k]) j--;
					if (!j)
					{
						int check_index = ZVoxelReactor::x6_opposing_escape[i][k]; 
						if( &St[check_index]->Data.Data[ SecondaryOffset[check_index] ] != Vp2[i][k]  )
							DebugBreak();
						//lprintf( "swap %d,%d %d", i, k, check_index );
						self.World->ExchangeVoxels( self.Sector, self.Offset
							, St[check_index], SecondaryOffset[check_index]
							, ZVoxelSector::CHANGE_UNIMPORTANT, false );
						St[check_index]->ModifTracker.Set(SecondaryOffset[check_index]);
						break;
					}
				}
				if( k < 5 )
					break;
			}
        }
        else if (vCount>0 && vCount < 4)
        {
			j = SRG_GetEntropy( ZVoxelReactor::Random2, 5, 0 );

			j = (j % vCount) +1;
			//lprintf( "j is %d of %d", j, vCount );
			for (i=0;i<6;i++)
			{
				if (DirEn[i]) j--;
				if (!j)
				{
					if( *Vp[i] )
						DebugBreak();
					//lprintf( "swap %d", i + 1 );
					if( &St[i+1]->Data.Data[ SecondaryOffset[i+1] ] != ( Vp[i] ) )
					{
						DebugBreak();
					}
					self.World->ExchangeVoxels(self.Sector, self.Offset, St[i+1], SecondaryOffset[i+1], ZVoxelSector::CHANGE_UNIMPORTANT, false );
					St[i+1]->ModifTracker.Set(SecondaryOffset[i+1]);
					break;
				}
			}
        }
    }
	return true;

	//lprintf( "Ground react at %g", tick );
}


