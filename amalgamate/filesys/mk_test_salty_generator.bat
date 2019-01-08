gcc -g -c -O3 -o a-opt.o sack_ucb_filelib.c

gcc -g -O3 a-opt.o test_salty_generator.c -lwinmm -lws2_32 -lole32
