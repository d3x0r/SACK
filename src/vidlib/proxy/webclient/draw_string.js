
function font_character()
{
	var character;
	var x, y, w, h;
}
function string_font()
{
	var server_id;
    var images;
	var baseline;
    var character_index = [];
    var characters = [];
	var image;
}

function tint_image(imageObj, context, canvas){
    var destX = 0;
    var destY = 0;
  
    context.drawImage(imageObj, destX, destY);
  
    var imageData = context.getImageData(0, 0, canvas.width, canvas.height);
    var pixels = imageData.data;
    for (var i = 0; i < pixels.length; i += 4) {
        pixels[i]   = 255 - pixels[i];   // red
        pixels[i+1] = 255 - pixels[i+1]; // green
        pixels[i+2] = 255 - pixels[i+2]; // blue
        // i+3 is alpha (the fourth element)
    }
  
    // overwrite original image
    context.putImageData(imageData, 0, 0);
}



function putString( image, x, y, color,background,string, font, orientation, justification, width )
{
	var _x = x;
	var _y = y;
	render = image.renderer;
	//console.log( "font image " + font.image + " " + image.renderer );
	var ctx= render.canvas.getContext("2d");
	//console.log( "yay, put a string: " + string + " output to " + ctx );
	if( orientation == 0 )
	{
		var charval = 0;
		for( i = 0; i < string.length; i++ )
		{
			charval = string[i].charCodeAt();
			// handle utf-8 encoding
			if( charval & 0x80 )
			{
				if( ( charval & 0xE0 ) == 0xC0 ) 
				{
					charval = ( ( string[i].charCodeAt() & 0x1F ) << 6 ) | ( string[i+1].charCodeAt() & 0x3f );
					i++;
				}
				else if( ( charval & 0xE0 ) == 0xe0 )
				{
					charval = ( ( string[i].charCodeAt() & 0xF ) << 12 ) | ( ( string[i+1].charCodeAt() & 0x3F ) << 6 ) | ( string[i+2].charCodeAt() & 0x3f );
					i+=2;
				}
			}
			//console.log( "Character is " + charval );
			{

				//source_image = find_image( msg.data.image_id );
				var character = font.character_index[charval];
				//console.log( character.x + " " + character.y + " " + character.w + " " + character.h + " " + x + " " + y + "  " + font.image );

				switch( orientation )
				{
				default:
				case 0://OrderPoints:
					xd = x;
					yd = y+(font.baseline - character.a/*scent*/);
					xd_back = xd;
					yd_back = y;
					break;
				case 1://OrderPointsInvert:
					//orientation = BLOT_ORIENT_INVERT;
					xd = x;
					yd = y- font.baseline + character.a/*scent*/;
					xd_back = xd;
					yd_back = y;
					break;
				case 2://OrderPointsVertical:
					//orientation = BLOT_ORIENT_VERTICAL;
					xd = x - (font.baseline - character.a/*scent*/);
					yd = y;
					xd_back = x;
					yd_back = yd;
					break;
				case 3://OrderPointsVerticalInvert:
					//orientation = BLOT_ORIENT_VERTICAL_INVERT;
					xd = x + (font.baseline - character.a/*scent*/);
					yd = y;
					xd_back = x;
					yd_back = yd;
					break;
				}

				ctx.drawImage( font.image.image
					, character.x, character.y
					, character.w, character.h
					, xd, yd
					, character.w, character.h
					);
				
			}
						
			if( charval == '\n' )
			{
				x = _x;
				_y += font.height;
			}
			else
			{
				//console.log( "using character " + character );
				x += character.w;
			}
		}
	}
}

