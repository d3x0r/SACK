
#include <image3d.h>
#define GLOBAL_SETTING_SOURCE

#include "ZGlobal_Settings.h"


ZGlobalSettings GlobalSettings;

ZGlobalSettings::ZGlobalSettings()
{
	VoxelBlockSizeBits = 7;
	VoxelBlockSize = 1 << VoxelBlockSizeBits;
	pi3d = GetImage3dInterface();

}
