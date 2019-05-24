

call emcc -s WASM=1 -o sha3.wasm.js sha3.c
call emcc -std=c++11 -o ebind.o ebind.cc
call emcc -o sha3.js sha3.o ebind.o
