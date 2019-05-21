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

set SRCS=%SRCS% ../../include/sack_vfs.h
set SRCS=%SRCS% ../../src/filesyslib/pathops.c
set SRCS=%SRCS% ../../src/utils/virtual_file_system/vfs_os.c
set SRCS=%SRCS%  ../../src/deadstart/deadstart_core.c
set SRCS=%SRCS% ../../src/memlib/sharemem.c
set SRCS=%SRCS% ../../src/memlib/memory_operations.c
set SRCS=%SRCS% ../../src/typelib/typecode.c
set SRCS=%SRCS% ../../src/typelib/text.c
set SRCS=%SRCS% ../../src/typelib/sets.c
set SRCS=%SRCS% ../../src/typelib/binarylist.c
set SRCS=%SRCS% ../../src/timerlib/timers.c
set SRCS=%SRCS% ../../src/configlib/configscript.c

set RNG=%RNG% ../../src/salty_random_generator/salty_generator.c 
set RNG=%RNG% ../../src/salty_random_generator/crypt_util.c 
set RNG=%RNG% ../../src/salty_random_generator/block_shuffle.c 
set RNG=%RNG% ../../src/contrib/K12/lib/KangarooTwelve.c
set RNG=%RNG% ../../src/contrib/sha3lib/sha3.c
set RNG=%RNG% ../../src/contrib/sha2lib/sha2.c
set RNG=%RNG% ../../src/contrib/sha1lib/sha1.c


:c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -oc/jsox.cc -DINCLUDE_LOGGING ../../include/jsox_parser.h ../../src/netlib/html5.websocket/json/jsox_parser.c ../../src/netlib/html5.websocket/json/jsox.h ../../src/deadstart/deadstart_core.c ../../src/memlib/sharemem.c ../../src/memlib/memory_operations.c ../../src/typelib/typecode.c ../../src/typelib/text.c ../../src/typelib/sets.c ../../src/typelib/binarylist.c 
c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -I../../src/contrib/K12/lib -DNO_SSL -p -oc/vfs_fs.cc -DINCLUDE_LOGGING %SRCS%  %RNG% 
copy c\vfs_fs.cc c\vfs_fs.c
mkdir h
cd h
c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -ovfs_fs.h ../../../include/sack_vfs.h 

cd ..
copy c\*
copy h\vfs_fs.h

gcc -g -o a tester.c vfs_fs.c  -lwinmm
copy tester.c tester.cc
gcc -g -o c tester.cc vfs_fs.cc  -lstdc++ -lwinmm

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE