#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHORT_NAMES_COMMON 1
#define MAX_PARTS 32

// define as \" ?
#ifdef __LINUX__
#define DOUBLEQUOTE "\\\""
// define as "
#else
//#define DOUBLEQUOTE "\\\""
#define DOUBLEQUOTE "\""
#endif

#ifdef ORDER_DEPENDANT_TARGETS_ON_PATTERN_RULES_WORK
#define DEPENDON " | $(RINTDEST)"
#else
#define DEPENDON
#endif


/* someday we should start a document about what features this make system has, and how to use it.
 * Every project is one of serveral things, a program(windows/console) a library (dynamic/static)
 * a plugin (a dynamic library with a specific name)
 *
 * a certain makefile may contain 1 project and use makefile.single - the following defined
 * names without modification.  It may have several components, each of which is built in order
 * and the makefile.many file should be included, each key defined below (most) then are appended
 * with a number/letter.  The letters are A-F capital case - up to 15 projects may be defined in a single
 * makefile.
 *
 * This make system can produce 4 targets.  The type of target may come from either the
 * command line or an environment variable.
 *
 *   debug   - which contains compiler/system specific debug
 *             information, and defines a flag _DEBUG which may be used to trigger alternate code
 *             definition.  This flag is defined for both C sources and ASM sources.
 *             targets/components are built into a sub directory from where the source is
 *             named 'debug'.  Final products are copied/built into $(FINALDEST)/[bin/lib]/debug.
 *   release - Intended to use optimized compiling which often confuses debuggers.  Also
 *             without the _DEBUG flag defined, the source may exclude many extra
 *             safety checks to provide more optimal performance.
 *             targets/components are built into a sub directory from where the source is
 *             named 'release'.  Final products are copied/built into $(FINALDEST)/[bin/lib]/release.
 *   static_debug - again - compilation is done with debug optimization and debug info stored.
 *                  If the target is a library, it will be a static archive type library.
 *                  a program will be linked with static libraries also.  This provides
 *                  for programs which don't need a ton of DLL/.so's to run, but in turn results
 *                  in much bigger programs.
 *   static_release - Same as release, except built statically... see static_debug/release above.
 *
 *
 *   based on the target type certain STATIC is defined for use in the makefiles.  as well as __STATIC__
 *   for use within source files.
 *
 * This is a list of usable options within a Makefile.  Following this
 * are environment variables that apply.
 *
 * -------- TARGETS -------
 *
 *   PROGNAME (target)
 *      build a program.  See related SUBSYSTEM key.  The program name alone
 *      is specified here, without extension.  Windows systems will result in .exe.
 *      a linux system will result in this name itself.
 *      The program name may be suffixed with a 's' before any applicable extension.
 *      this denotes that the program was linked statically.  This avoids conflicts
 *      with building both static and dynamically linked programs at the same time.
 *
 *   SUBSYSTEM (target option)
 *      When building a windows program, subsystem applies.  This option has no effect on
 *      Linux systems.  Possible options for this are 'windows' or 'console' lower case,
 *      without the single quotes.  A 'windows' subsystem program should have a WinMain().
 *      a 'console' program should have a main().  This is merely a guideline, and
 *      certain combinations of options and compilers will require one or the other.
 *
 *   LIBNAME (target)
 *      This will build a library.  Depending on whether the destination to build
 *      is static or not will determine whether this is a shared library or a static
 *      library.  The name itself will be mangled appropriately on various systems.
 *      for example - Linux/other like to prepend 'lib' and append '.so' to shared libraries
 *      and again prepend 'lib' and append '.a' to static libraries.
 *      Additionally library names will be built with an addtional 's' if static build is
 *      required, this avoids conflics with the link-library required for windows.
 *
 *   LITERAL_LIBNAME (target)
 *      This behaves much like 'LIBNAME' but no name mangling is done.  When building 
 *      static products, this generates a static library of the appropriate name
 *
 *   PLUGIN_NAME (target)
 *      This behaves much like 'LIBNAME' but no name mangling is done.  It is assumed
 *      that this will be a dynamic library, even when building static products.
 *
 *   APP_DEFAULT_DATA (target)
 *      Files listed here are copied to $(FINALDEST) after sources are built.
 *
 * -------- TARGET results -------
 *   Programs, and shared DLL libraries will resuilt in $(FINALDEST)$(BINPATH)/[debug/release].
 *   Shared libraries on linux result in $(FINALDEST)$(BINLIBPATH)/[debug/release].
 *   Static libraries and export libraries (for linking with DLLs) result in
 *   $(FINALDEST)$(LIBPATH)/[debug/release].
 *   APP_DEFAULT_DATA is output to $(FINALDEST)$(DATAPATH)/[debug/release]
 *
 * -------- TARGET Reagents (components/sources) -------
 *
 *   SRCS
 *      This is a list of source files which shall be compiled and linked into the
 *      final target.  Names are listed without suffixes, and will be matched using
 *      stem rules with .c or .asm.  There should one day also be a rule for .cpp, .cxx.
 *      The resulting object will be this name, appended with the project ID (number)
 *      related to building this component, and the system's respective object extension.
 *      Resulting objects are built into a destination directory [debug/release].  Some
 *      systems also produce the base name + project ID + .i when using a preprocessor
 *      before actual compilation.  The result name + project ID + .d will be built
 *      to contain the specific dependancies the source file uses (include files).
 *
 *   CXFLAGS
 *      additional defines/include paths which will be passed to the compiler.
 *      this should never contain compiler specific directions... since sometimes
 *      a preprocessor used, and these options are then passed there.
 *
 *   INCLUDE_DIRS
 *
 *
 *   RSRCS
 *      This defines a resource file for use in windows programs.  Rumor has it that
 *      a QNX environment also supports a form of resources.  This is the filename
 *      without extension, then extension is assumed to be .rc (a text source resource).
 *
 * -------- Library Requirement specifications ---------
 *
 *   COMMON_LIBS - ALL Projects in this makefile get these libraries added
 *     - does not take a numeric identifier
 *   COMMON_OBJS - ALL Projects in this makefile get these 'object files' (literal libname shared objects) added
 *     - does not take a numeric identifier
 *   COMMON_SYSLIBS - ALL Projects in this makefile get these system libraries added
 *     - does not take a numeric identifier
 *
 *   MOREOBJS - add more literal object names which are beyond the scope of
 *              standard link tools.
 *
 *   MORE_LIBS - depricated use MORELIBS
 *   SYSLIBS - specify libraries from the common compiler system you are using
 * deprecated
 *   SACK_LIBS - depricated - please use MORELIBS
 *   MORELIBS - addtional base libraries names to link against
 *
 *   LIBDIRS - additional directories where one might find libraries
 *   LINK_LIBDIRS - depricated(?) - specify addtional libraries to encode into
 *                  library/executable product.
 *
 *
 * ------- Configuration Keys -------------
 *   This is a rather complex section on how to maintain/write new .$(COMPILER).config files.
 *   REQUIRE_SHORT_NAMES
 *   C_LIB_FLAGS
 *   C_DLL_FLAGS
 *   C_PROG_FLAGS
 *   CPP_LIB_FLAGS
 *   CPP_DLL_FLAGS
 *   CPP_PROG_FLAGS
 *   CC
 *   CCRULE
 *     this didn't work... it's defined too late to be useful...
 *   CPP_ABLE = 1  * set this if CXX, CXXRULE get defined.
 *   CXX
 *   CXXRULE
 *   ASM
 *   LD
 *   LDRULE
 *   AR
 *   USES_RPATH
 *
 *  make-sys file
 *
 * ------- Environment Variables --------
 *   __LINUX__
 *
 *   DEFAULT_DEST
 *
 *   FINALDEST (also in makefiles)
 *
 *   COMPILER
 *
 *   SACK_BASE
 *
 *   x_BASE
 *
 * deprecated
 *   COMMONMAKE
 *
*/

int main( int argc, char **argv )
{
	int i;
        int _3point80 = 0;
	FILE *out, *subout;
	char fname[256];
#ifdef __LINUX__
        system( "$MAKE -v >makever" );
#else
        system( "%MAKE% -v >makever" );
#endif
	{
			  FILE *in;
			  in= fopen( "makever", "rt" );
                if( in )
                {
                	char line[256];
                	fgets( line, sizeof( line ), in );
                        if( strstr( line, "3.80" ) || strstr( line, "3.81" ) || strstr( line, "3.82" ) )
                            _3point80 = 1;
                	fclose( in );
	                unlink( "makever" );
        	}
		  }
		  printf( "Using make 3.80? %s\n", _3point80?"yes":"no" );
		  snprintf( fname, sizeof( fname ), "%s/makefile.mr", argv[1] );
        unlink( fname );
	out = fopen( fname, "wb" );
	fprintf( out, "###################################\n" );
	fprintf( out, "# common steps to setup common config\n" );
	fprintf( out, "###################################\n" );
	fprintf( out, "\n" );


	fprintf( out, "\n" );
	fprintf( out, "BUILD_DEP:=$(SACK_BASE)/makefiles/$(RINTDEST)/makefile.many.real $(BUILD_DEP)\n" );
	fprintf( out, "\n" );

	fprintf( out, "\n" );
	fprintf( out, "OUTNAMES=\n" );
	fprintf( out, "OBJLIST=\n" );
	fprintf( out, "DEPLIST=\n" );

	fprintf( out, "\n" );
   fprintf( out, "include $(SACK_BASE)/makefiles/$(RINTDEST)/make-sys\n" );
	fprintf( out, "$(SACK_BASE)/makefiles/$(RINTDEST)/make-sys: ;\n" );
	fprintf( out, "\n" );
	// moved to makefile.many to be built before cflags is set...
   // these end up being common includes for all sub-projects in a makefile.
	//fprintf( out, "INCLUDEDIRS+=$(SACK_BASE)/include\n" );
	//fprintf( out, "INCLUDEPATH=$(foreach IPATH,$(INCLUDEDIRS),-I$(IPATH) )\n" );
	fprintf( out, "\n" );

	for( i = 1; i < MAX_PARTS; i++ )
	{
//      fprintf( out, "$(SACK_BASE)/makefiles/make-dfs.%X: $(SACK_BASE)/makefiles/mm$(PROGEXT)\n", i );
//      fprintf( out, "\t$(RUNMM)\n" );
		fprintf( out, "ifneq '$(MORE_TARGETS%X)$(PROGNAME%X)$(LIBNAME%X)$(LITERAL_LIBNAME%X)$(PLUGIN_NAME%X)$(APP_DEFAULT_DATA%X)' ''\n", i, i, i, i, i, i );
		fprintf( out, "include $(SACK_BASE)/makefiles/$(RINTDEST)/make-dfs.%X\n", i );
		fprintf( out, "endif\n" );
		fprintf( out, "\n" );
	}

	//fprintf( out, "SUB_INCLUDE_PROJECTS=1\n" );
   //fprintf( out, "export SUB_INCLUDE_PROJECTS\n" );
	//fprintf( out, "ifdef PROJECTS\n" );
	//fprintf( out, "-include $(SACK_BASE)/makefiles/makefile.projects\n" );
	//fprintf( out, "OUTNAMES:=$(OUTNAMES) projectall\n" );
	//fprintf( out, "endif\n" );
	//fprintf( out, "\n" );

	fprintf( out, "\n" );
	//fprintf( out, "$(OUTNAMES): |$(INTDEST)\n" );
	fprintf( out, "final_targets: $(OUTNAMES) | $(sort $(foreach outname,$(OUTNAMES),$(subst /_,,$(dir $(outname))_)))\n" );
	fprintf( out, "\n" );

	// no makefile.cache rules in makefile.many.real...
   // think there's something about the sort(fora ll libs...)

	for( i = 1; i < MAX_PARTS; i++ )
	{
		char name[32];
		sprintf( name, "%s/make-dfs.%X", argv[1], i );
		unlink( name );
		subout = fopen( name, "wb" );
		fprintf( subout, "ifneq \'$(FINALDEST)\' \'\'\n");
		fprintf( subout, "MORE_TARGET_LIST := $(MORE_TARGET_LIST) $(MORE_TARGETS%X)\n", i );
		fprintf( subout, "OUTNAMES := $(OUTNAMES) $(MORE_TARGETS%X) $(foreach FILE,$(APP_DEFAULT_DATA%X),$(FINALDEST)$(DATAPATH)$(DEST_SUFFIX)/$(FILE))\n", i, i );
		fprintf( subout, "endif" );
		fprintf( subout, "\n" );
		fprintf( subout, "OBJLIST+=$(OBJS_%X)\n", i );
		fprintf( subout, "ifndef NO_DEPENDS\n" );
		fprintf( subout, "DEPLIST+=$(DEPS_%X)\n", i );
		fprintf( subout, "endif\n" );                
		fprintf( subout, "ifdef SHARED_ONLY\n" );
		fprintf( subout, " ifdef STATIC\n" );
		fprintf( subout, "   LIBDIRS%X+=$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)\n", i );
		fprintf( subout, " else\n" );
		fprintf( subout, "   LIBDIRS%X+=$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)\n", i );
		fprintf( subout, " endif\n" );
		fprintf( subout, "else\n" );
		fprintf( subout, "  LIBDIRS%X+=$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)\n", i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "ifeq '$(SUBSYSTEM%X)' ''\n", i );
		fprintf( subout, "SUBSYSTEM%X=console\n", i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "ifdef __LINUX__\n" );
		fprintf( subout, "DEFNAME%X=\n", i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "ifneq '$(RSRCS_IDS%X)' ''\n", i );
		fprintf( subout, " SRCS%X:=$(SACK_BASE)/include/resource_registry/resname $(SRCS%X)\n", i, i );
#ifdef __LINUX__
		fprintf( subout, " CXFLAGS%X:=$(CXFLAGS%X) -DRESOURCE_FILE_NAME=\\\"$(RSRCS_IDS%X)\\\"\n", i, i, i );
#else
		fprintf( subout, " CXFLAGS%X:=$(CXFLAGS%X) -DRESOURCE_FILE_NAME=\"$(RSRCS_IDS%X)\"\n", i, i, i );
#endif
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "# literal libnames dynamic linked\n" );
		fprintf( subout, "# are assumed to be things which are dynamically\n" );
		fprintf( subout, "# loaded -- specifically - by - name -\n" );
		fprintf( subout, "ifneq '$(LITERAL_LIBNAME%X)' ''\n", i );
		fprintf( subout, " SRCS%X:=$(ALL_PRE_OBJS) $(SRCS%X) $(ALLOBJS) $(LIBOBJS) $(ALL_POST_OBJS)\n", i, i );
		fprintf( subout, " XCXFLAGS%X+=$(C_DLL_FLAGS) -DTARGETNAME=" DOUBLEQUOTE "$(LITERAL_LIBNAME%X)$(STATIC)" DOUBLEQUOTE "\n", i, i );
		fprintf( subout, " XCXFLAGS%X+=-DTARGET_LABEL=$(subst -,_,$(subst .,_,$(LITERAL_LIBNAME%X)$(STATIC)))\n", i, i );
		fprintf( subout, " XCPPFLAGS%X+=$(CPP_DLL_FLAGS)\n", i );
		fprintf( subout, " #OUT_%X_PROGRAM=0\n", i );
		fprintf( subout, " OUT_%X_LIBRARY=1\n", i );
      fprintf( subout, " OUT_%X_LITERAL=yes\n", i );
		fprintf( subout, " LDFLAGS_%X=$(LDFLAGS) $(MAKE_DLL_OPT)\n", i );
		fprintf( subout, " ifdef STATIC\n" );
		fprintf( subout, "  OUTNAME_SHORT_%X:=$(LITERAL_LIBNAME%X)$(STATIC_LITERAL_NAME)\n", i, i );
		fprintf( subout, " else\n" );
		fprintf( subout, "  OUTNAME_SHORT_%X:=$(LITERAL_LIBNAME%X)\n", i, i );
		fprintf( subout, " endif\n" );
		fprintf( subout, " OUTNAME_%X:=$(INTDEST)/$(OUTNAME_SHORT_%X)\n", i, i );
		fprintf( subout, " AUTOLIBS:=$(AUTOLIBS) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)\n", i );
		fprintf( subout, " OUTNAMES:=$(OUTNAMES) $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X)\n", i );
		fprintf( subout, " ifneq '$(DEFNAME%X)' ''\n",i );
		fprintf( subout, "  ifneq '$(LD_DEF_OPT)' ''\n" );
		fprintf( subout, "   LDFLAGS_%X+=$(LD_DEF_OPT)$(DEFNAME%X)\n", i,i );
		fprintf( subout, "  endif\n" );
		fprintf( subout, " endif\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "ifneq '$(LIBNAME%X)' ''\n", i );
		fprintf( subout, " SRCS%X:=$(ALL_PRE_OBJS) $(SRCS%X) $(ALLOBJS) $(LIBOBJS) $(ALL_POST_OBJS)\n", i, i );
		fprintf( subout, " #OUT_%X_PROGRAM=0\n", i );
		fprintf( subout, " OUT_%X_LIBRARY=1\n", i );
      fprintf( subout, " OUT_%X_LITERAL=\n", i );
		fprintf( subout, " ifdef STATIC\n" );
      fprintf( subout, "  XCXFLAGS%X+=$(C_LIB_FLAGS) -DTARGETNAME=" DOUBLEQUOTE "$(LIBNAMEPREFIX)$(LIBNAME%X)$(STATIC)$(LIBEXT)" DOUBLEQUOTE "\n", i, i );
		fprintf( subout, "  XCXFLAGS%X+=-DTARGET_LABEL=$(subst -,_,$(subst .,_,$(LIBNAMEPREFIX)$(LIBNAME%X)$(STATIC)$(LIBEXT)))\n", i, i );
      fprintf( subout, "  XCPPFLAGS%X+=$(CPP_LIB_FLAGS)\n", i );
		fprintf( subout, "  LDFLAGS_%X=$(LDFLAGS) $(LDXFLAGS%X)\n", i, i );
		fprintf( subout, "  OUTNAME_SHORT_%X:=$(LIBNAME%X)$(STATIC)\n", i, i );
		fprintf( subout, "  OUTNAME_%X:=$(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)\n", i, i );
		fprintf( subout, "   OUTNAMES:=$(OUTNAMES) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)\n", i );
		fprintf( subout, " else\n" );
		fprintf( subout, "  XCXFLAGS%X+=$(C_DLL_FLAGS) -DTARGETNAME=" DOUBLEQUOTE "$(LIBNAMEPREFIX)$(LIBNAME%X)$(STATIC)$(SHLIBEXT)" DOUBLEQUOTE "\n", i, i );
		fprintf( subout, "  XCXFLAGS%X+=-DTARGET_LABEL=$(subst -,_,$(subst .,_,$(LIBNAMEPREFIX)$(LIBNAME%X)$(STATIC)$(SHLIBEXT)))\n", i, i );
      fprintf( subout, "  XCPPFLAGS%X+=$(CPP_DLL_FLAGS)\n", i );
		fprintf( subout, "  LDFLAGS_%X=$(LDFLAGS) $(MAKE_DLL_OPT)\n",i );
		fprintf( subout, "  ifneq '$(DEFNAME%X)' ''\n",i );
		fprintf( subout, "   ifneq '$(LD_DEF_OPT)' ''\n" );
		fprintf( subout, "    LDFLAGS_%X+=$(LD_DEF_OPT)$(DEFNAME%X)\n", i,i );
		fprintf( subout, "   endif\n" );
		fprintf( subout, "  endif\n" );
		fprintf( subout, "  OUTNAME_SHORT_%X:=$(LIBNAME%X)\n", i, i );
		fprintf( subout, "  OUTNAME_%X:=$(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT)\n" , i, i);
		fprintf( subout, "  AUTOLIBS:=$(AUTOLIBS) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)\n", i );
		fprintf( subout, "   OUTNAMES:=$(OUTNAMES) $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT)\n", i );
		fprintf( subout, "   ifneq '$(SHARED_ONLY)$(NO_LIB_DEPEND)' '1'\n" );
		fprintf( subout, "    OUTNAMES:=$(OUTNAMES) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)\n", i );
		fprintf( subout, "   endif\n" );
		fprintf( subout, " endif\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "ifneq '$(PROGNAME%X)' ''\n", i );
      fprintf( subout, " XCXFLAGS%X+=$(C_PROG_FLAGS) -DTARGETNAME=" DOUBLEQUOTE "$(PROGNAME%X)$(STATIC)$(PROGEXT)" DOUBLEQUOTE " $(if $(filter console,$(SUBSYSTEM%X)),-DCONSOLE,-DGUI)\n", i, i, i );
		fprintf( subout, " XCXFLAGS%X+=-DTARGET_LABEL=$(subst -,_,$(subst .,_,$(PROGNAME%X)$(STATIC)$(PROGEXT)))\n", i, i );
      fprintf( subout, " XCPPFLAGS%X+=$(CPP_PROG_FLAGS)\n", i );
		fprintf( subout, " SRCS%X:=$(ALL_PRE_OBJS) $(SRCS%X) $(ALLOBJS) $(EXEOBJS) $(ALL_POST_OBJS)\n", i, i );
				 //$(SACK_BASE)/src/deadstart/deadstart\n", i );
#ifndef BAG
		//fprintf( subout, " MORELIBS%X+=syslog\n", i );
#endif
		fprintf( subout, " OUT_%X_PROGRAM=1\n", i );
		fprintf( subout, " OUT_%X_LIBRARY=0\n", i );
      fprintf( subout, " OUT_%X_LITERAL=\n", i );
		fprintf( subout, " OUTNAME_SHORT_%X:=$(PROGNAME%X)$(STATIC)\n", i, i );
		fprintf( subout, " OUTNAME_%X:=$(INTDEST)/$(OUTNAME_SHORT_%X)$(PROGEXT)\n", i, i );
		fprintf( subout, " LDFLAGS_%X+=$(LDFLAGS) $(LD_EXEFLAGS) $(LDXFLAGS%X)\n", i, i, i );
		fprintf( subout, " ifneq '$(SUBSYSOPT)' ''\n" );
		fprintf( subout, "  LDFLAGS_%X+=$(SUBSYSOPT)$(SUBSYSTEM%X)\n", i, i );
		fprintf( subout, " endif\n" );
		fprintf( subout, " OUTNAMES:=$(OUTNAMES) $(if $(LOCAL_ONLY%X),$(INTDEST)/$(OUTNAME_SHORT_%X)$(PROGEXT),$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X)$(PROGEXT))\n", i, i, i );
      fprintf( subout, " ifndef NO_PROGLIB\n" );
		fprintf( subout, "  ifdef PROG_MAKELIB%X\n", i );
		fprintf( subout, "   OUTNAMES:=$(OUTNAMES) $(if $(LOCAL_ONLY%X),$(INTDEST)/$(LIBPREFIX)$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(STATIC)$(LIBEXT),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(STATIC)$(LIBEXT))\n", i, i, i );
		fprintf( subout, "  endif\n" );
		fprintf( subout, " endif\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
      //---- Resources
		fprintf( subout, " ifndef __LINUX__\n" );
		fprintf( subout, "  ifneq  '$(RSRCS%X)' ''\n", i );
		fprintf( subout, "   RESOURCE_%X=$(call MAKE_EXT,res,%X,$(dir $(RSRCS%X))$(RINTDEST)/$(notdir $(RSRCS%X)))\n", i, i, i, i );
		fprintf( subout, "   ALL_PATHS+=$(subst /_,,$(dir $(RESOURCE_%X))_)\n", i );
		fprintf( subout, "$(RESOURCE_%X): $(RSRCS%X).rc | $(subst /_,,$(dir $(RESOURCE_%X))_)\n", i, i, i );
		fprintf( subout, "\t$(TARGET)\n" );
		fprintf( subout, "\t$(QUIETCMD)$(RC) -I$(dir $<) $< $(RC_OUT_OPT)$@ \n" );
		fprintf( subout, "  else\n" );
		fprintf( subout, "   RESOURCE_%X=\n", i );
		fprintf( subout, "  endif\n" );
		fprintf( subout, " endif\n" );


		fprintf( subout, "\n" );
		fprintf( subout, "ifdef USES_RPATH\n" );
		fprintf( subout, "ifdef __LINUX__\n" );
		fprintf( subout, "LIBPATH_%X:=$(foreach LPATH, $(LIBDIRS%X), $(LIBPATHOPT)$(LPATH) -Wl,-rpath,$(LPATH) -Wl,-rpath-link,$(LPATH) )\n", i, i );
		fprintf( subout, "LIBPATH_%X:=$(LIBPATH_%X) $(foreach LPATH, $(LINK_LIBDIRS%X), $(LIBPATHOPT)$(LPATH) -Wl,-rpath,$(LPATH) -Wl,-rpath,$(LPATH) )\n", i, i, i );
		fprintf( subout, "else\n" );
		fprintf( subout, "LIBPATH_%X:=$(foreach LPATH, $(LIBDIRS%X), $(LIBPATHOPT)$(LPATH))\n", i, i );
		fprintf( subout, "LIBPATH_%X:=$(LIBPATH_%X) $(foreach LPATH, $(LINK_LIBDIRS%X), $(LIBPATHOPT)$(LPATH))\n", i, i, i );
		fprintf( subout, "endif\n" );
      fprintf( subout, "else\n" );
		fprintf( subout, "LIBPATH_%X:=$(foreach LPATH, $(LIBDIRS%X), $(LIBPATHOPT)$(LPATH) )\n", i, i );
		fprintf( subout, "LIBPATH_%X:=$(LIBPATH_%X) $(foreach LPATH, $(LINK_LIBDIRS%X), $(LIBPATHOPT)$(LPATH) )\n", i, i, i );
      fprintf( subout, "endif\n" );
		fprintf( subout, "LIBLIST_%X:=$(foreach LIB, $(SACK_LIBS%X), $(LIBPREFIX)$(LIBNAMEPREFIX)$(LIB)$(STATIC)$(LINK_LIBEXT) )\n", i, i );
		fprintf( subout, "LITERAL_LIBLIST_%X:=$(LITERAL_LIBLIST_%X) $(MORE_LITERAL_LIBS%X)\n", i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(MORE_LIBS%X), $(LIBPREFIX)$(LIB)$(STATIC)$(LINK_LIBEXT) )\n", i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(MORELIBS%X), $(LIBPREFIX)$(LIB)$(STATIC)$(LINK_LIBEXT) )\n", i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(SYSLIBS%X), $(LIBPREFIX)$(LIB)$(LINK_LIBEXT) )\n", i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(filter-out $(LIBNAME%X),$(COMMON_LIBS)), $(LIBPREFIX)$(LIB)$(STATIC)$(LINK_LIBEXT) )\n", i, i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(COMMON_SYSLIBS), $(LIBPREFIX)$(LIB)$(LINK_LIBEXT) )\n", i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(MOREOBJS%X), $(if $(findstring /,$(LIB)),$(LIB),$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIB)) )\n", i, i, i );
		fprintf( subout, "LIBLIST_%X:=$(LIBLIST_%X) $(foreach LIB, $(COMMON_OBJS), $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIB) )\n", i, i );
		fprintf( subout, "ifdef SHARED_ONLY\n" );
		fprintf( subout, " ifndef STATIC\n" );
		fprintf( subout, "   LIBDEPEND_%X:=$(LIBDEPEND_%X)$(foreach LIB, $(filter-out $(LIBNAME%X),$(COMMON_LIBS)) $(MORE_LIBS%X) $(MORELIBS%X) $(SACK_LIBS%X), $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(LIB)$(STATIC)$(SHLIBEXT) )\n", i, i, i, i, i, i, i );
		fprintf( subout, " else\n" );
		fprintf( subout, "   LIBDEPEND_%X:=$(LIBDEPEND_%X)$(foreach LIB, $(filter-out $(LIBNAME%X),$(COMMON_LIBS)) $(MORE_LIBS%X) $(MORELIBS%X) $(SACK_LIBS%X), $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(LIB)$(STATIC)$(LIBEXT) )\n", i, i, i, i, i, i, i );
		fprintf( subout, " endif\n" );
		fprintf( subout, "else\n" );
		fprintf( subout, "  LIBDEPEND_%X:=$(LIBDEPEND_%X)$(foreach LIB, $(filter-out $(LIBNAME%X),$(COMMON_LIBS)) $(MORE_LIBS%X) $(MORELIBS%X) $(SACK_LIBS%X), $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(LIB)$(STATIC)$(LIBEXT) )\n", i, i, i, i, i, i, i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "VPATH:=$(sort . $(VPATH) $(subst /:,,$(foreach OBJ,$(SRCS%X),$(dir $(OBJ)): )))\n", i, i );
		fprintf( subout, "VPATH_%X:=$(filter-out .,$(sort $(subst /:,,$(foreach OBJ,$(SRCS%X),$(dir $(OBJ)): ))))\n", i, i, i );
		fprintf( subout, "\n" );

		// require short names
#ifndef SHORT_NAMES_COMMON
		fprintf( subout, "ifdef REQUIRE_SHORT_NAMES\n" );
		fprintf( subout, "OBJS_%X:=$(foreach OBJ,$(SRCS%X),$(call OBJNAME,%X,$(call filterdestpath,$(OBJ))) )\n", i, i, i );
		fprintf( subout, "LOBJS_%X:=$(OBJS_%X)\n", i, i );
		fprintf( subout, "ifndef NO_DEPENDS\n" );
		fprintf( subout, "DEPS_%X:=$(OBJS_%X:$(call MAKE_EXT,$(OBJEXT),%X,):$(call MAKE_EXT,$(OBJEXT),%X,))\n", i, i, i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "\n" );

		fprintf( subout, "$(INTDEST)/%%.%Xrs:$(CURDIR)/%%.rc\n", i );
		fprintf( subout, "\t$(RC) $(RC_OUT_OPT)$@ $*.rc\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT): CXFLAGSX=$(CXFLAGS%X)\n", i, i );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT): XCXFLAGSX=$(XCXFLAGS%X)\n", i, i );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT): XCPPFLAGSX=$(XCPPFLAGS%X)\n", i, i );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT):$(CURDIR)/%%.c"DEPENDON"\n", i );
		fprintf( subout, "\t$(call SRC_DEPENDS,$<)\n" );
		fprintf( subout, "\t$(TARGET)\n" );
		fprintf( subout, "ifdef CCRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call CCRULE,$(CFLAGS) $(CXFLAGSX),$@,$<,$(@:.%X$(OBJEXT)=.d),$(XCXFLAGSX),$(XCPPFLAGSX)))\n", i );
		fprintf( subout, "else\n" );
		fprintf( subout, "\t$(call DOCMD,$(CC) $(CFLAGS) $(CXFLAGSX) $(XCXFLAGSX) $(XCPPFLAGSX) $(OUTNAMEOPT)$@ $<)\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\t$(call DOCMD,$(call MAKEDEPEND,$(MAKEDEPLIST),$(@:$(OBJEXT)=d)))\n" );
		fprintf( subout, "\n" );

		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT): CXFLAGSX=$(CXFLAGS%X)\n", i, i );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT): XCXFLAGSX=$(XCXFLAGS%X)\n", i, i );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT): XCPPFLAGSX=$(XCPPFLAGS%X)\n", i, i );
		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT):$(CURDIR)/%%.cpp"DEPENDON"\n", i );
		fprintf( subout, "\t$(call SRC_DEPENDS,$<)\n" );
		fprintf( subout, "\t$(TARGET)\n" );
		fprintf( subout, "ifdef CXXRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call CXXRULE,$(CFLAGS) $(CXFLAGSX),$@,$<,$(@:.%X$(OBJEXT)=.d),$(XCXFLAGSX),$(XCPPFLAGSX)))\n", i );
		fprintf( subout, "else\n" );
		fprintf( subout, "\t$(call DOCMD,$(CXX) $(CFLAGS) $(CXFLAGSX) $(XCXFLAGSX) $(XCPPFLAGSX) $(OUTNAMEOPT)$@ $<)\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\t$(call DOCMD,$(call MAKEDEPEND,$(MAKEDEPLIST),$(@:$(OBJEXT)=d)))\n" );
		fprintf( subout, "\n" );

		fprintf( subout, "$(INTDEST)/%%.%X$(OBJEXT):$(CURDIR)/%%.asm"DEPENDON"\n", i );
      fprintf( subout, "\t$(call SRC_DEPENDS,$<)\n" );
      fprintf( subout, "\t$(TARGET)\n" );
		fprintf( subout, " ifdef ASMRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call ASMRULE,$@, $<))\n");
		fprintf( subout, " else\n" );
		fprintf( subout, "\t$(call DOCMD,$(ASM) $(AFLAGS) -o $@ $<)\n" );
#ifdef _WIN32
		//fprintf( subout, "\t$(call DOCMD,echo $(subst \\,,$(shell nasm -M $(AFLAGS) $< -o $@)) > $(@:.$(OBJEXT)=.d))\n" );
#else
		//fprintf( subout, "\techo \"$(subst \\,,$(shell nasm -M $(AFLAGS) $< -o $@))\" > $(@:.$(OBJEXT)=.d)\n" );
#endif
		fprintf( subout, " endif\n" );
		// allow long filenames ( and require such :( )
		fprintf( subout, "else\n" );
#endif

		fprintf( subout, "OBJS_%X:=$(foreach OBJ,$(SRCS%X),$(call OBJNAME,%X,$(call filterdestpath,$(OBJ))) )\n", i, i, i );
		fprintf( subout, "LOBJS_%X:=$(OBJS_%X)\n", i, i );
		fprintf( subout, "$(OBJS_%X): $(MKFILE) $(BUILD_DEP)\n", i );
		fprintf( subout, "ifndef NO_DEPENDS\n" );
		fprintf( subout, " DEPS_%X:=$(OBJS_%X:$(call MAKE_EXT,$(OBJEXT),%X,)=$(call MAKE_EXT,d,%X,))\n", i, i, i, i );
		fprintf( subout, " ALL_DEPS:=$(ALL_DEPS) $(DEPS_%X)\n", i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		//fprintf( subout, "\n" );
		//fprintf( subout, "$(INTDEST)/%%%X.res:$(CURDIR)/%%.rc\n", i );
      //fprintf( subout, "\t$(TARGET)\n" );
		//fprintf( subout, "\t$(RC) $(RC_OUT_OPT)$@ $*.rc\n" );
		//fprintf( subout, "\n" );
/*
		fprintf( subout, "$(call OBJNAME,$(%X),$(INTDEST)/%%):$(CURDIR)/%%.cpp"DEPENDON"\n", i );
      fprintf( subout, "\t$(call SRC_DEPENDS,$<)\n" );
      fprintf( subout, "\t$(TARGET)\n" );
		fprintf( subout, "ifdef CXXRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call CXXRULE,$(CFLAGS) $(CXFLAGSX),$@,$<,$(@::$(call OBJEXT,%X,)=$(call DEFEXT,%X,)), $(XCXFLAGSX),$(XCPPFLAGSX)))\n", i, i );
		fprintf( subout, "else\n" );
		fprintf( subout, "\t$(call DOCMD,$(CXX) $(CFLAGS) $(CXFLAGSX) $(XCXFLAGSX) $(XCPPFLAGSX) $(OUTNAMEOPT)$@ $<)\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\t$(call DOCMD,$(call MAKEDEPEND,$(MAKEDEPLIST),$(@::$(call OBJEXT,%X,)=$(call DEFEXT,%X,))))\n", i, i );
		fprintf( subout, "\n" );
      */
/*
		fprintf( subout, "$(INTDEST)/%%%X.$(OBJEXT):$(CURDIR)/%%.asm"DEPENDON"\n", i );
      fprintf( subout, "\t$(call SRC_DEPENDS,$<)\n" );
      fprintf( subout, "\t$(TARGET)\n" );
		fprintf( subout, " ifdef ASMRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call ASMRULE,$@, $<))\n");
		fprintf( subout, " else\n" );
		fprintf( subout, "\t$(call DOCMD,$(ASM) $(AFLAGS) -o $@ $<)\n" );
#ifdef _WIN32
		//fprintf( subout, "\t$(call DOCMD,echo $(subst \\,,$(shell nasm -M $(AFLAGS) $< -o $@)) > $(@:.$(OBJEXT)=.d))\n" );
#else
		//fprintf( subout, "\t$(call DOCMD,echo \"$(subst \\,,$(shell nasm -M $(AFLAGS) $< -o $@))\" > $(@:.$(OBJEXT)=.d))\n" );
#endif
fprintf( subout, " endif\n" );
*/
#ifndef SHORT_NAMES_COMMON
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
#endif

		fprintf( subout, "ifeq '$(VPATH_%X)' ''\n", i );
		fprintf( subout, "Azz%X=$$(foreach vp,. $$(CURDIR),$$(eval $$(call std_obj_from_asm,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "Bzz%X=$$(foreach vp,. $$(CURDIR),$$(eval $$(call std_obj_from_c,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "Czz%X=$$(foreach vp,. $$(CURDIR),$$(eval $$(call std_obj_from_cpp,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "Dzz%X=$$(foreach vp,. $$(CURDIR),$$(eval $$(call std_obj_vars,%X,$$(vp))))\n", i, i );
		fprintf( subout, "Ezz%X=$$(foreach vp,. $$(CURDIR),$$(eval $$(call std_res_from_rc,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "else\n" );
		fprintf( subout, "Azz%X=$$(foreach vp,$$(VPATH),$$(eval $$(call std_obj_from_asm,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "Bzz%X=$$(foreach vp,$$(VPATH),$$(eval $$(call std_obj_from_c,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "Czz%X=$$(foreach vp,$$(VPATH),$$(eval $$(call std_obj_from_cpp,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "Dzz%X=$$(foreach vp,$$(VPATH),$$(eval $$(call std_obj_vars,%X,$$(vp))))\n", i, i );
		fprintf( subout, "Ezz%X=$$(foreach vp,$$(VPATH),$$(eval $$(call std_res_from_rc,%X,$$(vp),$$(vp)/$$(RINTDEST))))\n", i, i );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );
		fprintf( subout, "$(eval $(Azz%X))\n", i);
		fprintf( subout, "$(eval $(Bzz%X))\n", i);
		fprintf( subout, "$(eval $(Czz%X))\n", i);
		fprintf( subout, "$(eval $(Dzz%X))\n", i);
		fprintf( subout, "$(eval $(Ezz%X))\n", i);

      fprintf( subout, "\n" );
		fprintf( subout, "PARTIAL_TARGETS:=$(PARTIAL_TARGETS) $(OBJS_%X)\n", i );

		fprintf( subout, "\n" );
		//fprintf( subout, "$(SACK_BASE)/makefiles/make-dfs.%X: ;\n", i );
		fclose( subout );
	}

	fprintf( out, "$(sort" );
	for( i = 0; i < 16; i++ )
		fprintf( out, " $(LIBDEPEND%X)", i );
	fprintf( out, "): \n" );
	fprintf( out, "\techo Hey how can we make $<\n" );
	fprintf( out, "\n" );

  	for( i = 1; i < MAX_PARTS; i++ )
	{
		//fprintf( out, "$(SACK_BASE)/makefiles/make-rls.%X: ;\n", i );
		fprintf( out, "ifneq '$(PROGNAME%X)$(LIBNAME%X)$(LITERAL_LIBNAME%X)$(PLUGIN_NAME%X)$(APP_DEFAULT_DATA%X)' ''\n", i, i, i, i, i );
		fprintf( out, "include $(SACK_BASE)/makefiles/$(RINTDEST)/make-rls.%X\n", i );
		fprintf( out, "endif\n" );
		fprintf( out, "\n" );
	}

	for( i = 1; i < MAX_PARTS; i++ )
	{
		char name[32];
		sprintf( name, "%s/make-rls.%X", argv[1], i );
		unlink( name );
		subout = fopen( name, "wb" );
		fprintf( subout, " ############## Project make rules for %d\n", i );
      fprintf( subout, "\n" );

		//fprintf( subout, "$(FINALDEST)/bin$(DEST_SUFFIX)/%%:$(CURDIR)/%%\n" );
		////if( _3point80 )
	   ////  fprintf( subout, "$(foreach FILE,$(APP_DEFAULT_DATA%X),$(FINALDEST)/bin$(DEST_SUFFIX)/$(FILE)): $(foreach DATA,$(APP_DEFAULT_DATA%X),$(CURDIR)/$(DATA)) | $(FINALDEST)/bin$(DEST_SUFFIX)\n", i, i );
      ////else
	   ////  fprintf( subout, "$(foreach FILE,$(APP_DEFAULT_DATA%X),$(FINALDEST)/bin$(DEST_SUFFIX)/$(FILE)): $(FINALDEST)/bin/$(INTDEST) $(foreach DATA,$(APP_DEFAULT_DATA%X),$(CURDIR)/$(DATA)) \n", i, i );
      //fprintf( subout, "\t$(FINALTARGET)\n" );
      //fprintf( subout, "\t$(call DOCMD,$(QUIETCMD)cp $< $@) \n",i );
				 //fprintf( subout, "\n" );

		fprintf( subout, "ifneq '$(OUTNAME_SHORT_%X)' ''\n", i );
		fprintf( subout, "\n" );
		fprintf( subout, " ifdef USES_RPATH\n" );
		{
			fprintf( subout, "  ifdef STATIC\n" );
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(OBJS_%X) $(RESOURCE_%X)  $(LIBDEPEND_%X) | $(INTDEST) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)\n",i,i,i,i );
         else
				fprintf( subout, "$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX) $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X) \n",i,i,i,i );
         fprintf( subout, "\t@echo \"linking(1) $(notdir $@)\"\n" );
			fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "\t$(call DOCMD,$(QUIETCMD)rm -f $@)\n" );
			fprintf( subout, "   ifdef ARRULE\n" );
			fprintf( subout, "\t$(call DOCMD,$(call ARRULE,$@,$(LOBJS_%X),$(RESOURCE_%X),$(LITERAL_LIBLIST_%X)))\n", i, i, i );
			fprintf( subout, "   else\n" );
			fprintf( subout, "\t$(call DOCMD,$(AR) $(LN_OUTNAMEOPT)$@ $(LOBJS_%X) $(RESOURCE_%X))\n",i,i );
			fprintf( subout, "   endif\n" );
			fprintf( subout, "  else\n" );
			fprintf( subout, "   ifeq '$(OUT_%X_LIBRARY)' '1'\n", i );
			fprintf( subout, "    $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(if $(OUT_%X_LITERAL),,$(SHLIBEXT))\n", i, i, i );
			fprintf( subout, "	 $(FINALTARGET)\n" );
			fprintf( subout, "   else\n" );
			if( _3point80 )
				fprintf( subout, "#$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X) | $(INTDEST) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)\n", i, i, i, i );
         else
				fprintf( subout, "#$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT):$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)  $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i, i );

			fprintf( subout, "#\t@echo \"linking(2) $(notdir $@)\"\n" );
			fprintf( subout, "#\t$(FINALTARGET)\n" );
			fprintf( subout, "#    ifdef LIBLINKRULE\n" );
			fprintf( subout, "#\t$(call DOCMD,$(call LIBLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT),$(OUT_%X_LITERAL),$(LITERAL_LIBLIST_%X)))\n",i,i,i,i,i,i,i,i,i,i,i,i );
			fprintf( subout, "#    else\n" );
			fprintf( subout, "#\t$(call DOCMD,$(LD) $(LDFLAGS_%X) $(MAPOUTOPT)$(INTDEST)/$(OUTNAME_SHORT_%X).map $(LD_OUTNAMEOPT)$@  $(call LD_INPUT,$(LOBJS_%X)) $(RESOURCE_%X) $(LIBPATH_%X) $(LIBLIST_%X) $(LITERAL_LIBSLIST_%X) $(EXEOBJS) $(call LD_FINAL_INPUT,$(LOBJS_%X),$(LITERAL_LIBLIST_%X)))\n", i, i, i, i, i, i, i, i, i );
			fprintf( subout, "#    endif\n" );
			fprintf( subout, "   endif\n" );
			fprintf( subout, "  endif\n" );
      }
      fprintf( subout, " else\n" );

      { // alternate rules for simple compilers
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT) | $(INTDEST) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)\n", i, i );
         else
				fprintf( subout, "$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT):$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)  $(INTDEST)/$(OUTNAME_SHORT_%X)$(LIBEXT)\n", i, i );
         fprintf( subout, "\t@echo \"linking(3) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "\t$(call DOCMD,cp $< $@)\n" );
		}
      fprintf( subout, " endif\n" );

		fprintf( subout, "\n" );

		fprintf( subout, " ifneq '$(PROGEXT)' ''\n" );
      fprintf( subout, "  ifndef NO_PROGLIB\n" );
		fprintf( subout, "   ifdef PROG_MAKELIB%X\n", i );
		fprintf( subout, "     $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(PROGEXT)\n", i, i );
		fprintf( subout, "\t$(FINALTARGET)\n" );
		fprintf( subout, "   endif\n" );
		fprintf( subout, "  endif\n" );
		fprintf( subout, "  ifdef USES_RPATH\n" );
		{

			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X)$(PROGEXT): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X) | $(INTDEST) $(FINALDEST)$(BINPATH)$(DEST_SUFFIX)\n", i, i, i, i );
      	else
				fprintf( subout, "$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X)$(PROGEXT):$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)  $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i, i );

         fprintf( subout, "\t@echo \"linking(4) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "   ifdef USE_LINK_RULES\n" );
			fprintf( subout, "    ifeq '$(SUBSYSTEM%X)' 'windows'\n", i );
			fprintf( subout, "\t$(call DOCMD,$(call WIN_PRGLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT),$(LITERAL_LIBLIST_%X)))\n",i,i,i,i,i,i,i,i,i,i );
			fprintf( subout, "    else\n" );
			fprintf( subout, "\t$(call DOCMD,$(call CON_PRGLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT),$(LITERAL_LIBLIST_%X)))\n",i,i,i,i,i,i,i,i,i,i );
			fprintf( subout, "    endif\n" );
			fprintf( subout, "   else\n" );
			fprintf( subout, "\t$(call DOCMD,$(LD) $(LDFLAGS_%X) $(MAPOUTOPT)$(INTDEST)/$(OUTNAME_SHORT_%X).map $(LD_OUTNAMEOPT)$@ $(call LD_INPUT,$(LOBJS_%X)) $(RESOURCE_%X) $(LIBPATH_%X) $(LIBLIST_%X) $(call LD_FINAL_INPUT,$(LOBJS_%X),$(LITERAL_LIBLIST_%X)) )\n", i, i, i, i, i, i, i, i );
			fprintf( subout, "   endif\n" );

	   }
      fprintf( subout, "  else\n" );

      { // alternate rules for simple compilers
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X)$(PROGEXT): $(INTDEST)/$(OUTNAME_SHORT_%X)$(PROGEXT) | $(INTDEST) $(FINALDEST)$(BINPATH)$(DEST_SUFFIX)\n", i, i );
      	else
				fprintf( subout, "$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X)$(PROGEXT):$(FINALDEST)$(BINPATH)$(DEST_SUFFIX) $(INTDEST)/$(OUTNAME_SHORT_%X)$(PROGEXT)\n", i, i );
         fprintf( subout, "\t@echo \"linking(5) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "\t$(call DOCMD,cp $< $@)\n" );
		}
      fprintf( subout, "  endif\n" );
		fprintf( subout, " endif\n" );

		fprintf( subout, "\n" );

		fprintf( subout, " ifdef USES_RPATH\n" );
		{
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X) | $(INTDEST) $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX) \n", i, i, i, i );
      	else
				fprintf( subout, "$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT):$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX) $(FINALDEST)$(LIBPATH)$(DEST_SUFFIX) $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i, i );

			fprintf( subout, "\t@echo \"linking(6) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "   ifdef LIBLINKRULE\n" );
			fprintf( subout, "\t$(call DOCMD,$(call LIBLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT),$(OUT_%X_LITERAL),$(LITERAL_LIBLIST_%X)))\n",i,i,i,i,i,i,i,i,i,i,i );
			fprintf( subout, "   else\n" );
			fprintf( subout, "\t$(call DOCMD,$(LD) $(LDFLAGS_%X) $(MAPOUTOPT)$(INTDEST)/$(OUTNAME_SHORT_%X).map $(LD_OUTNAMEOPT)$@  $(call LD_INPUT,$(LOBJS_%X)) $(RESOURCE_%X) $(LIBPATH_%X) $(LIBLIST_%X) $(call LD_FINAL_INPUT,$(LOBJS_%X),$(LITERAL_LIBLIST_%X)) )\n", i, i, i, i, i, i, i, i );
			fprintf( subout, "   endif\n" );
		}
		fprintf( subout, " else\n" );
		{
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT): $(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT) | $(INTDEST) $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)\n", i, i );
         else
				fprintf( subout, "$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT):$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)  $(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT)\n", i, i );
			fprintf( subout, "\t@echo \"linking(7) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "\t$(call DOCMD,cp $< $@)\n" );
		}
      fprintf( subout, " endif\n" );
		fprintf( subout, "\n" );

		fprintf( subout, " ifdef USES_RPATH\n" );
		{
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(if $(OUT_%X_PROGRAM),$(BINPATH),$(BINLIBPATH))$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)| $(INTDEST) $(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX)\n", i, i, i, i, i );
      	else
				fprintf( subout, "$(FINALDEST)$(if $(OUT_%X_PROGRAM),$(BINPATH),$(BINLIBPATH))$(DEST_SUFFIX)/$(OUTNAME_SHORT_%X):$(FINALDEST)$(BINLIBPATH)$(DEST_SUFFIX) $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i, i, i );

         fprintf( subout, "\t@echo \"linking(8) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "   ifdef USE_LINK_RULES\n" );
			fprintf( subout, "    ifeq '$(OUT_%X_PROGRAM)' '1'\n", i );
			fprintf( subout, "     ifeq '$(SUBSYSTEM%X)' 'windows'\n", i );
			fprintf( subout, "\t$(call DOCMD,$(call WIN_PRGLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)))\n",i,i,i,i,i,i,i,i,i,i );
			fprintf( subout, "     else\n" );
			fprintf( subout, "\t$(call DOCMD,$(call CON_PRGLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)))\n",i,i,i,i,i,i,i,i,i,i );
			fprintf( subout, "     endif\n" );
			fprintf( subout, "    else\n" );
			fprintf( subout, "\t$(call DOCMD,$(call LIBLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT),$(OUT_%X_LITERAL),$(LITERAL_LIBLIST_%X)))\n",i,i,i,i,i,i,i,i,i,i,i );
         fprintf( subout, "    endif\n" );
			fprintf( subout, "   else\n" );
			fprintf( subout, "\t$(call DOCMD,$(LD) $(LDFLAGS_%X) $(MAPOUTOPT)$(INTDEST)/$(OUTNAME_SHORT_%X).map $(LD_OUTNAMEOPT)$@ $(call LD_INPUT,$(LOBJS_%X)) $(RESOURCE_%X) $(LIBPATH_%X) $(LIBLIST_%X) $(call LD_FINAL_INPUT,$(LOBJS_%X),$(foreach lib,$(LITERAL_LIBLIST_%X),$(lib)$(STATIC_LITERAL_NAME))) )\n", i, i, i, i, i, i, i, i );
			fprintf( subout, "   endif\n" );
		}
		fprintf( subout, " else\n" );
		{
			if( _3point80 )
				fprintf( subout, "$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X): $(INTDEST)/$(OUTNAME_SHORT_%X) | $(INTDEST) $(FINALDEST)$(BINPATH)$(DEST_SUFFIX)\n", i, i );
         else
				fprintf( subout, "$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X):$(FINALDEST)$(BINPATH)$(DEST_SUFFIX)  $(INTDEST)/(OUTNAME_SHORT_%X)\n", i, i );
			fprintf( subout, "\t@echo \"linking(9) $(notdir $@)\"\n" );
         fprintf( subout, "\t$(FINALTARGET)\n" );
			fprintf( subout, "\t$(call DOCMD,cp $< $@)\n" );
		
		}
      fprintf( subout, " endif\n" );


		fprintf( subout, "\n\n" );

		fprintf( subout, " ifeq '$(OUT_%X_LIBRARY)' '1'\n", i );
		fprintf( subout, "  ifdef STATIC\n" );
		fprintf( subout, "$(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i,i );
		fprintf( subout, "\t@echo \"linking(10) $(notdir $@)\"\n" );
      fprintf( subout, "\t$(QUIETCMD)$(TARGET)\n" );
		fprintf( subout, "\t$(call DOCMD,$(QUIETCMD)rm -f $@(call DOCMD,)\n" );
		fprintf( subout, "   ifdef ARRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call ARRULE,$@,$(LOBJS_%X),$(RESOURCE_%X),$(LITERAL_LIBLIST_%X)))\n", i, i, i );
		fprintf( subout, "   else\n" );
		fprintf( subout, "\t$(call DOCMD,$(AR) $(LN_OUTNAMEOPT)$@ $(LOBJS_%X) $(RESOURCE_%X))\n", i, i );
		fprintf( subout, "   endif\n" );
		fprintf( subout, "  else\n" );
		fprintf( subout, "$(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT): $(INTDEST) ;\n",i );
		fprintf( subout, "$(INTDEST)/$(OUTNAME_SHORT_%X) $(INTDEST)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(SHLIBEXT): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i, i, i );
		fprintf( subout, "\t@echo \"linking(11) $(notdir $@)\"\n" );
      fprintf( subout, "\t$(QUIETCMD)$(TARGET)\n" );
		fprintf( subout, "   ifdef LIBLINKRULE\n" );
		fprintf( subout, "\t$(call DOCMD,$(call LIBLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(OUT_%X_LITERAL),$(LITERAL_LIBLIST_%X)))\n",i,i,i,i,i,i,i,i,i,i );
		fprintf( subout, "   else\n" );
		fprintf( subout, "\t$(call DOCMD,$(LD) $(LDFLAGS_%X) $(MAPOUTOPT)$(INTDEST)/$(OUTNAME_SHORT_%X).map $(LD_OUTNAMEOPT)$(subst /,$(SYSPATHCHAR),$@)  $(call LD_INPUT,$(LOBJS_%X)) $(RESOURCE_%X) $(LIBPATH_%X) $(LIBLIST_%X) $(call LD_FINAL_INPUT,$(LOBJS_%X),$(LITERAL_LIBLIST_%X))) $(LD_MOREFLAGS)\n", i, i, i, i, i, i, i );
		fprintf( subout, "   endif\n" );
		fprintf( subout, "  endif\n" );
		fprintf( subout, " endif\n" );
		fprintf( subout, " ifeq '$(OUT_%X_PROGRAM)' '1'\n", i );
		fprintf( subout, "$(INTDEST)/$(OUTNAME_SHORT_%X)$(PROGEXT): $(OBJS_%X) $(RESOURCE_%X) $(LIBDEPEND_%X)\n", i, i, i, i );
		fprintf( subout, "\t@echo \"linking(12) $(notdir $@)\"\n" );
      fprintf( subout, "\t$(QUIETCMD)$(TARGET)\n" );
		fprintf( subout, "   ifdef USE_LINK_RULES\n" );
		fprintf( subout, "    ifeq '$(SUBSYSTEM%X)' 'windows'\n", i );
		fprintf( subout, "\t$(call DOCMD,$(call WIN_PRGLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)))\n",i,i,i,i,i,i,i,i,i );
		fprintf( subout, "    else\n" );
		fprintf( subout, "\t$(call DOCMD,$(call CON_PRGLINKRULE,$(INTDEST)$(PATHCHAR)$(OUTNAME_SHORT_%X),$@,$(LDFLAGS_%X),$(LOBJS_%X),$(LIBLIST_%X),$(DEFNAME%X),$(OUTNAME_SHORT_%X),,$(LIBPATH_%X),$(RESOURCE_%X),$(FINALDEST)$(LIBPATH)$(DEST_SUFFIX)/$(LIBNAMEPREFIX)$(OUTNAME_SHORT_%X)$(LIBEXT)))\n",i,i,i,i,i,i,i,i,i );
		fprintf( subout, "    endif\n" );
		fprintf( subout, "   else\n" );
		fprintf( subout, "\t$(call DOCMD,$(LD) $(LDFLAGS_%X) $(MAPOUTOPT)$(INTDEST)/$(OUTNAME_SHORT_%X).map $(LD_OUTNAMEOPT)$(subst /,$(SYSPATHCHAR),$@) $(call LD_INPUT,$(LOBJS_%X)) $(RESOURCE_%X) $(LIBPATH_%X) $(LIBLIST_%X) $(call LD_FINAL_INPUT,$(LOBJS_%X),$(LITERAL_LIBLIST_%X)))\n", i, i, i, i, i, i, i, i, i );
		fprintf( subout, "   endif\n" );
		fprintf( subout, " endif\n" );
		fprintf( subout, "endif\n" );
		fprintf( subout, "\n" );

      fprintf( subout, "ifeq \'$(filter makebuild,$(MAKECMDGOALS))\' \'\'\n" );

                fprintf( subout, " ifndef NO_DEPENDS\n" );
                fprintf( subout, "  ifneq '$(DEPS_%X)' ''\n", i );
		fprintf( subout, "$(DEPS_%X): ;\n", i );
		fprintf( subout, "-include $(DEPS_%X)\n", i );
                fprintf( subout, "  endif\n" );
                fprintf( subout, " endif\n" );
		          fprintf( subout, "endif\n" );
		fclose( subout );
	}

	snprintf( fname, sizeof( fname ), "%s/make-sys", argv[1] );
	unlink( fname );
	subout = fopen( fname, "wb" );
	if( subout )
	{
   fprintf( subout, "ifeq '$(filter distclean,$(MAKECMDGOALS))' ''\n" );
	fprintf( subout, " ifndef MAKE_SYSTEM_DEFINED\n");
   fprintf( subout, "MAKE_SYSTEM_DEFINED=1\n");
	fprintf( subout, "SYSTEM_MAKE_FILES=" );
	for( i = 1; i < MAX_PARTS; i++ )
	{
      fprintf( subout, "$(SACK_BASE)/makefiles/$(RINTDEST)/make-dfs.%X ", i );
      fprintf( subout, "$(SACK_BASE)/makefiles/$(RINTDEST)/make-rls.%X ", i );
	}
   fprintf( subout, "\n" );
	//fprintf( subout, "$(SACK_BASE)/makefiles/mm$(PROGEXT) mm$(PROGEXT) $(SACK_BASE)/makefiles/echoto$(PROGEXT) echoto$(PROGEXT) $(SACK_BASE)/makefiles/make-sys:\n" );
   //fprintf( subout, "\t$(MAKE) -C $(SACK_BASE)/makefiles -f Makefile\n" );
	//fprintf( subout, "$(SACK_BASE)/makefiles/$(RINTDEST)/make-sys: ;\n" );
	for( i = 1; i < MAX_PARTS; i++ )
	{
		//fprintf( subout, "$(SACK_BASE)/makefiles/make-dfs.%X $(SACK_BASE)/makefiles/make-rls.%X: $(SACK_BASE)/makefiles/echoto$(PROGEXT) $(SACK_BASE)/makefiles/mm$(PROGEXT)\n", i, i );
		//fprintf( subout, "$(SACK_BASE)/makefiles/$(RINTDEST)/make-dfs.%X $(SACK_BASE)/makefiles/$(RINTDEST)/make-rls.%X: $(SACK_BASE)/makefiles/$(RINTDEST)/echoto$(PROGEXT) $(SACK_BASE)/makefiles/$(RINTDEST)/mm$(PROGEXT)\n", i, i );
		fprintf( subout, "$(SACK_BASE)/makefiles/$(RINTDEST)/make-dfs.%X $(SACK_BASE)/makefiles/$(RINTDEST)/make-rls.%X: $(SACK_BASE)/makefiles/$(RINTDEST)/mm$(PROGEXT)\n", i, i );
      fprintf( subout, "\t$(RUNMM)\n" );
	}
	//fprintf( subout, "$(SACK_BASE)/makefiles/make-sys make-sys: ;\n" );
	fprintf( subout, " endif\n" );
	fprintf( subout, "endif\n" );
   fclose( subout );
	}
	// additional per-project object rules which may
	// be non-standard
	//fprintf( out, "makefile.cm2: ;\n" );
	//fprintf( out, "-include makefile.cm2\n" );
	//fprintf( out, "$(SACK_BASE)/makefiles/makefile.many.real: ;\n" );
	//fprintf( out, "$(SACK_BASE)/makefiles/makefile.mr: ;\n" );
	fprintf( out, "\n" );
	return 0;
}

// $Log: mmf.c,v $
// Revision 1.74  2005/05/06 21:40:02  jim
// Add deadstart as the FIRST object.  This causes its PRELOAD() code to run LAST under linux... wasn't an issue until using PRELOAD in an application.
//
// Revision 1.73  2005/03/22 01:40:59  panther
// Duh - never used 'fname' just open correct file and be happy
//
// Revision 1.72  2005/01/16 23:54:19  panther
// fix preload/deadstart code for linux
//
// Revision 1.71  2005/01/08 11:18:56  panther
// Define debug build flags so the program doesn't crash :(
//
// Revision 1.70  2004/12/20 19:38:39  panther
// Fix static library link line for non 3.80 make
//
// Revision 1.69  2004/12/15 08:29:33  panther
// Updates to the make system... quite significant changes actually
//
// Revision 1.68  2004/12/13 20:45:17  panther
// Remove extra paren that maeks certain systems break on compile
//
// Revision 1.67  2004/10/31 17:22:27  d3x0r
// Minor fixes to control library...
//
// Revision 1.66  2004/09/02 10:22:52  d3x0r
// tweaks for linux build
//
// Revision 1.65  2004/08/14 07:33:45  d3x0r
// Getting closer to a fully cached build all the time... plus collapsed more rules to common
//
// Revision 1.64  2004/08/14 00:11:43  d3x0r
// fixes for generateion of makefile rule files.
//
// Revision 1.63  2004/07/21 00:57:51  d3x0r
// Build Local VPATH and filter out . paths... also filter ./paths when building obj list
//
// Revision 1.62  2004/07/21 00:46:18  d3x0r
// Fixes for mult-part files without sub-paths
//
// Revision 1.61  2004/07/20 09:12:24  d3x0r
// Fixes for making projects with sub-pathed sources
//
// Revision 1.60  2004/07/17 00:12:07  d3x0r
// fixes for allowing a main makefile to define a project with sources in sub-paths.... also cleaned some commin icky junk...
//
// Revision 1.59  2004/07/13 04:16:03  d3x0r
// Minor fixes - removing HERE definition... remove MAKELIB optionally...
//
// Revision 1.58  2004/07/07 15:33:55  d3x0r
// Cleaned c++ warnings, bad headers, fixed make system, fixed reallocate...
//
// Revision 1.57  2004/06/30 23:10:06  d3x0r
// checkpoint.
//
// Revision 1.56  2004/06/29 23:15:18  d3x0r
// Fix assembly dependancies.  Include source depend files in the cache load...
//
// Revision 1.55  2004/06/21 10:00:39  d3x0r
// Enhanced makesystem...
//
// Revision 1.54  2004/06/21 08:45:33  d3x0r
// okay this should be a good upgrade, fairly transparent.... with a make cache system in place
//
// Revision 1.53  2004/06/20 14:00:24  d3x0r
// fixes to echoto to be able to pull the command line from system - otherwise quotes get lost.
//
// Revision 1.52  2004/06/20 13:25:28  d3x0r
// Update makesystem to allow makebuild which generates cached cooked makefiles
//
// Revision 1.51  2004/06/01 21:55:12  d3x0r
// build correct targetname under linux
//
// Revision 1.50  2004/06/01 20:54:15  d3x0r
// Define __LINUX__ so that we can build the correct quotes for TARGETNAME
//
// Revision 1.49  2004/04/25 09:32:39  d3x0r
// distclean works... make from scratch works, rebuild works?
//
// Revision 1.48  2004/04/21 10:37:50  d3x0r
// Checkpoint - still crippled in the make system...
//
// Revision 1.47  2004/04/21 08:30:31  d3x0r
// Stablized with cache reverted make system
//
// Revision 1.45  2004/04/20 09:08:19  d3x0r
// Had it, lost it, getting it back.  Fails when targets already exist.
//
// Revision 1.44  2004/04/19 19:44:59  d3x0r
// Support for app data improvied, also support foriegn trees better
//
// Revision 1.43  2004/04/19 15:36:53  d3x0r
// Have to fix NASM include directory handling, or so it would appear.
//
// Revision 1.42  2004/04/19 14:48:24  d3x0r
// Minor typo in generated files - long time broken.
//
// Revision 1.41  2004/04/19 14:04:26  d3x0r
// Okay this seems to layout the whole project tree - just not reassembling the parts right...
//
// Revision 1.40  2004/04/18 00:58:27  d3x0r
// Basic chaining and creation of build files works
//
// Revision 1.39  2004/04/17 10:03:11  d3x0r
// Shell seems to be cleaned - just need to do the real work into the makefile.build script.
//
// Revision 1.38  2004/04/16 23:54:30  d3x0r
// Well - it's nearly there - looks like I gotta consolidate and rewrite - SO DO IT ALREADY!
//
// Revision 1.37  2004/04/16 00:54:38  d3x0r
// Updated makemakefile to build corrent makefile.many... still doing code review
//
// Revision 1.36  2004/04/15 19:59:43  d3x0r
// Okay looks like this is about due for a redesign...
//
// Revision 1.35  2004/04/15 07:14:22  d3x0r
// Some massively sweeping makesystem changes
//
// Revision 1.34  2004/04/12 10:47:42  d3x0r
// Static literal libname support - still is a DLL - plugins are always going to be plugins - just different programs...
//
// Revision 1.33  2004/01/31 01:30:20  d3x0r
// Mods to extend/test procreglib.
//
// Revision 1.32  2004/01/02 11:16:35  panther
// Add checking for moretargets to load rules
//
// Revision 1.31  2003/12/10 15:39:03  panther
// Added definition for LOCAL_ONLY targets and MORE_TARGETS
//
// Revision 1.30  2003/12/09 16:16:24  panther
// Expand max parts built
//
// Revision 1.29  2003/12/04 11:28:32  panther
// Add quiet cmd to built linux, and cleaup rules
//
// Revision 1.28  2003/12/03 14:08:03  panther
// Fixup makefiles for static builds (windows)
//
// Revision 1.27  2003/10/31 01:27:41  panther
// Revert makefile.projects external rule def.  More fixes to distclean
//
// Revision 1.26  2003/10/31 01:11:26  panther
// Okay do need to filter distclean to prevent ../ building
//
// Revision 1.25  2003/10/31 00:56:14  panther
// Define otherclean - which are possible other targets to clean with distclean
//
// Revision 1.24  2003/10/30 22:21:04  panther
// Fix watcom makefile options - also provide ability to make link library for exe
//
// Revision 1.23  2003/10/18 04:40:06  panther
// Fix makefiles for windows, watcom
//
// Revision 1.22  2003/10/15 23:07:23  panther
// Fixup make system some more...
//
// Revision 1.21  2003/10/15 22:10:41  panther
// Fix multi-platform parallel builds
//
// Revision 1.20  2003/10/15 16:45:25  panther
// Build distclean to erase all possible targets (distribution clean)
//
// Revision 1.19  2003/10/14 20:49:53  panther
// Fix echo statement under windows.
//
// Revision 1.18  2003/10/08 11:04:12  panther
// Oops extra close paren static link
//
// Revision 1.17  2003/10/06 15:07:03  panther
// Fixes to ctllist, adopting frames...
//
// Revision 1.16  2003/10/01 02:35:08  panther
// fix literal_libname building
//
// Revision 1.15  2003/09/30 09:40:42  panther
// Implement direct build
//
// Revision 1.14  2003/09/30 01:18:34  panther
// Add some comments about config flags
//
// Revision 1.13  2003/09/30 01:17:17  panther
// split XCXFLAGS into XCXFLAGS and XCPPFLAGS and define CPP_PROGTYPE_FLAGS
//
// Revision 1.12  2003/09/29 14:13:37  panther
// Watcom build to build libraries/dll's directly to final destination
//
// Revision 1.11  2003/09/25 00:21:35  panther
// More makefile cleanings - still somewhat broken
//
// Revision 1.10  2003/09/24 23:30:39  panther
// Quite a few makfile system changes - trying to trim out the cruft
//
// Revision 1.9  2003/09/24 02:09:40  panther
// Export cpp build rules
//
// Revision 1.8  2003/09/08 10:44:16  panther
// Modified CXFLAGSX to be the actual content of flags not the name of the flags
//
// Revision 1.7  2003/08/12 10:52:28  panther
// Enable/fix compilation using watcom compiler with ppc frontend
//
// Revision 1.6  2003/08/12 09:59:25  panther
// Fix option order, added 5th parameter to ccrule
//
// Revision 1.5  2003/08/07 15:21:50  panther
// Add ARRULE which should have already been done
//
// Revision 1.4  2003/08/05 01:29:22  panther
// Add documentation to make makefile.  Add COMMON_LIB, COMMON_SYSLIB to make makefile
//
// Revision 1.3  2003/07/28 08:45:51  panther
// Attempt to make do less guessing... still does too much
//
// Revision 1.2  2003/06/12 10:33:02  panther
// Make make-system much more robust, and observant of self-changes
//
// Revision 1.1  2003/05/12 01:10:54  panther
// Makemakefile - simplest rules
//
// Revision 2.43  2003/05/12 01:06:44  panther
// Updates/fixed for linux building
//
// Revision 2.42  2003/05/12 00:32:52  panther
// Updates for more generality
//
// Revision 2.41  2003/05/11 23:41:15  panther
// UPdates for watcom, generalization, and cleaning
//
// Revision 2.40  2003/04/28 00:49:21  panther
// Disable nodepends better
//
// Revision 2.39  2003/04/27 23:05:26  panther
// Linux make nodpends option
//
// Revision 2.38  2003/04/18 22:20:40  panther
// Add parent level make to auto build my depends...
//
// Revision 2.37  2003/04/11 16:03:53  panther
// Added  LogN for gcc.  Fixed set code to search for first available instead of add at end always.  Added MKCFLAGS MKLDFLAGS for lnx makes.
// Fixed target of APP_DEFAULT_DATA.
// Updated display to use a meta buffer between for soft cursors.
//
// Revision 2.36  2003/04/08 16:08:23  panther
// Do a windows make stable distclean step
//
// Revision 2.35  2003/04/08 15:35:17  panther
// If PROJECTS are defined - call a distclean on them
//
// Revision 2.34  2003/04/07 15:23:46  panther
// Include sub-projects included in real projects
//
// Revision 2.33  2003/04/04 09:20:31  panther
// Fix the copy of app data files
//
// Revision 2.32  2003/04/03 11:06:58  panther
// Tweak for LCC compile - maybe other windows?
//
// Revision 2.31  2003/04/02 22:12:23  panther
// LCC Pathname fixes
//
// Revision 2.30  2003/04/02 20:23:24  panther
// Fix program targets
//
// Revision 2.29  2003/04/02 19:53:21  panther
// Fix non USES_RPATH style making
//
// Revision 2.28  2003/04/02 10:36:55  panther
// Oops ended up making program depenancies order only..
//
// Revision 2.27  2003/04/02 09:06:02  panther
// Make target directory for APP_DEFAULT_DATA
//
// Revision 2.26  2003/04/02 08:38:17  panther
// Oops - program files weren't made into final destination
//
// Revision 2.25  2003/04/02 08:26:39  panther
// Handle APP_DEFAULT_DATA entries better.
//
// Revision 2.24  2003/04/02 08:03:42  panther
// Don't build libraries (with rpath statements) except in final destinations
//
// Revision 2.23  2003/03/31 22:56:14  panther
// extend makesystem to basically support distclean.  Handle project makes better.
//
// Revision 2.22  2003/03/25 08:45:50  panther
// Added CVS logging tag
//
