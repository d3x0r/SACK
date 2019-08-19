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

@set SRCS=
@set SRCS= %SRCS%   ../../src/typelib/typecode.c 
@set SRCS= %SRCS%   ../../src/typelib/text.c 
@set SRCS= %SRCS%   ../../src/typelib/input.c 
@set SRCS= %SRCS%   ../../src/typelib/sets.c
@set SRCS= %SRCS%   ../../src/typelib/binarylist.c 
@set SRCS= %SRCS%   ../../src/typelib/http.c
@set SRCS= %SRCS%   ../../src/typelib/url.c
@set SRCS= %SRCS%   ../../src/typelib/familytree.c
@set SRCS= %SRCS%   ../../src/fractionlib/fractions.c
@set SRCS= %SRCS%   ../../src/memlib/sharemem.c 
@set SRCS= %SRCS%   ../../src/memlib/memory_operations.c 
@set SRCS= %SRCS%   ../../src/timerlib/timers.c 
@set SRCS= %SRCS%   ../../src/idlelib/idle.c 
@set SRCS= %SRCS%   ../../src/netlib/network.c 
@set SRCS= %SRCS%   ../../src/netlib/network_addresses.c 
@set SRCS= %SRCS%   ../../src/netlib/network_all.c 
@set SRCS= %SRCS%   ../../src/netlib/tcpnetwork.c 
@set SRCS= %SRCS%   ../../src/netlib/udpnetwork.c 
@set SRCS= %SRCS%   ../../src/netlib/ping.c 
@set SRCS= %SRCS%   ../../src/netlib/whois.c 
@set SRCS= %SRCS%   ../../src/netlib/ssl_layer.c 
@set SRCS= %SRCS%   ../../src/netlib/net_winsock2.c 
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/html5.websocket.common.c
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/server/html5.websocket.c 
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/client/html5.websocket.client.c

@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/json/json_parser.c
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/json/json6_parser.c
@set SRCS= %SRCS%   ../../src/netlib/html5.websocket/json/jsox_parser.c


@set SRCS= %SRCS%   ../../src/filesyslib/pathops.c
@set SRCS= %SRCS%   ../../src/filesyslib/winfiles.c
@set SRCS= %SRCS%   ../../src/filesyslib/filescan.c
@set SRCS= %SRCS%   ../../src/filesyslib/pathops.c

@set SRCS= %SRCS%   ../../src/sysloglib/syslog.c


@set SRCS= %SRCS%   ../../src/configlib/configscript.c

@set SRCS= %SRCS%   ../../src/systemlib/args.c
@set SRCS= %SRCS%   ../../src/systemlib/system.c
@set SRCS= %SRCS%   ../../src/systemlib/spawntask.c
@set SRCS= %SRCS%   ../../src/systemlib/args.c
@set SRCS= %SRCS%   ../../src/systemlib/oswin.c

@set SRCS= %SRCS%   ../../src/procreglib/names.c

@set SRCS= %SRCS%   ../../src/vectlib/vectlib.c


: @set SRCS= %SRCS%   ../../src/utils/virtual_file_system/vfs.c
@set SRCS= %SRCS%   ../../src/utils/virtual_file_system/vfs_fs.c
@set SRCS= %SRCS%   ../../src/utils/virtual_file_system/vfs_os.c
@set SRCS= %SRCS%   ../../src/contrib/md5lib/md5c.c
@set SRCS= %SRCS%   ../../src/contrib/sha1lib/sha1.c
@set SRCS= %SRCS%   ../../src/contrib/sha2lib/sha2.c
@set SRCS= %SRCS%   ../../src/contrib/sha3lib/sha3.c
@set SRCS= %SRCS%   ../../src/contrib/k12/lib/KangarooTwelve.c
@set SRCS= %SRCS%   ../../src/salty_random_generator/salty_generator.c
@set SRCS= %SRCS%   ../../src/salty_random_generator/crypt_util.c
@set SRCS= %SRCS%   ../../src/salty_random_generator/block_shuffle.c

@set SRCS= %SRCS%   ../../src/contrib/sqlite/sqlite_interface.c
@set SRCS= %SRCS%   ../../src/SQLlib/sqlstruc.h
@set SRCS= %SRCS%   ../../src/SQLlib/sqlstub.c
@set SRCS= %SRCS%   ../../src/SQLlib/sqlwrap.c
@set SRCS= %SRCS%   ../../src/SQLlib/sqlutil.c
@set SRCS= %SRCS%   ../../src/SQLlib/guid.c
@set SRCS= %SRCS%   ../../src/SQLlib/sqlparse3.c
@set SRCS= %SRCS%   ../../src/SQLlib/optlib/getoption.c
@set SRCS= %SRCS%   ../../src/SQLlib/optlib/getoption_v4.c
@set SRCS= %SRCS%   ../../src/SQLlib/optlib/optionutil.c
@set SRCS= %SRCS%   ../../src/SQLlib/optlib/optionutil_v4.c
@set SRCS= %SRCS%   ../../src/translationlib/translate.c

@set SRCS= %SRCS%   ../../src/deadstart/deadstart_core.c 
 
@del sack.c
@del sack.h

@set OPTS=
@set OPTS=%OPTS%	-I../../src/contrib/sqlite/3.27.1-TableAlias

c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I../../include -p -osack.c %SRCS%
@copy ..\..\src\contrib\sqlite\3.27.1-TableAlias\sqlite3.c .

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% ../../../include/stdhdrs.h
@set HDRS= %HDRS% ../../../include/network.h
@set HDRS= %HDRS% ../../../include/html5.websocket.h
@set HDRS= %HDRS% ../../../include/html5.websocket.client.h
@set HDRS= %HDRS% ../../../include/json_emitter.h
@set HDRS= %HDRS% ../../../include/jsox_parser.h
@set HDRS= %HDRS% ../../../include/configscript.h
@set HDRS= %HDRS% ../../../include/filesys.h
@set HDRS= %HDRS% ../../../include/procreg.h
@set HDRS= %HDRS% ../../../include/http.h
@set HDRS= %HDRS% ../../../include/pssql.h
@set HDRS= %HDRS% ../../../include/sqlgetoption.h
@set HDRS= %HDRS% ../../../include/md5.h
@set HDRS= %HDRS% ../../../include/sha1.h
@set HDRS= %HDRS% ../../../include/sha2.h
@set HDRS= %HDRS% ../../../src/contrib/sha3lib/sha.h

@set HDRS= %HDRS% ../../../include/idle.h
@set HDRS= %HDRS% ../../../include/filesys.h
@set HDRS= %HDRS% ../../../include/sack_vfs.h
@set HDRS= %HDRS% ../../../include/json_emitter.h
@set HDRS= %HDRS% ../../../include/vesl_emitter.h
@set HDRS= %HDRS% ../../../include/jsox_parser.h
@set HDRS= %HDRS% ../../../include/salty_generator.h
@set HDRS= %HDRS% ../../../include/sackcomm.h
@set HDRS= %HDRS% ../../../include/translation.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -o../sack.h %HDRS%
cd ..

gcc -c -g  -o  sack.o "-DTARGETNAME=""sack.o"""  sack.c
gcc -c -O3 -o sack-opt.o "-DTARGETNAME=""sack.o"""  sack.c 
