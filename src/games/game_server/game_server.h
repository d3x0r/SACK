#include <stdhdrs.h>
#include <html5.websocket.h>
#include <json_emitter.h>

struct game_client { // data attached to network
	POINTER buffer;
	size_t buf_size;
	size_t read_size;

};

struct game_entry
{
   CTEXTSTR name;
   CTEXTSTR server_address;
};

struct game_group_entry
{
   CTEXTSTR name;
	PLIST games; // list of struct game_entry *

};

struct game_message {
	int32_t ID;
   CTEXTSTR extra;
};

#ifndef GAME_SERVER_MAIN
extern
#endif
       struct game_server_global
{
	PLIST game_groups;
	PCLIENT server;
	struct json_context *context;
   struct json_context_object *base_message;
} gsg;
