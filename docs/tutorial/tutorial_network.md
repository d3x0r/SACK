
# Event based Networking

The model of networking SACK presents is threaded, interrupt driven.  Code registerd for network
events will run on a background thread, asyncrhounous to the main thread; this may lead to the 
consideration of locks on common data between sockets.   Using the `PLINKQUEUE` type provides for locking
by default.

``` c
#include <stdhdrs.h>
#include <network.h>
```

Networking requires an additional include.

## Start the Network Library

Various systems have various methods of providing information to the program about the data transmitted
on the network.  A few of the socket-open methods will trigger `NetworkStart()` if it hasn't already been
enabled, but this usually should be enabled, and the network library initialized before using it.

``` c
NetworkStart();
```

## Get a Network Address

In order to connect or serve, you will need a network address.  This is a complex, variable structure depending
on the character of the socket.

``` c
PSOCKADDR connectTo = CreateSockAddress( name, default_port );
```

The CreateSockAddress takes a text address as the `name`, and a number as the `default_port`.

If the name is formatted like a IP number, it is converted to an IP.  IPv6 addresses with brackets, or bare
may be used.  The Address is interpreted to see if it ends with a ':' followed by a port number (which is why
IPv6 addresses should be enclodes in `[]`, as in `[::0]:123`.  Any names are looked up using the system 
name resolution services, and IPv6 addresses are preferred to connect to over any IPv6 replies.


The text name passed is kept with the PSOCKADDR structure, and can be recovered; such that if you only kept the
address, you get not only the binary internals, but the original text;   If there was no string, and only 
the numeric information is kept, this string serves as a cache of the to-string value of the numeric form of the
message.

### Listening...

Modern network sockets should listen on `::0` which is a wild card for 'any address', including IPv4 addresses
which will be received and appear as a IPv6 address, but truly be a IPv4 connection.

Localhost is `::1`.

So this is an example of to get an address which can be used to listen on any address.

``` c
PSOCKADDR listenAt = CreateSockAddress( "::0", listen_port );
```


## Start a TCP socket

The network interface has gone through many iterations; and the final is implemented as macros.

``` c
PCLIENT pServer = OpenTCPListenerAddrEx( listenAt, TCPConnect );
```

The first parameter is a `SOCKADDR*` and the second parameter is a callback which is invoked with the new socket 
when a connection is received.

The events for this new client will have to be associated with the client;
         these can be set on the server socket, and by default will be copied to the client.
         There will never be a 'close' event on the listening socket.

``` c
static void TCPConnect( PCLIENT pcServer, PCLIENT pcClient ) {
	// pcServer is the same as pServer returned above.
        // pcClient is the new socket.
        
	SetNetworkReadComplete( pc, ReadComplete );
	SetNetworkCloseCallback( pc, Closed );   
}
```


## Client Events

### Read Completion

The first time ReadComplete is called, it is passed a `NULL` buffer and `0` length.  This allows
the read handler to allocate a buffer, and queue that buffer for the next read.  The read will never
be larger than the size of the buffer specified.

Otherwise, after the first call, when data is available from the network, the packet of data that is present will be passed
to the callback.

``` c
static void ReadComplete( PCLIENT pc, void *bufptr, int sz ) {
	if( !bufptr ) {
        	bufptr = malloc(4096);
                // use a user data work to keep the buffer pointer...
                SetNetworkLong( pc, 0, (uintptr_t)bufptr );
        } else {
        	// data of 'sz' is present.
        }
	return ReadTCP( pc, bufptr, 4096 );
}
```

### Closed

This will be called whether you close the socket on the application side, or if the socket is closed on the other side.
If you did NOT close the socket, the the close came from the other side, and you may still be able to send data.

``` c
static void Closed( PCLIENT pc ) {
	void* bufptr = (void*)GetNetworkLong( pc, 0 );
        free( bufptr );
}

```

## Send some Data

And of course you'll probably want to send data on a socket....

``` c
SendTCP( pc, (void*)data, (size_t)length );
```

There is no return code, close notification will be dispatched to the provided callback, and any other 
abort sort of condition is a close.  

This will queue the data to the network socket.  It will attempt to immediately send the data, but any 
portion which isn't accepted by the socket is copied to an internal buffer, until the buffer is completed.

You are free to dispose of the buffer immediately after calling `SendTCP()`.

### Sending LONG Data

The default internal backing copy is reasonable for typically small traffic, but obviously if there are
buffers of several MB, it might not be good to copy it.

``` c
SendTCPLong( pc, (void*)data, (size_t)length );
```

This will keep the original data pointer passed, and provide a `WriteComplete()` notification when the buffer
is done.  (Advanced usges below).




## Close a socket

When you want to end a conversation on a socket, you can call `RemoveClient()`, passing the `PCLIENT` object pointer.

``` c
RemoveClient( pc );
```

This does a graceful shutdown, and you may still receive data after closing the socket, until the other side has closed their side.

(There is a way to terminate a connection)


# UDP Sockets

UDP Sockets behave slightly differently than TCP sockets; other than receiving blocks of data at a time,
the source address of the UDP packet is also part of the data passed to the read callback.

UDP also does not require the user to allocate a buffer, it is known when the read event happens how much
data is avaialble, so the netwowkr allocates a buffer to receive the UDP data.

The 'buffer' passed to the read complete event should not be used after returning from the read complete handler.

Otherwise, the Close, and write complete event handling is exactly the same for UDP.

## Opening

``` c
PCLIENT pcUdp = ServeUDP( address, port, UDP_ReadComplete, UDP_Close) 
```


### Read Complete


``` c
static void UDP_ReadComplete( PCLIENT pc, CPOINTER buffer, size_t length, SOCKADDR* from ) {
	// got a packet.
}

```

### Close Event


``` c c
static void UDP_Close( PCLIENT pc ) {
	// not sure why you need this event, you know you just closed it, right?
        // nothing else causes this socket to close.
}
```


## SendTo

And, the address per-message may be specified for UDP messages...
probably using the same address that wa passed to the read complete callback which triggers
this response.


``` c
    SendUDPEx( pc, buffer, size, (SOCKADDR*)address );
```


## Connected UDP

You can open a UDP connection, which species both ends of the conversation,
or can later 'connect' the socket to a specific address, and use `SendUDP()` to send
data without re-specifying the target address.

``` c c
	PCLIENT pcConnectedUDP = ConnectUDPAddr( listenAt, sendTo, UDP_ReadComplete, UDP_Close );
```


### Directed Send

You will only need the socket, buffer and size to send to send on a directed socket.  You may still 
use `SendUDPEx()` with an address on such a socket.

``` c
    SendUDP( pc, buffer, size );
```

# WebSockets


Websockets provide connectivity with web browsers.  They are a wrapper over TCP sockets, which 
provides similar events, but may also fallback to HTTP requests, which may be served.  Websockets
re-add datagram functionality to TCP, that a message of N bytes sent will be received as N bytes.

``` c
#include <html5.websocket.h>
```

## Open a Server

The parameter to the websocket create is a proper URL.  It should be prefixed with a protocol
and a `://` to determine a default port; although the port may also be specified.

Any path parts of the URL are ignored, and only the domain and port are used to determine where
to listed.   `"http://0.0.0.0"` would also be valid as a server address.

``` c
PCLIENT pcWebServer = WebSocketCreate( "http://example.com:8080"
                                     , WebSocketOpened
                                     , WebSocketMessage
                                     , WebSocketClose
                                     , WebSocketError
                                     , (uintptr_t)anyUserValue
                                     );
```

``` c
static uintptr_t WebSocketOpened( PCLIENT pcNew, uintptr_t userValue ) {
	// pcNew is the newly accepted websocket client.
        // userValue is the value specified in the WebSocketCreate.

	// this gets the protocols requested by the client.
	const char *protocols = WebSocketGetProtocols( pcNew ); 
        
        // this sets the reply with what protocols are accepted by the server.
        // this example just accepts what was requested.
        WebSocketSetProtocols( pcNew, protocols );
        
        // it is possible to inspect all other headers of the request...
        PLIST headers = GetWebSocketHeaders( pcNew );
        {
        	INDEX idx;
                struct HTTPRequestHeader *field;
                LIST_FORALL( headers, idx, struct HTTPRequestHeader*, field ) {
                	const char *name = field->field;
                	const char *value = field->value;
                }
        }
        
        // This gets the path part of the client's request
        PTEXT url = GetWebSocketResource( pcNew );
        
        // finally, this should return a new user value...
        // this value is used to associate resources with this new client.
        
	return (uintptr_t)newUserValue;
}
```

### Close Event

``` c
static void WebSocketClose( PCLIENT pc, uintptr_t userValue, int code, const char *reason ) {
	// this socket is closing.
        // ths userValue will be the 'newUserValue' returned from the open.
        // the code will be a websocket error code
        // and the reason is the websocket error code text (if any)
        
}
```


### Read Event

``` c
static void WebSocketMessage( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen ) {
	// a completed packet received...
        // psv userValue will be the same as the 'newUserValue' returned from the open.
        // binary indiciates whether the buffer was sent as UTF8 Text or as binary; this doesn't mean a LOT to C code.
        // buffer is the pointer to the buffer
        // and msglen is the size of the message.
        
        
        // do not use 'buffer' after returning from this funciton.
}

```

### Error Event

This almost never occurs.

``` c
staic void WebSocketError( PCLIENT pc, uintptr_t psv, int error ){
	// psv will be the same as newUserValue returned from Open.
	// error indicates the websocket error code.
}
```


  

## Open a Client

Include the websocket client API.  (implicitly included with the above server API.)

``` c
#include <html5.websocket.client.h>
```

   

``` c
PCLIENT pcWebClient = WebSocketOpen( "https://d3x0r.org"
                                   , 0 
                                   , WebSocketOpen
                                   , WebSocketMessage  // same as server
                                   , WebSocketClose    // same as server
                                   , WebSocketError    // same as server
                                   , userDataValue
                                   , "[Protocols,To,Requst]" );



```


``` c

static uintptr_t WebSocketOpen( PCLIENT pcNew, uintptr_t userValue ) {
	// pcNew is the newly accepted websocket client.
        // userValue is the value specified in the WebSocketCreate.
        
        // this is a 1-to-1 relation with the above open, so the same
        // userValue might be passed here; in the server case, the open
        // event is triggered multiple times, once for each client.
        return userValue;
}
```

This another complete, minimal client example: https://gist.github.com/d3x0r/37736b8f3468f573f1e24821b9b16d46


## Sending on a WebSocket


Content sent on a websocket should either be UTF8(Text) or binary.  There are some restrictions like the
bytes `0xF8-0xFF` can never appear in a UTF8 text stream; although a simple hack is to encode binary 
by-codepoint into utf8, it's better to use a Base64 conversion method..



### Text

Calling send will complete a packet, and finish the message.

``` c 
	WebSocketSendText( pc, "text", 4 );
```

### Binary

Calling send will complete a packet, and finish the message.

``` c
	char *data = malloc(12);
	WebSocketSendBinary( pc, data, 12 );
```        



### Text (Progressive)

Using `BeginSend` you can progressively build a packet to send, and flush it when completed.
The packets should be sent as specified, with the last packet including the completion event.
There's no overall message size of a websocket message, it's just the sum of its fragments.

``` c
WebSocketBeginSendText( pc, "Hello", 5 ); 
WebSocketBeginSendText( pc, "World", 5 ); 
WebSocketSendText( pc, ".", 1 ); 
```


### Binary (Progressive)

``` c
WebSocketBeginSendBinary( pc, "Hello", 5 ); 
WebSocketBeginSendBinary( pc, "World", 5 ); 
WebSocketSendBinary( pc, ".", 1 ); 
```



## Closing a WebSocket

When closing the socket, you must specify the websocket close code, and appropriate text to send.


``` c
	WebSocketClose( pc, 1000, "Disconnect OK" );
```



## Misc Other Websocket Features


### Client Open Options

The client open ablve (`WebSocketOpen()`) takes a option parameter, which is set as 0 in the example, but 
may also be `WS_DELAY_OPEN`.  This allows setting up other information about the connection before actually
doing the connect.

``` c
	WebSocketConnect( pcWebClient );
```


The progress of transfers may be tracked.  As fragments of messsages are received, this callback
is completed; for content like a large binary file, it might be useful to see how much of the packet 
has already been received.

``` c
	SetWebSocketDataCompletion( pc, CompletionCallback );
```


``` c

static void CompletionCallback( PCLIENT pc, uintptr_t psv, int binary, int bytesRead ){
	// the socket that got data
        // the psv returned from the open
        // a flag whether the transfer is binary or not
        // the total bytes read so far...
}

```




## (TODO)
 
These are things that ou can do, but usually don't have to specify...
 
``` c
WEBSOCKET_EXPORT void SetWebSocketAcceptCallback( PCLIENT pc, web_socket_accept callback );
WEBSOCKET_EXPORT void SetWebSocketReadCallback( PCLIENT pc, web_socket_event callback );
WEBSOCKET_EXPORT void SetWebSocketCloseCallback( PCLIENT pc, web_socket_closed callback );
WEBSOCKET_EXPORT void SetWebSocketErrorCallback( PCLIENT pc, web_socket_error callback );
WEBSOCKET_EXPORT void SetWebSocketHttpCallback( PCLIENT pc, web_socket_http_request callback );
WEBSOCKET_EXPORT void SetWebSocketHttpCloseCallback( PCLIENT pc, web_socket_http_close callback );

// if set in server accept callback, this will return without extension set
// on client socket (default), does not request permessage-deflate
#define WEBSOCK_DEFLATE_DISABLE 0
// if set in server accept callback (or if not set, default); accept client request to deflate per message
// if set on client socket, sends request for permessage-deflate to server.
#define WEBSOCK_DEFLATE_ENABLE 1
// if set in server accept callback; accept client request to deflate per message, but do not deflate outbound messages
// if set on client socket, sends request for permessage-deflate to server, but does not deflate outbound messages(?)
#define WEBSOCK_DEFLATE_ALLOW 2

// set permessage-deflate option for client requests.
// allow server side to disable this when responding to a client.
WEBSOCKET_EXPORT void SetWebSocketDeflate( PCLIENT pc, int enable_flags );

// default is client masks, server does not
// this can be used to disable masking on client or enable on server
// (masked output from server to client is not supported by browsers)
WEBSOCKET_EXPORT void SetWebSocketMasking( PCLIENT pc, int enable );

// Set callback to get completed fragment size (total packet size collected so far)
WEBSOCKET_EXPORT void SetWebSocketDataCompletion( PCLIENT pc, web_socket_completion callback );

```



# HTTP

Then; of course there's a HTTP parser, but mostly it's extra functions that can be used with a socket, 
on the sever side.

On the client/request side there are some short hand utility functions which are blocking and either return content or NULL.





---




# Stuff only some people use

## Custom Parameters

Set the size of network resources allocoted can be set using `NetworkWait()`.  `NetworkStart()` calls this
with default parameters.

``` c
NetworkWait( NULL, 128, 5 );
```

The first parameter is unused, and should be passed as NULL; it used to be a window handle for event nofications, 
but the windows message queue is fairly slow for network events.

The second parameter is the starting number of sockets to allocate.  The system can always expend to have more than this count, 
but this will be a basic amount allocated.  It also will control how much is added when new clients are required.

The third parameter is the number of extra 'pointer-sized' values that the socket has available for the user; 
`SetNetworkLong()` and `GetNetworkLong()` are limited by this count.  It can be later expanded with another `NetworkWait()` but cannot be decreased.

NOTE: The Websocket and HTTP socket handlers built into SACK use at least 2 network words for user data; overloading
a socket used for an internal protocol for your own purposes may have consequences.

## Stuff noone uses

### Shutdown

``` c
NetworkShutdown();
```


# All together now...


## Complete Example, from `include/network.h`

	   Example One : A simple client side application. Reads
	   standard input, and writes it to a server it connects to. Read
	   the network and write as standard output.

``` c
	   #include <network.h>
	   #include <logging.h>
	   #include <sharemem.h>

	   void CPROC ReadComplete( PCLIENT pc, void *bufptr, int sz )
	   {
	      char *buf = (char*)bufptr;
	       if( buf )
	       {
	           buf[sz] = 0;
	           printf( "%s", buf );
	           fflush( stdout );
	       }
	       else
	       {
	           buf = (char*)Allocate( 4097 );
	           //SendTCP( pc, "Yes, I've connected", 12 );
	       }
	       ReadTCP( pc, buf, 4096 );
	   }

	   PCLIENT pc_user;

	   void CPROC Closed( PCLIENT pc )
	   {
	      pc_user = NULL;
	   }

	   int main( int argc, char** argv )
	   {
	       SOCKADDR *sa;
	       if( argc < 2 )
	       {
	           printf( "usage: %s <Telnet IP[:port]>\\n", argv[0] );
	           return 0;
	       }
	       SystemLog( "Starting the network" );
	       NetworkStart();
	       SystemLog( "Started the network" );
	       sa = CreateSockAddress( argv[1], 23 );
	       pc_user = OpenTCPClientAddrEx( sa, ReadComplete, Closed, NULL, 0 );
	       if( !pc_user )
	       {
	           SystemLog( "Failed to open some port as telnet" );
	           printf( "failed to open %s%s\\n", argv[1], strchr(argv[1],':')?"":":telnet[23]" );
	           return 0;
	       }
	      //SendTCP( pc_user, "Some data here...", 12 );
	       while( pc_user )
	       {
	           char buf[256];
	           if( !fgets( buf, 256, stdin ) )
	           {
	               RemoveClient( pc_user );
	               return 0;
	           }
	           SendTCP( pc_user, buf, strlen( buf ) );
	       }
	       return -1;
	   }
```


	   Example Two : A server application, opens a socket that it
	   accepts connections on. Reads the socket, and writes the
	   information it reads back to the socket as an echo.

``` c
	   #include <stdhdrs.h>
	   #include <sharemem.h>
	   #include <timers.h>
	   #include <network.h>

	   void CPROC ServerRecieve( PCLIENT pc, POINTER buf, int size )
	   {
	       //int bytes;
	       if( !buf )
	       {
	           buf = Allocate( 4096 );
	           //SendTCP( pc, (void*)"Hi, welccome to...", 15 );
	       }
	       //else
	           //SendTCP( pc, buf, size );

	       // test for waitread support...
	       // read will not result until the data is read.
	       //bytes = WaitReadTCP( pc, buf, 4096 );
	       //if( bytes > 0 )
	       //   SendTCP( pc, buf, bytes );

	       ReadTCP( pc, buf, 4095 );
	       // buffer does not have anything in it....
	   }

	   void CPROC ClientConnected( PCLIENT pListen, PCLIENT pNew )
	   {
	       SetNetworkReadComplete( pNew, ServerRecieve );
	   }

	   int main( int argc, char **argv )
	   {
	       PCLIENT pcListen;
	       SOCKADDR *port;
	       if( argc < 2 )
	       {
	           printf( "usage: %s <listen port> (defaulting to telnet)\\n", argv[0] );
	           port = CreateSockAddress( "localhost:23", 23 );
	       }
	       else
	           port = CreateSockAddress( argv[1], 23 );
	       NetworkStart();
	       pcListen = OpenTCPListenerAddrEx( port, ClientConnected );
	       if(pcListen)
	           while(1) WakeableSleep( SLEEP_FOREVER );
	       else
	           printf( "Failed to listen on port %s\\n", argv[1] );
	       return 0;
	   }

```
