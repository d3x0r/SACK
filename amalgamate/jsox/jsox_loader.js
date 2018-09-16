
"use strict";

var jsox;
try{
  jsox = require( "./build/RelWithDebInfo/jsox.node" );
} catch(err1) {
  try {
    //console.log( err1 );
    jsox = require( "./build/Debug/jsox.node" );
  } catch( err2 ){
    try {
      //console.log( err2 );
      jsox = require( "./build/Release/jsox.node" );
    } catch( err3 ){
      console.log( err1 )
      console.log( err2 )
      console.log( err3 )
    }
  }
}

const FS = require('fs');

require.extensions['.jsox'] = function (module, filename) {
    var content = FS.readFileSync(filename, 'utf8');
    //var content = disk.read(filename).toString();
    module.exports = jsox.parse(content);
};

module.exports = jsox.JSOX;
