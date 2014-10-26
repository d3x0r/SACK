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
#include <GL/glew.h>
#include <math.h>
#include <stdio.h>

#  include "ZRender_Basic.h"

#ifndef Z_ZHIGHPERFTIMER_H
#  include "ZHighPerfTimer.h"
#endif

#ifndef Z_ZGAME_H
#  include "ZGame.h"
#endif

#ifndef Z_ZGAMESTAT_H
#  include "ZGameStat.h"
#endif

ULong ZVoxelCuller_Basic::getFaceCulling( ZVoxelSector *Sector, int offset )
{
	return *( ( (UByte*)Sector->Culling) + offset );
}
void ZVoxelCuller_Basic::setFaceCulling( ZVoxelSector *Sector, int offset, ULong value )
{
	*( ( (UByte*)Sector->Culling) + offset ) = value;
}

void ZVoxelCuller_Basic::InitFaceCullData( ZVoxelSector *Sector )
{
	Sector->Culler = this;
	Sector->Culling = new UByte[Sector->DataSize];
}
void ZVoxelCuller_Basic::CullSector( ZVoxelSector *Sector, bool internal )
{
}
void ZVoxelCuller_Basic::CullSingleVoxel( ZVoxelSector *Sector, int x, int y, int z )
{
}

#if 0
bool ZVoxelWorld::SetVoxel_WithCullingUpdate(Long x, Long y, Long z, UShort VoxelValue, UByte ImportanceFactor, bool CreateExtension, VoxelLocation * Location)
{
  UShort * Voxel_Address[19];
  ULong  Offset[19];
  ULong * FaceCulling_Address[19];
  UShort VoxelState[19];
  UShort Voxel;
  ZVoxelSector * Sector[19];
  ZVoxelType ** VoxelTypeTable;
  ZVoxelType * VoxelType;

  UShort * ExtFaceState;
  UShort * IntFaceState;
  ZMemSize OtherInfos;

  VoxelTypeTable = VoxelTypeManager->VoxelTable;

  // Fetching sectors

  if ( 0== (Sector[VOXEL_INCENTER]= FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) return(false);
  if ( 0== (Sector[VOXEL_LEFT]    = FindSector( (x-1) >> ZVOXELBLOCSHIFT_X , (y)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_LEFT]    = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_RIGHT]   = FindSector( (x+1) >> ZVOXELBLOCSHIFT_X , (y)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_RIGHT]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_INFRONT] = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y)     >> ZVOXELBLOCSHIFT_Y , (z-1) >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_INFRONT] = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_BEHIND]  = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y)     >> ZVOXELBLOCSHIFT_Y , (z+1) >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_BEHIND]  = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_ABOVE]   = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y + 1) >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_ABOVE]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_BELOW]   = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y - 1) >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_BELOW]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_LEFT_ABOVE]    = FindSector( (x-1) >> ZVOXELBLOCSHIFT_X , (y+1)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_LEFT_ABOVE]    = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_RIGHT_ABOVE]   = FindSector( (x+1) >> ZVOXELBLOCSHIFT_X , (y+1)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_RIGHT_ABOVE]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_INFRONT_ABOVE] = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y+1)     >> ZVOXELBLOCSHIFT_Y , (z-1) >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_INFRONT_ABOVE] = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_BEHIND_ABOVE]  = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y+1)     >> ZVOXELBLOCSHIFT_Y , (z+1) >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_BEHIND_ABOVE]  = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_ABOVE_AHEAD]   = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y + 1) >> ZVOXELBLOCSHIFT_Y , (z-1)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_ABOVE_AHEAD]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_BELOW_AHEAD]   = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y - 1) >> ZVOXELBLOCSHIFT_Y , (z-1)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_BELOW_AHEAD]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_LEFT_BELOW]    = FindSector( (x-1) >> ZVOXELBLOCSHIFT_X , (y-1)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_LEFT_BELOW]    = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_RIGHT_BELOW]   = FindSector( (x+1) >> ZVOXELBLOCSHIFT_X , (y-1)     >> ZVOXELBLOCSHIFT_Y , (z)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_RIGHT_BELOW]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_INFRONT_BELOW] = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y-1)     >> ZVOXELBLOCSHIFT_Y , (z-1) >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_INFRONT_BELOW] = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_BEHIND_BELOW]  = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y-1)     >> ZVOXELBLOCSHIFT_Y , (z+1) >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_BEHIND_BELOW]  = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_ABOVE_BEHIND]   = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y + 1) >> ZVOXELBLOCSHIFT_Y , (z+1)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_ABOVE_BEHIND]   = this->WorkingScratchSector;
  if ( 0== (Sector[VOXEL_BELOW_BEHIND]   = FindSector( (x)   >> ZVOXELBLOCSHIFT_X , (y - 1) >> ZVOXELBLOCSHIFT_Y , (z+1)   >> ZVOXELBLOCSHIFT_Z ) ) ) Sector[VOXEL_BELOW_BEHIND]   = this->WorkingScratchSector;

  // Computing memory offsets from sector start

  Offset[VOXEL_LEFT]     = (y & ZVOXELBLOCMASK_Y)       + ( ((x - 1) & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y ) + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_RIGHT]    = (y & ZVOXELBLOCMASK_Y)       + ( ((x + 1) & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y ) + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_INFRONT]  = (y & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z - 1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_BEHIND]   = (y & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z + 1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_ABOVE]    = ((y + 1) & ZVOXELBLOCMASK_Y) + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_BELOW]    = ((y - 1) & ZVOXELBLOCMASK_Y) + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_LEFT_ABOVE]     = ((y + 1) & ZVOXELBLOCMASK_Y)       + ( ((x - 1) & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y ) + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_RIGHT_ABOVE]    = ((y + 1) & ZVOXELBLOCMASK_Y)       + ( ((x + 1) & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y ) + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_INFRONT_ABOVE]  = ((y + 1) & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z - 1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_BEHIND_ABOVE]   = ((y + 1) & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z + 1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_ABOVE_AHEAD]    = ((y + 1) & ZVOXELBLOCMASK_Y) + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z-1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_BELOW_AHEAD]    = ((y - 1) & ZVOXELBLOCMASK_Y) + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z-1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_LEFT_BELOW]     = ((y - 1) & ZVOXELBLOCMASK_Y)       + ( ((x - 1) & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y ) + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_RIGHT_BELOW]    = ((y - 1) & ZVOXELBLOCMASK_Y)       + ( ((x + 1) & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y ) + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_INFRONT_BELOW]  = ((y - 1) & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z - 1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_BEHIND_BELOW]   = ((y - 1) & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z + 1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_ABOVE_BEHIND]    = ((y + 1) & ZVOXELBLOCMASK_Y) + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z+1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_BELOW_BEHIND]    = ((y - 1) & ZVOXELBLOCMASK_Y) + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + (((z+1) & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));
  Offset[VOXEL_INCENTER] = (y & ZVOXELBLOCMASK_Y)       + ( (x & ZVOXELBLOCMASK_X)<<ZVOXELBLOCSHIFT_Y )       + ((z & ZVOXELBLOCMASK_Z) << (ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_X));

  // Computing absolute memory pointer of blocks
  for( int i = 0; i < 19; i++ )
  {
	Voxel_Address[i]     = Sector[i]->Data + Offset[i];
    FaceCulling_Address[i]     = Sector[i]->FaceCulling + Offset[i];
    Voxel = *Voxel_Address[i];    VoxelType = VoxelTypeTable[Voxel];
      VoxelState[i] = ( (Voxel==0) ? 1 : 0) 
		     | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) 
			 | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  }
  /*
  Voxel_Address[VOXEL_RIGHT]    = Sector[VOXEL_RIGHT]->Data + Offset[VOXEL_RIGHT];
  Voxel_Address[VOXEL_INFRONT]  = Sector[VOXEL_INFRONT]->Data + Offset[VOXEL_INFRONT];
  Voxel_Address[VOXEL_BEHIND]   = Sector[VOXEL_BEHIND]->Data + Offset[VOXEL_BEHIND];
  Voxel_Address[VOXEL_ABOVE]    = Sector[VOXEL_ABOVE]->Data + Offset[VOXEL_ABOVE];
  Voxel_Address[VOXEL_BELOW]    = Sector[VOXEL_BELOW]->Data + Offset[VOXEL_BELOW];
  Voxel_Address[VOXEL_INCENTER] = Sector[VOXEL_INCENTER]->Data + Offset[VOXEL_INCENTER];
  */
  // Computing absolute
  /*
  FaceCulling_Address[VOXEL_LEFT]     = Sector[VOXEL_LEFT]->FaceCulling + Offset[VOXEL_LEFT];
  FaceCulling_Address[VOXEL_RIGHT]    = Sector[VOXEL_RIGHT]->FaceCulling + Offset[VOXEL_RIGHT];
  FaceCulling_Address[VOXEL_INFRONT]  = Sector[VOXEL_INFRONT]->FaceCulling + Offset[VOXEL_INFRONT];
  FaceCulling_Address[VOXEL_BEHIND]   = Sector[VOXEL_BEHIND]->FaceCulling + Offset[VOXEL_BEHIND];
  FaceCulling_Address[VOXEL_ABOVE]    = Sector[VOXEL_ABOVE]->FaceCulling + Offset[VOXEL_ABOVE];
  FaceCulling_Address[VOXEL_BELOW]    = Sector[VOXEL_BELOW]->FaceCulling + Offset[VOXEL_BELOW];
  FaceCulling_Address[VOXEL_INCENTER] = Sector[VOXEL_INCENTER]->FaceCulling + Offset[VOXEL_INCENTER];
  */
  // Fetching Voxels and computing voxel state

  /*
  Voxel = *Voxel_Address[VOXEL_LEFT];    VoxelType = VoxelTypeTable[Voxel];
    VoxelState[VOXEL_LEFT] = ((Voxel==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  Voxel = *Voxel_Address[VOXEL_RIGHT];   VoxelType = VoxelTypeTable[Voxel];
    VoxelState[VOXEL_RIGHT] = ((Voxel==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  Voxel = *Voxel_Address[VOXEL_INFRONT]; VoxelType = VoxelTypeTable[Voxel];
    VoxelState[VOXEL_INFRONT] = ((Voxel==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  Voxel = *Voxel_Address[VOXEL_BEHIND];  VoxelType = VoxelTypeTable[Voxel];
    VoxelState[VOXEL_BEHIND] = ((Voxel==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  Voxel = *Voxel_Address[VOXEL_ABOVE];   VoxelType = VoxelTypeTable[Voxel];
    VoxelState[VOXEL_ABOVE] = ((Voxel==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  Voxel = *Voxel_Address[VOXEL_BELOW];   VoxelType = VoxelTypeTable[Voxel];
    VoxelState[VOXEL_BELOW] = ((Voxel==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );
  */
  // Delete Old voxel extended informations if any

  Voxel = *Voxel_Address[VOXEL_INCENTER];
  OtherInfos = *(Sector[VOXEL_INCENTER]->OtherInfos + Offset[VOXEL_INCENTER]);

  if (OtherInfos)
  {
    VoxelType = VoxelTypeTable[Voxel];
    if (VoxelType->Is_HasAllocatedMemoryExtension) VoxelType->DeleteVoxelExtension(OtherInfos);
  }

  // Storing Extension

  VoxelType = VoxelTypeTable[VoxelValue];
  if (CreateExtension)
  {
    *Voxel_Address[VOXEL_INCENTER] = 0; // Temporary set to 0 to prevent VoxelReactor for crashing while loading the wrong extension.
    *(Sector[VOXEL_INCENTER]->OtherInfos + Offset[VOXEL_INCENTER]) =(ZMemSize)VoxelType->CreateVoxelExtension();
  }

  // Storing Voxel

  *Voxel_Address[VOXEL_INCENTER] = VoxelValue;
  VoxelState[VOXEL_INCENTER] = ((VoxelValue==0) ? 1 : 0) | ( VoxelType->Draw_FullVoxelOpacity ? 2 : 0 ) | ( VoxelType->Draw_TransparentRendering ? 4 : 0 );


  if (VoxelTypeTable[VoxelValue]->Is_Active) Sector[VOXEL_INCENTER]->Flag_IsActiveVoxels = true;

  // Filling VoxelLocation if any

  if ((Location))
  {
    Location->Sector = Sector[VOXEL_INCENTER];
    Location->Offset = Offset[VOXEL_INCENTER];
  }

  // Getting case subtables.

  ExtFaceState = &ExtFaceStateTable[VoxelState[VOXEL_INCENTER]][0];
  IntFaceState = &IntFaceStateTable[VoxelState[VOXEL_INCENTER]][0];

  // Computing face culling for center main stored voxel.

  *FaceCulling_Address[VOXEL_INCENTER] =   (IntFaceState[VoxelState[VOXEL_LEFT]]   & DRAWFACE_LEFT)
                                         | (IntFaceState[VoxelState[VOXEL_RIGHT]]   & DRAWFACE_RIGHT)
                                         | (IntFaceState[VoxelState[VOXEL_INFRONT]] & DRAWFACE_AHEAD)
                                         | (IntFaceState[VoxelState[VOXEL_BEHIND]]  & DRAWFACE_BEHIND)
                                         | (IntFaceState[VoxelState[VOXEL_ABOVE]]   & DRAWFACE_ABOVE)
                                         | (IntFaceState[VoxelState[VOXEL_BELOW]]   & DRAWFACE_BELOW)
										 | (IntFaceState[VoxelState[VOXEL_LEFT_ABOVE]]   & DRAWFACE_LEFT_HAS_ABOVE)
                                         | (IntFaceState[VoxelState[VOXEL_RIGHT_ABOVE]]   & DRAWFACE_RIGHT_HAS_ABOVE)
                                         | (IntFaceState[VoxelState[VOXEL_INFRONT_ABOVE]] & DRAWFACE_AHEAD_HAS_ABOVE)
                                         | (IntFaceState[VoxelState[VOXEL_BEHIND_ABOVE]]  & DRAWFACE_BEHIND_HAS_ABOVE)
                                         | (IntFaceState[VoxelState[VOXEL_ABOVE_AHEAD]]   & DRAWFACE_ABOVE_HAS_AHEAD)
                                         | (IntFaceState[VoxelState[VOXEL_BELOW_AHEAD]]   & DRAWFACE_BELOW_HAS_AHEAD)
										 | (IntFaceState[VoxelState[VOXEL_LEFT_BELOW]]   & DRAWFACE_LEFT_HAS_BELOW)
                                         | (IntFaceState[VoxelState[VOXEL_RIGHT_BELOW]]   & DRAWFACE_RIGHT_HAS_BELOW)
                                         | (IntFaceState[VoxelState[VOXEL_INFRONT_BELOW]] & DRAWFACE_AHEAD_HAS_BELOW)
                                         | (IntFaceState[VoxelState[VOXEL_BEHIND_BELOW]]  & DRAWFACE_BEHIND_HAS_BELOW)
                                         | (IntFaceState[VoxelState[VOXEL_ABOVE_BEHIND]]   & DRAWFACE_ABOVE_HAS_BEHIND)
                                         | (IntFaceState[VoxelState[VOXEL_BELOW_BEHIND]]   & DRAWFACE_BELOW_HAS_BEHIND)
										 ;

  // Computing face culling for nearboring voxels faces touching center voxel.

  *FaceCulling_Address[VOXEL_LEFT]    &= DRAWFACE_ALL_BITS ^ DRAWFACE_RIGHT;  *FaceCulling_Address[VOXEL_LEFT]    |= ExtFaceState[ VoxelState[VOXEL_LEFT]   ]  & DRAWFACE_RIGHT;
  *FaceCulling_Address[VOXEL_RIGHT]   &= DRAWFACE_ALL_BITS ^ DRAWFACE_LEFT;   *FaceCulling_Address[VOXEL_RIGHT]   |= ExtFaceState[ VoxelState[VOXEL_RIGHT]  ]  & DRAWFACE_LEFT;
  *FaceCulling_Address[VOXEL_INFRONT] &= DRAWFACE_ALL_BITS ^ DRAWFACE_BEHIND; *FaceCulling_Address[VOXEL_INFRONT] |= ExtFaceState[ VoxelState[VOXEL_INFRONT]]  & DRAWFACE_BEHIND;
  *FaceCulling_Address[VOXEL_BEHIND]  &= DRAWFACE_ALL_BITS ^ DRAWFACE_AHEAD;  *FaceCulling_Address[VOXEL_BEHIND]  |= ExtFaceState[ VoxelState[VOXEL_BEHIND] ]  & DRAWFACE_AHEAD;
  *FaceCulling_Address[VOXEL_ABOVE]   &= DRAWFACE_ALL_BITS ^ DRAWFACE_BELOW;  *FaceCulling_Address[VOXEL_ABOVE]   |= ExtFaceState[ VoxelState[VOXEL_ABOVE]  ]  & DRAWFACE_BELOW;
  *FaceCulling_Address[VOXEL_BELOW]   &= DRAWFACE_ALL_BITS ^ DRAWFACE_ABOVE;  *FaceCulling_Address[VOXEL_BELOW]   |= ExtFaceState[ VoxelState[VOXEL_BELOW]  ]  & DRAWFACE_ABOVE;


  *FaceCulling_Address[VOXEL_LEFT_ABOVE]    &= DRAWFACE_ALL_BITS ^ DRAWFACE_RIGHT;  *FaceCulling_Address[VOXEL_LEFT]    |= ExtFaceState[ VoxelState[VOXEL_LEFT]   ]  & DRAWFACE_RIGHT;
  *FaceCulling_Address[VOXEL_RIGHT_ABOVE]   &= DRAWFACE_ALL_BITS ^ DRAWFACE_LEFT;   *FaceCulling_Address[VOXEL_RIGHT]   |= ExtFaceState[ VoxelState[VOXEL_RIGHT]  ]  & DRAWFACE_LEFT;
  *FaceCulling_Address[VOXEL_INFRONT_ABOVE] &= DRAWFACE_ALL_BITS ^ DRAWFACE_BEHIND; *FaceCulling_Address[VOXEL_INFRONT] |= ExtFaceState[ VoxelState[VOXEL_INFRONT]]  & DRAWFACE_BEHIND;
  *FaceCulling_Address[VOXEL_BEHIND_ABOVE]  &= DRAWFACE_ALL_BITS ^ DRAWFACE_AHEAD;  *FaceCulling_Address[VOXEL_BEHIND]  |= ExtFaceState[ VoxelState[VOXEL_BEHIND] ]  & DRAWFACE_AHEAD;
  *FaceCulling_Address[VOXEL_ABOVE_AHEAD]   &= DRAWFACE_ALL_BITS ^ DRAWFACE_BELOW;  *FaceCulling_Address[VOXEL_ABOVE]   |= ExtFaceState[ VoxelState[VOXEL_ABOVE]  ]  & DRAWFACE_BELOW;
  *FaceCulling_Address[VOXEL_BELOW_AHEAD]   &= DRAWFACE_ALL_BITS ^ DRAWFACE_ABOVE;  *FaceCulling_Address[VOXEL_BELOW]   |= ExtFaceState[ VoxelState[VOXEL_BELOW]  ]  & DRAWFACE_ABOVE;

  // printf("State[Center]:%x [Left]%x [Right]%x [INFRONT]%x [BEHIND]%x [ABOVE]%x [BELOW]%x\n",VoxelState[VOXEL_INCENTER],VoxelState[VOXEL_LEFT],VoxelState[VOXEL_RIGHT],VoxelState[VOXEL_INFRONT],VoxelState[VOXEL_BEHIND],VoxelState[VOXEL_ABOVE],VoxelState[VOXEL_BELOW]);

  // Updating sector status rendering flag status
  for( int i = 0; i < 19; i++ )
  {
	Sector[i]->Flag_Render_Dirty = true;
  }
  /*
  Sector[VOXEL_INCENTER]->Flag_Render_Dirty = true;
  Sector[VOXEL_LEFT    ]->Flag_Render_Dirty = true;
  Sector[VOXEL_RIGHT   ]->Flag_Render_Dirty = true;
  Sector[VOXEL_INFRONT ]->Flag_Render_Dirty = true;
  Sector[VOXEL_BEHIND  ]->Flag_Render_Dirty = true;
  Sector[VOXEL_ABOVE   ]->Flag_Render_Dirty = true;
  Sector[VOXEL_BELOW   ]->Flag_Render_Dirty = true;
  */

  //Sector[VOXEL_INCENTER]->Flag_Void_Regular = false;
  //Sector[VOXEL_LEFT    ]->Flag_Void_Regular = false;
  //Sector[VOXEL_RIGHT   ]->Flag_Void_Regular = false;
  //Sector[VOXEL_INFRONT ]->Flag_Void_Regular = false;
  //Sector[VOXEL_BEHIND  ]->Flag_Void_Regular = false;
  //Sector[VOXEL_ABOVE   ]->Flag_Void_Regular = false;
  //Sector[VOXEL_BELOW   ]->Flag_Void_Regular = false;
  //Sector[VOXEL_INCENTER]->Flag_Void_Transparent = false;
  //Sector[VOXEL_LEFT    ]->Flag_Void_Transparent = false;
  //Sector[VOXEL_RIGHT   ]->Flag_Void_Transparent = false;
  //Sector[VOXEL_INFRONT ]->Flag_Void_Transparent = false;
  //Sector[VOXEL_BEHIND  ]->Flag_Void_Transparent = false;
  //Sector[VOXEL_ABOVE   ]->Flag_Void_Transparent = false;
  //Sector[VOXEL_BELOW   ]->Flag_Void_Transparent = false;
  Sector[VOXEL_INCENTER]->Flag_IsModified |= ImportanceFactor;
            //Sector[VOXEL_LEFT    ]->Flag_IsModified = true;
            //Sector[VOXEL_RIGHT   ]->Flag_IsModified = true;
            //Sector[VOXEL_INFRONT ]->Flag_IsModified = true;
            //Sector[VOXEL_BEHIND  ]->Flag_IsModified = true;
            //Sector[VOXEL_ABOVE   ]->Flag_IsModified = true;
            //Sector[VOXEL_BELOW   ]->Flag_IsModified = true;
  return(true);
}


#endif

void ZRender_Basic::CullMatingSectors( ZVoxelSector *Sector )
{
#if 0
ULong ZVoxelWorld::SectorUpdateFaceCulling_Partial(Long x, Long y, Long z, UByte FacesToDraw, bool Isolated)
{
  ZVoxelSector * SectorTable[27];
  ZVoxelType ** VoxelTypeTable;
  ZVoxelSector * MissingSector;
  ZVoxelSector * Sector_In, * Sector_Out;

  ULong i, CuledFaces;
  ULong Off_Ip, Off_In, Off_Op , Off_Out, Off_Aux;
  UShort * VoxelData_In, * VoxelData_Out;
  ULong * VoxelFC_In;

  UByte FaceState;


  if (Isolated) MissingSector = WorkingEmptySector;
  else          MissingSector = WorkingFullSector;

  for (i=0;i<27;i++) SectorTable[i] = MissingSector;
  SectorTable[0] = FindSector(x,y,z);    if (!SectorTable[0] ) {SectorTable[0]  = MissingSector;}
  SectorTable[1] = FindSector(x-1,y,z);  if (!SectorTable[1] ) {SectorTable[1]  = MissingSector;}
  SectorTable[2] = FindSector(x+1,y,z);  if (!SectorTable[2] ) {SectorTable[2]  = MissingSector;}
  SectorTable[3] = FindSector(x,y,z-1);  if (!SectorTable[3] ) {SectorTable[3]  = MissingSector;}
  SectorTable[6] = FindSector(x,y,z+1);  if (!SectorTable[6] ) {SectorTable[6]  = MissingSector;}
  SectorTable[9] = FindSector(x,y-1,z);  if (!SectorTable[9] ) {SectorTable[9]  = MissingSector;}
  SectorTable[18]= FindSector(x,y+1,z);  if (!SectorTable[18]) {SectorTable[18] = MissingSector;}


  VoxelTypeTable = this->VoxelTypeManager->VoxelTable;

  Sector_In  = FindSector(x,y,z); if (!Sector_In) return(0);
  CuledFaces = 0;

  // Top Side

  if (FacesToDraw & DRAWFACE_ABOVE)
  {
    if ( (Sector_Out = FindSector(x,y+1,z)))
    {
      VoxelData_In  = Sector_In->Data;
      VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;

      for ( Off_Ip=ZVOXELBLOCSIZE_Y-1, Off_Op=0 ; Off_Ip < (ZVOXELBLOCSIZE_Y * 16) ; Off_Ip+=ZVOXELBLOCSIZE_Y, Off_Op+=ZVOXELBLOCSIZE_Y ) // x (0..15)
      {
        for ( Off_Aux=0; Off_Aux < (ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Y * 16) ; Off_Aux+=(ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Y) ) // z (0..15)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ VoxelData_Out[Off_Out] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) 
		  {
			  VoxelFC_In[Off_In] |= DRAWFACE_ABOVE; 
		  }
		  else 
		  {
			  VoxelFC_In[Off_In] &= ~DRAWFACE_ABOVE;
		  }
        }
      }
      CuledFaces |= DRAWFACE_ABOVE;
    }
	else
	{
      VoxelData_In  = Sector_In->Data;
      //VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;
      for ( Off_Ip=ZVOXELBLOCSIZE_Y-1, Off_Op=0 ; Off_Ip < (ZVOXELBLOCSIZE_Y * 16) ; Off_Ip+=ZVOXELBLOCSIZE_Y, Off_Op+=ZVOXELBLOCSIZE_Y ) // x (0..15)
      {
        for ( Off_Aux=0; Off_Aux < (ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Y * 16) ; Off_Aux+=(ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Y) ) // z (0..15)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ 0 ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) 
			  VoxelFC_In[Off_In] |= DRAWFACE_ABOVE; 
		  else 
			  VoxelFC_In[Off_In] &= ~DRAWFACE_ABOVE;
        }
      }

	}
  }
  // Bottom Side

  if (FacesToDraw & DRAWFACE_BELOW)
    if ((Sector_Out = FindSector(x,y-1,z)))
    {



      VoxelData_In  = Sector_In->Data;
      VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;

      for ( Off_Ip=0, Off_Op=ZVOXELBLOCSIZE_Y-1 ; Off_Ip < (ZVOXELBLOCSIZE_Y * 16) ; Off_Ip+=ZVOXELBLOCSIZE_Y, Off_Op+=ZVOXELBLOCSIZE_Y ) // x (0..15)
      {
        for ( Off_Aux=0; Off_Aux < (ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Y * 16) ; Off_Aux+=(ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Y) ) // z (0..15)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          UShort Voxel_In = VoxelData_In[Off_In];
          UShort Voxel_Out = VoxelData_Out[Off_Out];
          //ZVoxelType * VtIn =  VoxelTypeTable[ Voxel_In ];
          //ZVoxelType * VtOut = VoxelTypeTable[ Voxel_Out ];


          FaceState = IntFaceStateTable[ VoxelTypeTable[ Voxel_In ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ Voxel_Out ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];

          //FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ VoxelData_Out[Off_Out] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) VoxelFC_In[Off_In] |= DRAWFACE_BELOW; else VoxelFC_In[Off_In] &= ~DRAWFACE_BELOW;
        }
      }
      CuledFaces |= DRAWFACE_BELOW;
    }

  // Left Side

  if (FacesToDraw & DRAWFACE_LEFT)
    if ((Sector_Out = FindSector(x-1,y,z)))
    {
      VoxelData_In  = Sector_In->Data;
      VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;
      // VoxelData_In[63]=1;
      // VoxelData_In[63 + ZVOXELBLOCSIZE_Y*15 ]=14; // x
      // VoxelData_In[63 + ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X * 15] = 13; // z

      for ( Off_Ip=0, Off_Op=ZVOXELBLOCSIZE_Y * (ZVOXELBLOCSIZE_X-1) ; Off_Ip < (ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Z) ; Off_Ip+=(ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X), Off_Op+=(ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X) ) // z (0..15)
      {
        for ( Off_Aux=0; Off_Aux < ZVOXELBLOCSIZE_Y ; Off_Aux++  ) // y (0..63)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          //VoxelData_In[Off_In]=1; VoxelData_Out[Off_Out]=14;
          FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ VoxelData_Out[Off_Out] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) VoxelFC_In[Off_In] |= DRAWFACE_LEFT; else VoxelFC_In[Off_In] &= ~DRAWFACE_LEFT;


        }
      }
      CuledFaces |= DRAWFACE_LEFT;
    }

  // Right Side

  if (FacesToDraw & DRAWFACE_RIGHT)
    if ((Sector_Out = FindSector(x+1,y,z)))
    {
      VoxelData_In  = Sector_In->Data;
      VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;

      for ( Off_Ip=ZVOXELBLOCSIZE_Y * (ZVOXELBLOCSIZE_X-1), Off_Op=0 ; Off_Op < (ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X * ZVOXELBLOCSIZE_Z) ; Off_Ip+=(ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X), Off_Op+=(ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X) ) // z (0..15)
      {
        for ( Off_Aux=0; Off_Aux < ZVOXELBLOCSIZE_Y ; Off_Aux++  ) // y (0..63)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ VoxelData_Out[Off_Out] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) VoxelFC_In[Off_In] |= DRAWFACE_RIGHT; else VoxelFC_In[Off_In] &= ~DRAWFACE_RIGHT;
        }
      }
      CuledFaces |= DRAWFACE_RIGHT;
    }

  // Front Side

  if (FacesToDraw & DRAWFACE_AHEAD)
    if ((Sector_Out = FindSector(x,y,z-1)))
    {
      VoxelData_In  = Sector_In->Data;
      VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;
      for ( Off_Ip=0, Off_Op= (ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X * (ZVOXELBLOCSIZE_Z-1)) ; Off_Ip < (ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X) ; Off_Ip+=ZVOXELBLOCSIZE_Y, Off_Op+=ZVOXELBLOCSIZE_Y ) // x (0..15)
      {
        for ( Off_Aux=0; Off_Aux < ZVOXELBLOCSIZE_Y ; Off_Aux++  ) // y (0..63)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ VoxelData_Out[Off_Out] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) VoxelFC_In[Off_In] |= DRAWFACE_AHEAD; else VoxelFC_In[Off_In] &= ~DRAWFACE_AHEAD;
        }
      }
      CuledFaces |= DRAWFACE_AHEAD;
    }

  // Back Side

  if (FacesToDraw & DRAWFACE_BEHIND)
    if ((Sector_Out = FindSector(x,y,z+1)))
    {
      VoxelData_In  = Sector_In->Data;
      VoxelData_Out = Sector_Out->Data;
      VoxelFC_In    = Sector_In->FaceCulling;

      for ( Off_Ip=(ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X * (ZVOXELBLOCSIZE_Z-1)) , Off_Op=0 ; Off_Op < (ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X) ; Off_Ip+=ZVOXELBLOCSIZE_Y, Off_Op+=ZVOXELBLOCSIZE_Y ) // x (0..15)
      {
        for ( Off_Aux=0; Off_Aux < ZVOXELBLOCSIZE_Y ; Off_Aux++  ) // y (0..63)
        {
          Off_In = Off_Ip + Off_Aux;
          Off_Out= Off_Op + Off_Aux;
          FaceState = IntFaceStateTable[ VoxelTypeTable[ VoxelData_In[Off_In] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ][ VoxelTypeTable[ VoxelData_Out[Off_Out] ]->DrawInfo & ZVOXEL_DRAWINFO_CULLINGBITS ];
          if (FaceState) VoxelFC_In[Off_In] |= DRAWFACE_BEHIND; else VoxelFC_In[Off_In] &= ~DRAWFACE_BEHIND;
        }
      }
      CuledFaces |= DRAWFACE_BEHIND;
    }

  return(CuledFaces);
}

#endif

}


void ZRender_Basic::Render()
{

  ZHighPerfTimer Timer;
#if COMPILEOPTION_FINETIMINGTRACKING == 1
  ZHighPerfTimer Timer_SectorRefresh;
  ULong Time;
#endif
  ULong RenderedSectors;
  Long i;

   Timer.Start();

   Stat_RenderDrawFaces = 0;
   Stat_FaceTop = 0;
   Stat_FaceBottom = 0;
   Stat_FaceLeft = 0;
   Stat_FaceRight = 0;
   Stat_FaceFront = 0;
   Stat_FaceBack = 0;

 // Stats reset

   ZGameStat * Stat = GameEnv->GameStat;



  // Precomputing values for faster math

  //ZVector3d::ZTransformParam FastCamParameters;

  //FastCamParameters.SetRotation(Camera->orientation);
  //FastCamParameters.SetTranslation(-Camera->x, -Camera->y, -Camera->z);


   // Update per cycle.
   ULong UpdatePerCycle = 2;
   ULong n;


   if (Stat_RefreshWaitingSectorCount < 50) UpdatePerCycle = 1;
   if (Stat_RefreshWaitingSectorCount < 500) UpdatePerCycle = 2;
   else if (Stat->SectorRefresh_TotalTime <32) UpdatePerCycle = 5;
   Stat_RefreshWaitingSectorCount = 0;

   // Stat Reset

   Stat->SectorRefresh_Count = 0;
   Stat->SectorRefresh_TotalTime = 0;
   Stat->SectorRefresh_MinTime = 0;
   Stat->SectorRefresh_MaxTime = 0;
   Stat->SectorRender_Count = 0;
   Stat->SectorRender_TotalTime = 0;
   Stat->SectorRefresh_Waiting = 0;

   // Renderwaiting system

   for (i=0;i<64;i++) RefreshToDo[i] = 0;
   for (i=63;i>0;i--)
   {
     n = RefreshWaiters[i];
     if (n>UpdatePerCycle) n = UpdatePerCycle;
     UpdatePerCycle -= n;
     RefreshToDo[i] = n;
   }
   RefreshToDo[0] = UpdatePerCycle;

   for (i=0;i<64;i++) RefreshWaiters[i]=0;

  // Computing Frustum and Setting up Projection

   Aspect_Ratio = ((double)ViewportResolution.x / (double)ViewportResolution.y) * PixelAspectRatio;
   if (VerticalFOV < 5.0 ) VerticalFOV = 5.0;
   if (VerticalFOV > 160.0 ) VerticalFOV = 160.0;
   Frustum_V = tan(VerticalFOV / 2.0 * 0.017453293) * FocusDistance;
   Frustum_H = Frustum_V * Aspect_Ratio;

   Frustum_CullingLimit = ((Frustum_H > Frustum_V) ? Frustum_H : Frustum_V) * Optimisation_FCullingFactor;


   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(Frustum_H, -Frustum_H, -Frustum_V, Frustum_V, FocusDistance, 1000000.0); // Official Way
// glFrustum(50.0, -50.0, -31.0, 31.0, 50.0, 1000000.0); // Official Way

    // glFrustum(165.0, -165.0, -31.0, 31.0, 50.0, 1000000.0); // Eyefinity setting.


  // Objects of the world are translated and rotated to position camera at the right place.

    glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
	glLoadMatrixd( Camera->orientation.glMatrix() );
    //glRotatef(-Camera->Roll  , 0.0, 0.0, 1.0);
    //glRotatef(-Camera->Pitch , 1.0, 0.0, 0.0);
    //glRotatef(180-Camera->Yaw, 0.0, 1.0, 0.0);

    //glTranslatef(-(float)Camera->x,-(float)Camera->y,-(float)Camera->z);

  // Clearing FrameBuffer and Z-Buffer

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glAlphaFunc(GL_GREATER, 0.2);
    glEnable(GL_ALPHA_TEST);

    // Long Start_x,Start_y,Start_z;
    Long Sector_x,Sector_y, Sector_z;
    // Long End_x, End_y, End_z;
    Long x,y,z;

    ZVoxelSector * Sector;
    Long Priority, PriorityBoost;
    ULong Sector_Refresh_Count;


  // Transforming Camera coords to sector coords. One Voxel is 256 observer units. One sector is 16x16x32.

    Sector_x = (ELong)Camera->x() >> ( 	GlobalSettings.VoxelBlockSizeBits + 4 );
    Sector_y = (ELong)Camera->y() >> ( 	GlobalSettings.VoxelBlockSizeBits + 6 );
    Sector_z = (ELong)Camera->z() >> ( 	GlobalSettings.VoxelBlockSizeBits + 4 );


    // Start_x = Sector_x - hRenderRadius; End_x = Sector_x + hRenderRadius;
    // Start_y = Sector_y - vRenderRadius; End_y = Sector_y + vRenderRadius;
    // Start_z = Sector_z - hRenderRadius; End_z = Sector_z + hRenderRadius;

  // Rendering loop

    // printf("x: %lf, y: %lf, z: %lf Pitch: %lf Yaw: %lf \n",Camera->x, Camera->y, Camera->z, Camera->Pitch, Camera->Yaw);

  // Preparation and first rendering pass

    RenderedSectors = 0;
    Sector_Refresh_Count = 0;
    ZVector3d Cv, Cv2;
    ZSectorSphere::ZSphereEntry * SectorSphereEntry;
    ULong SectorsToProcess = SectorSphere.GetEntryCount();

    for (ULong Entry=0; Entry<SectorsToProcess; Entry++ )
    {
      SectorSphereEntry = SectorSphere.GetEntry(Entry);

      x = SectorSphereEntry->x + Sector_x;
      y = SectorSphereEntry->y + Sector_y;
      z = SectorSphereEntry->z + Sector_z;

      // for (x = Start_x ; x <= End_x ; x++)
      // for (y = Start_y; y <= End_y ; y++)
      // for (z = Start_z; z <= End_z ; z++)

          // try to see if sector is visible

          ZVector3d Cv, Cv2;
          bool SectorVisible;

          Cv.x = (double) ( ((ELong)x) << ( 	GlobalSettings.VoxelBlockSizeBits + 4 ) );
          Cv.y = (double) ( ((ELong)y) << ( 	GlobalSettings.VoxelBlockSizeBits + 6 ) );
          Cv.z = (double) ( ((ELong)z) << ( 	GlobalSettings.VoxelBlockSizeBits + 4 ) );

          SectorVisible = false;
          Cv2.x = (0 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (0 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (0 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (1 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (0 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (0 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (1 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (0 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (1 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (0 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (0 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (1 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (0 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (1 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (0 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (1 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (1 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (0 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (1 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (1 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (1 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);
          Cv2.x = (0 * ZVOXELBLOCSIZE_X * GlobalSettings.VoxelBlockSize); Cv2.y = (1 * ZVOXELBLOCSIZE_Y * GlobalSettings.VoxelBlockSize); Cv2.z = (1 * ZVOXELBLOCSIZE_Z * GlobalSettings.VoxelBlockSize); Cv2 += Cv ; SectorVisible |= Is_PointVisible(Camera->orientation, &Cv2);

          Sector = World->FindSector(x,y,z);
          Priority      = RadiusZones.GetZone(x-Sector_x,y-Sector_y,z-Sector_z);
          PriorityBoost = (SectorVisible && Priority <= 2) ? 1 : 0;
          // Go = true;

          if (Sector)
          {
            Sector->Flag_IsVisibleAtLastRendering = SectorVisible || Priority>=4;
            // Display lists preparation
            if (Sector->Flag_Render_Dirty && GameEnv->Enable_NewSectorRendering)
            {
              if (Sector->Flag_IsDebug)
              {
                printf("Debug\n");
                //Sector->Flag_IsDebug = false;
              }

              // if (Sector_Refresh_Count < 5 || Priority==4)
              if ((RefreshToDo[Sector->RefreshWaitCount]) || Sector->Flag_HighPriorityRefresh )
              {

                #if COMPILEOPTION_FINETIMINGTRACKING == 1
                  Timer_SectorRefresh.Start();
                #endif

                RefreshToDo[Sector->RefreshWaitCount]--;
                Sector->Flag_HighPriorityRefresh = false;

                if (Sector->Flag_NeedSortedRendering) MakeSectorRenderingData_Sorted(Sector);
                else                                  MakeSectorRenderingData(Sector);

                Sector_Refresh_Count++;
                Sector->RefreshWaitCount = 0;
                Stat->SectorRefresh_Count++;

                #if COMPILEOPTION_FINETIMINGTRACKING == 1
                  Timer_SectorRefresh.End(); Time = Timer_SectorRefresh.GetResult(); Stat->SectorRefresh_TotalTime += Time; if (Time < Stat->SectorRefresh_MinTime ) Stat->SectorRefresh_MinTime = Time; if (Time > Stat->SectorRefresh_MaxTime ) Stat->SectorRefresh_MaxTime = Time;
                #endif
              }
              else
              {
                Sector->RefreshWaitCount++;
                if (Sector->RefreshWaitCount > 31) Sector->RefreshWaitCount = 31;
                if (Priority==4) Sector->RefreshWaitCount++;
                RefreshWaiters[Sector->RefreshWaitCount]++;
                Stat_RefreshWaitingSectorCount++;
                Stat->SectorRefresh_Waiting++;
              }

            }


            // Rendering first pass
            if (   Sector->Flag_IsVisibleAtLastRendering
                && (!Sector->Flag_Void_Regular)
                && (Sector->DisplayData != 0)
                && (((ZRender_Interface_displaydata *)Sector->DisplayData)->DisplayList_Regular != 0)
                )
              {

                #if COMPILEOPTION_FINETIMINGTRACKING == 1
                Timer_SectorRefresh.Start();
                #endif

                glCallList( ((ZRender_Interface_displaydata *)Sector->DisplayData)->DisplayList_Regular );
                Stat->SectorRender_Count++;RenderedSectors++;

                #if COMPILEOPTION_FINETIMINGTRACKING == 1
                Timer_SectorRefresh.End(); Time = Timer_SectorRefresh.GetResult(); Stat->SectorRender_TotalTime += Time;
                #endif
              }
          }
          else
          {
            if (GameEnv->Enable_LoadNewSector) World->RequestSector(x,y,z,Priority + PriorityBoost );
          }

    }

  // Second pass rendering


    glDepthMask(GL_FALSE);

    SectorsToProcess = SectorSphere.GetEntryCount();

    for (ULong Entry=0; Entry<SectorsToProcess; Entry++ )
    {
      SectorSphereEntry = SectorSphere.GetEntry(Entry);

      x = SectorSphereEntry->x + Sector_x;
      y = SectorSphereEntry->y + Sector_y;
      z = SectorSphereEntry->z + Sector_z;

       Sector = World->FindSector(x,y,z);
          // printf("Sector : %ld %ld %ld %lu\n", x, y, z, (ULong)(Sector != 0));9
          if (Sector)
          {
            if (  Sector->Flag_IsVisibleAtLastRendering
               && (!Sector->Flag_Void_Transparent)
               && (Sector->DisplayData != 0)
               && (((ZRender_Interface_displaydata *)Sector->DisplayData)->DisplayList_Transparent != 0)
               )
            {
              #if COMPILEOPTION_FINETIMINGTRACKING == 1
                Timer_SectorRefresh.Start();
              #endif

              glCallList( ((ZRender_Interface_displaydata *)Sector->DisplayData)->DisplayList_Transparent );
              Stat->SectorRender_Count++;

              #if COMPILEOPTION_FINETIMINGTRACKING == 1
                Timer_SectorRefresh.End(); Time = Timer_SectorRefresh.GetResult(); Stat->SectorRender_TotalTime += Time;
              #endif
            }

          }
    }
    glDepthMask(GL_TRUE);


    // ***************************
    // Cube designation
    // ***************************

    ZRayCast_in In;

    In.Camera = Camera;
    In.MaxCubeIterations = 150;
    In.PlaneCubeDiff = 100;
    In.MaxDetectionDistance = 30000.0;

 //   if (World->RayCast( &In, PointedVoxel ))
 //   {
      // Render_VoxelSelector( &PointedVoxel->PointedVoxel, 1.0,1.0,1.0 );
      //Render_VoxelSelector( &PointedVoxel->PredPointedVoxel, 1.0, 0.0, 0.0);
//    }

    // Debug ****************************************************

    ZVector3d Norm, Tmp;
    Norm.x = 0; Norm.y = 0; Norm.z = -1;
	Camera->orientation.ApplyRotation( Tmp, Norm );
    // X axis rotation
    //Tmp.y = Norm.y * cos(-Camera->Pitch/57.295779513) - Norm.z * sin(-Camera->Pitch/57.295779513);
    //Tmp.z = Norm.y * sin(-Camera->Pitch/57.295779513) + Norm.z * cos(-Camera->Pitch/57.295779513);
    //Norm.y = Tmp.y; Norm.z = Tmp.z;
    // Y axis rotation
    //Tmp.x = Norm.z*sin(Camera->Yaw/57.295779513) + Norm.x * cos(Camera->Yaw/57.295779513);
    //Tmp.z = Norm.z*cos(Camera->Yaw/57.295779513) - Norm.x * sin(Camera->Yaw/57.295779513);
    //Norm.x = Tmp.x; Norm.z = Tmp.z;
    //Norm.y = Tmp.y;
    // printf("Norm(%lf %lf %lf)\n",Norm.x,Norm.y,Norm.z);

    In.MaxCubeIterations = 150;
    In.MaxDetectionDistance = 1536;//1000000.0;

    ZVector3d CamPoint(Camera->x(),Camera->y(),Camera->z());
    ZVector3d Zp;
    Zp = PointedVoxel->CollisionPoint; Zp.y = PointedVoxel->CollisionPoint.y + 100.0;

    if (World->RayCast_Vector(Camera->orientation, Tmp, &In, PointedVoxel))
    {
      if (PointedVoxel->CollisionDistance < In.MaxDetectionDistance)
      {
        PointedVoxel->Collided = true;
        if (BvProp_DisplayVoxelSelector) 
			Render_VoxelSelector( &PointedVoxel->PointedVoxel, 1.0,1.0,1.0 );
      }
	  else
	  {
		  ZVector3d a = Camera->orientation.origin() +
			  Camera->orientation.z_axis() * ( GlobalSettings.VoxelBlockSize * -(Actor->VoxelSelectDistance) );
		  //In.MaxCubeIterations = 6;

		  ZVoxelRef *v = World->GetVoxelRefPlayerCoord( a.x, a.y, a.z );
			//World->RayCast_Vector(Camera->orientation, Tmp, &In, PointedVoxel);
			PointedVoxel->PredPointedVoxel.x = v->x;
			PointedVoxel->PredPointedVoxel.y = v->y;
			PointedVoxel->PredPointedVoxel.z = v->z;
			delete v ;
		  PointedVoxel->Collided = true;
        if (BvProp_DisplayVoxelSelector) 
			Render_VoxelSelector( &PointedVoxel->PredPointedVoxel, 0.2,1.0,0.1 );
	  }
    }


    // ***************************
    // Réticule
    // ***************************



    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(0, 1440, 900.0 , 0.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (BvProp_CrossHairType==1 && BvProp_DisplayCrossHair)
    {
      glDisable(GL_TEXTURE_2D);
      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f-1.0f, 450.0f-20.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f-20.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f-10.0f , 0.0f);
        glVertex3f(720.0f-1.0f, 450.0f-10.0f , 0.0f);
      glEnd();

      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f-1.0f, 450.0f+10.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f+10.0f , 0.0f);
        glVertex3f(720.0f+1.0f, 450.0f+20.0f , 0.0f);
        glVertex3f(720.0f-1.0f, 450.0f+20.0f , 0.0f);
      glEnd();

      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f-20.0f, 450.0f-1.0f , 0.0f);
        glVertex3f(720.0f-10.0f, 450.0f-1.0f , 0.0f);
        glVertex3f(720.0f-10.0f, 450.0f+1.0f , 0.0f);
        glVertex3f(720.0f-20.0f, 450.0f+1.0f , 0.0f);
      glEnd();

      glBegin(GL_POLYGON);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(720.0f+20.0f, 450.0f+1.0f , 0.0f);
        glVertex3f(720.0f+10.0f, 450.0f+1.0f , 0.0f);
        glVertex3f(720.0f+10.0f, 450.0f-1.0f , 0.0f);
        glVertex3f(720.0f+20.0f, 450.0f-1.0f , 0.0f);
      glEnd();

      glColor3f(1.0,1.0,1.0);
      glEnable(GL_TEXTURE_2D);
    }

    // Voile coloré

    if (Camera->ColoredVision.Activate)
    {
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_DEPTH_TEST);
      glColor4f(Camera->ColoredVision.Red,Camera->ColoredVision.Green,Camera->ColoredVision.Blue,Camera->ColoredVision.Opacity);
      glBegin(GL_POLYGON);
        glVertex3f(0.0f   , 0.0f   , 0.0f);
        glVertex3f(1440.0f, 0.0f   , 0.0f);
        glVertex3f(1440.0f, 900.0f , 0.0f);
        glVertex3f(0.0f   , 900.0f , 0.0f);
      glEnd();
      glColor3f(1.0,1.0,1.0);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_DEPTH_TEST);
    }

/*
    if (Camera->ColoredVision.Activate)
    {
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_DEPTH_TEST);
      glColor4f(Camera->ColoredVision.Red,Camera->ColoredVision.Green,Camera->ColoredVision.Blue,Camera->ColoredVision.Opacity);
      glBegin(GL_POLYGON);
        glVertex3f(0.0f   , 0.0f   , 0.0f);
        glVertex3f(1440.0f, 0.0f   , 0.0f);
        glVertex3f(1440.0f, 900.0f , 0.0f);
        glVertex3f(0.0f   , 900.0f , 0.0f);
      glEnd();
      glColor3f(1.0,1.0,1.0);
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_DEPTH_TEST);
    }
*/



    Timer.End();

    /*printf("Frame Time : %lu Rend Sects: %lu Draw Faces :%lu Top:%lu Bot:%lu Le:%lu Ri:%lu Front:%lu Back:%lu\n",Timer.GetResult(), RenderedSectors, Stat_RenderDrawFaces, Stat_FaceTop, Stat_FaceBottom,
           Stat_FaceLeft,Stat_FaceRight,Stat_FaceFront,Stat_FaceBack);*/

    //printf("RenderedSectors : %lu\n",RenderedSectors);
    //SDL_GL_SwapBuffers( );
}


void ZRender_Basic::MakeSectorRenderingData(ZVoxelSector * Sector)
{
  Long x,y,z;
  ULong info;
  UShort cube, prevcube;

  ULong Offset;
  float cubx, cuby, cubz;
  Long Sector_Display_x, Sector_Display_y, Sector_Display_z;
  ZRender_Interface_displaydata * DisplayData;
  ULong Pass;
  Bool Draw;
  ZVoxelType ** VoxelTypeTable = VoxelTypeManager->VoxelTable;
  ZVector3f P0,P1,P2,P3,P4,P5,P6,P7;


  // Display list creation or reuse.

  if (Sector->DisplayData == 0) { Sector->DisplayData = new ZRender_Interface_displaydata; }
  DisplayData = (ZRender_Interface_displaydata *)Sector->DisplayData;
  if ( DisplayData->DisplayList_Regular == 0 )    DisplayData->DisplayList_Regular = glGenLists(1);
  if ( DisplayData->DisplayList_Transparent == 0) DisplayData->DisplayList_Transparent = glGenLists(1);

  if (Sector->Flag_Render_Dirty || 1 )
  {
    Sector_Display_x = Sector->Pos_x * Sector->Size_x * GlobalSettings.VoxelBlockSize;
    Sector_Display_y = Sector->Pos_y * Sector->Size_y * GlobalSettings.VoxelBlockSize;
    Sector_Display_z = Sector->Pos_z * Sector->Size_z * GlobalSettings.VoxelBlockSize;

    Sector->Flag_Void_Regular = true;
    Sector->Flag_Void_Transparent = true;

    for (Pass=0; Pass<2; Pass++)
    {
      switch(Pass)
      {
        case 0: glNewList(DisplayData->DisplayList_Regular, GL_COMPILE); break;
        case 1: glNewList(DisplayData->DisplayList_Transparent, GL_COMPILE); break;
      }
      prevcube = 0;
      for ( z=0 ; z < Sector->Size_z ; z++ )
      {
        for ( x=0 ; x < Sector->Size_x ; x++ )
        {
          for ( y=0 ; y < Sector->Size_y ; y++ )
          {
            Offset = y + ( x*Sector->Size_y )+ (z * (Sector->Size_y*Sector->Size_x));
            cube = Sector->Data[Offset];
            info = ((UByte*)Sector->Culling)[Offset];


            if (cube>0 && info != DRAWFACE_NONE)
            {
              switch(Pass)
              {
                case 0: if ( VoxelTypeTable[cube]->Draw_TransparentRendering ) { Draw = false;  Sector->Flag_Void_Transparent = false; }
                        else                                                   { Draw = true;   Sector->Flag_Void_Regular     = false; }
                        break;
                case 1: Draw = ( VoxelTypeTable[cube]->Draw_TransparentRendering ) ? true:false; break;
              }
            } else Draw = false;

            if (Draw)
            {
              // glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,VoxelTypeManager->VoxelTable[cube]->TextureLodBias);
              if (cube != prevcube) glBindTexture(GL_TEXTURE_2D, VoxelTypeManager->VoxelTable[cube]->OpenGl_TextureRef);
              prevcube = cube;
              cubx = (float)(x*GlobalSettings.VoxelBlockSize + Sector_Display_x);
              cuby = (float)(y*GlobalSettings.VoxelBlockSize + Sector_Display_y);
              cubz = (float)(z*GlobalSettings.VoxelBlockSize + Sector_Display_z);

              if (VoxelTypeTable[cube]->DrawInfo & ZVOXEL_DRAWINFO_SPECIALRENDERING ) {VoxelTypeTable[cube]->SpecialRender(cubx,cuby,cubz); continue; }

              P0.x = cubx;           P0.y = cuby;          P0.z = cubz;
              P1.x = cubx + GlobalSettings.VoxelBlockSize;  P1.y = cuby;          P1.z = cubz;
              P2.x = cubx + GlobalSettings.VoxelBlockSize;  P2.y = cuby;          P2.z = cubz+GlobalSettings.VoxelBlockSize;
              P3.x = cubx;           P3.y = cuby;          P3.z = cubz+GlobalSettings.VoxelBlockSize;
              P4.x = cubx;           P4.y = cuby + GlobalSettings.VoxelBlockSize; P4.z = cubz;
              P5.x = cubx + GlobalSettings.VoxelBlockSize;  P5.y = cuby + GlobalSettings.VoxelBlockSize; P5.z = cubz;
              P6.x = cubx + GlobalSettings.VoxelBlockSize;  P6.y = cuby + GlobalSettings.VoxelBlockSize; P6.z = cubz + GlobalSettings.VoxelBlockSize;
              P7.x = cubx;           P7.y = cuby + GlobalSettings.VoxelBlockSize; P7.z = cubz + GlobalSettings.VoxelBlockSize;

              //Left
              if (info & DRAWFACE_LEFT)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceLeft++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
                glEnd();
              }

              // Right
              if (info & DRAWFACE_RIGHT)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceRight++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
                glEnd();
              }

              //Front
              if (info & DRAWFACE_AHEAD)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceFront++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
                  glTexCoord2f(0.25,0.25);  glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
                  glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
                glEnd();
              }

              //Back
              if (info & DRAWFACE_BEHIND)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceBack++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
                  glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                  glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
                glEnd();
              }

              // Top
              if (info & DRAWFACE_ABOVE)
              {
                Stat_RenderDrawFaces++;
                Stat_FaceTop++;
                glBegin(GL_TRIANGLES);
                  glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
                  glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
                  glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
                  glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
                glEnd();
              }

             // Bottom
             if (info & DRAWFACE_BELOW)
             {
               Stat_RenderDrawFaces++;
               Stat_FaceBottom++;
               glBegin(GL_TRIANGLES);
                 glTexCoord2f(1.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
                 glTexCoord2f(1.0,0.50);  glVertex3f(P1.x, P1.y, P1.z );
                 glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                 glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
                 glTexCoord2f(1.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
                 glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
               glEnd();
              }
            }


          }
        }
      }
      glEndList();

      // if in the first pass, the sector has no transparent block, the second pass is cancelled.

      if (Sector->Flag_Void_Transparent) break;
    }
    Sector->Flag_Render_Dirty = false;
  }

}























void ZRender_Basic::MakeSectorRenderingData_Sorted(ZVoxelSector * Sector)
{
  Long x,y,z;
  ULong info, i;
  UShort VoxelType, prevVoxelType;

  // ULong Offset;
  float cubx, cuby, cubz;
  Long Sector_Display_x, Sector_Display_y, Sector_Display_z;
  ZRender_Interface_displaydata * DisplayData;
  ULong Pass;
  ZVoxelType ** VoxelTypeTable = VoxelTypeManager->VoxelTable;
  ZVector3f P0,P1,P2,P3,P4,P5,P6,P7;

  ZRender_Sorter::RenderBucket * RenderBucket;

  // Set flags

  Sector->Flag_Void_Regular = true;
  Sector->Flag_Void_Transparent = true;
  Sector->Flag_Render_Dirty = false;

  // Render sorter action

  RenderSorter.ProcessSector(Sector);
  if (!RenderSorter.GetBucketCount()) return;

  // Check what blocktypes

  RenderSorter.Rendering_Start();
  for (i=0;i<RenderSorter.GetBucketCount();i++)
  {
    VoxelType = RenderSorter.Rendering_GetNewBucket()->VoxelType;

    if (VoxelTypeTable[VoxelType]->Draw_TransparentRendering) Sector->Flag_Void_Transparent = false;
    else                                                      Sector->Flag_Void_Regular     = false;
  }

  // Display list creation or reuse.

  if (Sector->DisplayData == 0) { Sector->DisplayData = new ZRender_Interface_displaydata; }
  DisplayData = (ZRender_Interface_displaydata *)Sector->DisplayData;
  if ( (!Sector->Flag_Void_Regular)     && (DisplayData->DisplayList_Regular == 0) )    DisplayData->DisplayList_Regular = glGenLists(1);
  if ( (!Sector->Flag_Void_Transparent) && (DisplayData->DisplayList_Transparent == 0) ) DisplayData->DisplayList_Transparent = glGenLists(1);

  // Computing Sector Display coordinates;

  Sector_Display_x = (Sector->Pos_x * Sector->Size_x) << GlobalSettings.VoxelBlockSizeBits;
  Sector_Display_y = (Sector->Pos_y * Sector->Size_y) << GlobalSettings.VoxelBlockSizeBits;
  Sector_Display_z = (Sector->Pos_z * Sector->Size_z) << GlobalSettings.VoxelBlockSizeBits;

  for (Pass=0; Pass<2; Pass++)
  {
    if (!Pass) { if (Sector->Flag_Void_Regular)     continue; glNewList(DisplayData->DisplayList_Regular, GL_COMPILE); }
    else       { if (Sector->Flag_Void_Transparent) continue; glNewList(DisplayData->DisplayList_Transparent, GL_COMPILE); }

    prevVoxelType = 0;

    RenderSorter.Rendering_Start();

    while (( RenderBucket = RenderSorter.Rendering_GetNewBucket() ))
    {
      VoxelType = RenderBucket->VoxelType;

      // Is it the right voxel transparency type for that rendering pass ?

      if ( (Pass>0) != VoxelTypeTable[VoxelType]->Draw_TransparentRendering) continue;

      // Render one RenderBucket.

      for (i=0;i<RenderBucket->VoxelCount;i++)
      {
		  register uint64_t Pck;

        // Gettint Voxel Informations from the table.

        Pck = RenderBucket->RenderTable[i].PackedInfos;

        // Unpacking voxel infos

          info = Pck & ( ( 1 << (64 - ( ZVOXELBLOCSHIFT_X+ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_Z ) ) ) - 1 );
        z    = (Pck >> (64-(ZVOXELBLOCSHIFT_X+ZVOXELBLOCSHIFT_Y+ZVOXELBLOCSHIFT_Z))) & ZVOXELBLOCMASK_Z;
        y    = (Pck >> (64-(ZVOXELBLOCSHIFT_X+ZVOXELBLOCSHIFT_Y))) & ZVOXELBLOCMASK_Y;
        x    = (Pck >> (64-(ZVOXELBLOCSHIFT_X))) & ZVOXELBLOCMASK_X;
		//info = Pck & 0xFF;
        //z    = Pck >> 8 & 0xFF;
        //y    = Pck >> 16 & 0xFF;
        //x    = Pck >> 24 & 0xFF;

        // Offset = y + ( x << ZVOXELBLOCSHIFT_Y )+ (z << (ZVOXELBLOCSHIFT_Y + ZVOXELBLOCSHIFT_X));

        // glTexEnvf(0x8500 /* TEXTURE_FILTER_CONTROL_EXT */, 0x8501 /* TEXTURE_LOD_BIAS_EXT */,VoxelTypeManager->VoxelTable[VoxelType]->TextureLodBias);
        if (VoxelType != prevVoxelType) glBindTexture(GL_TEXTURE_2D, VoxelTypeManager->VoxelTable[VoxelType]->OpenGl_TextureRef);
        prevVoxelType = VoxelType;
        cubx = (float)(x*GlobalSettings.VoxelBlockSize + Sector_Display_x);
        cuby = (float)(y*GlobalSettings.VoxelBlockSize + Sector_Display_y);
        cubz = (float)(z*GlobalSettings.VoxelBlockSize + Sector_Display_z);

        if (VoxelTypeTable[VoxelType]->DrawInfo & ZVOXEL_DRAWINFO_SPECIALRENDERING ) {VoxelTypeTable[VoxelType]->SpecialRender(cubx,cuby,cubz); continue; }

        P0.x = cubx;           P0.y = cuby;          P0.z = cubz;
        P1.x = cubx + GlobalSettings.VoxelBlockSize;  P1.y = cuby;          P1.z = cubz;
        P2.x = cubx + GlobalSettings.VoxelBlockSize;  P2.y = cuby;          P2.z = cubz+GlobalSettings.VoxelBlockSize;
        P3.x = cubx;           P3.y = cuby;          P3.z = cubz+GlobalSettings.VoxelBlockSize;
        P4.x = cubx;           P4.y = cuby + GlobalSettings.VoxelBlockSize; P4.z = cubz;
        P5.x = cubx + GlobalSettings.VoxelBlockSize;  P5.y = cuby + GlobalSettings.VoxelBlockSize; P5.z = cubz;
        P6.x = cubx + GlobalSettings.VoxelBlockSize;  P6.y = cuby + GlobalSettings.VoxelBlockSize; P6.z = cubz + GlobalSettings.VoxelBlockSize;
        P7.x = cubx;           P7.y = cuby + GlobalSettings.VoxelBlockSize; P7.z = cubz + GlobalSettings.VoxelBlockSize;

        //Left
        if (info & DRAWFACE_LEFT)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceLeft++;
          glBegin(GL_TRIANGLES);
            glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
            glTexCoord2f(0.25,0.0);  glVertex3f(P0.x, P0.y, P0.z );
            glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
            glTexCoord2f(0.50,0.0);  glVertex3f(P3.x, P3.y, P3.z );
            glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
            glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
          glEnd();
        }

        // Right
        if (info & DRAWFACE_RIGHT)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceRight++;
          glBegin(GL_TRIANGLES);
            glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
            glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
            glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
            glTexCoord2f(0.50,0.75);  glVertex3f(P2.x, P2.y, P2.z );
            glTexCoord2f(0.25,0.75); glVertex3f(P1.x, P1.y, P1.z );
            glTexCoord2f(0.25,0.50); glVertex3f(P5.x, P5.y, P5.z );
          glEnd();
        }

        //Front
        if (info & DRAWFACE_AHEAD)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceFront++;
          glBegin(GL_TRIANGLES);
            glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
            glTexCoord2f(0.25,0.25);  glVertex3f(P4.x, P4.y, P4.z );
            glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            glTexCoord2f(0.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
            glTexCoord2f(0.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
          glEnd();
        }

        //Back
        if (info & DRAWFACE_BEHIND)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceBack++;
          glBegin(GL_TRIANGLES);
            glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
            glTexCoord2f(0.50,0.50);  glVertex3f(P6.x, P6.y, P6.z );
            glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
            glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
            glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
            glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
          glEnd();
        }

        // Top
        if (info & DRAWFACE_ABOVE)
        {
          Stat_RenderDrawFaces++;
          Stat_FaceTop++;
          glBegin(GL_TRIANGLES);
            glTexCoord2f(0.25,0.25); glVertex3f(P4.x, P4.y, P4.z );
            glTexCoord2f(0.50,0.25);  glVertex3f(P7.x, P7.y, P7.z );
            glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            glTexCoord2f(0.25,0.50);  glVertex3f(P5.x, P5.y, P5.z );
            glTexCoord2f(0.50,0.25); glVertex3f(P7.x, P7.y, P7.z );
            glTexCoord2f(0.50,0.50); glVertex3f(P6.x, P6.y, P6.z );
          glEnd();
        }

       // Bottom
       if (info & DRAWFACE_BELOW)
       {
         Stat_RenderDrawFaces++;
         Stat_FaceBottom++;
         glBegin(GL_TRIANGLES);
           glTexCoord2f(1.0,0.25); glVertex3f(P0.x, P0.y, P0.z );
           glTexCoord2f(1.0,0.50);  glVertex3f(P1.x, P1.y, P1.z );
           glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
           glTexCoord2f(0.75,0.25);  glVertex3f(P3.x, P3.y, P3.z );
           glTexCoord2f(1.0,0.50); glVertex3f(P1.x, P1.y, P1.z );
           glTexCoord2f(0.75,0.50); glVertex3f(P2.x, P2.y, P2.z );
         glEnd();
        }


      }
    }
    glEndList();
  }
}

#if 0
void ZRender_Basic::ComputeAndSetAspectRatio(double VerticalFOV, double PixelAspectRatio, ZVector2L & ViewportResolution)
{
  double FocusDistance = 50.0;
  VerticalFOV = 63.597825649;
  PixelAspectRatio = 1.0;

  double Frustum_V;
  double Frustum_H;
  double Aspect_Ratio;

  Aspect_Ratio = (ViewportResolution.x / ViewportResolution.y) * PixelAspectRatio;

  // FOV Limitation to safe values.

  if (VerticalFOV < 5.0 ) VerticalFOV = 5.0;
  if (VerticalFOV > 160.0 ) VerticalFOV = 160.0;

  Frustum_V = tan(VerticalFOV / 2.0) * FocusDistance;
  Frustum_H = Frustum_V * Aspect_Ratio;

  glFrustum(Frustum_H, -Frustum_H, -Frustum_V, Frustum_V, FocusDistance, 1000000.0); // Official Way

}
#endif

