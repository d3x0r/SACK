if ! command -v ppc &> /dev/null
then
  echo "ppc is not installed..."
  exit
fi
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

SRCS=""
SRCS="$SRCS  ../../src/typelib/typecode.c"
SRCS="$SRCS  ../../src/typelib/text.c"
SRCS="$SRCS  ../../src/typelib/input.c"
SRCS="$SRCS  ../../src/typelib/sets.c"
SRCS="$SRCS  ../../src/typelib/binarylist.c"
SRCS="$SRCS  ../../src/typelib/url.c"
SRCS="$SRCS  ../../src/netlib/html5.websocket/json/json_parser.c"
SRCS="$SRCS  ../../src/netlib/html5.websocket/json/json6_parser.c"
SRCS="$SRCS  ../../src/netlib/html5.websocket/json/jsox_parser.c"
SRCS="$SRCS  ../../src/fractionlib/fractions.c"
:SRCS="$SRCS  ../../src/vectlib/vectlib.c"
:SRCS="$SRCS  ../../src/filesyslib/winfiles.c"
SRCS="$SRCS  ../../src/memlib/sharemem.c"
SRCS="$SRCS  ../../src/memlib/memory_operations.c"
SRCS="$SRCS  ../../src/timerlib/timers.c"

SRCS="$SRCS   ../../src/deadstart/deadstart_core.c"
 

rm sack_ucb_typelib.h
rm sack_ucb_typelib.c

ppc -c -K -once -ssio -sd -I../../include -p -osack_ucb_typelib.c -DINCLUDE_LOGGING $SRCS

mkdir h
copy config.ppc.h h\config.ppc
cd h

HDRS=""
HDRS="$HDRS ../../../include/stdhdrs.h"
HDRS="$HDRS ../../../include/json_emitter.h"
HDRS="$HDRS ../../../include/jsox_parser.h"
HDRS="$HDRS ../../../include/fractions.h"

ppc -c -K -once -ssio -sd -I../../../include -p -o../sack_ucb_typelib.h $HDRS
cd ..

gcc -c -g -o a.o sack_ucb_typelib.c
gcc -c -O3 -o a-opt.o sack_ucb_typelib.c

gcc -g -o a a.o test.c -lpthread
gcc -O3 -o a-opt a-opt.o test.c -lpthread

#_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE
