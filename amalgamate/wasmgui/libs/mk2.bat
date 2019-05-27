
set CFLAGS=-D__NO_OPTIONS__ -D__NO_ODBC__ -D__WASM__ -D__LINUX__ -DMAKE_RCOORD_SINGLE -D__STATIC_GLOBALS__ ^
   -D__MANUAL_PRELOAD__ -DUSE_SQLITE
: -D__NO_MMAP__
: -D__NO_OPTIONS__ 

set CFLAGS=%CFLAGS% -I../../../src/contrib/sqlite/3.27.1-TableAlias
set CFLAGS=%CFLAGS% -std=gnu99
set CFLAGS=%CFLAGS%  -s ASSERTIONS=1 

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

set CFLAGS=%CFLAGS% -DSUPPORT_LOG_ALLOCATE -DDEFAULT_OUTPUT_STDERR
set CFLAGS=%CFLAGS% -Wno-parentheses -Wno-null-dereference
set CFLAGS=%CFLAGS%                     -Wno-address-of-packed-member



call emcc -s WASM=1 -g -D_DEBUG %CFLAGS% loader.c %LIBS% %MORESRCS%  sack_ucb_full.c -o x.js

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

call emcc -s WASM=1 -O3 %CFLAGS% loader.c %LIBS% %MORESRCS% sack_ucb_full.c -o xo.js



:-s EXPORT_NAME="'MyEmscriptenModule'"
:  MODULARIZE = 0
: https://github.com/emscripten-core/emscripten/blob/1.32.2/src/settings.js#L413
