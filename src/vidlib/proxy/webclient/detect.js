
var Detect = {};

Detect.supportsWebSocket = function() {
    return window.WebSocket || window.MozWebSocket;
};


function WebSocketTest()
{
   	var ws = undefined;
	if( "WebSocketDraft" in window )
  	{
		alert( "Draft websocket availabe" );
     		ws = new WebSocket("ws://localhost:9998/echo");
  	}
  	if( "MozWebSocket" in window )
  	{
     		alert( "mozwebsocket..." );
     
		try
                {
	  		var moztest = MozWebSocket( "ws://localhost:4240/Sack/Vidlib/Proxy" );
		}
		catch( err )
		{
			alert(err );
		}
	}
	if( "MozWebSocket" in window )
  		alert( "mozwebsocket..." );
  if ("WebSocket" in window)
  {
     alert("WebSocket is supported by your Browser!");
     // Let us open a web socket
     ws = new WebSocket("ws://localhost:9998/echo");
  }
  if( ws === undefined )
  {
  	alert( "No Support works." );
  }
     ws.onopen = function()
     {
        // Web Socket is connected, send data using send()
        ws.send("Message to send");
        alert("Message is sent...");
        
  // Construct a msg object containing the data the server needs to process the message from the chat client.  
  var msg = {  
    type: "message",  
    text: "banana_form_text",  
    id:   1324,  
    date: Date.now()  
  };  
  
  // Send the msg object as a JSON-formatted string.  
  ws.send(JSON.stringify(msg));  
    
       ws.send('say', {name:'foo', text:'baa'});
        alert("other Message is sent...");
     };
     ws.onmessage = function (evt) 
     { 
        var received_msg = evt.data;
        alert("Message is received..." + evt.data );
     };
     ws.onclose = function( evt )
     { 
        // websocket is closed.
        alert("Connection is closed..." + evt.data ); 
     };
     ws.onerror = function(evt)
     {
        alert( "connection error: " + evt.data );
     }
}
