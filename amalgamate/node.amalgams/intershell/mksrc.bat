:::
:   
:   
:   

@set S=../../..
@set SRCS=


@set SRCS= 
@set SRCS= %SRCS%   %S%/src/InterShell.stable/main.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/banner_button.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/fileopen.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/fonts.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/loadsave.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/macros.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/intershell_plugins.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/page_dialog.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/pages.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/resname.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/security.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/sprites.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/text_label.c
@set SRCS= %SRCS%   %S%/src/InterShell.stable/env_value.c



del sack_intershell.h
del sack_intershell.c

set OPTS=
set OPTS=%OPTS% -I%S%/src/contrib/libmng
set OPTS=%OPTS% -I%S%/src/contrib/zlib-1.2.11
set OPTS=%OPTS% -I%S%/src/contrib/jpeg-9
set OPTS=%OPTS% -I%S%/src/contrib

c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/include %OPTS% -p -osack_intershell.cc -DINCLUDE_LOGGING %SRCS%

mkdir h
copy config.ppc.h h\config.ppc
cd h

@set HDRS=
@set HDRS= %HDRS% %S%/../src/InterShell.stable/intershell_registry.h
@set HDRS= %HDRS% %S%/../src/InterShell.stable/intershell_export.h



c:\tools\ppc.exe -c -K -once -ssio -sd -I%S%/../include -p -o../sack_intershell.h %HDRS%
cd ..



