
#define MAKE_RCOORD_SINGLE
#include <render.h>
#include <vectlib.h>
#include <controls.h>
#include <image3d.h>
#ifdef USE_GLES2
#include <GLES2/gl2.h>
#endif
// bullet instance is defined in l....
#include <btBulletDynamicsCommon.h>


#define CheckErr()  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
						lprintf( "err=%d",err ); \
				}                               \

struct BulletInfo
{
	btBroadphaseInterface* broadphase;

	btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;

	btSequentialImpulseConstraintSolver* solver;

	btDiscreteDynamicsWorld* dynamicsWorld;


	btRigidBody* fallRigidBody;  // for test routine 
	btRigidBody* groundRigidBody; // the ground plane at 0.
};



#define l local_terrain_data


#ifndef TERRAIN_MAIN_SOURCE
extern
#endif
struct local_terrain_data_tag {
	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;
	PIMAGE_3D_INTERFACE pi3di;
	RCOORD *identity_depth;
	RCOORD *aspect ;
	Image logo;
	TEXTCHAR logo_name[256];
	Image player_image;
	Image wild_image;
	int logo_texture;
	struct {
		TEXTCHAR image_name[256];
		Image image;
		TEXTCHAR bump_image_name[256];
		Image bump_image;
		int max_size; // largest resolution this was allocated for.
		int rows;
		int cols;
		VECTOR ****coords;  // 4 dimensional arrays of rows each cols long
		int texture; // opengl texture handle (returned from ReloadOpenGLTexture)
	} numbers;
	struct local_flags {
		BIT_FIELD rack_balls : 1;
		BIT_FIELD nextball_mode : 1;
	} flags;

	struct {
		Image image;
      int texture;
	} ball_color;

	Image hotball_image[75];


	PSI_CONTROL frame;

	CDATA colors[5];

	float values[40];
	PSI_CONTROL sliders[40];

	_32 last_tick;
	PLIST patches;
	int hex_size; // how big initial patch size is.

	struct BulletInfo bullet;
	 btRigidBody* barRigidBody; 


	 PTRANSFORM transform;  // current draw transform
	
	 int next_active_ball;
	 int locking_position; // locks ball orientation, moves look-at for this ball.
	 int active_ball;
	 

	 RCOORD next_active_distance;
	 _32 watch_ball_tick; 
	 _32 next_active_tick;
	 _32 active_ball_forward_tick; 
	 _32 return_to_home;
    btVector3 ball_grab_position;

	 RCOORD initial_view_quat[4];
	 _POINT initial_view_origin;
	 RCOORD final_view_quat[4];
	 _POINT final_view_origin;

	 _32 show_ball_time; // how long to show the front of the ball... (if not nextball mode, othrwise call event clears)
	 _32 show_back_time; // how long to show the back of the ball if we're in next ball show?
	 _32 fade_duration; // how long before ball fades to nothing...

	 _32 demo_tick_delay;
	 _32 demo_time_wait_after_drop;
	 _32 demo_time_wait_turn;
	 _32 demo_time_wait_front;
	_32 demo_time_to_pick_ball;
	
	_32 time_to_rack;
	 _32 time_to_home;
	 _32 time_to_track;
	 _32 time_to_approach;
	 _32 time_to_turn_ball;

	int nNextBalls[75];
	int last_set; // last position set from ball queue...

	// added info to support shader
	struct gl_shader_data {
		struct {
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;

		struct {
			PImageShaderTracker shader_tracker;
			GLint projection;
			GLint worldview;
			GLint modelview;
			GLint eye_point;

			GLint shadowMapUniform;// = glGetUniformLocationARB(shaderId,"ShadowMap");
			GLint shadowMapBackUniform;// = glGetUniformLocationARB(shaderId,"BackShadowMap");

			GLint diffuseTextureUniform;// = glGetUniformLocationARB(shaderId,"diffuseTexture");
			GLint specularTextureUniform;// = glGetUniformLocationARB(shaderId,"specularTexture");
			GLint normalTextureUniform;// = glGetUniformLocationARB(shaderId,"normalTexture");
			GLint tangentLoc;  
			GLint backgroundLoc;
			GLuint shader; // shader program ID
			GLint frag_shader;// saved parts for later destruction?
			GLint vert_shader;// saved parts for later destruction? 
			GLint invRadiusUniform;
			GLint textureUniform;

			struct {
				GLint position;
				GLint ambient;
				GLint specular;
				GLint shine;
				GLint diffuse;
				GLint direction;
			} light[2];
			GLint global_ambient;
			struct {
				GLint specular;
				GLint diffuse;
				GLint ambient;
				GLint shine;
			} material;
		} normal_shader;
		struct {
			PImageShaderTracker shader_tracker;
			GLint projection;
			GLint worldview;
			GLint modelview;
			GLint eye_point;

			GLuint shader; // shader program ID
			GLint frag_shader;// saved parts for later destruction?
			GLint vert_shader;// saved parts for later destruction? 
			struct {
				GLint position;
				GLint ambient;
				GLint specular;
				GLint shine;
				GLint diffuse;
				GLint direction;
			} light[2];
			GLint global_ambient;
			struct {
				GLint specular;
				GLint diffuse;
				GLint ambient;
				GLint shine;
			} material;
		} simple_shader;
		struct {
			PImageShaderTracker shader_tracker;
			GLint projection;
			GLint worldview;
			GLint modelview;
			GLint eye_point;

			GLuint shader; // shader program ID
			GLint frag_shader;// saved parts for later destruction?
			GLint vert_shader;// saved parts for later destruction? 
			GLint global_ambient;
		} extra_simple_shader;

	} shader;

	GLfloat *projection;
	//float projection[16]; // retreived from opengl state 
	float worldview[16]; // obtained from camera translation
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

void FadeBall( int ball );

void		BeginPivot(void);
void		EndWatchBall(void);

// remote_hook function; checks ball states for overlays.
void DrawOverlay( PTRANSFORM camera );

// ------ Shader.cpp ----------------

struct SACK_3D_Surface {
		
	//float3 min;
	//float3 max;
	//float3 normal;
	//md5material* material;
	LOGICAL color; // if true, then use fore color

	int densityx;
	int densityy;
	//SurfaceType type;
	GLuint VAOobject;

	GLuint VBOvertices;
	GLuint VBOnormal;
	GLuint VBOcolor;
	//GLuint VBOTexture;
	// support multiple textures on the same fragment.
	PDATALIST pdl_VBOTexture;
	// support multiple textures on the same fragment.
	PDATALIST pdl_VBONormalTexture;
	GLuint VBOTangent;
	GLuint VBOelements;
	PCVECTOR *vertices;
	float cooTexture[8];
	float normals[12];
	float tangents[12];
	GLuint *elements;
	int verts;
} ;

void InitShader(PImageShaderTracker shader);
void InitSuperSimpleShader( PImageShaderTracker shader );


struct SACK_3D_Surface *CreateBumpTextureFragment( int verts
									, PCVECTOR *shape_array
									, PCVECTOR *normal_array
									, PCVECTOR *tangent_array
									, PCVECTOR *colors
									, PCVECTOR *texture_coord_array
									);
void RenderBumpTextureFragment( Image texture
								, Image bump
								, btScalar *m
								, INDEX texture_id
								, float *background
								, struct SACK_3D_Surface *surface
				);
INDEX AddBumpTextureFragmentTexture( struct SACK_3D_Surface *surface
								, INDEX texture_id
													, PCVECTOR *texture_coord_array
													);

void SetMaterial( void );
void SetLights( void );

