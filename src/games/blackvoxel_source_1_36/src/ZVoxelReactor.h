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
#ifndef Z_ZVOXELREACTOR_H
#define Z_ZVOXELREACTOR_H

//#ifndef Z_ZVOXELREACTOR_H
//#  include "ZVoxelReactor.h"
//#endif

#ifndef Z_ZTYPES_H
#  include "z/ZTypes.h"
#endif

#ifndef Z_ZVOXELSECTOR_H
#  include "ZVoxelSector.h"
#endif

#ifndef Z_ZVOXELTYPE_H
#  include "ZVoxelType.h"
#endif

#ifndef Z_ZWORLD_H
#  include "ZWorld.h"
#endif

#ifndef Z_ZVOXELTYPEMANAGER_H
#  include "ZVoxelTypeManager.h"
#endif

#ifndef Z_ZFASTBIT_ARRAY_H
#  include "z/ZFastBit_Array.h"
#endif

#ifndef Z_ZFASTRANDOM_H
#  include "z/ZFastRandom.h"
#endif

#ifndef Z_ZVOXELREACTION_H
#  include "ZVoxelReaction.h"
#endif

#ifndef Z_VOXELEXTENSION_FABMACHINE_H
#  include "ZVoxelExtension_FabMachine.h"
#endif

#ifndef Z_ZFABMACHINEINFOS_H
#  include "ZFabMachineInfos.h"
#endif

#ifndef Z_ZVOXELEXTENSION_PROGRAMMABLE_H
#  include "ZVoxelExtension_Programmable.h"
#endif

#ifndef Z_ZEGMYTARGETMANAGER_H
#  include "ZEgmyTargetManager.h"
#endif

#include <salty_generator.h>

class ZGame;

struct ZonePressure;

class ZVoxelReactor
{
  public:
    static ZLightSpeedRandom Random;
	static random_context *ZVoxelReactor::Random2;

    static ZVoxelSector * DummySector;
  protected:
    ZVector3d PlayerPosition;
    ZGame * GameEnv;
    ZVoxelWorld * World;
    ZVoxelTypeManager * VoxelTypeManager;
    ZEgmyTargetManager    EgmyWaveManager;
    ULong               CycleNum;


    ZVoxelReaction ** ReactionTable;

  public:
    class ZBlocPos { public: UByte x; UByte y; UByte z; };
    class ZBlocPosN{ public: Byte x;  Byte y;  Byte z; };
	static ZBlocPos bfta[26];  // bloc flow table (air)
	static ZBlocPos bfts[18];  // bloc flow table (smoothing)
    static ZBlocPos  bp6[6];   // Bloc positions with 6 slots around main cube.
    static ZBlocPos  bft[8];   // Bloc fall test positions with 4 slots around and 4 slots under;
    static ZBlocPos  bft6[10]; // Bloc fall test positions with 6 slots around main cube and 4 slots under (Special case for acid).
    static UByte     BlocOpposite[6];
    static ZBlocPosN nbp6[6];
    static ZBlocPos  xbp6[6];  // Bloc positions with 6 slots around main cube. ( New standardised robot order.).
    static ZBlocPos  xbp6_opposing[6];  // Bloc positions with 6 slots around main cube. ( New standardised robot order.).
    static RelativeVoxelOrds  x6_opposing_escape[6][5];  // Bloc positions with 6 slots around main cube. ( New standardised robot order.).
	static ZBlocPosN xbp6_opposing_escape[6][5]	;
    static ZBlocPosN xbp6_nc[6];// same as xbp6 with -1,+1 range

  public:

    // Fast computing offsets;
    static UByte Of_x[ZVOXELBLOCSIZE_X+2];
    static UByte Of_y[ZVOXELBLOCSIZE_Y+2];
    static UByte Of_z[ZVOXELBLOCSIZE_Z+2];
    static ULong If_x[ZVOXELBLOCSIZE_X+2];
    static ULong If_y[ZVOXELBLOCSIZE_Y+2];
    static ULong If_z[ZVOXELBLOCSIZE_Z+2];

    // DirCodes

    static UByte DirCodeTable[16];

  protected:

    ULong * ActiveTable;


// Time remaining on FireMine action

    ULong FireMineTime;


  public:
    void Init(ZGame * GameEnv);


    ZVoxelReactor();
    ~ZVoxelReactor();


    void UpdatePlayerPosition(ZVector3d * PlayerPosition)
    {
      this->PlayerPosition = *PlayerPosition;
    }



    void LightTransmitter_FindEndPoints(ZVector3L * Location, ZVector3L * NewCommingDirection);
    void LightTransmitter_FollowTransmitter(ZVector3L * Location, ZVector3L * FollowingDirection);
    bool VoxelFluid_ComputeVolumePressure(ZVector3L * Location, UShort VoxelType, bool EvenCycle);
    void VoxelFluid_ComputeVolumePressure_Recurse(ZVector3L * Location, ZonePressure * Pr  );
    void VoxelFluid_SetVolumePressure_Recurse(ZVector3L * Location, ZonePressure * Pr  );
    void ProcessSectors( double LastLoopTime );

#define OffsetDelta(x,y,z)  ( ((x)*ZVOXELBLOCSIZE_Y) + (y) + ((z)*ZVOXELBLOCSIZE_Y*ZVOXELBLOCSIZE_X) )


	static void GetVoxelRefs( const ZVoxelRef &self, ZVoxelSector **ResultSectors, ULong *ResultOffsets )
	{
		ResultSectors[VOXEL_INCENTER] = self.Sector;
		register ULong origin = ( self.x << ZVOXELBLOCSHIFT_Y ) + self.y + ( self.z << ( ZVOXELBLOCSHIFT_X + ZVOXELBLOCSHIFT_Y ) );
		{
			register ULong *in = ZVoxelSector::RelativeVoxelOffsets_Unwrapped;
			register ULong *out =ResultOffsets;
			int n;
			for( n= 0; n < 27; n++ )
				(*out++) = origin + (*in++);
			if( self.x == 0 ) 
			{
				ResultSectors[VOXEL_LEFT] = self.Sector->near_sectors[VOXEL_LEFT-1]; 	ResultSectors[VOXEL_RIGHT] = self.Sector;
				for( n = 0; n < 9; n++ ) 
					ResultOffsets[ZVoxelSector::VoxelFaceGroups[VOXEL_LEFT-1][n]] += ( ZVOXELBLOCSIZE_X ) * ZVOXELBLOCSIZE_Y;
			}
			else if( self.x == ( ZVOXELBLOCSIZE_X - 1 ) ) 
			{
				ResultSectors[VOXEL_LEFT] = self.Sector;	ResultSectors[VOXEL_RIGHT] = self.Sector->near_sectors[VOXEL_RIGHT-1];
				for( n = 0; n < 9; n++ ) ResultOffsets[ZVoxelSector::VoxelFaceGroups[VOXEL_RIGHT-1][n]] -= ( ZVOXELBLOCSIZE_X ) * ZVOXELBLOCSIZE_Y;
			}
			else
			{
				ResultSectors[VOXEL_LEFT] = self.Sector;			ResultSectors[VOXEL_RIGHT] = self.Sector;
			}
			if( self.y == 0 ) 
			{
				ResultSectors[VOXEL_ABOVE] = self.Sector;				ResultSectors[VOXEL_BELOW] = self.Sector->near_sectors[VOXEL_BELOW-1];
				for( n = 0; n < 9; n++ ) ResultOffsets[ZVoxelSector::VoxelFaceGroups[VOXEL_BELOW-1][n]] += ( ZVOXELBLOCSIZE_Y );
			}
			else if( self.y == ( ZVOXELBLOCSIZE_Y - 1 ) ) 
			{
				ResultSectors[VOXEL_ABOVE] = self.Sector->near_sectors[VOXEL_ABOVE-1];				ResultSectors[VOXEL_BELOW] = self.Sector;
				for( n = 0; n < 9; n++ ) ResultOffsets[ZVoxelSector::VoxelFaceGroups[VOXEL_ABOVE-1][n]] -= ( ZVOXELBLOCSIZE_Y );
			}
			else
			{
				ResultSectors[VOXEL_ABOVE] = self.Sector;				ResultSectors[VOXEL_BELOW] = self.Sector;
			}

			if( self.z == 0 ) 
			{
				ResultSectors[VOXEL_AHEAD] = self.Sector;				ResultSectors[VOXEL_BEHIND] =  self.Sector->near_sectors[VOXEL_BEHIND-1];
				for( n = 0; n < 9; n++ ) ResultOffsets[ZVoxelSector::VoxelFaceGroups[VOXEL_BEHIND-1][n]] += ( ZVOXELBLOCSIZE_Z ) * ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X;
			}
			else if( self.z == ( ZVOXELBLOCSIZE_Z - 1 ) ) 
			{
				ResultSectors[VOXEL_AHEAD] = self.Sector->near_sectors[VOXEL_AHEAD-1];				ResultSectors[VOXEL_BEHIND] = self.Sector;
				for( n = 0; n < 9; n++ ) ResultOffsets[ZVoxelSector::VoxelFaceGroups[VOXEL_AHEAD-1][n]] -= ( ZVOXELBLOCSIZE_Z ) * ZVOXELBLOCSIZE_Y * ZVOXELBLOCSIZE_X;
			}
			else
			{
				ResultSectors[VOXEL_AHEAD] = self.Sector;				ResultSectors[VOXEL_BEHIND] = self.Sector;
			}
			
			// test to make sure resulting offsets are within range.
			//for( n = 0; n < 27; n++ ) if( ResultOffsets[n] & 0xFFFF8000 ) DebugBreak();
		}
		if( self.x == 0 )
		{
			if( self.y == 0 )
			{
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_BELOW]->near_sectors[VOXEL_LEFT-1];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_BELOW];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_BELOW-1];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE_BEHIND]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] =  ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_BELOW-1];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_AHEAD];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW_AHEAD]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
			}
			else if( self.y == (ZVOXELBLOCSIZE_Y-1) )
			{
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_ABOVE]->near_sectors[VOXEL_LEFT-1];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_BELOW];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND_LEFT]->near_sectors[VOXEL_ABOVE-1];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_ABOVE-1];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD_LEFT]->near_sectors[VOXEL_ABOVE-1];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else //----------------------------------------------
				{
					// left bound, top bound, front nobound
					ResultSectors[VOXEL_AHEAD] = self.Sector;
					ResultSectors[VOXEL_BEHIND] = self.Sector;

					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
			}
			else //----------------------------------------------
			{
				// left bound, above/below unbound
				ResultSectors[VOXEL_ABOVE] = self.Sector;
				ResultSectors[VOXEL_BELOW] = self.Sector;
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_RIGHT];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_RIGHT];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_LEFT-1];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else
				{
					// left bound, y unbound z unbound
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
			}
		}
		else if( self.x == (ZVOXELBLOCSIZE_X-1) )
		{
			if( self.y == 0 )
			{
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_BELOW];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_BELOW];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_RIGHT]->near_sectors[VOXEL_BELOW-1];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_RIGHT-1];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW]->near_sectors[VOXEL_BEHIND-1];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW_BEHIND]->near_sectors[VOXEL_RIGHT-1];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_RIGHT-1];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_BELOW-1];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_AHEAD];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW_AHEAD]->near_sectors[VOXEL_RIGHT-1];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
			}
			else if( self.y == (ZVOXELBLOCSIZE_Y-1) )
			{
				ResultSectors[VOXEL_ABOVE] = self.Sector->near_sectors[VOXEL_ABOVE-1];
				ResultSectors[VOXEL_BELOW] = self.Sector;
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_BELOW];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_ABOVE]->near_sectors[VOXEL_RIGHT-1];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_RIGHT];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_RIGHT-1];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE_BEHIND]->near_sectors[VOXEL_RIGHT-1];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_RIGHT-1];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_ABOVE-1];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE_AHEAD];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_AHEAD];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE_AHEAD]->near_sectors[VOXEL_RIGHT-1];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
			}
			else
			{
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_RIGHT];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_RIGHT];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND]->near_sectors[VOXEL_RIGHT-1];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD]->near_sectors[VOXEL_RIGHT-1];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
				else
				{
					ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_RIGHT];
					ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_LEFT];
					ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_RIGHT];

					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

					ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_RIGHT_BELOW];
					ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_LEFT_ABOVE];
					ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_LEFT_BELOW];
					ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_RIGHT_ABOVE];
					ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_RIGHT_BELOW];
				}
			}
		}
		else //---------------------------------------------------------
		{
			// left/right unbound... left and right should never be terms of equality
			if( self.y == 0 )
			{
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_BELOW];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_BELOW];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW]->near_sectors[VOXEL_BEHIND-1];
				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
					ResultSectors[VOXEL_BELOW_AHEAD] =  ResultSectors[VOXEL_BELOW]->near_sectors[VOXEL_AHEAD-1];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];
				}
				else
				{
					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];
				}
				ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

				ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE_AHEAD];
				ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW_AHEAD];
				ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE_AHEAD];
				ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW_AHEAD];
				ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE_BEHIND];
				ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW_BEHIND];
				ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE_BEHIND];
				ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW_BEHIND];
			}
			else if( self.y == (ZVOXELBLOCSIZE_Y-1) )
			{
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_BELOW];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_ABOVE];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_BELOW];
				if( self.z == 0 )
				{
					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] =  ResultSectors[VOXEL_ABOVE]->near_sectors[VOXEL_BEHIND-1];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];

				}
				else if( self.z == (ZVOXELBLOCSIZE_Z-1) )
				{
					ResultSectors[VOXEL_ABOVE_AHEAD] =   ResultSectors[VOXEL_ABOVE]->near_sectors[VOXEL_AHEAD-1];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];
				}
				else
				{
					ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE];
					ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW];
					ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW];
				}
				ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

				ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE_AHEAD];
				ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW_AHEAD];
				ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_ABOVE_AHEAD];
				ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_BELOW_AHEAD];
				ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE_BEHIND];
				ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW_BEHIND];
				ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_ABOVE_BEHIND];
				ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_BELOW_BEHIND];
			}
			else  //----------------------------------------------
			{
				// x not on bound, y not on bound.
				ResultSectors[VOXEL_LEFT_ABOVE] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_LEFT_BELOW] = ResultSectors[VOXEL_LEFT];
				ResultSectors[VOXEL_RIGHT_ABOVE] = ResultSectors[VOXEL_RIGHT];
				ResultSectors[VOXEL_RIGHT_BELOW] = ResultSectors[VOXEL_RIGHT];

				ResultSectors[VOXEL_AHEAD_LEFT] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_AHEAD_RIGHT] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_BEHIND_LEFT] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_BEHIND_RIGHT] = ResultSectors[VOXEL_BEHIND];

				ResultSectors[VOXEL_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];

				ResultSectors[VOXEL_LEFT_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_LEFT_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_RIGHT_ABOVE_AHEAD] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_RIGHT_BELOW_AHEAD] = ResultSectors[VOXEL_AHEAD];
				ResultSectors[VOXEL_LEFT_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_LEFT_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_RIGHT_ABOVE_BEHIND] = ResultSectors[VOXEL_BEHIND];
				ResultSectors[VOXEL_RIGHT_BELOW_BEHIND] = ResultSectors[VOXEL_BEHIND];
			}
		}
	}

};


#endif /* Z_ZVOXELREACTOR_H */
