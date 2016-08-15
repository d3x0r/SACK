
#include <world.h>



typedef struct client_global_tag {
	struct {
		uint32_t connected : 1;
	} flags;
	uint32_t MsgBase;
	// pointer by index number...
	//PLIST worlds;
} CLIENT_GLOBAL;

typedef struct client_tag {
	DeclareLink( struct client_tag );
	struct {
		// if the appropraite create flag is set
		// said world does not get the create event
		uint32_t bCreateWorld : 1;
		uint32_t bCreateName : 1;
		uint32_t bCreateSector : 1;
		uint32_t bCreateTexture : 1;
		uint32_t bCreateLine : 1;
		uint32_t bCreateWall : 1;
	} flags;
	uint32_t pid;
} WORLD_CLIENT, *PWORLD_CLIENT;

typedef struct server_global_tag
{
	uint32_t MsgBase; // my message base?
	PWORLD_CLIENT clients;
	PWORLDSET worlds;
} SERVER_GLOBAL;

typedef struct library_global_tag
{
	PWORLDSET worlds;
} LIBRARY_GLOBAL;

#if defined( WORLD_CLIENT_LIBRARY )
#define GLOBAL CLIENT_GLOBAL
#elif defined( WORLD_SERVICE )
#define GLOBAL SERVER_GLOBAL
#else
#define GLOBAL LIBRARY_GLOBAL
#endif

//extern GLOBAL g;
