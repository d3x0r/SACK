
#define USE_IMAGE_3D_INTERFACE GlobalSettings.pi3d
#include <image3d.h>

#ifndef Z_ZGLOBAL_SETTINGS_H
#define Z_ZGLOBAL_SETTINGS_H

class ZGlobalSettings
{
public:
	ZGlobalSettings();
	PIMAGE_3D_INTERFACE pi3d;
	double VoxelBlockSize;
	int VoxelBlockSizeBits;
};

#  ifndef GLOBAL_SETTING_SOURCE
extern class ZGlobalSettings GlobalSettings;
#  endif

#endif
