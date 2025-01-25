:::
:   
:   ../../src/typelib/typecode.c 
:   ../../src/typelib/text.c 
:   ../../src/typelib/sets.c
:   ../../src/typelib/binarylist.c 
:   ../../src/memlib/sharemem.c 
:   ../../src/memlib/memory_operations.c 
:   ../../src/deadstart/deadstart_core.c 
:   
:   

call mksrc.bat

@set SQLITE_OPTS=-I../../src/contrib/sqlite/3.33.0-TableAlias -DSQLITE_ENABLE_COLUMN_METADATA


gcc  %SQLITE_OPTS% -c -g -o ac.o sack_ucb_full.c
gcc  %SQLITE_OPTS% -c -O3 -o ac-opt.o sack_ucb_full.c
gcc  %SQLITE_OPTS% -lstdc++ -c -g -o a.o sack_ucb_full.cc
gcc  %SQLITE_OPTS% -lstdc++ -c -O3 -o a-opt.o sack_ucb_full.cc

@set LIBS=
@set LIBS=%LIBS% -lwinmm 
@set LIBS=%LIBS% -lws2_32
@set LIBS=%LIBS% -liphlpapi
@set LIBS=%LIBS% -lrpcrt4
@set LIBS=%LIBS% -lodbc32 
@set LIBS=%LIBS% -lpsapi
@set LIBS=%LIBS% -lntdll
@set LIBS=%LIBS% -lcrypt32
@set LIBS=%LIBS% -lole32

@set LIBS=%LIBS% ../../src/contrib/sqlite/3.33.0-TableAlias/sqlite3.c

gcc  %SQLITE_OPTS% -g -o a.exe sack_ucb_full.cc test.cc %LIBS% -lstdc++ 
gcc  %SQLITE_OPTS% -lstdc++ -O3 -o a-opt.exe sack_ucb_full.cc test.cc %LIBS%

gcc %SQLITE_OPTS% -g -o ac.exe ac.o test.c %LIBS%
gcc %SQLITE_OPTS% -O3 -o ac-opt.exe ac-opt.o test.c %LIBS%

gcc %SQLITE_OPTS% -O3 -o ac-test2.exe ac-opt.o test2.c %LIBS%

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE

