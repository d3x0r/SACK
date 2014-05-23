

#define l local_terrain_data

#ifndef TERRAIN_MAIN_SOURCE
extern
#endif

struct local_terrain_data_tag {

	PIMAGE_3D_INTERFACE pi3i;
	PRENDER3D_INTERFACE pr3i;
	 PTRANSFORM transform;  // current draw transform
	
	PImageShaderTracker shader;

	float modelview[16];

	float projection[16];
} l;

