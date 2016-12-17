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
	
	PDATALIST msg_parse;
	CTEXTSTR output;


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
		json_parse_message( output, StrLen( output ), &msg_parse );
#define TEST_MESSAGE "{\"MsgID\":\"123\",\"Msg\":{\"From\":\"127.0.0.1\"}}"
		json_parse_message( TEST_MESSAGE
							, sizeof( TEST_MESSAGE ) - 1
							, &msg_parse );
	}


	return 0;
}

