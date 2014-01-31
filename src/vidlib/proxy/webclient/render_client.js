function render_surface( remote_id, canvas )
{
	this.server_id = remote_id;
	this.canvas = canvas;
}

function proxy_image()
{
	var x;
	var y;
	var w;
	var h;     
	var parent;  // relation with other images
	var child;   // relation with other images
	var next;   // relation with other images
	var prior; // relation with other images
	var renderer;  // which render_surface this represents (null if static resource, uses image)
	var server_id; // what the server calls this image (low number probably) 
	var image; // actual Image() instance
}

function OpenServer()
{
	var ws= WebSocketTest()
	if( ws == undefined )
		return;
     
	var image_list = [];
	var render_list = []; 

	function find_image( server_id )
	{
		if( server_id < 0 )
			return null;
		var idx;
		for( idx = 0; idx < image_list.length; idx++ )
		{
			//console.log( "is " + image_list[idx].server_id +"=="+ server_id + " ?" );
			if( image_list[idx].server_id == server_id )
				return image_list[idx];
		}	
		console.log( "did not find image " + server_id );		
		return null;
	}	
	
	function find_render( server_id )
	{
		if( server_id < 0 )
			return null;
		var idx;
		for( idx = 0; idx < render_list.length; idx++ )
		{
			if( render_list[idx].server_id == server_id )
				return render_list[idx];
		}		
		return null;
	}	
	 
 
     ws.onopen = function()
     {
        // Web Socket is connected, send data using send()
     };
    
	function getPosition(event)
	{
		var x = event.x;
		var y = event.y;

		var n;
		for( n = 0; n < render_list.length; n++ )
		{
			var canvas = render_list[n].canvas;
			if( x < canvas.offsetLeft || y < canvas.offsetTop )
			  continue;
			if( ( x - canvas.offsetLeft ) > canvas.offsetWidth || ( y - canvas.offsetTop ) > canvas.offsetHeight )
			  continue;
			x -= canvas.offsetLeft;
			y -= canvas.offsetTop;

			console.log( "click on render " + render_list[n].server_id + " x:" + x + " y:" + y);
		}
	}
	function mousemover(event)
	{
		var x = event.x;
		var y = event.y;

		var n;
		for( n = 0; n < render_list.length; n++ )
		{
			var canvas = render_list[n].canvas;
			if( x < canvas.offsetLeft || y < canvas.offsetTop )
			  continue;
			if( ( x - canvas.offsetLeft ) > canvas.offsetWidth || ( y - canvas.offsetTop ) > canvas.offsetHeight )
			  continue;
			x -= canvas.offsetLeft;
			y -= canvas.offsetTop;

			console.log( "move on render " + render_list[n].server_id + " x:" + x + " y:" + y);
		}
	}
	
	function HandleMessage( msg )
	{
	  console.log( "message:" + msg.MsgID );
//		if( msg.MsgID > 3 )
	//		console.log("Message is received..." + evt.data );
     	switch( msg.MsgID )
        {
        case 0: // PMID_Version
        	
        	break;
        case 1: // PMID_SetApplicationTitle
                break;
        case 2: // PMID_SetApplicationIcon
        	break;
        case 3: // PMID_OpenDisplayAboveUnderSizedAt
           //ws.send(JSON.stringify(new Buffer(fs.readFileSync('./frame_border.png'), 'binary').toString('base64')));
		  
		  
        	var canvas = document.createElement('canvas');
			canvas.id     = "Render" + msg.data.server_render_id;
			canvas.width  = msg.data.width;
			canvas.height = msg.data.height;
			canvas.style.zIndex   = 8;
			canvas.style.position = "absolute";
			canvas.style.border   = "1px solid";
			canvas.addEventListener("mousedown", getPosition, false);
			canvas.addEventListener("mousemove", mousemover, false);
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
			var canvas = null;
			canvas = find_render( msg.data.server_render_id );
			image_list.push( image = new proxy_image() );
			if( canvas == null )
			{
				image.image = new Image();
			}
			image.server_id = msg.data.server_image_id;
			image.width = msg.data.width;
			image.height = msg.data.height;
			image.renderer = canvas;
			break;
		case 7: // PMID_MakeSubImage
			image_list.push( image = new proxy_image() );
			image.server_id = msg.data.server_image_id;
			image.x = msg.data.x;
			image.y = msg.data.y;
			image.width = msg.data.width;
			image.height = msg.data.height;
			image.parent = find_image( msg.data.server_parent_image_id );
			if( image.parent != null )			
			{
				if( ( image.next = image.parent.child ) != null )
				{
					image.next.prior = image;
				}
				image.parent.child = image;
			}
			
			break;
        case 8: // PMID_BlatColor
        case 9: // PMID_BlatColorAlpha
			image = find_image( msg.data.server_image_id );
			parent_image = image;
			ofs_x = 0;
			ofs_y = 0;
			while( parent_image.parent != null )
			{
				ofs_x += parent_image.x;
				ofs_y += parent_image.y;
				parent_image = parent_image.parent;
			}
			render = parent_image.renderer;
			var ctx= render.canvas.getContext("2d");
			ctx.fillStyle=msg.data.color;
			ctx.fillRect(ofs_x + msg.data.x, ofs_y + msg.data.y, msg.data.width, msg.data.height );
			break;
		case 10: // PMID_ImageData
			image = find_image( msg.data.server_image_id );	
			image.image.src = msg.data.data;
			break;
		case 11: // PMID_BlotImageSizedTo
			image = find_image( msg.data.server_image_id );
			parent_image = image;
			src_image = parent_src_image =  find_image( msg.data.image_id );
			ofs_x = 0;
			ofs_y = 0;
			ofs_xs = 0;
			ofs_ys = 0;
			while( parent_image.parent != null )
			{
				ofs_x += parent_image.x;
				ofs_y += parent_image.y;
				parent_image = parent_image.parent;
			}
			
			while( parent_src_image.parent != null )
			{
				ofs_xs += parent_src_image.x;
				ofs_ys += parent_src_image.y;
				parent_src_image = parent_src_image.parent;
			}
			
			render = parent_image.renderer;
			var ctx= render.canvas.getContext("2d");
			source_image = find_image( msg.data.image_id );
			ctx.drawImage( parent_src_image.image
				, ofs_xs + msg.data.xs, ofs_ys + msg.data.ys 
				, msg.data.width, msg.data.height
				, ofs_x + msg.data.x, ofs_y + msg.data.y
				, msg.data.width, msg.data.height
				);
		
		    break;
		case 12: // PMID_BlotScaledImageSizedTo
			image = find_image( msg.data.server_image_id );
			parent_image = image;
			src_image = parent_src_image =  find_image( msg.data.image_id );
			ofs_x = 0;
			ofs_y = 0;
			ofs_xs = 0;
			ofs_ys = 0;
			while( parent_image.parent != null )
			{
				ofs_x += parent_image.x;
				ofs_y += parent_image.y;
				parent_image = parent_image.parent;
			}
			
			while( parent_src_image.parent != null )
			{
				ofs_xs += parent_src_image.x;
				ofs_ys += parent_src_image.y;
				parent_src_image = parent_src_image.parent;
			}
			
			render = parent_image.renderer;
			var ctx= render.canvas.getContext("2d");
			source_image = find_image( msg.data.image_id );
			ctx.drawImage( parent_src_image.image
				, ofs_xs + msg.data.xs, ofs_ys + msg.data.ys 
				, msg.data.ws, msg.data.hs
				, ofs_x + msg.data.x, ofs_y + msg.data.y
				, msg.data.width, msg.data.height
				);
			break;
		case 13: //PMID_DrawLine
			image = find_image( msg.data.server_image_id );
			parent_image = image;
			ofs_x = 0;
			ofs_y = 0;
			ofs_xs = 0;
			ofs_ys = 0;
			while( parent_image.parent != null )
			{
				ofs_x += parent_image.x;
				ofs_y += parent_image.y;
				parent_image = parent_image.parent;
			}
			render = parent_image.renderer;
			var ctx= render.canvas.getContext("2d");
			
			ctx.strokeStyle = msg.data.color;
			ctx.beginPath();
			ctx.moveTo( ofs_x + msg.data.x1, ofs_y + msg.data.y1 );
			ctx.lineTo( ofs_x + msg.data.x2, ofs_y + msg.data.y2 );
			ctx.stroke();
			break;
		
        }
	}

	
	
     ws.onmessage = function (evt) 
     { 
		console.log( evt.data );
        var msg = JSON.parse(evt.data);
		if(  Object.prototype.toString.call(msg) === '[object Array]' )
		{ 
			var m;
			for( m = 0; m < msg.length; m++ )
				HandleMessage( msg[m] );
		}
		else
			HandleMessage( msg );
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
