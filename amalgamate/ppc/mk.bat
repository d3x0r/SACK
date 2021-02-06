
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

del panthers_c_preprocessor.c

ppc.exe -c -K -once -ssio -sd -I../../include -p -opanthers_c_preprocessor.c -DINCLUDE_LOGGING %SRCS%

gcc -O3 -o ppc.exe panthers_c_preprocessor.c

echo Please Update https://gist.github.com/d3x0r/8c8ab33cd7130c3c9983e12d354ad067

