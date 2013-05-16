
#include <world.h>



typedef struct client_global_tag {
	struct {
		_32 connected : 1;
	} flags;
	_32 MsgBase;
	// pointer by index number...
	//PLIST worlds;
} CLIENT_GLOBAL;

typedef struct client_tag {
	DeclareLink( struct client_tag );
	struct {
		// if the appropraite create flag is set
		// said world does not get the create event
		_32 bCreateWorld : 1;
		_32 bCreateName : 1;
		_32 bCreateSector : 1;
		_32 bCreateTexture : 1;
		_32 bCreateLine : 1;
		_32 bCreateWall : 1;
	} flags;
	_32 pid;
} WORLD_CLIENT, *PWORLD_CLIENT;

typedef struct server_global_tag
{
	_32 MsgBase; // my message base?
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
