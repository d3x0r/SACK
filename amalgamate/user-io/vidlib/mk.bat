:::
:   
:

call mksrc.bat


set C_OPTS=
set C_OPTS=%C_OPTS% -DTARGETNAME=\"a.exe\"


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

@set LIBS=%LIBS% -lopengl32
@set LIBS=%LIBS% -lgdi32

gcc -shared -g -o a.so %C_OPTS% sack_vidlib.cc sack_vidlib_puregl2.cc ../../fullcore/a.o %LIBS%
gcc -c -O3 -o a-opt.o %C_OPTS% sack_vidlib.cc sack_vidlib_puregl2.cc

gcc -g -o a.exe %C_OPTS% sack_vidlib.c ../../fullcore/sack_ucb_full.c test.c %LIBS%
gcc -O3 -o a-opt.exe %C_OPTS% sack_vidlib.c ../../fullcore/sack_ucb_full.c test.c %LIBS%

gcc -g -o a.exe a.o ../../fullcore/a.o test.c %LIBS%
gcc -O3 -o a-opt.exe a-opt.o ../../fullcore/a-opt.o test.c %LIBS%

gcc -O3 -o a-test2.exe a-opt.o test2.c %LIBS%

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE

