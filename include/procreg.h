/*
 * Create: James Buckeyne
 *
 * Purpose: Provide a general structure to register names of
 *   routines and data structures which may be consulted
 *   for runtime linking.  Aliases and other features make this
 *   a useful library for tracking interface registration...
 *
 *  The namespace may be enumerated.
 */



#ifndef PROCEDURE_REGISTRY_LIBRARY_DEFINED
#define PROCEDURE_REGISTRY_LIBRARY_DEFINED
#include <sack_types.h>
#include <deadstart.h>

#ifdef PROCREG_SOURCE
#define PROCREG_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PROCREG_PROC(type,name) IMPORT_METHOD type CPROC name
#endif



#ifdef __cplusplus
#ifdef __cplusplus_cli
#include <vcclr.h>
//using namespace System;
#endif
#   define _INTERFACE_NAMESPACE namespace Interface {
#   define _INTERFACE_NAMESPACE_END }
#define PROCREG_NAMESPACE namespace sack { namespace app { namespace registry {
#define _PROCREG_NAMESPACE namespace registry {
#define _APP_NAMESPACE namespace app { 
#define PROCREG_NAMESPACE_END }}}
//extern "C"  {
#else
#   define _INTERFACE_NAMESPACE
#   define _INTERFACE_NAMESPACE_END
#define _PROCREG_NAMESPACE 
#define _APP_NAMESPACE 
#define PROCREG_NAMESPACE
#define PROCREG_NAMESPACE_END
#endif

SACK_NAMESPACE
/* Deadstart is support which differs per compiler, but allows
   applications access a C++ feature - static classes with
   constructors that initialize at loadtime, but, have the
   feature that you can create threads. Deadstart code is run
   after the DLL load lock under windows that prevents creation
   of threads; however, deadstart is run before main. Deadstart
   routines can have a priority. Certain features require others
   to be present always. This allows explicit control of
   priority unlink using classes with static constructors, which
   requires ordering of objects to provide linking order. Also
   provides a similar registration mechanism for atexit, but
   extending with priority. Deadstop registrations are done
   sometime during normal C atexit() handling, but may be
   triggered first by calling BAG_Exit.
   
   Registry offers support to register functions, and data under
   a hierarchy of names. Names are kept in a string cache, which
   applications can take benefit of. Strings will exist only a
   single time. This table could be saved, and a look-aside
   table for language translation purposes. Registry is the
   support that the latest PSI relies on for registering event
   callbacks for controls. The registry was always used, but,
   the access to it was encapsulated by DoRegisterControl
   registering the appropriate methods.                          */
	_APP_NAMESPACE
   /* Contains methods dealing with registering routines and values
      in memory. Provisions are available to save the configuration
      state, but the best that can be offered here would be a
      translation tool for text strings. The namespace is savable,
      but most of the content of the registration space are short
      term pointers. Namespace containing registry namespace.
      
      
      old notes - very discongruant probably should delete them.
      
      
      
      Process name registry
      
      
      
      it's a tree of names.
      
      there are paths, and entries
      
      paths are represented as class_name
      
      PCLASSROOT is also a suitable class name
      
      PCLASSROOT is defined as a valid CTEXTSTR.
      
      there is (apparently) a name that is not valid as a path name
      
      that is TREE
      
      
      
      guess.
      
      POINTER in these two are equal to (void(*)(void)) but -
      that's rarely the most useful thing... so
      
      name class is a tree of keys... /\<...\>
      
      psi/control/## might contain procs Init Destroy Move
      
      RegAlias( WIDE("psi/control/3"), WIDE("psi/control/button")
      ); psi/control/button and psi/control/3 might reference the
      same routines
      
      psi/frame Init Destroy Move memlib Alloc Free
      
      network/tcp
      
      I guess name class trees are somewhat shallow at the moment
      not going beyond 1-3 layers
      
      names may eventually be registered and reference out of body
      services, even out of box...
      
      
      
      the values passed as returntype and parms/args need not be
      real genuine types, but do need to be consistant between the
      registrant and the requestor... this provides for full name
      dressing, return type and paramter type may both cause
      overridden functions to occur...                              */
_PROCREG_NAMESPACE


#ifndef REGISTRY_STRUCTURE_DEFINED
	// make these a CTEXTSTR to be compatible with name_class...
#ifdef __cplusplus
	// because of name mangling and stronger type casting
	// it becomes difficult to pass a tree_def_tag * as a CTEXTSTR classname
	// as valid as this is.
	typedef struct tree_def_tag const * PCLASSROOT;
#else
	typedef CTEXTSTR PCLASSROOT;
#endif

	typedef void (CPROC *PROCEDURE)(void);
#ifdef __cplusplus_cli
	typedef void (__stdcall *STDPROCEDURE)(array<System::Object^>^);
#endif
#else
	typedef struct tree_def_tag const * PCLASSROOT;
	typedef void (CPROC *PROCEDURE)(void);
#ifdef __cplusplus_cli
	typedef void (__stdcall *STDPROCEDURE)(array<System::Object^>^);
#endif
#endif


/* CheckClassRoot reads for a path of names, but does not create
   it if it does not exist.                                      */
PROCREG_PROC( PCLASSROOT, CheckClassRoot )( CTEXTSTR class_name );
/* \Returns a PCLASSROOT of a specified path. The path may be
   either a PCLASSROOT or a text string indicating the path. the
   Ex versions allow passing a base PCLASSROOT path and an
   additional subpath to get. GetClassRoot will always create
   the path if it did not exist before, and will always result
   with a root.
   
   
   
   
   Remarks
   a CTEXTSTR (plain text string, probably wide character if
   compiled unicode) and a PCLASSROOT are always
   interchangeable. Though you may need a forced type cast, I
   have defined both CTEXTSTR and PCLASSROOT function overloads
   for c++ compiled code, and C isn't so unkind about the
   conversion. I think problem might lie that CTEXTSTR has a
   const qualifier and PCLASSROOT doesn't (but should).
   Example
   <code lang="c++">
   
   PCLASSROOT root = GetClassRoot( "psi/resource" );
   // returns the root of all resource names.
   
   </code>
   <code>
   PCLASSROOT root2 = GetClassRootEx( "psi/resource", "buttons" );
   
   </code>                                                         */
PROCREG_PROC( PCLASSROOT, GetClassRoot )( CTEXTSTR class_name );
/* <combine sack::app::registry::GetClassRoot@CTEXTSTR>
   
   \ \                                                  */
PROCREG_PROC( PCLASSROOT, GetClassRootEx )( PCLASSROOT root, CTEXTSTR name_class );
#ifdef __cplusplus
/* <combine sack::app::registry::GetClassRoot@CTEXTSTR>
   
   \ \                                                  */
PROCREG_PROC( PCLASSROOT, GetClassRoot )( PCLASSROOT class_name );
/* <combine sack::app::registry::GetClassRoot@CTEXTSTR>
   
   \ \                                                  */
PROCREG_PROC( PCLASSROOT, GetClassRootEx )( PCLASSROOT root, PCLASSROOT name_class );
#endif

/* Fills a string with the path name to the specified node */
PROCREG_PROC( int, GetClassPath )( TEXTSTR out, size_t len, PCLASSROOT root );


PROCREG_PROC( void, SetInterfaceConfigFile )( TEXTCHAR *filename );


/* Get[First/Next]RegisteredName( WIDE("classname"), &amp;data );
   these operations are not threadsafe and multiple thread
   accesses will cause mis-stepping
   
   
   
   These functions as passed the address of a POINTER. this
   POINTER is for the use of the browse routines and should is
   meaningless to he calling application.
   Parameters
   root :       The root to search from
   classname :  A sub\-path from the root to search from
   data :       the address of a pointer that keeps track of
                information about the search. (opaque to user)
   
   Example
   Usage:
   <code lang="c++">
   CTEXTSTR result;
   POINTER data = NULL;
   for( result = GetFirstRegisteredName( "some/class/path", &amp;data );
        \result;
        \result = GetNextRegisteredName( &amp;data ) )
   {
        // result is a string name of the current node.
        // can use that name and GetRegistered____ (function/int/value)
        if( NameHasBranches( &amp;data ) ) // for consitancy in syntax
        {
            // consider recursing through tree, name becomes a valid classname for GetFirstRegisteredName()
        }
   }
   </code>                                                                                                  */
PROCREG_PROC( CTEXTSTR, GetFirstRegisteredNameEx )( PCLASSROOT root, CTEXTSTR classname, PCLASSROOT *data );
#ifdef __cplusplus
/* <combine sack::app::registry::GetFirstRegisteredNameEx@PCLASSROOT@CTEXTSTR@PCLASSROOT *>
   
   \ \                                                                                      */
	PROCREG_PROC( CTEXTSTR, GetFirstRegisteredName )( PCLASSROOT classname, PCLASSROOT *data );
#endif
/* <combine sack::app::registry::GetFirstRegisteredNameEx@PCLASSROOT@CTEXTSTR@PCLASSROOT *>
   
   \ \                                                                                      */
PROCREG_PROC( CTEXTSTR, GetFirstRegisteredName )( CTEXTSTR classname, PCLASSROOT *data );
/* Steps to the next registered name being browsed. Is passed
   only the pointer to data. See GetFirstRegisteredName for
   usage.
   See Also
   <link sack::app::registry::GetFirstRegisteredNameEx@PCLASSROOT@CTEXTSTR@PCLASSROOT *, sack::app::registry::GetFirstRegisteredNameEx Function> */
PROCREG_PROC( CTEXTSTR, GetNextRegisteredName )( PCLASSROOT *data );
/* When using GetFirstRegisteredName and GetNextRegisteredName
   to browse through names, this function is able to get the
   current PCLASSROOT of the current node, usually you end up
   with just the content of that registered name.
   
   
   
   \result with the current node ( useful for pulling registered
   subvalues like description, or file and line )
                                                                 */
PROCREG_PROC( PCLASSROOT, GetCurrentRegisteredTree )( PCLASSROOT *data );
#ifdef __cplusplus
//PROCREG_PROC( CTEXTSTR, GetFirstRegisteredName )( CTEXTSTR classname, POINTER *data );
//PROCREG_PROC( CTEXTSTR, GetNextRegisteredName )( POINTER *data );
#endif
// while doing a scan for registered procedures, allow applications to check for branches
//PROCREG_PROC( int, NameHasBranches )( POINTER *data );
PROCREG_PROC( int, NameHasBranches )( PCLASSROOT *data );
// while doing a scan for registered procedures, allow applications to ignore aliases...
PROCREG_PROC( int, NameIsAlias )( PCLASSROOT *data );

/*
 * RegisterProcedureExx( 
 *
 */
PROCREG_PROC( int, RegisterProcedureExx )( PCLASSROOT root // root name or PCLASSROOT of base path
													  , CTEXTSTR name_class // an additional path on root
													  , CTEXTSTR public_name // the name of the value entry saved in the tree
													  , CTEXTSTR returntype // the text return type of this function - may be checked to validate during GetRegisteredProcedure
													  , CTEXTSTR library // name of the library this symbol is in - may be checked to validate during GetRegisteredProcedure
													  , CTEXTSTR name // actual C function name in library - may be checked to validate during GetRegisteredProcedure
													  , CTEXTSTR args // preferably the raw argument string of types and no variable references "([type][,type]...)"
													  DBG_PASS // file and line of the calling application.  May be no parameter in release mode.
													  );


/*
 * RegisterProcedureEx( root       // root path
 *                    , name_class // additional name
 *                    , nice_name  // nice name
 *                    , return type not in quotes  'void'
 *                    , function_name in quotes '"Function"'
 *                    , args not in quotes '(int,char,float,UserType*)'
 */
#define RegisterProcedureEx(root,nc,n,rtype,proc,args)  RegisterProcedureExx( (root),(nc),(n),#rtype,TARGETNAME,(proc), #args DBG_SRC)
/*
 * RegisterProcedure( name_class // additional name
 *                    , nice_name  // nice name
 *                    , return type not in quotes  'void'
 *                    , function_name in quotes '"Function"'
 *                    , args not in quotes '(int,char,float,UserType*)'
 */
#define RegisterProcedure(nc,n,rtype,proc,args)  RegisterProcedureExx( NULL, (nc),(n),#rtype,TARGETNAME,(proc), #args DBG_SRC)


/* 
 * Branches on the tree may be aliased together to form a single branch
 * 
 */
				// RegisterClassAlias( WIDE("psi/control/button"), WIDE("psi/control/3") );
				// then the same set of values can be referenced both ways with
				// really only a single modified value.
/* parameters to RegisterClassAliasEx are the original name, and the new alias name for the origianl branch*/
PROCREG_PROC( PCLASSROOT, RegisterClassAliasEx )( PCLASSROOT root, CTEXTSTR original, CTEXTSTR alias );
/* <combine sack::app::registry::RegisterClassAliasEx@PCLASSROOT@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                              */
PROCREG_PROC( PCLASSROOT, RegisterClassAlias )( CTEXTSTR original, CTEXTSTR newalias );



// root, return, public, args, address

PROCREG_PROC( PROCEDURE, ReadRegisteredProcedureEx )( PCLASSROOT root
                                                    , CTEXTSTR returntype
																	 , CTEXTSTR parms
																  );
#define ReadRegisteredProcedure( root,rt,a) ((rt(CPROC*)a)ReadRegisteredProcedureEx(root,WIDE(#rt),WIDE(#a)))
/* Gets a function that has been registered. */
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( PCLASSROOT root
																	 , PCLASSROOT name_class
                                                    , CTEXTSTR returntype
																	 , CTEXTSTR name
																	 , CTEXTSTR parms
																	 );
#define GetRegisteredProcedureExx(root,nc,rt,n,a) ((rt (CPROC*)a)GetRegisteredProcedureExxx(root,nc,_WIDE(#rt),n,_WIDE(#a)))
#define GetRegisteredProcedure2(nc,rtype,name,args) (rtype (CPROC*)args)GetRegisteredProcedureEx((nc),WIDE(#rtype), name, WIDE(#args) )
#define GetRegisteredProcedureNonCPROC(nc,rtype,name,args) (rtype (*)args)GetRegisteredProcedureEx((nc),WIDE(#rtype), name, WIDE(#args) )

/* <combine sack::app::registry::GetRegisteredProcedureExxx@PCLASSROOT@PCLASSROOT@CTEXTSTR@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                                        */
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureEx )( PCLASSROOT name_class
																	, CTEXTSTR returntype
																	, CTEXTSTR name
																	, CTEXTSTR parms 
																	);

PROCREG_PROC( LOGICAL, RegisterFunctionExx )( PCLASSROOT root
													, PCLASSROOT name_class
													, CTEXTSTR public_name
													, CTEXTSTR returntype
													, PROCEDURE proc
													 , CTEXTSTR args
													 , CTEXTSTR library
													 , CTEXTSTR real_name
													  DBG_PASS
														  );
#ifdef __cplusplus
/* <combine sack::app::registry::GetRegisteredProcedureExxx@PCLASSROOT@PCLASSROOT@CTEXTSTR@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                                        */
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( CTEXTSTR root
																	 , CTEXTSTR name_class
                                                    , CTEXTSTR returntype
																	 , CTEXTSTR name
																	 , CTEXTSTR parms
																	 );
/* <combine sack::app::registry::GetRegisteredProcedureExxx@PCLASSROOT@PCLASSROOT@CTEXTSTR@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                                        */
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( CTEXTSTR root
																	 , PCLASSROOT name_class
                                                    , CTEXTSTR returntype
																	 , CTEXTSTR name
																	 , CTEXTSTR parms
																	 );
/* <combine sack::app::registry::GetRegisteredProcedureExxx@PCLASSROOT@PCLASSROOT@CTEXTSTR@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                                        */
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( PCLASSROOT root
																	 , CTEXTSTR name_class
                                                    , CTEXTSTR returntype
																	 , CTEXTSTR name
																	 , CTEXTSTR parms
																	 );
/* <combine sack::app::registry::GetRegisteredProcedureExxx@PCLASSROOT@PCLASSROOT@CTEXTSTR@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                                        */
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureEx )( CTEXTSTR name_class
																	, CTEXTSTR returntype
																	, CTEXTSTR name
																	, CTEXTSTR parms 
																	);
PROCREG_PROC( LOGICAL, RegisterFunctionExx )( CTEXTSTR root
													, CTEXTSTR name_class
													, CTEXTSTR public_name
													, CTEXTSTR returntype
                                       , PROCEDURE proc
													 , CTEXTSTR args
													 , CTEXTSTR library
													 , CTEXTSTR real_name
													  DBG_PASS
														  );
#endif
//#define RegisterFunctionExx( r,nc,p,rt,proc,ar ) RegisterFunctionExx( r,nc,p,rt,proc,ar,TARGETNAME,NULL DBG_SRC )
//#define RegisterFunctionEx(r,nc,pn,rt,proc,args,lib,rn) RegisterFunctionExx(r,nc,pn,rt,proc,args,lib,rn DBG_SRC)
#define RegisterFunctionEx( root,proc,rt,pn,a) RegisterFunctionExx( root,NULL,pn,rt,(PROCEDURE)(proc),a,NULL,NULL DBG_SRC )
#define RegisterFunction( nc,proc,rt,pn,a) RegisterFunctionExx( (PCLASSROOT)NULL,nc,pn,rt,(PROCEDURE)(proc),a,TARGETNAME,NULL DBG_SRC )

#define SimpleRegisterMethod(r,proc,rt,name,args) RegisterFunctionExx(r,NULL,name,rt,(PROCEDURE)proc,args,NULL,NULL DBG_SRC )

#define GetRegisteredProcedure(nc,rtype,name,args) (rtype (CPROC*)args)GetRegisteredProcedureEx((nc),_WIDE(#rtype), _WIDE(#name), _WIDE(#args) )
PROCREG_PROC( int, RegisterIntValueEx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name, uintptr_t value );
PROCREG_PROC( int, RegisterIntValue )( CTEXTSTR name_class, CTEXTSTR name, uintptr_t value );

PROCREG_PROC( int, RegisterValueExx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name, int bIntVal, CTEXTSTR value );
PROCREG_PROC( int, RegisterValueEx )( CTEXTSTR name_class, CTEXTSTR name, int bIntVal, CTEXTSTR value );
PROCREG_PROC( int, RegisterValue )( CTEXTSTR name_class, CTEXTSTR name, CTEXTSTR value );

/* \ \ 
   Parameters
   root :        Root class to start searching from
   name_class :  An additional sub\-path to get the name from
   name :        the name within the path specified
   bIntVal :     a true/false whether to get the string or
                 integer value from the specified node.
   
   Returns
   A pointer to a string if bIntVal is not set. (NULL if there
   was no string).
   
   Otherwise will be an int shorter than or equal to the size of
   a pointer, which should be cast to an int if bIntVal is set,
   and there is a value registered there. Probably 0 if no
   value, so registered 0 value and no value is
   indistinguisable.                                             */
PROCREG_PROC( CTEXTSTR, GetRegisteredValueExx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name, int bIntVal );
/* <combine sack::app::registry::GetRegisteredValueExx@PCLASSROOT@CTEXTSTR@CTEXTSTR@int>
   
   \ \                                                                                   */
PROCREG_PROC( CTEXTSTR, GetRegisteredValueEx )( CTEXTSTR name_class, CTEXTSTR name, int bIntVal );
/* <combine sack::app::registry::GetRegisteredValueExx@PCLASSROOT@CTEXTSTR@CTEXTSTR@int>
   
   \ \                                                                                   */
PROCREG_PROC( CTEXTSTR, GetRegisteredValue )( CTEXTSTR name_class, CTEXTSTR name );
#ifdef __cplusplus
/* <combine sack::app::registry::GetRegisteredValueExx@PCLASSROOT@CTEXTSTR@CTEXTSTR@int>
   
   \ \                                                                                   */
PROCREG_PROC( CTEXTSTR, GetRegisteredValueExx )( CTEXTSTR root, CTEXTSTR name_class, CTEXTSTR name, int bIntVal );
PROCREG_PROC( int, RegisterIntValueEx )( CTEXTSTR root, CTEXTSTR name_class, CTEXTSTR name, uintptr_t value );
#endif

/* This is like GetRegisteredValue, but takes the address of the
   type to return into instead of having to cast the final
   \result.
   
   
   
   if bIntValue, result should be passed as an (&amp;int)        */
PROCREG_PROC( int, GetRegisteredStaticValue )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name
															, CTEXTSTR *result
															, int bIntVal );
#define GetRegisteredStaticIntValue(r,nc,name,result) GetRegisteredStaticValue(r,nc,name,(CTEXTSTR*)result,TRUE )

/* <combine sack::app::registry::GetRegisteredValueExx@PCLASSROOT@CTEXTSTR@CTEXTSTR@int>
   
   \ \                                                                                   */
PROCREG_PROC( int, GetRegisteredIntValueEx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name );
/* <combine sack::app::registry::GetRegisteredIntValueEx@PCLASSROOT@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                 */
PROCREG_PROC( int, GetRegisteredIntValue )( CTEXTSTR name_class, CTEXTSTR name );
#ifdef __cplusplus
/* <combine sack::app::registry::GetRegisteredIntValueEx@PCLASSROOT@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                 */
PROCREG_PROC( int, GetRegisteredIntValue )( PCLASSROOT name_class, CTEXTSTR name );
#endif

typedef void (CPROC*OpenCloseNotification)( POINTER, uintptr_t );
#define PUBLIC_DATA( public, struct, open, close )    \
	PRELOAD( Data_##open##_##close ) { \
	RegisterDataType( WIDE("system/data/structs")  \
      	, public, sizeof(struct)    \
	, (OpenCloseNotification)open, (OpenCloseNotification)close ); }

#define PUBLIC_DATA_EX( public, struct, open, update, close )    \
	PRELOAD( Data_##open##_##close ) { \
	RegisterDataTypeEx( WIDE("system/data/structs")  \
      	, public, sizeof(struct)    \
	, (OpenCloseNotification)open, (OpenCloseNotification)update, (OpenCloseNotification)close ); }

#define GET_PUBLIC_DATA( public, type, instname ) \
   (type*)CreateRegisteredDataType( WIDE("system/data/structs"), public, instname )
PROCREG_PROC( uintptr_t, RegisterDataType )( CTEXTSTR classname
												 , CTEXTSTR name
												 , uintptr_t size
												 , OpenCloseNotification open
												 , OpenCloseNotification close );
/* Registers a structure as creatable in shared memory by name.
   So a single name of the structure can be used to retrieve a
   pointer to one created.
   
   
   
   
   Example
   \ \ 
   <code lang="c++">
   
   POINTER p = CreateRegisteredDataType( "My types", "my_registered_type", "my instance" );
   // p will result to a region of type 'my_registered_type' called 'my_instance'
   // if it did not exist, it will be created, otherwise the one existing is returned.
   
   </code>
   
   Parameters
   root :          optional root name (ex version uses this)
   classname :     path to the type
   name :          name of the type to create an instance of
   instancename :  a name for the instance created.                                         */
PROCREG_PROC( uintptr_t, CreateRegisteredDataType)( CTEXTSTR classname
																 , CTEXTSTR name
																 , CTEXTSTR instancename );

PROCREG_PROC( uintptr_t, RegisterDataTypeEx )( PCLASSROOT root
													, CTEXTSTR classname
													, CTEXTSTR name
													, uintptr_t size
													, OpenCloseNotification Open
													, OpenCloseNotification Close );

/* <combine sack::app::registry::CreateRegisteredDataType@CTEXTSTR@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                                */
PROCREG_PROC( uintptr_t, CreateRegisteredDataTypeEx)( PCLASSROOT root
																	, CTEXTSTR classname
																	, CTEXTSTR name
																	, CTEXTSTR instancename );

/* Outputs through syslog a tree dump of all names registered. */
PROCREG_PROC( void, DumpRegisteredNames )( void );
/* Dumps through syslog all names registered from the specified
   root point. (instead of dumping the whole tree)              */
PROCREG_PROC( void, DumpRegisteredNamesFrom )( PCLASSROOT root );

PROCREG_PROC( int, SaveTree )( void );
PROCREG_PROC( int, LoadTree )( void );

#define METHOD_PTR(type,name) type (CPROC *_##name)
#define DMETHOD_PTR(type,name) type (CPROC **_##name)

#define METHOD_ALIAS(i,name) ((i)->_##name)
#define PDMETHOD_ALIAS(i,name) (*(i)->_##name)

/* Releases an interface. When interfaces are registered, they
   register with a OnGetInterface and an OnDropInterface
   callback so that it may do additional work to cleanup from
   giving you a copy of the interface.
   
   
   Example
   <code lang="c++">
   
   POINTER p = GetInterface( "image" );
   DropInterface( p );
   
   </code>                                                     */
PROCREG_PROC( void, DropInterface )( CTEXTSTR pServiceName, POINTER interface_x );
/* \Returns the pointer to a registered interface. This is
   typically a structure that contains pointer to functions. Takes
   a text string to an interface. Interfaces are registered at a
   known location in the registry tree.                            */
PROCREG_PROC( POINTER, GetInterfaceDbg )( CTEXTSTR pServiceName DBG_PASS );
#define GetInterface(n) GetInterfaceDbg( n DBG_SRC )

#define GetRegisteredInterface(name) GetInterface(name)
PROCREG_PROC( LOGICAL, RegisterInterfaceEx )( CTEXTSTR name, POINTER(CPROC*load)(void), void(CPROC*unload)(POINTER) DBG_PASS );
PROCREG_PROC( LOGICAL, RegisterInterface )(CTEXTSTR name, POINTER( CPROC*load )(void), void(CPROC*unload)(POINTER));
#define RegisterInterface(n,l,u) RegisterInterfaceEx( n,l,u DBG_SRC )

// unregister a function, should be smart and do full return type
// and parameters..... but for now this only references name, this indicates
// that this has not been properly(fully) extended, and should be layered
// in such a way as to allow this function work in it's minimal form.
PROCREG_PROC( int, ReleaseRegisteredFunctionEx )( PCLASSROOT root
													, CTEXTSTR name_class
													, CTEXTSTR public_name
													  );
#define ReleaseRegisteredFunction(nc,pn) ReleaseRegisteredFunctionEx(NULL,nc,pn)

/* This is a macro used to paste two symbols together. */
#define paste_(a,b) _WIDE(a##b)
#define paste(a,b) paste_(a,b)


#define ___DefineRegistryMethod2(task,name,classtype,methodname,desc,returntype,argtypes,line)   \
	CPROC paste(name,line)argtypes;       \
	PRIORITY_PRELOAD( paste(paste(paste(Register,name),Method),line), SQL_PRELOAD_PRIORITY ) {  \
	SimpleRegisterMethod( task WIDE("/") classtype, paste(name,line)  \
	, _WIDE(#returntype), methodname, _WIDE(#argtypes) ); \
   RegisterValue( task WIDE("/") classtype WIDE("/") methodname, WIDE("Description"), desc ); \
}                                                                          \
	static returntype CPROC paste(name,line)

#define __DefineRegistryMethod2(task,name,classtype,methodname,desc,returntype,argtypes,line)   \
	___DefineRegistryMethod2(task,name,classtype,methodname,desc,returntype,argtypes,line)

#define ___DefineRegistryMethod2P(priority,task,name,classtype,methodname,desc,returntype,argtypes,line)   \
	CPROC paste(name,line)argtypes;       \
	PRIORITY_PRELOAD( paste(paste(paste(Register,name),Method),line), priority ) {  \
	SimpleRegisterMethod( task WIDE("/") classtype, paste(name,line)  \
	, _WIDE(#returntype), methodname, _WIDE(#argtypes) ); \
   RegisterValue( task WIDE("/") classtype WIDE("/") methodname, WIDE("Description"), desc ); \
}                                                                          \
	static returntype CPROC paste(name,line)

#define __DefineRegistryMethod2P(priority,task,name,classtype,methodname,desc,returntype,argtypes,line)   \
	___DefineRegistryMethod2P(priority,task,name,classtype,methodname,desc,returntype,argtypes,line)

#define ___DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes,line)   \
	CPROC paste(name,line)argtypes;       \
	PRELOAD( paste(Register##name##Button,line) ) {  \
	SimpleRegisterMethod( task WIDE("/") classtype WIDE("/") classbase, paste(name,line)  \
	, _WIDE(#returntype), methodname, _WIDE(#argtypes) ); \
}                                                                          \
	static returntype CPROC paste(name,line)

#define __DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes,line)   \
	___DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes,line)

#define _DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes,line)   \
	static returntype __DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes,line)

#define DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes)  \
	__DefineRegistryMethod(task,name,classtype,classbase,methodname,returntype,argtypes,__LINE__)

// this macro is used for ___DefineRegistryMethodP. Because this is used with complex names
// an extra define wrapper of priority_preload must be used to fully resolve paramters.
#define PRIOR_PRELOAD(a,p) PRIORITY_PRELOAD(a,p)
#define ___DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes,line)   \
	CPROC paste(name,line)argtypes;       \
	PRIOR_PRELOAD( paste(Register##name##Button,line), priority ) {  \
	SimpleRegisterMethod( task WIDE("/") classtype WIDE("/") classbase, paste(name,line)  \
	, _WIDE(#returntype), methodname, _WIDE(#argtypes) ); \
}                                                                          \
	static returntype CPROC paste(name,line)

/* <combine sack::app::registry::SimpleRegisterMethod>
   
   General form to build a registered procedure. Used by simple
   macros to create PRELOAD'ed registered functions. This flavor
   requires the user to provide 'static' and a return type that
   matches the return type specified in the macro. This makes
   usage most C-like, and convenient to know what the return
   value of a function should be (if any).
   
   
   Parameters
   priority :    The preload priority to load at.
   task :        process level name registry. This would be
                 "Intershell" or "psi" or some other base prefix.
                 The prefix can contain a path longer than 1
                 level.
   name :        This is the function name to build. (Can be used
                 for link debugging sometimes)
   classtype :   class of the name being registered
   methodname :  name of the routine to register
   returntype :  the literal type of the return type of this
                 function (void, int, PStruct* )
   argtypes :    Argument signature of the routine in parenthesis
   line :        this is usually filled with __LINE__ so that the
                 same function name (name) will be different in
                 different files (even in the same file)
   
   Remarks
   This registers a routine at the specified preload priority.
   Registers under [task]/[classname]/methodname. The name of
   the registered routine from a C perspective is [name][line]. This
   function is not called directly, but will only be referenced
   from the registered name.
   
   
   
   
   Example
   See <link sack::app::registry::GetFirstRegisteredNameEx@PCLASSROOT@CTEXTSTR@PCLASSROOT *, GetFirstRegisteredNameEx> */
#define __DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes,line)   \
	___DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes,line)


#define _DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes,line)   \
	__DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes,line)

#define DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes)  \
	_DefineRegistryMethodP(priority,task,name,classtype,classbase,methodname,returntype,argtypes,__LINE__)

#define _DefineRegistrySubMethod(task,name,classtype,classbase,methodname,subname,returntype,argtypes,line)   \
	CPROC paste(name,line)argtypes;       \
	PRELOAD( paste(Register##name##Button,line) ) {  \
	SimpleRegisterMethod( task WIDE("/") classtype WIDE("/") classbase WIDE("/") methodname, paste(name,line)  \
	, _WIDE(#returntype), subname, _WIDE(#argtypes) ); \
}                                                                          \
	static returntype CPROC paste(name,line)

#define DefineRegistrySubMethod(task,name,classtype,classbase,methodname,subname,returntype,argtypes)  \
	_DefineRegistrySubMethod(task,name,classtype,classbase,methodname,subname,returntype,argtypes,__LINE__)

/* attempts to use dynamic linking functions to resolve passed
   global name if that fails, then a type is registered for this
   global, and an instance created, so that that instance may be
   reloaded again, otherwise the data in the main application is
   used... actually we should deprecate the dynamic loading
   part, and just register the type.
   
   
   
   SimpleRegisterAndCreateGlobal Simply registers the type as a
   global variable type. Allows creation of the global space
   later.
   Parameters
   name :         name of the pointer to global type to create.<p />text
                  string to register this created global as.
   ppGlobal :     address of the pointer to global memory.
   global_size :  size of the global area to create
   Example
   <code lang="c++">
   typedef struct {
      int data;
   } my_global;
   my_global *global;
   
   PRELOAD( Init )
   {
       SimpleRegisterAndCreateGlobal( global );
   }
   </code>                                                               */
PROCREG_PROC( void, RegisterAndCreateGlobal )( POINTER *ppGlobal, uintptr_t global_size, CTEXTSTR name );
/* <combine sack::app::registry::RegisterAndCreateGlobal@POINTER *@uintptr_t@CTEXTSTR>
   
   \ \                                                                                   */
#define SimpleRegisterAndCreateGlobal( name ) 	RegisterAndCreateGlobal( (POINTER*)&name, sizeof( *name ), WIDE(#name) )
/* Init routine is called, otherwise a 0 filled space is
   returned. Init routine is passed the pointer to the global
   and the size of the global block the global data block is
   zero initialized.
   Parameters
   ppGlobal :     Address of the pointer to the global region
   global_size :  size of the global region to create
   name :         name of the global region to register (so
                  future users get back the same data area)
   Init :         function to call to initialize the region when
                  created. (doesn't have to be a global. Could be
                  used to implement types that have class
                  constructors \- or not, since there's only one
                  instance of a global \- this is more for
                  singletons).
   Example
   <code>
   typedef struct {
      int data;
   } my_global;
   my_global *global;
   </code>
   <code lang="c++">
   
   void __cdecl InitRegion( POINTER region, uintptr_t region_size )
   {
       // do something to initialize 'region'
   }
   
   PRELOAD( InitGlobal )
   {
       SimpleRegisterAndCreateGlobalWithInit( global, InitRegion );
   }
   </code>                                                          */
PROCREG_PROC( void, RegisterAndCreateGlobalWithInit )( POINTER *ppGlobal, uintptr_t global_size, CTEXTSTR name, void (CPROC*Init)(POINTER,uintptr_t) );
/* <combine sack::app::registry::RegisterAndCreateGlobalWithInit@POINTER *@uintptr_t@CTEXTSTR@void __cdecl*InitPOINTER\,uintptr_t>
   
   \ \                                                                                                                              */
#define SimpleRegisterAndCreateGlobalWithInit( name,init ) 	RegisterAndCreateGlobalWithInit( (POINTER*)&name, sizeof( *name ), WIDE(#name), init )

/* a tree dump will result with dictionary names that may translate automatically. */
/* This has been exported as a courtesy for StrDup.
 * this routine MAY result with a translated string.
 * this routine MAY result with the same pointer.
 * this routine MAY need to be improved if MANY more strdups are replaced
 * Add a binary tree search index when large.
 * Add a transaltion tree index at the same time.
 */
PROCREG_PROC( CTEXTSTR, SaveNameConcatN )( CTEXTSTR name1, ... );
// no space stripping, saves literal text
PROCREG_PROC( CTEXTSTR, SaveText )( CTEXTSTR text );


PROCREG_NAMESPACE_END


#ifdef __cplusplus
	using namespace sack::app::registry;
#endif

#endif
