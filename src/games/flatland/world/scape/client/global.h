
#define BASE_MESSAGE_ID g.MsgBase

#include <world.h>

struct client_global_UpdateCallback_tag {
	void (CPROC *proc)( PTRSZVAL );
	PTRSZVAL psv;
};
typedef struct client_global_tag {
	struct {
		BIT_FIELD connected : 1;
		BIT_FIELD changes_done : 1;
		BIT_FIELD accepting_changes : 1;
	} flags;
	PSERVICE_ROUTE MsgBase;
	// pointer by index number...
	PWORLDSET worlds;
	PLIST callbacks;
	PLINKQUEUE pending_changes;
} CLIENT_GLOBAL;

#define g global_worldscape_client
#define GLOBAL CLIENT_GLOBAL

#define MyInterface world_scape_interface
int CPROC ConnectToServer( void );

//extern GLOBAL g;
