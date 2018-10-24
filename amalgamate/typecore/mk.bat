:::
:   
:   ../../src/typelib/typecode.c 
:   ../../src/typelib/text.c 
:   ../../src/typelib/sets.c
:   ../../src/typelib/binarylist.c 
:   ../../src/typelib/familytree.c 
:   ../../src/typelib/http.c 
:   ../../src/typelib/url.c 
:   ../../src/memlib/sharemem.c 
:   ../../src/memlib/memory_operations.c 
:   ../../src/deadstart/deadstart_core.c 
:   
:   

@set SRCS=
@set SRCS= %SRCS%   ../../src/typelib/typecode.c 
@set SRCS= %SRCS%   ../../src/typelib/text.c 
@set SRCS= %SRCS%   ../../src/typelib/input.c
@set SRCS= %SRCS%   ../../src/typelib/sets.c
@set SRCS= %SRCS%   ../../src/typelib/binarylist.c 
@set SRCS= %SRCS%   ../../src/typelib/url.c
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/json/json_parser.c
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/json/json6_parser.c
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/json/jsox_parser.c
@set SRCS= %SRCS%   ../../src/fractionlib/fractions.c
:@set SRCS= %SRCS%   ../../src/vectlib/vectlib.c
:@set SRCS= %SRCS%   ../../src/filesyslib/winfiles.c
@set SRCS= %SRCS%   ../../src/memlib/sharemem.c 
@set SRCS= %SRCS%   ../../src/memlib/memory_operations.c 
@set SRCS= %SRCS%   ../../src/deadstart/deadstart_core.c 
 

del sack_ucb_typelib.h
del sack_ucb_typelib.c

c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -osack_ucb_typelib.c -DINCLUDE_LOGGING %SRCS%

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% ../../../include/stdhdrs.h
@set HDRS= %HDRS% ../../../include/json_emitter.h
@set HDRS= %HDRS% ../../../include/jsox_parser.h
@set HDRS= %HDRS% ../../../include/fractions.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -o../sack_ucb_typelib.h %HDRS%
cd ..

gcc -c -g -o a.o sack_ucb_typelib.c
gcc -c -O3 -o a-opt.o sack_ucb_typelib.c

gcc -g -o a.exe sack_ucb_typelib.c test.c
gcc -O3 -o a-opt.exe sack_ucb_typelib.c test.c

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE