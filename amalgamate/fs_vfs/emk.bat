:call emcc --bind --memory-init-file 0 -std=c++11 -O2 -o ./jsox-wasm.js -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -D__LINUX__  -s EXPORTED_FUNCTIONS="['_jsox_begin_parse','_jsox_parse_clear_state','_jsox_parse_add_data']" jsox.c

:--pre-js or --post-js
call emcc -O2 -o ./vfs-fs-w.js -std=c++11 --bind -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.cc emscripten_bindings.cc

call emcc -o ./vfs-fs-w0.js -std=c++11 --bind -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.cc emscripten_bindings.cc

call google-closure-compiler.cmd --language_out NO_TRANSPILE --formatting=pretty_print  --js=vfs-fs-w.js --js_output_file=vfs-fs-w-pretty.js
