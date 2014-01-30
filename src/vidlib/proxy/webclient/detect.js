


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
     		ws = new WebSocket( default_connect_address );
  	}
  	if( "MozWebSocket" in window )
  	{
     		alert( "mozwebsocket..." );
     
		try
                {
	  		var moztest = MozWebSocket( default_connect_address );
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
     ws = new WebSocket( default_connect_address );
  }
  
  if( ws === undefined )
  {
  	alert( "No Support works." );
  }
  return ws;
} 
