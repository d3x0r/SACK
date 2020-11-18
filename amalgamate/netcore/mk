if ! command -v ppc &> /dev/null
then
  echo "ppc is not installed..."
  exit
fi

#:::
#:   
#:   ../../src/typelib/typecode.c 
#:   ../../src/typelib/text.c 
#:   ../../src/typelib/sets.c
#:   ../../src/typelib/binarylist.c 
#:   ../../src/memlib/sharemem.c 
#:   ../../src/memlib/memory_operations.c 
#:   ../../src/deadstart/deadstart_core.c 
#:   
#:   

SRCS=""
SRCS="$SRCS   ../../src/typelib/typecode.c"
SRCS="$SRCS   ../../src/typelib/text.c "
SRCS="$SRCS   ../../src/typelib/sets.c"
SRCS="$SRCS   ../../src/typelib/binarylist.c "
SRCS="$SRCS   ../../src/typelib/http.c"
SRCS="$SRCS   ../../src/typelib/url.c"
SRCS="$SRCS   ../../src/typelib/familytree.c"
SRCS="$SRCS   ../../src/fractionlib/fractions.c"
SRCS="$SRCS   ../../src/memlib/sharemem.c "
SRCS="$SRCS   ../../src/memlib/memory_operations.c "
SRCS="$SRCS   ../../src/timerlib/timers.c "
SRCS="$SRCS   ../../src/idlelib/idle.c "
SRCS="$SRCS   ../../src/netlib/network.c "
SRCS="$SRCS   ../../src/netlib/network_all.c "
SRCS="$SRCS   ../../src/netlib/network_addresses.c "
SRCS="$SRCS   ../../src/netlib/tcpnetwork.c "
SRCS="$SRCS   ../../src/netlib/udpnetwork.c "
SRCS="$SRCS   ../../src/netlib/ping.c "
SRCS="$SRCS   ../../src/netlib/whois.c "
SRCS="$SRCS   ../../src/netlib/ssl_layer.c "
SRCS="$SRCS   ../../src/netlib/net_winsock2.c "
SRCS="$SRCS   ../../src/netlib/html5.websocket/html5.websocket.common.c"
SRCS="$SRCS   ../../src/netlib/html5.websocket/server/html5.websocket.c "
SRCS="$SRCS   ../../src/netlib/html5.websocket/client/html5.websocket.client.c"

SRCS="$SRCS   ../../src/netlib/html5.websocket/json/json_parser.c"
SRCS="$SRCS   ../../src/netlib/html5.websocket/json/json6_parser.c"
SRCS="$SRCS   ../../src/netlib/html5.websocket/json/jsox_parser.c"


SRCS="$SRCS   ../../src/filesyslib/pathops.c"
SRCS="$SRCS   ../../src/filesyslib/winfiles.c"
SRCS="$SRCS   ../../src/filesyslib/filescan.c"
SRCS="$SRCS   ../../src/filesyslib/pathops.c"

SRCS="$SRCS   ../../src/sysloglib/syslog.c"


SRCS="$SRCS   ../../src/configlib/configscript.c"

SRCS="$SRCS   ../../src/systemlib/args.c"
SRCS="$SRCS   ../../src/systemlib/system.c"
SRCS="$SRCS   ../../src/systemlib/spawntask.c"
SRCS="$SRCS   ../../src/systemlib/args.c"
SRCS="$SRCS   ../../src/systemlib/oswin.c"

SRCS="$SRCS   ../../src/procreglib/names.c"

SRCS="$SRCS   ../../src/vectlib/vectlib.c"

SRCS="$SRCS   ../../src/contrib/md5lib/md5c.c"
SRCS="$SRCS   ../../src/contrib/sha1lib/sha1.c"
SRCS="$SRCS   ../../src/contrib/sha2lib/sha2.c"
SRCS="$SRCS   ../../src/contrib/sha3lib/sha3.c"
SRCS="$SRCS   ../../src/contrib/K12/lib/KangarooTwelve.c"
SRCS="$SRCS   ../../src/salty_random_generator/salty_generator.c"
SRCS="$SRCS   ../../src/salty_random_generator/crypt_util.c"
SRCS="$SRCS   ../../src/salty_random_generator/block_shuffle.c"


SRCS="$SRCS   ../../src/deadstart/deadstart_core.c "
 
CPP_CFLAGS="-DWINFILE_COMMON_SOURCE"

rm sack_ucb_networking.c
rm sack_ucb_networking.h

ppc -c -K -once -ssio -sd -I../../include -I../../src/contrib/K12/lib -p -osack_ucb_networking.c -DINCLUDE_LOGGING $SRCS

mkdir h
cp config.ppc.h h\config.ppc
cd h

HDRS=""
HDRS="$HDRS ../../../include/stdhdrs.h"
HDRS="$HDRS ../../../include/network.h"
HDRS="$HDRS ../../../include/html5.websocket.h"
HDRS="$HDRS ../../../include/html5.websocket.client.h"
HDRS="$HDRS ../../../include/json_emitter.h"
HDRS="$HDRS ../../../include/jsox_parser.h"
HDRS="$HDRS ../../../include/configscript.h"
HDRS="$HDRS ../../../include/filesys.h"
HDRS="$HDRS ../../../include/procreg.h"
HDRS="$HDRS ../../../include/http.h"
HDRS="$HDRS ../../../include/md5.h"
HDRS="$HDRS ../../../include/sha1.h"
HDRS="$HDRS ../../../include/sha2.h"
HDRS="$HDRS ../../../src/contrib/sha3lib/sha.h"


ppc -c -K -once -ssio -sd -I../../../include -p -o../sack_ucb_networking.h $HDRS
cd ..

CFLAGS=""
CFLAGS="$CFLAGS -DTARGETNAME=NULL"

gcc -c -g -o a.o $CFLAGS sack_ucb_networking.c
gcc -c -O3 -o a-opt.o $CFLAGS sack_ucb_networking.c

LIBS=""
LIBS="${LIBS} -lm"
LIBS="${LIBS} -lpthread"
LIBS="${LIBS} -ldl"
LIBS="${LIBS} -lodbc"
LIBS="${LIBS} -luuid"

gcc -g -o a.exe $CFLAGS sack_ucb_networking.c test.c $LIBS
gcc -O3 -o a-opt.exe $CFLAGS sack_ucb_networking.c test.c $LIBS

gcc -g -o a.exe a.o test.c $LIBS
gcc -O3 -o a-opt.exe a-opt.o test.c $LIBS

#:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE