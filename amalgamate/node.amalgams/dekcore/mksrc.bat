:::
:   
:   
:   

@set S=../../..
@set SRCS=


@set SRCS= 
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/nexus.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/commands.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/datacmds.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/entcmds.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/input.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/interface.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/plugins.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/sentcmds.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/syscmds.c
@set SRCS= %SRCS%   %S%/src/games/dekware/syscore/varcmds.c


del sack_dekware.h
del sack_dekware.cc

set OPTS=
set OPTS=%OPTS% -I%S%/src/games/dekware/include

:c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/include %OPTS% -p -osack_dekware.c -DINCLUDE_LOGGING %SRCS%
c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/include %OPTS% -p -osack_dekware.cc -DINCLUDE_LOGGING %SRCS%

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% %S%/../src/games/dekware/include/plugin.h



c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/../include -p -o../sack_dekware.h %HDRS%
cd ..



