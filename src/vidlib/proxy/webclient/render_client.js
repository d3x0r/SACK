
function render_surface( remote_id, canvas )
{
    render_surface.lastStorageId = 0;

   
   this.server_render_id = remote_id;
   this.canvas = canvas;
   this.local_id = render_surface.lastStorageId++;
   
}

function OpenServer()
{
     var ws= WebSocketTest()
     if( ws == undefined )
     	return;
     
	 var image_list[];
     var render_list = [];     
     ws.onopen = function()
     {
        // Web Socket is connected, send data using send()
     };
    
     ws.onmessage = function (evt) 
     { 
        alert("Message is received..." + evt.data );
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
            render_list.push( render = new render_surface( msg.data.server_render_id, canvas ) );
            ws.send( JSON.stringify( 
                	{
                        	MsgID:5 /*PMID_Reply_OpenDisplayAboveUnderSizedAt*/,
                                data: {
                                	client_render_id:render.local_id,
                                        server_render_id:msg.data.server_render_id
                                }
                        } )  );
	        break;
        case 4: // PMID_CloseDisplay
        	var canvas = document.getElementById("Render" + msg.data.server_render_id);
        	document.body.removeChild( canvas );
        	break;
		case 6: // PMID_MakeImage
		case 7: // PMID_MakeSubImage
        case 8: // PMID_BlatColor
        case 9: // PMID_BlatColorAlpha
			
        }
     };
     ws.onclose = function( evt )
     { 
        // websocket is closed.
        alert("Connection is closed... (closing window)" + evt.data ); 
      	for (var i = 0; i < render_list.length; i++) {
               	document.body.removeChild( render_list[i].canvas );
        }
       
     };
     ws.onerror = function(evt)
     {
        alert( "connection error: " + evt.data );
     }
}
