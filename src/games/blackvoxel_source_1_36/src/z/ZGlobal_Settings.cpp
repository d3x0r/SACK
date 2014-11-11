
#define GLOBAL_SETTING_SOURCE

#include "ZGlobal_Settings.h"


ZGlobalSettings GlobalSettings;

ZGlobalSettings::ZGlobalSettings()
{
	VoxelBlockSizeBits = 4;
	VoxelBlockSize = 1 << VoxelBlockSizeBits;
}
