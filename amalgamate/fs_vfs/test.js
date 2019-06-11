	import { Module as vfsModule } from "./sack.vfs.wasm.js" 

async function asdf() {
	vfsModule.onRuntimeInitialized = (module)=>{
		vfsModule._initFS();
		const sack = vfsModule.SACK;
		window.SACK = sack;
		//JSOXwasm._initJSOX();

		console.log( "Did I get a module?", sack );
		//var v = sack.Volume();
		var v = sack.Volume( "mount", "./file.dat" );
		//var f = v.File( "./Test.dat" );
		//f.close();
	}
}
asdf();
