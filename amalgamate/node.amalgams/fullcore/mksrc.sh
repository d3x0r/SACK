#!/bin/bash

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
S=../../..

SRCS=''
SRCS+=*$S/src/typelib/typecode.c
SRCS+=*$S/src/typelib/text.c
SRCS+=*$S/src/typelib/input.c
SRCS+=*$S/src/typelib/lets.c
echo $SRCS
SRCS+=*$S/src/typelib/binarylist.c
SRCS+=*$S/src/typelib/http.c
SRCS+=*$S/src/typelib/url.c
SRCS+=*$S/src/typelib/familytree.c
SRCS+=*$S/src/fractionlib/fractions.c
SRCS+=*$S/src/memlib/sharemem.c
SRCS+=*$S/src/memlib/memory_operations.c
SRCS+=*$S/src/timerlib/timers.c
SRCS+=*$S/src/idlelib/idle.c
SRCS+=*$S/src/netlib/network.c
SRCS+=*$S/src/netlib/network_all.c
SRCS+=*$S/src/netlib/network_addresses.c
SRCS+=*$S/src/netlib/network_addresses.c
SRCS+=*$S/src/netlib/tcpnetwork.c
SRCS+=*$S/src/netlib/udpnetwork.c
SRCS+=*$S/src/netlib/ping.c
SRCS+=*$S/src/netlib/whois.c
SRCS+=*$S/src/netlib/ssl_layer.c
SRCS+=*$S/src/netlib/net_winsock2.c
SRCS+=*$S/src/netlib/html5.websocket/html5.websocket.common.c
SRCS+=*$S/src/netlib/html5.websocket/server/html5.websocket.c
SRCS+=*$S/src/netlib/html5.websocket/client/html5.websocket.client.c

SRCS+=*$S/src/netlib/html5.websocket/json/json_parser.c
SRCS+=*$S/src/netlib/html5.websocket/json/json6_parser.c
SRCS+=*$S/src/netlib/html5.websocket/json/jsox_parser.c
SRCS+=*$S/src/netlib/html5.websocket/json/vesl_parser.c


SRCS+=*$S/src/filesyslib/winfiles.c
SRCS+=*$S/src/filesyslib/pathops.c
SRCS+=*$S/src/filesyslib/filescan.c

SRCS+=*$S/src/filesyslib/filemon/allfiles.c
SRCS+=*$S/src/filesyslib/filemon/linuxfiles.c
SRCS+=*$S/src/filesyslib/filemon/windowsfiles.c


SRCS+=*$S/src/sysloglib/syslog.c


SRCS+=*$S/src/configlib/configscript.c

SRCS+=*$S/src/systemlib/args.c
SRCS+=*$S/src/systemlib/system.c
SRCS+=*$S/src/systemlib/spawntask.c
SRCS+=*$S/src/systemlib/args.c
SRCS+=*$S/src/systemlib/oswin.c

SRCS+=*$S/src/procreglib/names.c

SRCS+=*$S/src/vectlib/vectlib.c

SRCS+=*$S/src/commlib/sackcomm.c


SRCS+=*$S/src/utils/virtual_file_system/vfs.c
SRCS+=*$S/src/utils/virtual_file_system/vfs_fs.c
SRCS+=*$S/src/utils/virtual_file_system/vfs_os.c
SRCS+=*$S/src/contrib/md5lib/md5c.c
SRCS+=*$S/src/contrib/sha1lib/sha1.c
SRCS+=*$S/src/contrib/sha2lib/sha2.c
SRCS+=*$S/src/contrib/sha3lib/sha3.c
SRCS+=*$S/src/contrib/k12/lib/KangarooTwelve.c
SRCS+=*$S/src/salty_random_generator/salty_generator.c
SRCS+=*$S/src/salty_random_generator/crypt_util.c
SRCS+=*$S/src/salty_random_generator/block_shuffle.c

SRCS+=*$S/src/contrib/sqlite/sqlite_interface.c
SRCS+=*$S/src/SQLlib/sqlstruc.h
SRCS+=*$S/src/SQLlib/sqlstub.c
SRCS+=*$S/src/SQLlib/sqlwrap.c
SRCS+=*$S/src/SQLlib/sqlutil.c
SRCS+=*$S/src/SQLlib/guid.c
SRCS+=*$S/src/SQLlib/sqlparse3.c
SRCS+=*$S/src/SQLlib/optlib/getoption.c
SRCS+=*$S/src/SQLlib/optlib/getoption_v4.c
SRCS+=*$S/src/SQLlib/optlib/optionutil.c
SRCS+=*$S/src/SQLlib/optlib/optionutil_v4.c
SRCS+=*$S/src/translationlib/translate.c

SRCS+=*$S/src/deadstart/deadstart_core.c

SRCS=$(echo $SRCS|sed 's/*/ /g')
 
rm sack_typelib.c
rm sack_typelib.h

OPTS=
OPTS+= -I$S/src/contrib/sqlite/3.23.0-MySqlite
OPTS=$(echo $OPTS|sed 's/*/ /g')

ppc -c -K -once -ssio -sd $OPTS -I$S/include -p -osack_ucb_full.cc -DINCLUDE_LOGGING $SRCS

mkdir h
cp config.ppc.h h/config.ppc
cd h



HDRS=
HDRS+=*$S/../include/stdhdrs.h
HDRS+=*$S/../include/network.h
HDRS+=*$S/../include/html5.websocket.h
HDRS+=*$S/../include/html5.websocket.client.h
HDRS+=*$S/../include/json_emitter.h
HDRS+=*$S/../include/jsox_parser.h
HDRS+=*$S/../include/configscript.h
HDRS+=*$S/../include/filesys.h
HDRS+=*$S/../include/filemon.h
HDRS+=*$S/../include/procreg.h
HDRS+=*$S/../include/http.h
HDRS+=*$S/../include/pssql.h
HDRS+=*$S/../include/sqlgetoption.h
HDRS+=*$S/../include/md5.h
HDRS+=*$S/../include/sha1.h
HDRS+=*$S/../include/sha2.h
HDRS+=*$S/../src/contrib/sha3lib/sha.h

HDRS+=*$S/../include/idle.h
HDRS+=*$S/../include/filesys.h
HDRS+=*$S/../include/sack_vfs.h
HDRS+=*$S/../include/json_emitter.h
HDRS+=*$S/../include/vesl_emitter.h
HDRS+=*$S/../include/jsox_parser.h
HDRS+=*$S/../include/salty_generator.h
HDRS+=*$S/../include/sackcomm.h
HDRS+=*$S/../include/translation.h

HDRS=$(echo $HDRS|sed 's/*/ /g')

ppc -c -K -once -ssio -sd -I$S/../include -p -o../sack_ucb_full.h $HDRS
cd ..

