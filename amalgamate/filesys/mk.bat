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
@set SRCS= %SRCS%   ../../src/typelib/sets.c
@set SRCS= %SRCS%   ../../src/typelib/binarylist.c 
@set SRCS= %SRCS%   ../../src/typelib/url.c
@set SRCS= %SRCS%   ../../src/typelib/familytree.c
@set SRCS= %SRCS%   ../../src/fractionlib/fractions.c
@set SRCS= %SRCS%   ../../src/vectlib/vectlib.c

@set SRCS= %SRCS%   ../../src/memlib/sharemem.c 
@set SRCS= %SRCS%   ../../src/memlib/memory_operations.c

: Timers and threads (and idle callback registration)
@set SRCS= %SRCS%   ../../src/timerlib/timers.c 
@set SRCS= %SRCS%   ../../src/idlelib/idle.c 

: File system unifications
@set SRCS= %SRCS%   ../../src/filesyslib/pathops.c
@set SRCS= %SRCS%   ../../src/filesyslib/winfiles.c
@set SRCS= %SRCS%   ../../src/filesyslib/filescan.c

: debug logging
@set SRCS= %SRCS%   ../../src/sysloglib/syslog.c

: plain text configuration reader.
@set SRCS= %SRCS%   ../../src/configlib/configscript.c

: system abstraction (launch process, parse arguments, build arguments)
: Also includes environment utilities, and system file paths
:   (paths: CWD, dir of program, dir of this library)@set SRCS= %SRCS%   ../../src/systemlib/args.c
@set SRCS= %SRCS%   ../../src/systemlib/system.c
@set SRCS= %SRCS%   ../../src/systemlib/spawntask.c
@set SRCS= %SRCS%   ../../src/systemlib/args.c
@set SRCS= %SRCS%   ../../src/systemlib/oswin.c

: Process extension; dynamic procedure registration.
: Reading the system configuration brings module support
: which requires this.
@set SRCS= %SRCS%   ../../src/procreglib/names.c


@set SRCS= %SRCS%   ../../src/utils/virtual_file_system/vfs.c
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


@set SRCS= %SRCS%   ../../src/deadstart/deadstart_core.c 
 

del sack_ucb_filelib.c
del sack_ucb_filelib.h

c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -osack_ucb_filelib.c -DINCLUDE_LOGGING %SRCS%

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
@set HDRS= %HDRS% ../../../include/sack_vfs.h
@set HDRS= %HDRS% ../../../include/md5.h
@set HDRS= %HDRS% ../../../include/sha1.h
@set HDRS= %HDRS% ../../../include/sha2.h
@set HDRS= %HDRS% ../../../src/contrib/sha3lib/sha.h
@set HDRS= %HDRS% ../../../include/salty_generator.h


c:\tools\ppc.exe -c -K -once -ssio -sd -I../../../include -p -o../sack_ucb_filelib.h %HDRS%
cd ..

set MOREFLAGS="-DTARGETNAME=""a.exe"""
set MOREFLAGS=%MOREFLAGS% -Wno-parentheses
gcc %MOREFLAGS% -c -g -o a.o sack_ucb_filelib.c

set MOREFLAGS-OPT="-DTARGETNAME=""a-opt.exe"""
set MOREFLAGS-OPT=%MOREFLAGS-OPT% -Wno-parentheses
gcc %MOREFLAGS-OPT% -c -O3 -o a-opt.o sack_ucb_filelib.c

: - ws2_32 
: - iphlpapi 
: - crypt32
: - rpcrt4 
: - odbc32 
: - ole32

gcc %MOREFLAGS% -g -o a.exe sack_ucb_filelib.c test.c -lwinmm -lpsapi -lntdll -lole32 -lws2_32
gcc %MOREFLAGS% -O3 -o a-opt.exe sack_ucb_filelib.c test.c -lwinmm -lws2_32 -liphlpapi -lrpcrt4 -lodbc32 -lpsapi -lntdll -lcrypt32 -lole32

gcc %MOREFLAGS% -g -o a.exe a.o test.c -lwinmm -lws2_32 -liphlpapi  -lrpcrt4 -lodbc32 -lpsapi -lntdll -lcrypt32  -lole32
gcc %MOREFLAGS-OPT% -O3 -o a-opt.exe a-opt.o test.c -lwinmm -lws2_32 -liphlpapi -lrpcrt4 -lodbc32 -lpsapi -lntdll -lcrypt32 -lole32

:_CRT_NONSTDC_NO_DEPRECATE;NO_OPEN_MACRO;_DEBUG;NO_OPEN_MACRO;__STATIC__;USE_SQLITE;USE_SQLITE_INTERFACE;FORCE_COLOR_MACROS;NO_OPEN_MACRO;__STATIC__;NO_FILEOP_ALIAS;_CRT_SECURE_NO_WARNINGS;NEED_SHLAPI;NEED_SHLOBJ;JSON_PARSER_MAIN_SOURCE;SQLITE_ENABLE_LOCKING_STYLE=0;MINIMAL_JSON_PARSE_ALLOCATE