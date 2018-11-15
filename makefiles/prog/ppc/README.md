
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
void usage( void )
{
	printf( "usage: %s (options) <files...>\n"), g.pExecName );
	printf( "\toptions to include\n")
				"	------------------------------------------\n" );
	printf( "\t -[Ii]<path(s)>      add include path to default\n") );
	printf( "\t -[Ss][Ii]<path(s)>  add include path to system default\n") );
	printf( "\t -[Dd]<symbol>       define additional symbols\n") );
	printf( "\t -MF<file>           dump out auto-depend info\n") );
	printf( "\t -MT<file>           use (file) as name of target in depend file\n") );
	printf( "\t -L                  write file/line info prefixing output lines\n") );
	printf( "\t -l                  write file/line info prefixing output lines\n") );
	printf(  "	                           (without line directive)\n" ) );
	printf( "\t -K                  emit unknown pragmas into output\n") );
	printf( "\t -k                  do not emit unknown pragma (default)\n") );
	printf( "\t -c                  keep comments in output\n") );
	printf( "\t -p                  keep includes in output (don't output content of include)\n") );
	printf( "\t -f                  force / into \\ in include statements with paths\n" ) );
	printf( "\t -sd                 skip define processing (if,else,etc also skippped)\n" ) );
	printf( "\t -sl                 skip logic processing (if,else,endif skippped; process defines for include subst)\n" ) );
	printf( "\t -ssio               Skip System Include Out;try to keep #includes that are missing as #include\n" ) );
	printf( "\t -F                  force \\ into /\n") );
	printf( "\t -[Oo]<file>         specify the output filename; output filename must be set before input(s) are read\n") );
	printf( "\t                         -o output \"process this file into output.c\"\n" ) );
	printf( "\t -once               only load any include once\n") );
	printf( "\t -[Zz]#              debug info mode. where number is in ( 1, 2, 4 )\n") );
	printf( "\t @<file>              any option read from a file...\n") );
	printf( "  Any option prefixed with a - will force the option off...  --c to force not keeping comments\n") );
	printf( "  Option L is by default on. (line info with #line keyword)\n") );
	printf( "  output default is input name substituing the last character for an i...\n") );
	printf( "\t\t  test.cpp -> test.cpi  test.c -> test.i\n") );
	printf( "\t examples : \n" ) );
	printf( "\t\tppc source_file.c\n" ) );
	printf( "\t\t# the following is how to do an amalgamtion\n" ) );
	printf( "\t\tppc -sd -ssio -K -c -once -Iinclude -o file_all.c file1.c file2.c file3.c\n" ) );
	printf( "\t\tppc -sdssioKconce -Iinclude -o file_all.c file1.c file2.c file3.c\n" ) );
	printf( "\t -? for more help\n") );
	printf( " (startup directory)/config.ppc is read first.  It is only read once.   Usage is intended to generate\n" ) );
	printf( " one file at a time.   It IS possible to do -o file1.i file1.c -o file2.i file2.c but the config.ppc won't\n" ) );
	printf( " be read the second time.  However, symbols meant to be kept in an amalgamtion output can be define here, and\n" ) );
   printf( WIDE(" those defines will be kept.\n" ) );
}
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

(Other examples)[amalgamate]

## Internals

This includes an early variation of allocation both shared memory routines and malloc wrapper
allocation routines.  It also includes many of the primitive types, and PTEXT segment library
functionality.