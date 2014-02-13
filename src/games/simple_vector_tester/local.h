

#define l local_terrain_data

#ifndef TERRAIN_MAIN_SOURCE
extern
#endif

struct local_terrain_data_tag {

	 PTRANSFORM transform;  // current draw transform
	
	// added info to support shader
	struct gl_shader_data {
struct {
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;

		struct {
			GLuint shader; // shader program ID
			GLint frag_shader;// saved parts for later destruction?
			GLint vert_shader;// saved parts for later destruction? 
		} simple_shader;
	} shader;

	float modelview[16];

	float projection[16];
} l;

