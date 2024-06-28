
copy test.cc test.c

gcc -O3 -S test.cc -o test.gcc.cc.s
gcc -O3 -S test.c -o test.gcc.c.s

cl /Ox /Fatest.cl.cc.asm test.cc
cl /Ox /Fatest.cl.c.asm test.c

clang-cl /Ox /Fatest.clang.cc.asm test.cc
clang-cl /Ox /Fatest.clang.c.asm test.c


gcc  -S test.cc -o test.gcc.cc.no-opt.s
gcc  -S test.c -o test.gcc.c.no-opt.s

cl  /Fatest.cc.no-opt.asm test.cc
cl  /Fatest.c.no-opt.asm test.c

clang-cl  /Fatest.clang.cc.no-opt.asm test.cc
clang-cl  /Fatest.clang.c.no-opt.asm test.c

