
#include <msgserver.h>


// a table of SERVER_FUNCTION type function points
// needs to be created.  (if prototypes are supplied ahead
// of this, then this may be the first part of this file)
//
// the wrapper ServerFunctionEntry, takes care of creating
// both fields in this table, which are the text name of the function
// (for external diagnostics) and the function pointer...
// perhaps further information oneday.
//
// These functions MUST match an enumeration starting at 0
// the first 4 of these are reserved, and must be
//  MSG_MateEnded       0
//  MSG_DispatchPending 1
//  MSG_ServiceClose    2
//  MSG_....            3
// (MSG_EventUser...) 4
//


enum {
	MSG_FirstProc = MSG_EventUser
	  , MSG_SecondProc
     , MSG_ThirdProc
};


// this define
#define NUM_FUNCTIONS (sizeof(MyMessageHandlerTable)/sizeof( server_function))
static SERVER_FUNCTION MyMessageHandlerTable[] = {
#ifdef GCC
	// oh my, we just LOVE C99 array member initialization
   // someday the rest of the world will be as kewl as GNU.
	[MSG_MateEnded] =
#endif
		// when MSG_MateEnded is received by the server
      // this will be invoked...
		ServerFunctionEntry( ClientClosed ),
		{0}, // NULL ... this entry is not used.
#ifdef GCC
		[MSG_ServiceClose] =
#endif
		ServerFunctionEntry( ServerUnload ),
		// these NULLS would be better skipped with C99 array init - please do look
		// that up and implement here.
		{0},
#ifdef GCC
		[MSG_EventUser] =
#endif
      // first message...
		ServerFunctionEntry( FirstServerMessage )

		, ServerFunctionEntry( SecondServerFunction )
		, ServerFunctionEntry( ThirdServerFunction ) // co-responds to MSG_ThirdProc...


};

// this is a server function which does nothing
// responds with 0 bytes of data (if any)
// and does not respond with any data (FALSE)
static int DoNothing( uint32_t *params, uint32_t param_length
						  , uint32_t *result, uint32_t *result_length)
{
	*result_length = 0;
   return FALSE;
}

static int CPROC GetDisplayFunctionTable( server_function_table *table, int *entries, uint32_t MsgBase )
{
	// this is the function invoked by the service loader
	// to query how many entries this server supports (so
	// it can compute the NEXT message base to give the client)
	// it results with its function table
	// Saves the message base which the service loader
   // has assigned this service.
	*table = MyMessageHandlerTable;
	*entries = NUM_FUNCTIONS;
	// good as a on_entry procedure.
	g.MsgBase = MsgBase;
	Log( WIDE("Resulting load to server...") );
	// any custom service initialization for on_load
	// should be done here, and the result
   // returned TRUE for good init, or FALSE for bad init.
   return InitDisplay();
}

PRELOAD( RegisterService )
{
	// Register the service in the system interfaces...
	RegisterFunction( WIDE("system/interfaces/msg_service"), GetDisplayFunctionTable
						 , WIDE("int"), WIDE("display"), WIDE("(server_function_table*,int*,uint32_t)") );
}


