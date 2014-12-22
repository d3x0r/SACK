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
 * ZVoxelType_AromaGenerator.cpp
 *
 *  Created on: 4 févr. 2013
 *      Author: laurent
 */

#ifndef Z_VOXELTYPE_AROMAGENERATOR_H
#  include "ZVoxelType_AromaGenerator.h"
#endif

#ifndef Z_ZVOXELEXTENSION_AROMAGENERATOR_H
#  include "ZVoxelExtension_AromaGenerator.h"
#endif

#include "ZVoxelReactor.h"

ZVoxelExtension * ZVoxelType_AromaGenerator::CreateVoxelExtension(bool IsLoadingPhase)
{
  ZVoxelExtension_AromaGenerator * NewVoxelExtension = 0;

  NewVoxelExtension = new ZVoxelExtension_AromaGenerator;
  NewVoxelExtension->time_since_spawn = ( ( -rand() * 2000.0 ) / RAND_MAX );
  return (NewVoxelExtension);
}

bool ZVoxelType_AromaGenerator::React( const ZVoxelRef &self, double tick )
{
	ZVoxelExtension_AromaGenerator *instance = (ZVoxelExtension_AromaGenerator *)self.VoxelExtension;
	int spawned = 0;
    self.Sector->ModifTracker.Set(self.Offset);
	instance->time_since_spawn += tick;
	//lprintf( "tick is %g", tick );
	while( instance->time_since_spawn > 25 )
	{
		ULong number = SRG_GetEntropy( ZVoxelReactor::Random2, 5, 0 );
		number = number % 27;
		if( number != 0 )
		{ 
			// add an aroma
			ZVoxelSector *next_sector;
			ULong offset;
			ZVoxelSector::GetNearVoxel( self.Sector, self.Offset, &next_sector, offset, (RelativeVoxelOrds)number ); 

			UShort next = next_sector->Data.Data[offset];
			if( VoxelTypeManager->VoxelTable[next]->Is_CanBeReplacedBy_Water) 
			{
				self.World->SetVoxel_WithCullingUpdate( next_sector, offset, (self.x & 1)/*SRG_GetEntropy( ZVoxelReactor::Random2, 1, 0 )*/?242:244, ZVoxelSector::CHANGE_CRITICAL, true );                                         
				/*
				lprintf( "Voxel generator %d %d %d into %s %d,%d,%d", self.wx, self.wy, self.wz
					, (next_sector == self.Sector)?"self":"near"
					, (offset>>ZVOXELBLOCSHIFT_Y)&ZVOXELBLOCMASK_X, offset & ZVOXELBLOCMASK_Y, offset >> ( ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X) );
				*/
				spawned++;
			}
		}
		instance->time_since_spawn -= 25;
	}
	//lprintf( "Ground react at %g", tick );
	return true;
}


