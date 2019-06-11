#include <stdint.h>


#include <stdint.h>
#include <emscripten.h>

#include "sack.h"

enum buildinTypes {
	JS_VALUE_UNDEFINED,
	JS_VALUE_FALSE,
	JS_VALUE_TRUE,
	JS_VALUE_NULL,
	JS_VALUE_NEG_INFINITY,
	JS_VALUE_INFINITY,
	JS_VALUE_NAN,
};



static void throwError( const char *error ){
	EM_ASM( {
		const string = UTF8ToString( $0 );
		throw new Error( str );
	},error);
}

PLINKQUEUE plqLists;

uintptr_t getValueList( void ) {
	PDATALIST *ppdl;
	if( !(ppdl = (PDATALIST*)DequeLink( &plqLists ) ) ){
		ppdl = New( PDATALIST );
		ppdl[0] = CreateDataList( sizeof( struct jsox_value_container ) );
	}else {
		ppdl[0]->Cnt = 0;		
	}
	return (uintptr_t)ppdl;
}

void dropValueList( PDATALIST *ppdl ) {
	EnqueLink( &plqLists, ppdl );
}

//----------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------

//-s EXPORTED_FUNCTIONS="['_initJSOX']" 
void initFS( void )  EMSCRIPTEN_KEEPALIVE;
void initFS( void ) 
{
	EM_ASM( (

		var r = FS.mount(IDBFS, {}, '/home/web_user');

		  
        //persist changes
		FS.syncfs(false,function (err) {
                          assert(!err);
        });

		//console.log( "Log:", r );

		if( !Module.this_ ) {
			Module.this_ = {};
		}

		Module.this_.callbacks = Module.this_.callbacks || [];
		Module.this_.objects = Module.this_.objects || [undefined,false,true,null,-Infinity,Infinity,NaN];

		function TranslateText(s){ return s; }
		
		function Volume(mountName, fileName, version ) {
			var key1, key2;
			if( arguments.length == 0 ) {
				fileName = null;
				mountName = null;
				version = 0;
				key1 = null;
				key2 = null;
			} else if( arguments.length == 1 ) {
				fileName = mountName;
				mountName = NULL;
				version = 0;
				key1 = null;
				key2 = null;
			}
			else if( arguments.length == 2 ) {
				version = 0;
				key1 = null;
				key2 = null;
			}else {
				key1 = arguments[3];
				key2 = arguments[4];
			}
			if( !(this instanceof Volume) ) return new Volume( mountName,fileName,version,key1,key2);

			var s; // u8array
			var mni = mountName?allocate(s = intArrayFromString(mountName), 'i8', ALLOC_NORMAL):0;
			var fni = fileName?allocate(s = intArrayFromString(fileName), 'i8', ALLOC_NORMAL):0;
			var k1i = key1?allocate(s = intArrayFromString(key1), 'i8', ALLOC_NORMAL):0;
			var k2i = key2?allocate(s = intArrayFromString(key2), 'i8', ALLOC_NORMAL):0;
			if( typeof version !== "number" )
				version = 0;
			this.vol = Module._Volume(mni,fni,version,k1i,k2i);
			var result = Module.this_.objects[this.vol];
			if( arguments.length == 1 ) {
				fileName = mountName;
				mountName = null;
			}
		};

		Volume.mapFile =function(name) {
			console.log( "map File is not supported in emscripten" );
		};
		Volume.mkdir =function(name) {
			Module._sack_mkdir( name )
		};
		Volume.readAsString =function(name) {
			var intName = allocate(s = intArrayFromString(mountName), 'i8', ALLOC_NORMAL);
			var intstr = Module._readAsString( intName );
			Module._Release( intName );
			const string = UTF8ToString( intstr );
			Module._Release( intstr );
			return string;
		};


		const volumeMethods = {
			File(filename) {
				return new File( this.vol, filename );
			},
			dir(){
			},
			exists(fileOrPathName){
			},
			read(fileName){
			},
			readJSON(fileName, callback){
			},
			readJSON6(fileName, callback){
			},
			readJSOX(fileName, callback){
			},
			write(fileName,buffer){
			},
			Sqlite(database){
				return Module._openSqlite( this.vol, database );
			},
			rm(file){
			},
			mv(file,new_filename){
			},
			mkdir(path){
			},
			rekey(version,a,b){
			},
		};
			
		volumeMethods.delete = volumeMethods.rm;
		volumeMethods.unlink = volumeMethods.rm;
		volumeMethods.setOption = volumeMethods.so;
		volumeMethods.rename = volumeMethods.mv;
	        
		Object.defineProperties( Volume.prototype, Object.getOwnPropertyDescriptors( volumeMethods ));

		//-------------------------------------------------------------------


		function File(vol, name) {
			if( !(this instanceof File))return new File(vol,name);
			if( name ) {
				this.vol = vol;
				var s;
				var ni = allocate(s = intArrayFromString(name), 'i8', ALLOC_NORMAL);
				this.file = Module._File( vol, ni );
				Module._free( ni );
			}
		};

		File.SeekSet = 0;
		File.SeekCurrent = 1;
		File.SeekEnd = 2;

		const fileMethods = {
			read(size, position){
				if( position ) {

				}
			},
			readLine( position ){
			},
			write(value){
			},
			close(){
				Module._closeFile( this.file );
				FS.syncfs(false,function (err) {
					assert(!err);
		        });
			},
			writeLine( line, position ){

			},
			seek(pos,whence){
				if( typeof pos == "number" ){
					if( typeof whence == "number"){
						Module._seekFile(this.file, pos, whence );
					}else {
						Module._seekFile( this_file, pos, SEEK_SET );
					}
				}
			},
			trunc(){
				Module._truncateFile( this.file );
			},
            pos(){
				return Module._tellFile( this.file );
			},
		};
		Object.defineProperties( File.prototype, Object.getOwnPropertyDescriptors( fileMethods ));

		//-------------------------------------------------------------------

		function Sqlite( db ) {
			if( !this instanceof Sqlite ) return new Sqlite(db);
			
		};
		Sqlite.eo = function(cb) {
			console.log( "enum option not implemented" );
		};
		Sqlite.go = function(a,b,c) {
			console.log( "get option not implemented" );
			if( !c ) {
				c = b;
				b = null;
			}
		};
		Sqlite.so = function(a,b,c) {
			console.log( "set option not implemented" );
		};
		Sqlite.options = {};
		Sqlite.optionTreeNode = function( real ) {
			this.this_ = real;			
		}
		Sqlite.optionTreeNode.prototype.eo = function(cb) {
			
		}
		Sqlite.optionTreeNode.prototype.go = function(cb) {
			
		}
		Sqlite.optionTreeNode.prototype.so = function(cb) {
			
		}
		Sqlite.optionEditor = function() {
			console.log( "editor not implemented" );
		};
		
		const sqliteMethods = {

			escape (s) {
				var sa;
				var si = s?allocate(sa=intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				var intstr = Module._sqlescape( this.sql, si, s?sa.length:0 );
				Module._free( si );
				const string = UTF8ToString( intstr );
				Module._free(intstr);
				return string;
			},
			unescape (s) {
				var sa;
				var si = s?allocate(sa=intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				var intstr = Module._sqlunescape( this.sql, si, s?sa.length:0 );
				Module._free( si );
				const string = UTF8ToString( intstr );
				Module._free(intstr);
				return string;
			},

			end(s) {
				Module._sqlEnd( this.sql );
			},
			transaction(s) {
				Module._sqlTransaction( this.sql );
				
			},
			commit(s) {
				Module._sqlCommit( this.sql );
				
			},
			do(s,...args) {
				if( argguments.length == 0 ) {
					throw new Error( TranslateText( "Required parameter, SQL query, is missing.") );
					return;
				}
				var statement = null;
				var si = s?allocate(intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				if( args.length ) {
					var arg = 1;
					var isFormatString;
					var pvtStmt = '';
					
					if( s.include( ":" )
						|| s.include( '@' )
						|| s.include( '$' ) ) {
						if( typeof args[1] === "object" ) {
							arg = 2;
							pdlParams = CreateDataList( sizeof( struct jsox_value_container ) );
							var params =  args[1] ;
							var paramNames = Object.keys( params );
							for( var p = 0; p < paramNames->length; p++ ) {
								var valName = paramNames[p];
								var value = params[valName];
								var na
								var ni = valName?allocate(na=intArrayFromString(valName), 'i8', ALLOC_NORMAL):0;
								switch( typeof value ) {
								default:
									throw new Error( "Unsupported value type:" + typeof value );
								case "string":
									var vala
									var vali = value?allocate(na=intArrayFromString(value), 'i8', ALLOC_NORMAL):0;
									Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_NUMBER, 0, 0, 0, vala, vala.length );
									break;
								case "number":
									if( value|0 === value )
										Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_NUMBER, 0, 0, value, 0, 0 );
									else
										Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_NUMBER, 1, value, 0, 0, 0 );
									break;
								case "boolean":
									if( value )
										Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_TRUE, 0, 0, 0, 0, 0 );
									else
										Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_TRUE, 0, 0, 0, 0, 0 );
									break;
							 	case "undefined":
									Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_UNDEFINED, 0, 0, 0, 0, 0 );
									break;
							 	case "object":
								 	if( value === null ){
									 	Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_NULL, 0, 0, 0, 0, 0 );
										break;
									} 
								}
							}
						}
						else {
							throw new Error( TranslateText("Required parameter 2, Named Paramter Object, is missing.") );
							return;
						}
						isFormatString = true;
					}
					else if( s.include( '?' ) ) {
						isFormatString = true;
					} 
					else {
						arg = 0;
						isFormatString = false;
					}

					if( !pdlParams )
						pdlParams = Module._getValueList();
					if( !isFormatString ) {
						for( ; arg < args.Length(); arg++ ) {
							var value = args[arg];
							switch( typeof value ) {
							case  "string": {
								//String::Utf8Value text( USE_ISOLATE(isolate) args[arg]->ToString( isolate->GetCurrentContext() ).ToLocalChecked() );
								if( arg & 1 ) { // every odd parameter is inserted
									var si = s?allocate(sa=intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_STRING, 0, 0, 0, args[arg], args[arg].length );
									
									pvtStmt += '?';
								}
								else {
									pvtStmt += text;
									continue;
								}
							}
							break;
							case "object":							
								if( args[arg] === null ) {
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_NULL, 0, 0, 0, 0, 0 );
								}
								switch( Object.getPrototypeOf(args[arg]).constructor.name ){
								case "ArrayBuffer": {
										Local<ArrayBuffer> myarr = arg.As<ArrayBuffer>();
										val.string = (char*)myarr->GetContents().Data();
										val.stringLen = myarr->ByteLength();
										val.value_type = JSOX_VALUE_TYPED_ARRAY;
										AddDataItem( pdlParams, &val );
									}
									break;
								case "Uint8Array":  {
										Local<Uint8Array> _myarr = arg.As<Uint8Array>();
										Local<ArrayBuffer> buffer = _myarr->Buffer();
										val.string = (char*)buffer->GetContents().Data();
										val.stringLen = buffer->ByteLength();
										val.value_type = JSOX_VALUE_TYPED_ARRAY;
										AddDataItem( pdlParams, &val );
									}
									break;
								}
								break;
							case "number"
								if( value|0 === value )
									Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_NUMBER, 0, 0, value, 0, 0 );
								else
									Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_NUMBER, 1, value, 0, 0, 0 );
								break;
							case "boolean":
								if( value )
									Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_TRUE, 0, 0, 0, 0, 0 );
								else
									Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_TRUE, 0, 0, 0, 0, 0 );
								break;
							case "undefined":
								Module._PushValue( pdlParams, valName, valName.length, JSOX_VALUE_UNDEFINED, 0, 0, 0, 0, 0 );
								break;

							default: {
								throw new Error( "Unsupported object conversion:" + value )	lprintf( "Unsupported TYPE" );
								}
							}
							pvtStmt += '?';
						}
						statement = pvtStmt;
					}
					else {
						statement = s;
						for( ; arg < args.Length(); arg++ ) {
							var value = args[arg];
							switch( typeof value ) {
							default:
								throw new Error( "Unsupported value type:" + typeof value );
							case "string":
								var vala
								var vali = value?allocate(na=intArrayFromString(value), 'i8', ALLOC_NORMAL):0;
								Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_NUMBER, 0, 0, 0, vala, vala.length );
								break;
							case "number":
								if( value|0 === value )
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_NUMBER, 0, 0, value, 0, 0 );
								else
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_NUMBER, 1, value, 0, 0, 0 );
								break;
							case "boolean":
								if( value )
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_TRUE, 0, 0, 0, 0, 0 );
								else
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_TRUE, 0, 0, 0, 0, 0 );
								break;
							case "undefined":
								Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_UNDEFINED, 0, 0, 0, 0, 0 );
								break;
							case "object":
								if( value === null ){
									Module._PushValue( pdlParams, 0, 0, JSOX_VALUE_NULL, 0, 0, 0, 0, 0 );
									break;
								} 
							}
						}
					}
				}
				else
					Module._sqlDo( this.sql, si  );

				if( statement ) {
					//String::Utf8Value sqlStmt( USE_ISOLATE( isolate ) args[0] );
					PDATALIST pdlRecord = NULL;
					INDEX idx = 0;
					int items;
					struct jsox_value_container * jsval;

					if( !SQLRecordQuery_js( sql->odbc, statement, statementlen, &pdlRecord, pdlParams DBG_SRC ) ) {
						const char *error;
						FetchSQLError( sql->odbc, &error );
						isolate->ThrowException( Exception::Error(
							String::NewFromUtf8( isolate, error, v8::NewStringType::kNormal ).ToLocalChecked() ) );
						DeleteDataList( &pdlParams );
						return;
					}

					DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
						if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;
					}
					items = (int)idx;

					//&sql->columns, &sql->result, &sql->resultLens, &sql->fields
					if( pdlRecord )
					{
						int usedFields = 0;
						int maxDepth = 0;
						struct fieldTypes {
							const char *name;
							int used;
							int first;
							Local<Array> array;
						} *fields = NewArray( struct fieldTypes, items ) ;
						int usedTables = 0;
						struct tables {
							const char *table;
							const char *alias;
							Local<Object> container;
						}  *tables = NewArray( struct tables, items + 1);
						struct colMap {
							int depth;
							int col;
							const char *table;
							const char *alias;
							Local<Object> container;
							struct tables *t;
						}  *colMap = NewArray( struct colMap, items );
						tables[usedTables].table = NULL;
						tables[usedTables].alias = NULL;
						usedTables++;



						DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
							int m;
							if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;

							for( m = 0; m < usedFields; m++ ) {
								if( StrCaseCmp( fields[m].name, jsval->name ) == 0 ) {
									colMap[idx].col = m;
									colMap[idx].depth = fields[m].used;
									if( colMap[idx].depth > maxDepth )
										maxDepth = colMap[idx].depth+1;
									colMap[idx].alias = PSSQL_GetColumnTableAliasName( sql->odbc, (int)idx );
									int table;
									for( table = 0; table < usedTables; table++ ) {
										if( StrCmp( tables[table].alias, colMap[idx].alias ) == 0 ) {
											colMap[idx].t = tables + table;
											break;
										}
									}
									if( table == usedTables ) {
										tables[table].table = colMap[idx].table;
										tables[table].alias = colMap[idx].alias;
										colMap[idx].t = tables + table;
										usedTables++;
									}
									fields[m].used++;
									break;
								}
							}
							if( m == usedFields ) {
								colMap[idx].col = m;
								colMap[idx].depth = 0;
								colMap[idx].table = PSSQL_GetColumnTableName( sql->odbc, (int)idx );
								colMap[idx].alias = PSSQL_GetColumnTableAliasName( sql->odbc, (int)idx );
								if( colMap[idx].table && colMap[idx].alias ) {
									int table;
									for( table = 0; table < usedTables; table++ ) {
										if( StrCmp( tables[table].alias, colMap[idx].alias ) == 0 ) {
											colMap[idx].t = tables + table;
											break;
										}
									}
									if( table == usedTables ) {
										tables[table].table = colMap[idx].table;
										tables[table].alias = colMap[idx].alias;
										colMap[idx].t = tables + table;
										usedTables++;
									}
								} else
									colMap[idx].t = tables;
								fields[usedFields].first = (int)idx;
								fields[usedFields].name = jsval->name;// sql->fields[idx];
								fields[usedFields].used = 1;
								usedFields++;
							}
						}
						if( usedTables > 1 )
							for( int m = 0; m < usedFields; m++ ) {
								for( int t = 1; t < usedTables; t++ ) {
									if( StrCaseCmp( fields[m].name, tables[t].alias ) == 0 ) {
										PVARTEXT pvtSafe = VarTextCreate();
										vtprintf( pvtSafe, "%s : %s", TranslateText( "Column name overlaps table alias" ), tables[t].alias );
										throwError(  GetText( VarTextPeek( pvtSafe ) ) );
										VarTextDestroy( &pvtSafe );
										DeleteDataList( &pdlParams );
										return;
									}
								}
							}
						int records = makeArray();
						int record;
						if( pdlRecord ) {
							int row = 0;
							do {
								Local<Value> val;
								tables[0].container = record = makeObject();
								if( usedTables > 1 && maxDepth > 1 )
									for( int n = 1; n < usedTables; n++ ) {
										tables[n].container = makeObject();
										SET( record, tables[n].alias, tables[n].container );
									}
								else
									for( int n = 0; n < usedTables; n++ )
										tables[n].container = record;

								DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
									if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;

									Local<Object> container = colMap[idx].t->container;
									if( fields[colMap[idx].col].used > 1 ) {
										if( fields[colMap[idx].col].first == idx ) {
											if( !jsval->name )
												lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( statement ) );
											else
												SET( record, jsval->name
														, fields[colMap[idx].col].array = makeArray()
														);
										}
									}

									switch( jsval->value_type ) {
									default:
										lprintf( "Unhandled value result type:%d", jsval->value_type );
										break;
									case JSOX_VALUE_DATE:
										{
											val = makeDate( jsval->string, jsval->stringLen );
										}
										break;
									case JSOX_VALUE_TRUE:
										val = JS_VALUE_TRUE;
										break;
									case JSOX_VALUE_FALSE:
										val = JS_VALUE_FALSE;
										break;
									case JSOX_VALUE_NULL:
										val = JS_VALUE_NULL;
										break;
									case JSOX_VALUE_NUMBER:
										if( jsval->float_result ) {
											val = makeNumberf( jsval->result_d );
										}
										else {
											val = makeNumber( jsval->result_n );
										}
										break;
									case JSOX_VALUE_STRING:
										if( !jsval->string )
											val = JS_VALUE_NULL;
										else
											val = makeString( jsval->string, jsval->stringLen );
										break;
									case JSOX_VALUE_TYPED_ARRAY:
										//lprintf( "Should result with a binary thing" );
										{
											int ab = fillArrayBuffer( jsval->string, jsval->stringLen );
											//ArrayBuffer::New( isolate, (char*)Hold( jsval->string ), jsval->stringLen );
											val = ab;
										}
										break;
									}

									if( fields[colMap[idx].col].used == 1 ){
										if( !jsval->name )
											lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( statement ) );
										else
											SET( container, jsval->name, val );
									}
									else if( usedTables > 1 || ( fields[colMap[idx].col].used > 1 ) ) {
										if( fields[colMap[idx].col].used > 1 ) {
											if( !jsval->name )
												lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", GetText( statement ) );
											else
												SET( colMap[idx].t->container, jsval->name, val );
											if( colMap[idx].alias )
												SET( fields[colMap[idx].col].array, colMap[idx].alias, val );
											SETN( fields[colMap[idx].col].array, colMap[idx].depth, val );
										}
									}
								}
								SETN( records, row++, record );
							} while( FetchSQLRecordJS( sql->odbc, &pdlRecord ) );
						}
						Deallocate( struct fieldTypes*, fields );
						Deallocate( struct tables*, tables );
						Deallocate( struct colMap*, colMap );

						SQLEndQuery( sql->odbc );
						args.GetReturnValue().Set( records );
					}
					else
					{
						SQLEndQuery( sql->odbc );
						args.GetReturnValue().Set( Array::New( isolate ) );
					}
					DeleteDataList( &pdlParams );
				}
				
			},
			autoTransact(s, yesno) {
				Module._sqlAutoTransact( this.sql, yesno?1:0 );
			},
			makeTable(s) {
				var si = s?allocate(sa=intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				var r = Module._sqlmakeTable( this.sql, si );
				Module._free( si );
				return r?true:false;
			},
			op(...args) {
				var defaultVal, sect, optname;
				if( args.length > 0 ) {
					defaultVal = allocate(s = intArrayFromString(args[0]), 'i8', ALLOC_NORMAL);
				}
				else
					defaultVal = allocate(s = intArrayFromString(""), 'i8', ALLOC_NORMAL);

				if( args.length > 1 ) {
					sect = defaultVal;
					defaultVal = allocate(s = intArrayFromString(args[1]), 'i8', ALLOC_NORMAL);
				}
				else
					sect = 0;

				if( args.length > 2 ) {
					optname = defaultVal;
					defaultVal = allocate(s = intArrayFromString(args[2]), 'i8', ALLOC_NORMAL);
				}
				else
					optname = 0;
				Module._sqlgetOption( this.sql, sect, optname, defaultVal );
				if( sect ) Module._free( sect );
				if( optname ) Module._free( optname );
				if( defaultVal ) Module._free( defaultVal );
			},
			so(...args) {
				var defaultVal, sect, optname;
				if( args.length > 0 ) {
					defaultVal = allocate(s = intArrayFromString(args[0]), 'i8', ALLOC_NORMAL);
				}
				else
					defaultVal = allocate(s = intArrayFromString(""), 'i8', ALLOC_NORMAL);

				if( args.length > 1 ) {
					sect = defaultVal;
					defaultVal = allocate(s = intArrayFromString(args[1]), 'i8', ALLOC_NORMAL);
				}
				else
					sect = 0;

				if( args.length > 2 ) {
					optname = defaultVal;
					defaultVal = allocate(s = intArrayFromString(args[2]), 'i8', ALLOC_NORMAL);
				}
				else
					optname = 0;
				Module._sqlsetOption( this.sql, sect, optname, defaultVal );
				if( sect ) Module._free( sect );
				if( optname ) Module._free( optname );
				if( defaultVal ) Module._free( defaultVal );

			},
			eo(s) {
				console.log( "EnumOption not implemented" );
				
			},
			fo(s) {
				console.log( "FindOption not implemented" );
				
			},
			go(s) {
				console.log( "GetOption not implemented" );
			},
	        
			function(s,cb) {
				var cbi = makeFunction(cb);
				var si = s?allocate(intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				Module._sqluserFunction(si,cbi);
				Module._free( si );
			},
			procedure(s,cb) {
				var cbi = makeFunction(cb);
				var si = s?allocate(intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				Module._sqluserProcedure(si,cbi);
				Module._free( si );
				
			},
			aggregate(s,cb1,cb2) {
				var cb1i = makeFunction(cb1);
				var cb2i = makeFunction(cb2);
				var si = s?allocate(intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				Module._sqlAggregateFunction(si,cbi);
				Module._free( si );
				
			},
			onCorruption(s,cb) {
				var cbi = makeFunction(cb);
				var si = s?allocate(intArrayFromString(s), 'i8', ALLOC_NORMAL):0;
				Module._sqlsetOnCorruption(si,cbi);
				Module._free( si );
				
			},
			
			get log() {
				
			},
			get log() {
				
			},
		
			optionEditor(s) {
				console.log( "sqlite specific editor not implemented" );
			}
	        

		};
		sqliteMethods.getOption = sqliteMethods.go;
		sqliteMethods.setOption = sqliteMethods.so;
	        
		Object.defineProperties( Object.getPrototypeOf( Sqlite ), Object.getOwnPropertyDescriptors( sqliteMethods ));



		function ObjectStore( fileName, version ) {
			
		};


		const objectMethods = {
			read( buf ) {
					},
					write() {
					},
					createIndex() {
					},
					put() {
					},
					get() {
					},
					delete() {
					}
		};

		Object.defineProperties( Object.getPrototypeOf( ObjectStore ), Object.getOwnPropertyDescriptors( objectMethods ));


		Module.defineFunction = function(cb) {
			return Module.this_.callbacks.push(cb);
		};

		Module.SACK = {
			Volume : Volume,
			Sqlite : Sqlite,
			ObjectStorage : ObjectStore,
			JSON : null,
			JSON6 : null,
			JSOX : null,
			VESL : null,
		};
	) );
}


void defineFunction( cb ){
	EM_ASM( {
          return Module.this_.callbacks.push(cb);
	} );
}

#define  makeObject() EM_ASM_INT( { return Module.this_.objects .push( {} )-1; })
#define  makeArray() EM_ASM_INT( { return Module.this_.objects .push( [] )-1; })

static inline int makeString( char *string, int stringlen ) {
	int x = EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.objects.push( string )-1;
	},string, stringlen);
	return x;
}

static inline int fillArrayBuffer(char *data, int len) {
	return EM_ASM_INT( {
		var ab = new ArrayBuffer( $1 );
		var u8 = new Uint8Array(ab);
		for( var i = 0; i < $1; i++ )
			u8[i] = Module.U8HEAP[data+i];
		return Module.this_.objects .push( ab )-1;
	},data, len);
}

static inline int makeArrayBuffer(int len) {
	return EM_ASM_INT( {
		return Module.this_.objects .push( new ArrayBuffer($0) )-1;
	},len);
}

static inline int makeBigInt( char *s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $0, $1-1 );
		return Module.this_.objects .push( BigInt(string) )-1;
	},n);
}

static inline int makeDate( char *s, int n ) {
	return EM_ASM_INT( {
		const string = UTF8ToString( $0, $1 );
		return Module.this_.objects .push( new Date(string) )-1;
	},n);
}


static inline int makeNumber( int n ) {
	return EM_ASM_INT( {
		return Module.this_.objects .push( $0 )-1;
	},n);
}

static inline int makeNumberf( double n ) {
	return EM_ASM_INT( {
		return Module.this_.objects .push( $0 )-1;
	},n);
}

static inline int makeTypedArray(int ab, int type ) {
	return EM_ASM_INT( {
		var ab = Module.this_.objects[$0];
		//console.log( "This:", Module.this_, Object.getPrototypeOf( Module.this_ ) );
			switch( $1 ) {
			case 0:
				result = $0;
				break;
			case 1: // "u8"
				Module.this_.objects .push( new Uint8Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 2:// "cu8"
				Module.this_.objects .push( new Uint8ClampedArray(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 3:// "s8"
				Module.this_.objects .push( new Int8Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 4:// "u16"
				Module.this_.objects .push( new Uint16Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 5:// "s16"
				Module.this_.objects .push( new Int16Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 6:// "u32"
				Module.this_.objects .push( new Uint32Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 7:// "s32"
				Module.this_.objects .push( new Int32Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			//case 8:// "u64"
			//	result = Uint64Array::New( ab, 0, val->stringLen );
			//	break;
			//case 9:// "s64"
			//	result = Int64Array::New( ab, 0, val->stringLen );
			//	break;
			case 10:// "f32"
				Module.this_.objects .push( new Float32Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			case 11:// "f64"
				Module.this_.objects .push( new Float64Array(ab) );
				result = Module.this_.objects.length-1;
				break;
			default:
				result = 0; // undefined constant
			}
		return result;
	}, ab, type );
}


static inline void setObject( int object, int field, int value ) {
	EM_ASM_( {
		const fieldName = Module.this_.objects[$1];
		Module.this_.objects[$0][fieldName] = Module.this_.objects[$2]; 
		//console.log( "We Good?", Module.this_.objects )
	}, object, field, value);
}

static void setObjectByIndex( int object, int index, int value ) {
	EM_ASM_( {
		Module.this_.objects[$0][$1] = Module.this_.objects[$2]; 
	}, object, index, value);
}

static void setObjectByName( int object, char*field, int value ) {
	EM_ASM_( {
		const fieldName = Pointer_stringify( $1 );
		Module.this_.objects[$0][fieldName] = Module.this_.objects[$2]; 
		//console.log( "We Good?", Module.this_.objects )
	}, object, field, value);
}

#define SET(o,f,v) setObjectByName( o,f,v )
#define SETN(o,f,v) setObjectByIndex( o,f,v )

struct os_Vol {
	struct sack_vfs_fs_volume *vol;
	char *mountName;
	char *fileName;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;

	LOGICAL volNative;
};

struct os_File {
	struct os_Vol *vol;
	struct sack_vfs_fs_file *file;
	FILE *cfile;
	//Local<Object> sack_vfs_volume;
	PLIST buffers; // because these are passed as physical data buffers, don't release them willy-nilly
	char* buf;
	size_t size;
};

uintptr_t Volume( char *mount, char *filename, int version, char *key1, char *key2 ) EMSCRIPTEN_KEEPALIVE;
uintptr_t Volume( char *mount, char *filename, int version, char *key1, char *key2 ) 
{
	struct os_Vol *vol = New( struct os_Vol );

	if( !mount && !filename ) {
		vol->volNative = FALSE;
		vol->fsInt = sack_get_filesystem_interface( "native" );	
		vol->fsMount = sack_get_default_mount();
	}else if( mount && !filename ) {
		vol->volNative = FALSE;
		vol->fsMount = sack_get_mounted_filesystem( mount );
		vol->fsInt = sack_get_mounted_filesystem_interface( vol->fsMount );
		vol->vol = (struct sack_vfs_fs_volume*)sack_get_mounted_filesystem_instance( vol->fsMount );
		//lprintf( "open native mount" );
	} else {
		//lprintf( "sack_vfs_volume: %s %p %p", filename, key1, key2 );
		vol->fileName = StrDup( filename );
		vol->volNative = TRUE;
		vol->vol = sack_vfs_fs_load_crypt_volume( filename, version, key1, key2 );
		//lprintf( "VOL: %p for %s %d %p %p", vol, filename, version, key1, key2 );
		if( vol->vol )
			vol->fsMount = sack_mount_filesystem( mount, vol->fsInt = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-fs" )
					, 2000, (uintptr_t)vol->vol, TRUE );
		else
			vol->fsMount = NULL;
	}
	return (uintptr_t)vol;
}

static void resetVolume( struct os_Vol *v ) {
	/* volumes that got garbage collection might still have files in them... */
	(void)v;

}


uintptr_t readAsString( char *filename )   EMSCRIPTEN_KEEPALIVE;
uintptr_t readAsString( char *filename ) {
	FILE *file = sack_fopen( 0, filename, "rb" );
	{
		size_t len = sack_fsize( file );
		char *string = Allocate( len );
		sack_fread( string, 1, len, file );
		sack_fclose( file );
		return (uintptr_t)string;
	}
	return 0;
}

uintptr_t File( uintptr_t psvVol, char *name )  EMSCRIPTEN_KEEPALIVE;
uintptr_t File( uintptr_t psvVol, char *name ) {
	struct os_File *file = New( struct os_File);
	file->vol = (struct os_Vol*)psvVol;
	file->buf = NULL;
	
	if( file->vol->volNative ){
		if( !file->vol->vol ) return 0;
		file->file = sack_vfs_fs_openfile( file->vol->vol, name );
	}else {
		file->cfile = sack_fopenEx( 0, name, "rb+", file->vol->fsMount );
		if( !file->cfile )
			file->cfile = sack_fopenEx( 0, name, "wb", file->vol->fsMount );
	}
	return (uintptr_t)file;
}

void closeFile( uintptr_t psvFile )  EMSCRIPTEN_KEEPALIVE;
void closeFile( uintptr_t psvFile ){
	struct os_File *file = (struct os_File *)psvFile;
	lprintf( "Closing file..." );
	if( file->buf )
		Release( file->buf );
	lprintf( "calling real function %p %p", file, file->file );
	if( file->vol->volNative )
		sack_vfs_fs_close( file->file );
	else
		sack_fclose( file->cfile );
	lprintf( "calling reset..." );
	resetVolume( file->vol );
	lprintf( "Release file" );
	Release( file );
	lprintf( "and done?");

}

size_t tellFile( uintptr_t psvFile )  EMSCRIPTEN_KEEPALIVE;
size_t tellFile( uintptr_t psvFile ) {
	struct os_File *file = (struct os_File *)psvFile;
	if( file->vol->volNative )
		return sack_vfs_fs_tell( file->file );
	else
		return sack_ftell( file->cfile );
}

void truncateFile(uintptr_t psvFile)  EMSCRIPTEN_KEEPALIVE;
void truncateFile(uintptr_t psvFile) {
	struct os_File *file = (struct os_File *)psvFile;
	if( file->vol->volNative )
		sack_vfs_fs_truncate( file->file ); // sets end of file mark to current position.
	else
		sack_ftruncate( file->cfile );
}

void seekFile( uintptr_t psvFile, int pos, int whence )  EMSCRIPTEN_KEEPALIVE;
void seekFile( uintptr_t psvFile, int pos, int whence ) {
	struct os_File *file = (struct os_File *)psvFile;
	if( file->vol->volNative )
		sack_vfs_fs_seek( file->file, pos, whence );
	else
		sack_fseek( file->cfile, pos, whence );
}

static struct SqlObject* createSqlObject( const char *dsn );
			
uintptr_t openVolDb( struct os_Vol *vol, char *filename ) EMSCRIPTEN_KEEPALIVE;
uintptr_t openVolDb( struct os_Vol *vol, char *filename ) {
			char dbName[256];
 			snprintf( dbName, 256, "$sack@%s$%s", vol->mountName, filename );
			uintptr_t o = (uintptr_t)createSqlObject( dbName );
			return o;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//   SQLite interface.
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


struct SqlObjectUserFunction {
	struct SqlObject *sql;
	int cb;
	int cb2;
};

struct SqlStmtObject {
	struct SqlObject *sql;
	PDATALIST values;
};

struct SqlObject  {

	PODBC odbc;
	int optionInitialized;
	//int columns;
	//CTEXTSTR *result;
	//size_t *resultLens;
	//CTEXTSTR *fields;
	int onCorruption;
	int _this;
	//Persistent<Object> volume;

	//PTHREAD thread;
	//uv_async_t async; // keep this instance around for as long as we might need to do the periodic callback
	PLIST userFunctions;
	PLINKQUEUE messages;

	//static void Init( Local<Object> exports );
	//SqlObject( const char *dsn );
};


struct OptionTreeObject {
	POPTION_TREE_NODE node;
	PODBC odbc;
	int o;
};


struct userMessage{
	int mode;
	struct sqlite3_context*onwhat;
	int argc;
	struct sqlite3_value**argv;
	int done;
	PTHREAD waiter;
};


struct SqlObject* createSqlObject( const char *dsn )
{
	struct SqlObject *sql = New( struct SqlObject);
	sql->messages = NULL;
	sql->userFunctions = NULL;
	//sql->thread = NULL;
	//memset( &async, 0, sizeof( async ) );
	sql->odbc = ConnectToDatabase( dsn );
	SetSQLThreadProtect( sql->odbc, FALSE );
	//SetSQLAutoClose( odbc, TRUE );
	sql->optionInitialized = FALSE;
	return sql;
}


//-----------------------------------------------------------

struct SqlObject * SqlObjectNew( char *arg ) {
	struct SqlObject* obj;
	if( arg ) {
		obj = createSqlObject( arg );
	}
	else {
		obj = createSqlObject( ":memory:" );
	}
	return obj;
}

//-----------------------------------------------------------

int IsTextAnyNumber( CTEXTSTR text, double *fNumber, int64_t *iNumber )
{
	CTEXTSTR pCurrentCharacter;
	int decimal_count, s, begin = TRUE, digits;
	// remember where we started...
	// if the first segment is indirect, collect it and only it
	// as the number... making indirects within a number what then?

	decimal_count = 0;
	s = 0;
	digits = 0;
	pCurrentCharacter = text;
	while( pCurrentCharacter[0] )
	{
		pCurrentCharacter = text;
		while( pCurrentCharacter && *pCurrentCharacter )
		{
			if( *pCurrentCharacter == '.' )
			{
				if( !decimal_count && digits )
					decimal_count++;
				else
					break;
			}
			else if( ((*pCurrentCharacter) == '-') && begin)
			{
				s++;
			}
			else if( ((*pCurrentCharacter) < '0') || ((*pCurrentCharacter) > '9') )
			{
				break;
			}
			else {
				digits++;
				if( !decimal_count && digits > 11 )
					return 0;
			}
			begin = FALSE;
			pCurrentCharacter++;
		}
		// invalid character - stop, we're to abort.
		if( *pCurrentCharacter )
			break;

		//while( pText );
	}

	if( *pCurrentCharacter || ( decimal_count > 1 ) || !digits )
	{
		// didn't collect enough meaningful info to be a number..
		// or information in this state is
		return 0;
	}
	if( decimal_count == 1 )
	{
		if( fNumber )
			(*fNumber) = FloatCreateFromText( text, NULL );
		// return specifically it's a floating point number
		return 2;
	}
	if( iNumber )
		(*iNumber) = IntCreateFromText( text );
	// return yes, and it's an int number
	return 1;
}
//-----------------------------------------------------------
void sqlCloseDb( struct SqlObject *sql ) {
	CloseDatabase( sql->odbc );
}

void sqlAutoTransact( struct SqlObject *sql, LOGICAL b ) {
	SetSQLAutoTransact( sql->odbc, b );
}
//-----------------------------------------------------------
void aqlTransact( struct SqlObject *sql ) {
	SQLBeginTransact( sql->odbc );
}
//-----------------------------------------------------------
void sqlCommit( struct SqlObject *sql ) {
	SQLCommit( sql->odbc );
}
//-----------------------------------------------------------

char * sqlescape( struct SqlObject *sql, char *tmp, size_t tmplen ) {
	if( tmp ) {
		size_t resultlen;
		char *out = EscapeSQLBinaryExx(sql?sql->odbc:NULL, tmp, tmplen, &resultlen, FALSE DBG_SRC );
		return out;
	}
	return tmp;
}

char * sqlunescape(  struct SqlObject *sql, char *tmp, size_t tmplen ) {
	if( tmp ) {
		size_t outlen;
		char *out = RevertEscapeBinary( tmp, &outlen );		
		return out;
	}
	return tmp;
}
//-----------------------------------------------------------

void SqlStmtObjectSet( struct SqlStmtObject *stmt, int col, int valType, int isDbl, int intVal, double dVal, char *sVal ) {
	struct jsox_value_container val;
	memset( &val, 0, sizeof( val ) );
	int arg = 1;
	val.value_type = valType;
	switch( valType ) {
	case JSOX_VALUE_NUMBER:
		if( !isDbl ) {
			val.result_n = intVal;
			val.float_result = 0;
			SetDataItem( &stmt->values, col, &val );
		} else  {
			val.result_d = dVal;
			val.float_result = 1;
			SetDataItem( &stmt->values, col, &val );
		}
		break;
	default:
		throwError( "Type is not supported yet; conversion ");
		return;
	}
}

static void PushValue( PDATALIST *pdlParams, char *name, size_t nameLen, int valType, int isDbl, int intVal, double dVal, char *sVal, size_t sValLen ) EMSCRIPTEN_KEEPALIVE;
static void PushValue( PDATALIST *pdlParams, char *name, size_t nameLen, int valType, int isDbl, int intVal, double dVal, char *sVal, size_t sValLen ) 
{
	struct jsox_value_container val;
	if( name ) {
		val.name = DupCStrLen( name, val.nameLen = nameLen );
	}
	else {
		val.name = NULL;
		val.nameLen = 0;
	}
	switch( val.value_type = valType ) {
	case JSOX_VALUE_NULL:
		AddDataItem( pdlParams, &val );
		break;
	case JSOX_VALUE_STRING:
		val.string = DupCStrLen( sVal, val.stringLen = sValLen );
		AddDataItem( pdlParams, &val );
		break;
	case JSOX_VALUE_NUMBER:
		if( isDbl ){
			val.result_d = dVal;
			val.float_result = 1;
		}
		else {
			val.result_n = intVal;
			val.float_result = 0;
		}
		AddDataItem( pdlParams, &val );
		break;
	case JSOX_VALUE_TYPED_ARRAY:
		val.string = (char*)sVal;
		val.stringLen = sValLen;
		AddDataItem( pdlParams, &val );
		break;
	default:
		lprintf( "Unsupported TYPE" );
		break;
	}
}

int sqlquery( struct SqlObject *sql, char *statement, size_t statementlen, PDATALIST *pdlParams ) {

		PDATALIST pdlRecord = NULL;
		INDEX idx = 0;
		int items;
		struct jsox_value_container * jsval;

		if( !SQLRecordQuery_js( sql->odbc, statement, statementlen, &pdlRecord, pdlParams[0] DBG_SRC ) ) {
			const char *error;
			FetchSQLError( sql->odbc, &error );
			throwError( error );
			dropValueList( pdlParams );
			return JS_VALUE_UNDEFINED;
		}

		DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
			if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;
		}
		items = (int)idx;

		//&sql->columns, &sql->result, &sql->resultLens, &sql->fields
		if( pdlRecord )
		{
			int usedFields = 0;
			int maxDepth = 0;
			struct fieldTypes {
				const char *name;
				int used;
				int first;
				int array;
			} *fields = NewArray( struct fieldTypes, items ) ;
			int usedTables = 0;
			struct tables {
				const char *table;
				const char *alias;
				int container;
			}  *tables = NewArray( struct tables, items + 1);
			struct colMap {
				int depth;
				int col;
				const char *table;
				const char *alias;
				int container;
				struct tables *t;
			}  *colMap = NewArray( struct colMap, items );
			tables[usedTables].table = NULL;
			tables[usedTables].alias = NULL;
			usedTables++;

			DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
				int m;
				if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;

				for( m = 0; m < usedFields; m++ ) {
					if( StrCaseCmp( fields[m].name, jsval->name ) == 0 ) {
						colMap[idx].col = m;
						colMap[idx].depth = fields[m].used;
						if( colMap[idx].depth > maxDepth )
							maxDepth = colMap[idx].depth+1;
						colMap[idx].alias = PSSQL_GetColumnTableAliasName( sql->odbc, (int)idx );
						int table;
						for( table = 0; table < usedTables; table++ ) {
							if( StrCmp( tables[table].alias, colMap[idx].alias ) == 0 ) {
								colMap[idx].t = tables + table;
								break;
							}
						}
						if( table == usedTables ) {
							tables[table].table = colMap[idx].table;
							tables[table].alias = colMap[idx].alias;
							colMap[idx].t = tables + table;
							usedTables++;
						}
						fields[m].used++;
						break;
					}
				}
				if( m == usedFields ) {
					colMap[idx].col = m;
					colMap[idx].depth = 0;
					colMap[idx].table = PSSQL_GetColumnTableName( sql->odbc, (int)idx );
					colMap[idx].alias = PSSQL_GetColumnTableAliasName( sql->odbc, (int)idx );
					if( colMap[idx].table && colMap[idx].alias ) {
						int table;
						for( table = 0; table < usedTables; table++ ) {
							if( StrCmp( tables[table].alias, colMap[idx].alias ) == 0 ) {
								colMap[idx].t = tables + table;
								break;
							}
						}
						if( table == usedTables ) {
							tables[table].table = colMap[idx].table;
							tables[table].alias = colMap[idx].alias;
							colMap[idx].t = tables + table;
							usedTables++;
						}
					} else
						colMap[idx].t = tables;
					fields[usedFields].first = (int)idx;
					fields[usedFields].name = jsval->name;// sql->fields[idx];
					fields[usedFields].used = 1;
					usedFields++;
				}
			}
			if( usedTables > 1 )
				for( int m = 0; m < usedFields; m++ ) {
					for( int t = 1; t < usedTables; t++ ) {
						if( StrCaseCmp( fields[m].name, tables[t].alias ) == 0 ) {
							PVARTEXT pvtSafe = VarTextCreate();
							vtprintf( pvtSafe, "%s : %s", TranslateText( "Column name overlaps table alias" ), tables[t].alias );
							throwError( GetText( VarTextPeek( pvtSafe ) ) );
							VarTextDestroy( &pvtSafe );
							DeleteDataList( &pdlParams );
							return JS_VALUE_UNDEFINED;
						}
					}
				}
			int records = makeArray();
			int record;
			if( pdlRecord ) {
				int row = 0;
				do {
					int val;
					tables[0].container = record = makeObject();
					if( usedTables > 1 && maxDepth > 1 )
						for( int n = 1; n < usedTables; n++ ) {
							tables[n].container = makeObject();
							SET( record, tables[n].alias, tables[n].container );
						}
					else
						for( int n = 0; n < usedTables; n++ )
							tables[n].container = record;

					DATA_FORALL( pdlRecord, idx, struct jsox_value_container *, jsval ) {
						if( jsval->value_type == JSOX_VALUE_UNDEFINED ) break;

						int container = colMap[idx].t->container;
						if( fields[colMap[idx].col].used > 1 ) {
							if( fields[colMap[idx].col].first == idx ) {
								if( !jsval->name )
									lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s", statement );
								else
									SET( record, jsval->name
									           , fields[colMap[idx].col].array = makeArray()
									           );
							}
						}

						switch( jsval->value_type ) {
						default:
							lprintf( "Unhandled value result type:%d", jsval->value_type );
							break;
						case JSOX_VALUE_DATE:
							val = makeDate( jsval->string, jsval->stringLen );
							break;
						case JSOX_VALUE_TRUE:
							val = JS_VALUE_TRUE;
							break;
						case JSOX_VALUE_FALSE:
							val = JS_VALUE_FALSE;
							break;
						case JSOX_VALUE_NULL:
							val = JS_VALUE_NULL;
							break;
						case JSOX_VALUE_NUMBER:
							if( jsval->float_result ) {
								val = makeNumberf( jsval->result_d );
							}
							else {
								val = makeNumber( jsval->result_n );
							}
							break;
						case JSOX_VALUE_STRING:
							if( !jsval->string )
								val = JS_VALUE_NULL;
							else
								val = makeString( jsval->string, jsval->stringLen);
							break;
						case JSOX_VALUE_TYPED_ARRAY:
							//lprintf( "Should result with a binary thing" );
							{
							int ab;ab = fillArrayBuffer( jsval->string, jsval->stringLen );
							
							val = ab;
							}
							break;
						}

						if( fields[colMap[idx].col].used == 1 ){
							if( !jsval->name )
								lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s",  statement  );
							else
								SET( container, jsval->name, val );
						}
						else if( usedTables > 1 || ( fields[colMap[idx].col].used > 1 ) ) {
							if( fields[colMap[idx].col].used > 1 ) {
								if( !jsval->name )
									lprintf( "FAILED TO GET RESULTING NAME FROM SQL QUERY: %s",  statement  );
								else
									SET( colMap[idx].t->container, jsval->name, val );
								if( colMap[idx].alias )
									SET( fields[colMap[idx].col].array, colMap[idx].alias, val );
								SETN( fields[colMap[idx].col].array, colMap[idx].depth, val );
							}
						}
					}
					SETN( records, row++, record );
				} while( FetchSQLRecordJS( sql->odbc, &pdlRecord ) );
			}
			Deallocate( struct fieldTypes*, fields );
			Deallocate( struct tables*, tables );
			Deallocate( struct colMap*, colMap );

			SQLEndQuery( sql->odbc );
			return records;
		}
		else
		{
			SQLEndQuery( sql->odbc );
			return makeArray();
		}
		DeleteDataList( &pdlParams );

}

//-----------------------------------------------------------

void deletesqlSqlObject( struct SqlObject *sql ) {
	#if 0
	INDEX idx;
	struct SqlObjectUserFunction *data;
	LIST_FORALL( userFunctions, idx, struct SqlObjectUserFunction*, data ) {
		data->cb.Reset();
	}
	if( thread )
	{
		struct userMessage msg;
		msg.mode = 0;
		msg.onwhat = NULL;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &messages, &msg );
		uv_async_send( &async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}
	}
	#endif
	CloseDatabase( sql->odbc );
}

//-----------------------------------------------------------

struct OptionTreeObject* makeOptionTreeObject( PODBC odbc ) {
	struct OptionTreeObject* oto = New( struct OptionTreeObject );
	oto->odbc = odbc;
	oto->o = EM_ASM_INT( { return Module.this_.objects .push( new Module.SACK.Sqlite.optionTreeNode($0) )-1; }, oto );
	return oto;
}

struct OptionTreeObject * sqlgetOptionNode( struct SqlObject *sqlParent, char *optionPath ) {

	if( !sqlParent->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->odbc );
		sqlParent->optionInitialized = TRUE;
	}

	struct OptionTreeObject *oto = makeOptionTreeObject( sqlParent->odbc );
	oto->node = GetOptionIndexExx( sqlParent->odbc, NULL, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
	//lprintf( "SO Get %p ", sqlParent->odbc );
	return oto;
}


struct OptionTreeObject* OptionTreeObjectgetOptionNode( struct OptionTreeObject*parent, char *optionPath ) {
	struct OptionTreeObject *oto = makeOptionTreeObject( parent->odbc );
	oto->node =  GetOptionIndexExx( parent->odbc, parent->node, optionPath, NULL, NULL, NULL, TRUE, TRUE DBG_SRC );
	return oto;
}

int sqlfindOptionNode( struct SqlObject *sqlParent, char *optionPath ) {
	if( !sqlParent->optionInitialized ) {
		SetOptionDatabaseOption( sqlParent->odbc );
		sqlParent->optionInitialized = TRUE;
	}
	POPTION_TREE_NODE newNode = GetOptionIndexExx( sqlParent->odbc, NULL, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );
	struct OptionTreeObject *oto = NULL;

	if( newNode ) {
		oto = makeOptionTreeObject( sqlParent->odbc );
		oto->node = newNode;		
	}
	if( newNode ) {
		return oto->o;
	}
	return JS_VALUE_NULL;
}


int OptionTreeObjectfindOptionNode( struct OptionTreeObject *parent, char *optionPath ) {

	POPTION_TREE_NODE newOption;

	newOption = GetOptionIndexExx( parent->odbc, parent->node, optionPath, NULL, NULL, NULL, FALSE, TRUE DBG_SRC );
	struct OptionTreeObject *oto = NULL;
	if( newOption ) {
		oto = makeOptionTreeObject(parent->odbc);
		oto->node = newOption;
	}
	if( newOption )
		return oto->o;
	return JS_VALUE_NULL;
}

struct enumArgs {
	int cb;
	PODBC odbc;
};

int callFunction( int cb, int o, char *string ) {
	return EM_ASM_INT( {
		Module.this_.objects[$0](Module.this_.objects[$1], UTF8ToString( $2 ) );
	},cb, o, string);
}

int CPROC invokeCallback( uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags ) {
	struct enumArgs *args = (struct enumArgs*)psv;
	struct OptionTreeObject *oto = makeOptionTreeObject( args->odbc );
	oto->node = ID;

	return callFunction( args->cb, oto->o, name );
}


void enumOptionNodes( struct SqlObject *sqlParent, int cb ) EMSCRIPTEN_KEEPALIVE;
void enumOptionNodes( struct SqlObject *sqlParent, int cb ) 
{
	struct enumArgs callbackArgs;

	LOGICAL dropODBC;
	if( sqlParent ) {
		if( !sqlParent->optionInitialized ) {
			SetOptionDatabaseOption( sqlParent->odbc );
			sqlParent->optionInitialized = TRUE;
		}
		callbackArgs.odbc = sqlParent;
		dropODBC = FALSE;
	}
	else {
		callbackArgs.odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
		dropODBC = TRUE;
	}

	callbackArgs.cb = cb;

	EnumOptionsEx( callbackArgs.odbc, NULL, invokeCallback, (uintptr_t)&callbackArgs );
	if( dropODBC )
		DropOptionODBC( callbackArgs.odbc );
}

/*
void sqlenumOptionNodes( const v8::FunctionCallbackInfo<Value>& args ) {
	SqlObject *sqlParent = ObjectWrap::Unwrap<SqlObject>( args.This() );
	::enumOptionNodes( args, sqlParent );
}
void sqlenumOptionNodesInternal( const v8::FunctionCallbackInfo<Value>& args ) {
	::enumOptionNodes( args, NULL );
}
*/
void OptionTreeObjectenumOptionNodes( struct OptionTreeObject *oto, int cb ) {
	struct enumArgs callbackArgs;

	callbackArgs.odbc = oto->odbc;
	callbackArgs.cb = cb;

	EnumOptionsEx( oto->odbc, oto->node, invokeCallback, (uintptr_t)&callbackArgs );
}

int OptionTreeObjectreadOptionNode( struct OptionTreeObject *oto ) {
	char *buffer;
	size_t buflen;
	int res = (int)GetOptionStringValueEx( oto->odbc, oto->node, &buffer, &buflen DBG_SRC );
	if( !buffer || res < 0 )
		return JS_VALUE_NULL;
	return makeString( buffer, strlen(buffer) );
}

void OptionTreeObjectwriteOptionNode( struct OptionTreeObject *oto, char *tmp ) {
	SetOptionStringValueEx( oto->odbc, oto->node, tmp );
}


static int sqlgetOption( struct SqlObject *sql, const char *sect, const char *optname, const char *defaultVal ) {
	TEXTCHAR *readbuf = NewArray( TEXTCHAR, 1024 );
	PODBC use_odbc = NULL;
	if( !sql ) {
		// use_odbc = NULL;
	} else  {
		if( !sql->optionInitialized ) {
			SetOptionDatabaseOption( sql->odbc );
			sql->optionInitialized = TRUE;
		}
		use_odbc = sql->odbc;
	}
	SACK_GetPrivateProfileStringExxx( use_odbc
		, sect
		, optname
		, defaultVal
		, readbuf
		, 1024
		, NULL
		, TRUE
		DBG_SRC
		);

	Deallocate( char*, optname );
	Deallocate( char*, sect );
	Deallocate( char*, defaultVal );
	return readbuf;
}

//-----------------------------------------------------------
//SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateOptionStringEx )(PODBC odbc, CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL flush);

static void sqlsetOption( struct SqlObject *sql, char *sect, char *optname, char *defaultVal, int internal ) {

	TEXTCHAR readbuf[1024];

	PODBC use_odbc = NULL;
	if( internal ) {
	} else 
	{
		use_odbc = sql->odbc;
	}
	if( ( sect && sect[0] == '/' ) ) {
			SACK_WritePrivateOptionStringEx( use_odbc
			, NULL
			, optname
			, defaultVal
			, sect, FALSE );
	} 
	else
		SACK_WriteOptionString( use_odbc
			, sect
			, optname
			, defaultVal
		);


	Deallocate( char*, optname );
	Deallocate( char*, sect );
	Deallocate( char*, defaultVal );

}

//-----------------------------------------------------------

LOGICAL sqlmakeTable( struct SqlObject *sql, const char *tableCommand ) {
	if( tableCommand ) {
		PTABLE table = GetFieldsInSQLEx( tableCommand, FALSE DBG_SRC );
		if( CheckODBCTable( sql->odbc, table, CTO_MERGE ) )
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

static void callUserFunction( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv );
static void callAggStep( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv );
static void callAggFinal( struct sqlite3_context*onwhat );


static void releaseBuffer( void *buffer ) {
	Deallocate( void*, buffer );
}

void callUserFunction( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv ) {
	struct SqlObjectUserFunction *userData = (struct SqlObjectUserFunction*)PSSQL_GetSqliteFunctionData( onwhat );
#if 0
	if( userData->sql->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = 1;
		msg.onwhat = onwhat;
		msg.argc = argc;
		msg.argv = argv;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}
		return;
	}
#endif
	//Local<Value> *args;
	if( argc > 0 ) {
		int n;
		int *args;
		char *text;
		int textLen;
		args = NewArray( int, argc );//new Local<Value>[argc];
		for( n = 0; n < argc; n++ ) {
			int type = PSSQL_GetSqliteValueType( argv[n] );
			switch( type ) {
			case 1:
			{
				int64_t val;
				PSSQL_GetSqliteValueInt64( argv[n], &val );
				if( val & 0xFFFFFFFF00000000ULL )
					args[n] = makeNumberf( (double)val );
				else
					args[n] = makeNumber( (int32_t)val );
				break;
			}
			case 2:
			{
				double val;
				PSSQL_GetSqliteValueDouble( argv[n], &val );
				args[n] = makeNumber( val );
				break;
			}
			case 4:
			{
				const char *data;
				char *_data;
				int len;
				PSSQL_GetSqliteValueBlob( argv[n], &data, &len );
				_data = NewArray( char, len );
				memcpy( _data, data, len );
				int ab = fillArrayBuffer( _data, len );
				break;
			}
			case 5:
				args[n] = JS_VALUE_NULL;
				break;
			case 3:
			default:
				PSSQL_GetSqliteValueText( argv[n], (const char**)&text, &textLen );
				args[n] = makeString( text, textLen );
			}
		}
	} else {
		//args = JS_VALUE_NULL;
	}
	//Local<Function> cb = Local<Function>::New( userData->isolate, userData->cb );
	int str = callFunction( userData->cb, argv[0], argv[1] );
#if 0	
	Local<Value> str = cb->Call( userData->isolate->GetCurrentContext(), userData->sql->handle(), argc, args ).ToLocalChecked();
#endif
	//String::Utf8Value result( USE_ISOLATE( userData->isolate ) str->ToString( userData->isolate->GetCurrentContext() ).ToLocalChecked() );
	#if 0
	int type;
	if( ( ( type = 1 ), str->IsArrayBuffer() ) || ( ( type = 2 ), str->IsUint8Array() ) ) {
		uint8_t *buf = NULL;
		size_t length;
		if( type == 1 ) {
			Local<ArrayBuffer> myarr = str.As<ArrayBuffer>();
			buf = (uint8_t*)myarr->GetContents().Data();
			length = myarr->ByteLength();
		} else if( type == 2 ) {
			Local<Uint8Array> _myarr = str.As<Uint8Array>();
			Local<ArrayBuffer> buffer = _myarr->Buffer();
			buf = (uint8_t*)buffer->GetContents().Data();
			length = buffer->ByteLength();
		}
		if( buf )
			PSSQL_ResultSqliteBlob( onwhat, (const char *)buf, (int)length, NULL );
	} else if( str->IsNumber() ) {
		if( str->IsInt32() )
			PSSQL_ResultSqliteInt( onwhat, (int)str->IntegerValue( userData->isolate->GetCurrentContext() ).FromMaybe(0) );
		else
			PSSQL_ResultSqliteDouble( onwhat, str->NumberValue( userData->isolate->GetCurrentContext() ).FromMaybe( 0 ) );
	} else if( str->IsString() )
		PSSQL_ResultSqliteText( onwhat, DupCStrLen( *result, result.length() ), result.length(), releaseBuffer );
	else
		lprintf( "unhandled result type (object? array? function?)" );
	if( argc > 0 ) {
		delete[] args;
	}
	#endif
}


static void destroyUserData( void *vpUserData ) {
	struct SqlObjectUserFunction *userData = (struct SqlObjectUserFunction *)vpUserData;
	Release( userData );
}

void sqluserFunction( struct SqlObject *sql , char *name, int cb ) {
	struct SqlObjectUserFunction *userData = New( struct SqlObjectUserFunction );
	userData->cb = cb;
	userData->sql = sql;
	PSSQL_AddSqliteFunction( sql->odbc, name, callUserFunction, destroyUserData, -1, userData );
}

void sqluserProcedure( struct SqlObject *sql, char *name, int cb ) {
	struct SqlObjectUserFunction *userData = New( struct SqlObjectUserFunction );
	userData->cb = cb;
	userData->sql = sql;
	PSSQL_AddSqliteProcedure( sql->odbc, name, callUserFunction, destroyUserData, -1, userData );
}

void callAggStep( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv ) {
	struct SqlObjectUserFunction *userData = ( struct SqlObjectUserFunction* )PSSQL_GetSqliteFunctionData( onwhat );
#if 0
	if( userData->sql->thread != MakeThread() ) {
		struct userMessage msg;
		msg.mode = 2;
		msg.onwhat = onwhat;
		msg.argc = argc;
		msg.argv = argv;
		msg.done = 0;
		msg.waiter = MakeThread();
		//EnqueLink( &userData->sql->messages, &msg );
		//uv_async_send( &userData->sql->async );

		//while( !msg.done ) 
		{
			WakeableSleep( SLEEP_FOREVER );
		}
		return;
	}
#endif
	//Local<Value> *args;
	if( argc > 0 ) {
		int n;
		int args;
		char *text;
		int textLen;
		args = makeArray();//NewArray( new Local<Value>[argc];
		for( n = 0; n < argc; n++ ) {
			int type = PSSQL_GetSqliteValueType( argv[n] );
			switch( type ) {
			case 1:
			{
				int64_t val;
				PSSQL_GetSqliteValueInt64( argv[n], &val );
				if( val & 0xFFFFFFFF00000000ULL )
					SETN( args, n, makeNumberf( (double)val ) );
				else
					SETN( args, n, makeNumber(  (int32_t)val ) );
				break;
			}
			case 2:
			{
				double val;
				PSSQL_GetSqliteValueDouble( argv[n], &val );
				SETN( args,n, makeNumber( val ) );
				break;
			}
			case 4:
			{
				const char *data;
				char *_data;
				int len;
				PSSQL_GetSqliteValueBlob( argv[n], &data, &len );
				SETN( args, n, fillArrayBuffer( data, len ) );
				break;
			}
			case 5:
				SETN( args,n, JS_VALUE_NULL );
				break;
			case 3:
			default:
				PSSQL_GetSqliteValueText( argv[n], (const char**)&text, &textLen );
				SETN( args, n, makeString( text, textLen ) );
			}
		}
	} else {
		//args = JS_VALUE_NULL;
	}

#if 0	
	Local<Function> cb = Local<Function>::New( userData->isolate, userData->cb );
	cb->Call( userData->isolate->GetCurrentContext(), userData->sql->handle(), argc, args );
	if( argc > 0 ) {
		delete[] args;
	}
	#endif
}


static int callSomeFunction( int cb, int sql ) {
	return EM_ASM_INT( { 
		var r= Module.this_.objects[$0].call(Module.this_.objects[$1]); 
		var si = s?allocate(sa=intArrayFromString(s), 'i8', ALLOC_NORMAL):0;

/*
		if( ( ( type = 1 ), r.IsArrayBuffer() ) || ( ( type = 2 ), r.IsUint8Array() ) ) {
			uint8_t *buf = NULL;
			size_t length;
			if( type == 1 ) {
				Local<ArrayBuffer> myarr = str.As<ArrayBuffer>();
				buf = (uint8_t*)myarr->GetContents().Data();
				length = myarr->ByteLength();
			} else if( type == 2 ) {
				Local<Uint8Array> _myarr = str.As<Uint8Array>();
				Local<ArrayBuffer> buffer = _myarr->Buffer();
				buf = (uint8_t*)buffer->GetContents().Data();
				length = buffer->ByteLength();
			}
			if( buf )
				PSSQL_ResultSqliteBlob( onwhat, (const char *)buf, (int)length, releaseBuffer );
		} else if( str->IsNumber() ) {
			if( str->IsInt32() )
				PSSQL_ResultSqliteInt( onwhat, (int)str->IntegerValue( userData->isolate->GetCurrentContext() ).FromMaybe( 0 ) );
			else
				PSSQL_ResultSqliteDouble( onwhat, str->NumberValue( userData->isolate->GetCurrentContext() ).FromMaybe(0) );
		} else if( typeof r === "string" ) {
			String::Utf8Value result( USE_ISOLATE( userData->isolate) str->ToString( userData->isolate->GetCurrentContext() ).ToLocalChecked() );
			PSSQL_ResultSqliteText( onwhat, DupCStrLen( *result, result.length() ), result.length(), releaseBuffer );
		}
		else
			lprintf( "unhandled result type (object? array? function?)" );
*/

		return si;
	 }, cb, sql );
}

void callAggFinal( struct sqlite3_context*onwhat ) {
	struct SqlObjectUserFunction *userData = ( struct SqlObjectUserFunction* )PSSQL_GetSqliteFunctionData( onwhat );
	#if 0
	{
		struct userMessage msg;
		msg.mode = 3;
		msg.onwhat = onwhat;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );

		while( !msg.done ) {
			WakeableSleep( SLEEP_FOREVER );
		}

		return;
	} 
	#endif
	#if 0
	else {
	if( userData->sql->thread != MakeThread() ) 
		struct userMessage msg;
		msg.mode = 3;
		msg.onwhat = NULL;
		msg.done = 0;
		msg.waiter = MakeThread();
		EnqueLink( &userData->sql->messages, &msg );
		uv_async_send( &userData->sql->async );
	}
#endif
	char *str = callSomeFunction( userData->cb2, userData->sql->_this );
}

int sqlgetLogging(struct  SqlObject *sql  ) {
	return FALSE;
}

void sqlsetLogging( struct SqlObject *sql, int b ) {
		if( b )
			SetSQLLoggingDisable( sql->odbc, FALSE );
		else
			SetSQLLoggingDisable( sql->odbc, TRUE );
}


const char * sqlerror( struct SqlObject *sql ) {
	const char *error;
	FetchSQLError( sql->odbc, &error );
	return error;
}

static void handleCorruption( uintptr_t psv, PODBC odbc ) {
	struct SqlObject *sql = (struct SqlObject*)psv;
	//Local<Function> cb = Local<Function>::New( isolate, sql->onCorruption.Get( isolate ) );
	//cb->Call( isolate->GetCurrentContext(), sql->_this.Get( isolate ), 0, 0 );
}


void sqlsetOnCorruption( struct SqlObject *sql, int cb ) {
	sql->onCorruption = cb;
	SetSQLCorruptionHandler( sql->odbc, handleCorruption, (uintptr_t)sql );
}

void sqlaggregateFunction( struct SqlObject *sql, char *name, int cb1, int cb2 ) {
	struct SqlObjectUserFunction *userData = New( struct SqlObjectUserFunction );
	userData->cb = cb1;
	userData->cb2 = cb2;
	userData->sql = sql;
	PSSQL_AddSqliteAggregate( sql->odbc, *name, callAggStep, callAggFinal, destroyUserData, -1, userData );
#if 0
	if( !sql->thread ) {
		//sql->thread = MakeThread();
		//uv_async_init( uv_default_loop(), &sql->async, sqlUserAsyncMsg );
		sql->async.data = sql;
	}
#endif
}

