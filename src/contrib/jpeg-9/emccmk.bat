
@SET  SYSDEPMEM= jmemnobs.c

: library object files common to compression and decompression
@set  COMSRCS= jaricom.c jcomapi.c jutils.c jerror.c jmemmgr.c %SYSDEPMEM%

: compression library object files
@SET CLIBSRCS= jcarith.c jcapimin.c jcapistd.c jctrans.c jcparam.c jdatadst.c ^
        jcinit.c jcmaster.c jcmarker.c jcmainct.c jcprepct.c ^
        jccoefct.c jccolor.c jcsample.c jchuff.c ^
        jcdctmgr.c jfdctfst.c jfdctflt.c jfdctint.c

: decompression library object files
@SET  DLIBSRCS= jdarith.c jdapimin.c jdapistd.c jdtrans.c jdatasrc.c ^
        jdmaster.c jdinput.c jdmarker.c jdhuff.c ^
        jdmainct.c jdcoefct.c jdpostct.c jddctmgr.c jidctfst.c ^
        jidctflt.c jidctint.c jdsample.c jdcolor.c ^
        jquant1.c jquant2.c jdmerge.c 
: These objectfiles are included in libjpeg.lib

@Set  ExternalExtraDefinitions = -DJPEG_SOURCE -DNO_GETENV
:# ya, this is sorta redundant... should fix that someday.

@set SRCS=%COMSRCS% %CLIBSRCS% %DLIBSRCS%

@SET CFLAGS=%COMMON_CFLAGS% -I../../../include -D__LINUX__


call emcc -g -D_DEBUG -o ./jpeg9.lo %CFLAGS%  -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %SRCS%
@echo on
call emcc -O3 -o ./jpeg9o.lo %CFLAGS% -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference %SRCS%
@echo on
