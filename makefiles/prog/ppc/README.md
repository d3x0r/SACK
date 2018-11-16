
# Pro-Process for C (PPC)

This is a low level preprocess for C.  Generally it process .c files into .i files.

It handles #if expressions, and all proprocessor flow control.

Running 'ppc' without arguments shows usage.
Arguments may be concatentated together after a '-' unless the have a second parameter.


```
These options are the same.  If there is no space, but extra data
after the option, the remaining data in the argument is used as the
parameter.  In this case specifying that '.' is in the include path.

   
   -I.
   
   -I .

```

```
usage:  (options) <files...>
        options to include
        ------------------------------------------
         -[Ii]<path(s)>      add include path to default
         -[Ss][Ii]<path(s)>  add include path to system default
         -[Dd]<symbol>       define additional symbols
         -MF<file>           dump out auto-depend info
         -MT<file>           use (file) as name of target in depend file
         -L                  write file/line info prefixing output lines
         -l                  write file/line info prefixing output lines
                                   (without line directive)
         -K                  emit unknown pragmas into output
         -k                  do not emit unknown pragma (default)
         -c                  keep comments in output
         -p                  keep includes in output (don't output content of include)
         -f                  force / into \ in include statements with paths
         -sd                 skip define processing (if,else,etc also skippped)
         -sl                 skip logic processing (if,else,endif skippped; process defines for include subst)
         -ssio               Skip System Include Out;try to keep #includes that are missing as #include
         -F                  force \ into /
         -[Oo]<file>         specify the output filename; output filename must be set before input(s) are read
                                 -o output "process this file into output.c"
         -once               only load any include once
         -[Zz]#              debug info mode. where number is in ( 1, 2, 4 )
         @<file>              any option read from a file...
  Any option prefixed with a - will force the option off...  --c to force not keeping comments
  Option L is by default on. (line info with #line keyword)
  output default is input name substituing the last character for an i...
                  test.cpp -> test.cpi  test.c -> test.i
         examples :
                ppc source_file.c
                # the following is how to do an amalgamtion
                ppc -sd -ssio -K -c -once -Iinclude -o file_all.c file1.c file2.c file3.c
                ppc -sdssioKconce -Iinclude -o file_all.c file1.c file2.c file3.c
         -? for more help
 (startup directory)/config.ppc is read first.  It is only read once.   Usage is intended to generate
 one file at a time.   It IS possible to do -o file1.i file1.c -o file2.i file2.c but the config.ppc won't
 be read the second time.  However, symbols meant to be kept in an amalgamtion output can be define here, and
 those defines will be kept.
```

## Usage

Double dash on arguments '--' sets a negate option.

@<filename> can be used to specify a file which specifies options.  Each line
parsed into options and used.

A configuration file `config.ppc` is read from the current working directory.  This is treated
as a C header that's pre-included.  It should have #define's that are appropriate for processing
the next bit of code (or rather that are common for processing all code files).  If the option
to skip define processing is used, This file's contents are also emitted as part of the output.

There are no default paths defined.  `-[Ss][Ii]` as in `-si` or `-SI` defines a 'system include path'.
that is a path which is loaded from using `#include <header.h>`.

User include apths are specified using `-I` of `-i` and are paths that are checked first for 
`#include "header.h"`, after these default include paths are checked, then system include paths are checked.


## Notes

If comments are retained, processed sources will be reformatted such that any command that 
follow a line are moved to preceed that line.  


```
   int a; // some useful variable
```

becomes

```
// some useful variable
   int a;
```


## Example

This example command produces a 'file_all.c' from 'file1.c', etc.
Either command line is valid, showing how options CAN be appended together.



```
ppc -sd -ssio -K -c -once -Iinclude -o file_all.c file1.c file2.c file3.c

ppc -sdssioKconce -Iinclude -o file_all.c file1.c file2.c file3.c
```

[Other examples](/amalgamate)

## Internals

This includes an early variation of allocation both shared memory routines and malloc wrapper
allocation routines.  It also includes many of the primitive types, and PTEXT segment library
functionality.