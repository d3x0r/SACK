:::
:   
:   
:   

call mksrc.bat

gcc -c -g -o a.o ../fullcore/a.o sack_ucb_psi.c
gcc -c -O3 -o a-opt.o ../fullcore/a-opt.o sack_ucb_psi.c

gcc -g -o a.exe ../fullcore/a.o sack_ucb_psi.c test.c
gcc -O3 -o a-opt.exe ../fullcore/a-opt.o sack_ucb_psi.c test.c

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE