
var default_connect_address = "ws://localhost:4240/Sack/Vidlib/Proxy"
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
  
     ws.onopen = function()
     {
        // Web Socket is connected, send data using send()
     };
    
     ws.onmessage = function (evt) 
     { 
        //alert("Message is received..." + evt.data );
        var msg = JSON.parse(evt.data);
     	switch( msg.MsgID )
        {
        case 0: // PMID_Version
        	
        	break;
        case 1: // PMID_SetApplicationTitle
                break;
        case 2: // PMID_SetApplicationIcon
        	break;
        case 3: // PMID_OpenDisplayAboveUnderSizedAt
          
        	var canvas = document.createElement('canvas');
		canvas.id     = "Render" + msg.data.server_render_id;
		canvas.width  = msg.data.width;
		canvas.height = msg.data.height;
		canvas.style.zIndex   = 8;
		canvas.style.position = "absolute";
		canvas.style.border   = "1px solid";
                document.body.appendChild(canvas);
	        break;
        case 4: // PMID_CloseDisplay
        	var canvas = document.getElementById("Render" + msg.data.server_render_id);
                document.body.removeChild( canvas );
        	break;
        }
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
