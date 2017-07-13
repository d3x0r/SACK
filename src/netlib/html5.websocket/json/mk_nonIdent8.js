
var out = '';

for( var c = 0; c < 255; c++ ) {
	switch( c ){
				case '.'.codePointAt(0):
				// the following two are allowed in JS identifiers
				// case '$'.codePointAt(0): case '_'.codePointAt(0):
				case '!'.codePointAt(0): case '?'.codePointAt(0): case '#'.codePointAt(0): case '%'.codePointAt(0): case '@'.codePointAt(0): 
				case '+'.codePointAt(0): case '-'.codePointAt(0): case '/'.codePointAt(0): case '\\'.codePointAt(0): case '*'.codePointAt(0): case '&'.codePointAt(0): case '='.codePointAt(0):
				case '('.codePointAt(0): case ')'.codePointAt(0): 
				case ','.codePointAt(0): case ';'.codePointAt(0): case ':'.codePointAt(0):
				case '['.codePointAt(0): case ']'.codePointAt(0): case '<'.codePointAt(0): case '>'.codePointAt(0):
				case '~'.codePointAt(0): case '|'.codePointAt(0):  // filtered above
				case '{'.codePointAt(0): case '}'.codePointAt(0):  // filtered above anyway
				case 0x7f:  // filtered above
                                		out +=  ',1' ;
                                	break;
                                default:
					if( ( c > 0x7b && c < 0xbf )  
						|| (c <= 32)    // control characters and space
						|| ( c == 0xD7 || c == 0xF7 ) ) { // multiplication sign, division sign )
						if( c == '\n'.codePointAt(0) 
							|| c  == ' '.codePointAt(0)
							|| c == '\t'.codePointAt(0) 
							|| c  == '\r'.codePointAt(0) )
	                                               out += ',0';  // white space characters have special handling
						else
							out += ',1';
					} else 
                                               out += ',0' ;
	                                break;	

	}
}
	console.log( out );
