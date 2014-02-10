


var Detect = {};

Detect.supportsWebSocket = function() {
    return window.WebSocket || window.MozWebSocket;
};


function WebSocketTest()
{
	var ws = undefined;
	if( "WebSocketDraft" in window )
	{
		//alert( "Draft websocket availabe" );
		ws = new WebSocket( default_connect_address );
	}
	if( "MozWebSocket" in window )
	{
		//alert( "mozwebsocket..." );
		ws = new MozWebSocket( default_connect_address );
	}
  if ("WebSocket" in window)
  {
     //alert("WebSocket is supported by your Browser!");
     // Let us open a web socket
     ws = new WebSocket( default_connect_address );
  }
  
  if( ws === undefined )
  {
  	alert( "No WebSocket Support... ?" );
  }
  return ws;
} 
