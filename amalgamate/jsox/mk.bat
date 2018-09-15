mkdir c
del jsox.h
c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -oc/jsox.cc -DINCLUDE_LOGGING ../../include/jsox_parser.h ../../src/netlib/html5.websocket/json/jsox_parser.c ../../src/netlib/html5.websocket/json/jsox.h ../../src/deadstart/deadstart_core.c ../../src/procreglib/names.c ../../src/memlib/sharemem.c ../../src/memlib/memory_operations.c ../../src/memlib/sharestruc.h ../../src/typelib/typecode.c ../../src/typelib/text.c ../../src/typelib/binarylist.c ../../src/typelib/familytree.c ../../src/typelib/sets.c
copy c\jsox.cc c\jsox.c
mkdir h
cd h
c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -ojsox.h ../../../include/jsox_parser.h 

cd ..
copy c\*
copy h\jsox.h

gcc -g jsox_parser.c jsox.c

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE