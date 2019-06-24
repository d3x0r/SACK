:call emcc --bind --memory-init-file 0 -std=c++11 -O2 -o ./jsox-wasm.js -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -D__LINUX__  -s EXPORTED_FUNCTIONS="['_jsox_begin_parse','_jsox_parse_clear_state','_jsox_parse_add_data']" jsox.c

:--pre-js or --post-js
:call emcc --memory-init-file 0 -O2 -o ./vfs-fs-w.js -std=c++11 --bind -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.cc emscripten_bindings.cc

:call emcc --memory-init-file 0 -o ./vfs-fs-w0.js -std=c++11 --bind -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.cc emscripten_bindings.cc


set EXPORTS=
set EXPORTS='_initFS'
:set EXPORTS=%EXPORTS%,


@set CFLAGS=
@set DBG_CFLAGS=
@set CFLAGS=%CFLAGS% -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -Wno-invalid-pp-token
@set CFLAGS=%CFLAGS% -D__LINUX__ 
@set CFLAGS=%CFLAGS% -DFORCE_COLOR_MACROS
@set CFLAGS=%CFLAGS% -s WASM=1


@set DBG_CFLAGS=%DBG_CFLAGS% -D_DEBUG
@set CFLAGS=%CFLAGS% -DDEFAULT_OUTPUT_STDOUT
@set CFLAGS=%CFLAGS% -DSQLITE_ENABLE_COLUMN_METADATA


@set CFLAGS=%CFLAGS% -D_GNU_SOURCE
:@set CFLAGS=%CFLAGS% -s ASSERTIONS=2

@set CFLAGS=%CFLAGS% -D__FILESYS_NO_FILE_LOGGING__
:@set CFLAGS=%CFLAGS% -DDEBUG_TRACE_LOG 
:@set CFLAGS=%CFLAGS% -DLOG_OPERATIONS
:@set CFLAGS=%CFLAGS% -DDEBUG_FILE_OPS -DDEBUG_DISK_IO -DDEBUG_DIRECTORIESa -DDEBUG_BLOCK_INIT -DDEBUG_TIMELINE_AVL -DDEBUG_TIMELINE_DIR_TRACKING -DDEBUG_FILE_SCAN -DDEBUG_FILE_OPEN

:@set CFLAGS=%CFLAGS% -s LINKABLE=1

: -s EXPORTED_FUNCTIONS="[%EXPORTS%]"
: -s EXTRA_EXPORTED_RUNTIME_METHODS=["stringToUTF8"] -s EXPORTED_FUNCTIONS="[%EXPORTS%]"

@set CFLAGS=%CFLAGS%  -s EXPORT_ALL=1

@set SRCS=
@set SRCS=%SRCS% sack.c
@set SRCS=%SRCS% sqlite3.c


@set SRCS=%SRCS% common.c
@set SRCS=%SRCS% objStore.c
@set SRCS=%SRCS% sql_module.c
@set SRCS=%SRCS% srg_module.c
@set SRCS=%SRCS% json6_parser.c
@set SRCS=%SRCS% jsox_parser.c

@set LIBS=-lcrypto -lssl -ltls
@set LIBPATH=-Llibs

@set CFLAGS=%CFLAGS% %LIBPATH% %LIBS%

:-s RESERVED_FUNCTION_POINTERS=20

call emcc  -g -o ./sack.vfs.wasm.dbg.js %DBG_CFLAGS% %CFLAGS% %SRCS% --post-js exportTail.js
call emcc  -O3 -o ./sack.vfs.wasm.js %CFLAGS% %SRCS% --post-js exportTail.js

:call emcc --memory-init-file 0 -o ./vfs-fs-w0.js   -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.c

:call google-closure-compiler.cmd --language_out NO_TRANSPILE --formatting=pretty_print  --js=vfs-fs-w.js --js_output_file=vfs-fs-w-pretty.js
