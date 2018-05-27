call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/add.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/code.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/debug.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/decode.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/long_term.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/lpc.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/preprocess.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/rpe.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_destroy.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_decode.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_encode.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_explode.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_implode.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_create.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_print.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/gsm_option.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/short_term.c
call emcc -ansi -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/table.c
call em++ -std=c++11 -pedantic -c -O2 -DNeedFunctionPrototypes=1 -DSASR     -DWAV49  -I./inc src/emscripten_bindings.cc

call emcc -O2 --memory-init-file 0 -o ./lib/libgsm.js ./src/emscripten_bindings.o ./src/add.o ./src/code.o ./src/debug.o ./src/decode.o ./src/long_term.o ./src/lpc.o ./src/preprocess.o ./src/rpe.o ./src/gsm_destroy.o ./src/gsm_decode.o ./src/gsm_encode.o ./src/gsm_explode.o ./src/gsm_implode.o ./src/gsm_create.o ./src/gsm_print.o ./src/gsm_option.o ./src/short_term.o ./src/table.o
call emcc -O2 --memory-init-file 0 -o ./lib/libgsm2.js -s EXPORTED_FUNCTIONS="['_gsm_create','_gsm_decode','_gsm_encode']" ./src/add.o ./src/code.o ./src/debug.o ./src/decode.o ./src/long_term.o ./src/lpc.o ./src/preprocess.o ./src/rpe.o ./src/gsm_destroy.o ./src/gsm_decode.o ./src/gsm_encode.o ./src/gsm_explode.o ./src/gsm_implode.o ./src/gsm_create.o ./src/gsm_print.o ./src/gsm_option.o ./src/short_term.o ./src/table.o

call emar cr ./lib/libgsm.a ./src/add.o ./src/code.o ./src/debug.o ./src/decode.o ./src/long_term.o ./src/lpc.o ./src/preprocess.o ./src/rpe.o ./src/gsm_destroy.o ./src/gsm_decode.o ./src/gsm_encode.o ./src/gsm_explode.o ./src/gsm_implode.o ./src/gsm_create.o ./src/gsm_print.o ./src/gsm_option.o ./src/short_term.o ./src/table.o
call emranlib ./lib/libgsm.a
