
@set SRCS=
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/cppmain.c 
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/args.c    
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/mem.c     
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/define.c  
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/links.c   
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/text.c    
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/input.c   
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/fileio.c  
@set SRCS= %SRCS%   ../../makefiles/prog/ppc/expr.c    

del panther_preprocessor.c

c:\tools\ppc.exe -c -K -once -ssio -sd -I../../include -p -opanther_preprocessor.c -DINCLUDE_LOGGING %SRCS%

gcc -O3 -o ppc.exe panther_preprocessor.c

