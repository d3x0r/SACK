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
 * ZVoxelExtension.cpp
 *
 *  Created on: 11 nov. 2014
 *      Author: laurent
 */

#include "ZVoxelExtension.h"


bool ZVoxelExtension::_ThrowExtension(ZStream_SpecialRamStream * Stream, ZMemSize ExtensionSize)
{
  bool Ok;
  UByte Temp_Byte;

  Ok = true;
  ExtensionSize-=2;
  for(ZMemSize i=0;i<ExtensionSize;i++) Ok = Ok && Stream->Get(Temp_Byte);

  return(Ok);
}


ULong ZVoxelExtension::ExtensionCharCodes[] = {	
	0
	, MulticharConst('S','T','O','R')
	,  MulticharConst('U','T','T','R')
	, MulticharConst('P','L','Z','1')	
	, MulticharConst('F','M','C','H')
	, MulticharConst('P','R','O','G')
	, MulticharConst('F','U','S','E') 
	, MulticharConst('B','F','U','R') // 7 blst furnace
	, MulticharConst('M','R','X','1')  //8
	, MulticharConst('S','E','Q','U')
	, MulticharConst('E','M','Y','1') // 10 egmy
	, MulticharConst('B','F','G','R') // fertile ground
	, MulticharConst('F','O','O','D')
	, MulticharConst('B','A','N','I')  // animal
	, MulticharConst('A','R','M','A')  // aroma
	, MulticharConst('A','R','M','G')   // aroma generator
	//, // count unused
	};

