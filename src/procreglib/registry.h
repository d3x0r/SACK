#ifndef REGISTRY_STRUCTURE_DEFINED
#define REGISTRY_STRUCTURE_DEFINED


#ifdef __LINUX__
// pid_t for service process registrations...
#include <sys/types.h>
#endif

PROCREG_NAMESPACE

//typedef struct tree_def_tag *PCLASSROOT;
//typedef void (CPROC *PROCEDURE)(void);

enum proc_types {
	ARG_FLOAT
					, ARG_DOUBLE
					, ARG_CHAR
					, ARG_SHORT
					, ARG_LONG
					, ARG_LONGLONG
					, ARG_UCHAR
					, ARG_USHORT
					, ARG_ULONG
					, ARG_ULONGLONG
					, ARG_PTRFLOAT
					, ARG_PTRDOUBLE
					, ARG_PTRCHAR
					, ARG_PTRSHORT
					, ARG_PTRLONG
					, ARG_PTRLONGLONG
					, ARG_PTRUCHAR
					, ARG_PTRUSHORT
					, ARG_PTRULONG
					, ARG_PTRULONGLONG
               , ARG_PTRSTRING
};

#if 0
typedef struct proc_param_tag {
	enum proc_types type;
   int count;
//1>c:\work\sack\src\procreglib\registry.h(42) : warning C4200: nonstandard extension used : zero-sized array in struct/union
//1>        Cannot generate copy-ctor or copy-assignment operator when UDT contains a zero-sized array`
   char name[];
} PROC_PARAM;
#endif

typedef struct proc_def_tag
{
	//CTEXTSTR ret;
	// name is a NUL seperated list of fields
	// library:procname:return type:args
	// this is truly the full name of a procedure...
	// arguments have any spaces or odd characters stripped
	CTEXTSTR name;
	//CTEXTSTR args;
	CTEXTSTR library;
	CTEXTSTR procname;
	PROCEDURE proc; // proc is just a cache of library:name
} PROCDEF, *PPROCDEF;

#ifdef __cplusplus_cli
typedef struct stdproc_def_tag
{
	//CTEXTSTR ret;
	// name is a NUL seperated list of fields
	// library:procname:return type:args
	// this is truly the full name of a procedure...
	// arguments have any spaces or odd characters stripped
	CTEXTSTR name;
	//CTEXTSTR args;
	CTEXTSTR library;
	CTEXTSTR procname;
	STDPROCEDURE proc; // proc is just a cache of library:name
} STDPROCDEF, *PSTDPROCDEF;
#endif

#define MAGIC_TREE_NUMBER 0x20040525
#define IS_TREE_MAGIC(tree)  (*((_32*)(&(tree))) == (_32)MAGIC_TREE_NUMBER )
#define MAGIC_LIST_NUMBER 0x20040911
#define IS_LIST_MAGIC(tree)  (*((_32*)(&(tree))) == (_32)MAGIC_LIST_NUMBER )
typedef struct tree_def_tag
{
////////
// this and "/blah/blah/blah" look the same
// to an application.  Therefore a name is as good
// as text... internally a pointer to this structure
// when CTEXTSTR and PTREEDEF are received through a
// CTEXTSTR parameter, the distinction can be made by
// examinging the first 4 characters... however!
// the string passed must be at least 4 bytes of
   // real memory.
	_32 Magic;  // magic number 0x20040525
	union {
		PTREEROOT Tree; // any option can have a tree of suboptions?
			// a list of things may serve as lightweight alternative
		PLIST List; 
	};
   POINTER cursor;
} TREEDEF, *PTREEDEF;
#define MAXTREEDEFSPERSET 256
DeclareSet( TREEDEF );

typedef struct data_class_def_tag
{
   PTRSZVAL size;
   OpenCloseNotification Open;
   OpenCloseNotification Close;
   INDEX unique;
   TREEDEF instances;
} DATADEF, *PDATADEF;

typedef struct name_def_tag
{
	// a member can have a string and a number
   // associated with it.
   CTEXTSTR sValue;
   PTRSZVAL iValue;
} NAMEDEF, *PNAMEDEF;

#ifdef __LINUX__
typedef  struct service_tag {
	pid_t pid;
	_32 Msg;
	// paramters...
} SERVICE, *PSERVICE;
#endif
typedef struct proc_name_tag
{
	struct {
		BIT_FIELD bAlias: 1; // really only important when saving the tree
		// each and every node may reference a tree
		// if it does, then the tree will be non-NULL
		BIT_FIELD bTree : 1; // else it's a proc leef...
		BIT_FIELD bValue : 1; // name value points at a name
		BIT_FIELD bIntVal : 1; // and value is a PTRSZVAL integer...
		BIT_FIELD bStringVal : 1; // and value is a PTRSZVAL integer...
		BIT_FIELD bProc : 1; // name points at a function
		BIT_FIELD bService : 1;  // this might be fun to register names across msgsvr's
		BIT_FIELD bData : 1; // data member defines data..
		BIT_FIELD bStdProc : 1; // is a (__stdcall *) instead of a (CPROC *)
	} flags;
	CTEXTSTR name;
	TREEDEF   tree;
	struct {
		NAMEDEF    name;
		PROCDEF    proc;
#ifdef __cplusplus_cli
		STDPROCDEF stdproc;
#endif
		DATADEF    data;
#ifdef __LINUX__
		SERVICE    service;
#endif
	}data;
} NAME, *PNAME;

#define MAXNAMESPERSET 256
DeclareSet( NAME );


#define NAMESPACE_SIZE 4096 - sizeof( _32 ) - 2 * sizeof( POINTER )

typedef struct namespace_tag
{
   _32 nextname;
	TEXTCHAR buffer[NAMESPACE_SIZE];
   DeclareLink( struct namespace_tag );
} NAMESPACE, *PNAMESPACE;

PROCREG_NAMESPACE_END

#endif
