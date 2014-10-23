
#define GLOBAL_SETTING_SOURCE

#include "ZGlobal_Settings.h"


ZGlobalSettings GlobalSettings;

ZGlobalSettings::ZGlobalSettings()
{
	VoxelBlockSizeBits = 6;
	VoxelBlockSize = 1 << VoxelBlockSizeBits;
}
