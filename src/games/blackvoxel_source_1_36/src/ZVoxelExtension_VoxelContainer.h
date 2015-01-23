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
 * ZVoxelExtension_VoxelContainer.h
 *
 *  Created on: 10 nov 2014
 *      Author: d3x0r
 */

#ifndef Z_ZVOXELEXTENSION_VOXELCONTAINER_H
#define Z_ZVOXELEXTENSION_VOXELCONTAINER_H

#ifndef Z_ZVOXELEXTENSION_H
#  include "ZVoxelExtension.h"
#endif

class ZVoxelExtension_VoxelContainer : public ZVoxelExtension
{
	class ZFreeVoxel
	{
		UShort VoxelType;
		ZMemSize  OtherInfos;
	};
	class ZVoxelPool : public ZBasicMemoryPool
	{
#define MAXZFreeVoxelSPERSET 256
		DeclareSet( ZFreeVoxel, VoxelPool );
		ZFreeVoxel *GetFreeVoxel( UShort voxelType )
		{
			ZFreeVoxel *result = GetFromSet( ZFreeVoxel, &VoxelPool ); 
		}
		void DropFreeVoxel( ZFreeVoxel *voxel )
		{
			ZFreeVoxel *result = DeleteFromSet( ZFreeVoxel, &VoxelPool ); 
		}
	};
  public:
    ZFreeVoxel *voxels;
  public:

  ZVoxelExtension_VoxelContainer()
  {
  }

  virtual ZVoxelExtension * GetNewCopy()
  {
    ZVoxelExtension_VoxelContainer * NewCopy;
    NewCopy = new ZVoxelExtension_VoxelContainer(*this);
    return(NewCopy);
  }

  virtual ULong GetExtensionID()
  {
    return( MulticharConst('B','C','O','N') );
  }

  virtual bool Save(ZStream_SpecialRamStream * Stream)
  {
	  base->Save( Stream );
	  /*
    ULong * ExtensionSize;
    ULong   StartLen;

    ExtensionSize = Stream->GetPointer_ULong();
    Stream->Put(0u);       // The size of the extension (defered storage).
    StartLen = Stream->GetActualBufferLen();
    Stream->Put((UShort)1); // Extension Version;

    // Storage informations.

    Stream->Put(Quantity_Carbon);

    *ExtensionSize = Stream->GetActualBufferLen() - StartLen;
	*/
    return(true);
  }

  virtual bool Load(ZStream_SpecialRamStream * Stream)
  {
	  /*
    bool Ok;
    ULong  ExtensionSize;
    UShort ExtensionVersion;
    UByte  Temp_Byte;

    Ok = Stream->Get(ExtensionSize);
    Ok&= Stream->Get(ExtensionVersion);     if(!Ok) return(false);

    // Check for supported extension version. If unsupported new version, throw content and continue with a blank extension.

    if (ExtensionVersion!=1) { ExtensionSize-=2; for(ZMemSize i=0;i<ExtensionSize;i++) Ok = Stream->Get(Temp_Byte); if (Ok) return(true); else return(false);}

    Stream->Get(Quantity_Carbon);

    return(Ok);
	*/
	  return true;
  }

};

#endif /* Z_ZVOXELEXTENSION_VOXELCONTAINER_H */
