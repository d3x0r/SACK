#include <stdhdrs.h>
#include <vectlib.h>
#include <json_emitter.h>

struct part1 {
	RCOORD x;
};

struct message1 {
   S_64 integer;
   RCOORD v[3];
   CTEXTSTR string;
   struct msg_sub1 {
	   int a;
	   int b;
   }sub1;
   struct part1 *sub2;
};

int main( void )
{
	struct json_context *context = json_create_context();
	struct json_context_object *object1 = json_create_object( context );
	struct json_context_object *object1_sub1;
	struct json_context_object *object1_sub2;
	struct message1 msg1;
	CTEXTSTR output;

	json_add_object_member( object1, "intval", offsetof( struct message1, integer), JSON_Element_Integer_64 );
	json_add_object_member_array( object1, "v1", offsetof( struct message1, v), JSON_Element_Integer_64, 3 );
	json_add_object_member( object1, "string", offsetof( struct message1, string), JSON_Element_String );

	object1_sub1 = json_add_object_member( object1, "sub1", offsetof( struct message1, sub1), JSON_Element_Object );
	json_add_object_member( object1_sub1, "a", offsetof( struct msg_sub1, a), JSON_Element_Integer_32 );
	json_add_object_member( object1_sub1, "b", offsetof( struct msg_sub1, b), JSON_Element_Integer_32 );

	object1_sub1 = json_add_object_member( object1, "sub2", offsetof( struct message1, sub2), JSON_Element_ObjectPointer );
	json_add_object_member( object1_sub2, "a", offsetof( struct part1, x), JSON_Element_Float );


	msg1.integer=123456789123456789;//0x123456789abcdef0;
	msg1.v[0] = 15.3;
	msg1.v[1] = 18.333;
	msg1.v[2] = -2218.333;

	output = json_build_message( object1, &msg1 );
	printf( "result:\n" );
	printf( "%s\n", output );
	MemSet( &msg1, 0, sizeof( msg1 ) );
	json_parse_message( object1, output, &msg1 );


	return 0;
}

