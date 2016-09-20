#include <stdhdrs.h>
#include <vectlib.h>
#include <json_emitter.h>

struct part1 {
	RCOORD x;
};

struct message1 {
   int64_t integer;
   RCOORD v[3];
   CTEXTSTR string;
   struct msg_sub1 {
	   int a;
	   int b;
   }sub1;
	struct part1 *sub2;
   PLIST object_names;
};

struct message2 {
	int32_t MsgID;
	CTEXTSTR Msg;
};

int main( void )
{
	struct json_context *context = json_create_context();
	struct json_context_object *object1 = json_create_object( context, sizeof( struct message1 ) );
	struct json_context_object *object1_sub1;
	struct json_context_object *object1_sub2;
	struct json_context_object *object2 = json_create_object( context, sizeof( struct message2 ) );
	struct message1 *msg1 = New( struct message1 );
	struct message2 *msg2 = NULL;
	struct part1 msg_sub2;
	
	CTEXTSTR output;

	json_add_object_member( object1, WIDE("intval"), offsetof( struct message1, integer), JSON_Element_Integer_64, 0 );
	json_add_object_member_array( object1, WIDE("v1"), offsetof( struct message1, v), JSON_Element_Double, 0, 3, JSON_NO_OFFSET );
	json_add_object_member( object1, WIDE("string"), offsetof( struct message1, string), JSON_Element_String, 0 );

	object1_sub1 = json_add_object_member( object1, WIDE("sub1"), offsetof( struct message1, sub1), JSON_Element_Object, 0 );
	json_add_object_member( object1_sub1, WIDE("a"), offsetof( struct msg_sub1, a), JSON_Element_Integer_32, 0 );
	json_add_object_member( object1_sub1, WIDE("b"), offsetof( struct msg_sub1, b), JSON_Element_Integer_32, 0 );

	object1_sub2 = json_add_object_member( object1, WIDE("sub2"), offsetof( struct message1, sub2), JSON_Element_ObjectPointer, sizeof( struct part1 ) );
	json_add_object_member( object1_sub2, WIDE("x"), offsetof( struct part1, x), JSON_Element_Double, 0 );
	//json_add_object_member_list( object1, WIDE("object_names"), offsetof( struct message1, object_names), JSON_Element_String, 0 );
	//json_add_object_member_object( object1_sub2, WIDE("a"), offsetof( struct part1, object_names), JSON_ElementObjectPointer, object1_sub1 );

	json_add_object_member( object2, WIDE("MsgID"), offsetof( struct message2, MsgID), JSON_Element_Integer_32, 0 );
	json_add_object_member( object2, WIDE("Msg"), offsetof( struct message2, Msg), JSON_Element_Raw_Object, 0 );


	msg1->integer=123456789123456789;//0x123456789abcdef0;
	msg1->string = "String value";
	msg1->v[0] = 15.3;
	msg1->v[1] = 18.333;
	msg1->v[2] = -2218.333;
	msg1->sub2 = &msg_sub2;
	msg_sub2.x = 5214.2342;
	//msg1->sub2.x = 432.21;
	msg1->object_names = NULL;

	output = json_build_message( object1, msg1 );
	printf( WIDE("result:\n") );
	printf( WIDE("%s\n"), output );
	MemSet( &msg1, 0, sizeof( msg1 ) );
	{
		struct json_context_object *format;
		json_parse_message( context, output, StrLen( output ), &format, &msg1 );
#define TEST_MESSAGE "{\"MsgID\":\"123\",\"Msg\":{\"From\":\"127.0.0.1\"}}"
		json_parse_message( context, TEST_MESSAGE
							, sizeof( TEST_MESSAGE ) - 1
							, &format, &msg2 );
	}


	return 0;
}

