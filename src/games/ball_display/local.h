


#define l local_terrain_data

#ifndef TERRAIN_MAIN_SOURCE
extern
#endif

struct local_terrain_data {
	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;

	struct {
		Image image;
		int rows;
		int cols;
		VECTOR **coords;  // 2 dimensional arrays of rows each cols long
		int texture; // opengl texture handle (returned from ReloadOpenGLTexture)
	} numbers;

	struct {
		Image image;
      int texture;
	} ball_color;
	PSI_CONTROL frame;

   CDATA colors[5];

	float values[40];
   PSI_CONTROL sliders[40];
} l;

enum color_defs{
	MAT_AMBIENT0,
	MAT_AMBIENT1,
	MAT_AMBIENT2,
	MAT_DIFFUSE0,
	MAT_DIFFUSE1,
	MAT_DIFFUSE2,
	MAT_SPECULAR0,
	MAT_SPECULAR1,
	MAT_SPECULAR2,
	MAT_SHININESS,

	LIT_AMBIENT0,
	LIT_AMBIENT1,
	LIT_AMBIENT2,
	LIT_DIFFUSE0,
	LIT_DIFFUSE1,
	LIT_DIFFUSE2,
	LIT_SPECULAR0,
	LIT_SPECULAR1,
	LIT_SPECULAR2,

};
