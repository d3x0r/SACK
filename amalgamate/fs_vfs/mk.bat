:::
:   
:   ../../include/sack_vfs.h 
:   ../../src/utils/virtual_file_system/vfs_fs.c 
: 
:   ../../src/deadstart/deadstart_core.c 
:   ../../src/memlib/sharemem.c 
:   ../../src/memlib/memory_operations.c 
:   ../../src/typelib/typecode.c 
:   ../../src/typelib/text.c 
:   ../../src/typelib/sets.c
:   ../../src/typelib/binarylist.c 
: 
:   ../../src/filesyslib/pathops.c
:   
:   ../../src/salty_random_generator/salty_generator.c 
:   ../../src/contrib/sha3lib/sha3.c
:   ../../src/contrib/sha2lib/sha2.c
:   ../../src/contrib/sha1lib/sha.c

mkdir c
del jsox.h

set RNG=%RNG% ../../src/salty_random_generator/salty_generator.c 
set RNG=%RNG% ../../src/salty_random_generator/crypt_util.c 
set RNG=%RNG% ../../src/salty_random_generator/block_shuffle.c 
set RNG=%RNG% ../../src/contrib/K12/lib/KangarooTwelve.c
set RNG=%RNG% ../../src/contrib/sha3lib/sha3.c
set RNG=%RNG% ../../src/contrib/sha2lib/sha2.c
set RNG=%RNG% ../../src/contrib/sha1lib/sha1.c


:c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -oc/jsox.cc -DINCLUDE_LOGGING ../../include/jsox_parser.h ../../src/netlib/html5.websocket/json/jsox_parser.c ../../src/netlib/html5.websocket/json/jsox.h ../../src/deadstart/deadstart_core.c ../../src/memlib/sharemem.c ../../src/memlib/memory_operations.c ../../src/typelib/typecode.c ../../src/typelib/text.c ../../src/typelib/sets.c ../../src/typelib/binarylist.c 
c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -I../../src/contrib/K12/lib -DNO_SSL -p -oc/vfs_fs.cc -DINCLUDE_LOGGING ../../include/sack_vfs.h ../../src/filesyslib/pathops.c ../../src/utils/virtual_file_system/vfs_os.c  ../../src/deadstart/deadstart_core.c ../../src/memlib/sharemem.c ../../src/memlib/memory_operations.c ../../src/typelib/typecode.c ../../src/typelib/text.c ../../src/typelib/sets.c ../../src/typelib/binarylist.c  %RNG% 
copy c\vfs_fs.cc c\vfs_fs.c
mkdir h
cd h
c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -ovfs_fs.h ../../../include/sack_vfs.h 

cd ..
copy c\*
copy h\vfs_fs.h

gcc -g -o a tester.c vfs_fs.c
copy tester.c tester.cc
gcc -g -o c tester.cc vfs_fs.cc

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE