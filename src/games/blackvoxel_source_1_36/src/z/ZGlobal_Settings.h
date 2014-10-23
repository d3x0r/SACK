

#ifndef Z_ZGLOBAL_SETTINGS_H
#define Z_ZGLOBAL_SETTINGS_H

class ZGlobalSettings
{
public:
	ZGlobalSettings();

	double VoxelBlockSize;
	int VoxelBlockSizeBits;
};

#  ifndef GLOBAL_SETTING_SOURCE
extern class ZGlobalSettings GlobalSettings;
#  endif

#endif
