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
	instance->time_since_spawn += tick;
	//lprintf( "tick is %g", tick );
	while( instance->time_since_spawn > 25 )
	{
		    Long RSx = self.Sector->Pos_x << ZVOXELBLOCSHIFT_X;
			Long RSy = self.Sector->Pos_y << ZVOXELBLOCSHIFT_Y;
			Long RSz = self.Sector->Pos_z << ZVOXELBLOCSHIFT_Z;

		//UShort above = self.World->GetVoxel( RSx +self.x, RSy +self.y+1, RSz +self.z );
		//if( above == 0 )
		{
			ULong number = SRG_GetEntropy( ZVoxelReactor::Random2, 5, 0 );
			number = number % 27;
			if( number != 13 )
			{ int x, y, z;
				// add an aroma
				UShort next = self.World->GetVoxel( x = RSx +self.x + ( number % 3 - 1 ), y = RSy +self.y+1+ ( (number / 3) % 3 - 1 ), z = RSz +self.z + ( (number / 9) % 3 - 1 ) );
				if( VoxelTypeManager->VoxelTable[next]->Is_CanBeReplacedBy_Water) 
				{
					self.World->SetVoxel_WithCullingUpdate(x, y, z, (self.x & 1)/*SRG_GetEntropy( ZVoxelReactor::Random2, 1, 0 )*/?242:244, ZVoxelSector::CHANGE_CRITICAL);                                         
					spawned++;
				}
			}
		}
		//lprintf( "Setting %p to 0", &instance->time_since_spawn );
		instance->time_since_spawn -= 25;
	}
	//lprintf( "Ground react at %g", tick );
	return true;
}


