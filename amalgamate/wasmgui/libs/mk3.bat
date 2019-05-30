
set CFLAGS=-D__NO_OPTIONS__ -D__NO_ODBC__ -D__WASM__ -D__LINUX__ -DMAKE_RCOORD_SINGLE -D__STATIC_GLOBALS__ ^
   -D__MANUAL_PRELOAD__ -DUSE_SQLITE
: -D__NO_MMAP__
: -D__NO_OPTIONS__ 

set CFLAGS=%CFLAGS% -I../../../src/contrib/sqlite/3.27.1-TableAlias
set CFLAGS=%CFLAGS% -std=gnu99
set CFLAGS=%CFLAGS%  -s ASSERTIONS=1 

set CFLAGS=%CFLAGS% -DSUPPORT_LOG_ALLOCATE -DDEFAULT_OUTPUT_STDERR
set CFLAGS=%CFLAGS% -Wno-parentheses -Wno-null-dereference
set CFLAGS=%CFLAGS% -Wno-address-of-packed-member

del libsack.core* libbag.image.* libbag.render* libbag.psi*


call emcc -s WASM=1 -s MAIN_MODULE=1 -g -D_DEBUG %CFLAGS% sack_ucb_full.c -L. -lgenx.wasm -lsexpat.wasm
rename a.out.js libsack.core.js
rename a.out.wasm libsack.core.wasm.so
rename a.out.wast libsack.core.wast
rename a.out.wasm.map libsack.core.wasm.map

call emcc -s WASM=1 -s SIDE_MODULE=1 -g -D_DEBUG %CFLAGS% imglib_puregl2.lo -L. -lsack.core.wasm -lfreetype.wasm -lpng.wasm -ljpeg9.wasm -lz.wasm
rename a.out.js libbag.image.js
rename a.out.wasm libbag.image.wasm.so
rename a.out.wast libbag.image.wast
rename a.out.wasm.map libbag.image.wasm.map

call emcc -s WASM=1 -s SIDE_MODULE=1 -g -D_DEBUG %CFLAGS% vidlib_puregl2.lo -L. -lsack.core -lbag.image.wasm
rename a.out.js libbag.render.js
rename a.out.wasm libbag.render.wasm.so
rename a.out.wast libbag.render.wast
rename a.out.wasm.map libbag.render.wasm.map

call emcc -s WASM=1 -s SIDE_MODULE=1 -g -D_DEBUG %CFLAGS% psi.lo -L. -lsack.core.wasm
rename a.out.js libbag.psi.js
rename a.out.wasm libbag.psi.wasm.so
rename a.out.wast libbag.psi.wast
rename a.out.wasm.map libbag.psi.wasm.map

call emcc -s WASM=1 -s SIDE_MODULE=1 -g -D_DEBUG %CFLAGS% loader.c  -L. -lsack.core.wasm 
rename a.out.js loader.js
rename a.out.wasm loader.so
rename a.out.wast loader.wast
rename a.out.wasm.map loader.wasm.map

set LIBS=
set LIBS=%LIBS% jpeg9.lo
set LIBS=%LIBS% png.lo
set LIBS=%LIBS% zlib.lo
set LIBS=%LIBS% freetype.lo

:set LIBS=%LIBS% imglib.lo 
set LIBS=%LIBS% imglib_puregl2.lo 
set LIBS=%LIBS% vidlib_puregl2.lo 

set LIBS=%LIBS% genx.lo
set LIBS=%LIBS% expat.lo
set LIBS=%LIBS% psi.lo

set MORESRCS=..\..\..\src\contrib\sqlite\3.27.1-TableAlias\sqlite3.c


call emcc -s WASM=1 -g -D_DEBUG %CFLAGS% loader.c -L. -lsack.core -lbag.psi

set LIBS=
set LIBS=%LIBS% jpeg9o.lo
set LIBS=%LIBS% pngo.lo
set LIBS=%LIBS% zlibo.lo
set LIBS=%LIBS% freetypeo.lo

:set LIBS=%LIBS% imglib.lo 
set LIBS=%LIBS% imglib_puregl2o.lo 
set LIBS=%LIBS% vidlib_puregl2o.lo 

set LIBS=%LIBS% genxo.lo
set LIBS=%LIBS% expato.lo
set LIBS=%LIBS% psio.lo

call emcc -s WASM=1 -O3 %CFLAGS% loader.c -L. -lsack.core -lbag.psi



:-s EXPORT_NAME="'MyEmscriptenModule'"
:  MODULARIZE = 0
: https://github.com/emscripten-core/emscripten/blob/1.32.2/src/settings.js#L413
