#include <stdhdrs.h>

#include <json_emitter.h>

struct message1 {
   S_64 integer;
};

int main( void )
{
	struct json_context *context = json_create_context();
	struct json_context_object *object1 = json_create_object( context );

	struct message1 msg1;
   CTEXTSTR output;

	json_add_object_member( object1, "intval", offsetof( struct message1, integer), JSON_Element_Integer_64 );
	msg1.integer=123456789123456789;//0x123456789abcdef0;

	output = json_build_message( object1, &msg1 );
	printf( "result:\n" );
   printf( "%s\n", output );
   json_parse_message( object1, output, &msg1 );


   return 0;
}

