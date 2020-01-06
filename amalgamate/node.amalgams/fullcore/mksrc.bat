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
@SET S=../../..

@set SRCS=
@set SRCS= %SRCS%   %S%/src/typelib/typecode.c 
@set SRCS= %SRCS%   %S%/src/typelib/text.c 
@set SRCS= %SRCS%   %S%/src/typelib/input.c 
@set SRCS= %SRCS%   %S%/src/typelib/sets.c
@set SRCS= %SRCS%   %S%/src/typelib/binarylist.c 
@set SRCS= %SRCS%   %S%/src/typelib/http.c
@set SRCS= %SRCS%   %S%/src/typelib/url.c
@set SRCS= %SRCS%   %S%/src/typelib/familytree.c
@set SRCS= %SRCS%   %S%/src/fractionlib/fractions.c
@set SRCS= %SRCS%   %S%/src/memlib/sharemem.c 
@set SRCS= %SRCS%   %S%/src/memlib/memory_operations.c 
@set SRCS= %SRCS%   %S%/src/timerlib/timers.c 
@set SRCS= %SRCS%   %S%/src/idlelib/idle.c 
@set SRCS= %SRCS%   %S%/src/netlib/network.c 
@set SRCS= %SRCS%   %S%/src/netlib/network_all.c 
@set SRCS= %SRCS%   %S%/src/netlib/network_addresses.c 
@set SRCS= %SRCS%   %S%/src/netlib/network_addresses.c 
@set SRCS= %SRCS%   %S%/src/netlib/tcpnetwork.c 
@set SRCS= %SRCS%   %S%/src/netlib/udpnetwork.c 
@set SRCS= %SRCS%   %S%/src/netlib/ping.c 
@set SRCS= %SRCS%   %S%/src/netlib/whois.c 
@set SRCS= %SRCS%   %S%/src/netlib/ssl_layer.c 
@set SRCS= %SRCS%   %S%/src/netlib/net_winsock2.c 
@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/html5.websocket.common.c
@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/server/html5.websocket.c 
@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/client/html5.websocket.client.c

@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/json/json_parser.c
@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/json/json6_parser.c
@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/json/jsox_parser.c
@set SRCS= %SRCS%   %S%/src/netlib/html5.websocket/json/vesl_parser.c


@set SRCS= %SRCS%   %S%/src/filesyslib/winfiles.c
@set SRCS= %SRCS%   %S%/src/filesyslib/pathops.c
@set SRCS= %SRCS%   %S%/src/filesyslib/filescan.c

@set SRCS= %SRCS%   %S%/src/filesyslib/filemon/allfiles.c
@set SRCS= %SRCS%   %S%/src/filesyslib/filemon/linuxfiles.c
@set SRCS= %SRCS%   %S%/src/filesyslib/filemon/windowsfiles.c


@set SRCS= %SRCS%   %S%/src/sysloglib/syslog.c


@set SRCS= %SRCS%   %S%/src/configlib/configscript.c

@set SRCS= %SRCS%   %S%/src/systemlib/args.c
@set SRCS= %SRCS%   %S%/src/systemlib/system.c
@set SRCS= %SRCS%   %S%/src/systemlib/spawntask.c
@set SRCS= %SRCS%   %S%/src/systemlib/args.c
@set SRCS= %SRCS%   %S%/src/systemlib/oswin.c

@set SRCS= %SRCS%   %S%/src/procreglib/names.c

@set SRCS= %SRCS%   %S%/src/vectlib/vectlib.c

@set SRCS= %SRCS%   %S%/src/commlib/sackcomm.c


@set SRCS= %SRCS%   %S%/src/utils/virtual_file_system/vfs.c
@set SRCS= %SRCS%   %S%/src/utils/virtual_file_system/vfs_fs.c
@set SRCS= %SRCS%   %S%/src/utils/virtual_file_system/vfs_os.c
@set SRCS= %SRCS%   %S%/src/contrib/md5lib/md5c.c
@set SRCS= %SRCS%   %S%/src/contrib/sha1lib/sha1.c
@set SRCS= %SRCS%   %S%/src/contrib/sha2lib/sha2.c
@set SRCS= %SRCS%   %S%/src/contrib/sha3lib/sha3.c
@set SRCS= %SRCS%   %S%/src/contrib/k12/lib/KangarooTwelve.c
@set SRCS= %SRCS%   %S%/src/salty_random_generator/salty_generator.c
@set SRCS= %SRCS%   %S%/src/salty_random_generator/crypt_util.c
@set SRCS= %SRCS%   %S%/src/salty_random_generator/block_shuffle.c

@set SRCS= %SRCS%   %S%/src/contrib/sqlite/sqlite_interface.c
@set SRCS= %SRCS%   %S%/src/SQLlib/sqlstruc.h
@set SRCS= %SRCS%   %S%/src/SQLlib/sqlstub.c
@set SRCS= %SRCS%   %S%/src/SQLlib/sqlwrap.c
@set SRCS= %SRCS%   %S%/src/SQLlib/sqlutil.c
@set SRCS= %SRCS%   %S%/src/SQLlib/guid.c
@set SRCS= %SRCS%   %S%/src/SQLlib/sqlparse3.c
@set SRCS= %SRCS%   %S%/src/SQLlib/optlib/getoption.c
@set SRCS= %SRCS%   %S%/src/SQLlib/optlib/getoption_v4.c
@set SRCS= %SRCS%   %S%/src/SQLlib/optlib/optionutil.c
@set SRCS= %SRCS%   %S%/src/SQLlib/optlib/optionutil_v4.c
@set SRCS= %SRCS%   %S%/src/translationlib/translate.c

@set SRCS= %SRCS%   %S%/src/deadstart/deadstart_core.c 
 
del sack_typelib.c
del sack_typelib.h

@set OPTS=
@set OPTS=%OPTS%	-I${SACK_BASE}/src/contrib/sqlite/3.23.0-MySqlite

c:\tools\ppc.exe -c -K -once -ssio -sd %OPTS% -I%S%/include -p -osack_ucb_full.cc -DINCLUDE_LOGGING %SRCS%

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% %S%/../include/stdhdrs.h
@set HDRS= %HDRS% %S%/../include/network.h
@set HDRS= %HDRS% %S%/../include/html5.websocket.h
@set HDRS= %HDRS% %S%/../include/html5.websocket.client.h
@set HDRS= %HDRS% %S%/../include/json_emitter.h
@set HDRS= %HDRS% %S%/../include/jsox_parser.h
@set HDRS= %HDRS% %S%/../include/configscript.h
@set HDRS= %HDRS% %S%/../include/filesys.h
@set HDRS= %HDRS% %S%/../include/filemon.h
@set HDRS= %HDRS% %S%/../include/procreg.h
@set HDRS= %HDRS% %S%/../include/http.h
@set HDRS= %HDRS% %S%/../include/pssql.h
@set HDRS= %HDRS% %S%/../include/sqlgetoption.h
@set HDRS= %HDRS% %S%/../include/md5.h
@set HDRS= %HDRS% %S%/../include/sha1.h
@set HDRS= %HDRS% %S%/../include/sha2.h
@set HDRS= %HDRS% %S%/../src/contrib/sha3lib/sha.h

@set HDRS= %HDRS% %S%/../include/idle.h
@set HDRS= %HDRS% %S%/../include/filesys.h
@set HDRS= %HDRS% %S%/../include/sack_vfs.h
@set HDRS= %HDRS% %S%/../include/json_emitter.h
@set HDRS= %HDRS% %S%/../include/vesl_emitter.h
@set HDRS= %HDRS% %S%/../include/jsox_parser.h
@set HDRS= %HDRS% %S%/../include/salty_generator.h
@set HDRS= %HDRS% %S%/../include/sackcomm.h
@set HDRS= %HDRS% %S%/../include/translation.h



c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/../include -p -o../sack_ucb_full.h %HDRS%
cd ..

