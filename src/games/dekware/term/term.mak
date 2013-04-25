#
# Borland C++ IDE generated makefile
# Generated 3/11/01 at 4:44:48 PM 
#
.AUTODEPEND


#
# Borland C++ tools
#
IMPLIB  = Implib
BCC32   = Bcc32 +BccW32.cfg 
BCC32I  = Bcc32i +BccW32.cfg 
TLINK32 = TLink32
ILINK32 = Ilink32
TLIB    = TLib
BRC32   = Brc32
TASM32  = Tasm32
#
# IDE macros
#


#
# Options
#
IDE_LinkFLAGS32 =  -LC:\BC5\LIB
IDE_ResFLAGS32 = 
LinkerLocalOptsAtW32_ddbtermdnexdlib =  -Tpd -aa -V4.0 -c
ResLocalOptsAtW32_ddbtermdnexdlib = 
BLocalOptsAtW32_ddbtermdnexdlib = 
CompInheritOptsAt_ddbtermdnexdlib = -IC:\BC5\INCLUDE;..\INCLUDE;\COMMON\INCLUDE -D_RTLDLL
LinkerInheritOptsAt_ddbtermdnexdlib = -x
LinkerOptsAt_ddbtermdnexdlib = $(LinkerLocalOptsAtW32_ddbtermdnexdlib)
ResOptsAt_ddbtermdnexdlib = $(ResLocalOptsAtW32_ddbtermdnexdlib)
BOptsAt_ddbtermdnexdlib = $(BLocalOptsAtW32_ddbtermdnexdlib)

#
# Dependency List
#
Dep_term = \
   ..\term.nex.lib

term : BccW32.cfg $(Dep_term)
  echo MakeNode

..\term.nex.lib : ..\term.nex.dll
  $(IMPLIB) $@ ..\term.nex.dll


Dep_ddbtermdnexddll = \
   ..\..\common\lib\netdll.lib\
   ..\bccplugin.def\
   BCC\ntlink.obj\
   BCC\term.obj

..\term.nex.dll : $(Dep_ddbtermdnexddll)
  $(ILINK32) @&&|
 /v $(IDE_LinkFLAGS32) $(LinkerOptsAt_ddbtermdnexdlib) $(LinkerInheritOptsAt_ddbtermdnexdlib) +
C:\BC5\LIB\c0d32.obj+
BCC\ntlink.obj+
BCC\term.obj
$<,$*
..\..\common\lib\netdll.lib+
C:\BC5\LIB\import32.lib+
C:\BC5\LIB\cw32mti.lib
..\bccplugin.def


|
BCC\ntlink.obj :  ntlink.c
  $(BCC32) -P- -c @&&|
 $(CompOptsAt_ddbtermdnexdlib) $(CompInheritOptsAt_ddbtermdnexdlib) -o$@ ntlink.c
|

BCC\term.obj :  term.c
  $(BCC32) -P- -c @&&|
 $(CompOptsAt_ddbtermdnexdlib) $(CompInheritOptsAt_ddbtermdnexdlib) -o$@ term.c
|

# Compiler configuration file
BccW32.cfg : 
   Copy &&|
-w
-R
-v
-WM-
-vi
-H
-H=term.csm
-WM
-WD
-K
| $@


