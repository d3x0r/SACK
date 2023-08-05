


@set SRCS=
@set SRCS= %SRCS%   uuid.c
@set SRCS= %SRCS%   uuid_md5.c
@set SRCS= %SRCS%   uuid_sha1.c
:@set SRCS= %SRCS%   uuid_dce.c
@set SRCS= %SRCS%   uuid_mac.c
@set SRCS= %SRCS%   uuid_prng.c
@set SRCS= %SRCS%   uuid_str.c
@set SRCS= %SRCS%   uuid_time.c
@set SRCS= %SRCS%   uuid_ui128.c
@set SRCS= %SRCS%   uuid_ui64.c

c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -ouuid_amalg.c %SRCS%

c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -ouuid_amalg.h uuid.h
