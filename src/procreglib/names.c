#define NO_UNICODE_C
#define PROCREG_SOURCE
#include <stdhdrs.h>
#include <sharemem.h>
#include <filesys.h>
#include <configscript.h>

#ifdef __ANDROID__
#  ifdef DEBUG_FIRST_UNICODE_OPERATION
#    include <android/log.h>
#  endif
#endif

//#define DEBUG_GLOBAL_REGISTRATION
#define REGISTRY_STRUCTURE_DEFINED
//#define PROCREG_SOURCE
// include this first so we can have the namespace...
#include <procreg.h>
#include <sqlgetoption.h>
#undef REGISTRY_STRUCTURE_DEFINED
#include "registry.h"

PROCREG_NAMESPACE

// using lower level syslog bypasses some allocation requirements...
//#define lprintf( f, ... ) { TEXTCHAR buf[256]; tnprintf( buf, 256, f,##__VA_ARGS__ ); SystemLogFL( buf DBG_SRC ); }

	static struct procreg_local_private_tag {
		// don't use critical sections while registering.
		struct {
			BIT_FIELD enable_critical_sections : 1;
		} flags;

	} procreg_local_private_data;

struct tmp_namebuf
{
	TEXTCHAR *buffer;
	size_t length;
};

struct procreg_local_tag {
	struct {
		BIT_FIELD bInterfacesLoaded : 1;
		BIT_FIELD bIndexNameTable : 1;

		// if neither of the next two are set, then the statement can be processed; both are cleared at endif
		BIT_FIELD bFindEndif : 5; // set if 'if' condition is false - SUPPORT 32 levels of if (more than 2 and you're crazy with this)
		BIT_FIELD bFindElse : 1; // set if 'if' condition is false - SUPPORT 32 levels of if (more than 2 and you're crazy with this)
		//BIT_FIELD bIfSuccess : 1; // set if 'if' condition is true
		BIT_FIELD bTraceInterfaceLoading : 1;
		BIT_FIELD bDisableMemoryLogging : 1;
		BIT_FIELD bReadConfiguration : 1; // having read the configuration file
		BIT_FIELD bHeldDeadstart : 1;
	} flags;

	PTREEDEF Names;
	PTREEROOT NameIndex;
	PTREEDEFSET TreeNodes;
	PNAMESET NameSet;

	PNAMESPACE NameSpace;
	PLIST TransationSpaces;

	int translations; // open group ID

	TEXTCHAR *config_filename;
	FILE *file;
	CRITICALSECTION csName;
	uint32_t simple_lock;

	PLINKQUEUE tmp_names;
	int reference_count; 
	PLIST global_spaces;
	//gcroot<System::IO::FileStream^> fs;
};

#ifdef l
#   undef l
#endif
#ifdef __cplusplus
#  define procreg_local_data  procreg_local_data_pp
#endif
#define l (*procreg_local_data)

static struct procreg_local_tag *procreg_local_data;

static CTEXTSTR SaveName( CTEXTSTR name );
PTREEDEF GetClassTreeEx( PCTREEDEF root
										, PCTREEDEF name_class
										, PTREEDEF alias, LOGICAL bCreate );
#define GetClassTree( root, name_class ) GetClassTreeEx( root, name_class, NULL, TRUE )

//---------------------------------------------------------------------------


static int CPROC SavedNameCmpEx(CTEXTSTR dst, CTEXTSTR src, size_t srclen)
{
	// NUL does not nessecarily terminate strings
	// instead slave off the length...
	 TEXTCHAR f,last;
	size_t l1 = srclen; // one for length, one for nul
	size_t l2 = dst[-1] - 2; // one for length, one for nul
	// case insensitive loop..
	//lprintf( WIDE("Compare %s(%d) vs %s[%p](%d)"), src, l1, dst, dst, l2 );
	// interesting... first sort by length
	// and then by content?
	//if( l1 != l2 )
	 //  return l2-l1;
	do {
		if( (*src) == '\\' || (*src)=='/' )
		{
			l1 = 0; // no more length .. should have gotten a matched length on dst...
			break;
		}
		if ( ((f = (TEXTCHAR)(*(dst++))) >= 'A') && (f <= 'Z') )
			f -= ('A' - 'a');
		if ( ((last = (TEXTCHAR)(*(src++))) >= 'A') && (last <= 'Z') )
			last -= ('A' - 'a');
		--l2;
		--l1;
	} while ( l2 && l1 && (f == last) );
	//lprintf( WIDE("Results to compare...%d,%d  %c,%c"), l1, l2, f, last );
	// if up to the end of some portion of the strings matched...
	if( !f && !last )
	{
		return 0;
	}
	if( !l2 && !l1 )
	{
		return f-last;
	}
	if( f == last )
	{
		if( l2 && !l1 )
			return 1;
		if( l1 && !l2 )
			return -1;
	}
	return(f - last);
}
//---------------------------------------------------------------------------

static int CPROC SavedNameCmp(CTEXTSTR dst, CTEXTSTR src)
{
	//lprintf( WIDE("Compare names... (tree) %s,%s"), dst, src );
	if( !src && !dst )
		return 0;
	if( !src ) {
		DebugBreak();
		return 1;
	}
	if( !dst && src )
		return -1;

	return SavedNameCmpEx( dst, src, src[-1]-2 );
}
//---------------------------------------------------------------------------

static TEXTSTR StripName( TEXTSTR buf, CTEXTSTR name )
{
	TEXTSTR savebuf = buf;
	int escape = 0;
	if( !name )
	{
		buf[0] = 0;
		return buf;
	}
	while( name[0] )
	{
		if( name[0] == '\\' )
		{
			escape = 1;
		}
		else
		{
			// drop spaces...
			if( escape || ( name[0] > ' ' && name[0] < 127 ) )
			{
				*(buf++) = name[0];
			}
			escape = 0;
		}
		name++;
	}
	buf[0] = 0;
	return savebuf;
}

//---------------------------------------------------------------------------

static TEXTSTR GetFullName( CTEXTSTR name )
{
	int len;
	int out;
	int totlen = name[-1];
	TEXTSTR result;
	//for( len = 0; name[len] != 0 || name[len+1] != 0; len++ );
	result = NewArray( TEXTCHAR, totlen + 1);
	out = 0;
	for( len = 0; len < totlen; len++ )
		if( name[len] )
			result[out++] = name[len];
	result[out] = 0;
	return result;
	

}

//---------------------------------------------------------------------------


static CTEXTSTR DressName( TEXTSTR buf, CTEXTSTR name )
{
	TEXTSTR savebuf = buf;
	savebuf[0] = 2;
	buf++;
	if( !name )
	{
		savebuf[0] = 0;
		return buf;
	}
	while( name[0] )
	{
		if( name[0] == '/' || name[0] == '\\' )
			break;
		if( name[0] < ' ' || name[0] >= 127 )
		{
			savebuf[0]++;
			(*buf++) = '\\';
			savebuf[0]++;
			(*buf++) = name[0];
		}
		else
		{
			savebuf[0]++;
			(*buf++) = name[0];
		}
		name++;
	}
	buf[0] = 0;
	return savebuf + 1;
}

//---------------------------------------------------------------------------

static CTEXTSTR DoSaveNameEx( CTEXTSTR stripped, size_t len DBG_PASS )
#define DoSaveName(a,b) DoSaveNameEx(a,b DBG_SRC )
{
	PNAMESPACE space = l.NameSpace;
	TEXTCHAR *p = NULL;
	// cannot save 0 length strings.
	if( !stripped || !stripped[0] || !len )
	{
		//lprintf( WIDE("zero length string passed") );
		return NULL;
	}

	// otherwise it will be single threaded?
	if( procreg_local_private_data.flags.enable_critical_sections )
	{
#if USE_CUSTOM_ALLOCER
		EnterCriticalSec( &l.csName );
#else
		while( LockedExchange( &l.simple_lock, 1 ) )
			Relinquish();
#endif
	}
	if( l.flags.bIndexNameTable )
	{
		POINTER p;
		p = (POINTER)FindInBinaryTree( l.NameIndex, (uintptr_t)stripped );
		if( p )
		{
			// otherwise it will be single threaded?
			if( procreg_local_private_data.flags.enable_critical_sections )
			{
#if USE_CUSTOM_ALLOCER
				LeaveCriticalSec( &l.csName );
#else
				l.simple_lock = 0;
#endif
			}
			return ((CTEXTSTR)p);
		}
	}
	else
	{
		for( space = l.NameSpace; space; space = space->next )
		{
			p = space->buffer;
			while( p[0] && len )
			{
				//lprintf( WIDE("Compare %s(%d) vs %s(%d)"), p+1, p[0], stripped,len );
				if( SavedNameCmpEx( p+1, stripped, len ) == 0 )
				{
					// otherwise it will be single threaded?
					if( procreg_local_private_data.flags.enable_critical_sections )
					{
						l.simple_lock = 0;
						//LeaveCriticalSec( &l.csName );
					}
					return (CTEXTSTR)p+1;
				}
				p +=
#if defined( __ARM__ ) || defined( UNDER_CE )
					(
#endif
					 p[0]
#if defined( __ARM__ ) || defined( UNDER_CE )
					 +3 ) & 0xFC;
#endif
				;
			}
		}
	}
	for( space = l.NameSpace; space; space = space->next )
	{
		//lprintf( "Finding next name space free %p %p %p", l.NameSpace, space, space->next );
		if( ( space->nextname + len ) < ( NAMESPACE_SIZE - 3 ) )
		{
			p = NULL;
			break;
		}
	}
	if( !space || !p )
	{
		size_t alloclen;
		if( !space )
		{
			space = (PNAMESPACE)Allocate( sizeof( NAMESPACE ) );
			space->nextname = 0;
			//lprintf( "Adding new namespace %p", space );
			LinkThing( l.NameSpace, space );
		}
		MemCpy( p = space->buffer + space->nextname + 1, stripped,(uint32_t)(sizeof( TEXTCHAR)*(len + 1)) );
		p[len] = 0; // make sure we get a null terminator...
		// +2 1 for byte of len, 1 for nul at end.
		alloclen = (len + 2);
		space->buffer[space->nextname] = (TEXTCHAR)(alloclen);
		space->nextname += (uint32_t)alloclen;
		space->buffer[space->nextname] = 0;
#if defined( __ARM__ ) || defined( UNDER_CE )
		space->nextname = ( space->nextname + 3 ) & 0xFFFFC;
		// +3&0xFC rounds to next full dword segment
		// arm requires this name be aligned on a dword boundry
		// because later code references this as a DWORD value.
#endif

		if( l.flags.bIndexNameTable )
		{
			AddBinaryNode( l.NameIndex, p, (uintptr_t)p );
			//BalanceBinaryTree( l.NameIndex );
		}
	}
	// otherwise it will be single threaded?
	if( procreg_local_private_data.flags.enable_critical_sections )
	{
#if USE_CUSTOM_ALLOCER
		LeaveCriticalSec( &l.csName );
#else
		l.simple_lock = 0;
#endif
	}
	return (CTEXTSTR)p;
}

//---------------------------------------------------------------------------

static CTEXTSTR SaveName( CTEXTSTR name )
{
	if( name )
	{
		size_t len = StrLen( name );
		struct tmp_namebuf *tmp_namebuf = (struct tmp_namebuf*)DequeLink( &l.tmp_names );
		TEXTSTR stripped;
		size_t n;
		if( !tmp_namebuf )
		{
			tmp_namebuf = New( struct tmp_namebuf );
			tmp_namebuf->length = len + 2;
			tmp_namebuf->buffer = NewArray( TEXTCHAR, len + 2 );
		}
		else
		{
			if( tmp_namebuf->length < ( len + 2 ) )
			{
				Release( tmp_namebuf->buffer );
				tmp_namebuf->length = len + 2;
				tmp_namebuf->buffer = NewArray( TEXTCHAR, len + 2 );
			}
		}
		stripped = tmp_namebuf->buffer;
		stripped[0] = (TEXTCHAR)(len + 2);
		for( n = 0; n < len; n++ )
			if( name[n] == '\\' || name[n] == '/' )
			{
				len = n;
				break;
			}
		StrCpyEx( stripped + 1, name, len + 1 ); // allow +1 length for null after string; otherwise strncpy dropps the nul early
		stripped[0] = (TEXTCHAR)(len + 2);
		{
			CTEXTSTR result = DoSaveName( stripped + 1, len );
			EnqueLink( &l.tmp_names, tmp_namebuf );
			return result;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------
CTEXTSTR SaveNameConcatN( CTEXTSTR name1, ... )
#define SaveNameConcat(n1,n2) SaveNameConcatN( (n1),(n2),NULL )
{
	// space concat since that's eaten by strip...
	TEXTCHAR _stripbuffer[256];
	TEXTCHAR *stripbuffer = (_stripbuffer+1);
	size_t len = 0;
	CTEXTSTR namex;
	va_list args;
	va_start( args, name1 );

	for( namex = name1;
			 namex;
			 namex = va_arg( args, CTEXTSTR ) )
	{
		size_t newlen;
		// concat order for libraries is
		// args, return type, library, library_procname
		// this is appeneded to the key value FUNCTION
		//lprintf( WIDE("Concatting %s"), namex );
		newlen = StrLen( StripName( stripbuffer + len, namex ) );
		//if( newlen )
		newlen++;
		len += newlen;
	}
	_stripbuffer[0] = (TEXTCHAR)(len + 2);
	// and add another - final part of string is \0\0
	//stripbuffer[len] = 0;
	//len++;
	return DoSaveName( stripbuffer, len );
}

//---------------------------------------------------------------------------
CTEXTSTR SaveText( CTEXTSTR text )
#define SaveNameConcat(n1,n2) SaveNameConcatN( (n1),(n2),NULL )
{
	size_t len = StrLen( text );
	TEXTSTR stripped = NewArray( TEXTCHAR, len + 2 );
	CTEXTSTR result;
	StrCpyEx( stripped + 1, text, len + 1 );
	stripped[0] = (TEXTCHAR)(len + 2);
	result = DoSaveName( stripped + 1, len);
	Release( stripped );
	return result;
}

//---------------------------------------------------------------------------

static void CPROC KillName( CPOINTER user, uintptr_t key )
{
	PNAME name = (PNAME)user;
	if( name->tree.Tree )
	{
	}
	else if( name->flags.bValue )
	{
	}
	else if( name->flags.bProc )
	{
	}
	else if( name->flags.bData )
	{
	}
	//DeleteFromSet( NAME, l.TreeNodes, user );
}

//---------------------------------------------------------------------------

// p would be the global space, but it's also already set in it's correct spot
static void CPROC InitGlobalSpace( POINTER p, uintptr_t size )
{
	InitializeCriticalSec( &(*(struct procreg_local_tag*)p).csName );

	(*(struct procreg_local_tag*)p).Names = (PTREEDEF)GetFromSet( TREEDEF, &(*(struct procreg_local_tag*)p).TreeNodes );
	(*(struct procreg_local_tag*)p).Names->Magic = MAGIC_TREE_NUMBER;
	(*(struct procreg_local_tag*)p).Names->Tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, (int(CPROC *)(uintptr_t,uintptr_t))SavedNameCmp, KillName );

	// enable name indexing.
	// if we have 500 names, 9 searches is much less than 250 avg
	(*(struct procreg_local_tag*)p).flags.bIndexNameTable = 1;
	(*(struct procreg_local_tag*)p).NameIndex = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, (int(CPROC *)(uintptr_t,uintptr_t))SavedNameCmp, KillName );
	(*(struct procreg_local_tag*)p).reference_count++;
}

static void Init( void )
{
	// don't call this function, preserves the process line cache, just check the flag and simple skip any call.
	// use SAFE_INIT();
#define SAFE_INIT() if( !procreg_local_data ) RegisterAndCreateGlobalWithInit( (POINTER*)&procreg_local_data, sizeof( *procreg_local_data ), "procreg_local_data", InitGlobalSpace )
	SAFE_INIT();
}

static void ReadConfiguration( void );

//PRIORITY_UNLOAD( InitProcreg, NAMESPACE_PRELOAD_PRIORITY )
//{
	// release other members too, kindly
	//Deallocate( struct procreg_local_tag*, procreg_local_data );
	//procreg_local_data = NULL;
//}
PRIORITY_PRELOAD( InitProcReg2, SYSLOG_PRELOAD_PRIORITY )
{
	// this has to be done after timer's init is done, which is SYSLOG_PRELOAD_PRIORITY-1
	procreg_local_private_data.flags.enable_critical_sections = 1;
}

PRIORITY_PRELOAD( InitProcreg, NAMESPACE_PRELOAD_PRIORITY )
{
	Init();
#ifndef __NO_INTERFACE_SUPPORT__
#ifndef __NO_DEFAULT_INTERFACES__
	if( !l.flags.bReadConfiguration )
	{
		l.flags.bReadConfiguration = 1;
		ReadConfiguration();
	}
#endif
#endif
#ifndef __NO_OPTIONS__
	l.flags.bDisableMemoryLogging = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Process Registry/Disable Memory Logging"), 1, TRUE );
#else
	l.flags.bDisableMemoryLogging = 1;
#endif
}

//---------------------------------------------------------------------------

int GetClassPath( TEXTSTR out, size_t len, PCLASSROOT root )
{
	int ofs = 0;
	PLINKSTACK pls = CreateLinkStack();
	PTREEDEF current;
	PNAME name;
	for( current = (PTREEDEF)root; current && current->self; current = current->self->parent )
	{
		PushLink( &pls, current->self );
	}
	while( ( name = (PNAME)PopLink( &pls ) ) )
	{
		//pcr->
		ofs += tnprintf( out + ofs, len - ofs, WIDE("/%s"), name->name );
	}
	DeleteLinkStack( &pls );
	return ofs;
}

//---------------------------------------------------------------------------

static PTREEDEF AddClassTree( PCTREEDEF class_root, TEXTCHAR *name, PTREEROOT root, int bAlias )
{
	if( root && class_root )
	{
		PNAME classname = GetFromSet( NAME, &l.NameSet ); //Allocate( sizeof( NAME ) );
		//MemSet( classname, 0, sizeof( NAME ) );
		classname->flags.bAlias = bAlias;
		classname->name = SaveName( name );

		classname->tree.Magic = MAGIC_TREE_NUMBER;
		classname->tree.Tree = root;
		classname->tree.self = classname;
		classname->flags.bTree = TRUE;
		classname->parent = (PTREEDEF)class_root;

		//lprintf( WIDE("Adding class tree thing %p  %s"), class_root->Tree, classname->name );
		if( !AddBinaryNode( class_root->Tree, classname, (uintptr_t)classname->name ) )
		{
			//Log( WIDE("For some reason could not add new class tree to tree!") );
			DeleteFromSet( NAME, l.NameSet, classname );
			return NULL;
		}
		return &classname->tree;
	}
	return NULL;
}

//---------------------------------------------------------------------------

static CTEXTSTR  my_pathchr ( CTEXTSTR path )
{
	CTEXTSTR end1, end2;
	end1 = StrChr( path, (int)'\\' );
	end2 = StrChr( path, (int)'/' );
	if( end1 && end2 )
	{
		if( end1 < end2 )
			return end1;
		return end2;
	}
	else if( end1 )
		return end1;
	else if( end2 )
		return end2;
	return NULL;
}

//---------------------------------------------------------------------------


// if name_class is NULL then root is returned.
// if name_class is not NULL then if name_class references
// PTREEDEF structure, then name_class is returned.
// if root is NULL then it is set to l.nmaes... if this library has
// never been initialized it will return NULL.
// if name_class does not previously exist, then it is created.
// There is no protection for someone to constantly create large trees just
// by asking for them.
PTREEDEF GetClassTreeEx( PCTREEDEF root, PCTREEDEF _name_class, PTREEDEF alias, LOGICAL bCreate )
{
	PCTREEDEF class_root;

	if( !root )
	{
		Init();
		root = (PCTREEDEF)l.Names;
	}// fix root...
	class_root = root;

	if(
#if defined( __ARM__ ) || defined( UNDER_CE )
		// if its odd, it comes from the name space
		// (savename)
		(((uintptr_t)class_root)&0x3) ||
#endif
		(class_root->Magic != MAGIC_TREE_NUMBER) )
	{
		// if root name is passed as a NAME, then resolve it
		// assuming the root of all names as the root...
		class_root = GetClassTreeEx( l.Names, class_root, NULL, bCreate );
	}

	if( _name_class )
	{
		if(
#if defined( __ARM__ ) || defined( UNDER_CE )
	  // if its odd, it comes from the name space
		// (savename)
			 !(((uintptr_t)_name_class)&0x3) &&
#endif
			(_name_class->Magic == MAGIC_TREE_NUMBER) )
		{
			return (PTREEDEF)_name_class;
		}
		else
		{
			size_t buflen = 0;
			//TEXTCHAR *original;
			TEXTCHAR *end, *start;
			CTEXTSTR name_class = (CTEXTSTR)_name_class;
			size_t len = StrLen( name_class ) + 1;
			PNAME new_root;
			int retry = 0;
			if( len > buflen )
			{
				buflen = len + 32;
			}
			start = (TEXTCHAR*)name_class;
			do
			{
				end = (TEXTCHAR*)my_pathchr( start );
				do
				{
					if( end == start )
					{
						start = start+1;
						end = (TEXTCHAR*)my_pathchr( start );
						continue;
					}
					if( !end || ((my_pathchr(end+1) - end) != 1) )
						break;
					end++;
				}
				while( 1 );

				do
				{
					{
						// dress name terminates on a '/'
						TEXTCHAR buf[256];
						//lprintf( "Finding a..." );
						new_root = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, start ) );
						//lprintf( WIDE("Found %p %s(%d)=%s"), new_root, buf+1, buf[0], start );
					}
					if( !new_root )
					{
						if( !bCreate )
							return NULL;
						if( alias && !end )
						{
							// added name in this place name terminates on a '/'
							//lprintf( WIDE("name not found, adding...!end && alias") );
							class_root = AddClassTree( class_root
															 , start
															 , alias->Tree
															 , TRUE );
							((PTREEDEF)class_root)->self = alias->self;
						}
						else
						{
							PTREEDEF new_root;
							PTREEROOT tree;

							// added name in this place name terminates on a '/'

							// interesting note - while searching for
							// a member, branches are created.... should consider
							// perhaps offering an option to read for class root without creating
							// however it gives one an idea of what methods might be avaialable...
							//lprintf( WIDE("name not found, adding.. [%s] %s"), start, class_root->self?class_root->self->name:"." );
							new_root = AddClassTree( class_root
															 , start
															 , tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
																						 , (int(CPROC *)(uintptr_t,uintptr_t))SavedNameCmp
																						 , KillName )
															 , FALSE
															 );
							if( !new_root )
							{
								// if this happens it was probably added while adding...
								DebugBreak();
								DestroyBinaryTree( tree );
								if( retry < 2 )
								{
									retry++;
									continue;
								}
#ifndef NO_LOGGING
								SystemLog( WIDE("Failed to register...") );
								lprintf( WIDE("name not found, adding.. [%s] %s"), start, class_root->self?class_root->self->name:WIDE(".") );
#endif
								return NULL;
							}
							class_root = new_root;
						}
					}
					else
					{
						if( !end && alias && !new_root->flags.bAlias )
						{
							static int error_count;
							error_count++;
							// this orphans the prior tree; but probably results from requests for values that aren't present
							// and later are filled by an alias.
							if( error_count > 20 )
								lprintf( WIDE( " Name %s exists, but we want it to be an alias, and it is not...(a LOT of this is bad) " ), new_root->name );

							if( new_root->tree.Magic != MAGIC_TREE_NUMBER )
								lprintf( WIDE( "Hell it's not even a tree!" ) );
							new_root->flags.bAlias = 1;
							new_root->tree.Tree = alias->Tree;
							new_root->tree.self = alias->self;
						}
						class_root = &new_root->tree;
					}
					break;
				} while( 1 );
				if( end )
					start = end + 1;
				else
					break;
			}
			while( class_root && start[0] );
		}
	}
	return (PTREEDEF)class_root;
}

//---------------------------------------------------------------------------
PROCREG_PROC( PCLASSROOT, CheckClassRoot )( CTEXTSTR name_class )
{
	return (PCLASSROOT)GetClassTreeEx( NULL, (PTREEDEF)name_class, NULL, FALSE );
}

//---------------------------------------------------------------------------

PROCREG_PROC( PCLASSROOT, GetClassRootEx )( PCLASSROOT root, CTEXTSTR name_class )
{
	return (PCLASSROOT)GetClassTreeEx( (PTREEDEF)root, (PTREEDEF)name_class, NULL, TRUE );
}

PROCREG_PROC( PCLASSROOT, GetClassRoot )( CTEXTSTR name_class )
{
	return (PCLASSROOT)GetClassTreeEx( l.Names, (PTREEDEF)name_class, NULL, TRUE );
}
#ifdef __cplusplus
PROCREG_PROC( PCLASSROOT, GetClassRootEx )( PCLASSROOT root, PCLASSROOT name_class )
{
	return GetClassTreeEx( root, (PTREEDEF)name_class, NULL, TRUE );
}

PROCREG_PROC( PCLASSROOT, GetClassRoot )( PCLASSROOT name_class )
{
	return GetClassTreeEx( l.Names, (PTREEDEF)name_class, NULL, TRUE );
}
#endif
//---------------------------------------------------------------------------

int AddNode( PTREEDEF class_root, POINTER data, uintptr_t key )
{
	if( class_root )
	{
		TEXTCHAR buf[256];
		PNAME oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, (CTEXTSTR)key ) );
		if( oldname )
		{
			//lprintf( WIDE("Name already in the tree... %s"), (CTEXTSTR)key );
			return FALSE;
		}
		else
		{
			//lprintf( WIDE("addnode? a data ndoe - create data structure") );
			if( !AddBinaryNode( class_root->Tree, data, key ) )
			{
				Log( WIDE("For some reason could not add new name to tree!") );
				return FALSE;
			}
		}
		return TRUE;
	}
	Log( WIDE("Nowhere to add the node...") );
	return FALSE;
}

//---------------------------------------------------------------------------

static int CPROC MyStrCmp( uintptr_t s1, uintptr_t s2 )
{
	//lprintf( WIDE("Compare (%s) vs (%s)"), s1, s2 );
	return StrCaseCmp( (TEXTCHAR*)s1, (TEXTCHAR*)s2 );
}
//---------------------------------------------------------------------------
#undef RegisterFunctionExx
PROCREG_PROC( LOGICAL, RegisterFunctionExx )( PCLASSROOT root
													 , PCLASSROOT name_class
													 , CTEXTSTR public_name
													 , CTEXTSTR returntype
													 , PROCEDURE proc
													 , CTEXTSTR args
													 , CTEXTSTR library
													 , CTEXTSTR real_name
													  DBG_PASS
													)
{
	if( root || name_class )
	{
		PNAME newname = GetFromSet( NAME, &l.NameSet );//Allocate( sizeof( NAME ) );
		TEXTCHAR strippedargs[256];
		CTEXTSTR func_name = real_name?real_name:public_name;
		CTEXTSTR root_func_name = func_name;
		PTREEDEF class_root = (PTREEDEF)GetClassTree( (PCTREEDEF)root, (PCTREEDEF)name_class );
		int tmp;
		MemSet( newname, 0, sizeof( NAME ) );
		newname->flags.bProc = 1;
		// this is kinda messed up...
		newname->name = SaveName( public_name );
		newname->data.proc.library = SaveName( library );
		newname->data.proc.procname = SaveName( real_name );
		//newname->data.proc.ret = SaveName( returntype );
		for( tmp = 0; func_name[tmp]; tmp++ )
			if( func_name[tmp] == '/' ||
				func_name[tmp] == '\\' )
			{
				func_name = func_name + tmp + 1;
				tmp = -1;
			}
		if( func_name != root_func_name )
		{
			size_t len;
			TEXTSTR new_root_func_name = NewArray( TEXTCHAR, len = ( func_name - root_func_name ) );
			StrCpyEx( new_root_func_name, root_func_name, len );
			new_root_func_name[len-1] = 0;
			//lprintf( "trimmed name would be %s  /   %s", new_root_func_name, func_name );
			class_root = GetClassTree( (PCTREEDEF)class_root, (PCTREEDEF)new_root_func_name );
			Release( new_root_func_name );
		}
		//newname->data.proc.args = SaveName( StripName( strippedargs, args ) );

		newname->data.proc.name = SaveNameConcatN( StripName( strippedargs, args )
															  , returntype
															  , library?library:WIDE("_")
															  , func_name
															  , NULL
															  );
		newname->data.proc.proc = proc;
		if( class_root )
		{
			PNAME oldname;
			oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)newname->name);
			if( oldname )
			{
				if( !oldname->data.proc.proc )
				{
					// old branch location might have existed, but no value assigned...
					//lprintf( WIDE( "overloading prior %p with %p and %p with %p" )
					//		 , oldname->data.proc.proc, proc
					//		 , oldname->data.proc.name, newname->data.proc.name
					//		 );
					oldname->flags.bProc = 1;
					oldname->data.proc.proc = proc;
					oldname->data.proc.name = newname->data.proc.name;
					oldname->data.proc.library = newname->data.proc.library;
					oldname->data.proc.procname = newname->data.proc.procname;
					newname->data.proc.name = NULL;
				}
				else if( oldname->data.proc.proc == proc )
					Log( WIDE("And fortunatly it's the same address... all is well...") );
				else
				{
					TEXTSTR s1, s2;
#ifndef NO_LOGGING
					CTEXTSTR file = GetRegisteredValue( (CTEXTSTR)&oldname->tree, WIDE( "Source File" ) );
					int line = (int)(uintptr_t)GetRegisteredValueEx( (CTEXTSTR)&oldname->tree, WIDE( "Source Line" ), TRUE );
					_xlprintf( 2 DBG_RELAY)( WIDE("proc %s/%s regisry by %s of %s(%s) conflicts with %s(%d):%s(%s)...")
												  , (CTEXTSTR)name_class?(CTEXTSTR)name_class:WIDE("@")
												  , public_name?public_name:WIDE("@")
												  , newname->name
												  , s1 = GetFullName( newname->data.proc.name )
													//,library
												  , newname->data.proc.procname
												  , file
												  , line
												  , s2 = GetFullName( oldname->data.proc.name )
												  //,library
												  , oldname->data.proc.procname );
					DumpRegisteredNames();
					Release( s1 );
					Release( s2 );
					// perhaps it's same in a different library...
					Log( WIDE("All is not well - found same function name in tree with different address. (ignoring second) ") );
#endif
				}
				DeleteFromSet( NAME, l.NameSet, newname );
				return TRUE;
			}
			else
			{
				if( !AddBinaryNode( class_root->Tree, (PCLASSROOT)newname, (uintptr_t)newname->name ) )
				{
					Log( WIDE("For some reason could not add new name to tree!") );
					DeleteFromSet( NAME, l.NameSet, newname );
					return FALSE;
				}
			}
			{
				//PTREEDEF root = GetClassRoot( newname );
				newname->parent = class_root;
				newname->tree.Magic = MAGIC_TREE_NUMBER;
				newname->tree.Tree = CreateBinaryTreeExx( 0 // dups okay BT_OPT_NODUPLICATES
																	 , (int(CPROC *)(uintptr_t,uintptr_t))MyStrCmp
																	 , KillName );
#ifdef _DEBUG
				{
					CTEXTSTR name = pathrchr( pFile );
					// chop the trailing filename, removing path of filename.
					if( name )
						name++;
					else
						name = pFile;
					RegisterValue( (CTEXTSTR)&newname->tree, WIDE( "Source File" ), name );
					RegisterIntValue( (CTEXTSTR)&newname->tree, WIDE( "Source Line" ), nLine );
				}
#endif
			}
		}
		else
		{
			lprintf( WIDE("I'm relasing this name!?") );
			DeleteFromSet( NAME, l.NameSet, newname );
		}
		return 1;
	}
	return FALSE;
}
#ifdef __cplusplus
PROCREG_PROC( LOGICAL, RegisterFunctionExx )( CTEXTSTR root
													 , CTEXTSTR name_class
													 , CTEXTSTR public_name
													 , CTEXTSTR returntype
													 , PROCEDURE proc
													 , CTEXTSTR args
													 , CTEXTSTR library
													 , CTEXTSTR real_name
													  DBG_PASS
														  )
{
	return RegisterFunctionExx( (PCLASSROOT)root, (PCLASSROOT)name_class, public_name, returntype
	                          , proc, args, library, real_name DBG_RELAY );
}
#endif

//---------------------------------------------------------------------------

#if 0
int ReleaseRegisteredFunctionEx( PCLASSROOT root, CTEXTSTR name_class
							 , CTEXTSTR public_name
							 )
{
	PTREEDEF class_root = GetClassTree( root, (PCLASSROOT)name_class );
	TEXTCHAR buf[256];
	PNAME node = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, public_name ) );
	if( node )
	{
		if( node->flags.bProc )
		{
			UnloadFunction( &node->data.proc.proc );
			//node->data.proc.proc = NULL;
			node->flags.bProc = 0;
			return 1;
		}
	}
	return 0;
}
#endif

//---------------------------------------------------------------------------

PROCREG_PROC( int, RegisterProcedureExx )( PCLASSROOT root
														, CTEXTSTR name_class
														, CTEXTSTR public_name
														, CTEXTSTR returntype
														, CTEXTSTR library
														, CTEXTSTR name
														, CTEXTSTR args
														 DBG_PASS
														)
{
	//PROCEDURE proc = (PROCEDURE)LoadFunction( library, name );
	//if( proc )
	{
		return RegisterFunctionExx( root, (PCLASSROOT)name_class
		                          , public_name
		                          , returntype
		                          , NULL
		                          , args
		                          , library
		                          , name
		                          DBG_RELAY );
	}
   //return 0;
}

#undef RegisterProcedureEx
PROCREG_PROC( int, RegisterProcedureEx )( CTEXTSTR name_class
                                        , CTEXTSTR public_name
                                        , CTEXTSTR returntype
                                        , CTEXTSTR library
                                        , CTEXTSTR name
                                        , CTEXTSTR args
                                         DBG_PASS
                                        )
{
   return RegisterProcedureExx( NULL, name_class, public_name, returntype, library, name, args DBG_RELAY );
}

#ifndef __NO_INTERFACE_SUPPORT__
// used in dekware.
PROCREG_PROC( PROCEDURE, ReadRegisteredProcedureEx )( PCLASSROOT root
                                                    , CTEXTSTR returntype
                                                    , CTEXTSTR parms
                                                    )
{
	PTREEDEF class_root = GetClassTree( (PCTREEDEF)root, NULL );
	PNAME oldname = (PNAME)GetCurrentNodeEx( class_root->Tree, &class_root->cursor );
	if( oldname )
	{
		PROCEDURE proc = oldname->data.proc.proc;
		if( !proc && ( oldname->data.proc.library && oldname->data.proc.procname ) )
		{
			proc = (PROCEDURE)LoadFunction( oldname->data.proc.library
													, oldname->data.proc.procname );
			//lprintf( WIDE("Found a procedure %s=%p  (%p)"), name, oldname, proc );
			// should compare whether the types match...
			if( !proc )
			{
				Log( WIDE("Failed to load function when requested from tree...") );
			}
			oldname->data.proc.proc = proc;
		}
		return oldname->data.proc.proc;
	}
	return NULL;
}
#endif
//---------------------------------------------------------------------------
// can use the return type and args to validate the correct
// type of routine is called...
// name is not the function name, but rather the public/common name...
// this name may optionally include a # remark detailing more information
// about the name... the comparison of this name is done up to the #
// and data after a # is checked only if both values have a sub-comment.
// library name is not checked.

// this routine may find more than 1 routine which matches the given
// criteria.  return type and args may be NULL indicating a care-less
// approach.

void DumpRegisteredNamesWork( PTREEDEF tree, int level );

#undef GetRegisteredProcedureExx
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( PCLASSROOT root, PCLASSROOT name_class, CTEXTSTR returntype, CTEXTSTR name, CTEXTSTR args )
#define GetRegisteredProcedureExx GetRegisteredProcedureExxx
{
	PTREEDEF class_root = GetClassTreeEx( (PCTREEDEF)root, (PCTREEDEF)name_class, NULL, FALSE );
	if( class_root )
	{
		PNAME oldname;
		//TEXTCHAR buf[256];
		//lprintf( WIDE("Found class %s=%p for %s"), name_class, class_root, name );
		//DumpRegisteredNamesWork( class_root, 5 );
		oldname = (PNAME)LocateInBinaryTree( class_root->Tree, (uintptr_t)name, NULL );
		//oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, name ) );
		if( oldname )
		{
#if 0
			PROCEDURE proc = oldname->data.proc.proc;
			if( !proc && ( oldname->data.proc.library && oldname->data.proc.procname ) )
			{
				proc = (PROCEDURE)LoadFunction( oldname->data.proc.library
														, oldname->data.proc.procname );
				//lprintf( WIDE("Found a procedure %s=%p  (%p)"), name, oldname, proc );
				// should compare whether the types match...
				if( !proc )
				{
					Log( WIDE("Failed to load function when requested from tree...") );
				}
				oldname->data.proc.proc = proc;
			}
#endif
			return oldname->data.proc.proc;
		}
		//else
      //   lprintf( WIDE("Failed to find %s in the tree"), buf );
	}
	//lprintf( WIDE("Failed to find the class root...") );
	return NULL;
}

#ifdef __cplusplus
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( CTEXTSTR root, CTEXTSTR name_class, CTEXTSTR returntype, CTEXTSTR name, CTEXTSTR args )
{
   return GetRegisteredProcedureExxx( (PCLASSROOT)root, (PCLASSROOT)name_class, returntype, name, args );
}
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR returntype, CTEXTSTR name, CTEXTSTR args )
{
   return GetRegisteredProcedureExxx( (PCLASSROOT)root, (PCLASSROOT)name_class, returntype, name, args );
}
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureExxx )( CTEXTSTR root, PCLASSROOT name_class, CTEXTSTR returntype, CTEXTSTR name, CTEXTSTR args )
{
   return GetRegisteredProcedureExxx( (PCLASSROOT)root, name_class, returntype, name, args );
}
#endif
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureEx )( PCLASSROOT name_class, CTEXTSTR returntype, CTEXTSTR name, CTEXTSTR args )
{
	Init();
   return GetRegisteredProcedureExx( (PCLASSROOT)l.Names, name_class, returntype, name, args );
}
#ifdef __cplusplus
PROCREG_PROC( PROCEDURE, GetRegisteredProcedureEx )( CTEXTSTR name_class, CTEXTSTR returntype, CTEXTSTR name, CTEXTSTR args )
{
	Init();
   return GetRegisteredProcedureExx( l.Names, name_class, returntype, name, args );
}
#endif

//---------------------------------------------------------------------------

void DumpRegisteredNamesWork( PTREEDEF tree, int level )
{
	PNAME name;
	PVARTEXT pvt;
	PTEXT pText;
   POINTER data;
	int bLogging = 0;
#if 0
	if( level == 0 )
	{
		Init();
		lprintf( "Names %p  %p", l.Names, l.NameSpace );
	}
#endif
   if( l.flags.bDisableMemoryLogging )
		bLogging = SetAllocateLogging( FALSE );
   // at least save the create/destroy uselessness...
	if( !tree->Tree )
	{
		if( l.flags.bDisableMemoryLogging )
			SetAllocateLogging( bLogging );
		return;
	}
	pvt = VarTextCreateExx( 512, 1024 );
#if 0
	DumpTree( tree->Tree, NULL );
	lprintf( "Tree is %p %p", tree, tree->Tree );
#endif
	for( name = (PNAME)GetLeastNodeEx( tree->Tree, &data );
		  name;
		  name = (PNAME)GetGreaterNodeEx( tree->Tree, &data ) )
	{
		int n;
		for( n = 0; n < level; n++ )
			vtprintf( pvt, WIDE("   ") );
		vtprintf( pvt, WIDE("%s"), name->name );
		if( name->flags.bValue )
		{
			vtprintf( pvt, WIDE(" = ") );
			if( name->flags.bIntVal )
				vtprintf( pvt, WIDE("[%ld]"), name->data.name.iValue );
			if( name->flags.bStringVal )
				vtprintf( pvt, WIDE("\"%s\""), name->data.name.sValue );
			if( name->flags.bProc )
            vtprintf( pvt, WIDE("*%p"), name->data.proc.proc );
		}
		else if( name->flags.bProc )
		{
			CTEXTSTR p = name->data.proc.name;
			if( p )
			{
				size_t len = p[-1] - 2;
				vtprintf( pvt, WIDE(" = ") );
				while( len )
				{
					size_t tmp;
					vtprintf( pvt, WIDE("%s "), p );
					tmp = StrLen( p ) + 1;
					len-= tmp;
					p += tmp;
				}
				vtprintf( pvt, WIDE("*%p"), name->data.proc.proc );

			}
		}
		pText = VarTextGet( pvt );
		xlprintf(LOG_INFO)( WIDE("%s"), GetText( pText ) );
		LineRelease( pText );
		DumpRegisteredNamesWork( &name->tree, level + 1 );
	}
	VarTextDestroy( &pvt );
	if( l.flags.bDisableMemoryLogging )
		SetAllocateLogging( bLogging );
}

//---------------------------------------------------------------------------

struct browse_index
{
	PTREEDEF current_limbs;
	PTREEDEF current_branch;
};

PROCREG_PROC( int, NameHasBranches )( PCLASSROOT *data )
{
	PTREEDEF class_root;
	PNAME name;
	class_root = (PTREEDEF)*data;
	name = (PNAME)GetCurrentNodeEx( class_root->Tree, &class_root->cursor );
	return name->flags.bTree; // may also have a value, but it was created as a path node in the tree
}


int NewNameIsAlias( PCLASSROOT *data )
{
	struct browse_index *class_root = (struct browse_index*)(*data);
	PNAME name;
	name = (PNAME)GetCurrentNodeEx( class_root->current_branch->Tree, &class_root->current_branch->cursor );
	return name->flags.bAlias; // may also have a value, but it was created as a path node in the tree
}

int NameIsAlias( PCLASSROOT *data )
{
	PTREEDEF class_root;
	PNAME name;
	class_root = (PTREEDEF)*data;
	name = (PNAME)GetCurrentNodeEx( class_root->Tree, &class_root->cursor );
	return name->flags.bAlias; // may also have a value, but it was created as a path node in the tree
}

PROCREG_PROC( PCLASSROOT, GetCurrentRegisteredTree )( PCLASSROOT *data )
{
	PTREEDEF class_root;
	PNAME name;
	class_root = (PTREEDEF)*data;
	name = (PNAME)GetCurrentNodeEx( class_root->Tree, &class_root->cursor );
	if( name )
		return (PCLASSROOT)&name->tree;
	return NULL;
}



//---------------------------------------------------------------------------


PROCREG_PROC( CTEXTSTR, GetFirstRegisteredNameEx )( PCLASSROOT root, CTEXTSTR classname, PCLASSROOT *data )
{
	PTREEDEF class_root;
	PNAME name;
	*data =
		(PCLASSROOT)(class_root = (PTREEDEF)GetClassTree( (PCTREEDEF)root, (PCTREEDEF)classname ));
	if( class_root )
	{
		name = (PNAME)GetLeastNodeEx( class_root->Tree, &class_root->cursor );
		if( name )
		{
			//lprintf( WIDE("Resulting first name: %s"), name->name );
			return name->name;
		}
	}
	return NULL;
}

PROCREG_PROC( CTEXTSTR, GetFirstRegisteredName )( CTEXTSTR classname, PCLASSROOT *data )
{
	return GetFirstRegisteredNameEx( NULL, classname, data );
}
#ifdef __cplusplus
PROCREG_PROC( CTEXTSTR, GetFirstRegisteredName )( PCLASSROOT classname, PCLASSROOT *data )
{
	return GetFirstRegisteredNameEx( NULL, (CTEXTSTR)classname, data );
}
#endif

//---------------------------------------------------------------------------

PROCREG_PROC( CTEXTSTR, GetNextRegisteredName )( PCLASSROOT *data )
{
	PTREEDEF class_root;
	PNAME name;
	class_root = (PTREEDEF)*data;
	if( class_root )
	{
		name = (PNAME)GetGreaterNodeEx( class_root->Tree, &class_root->cursor );
		if( name )
		{
			//lprintf( WIDE("Resulting next name: %s"), name->name );
			return name->name;
		}
	}
	return NULL;

}

//---------------------------------------------------------------------------

PROCREG_PROC( void, DumpRegisteredNames )( void )
{
	if( l.Names )
		DumpRegisteredNamesWork( l.Names, 0 );
}

//---------------------------------------------------------------------------

PROCREG_PROC( void, DumpRegisteredNamesFrom )( PCLASSROOT root )
{
	DumpRegisteredNamesWork( GetClassTreeEx( l.Names, (PCTREEDEF)root, NULL, TRUE ), 0 );
}

//---------------------------------------------------------------------------

PROCREG_PROC( void, InvokeProcedure )( void )
{
}

//---------------------------------------------------------------------------

PROCREG_PROC( int, RegisterValueExx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name, int bIntVal, CTEXTSTR value )
{
	PTREEDEF class_root = GetClassTree( (PCTREEDEF)root, (PCTREEDEF)name_class );
	if( class_root )
	{
		TEXTCHAR buf[256];
		PNAME oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, name ) );
		//lprintf( "... existed? %p", oldname );

		if( oldname )
		{
			oldname->flags.bValue = 1; // it is now a value, okay?
			if( bIntVal )
			{
				oldname->flags.bIntVal = 1;
				oldname->data.name.iValue = (uintptr_t)value;
			}
			else
			{
				oldname->flags.bStringVal = 1;
				oldname->data.name.sValue = SaveName( value );
			}
		}
		else
		{
			PNAME newname = GetFromSet( NAME, &l.NameSet ); //Allocate( sizeof( NAME ) );
			//MemSet( newname, 0, sizeof( NAME ) );
			if( name )
				newname->name = SaveName( name );
			newname->flags.bValue = 1;
			newname->parent = class_root;
			if( bIntVal )
			{
				newname->flags.bIntVal = 1;
				newname->data.name.iValue = (uintptr_t)value;
			}
			else
			{
				newname->flags.bStringVal = 1;
				newname->data.name.sValue = SaveName( value ); //StrDup( value );
			}
			//lprintf( "... adding %s (%s)", name, newname->name );
			if( !AddBinaryNode( class_root->Tree, newname, (uintptr_t)newname->name ) )
			{
				lprintf( WIDE("Failed to add name to tree...%s"), name );
			}
		}
		return TRUE;
	}
	return FALSE;
}

PROCREG_PROC( int, RegisterValueEx )( CTEXTSTR name_class, CTEXTSTR name, int bIntVal, CTEXTSTR value )
{
	Init();
	return RegisterValueExx( (PCLASSROOT)l.Names, name_class, name, bIntVal, value );
}
//---------------------------------------------------------------------------

PROCREG_PROC( int, RegisterValue )( CTEXTSTR name_class, CTEXTSTR name, CTEXTSTR value )
{
	return RegisterValueEx( name_class, name, FALSE, value );
}
//---------------------------------------------------------------------------
PROCREG_PROC( int, RegisterIntValueEx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name, uintptr_t value )
{
	return RegisterValueExx( root, name_class, name, TRUE, (CTEXTSTR)value );
}

PROCREG_PROC( int, RegisterIntValue )( CTEXTSTR name_class, CTEXTSTR name, uintptr_t value )
{
	return RegisterValueEx( name_class, name, TRUE, (CTEXTSTR)value );
}

//---------------------------------------------------------------------------

int GetRegisteredStaticValue( PCLASSROOT root, CTEXTSTR name_class
									 , CTEXTSTR name
									 , CTEXTSTR *result
									 , int bIntVal )
{
	PTREEDEF class_root = GetClassTree( (PCTREEDEF)root, (PCTREEDEF)name_class );
	TEXTCHAR buf[256];
	PNAME oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, name ));
	if( oldname )
	{
		if( bIntVal )
		{
			*((int*)result) = (int)oldname->data.name.iValue;
			return TRUE;
		}
		else if( oldname->flags.bStringVal )
		{
			(*result) = oldname->data.name.sValue;
			return TRUE;
		}
	}
	return FALSE;
}

//---------------------------------------------------------------------------

PROCREG_PROC( CTEXTSTR, GetRegisteredValueExx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name, int bIntVal )
{
	PTREEDEF class_root;
	TEXTCHAR buf[256];
	PNAME oldname;
	class_root = GetClassTree( (PCTREEDEF)root, (PCTREEDEF)name_class );
	oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, name ));
	if( oldname )
	{
		if( bIntVal )
			return (CTEXTSTR)oldname->data.name.iValue;
		else if( oldname->flags.bStringVal )
			return oldname->data.name.sValue;
	}
	return NULL;
}
#ifdef __cplusplus
PROCREG_PROC( CTEXTSTR, GetRegisteredValueExx )( CTEXTSTR root, CTEXTSTR name_class, CTEXTSTR name, int bIntVal )
{
	return GetRegisteredValueExx( (PCLASSROOT)root, name_class, name, bIntVal );
}
#endif

PROCREG_PROC( CTEXTSTR, GetRegisteredValueEx )( CTEXTSTR name_class, CTEXTSTR name, int bIntVal )
{
	Init();
	return GetRegisteredValueExx( (PCLASSROOT)l.Names, name_class, name, bIntVal );
}
//---------------------------------------------------------------------------

PROCREG_PROC( CTEXTSTR, GetRegisteredValue )( CTEXTSTR name_class, CTEXTSTR name )
{
	return GetRegisteredValueEx( name_class, name, FALSE );
}

//---------------------------------------------------------------------------

PROCREG_PROC( int, GetRegisteredIntValue )( CTEXTSTR name_class, CTEXTSTR name )
{
/*
 * this has a warning - typecast to value of differnet size.
 * this is OK.  the value originates as an 'int' and is typecast to a
 * CTEXTSTR which this then down converts back to 'int'
 */
	return (int)(uintptr_t)GetRegisteredValueEx( (CTEXTSTR)name_class, name, TRUE );
}

PROCREG_PROC( int, GetRegisteredIntValueEx )( PCLASSROOT root, CTEXTSTR name_class, CTEXTSTR name )
{
/*
 * this has a warning - typecast to value of differnet size.
 * this is OK.  the value originates as an 'int' and is typecast to a
 * CTEXTSTR which this then down converts back to 'int'
 */
	return (int)(uintptr_t)GetRegisteredValueExx( root, name_class, name, TRUE );
}

#ifdef __cplusplus
PROCREG_PROC( int, GetRegisteredIntValue )( PCLASSROOT name_class, CTEXTSTR name )
{
/*
 * this has a warning - typecast to value of differnet size.
 * this is OK.  the value originates as an 'int' and is typecast to a
 * CTEXTSTR which this then down converts back to 'int'
 */
	return (int)(uintptr_t)GetRegisteredValueEx( (CTEXTSTR)name_class, name, TRUE );
}
#endif
//---------------------------------------------------------------------------

PROCREG_PROC( PCLASSROOT, RegisterClassAliasEx )( PCLASSROOT root, CTEXTSTR original, CTEXTSTR alias )
{
	PTREEDEF class_root = GetClassTreeEx( (PCTREEDEF)root, (PCTREEDEF)original, NULL, TRUE );
	return (PCLASSROOT)GetClassTreeEx( (PCTREEDEF)root, (PCTREEDEF)alias, class_root, TRUE );
}

//---------------------------------------------------------------------------

PROCREG_PROC( PCLASSROOT, RegisterClassAlias )( CTEXTSTR original, CTEXTSTR alias )
{
	Init();
	return (PCLASSROOT)RegisterClassAliasEx( (PCLASSROOT)l.Names, original, alias );
}

//---------------------------------------------------------------------------

PROCREG_PROC( uintptr_t, RegisterDataTypeEx )( PCLASSROOT root
												 , CTEXTSTR classname
												 , CTEXTSTR name
												 , uintptr_t size
												 , void (CPROC *Open)(POINTER,uintptr_t)
												 , void (CPROC *Close)(POINTER,uintptr_t) )
{
	PTREEDEF class_root = GetClassTreeEx( (PCTREEDEF)root, (PCTREEDEF)classname, NULL, TRUE );
	if( class_root )
	{
		PNAME pName = GetFromSet( NAME, &l.NameSet ); //(PNAME)Allocate( sizeof( NAME ) );
		//MemSet( pName, 0, sizeof( NAME ) );
		pName->flags.bData = 1;
		pName->name = SaveName( name );
		pName->data.data.Open = Open;
		pName->data.data.Close = Close;
		pName->data.data.size = size;
		pName->data.data.instances.Magic = MAGIC_TREE_NUMBER;
		pName->data.data.instances.Tree = CreateBinaryTreeExx( 0 // dups okay BT_OPT_NODUPLICATES
														, (int(CPROC *)(uintptr_t,uintptr_t))MyStrCmp
														, KillName );
		pName->parent = class_root;
		if( !AddNode( class_root, pName, (uintptr_t)pName->name ) )
		{
			DeleteFromSet( NAME, l.NameSet, pName );
			return 0; // NULL
		}
		return (uintptr_t)pName;
	}
	return 0; // NULL
}

PROCREG_PROC( uintptr_t, RegisterDataType )( CTEXTSTR classname
												 , CTEXTSTR name
												 , uintptr_t size
												 , void (CPROC *Open)(POINTER,uintptr_t)
												 , void (CPROC *Close)(POINTER,uintptr_t) )
{
	Init();
	return RegisterDataTypeEx( (PCLASSROOT)l.Names, classname, name, size, Open, Close );
}

//---------------------------------------------------------------------------

PROCREG_PROC( uintptr_t, MakeRegisteredDataTypeEx)( PCLASSROOT root
																 , CTEXTSTR classname
																 , CTEXTSTR name
																 , CTEXTSTR instancename
																 , POINTER data
																 , uintptr_t datasize
																 )
{
	PTREEDEF class_root = GetClassTree( (PCTREEDEF)root, (PCTREEDEF)classname );
	if( class_root )
	{
		TEXTCHAR buf[256];
		PNAME pName = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, name ));
		if( !pName )
			pName = (PNAME)RegisterDataTypeEx( root, classname, name, datasize, NULL, NULL );

		if( pName && pName->flags.bData )
		{
			PDATADEF pDataDef = &pName->data.data;
			if( pDataDef )
			{
				if( !instancename )
				{
					TEXTCHAR buf[256];
					tnprintf( buf, sizeof(buf), WIDE("%s_%d"), name, (int)pDataDef->unique++ );
					instancename = SaveName( buf );
				}
				else
					instancename = SaveName( instancename );
				{
					// look up prior instance...
					if( !FindInBinaryTree( pDataDef->instances.Tree, (uintptr_t)instancename ) )
					{
						AddBinaryNode( pDataDef->instances.Tree
										 , data
										 , (uintptr_t)instancename );
					}
					else
					{
						lprintf( WIDE("Suck. We just created one externally, and want to use that data, but it already exists.") );
						DumpRegisteredNames();
						DebugBreak();
						// increment instances referenced so that close does not
						// destroy - fortunatly this is persistant data, and therefore
						// doesn't get destroyed yet.
					}
					return (uintptr_t)data;
				}
			}
		}
		else
		{
			lprintf( WIDE("No such struct defined: %s"), name );
		}
	}
	return 0;
}

//---------------------------------------------------------------------------

PROCREG_PROC( uintptr_t, CreateRegisteredDataTypeEx)( PCLASSROOT root
																	, CTEXTSTR classname
																	, CTEXTSTR name
																	, CTEXTSTR instancename )
{
	PTREEDEF class_root = GetClassTree( (PCTREEDEF)root, (PCTREEDEF)classname );
	if( class_root )
	{
		TEXTCHAR buf[256];
		PNAME pName = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)DressName( buf, name ));
		if( pName && pName->flags.bData )
		{
			PDATADEF pDataDef = &pName->data.data;
			if( pDataDef )
			{
				if( !instancename )
				{
					TEXTCHAR buf[256];
					tnprintf( buf, sizeof(buf), WIDE("%s_%d"), name, (int)pDataDef->unique++ );
					instancename = SaveName( buf );
				}
				else
					instancename = SaveName( instancename );
				{
					POINTER p;
					// look up prior instance...
					if( !( p = (POINTER)FindInBinaryTree( pDataDef->instances.Tree, (uintptr_t)instancename ) ) )
					{
#ifdef DEBUG_GLOBAL_REGISTRATION
						lprintf( WIDE( "Allocating new struct data :%" )_32f, pDataDef->size );
#endif
						p = Allocate( pDataDef->size + sizeof( PLIST ) );
						((PLIST*)p)[0] = NULL;
						p = (POINTER)( ((uintptr_t)p) + sizeof( PLIST ) );
						MemSet( p, 0, pDataDef->size );
						if( pDataDef->Open )
							pDataDef->Open( p, pDataDef->size );
						AddBinaryNode( pDataDef->instances.Tree
										 , p
										 , (uintptr_t)instancename );
					}
					else
					{
						// registered one, returned, needs to be offset for hold purposes.
						POINTER tmp_p = (POINTER)( (uintptr_t)p - sizeof( PLIST ) );
						Hold( tmp_p );
#ifdef DEBUG_GLOBAL_REGISTRATION
						lprintf( WIDE("Resulting with previuosly created instance.") );
						// increment instances referenced so that close does not
						// destroy - fortunatly this is persistant data, and therefore
						// doesn't get destroyed yet.
#endif
					}
					return (uintptr_t)p;
				}
			}
		}
#ifdef DEBUG_GLOBAL_REGISTRATION
		else
		{
			lprintf( WIDE("No such struct defined:[%s]%s"), classname, name );
		}
#endif
	}
	return 0;
}

//---------------------------------------------------------------------------

PROCREG_PROC( uintptr_t, CreateRegisteredDataType)( CTEXTSTR classname
																 , CTEXTSTR name
																 , CTEXTSTR instancename )
{
	Init();
	return CreateRegisteredDataTypeEx( (PCLASSROOT)l.Names, classname, name, instancename );
}

//---------------------------------------------------------------------------

typedef POINTER (CPROC *LOADPROC)( void );
typedef void	 (CPROC *UNLOADPROC)( POINTER );

//-----------------------------------------------------------------------

LOGICAL RegisterInterfaceEx( CTEXTSTR servicename, POINTER(CPROC*load)(void), void(CPROC*unload)(POINTER) DBG_PASS )
{
	//PARAM( args, TEXTCHAR*, servicename );
	//PARAM( args, TEXTCHAR*, library );
	//PARAM( args, TEXTCHAR*, load_proc_name );
	//PARAM( args, TEXTCHAR*, unload_proc_name );
	PCLASSROOT pcr = GetClassRoot( WIDE("system/interfaces") );
	if( GetRegisteredProcedureExx( pcr, (PCLASSROOT)servicename, WIDE("POINTER"), WIDE("load"), WIDE("void") ) )
	{
		lprintf( WIDE("Service: %s has multiple definitions, using first registered.")
				 , servicename );
		return FALSE;
	}
	//lprintf( WIDE("Registering library l:%p ul:%p"), load, unload );
	{
		RegisterFunctionExx( pcr
								  , (PCLASSROOT)servicename
								  , WIDE("load")
								  , WIDE("POINTER")
								  , (PROCEDURE)load
								  , WIDE("(void)"), NULL, NULL DBG_RELAY );
		RegisterFunctionExx( pcr
								  , (PCLASSROOT)servicename
								  , WIDE("unload")
								  , WIDE("void")
								  , (PROCEDURE)unload
								  , WIDE("(POINTER)"), NULL, NULL DBG_RELAY );
	}
	return TRUE;
}

//-----------------------------------------------------------------------
#ifndef __NO_INTERFACE_SUPPORT__

static uintptr_t CPROC HandleLibrary( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR*, servicename );
	PARAM( args, TEXTCHAR*, library );
	PARAM( args, TEXTCHAR*, load_proc_name );
	PARAM( args, TEXTCHAR*, unload_proc_name );
	PCLASSROOT pcr = GetClassRoot( WIDE("system/interfaces") );
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
	if( GetRegisteredProcedureExx( pcr, (PCLASSROOT)servicename, WIDE("POINTER"), WIDE("load"), WIDE("void") ) )
	{
		lprintf( WIDE("Service: %s has multiple definitions, will use last first.")
				 , servicename );
		return psv;
	}
	//lprintf( WIDE("Registering library %s function %s"), library, load_proc_name );
	{
		RegisterProcedureExx( pcr
		                    , servicename
		                    , WIDE("load")
		                    , WIDE("POINTER")
		                    , library
		                    , load_proc_name
		                    , WIDE("void") DBG_SRC );
		RegisterProcedureExx( pcr
		                    , servicename
		                    , WIDE("unload")
		                    , WIDE("void")
		                    , library
		                    , unload_proc_name, WIDE("POINTER") DBG_SRC );
	}
	return psv;
}

//-----------------------------------------------------------------------
static uintptr_t CPROC HandleAlias( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR*, servicename );
	PARAM( args, TEXTCHAR*, originalname );
	TEXTCHAR fullservicename[256];
	TEXTCHAR fulloriginalname[256];
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "alias %s=%s" ), servicename, originalname );
	tnprintf( fullservicename, sizeof( fullservicename), WIDE("system/interfaces/%s"), servicename );
	tnprintf( fulloriginalname, sizeof( fulloriginalname), WIDE("system/interfaces/%s"), originalname );
	RegisterClassAlias( fulloriginalname, fullservicename );
	return psv;
}

//-----------------------------------------------------------------------

static uintptr_t CPROC HandleModule( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR*, module );
	LOGICAL tempPath = FALSE;
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
	if( module[0] == '~' || module[0] == '@' || module[0] == '^' || module[0] == '*' ) {
		module = ExpandPath( module );
		tempPath = TRUE;
	}
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "load module %s" ), module );
	if( !l.flags.bHeldDeadstart )
	{
		l.flags.bHeldDeadstart = 1;
		SuspendDeadstart();
	}

	LoadFunction( module, NULL );
	if( tempPath )
		Deallocate( TEXTCHAR*, module );
	return psv;
}	

//-----------------------------------------------------------------------

static uintptr_t CPROC HandlePrivateModule( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR*, module );
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "load private module %s" ), module );
	if( !l.flags.bHeldDeadstart )
	{
		l.flags.bHeldDeadstart = 1;
		SuspendDeadstart();
	}

	LoadPrivateFunction( module, NULL );
	return psv;
}	

//-----------------------------------------------------------------------

static TEXTSTR SubstituteNameVars( CTEXTSTR name )
{
	PVARTEXT pvt = VarTextCreate();
	const TEXTCHAR *start = name;
	const TEXTCHAR *this_var = name;
	const TEXTCHAR *end;

	while( ( this_var = StrChr( start, '%' ) ) )
	{
		// allow specifying %% for a single %.
		// emit the stuff from start to the variable
		if( start < this_var )
			vtprintf( pvt, WIDE("%*.*s"), this_var-start, this_var-start, start );

		if( this_var[1] == '%' )
		{
			VarTextAddCharacter( pvt, '%' );
			start = this_var + 2;
			continue;
		}
		end = StrChr( this_var + 1, '%' );
		if( end )
		{
			TEXTCHAR *tmpvar = NewArray( TEXTCHAR, end - this_var );
			CTEXTSTR envvar;
			tnprintf( tmpvar, end-this_var, WIDE("%*.*s"), (int)(end-this_var-1), (int)(end-this_var-1), this_var + 1 );
			envvar = OSALOT_GetEnvironmentVariable( tmpvar );
			if( envvar )
				vtprintf( pvt, WIDE("%s"), OSALOT_GetEnvironmentVariable( tmpvar ) );
			else
				lprintf( WIDE("failed to find environment variable '%s'"), tmpvar );
			Release( tmpvar );
			start = end + 1;
		}
		else
			lprintf( WIDE("Bad framing on environment variable %%var%% syntax got [%s]"), start );
	}
	if( start[0] )
		vtprintf( pvt, WIDE("%s"), start );
	{
		TEXTSTR result = StrDup( GetText( VarTextPeek( pvt ) ) );
		VarTextDestroy( &pvt );
		return result;
	}
}
//-----------------------------------------------------------------------

static uintptr_t CPROC HandleModulePath( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTSTR, filepath );
	filepath = ExpandPath( filepath );
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
# ifdef __LINUX__
	OSALOT_AppendEnvironmentVariable( WIDE("LD_LIBRARY_PATH"), filepath );
# else
#  ifndef UNDER_CE
	OSALOT_AppendEnvironmentVariable( WIDE("PATH"), filepath );
#  endif
# endif
	Release( filepath );
	return psv;
}


PROCREG_PROC( void, SetInterfaceConfigFile )( TEXTCHAR *filename )
{
	if( l.config_filename )
		Release( l.config_filename );
	l.config_filename = StrDup( filename );
}


static uintptr_t CPROC SetDefaultDirectory( uintptr_t psv, arg_list args )
{
#ifndef __NO_OPTIONS__
	PARAM( args, CTEXTSTR, path );
	SetCurrentPath( path );
#endif
	return psv;
}

static uintptr_t CPROC SetOptionDefault( uintptr_t psv, arg_list args )
{
#ifndef __NO_OPTIONS__
	PARAM( args, TEXTSTR, key );
	PARAM( args, CTEXTSTR, value );
	TEXTCHAR buf[256];
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
	if( key[0] != '/' && key[0] != '\\' )
	{
		if( l.flags.bTraceInterfaceLoading )
			lprintf( WIDE( "Default Option %s / [%s] = [%s}" ), GetProgramName(), key, value );
		key = SubstituteNameVars( key );
		SACK_GetProfileStringEx( GetProgramName(), key, value, buf, sizeof( buf ), TRUE );
		Release( key );
	}
	else
	{
		TEXTSTR optpath = (TEXTSTR)pathchr( key + 1 );
		TEXTSTR optname = (TEXTSTR)pathrchr( key );
		optname[0] = 0;
		optname++;
		optpath[0] = 0;
		optpath++;

		optname = SubstituteNameVars( optname );
		optpath = SubstituteNameVars( optpath );
		if( l.flags.bTraceInterfaceLoading )
			lprintf( WIDE( "Default Option [%s]/[%s]/[%s] = [%s}" ), key, optpath, optname, value );

		SACK_GetPrivateProfileStringEx( optpath, optname, value, buf, sizeof( buf ), key, TRUE );
		Release( optname );
		Release( optpath );
	}
#endif
	return psv;
}
static uintptr_t CPROC SetOptionSet( uintptr_t psv, arg_list args )
{
#ifndef __NO_OPTIONS__
	PARAM( args, TEXTSTR, key );
	PARAM( args, CTEXTSTR, value );
	if( l.flags.bFindEndif || l.flags.bFindElse )
		return psv;
	if( key[0] != '/' && key[0] != '\\' )
	{
		if( l.flags.bTraceInterfaceLoading )
			lprintf( WIDE( "Set Option %s / [%s] = [%s}" ), GetProgramName(), key, value );
		key = SubstituteNameVars( key );
		SACK_WriteProfileStringEx( GetProgramName(), key, value, key, TRUE );
		Release( key );
	}
	else
	{
		TEXTSTR optpath = (TEXTSTR)pathchr( key + 1 );
		TEXTSTR optname = (TEXTSTR)pathrchr( key );
		optname[0] = 0;
		optname++;
		optpath[0] = 0;
		optpath++;

		optname = SubstituteNameVars( optname );
		optpath = SubstituteNameVars( optpath );
		if( l.flags.bTraceInterfaceLoading )
			lprintf( WIDE( "Set Option [%s]/[%s]/[%s] = [%s}" ), key, optpath, optname, value );

		SACK_WritePrivateProfileStringEx( optpath, optname, value, key, TRUE );
		Release( optname );
		Release( optpath );
	}
#endif
	return psv;
}

static uintptr_t CPROC TestOption( uintptr_t psv, arg_list args )
{
#ifndef __NO_OPTIONS__
	PARAM( args, CTEXTSTR, key );
	PARAM( args, CTEXTSTR, value );
	TEXTCHAR buf[256];
	SACK_GetProfileStringEx( GetProgramName(), key, WIDE( "" ), buf, sizeof( buf ), TRUE );
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( " is [%s] == [%s]  buf = [%s]" ), key, value, buf );
	if( buf[0] == 0 )
	{
		l.flags.bFindEndif++;
		l.flags.bFindElse = 1;
	}
	else if( StrCaseCmp( buf, value ) != 0 )
	{
		l.flags.bFindEndif++;
		l.flags.bFindElse = 1;
	}
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "seek(findendif, findelse) = %d %d" ), l.flags.bFindEndif, l.flags.bFindElse );
#endif
	return psv;
}
static uintptr_t CPROC EndTestOption( uintptr_t psv, arg_list args )
{
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "found endif..." ) );
	if(l.flags.bFindEndif)
	{
		l.flags.bFindEndif--;
		l.flags.bFindElse = 0;
	}
	return psv;
}

static uintptr_t CPROC ElseTestOption( uintptr_t psv, arg_list args )
{
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "found else..." ) );
	if(l.flags.bFindElse)
	{
		l.flags.bFindElse = 0;
		l.flags.bFindEndif = 0;
	}
	else
		l.flags.bFindEndif++;
	return psv;
}
static uintptr_t CPROC SetTrace( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, yesno );
	l.flags.bTraceInterfaceLoading = yesno;
	return psv;
}

static uintptr_t CPROC IncludeAdditional( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	TEXTSTR old_configname = l.config_filename;
	l.config_filename = ExpandPath( path );
	if( l.flags.bTraceInterfaceLoading )
		lprintf( WIDE( "include:%s from %s" ), l.config_filename, old_configname );
	if( !l.flags.bHeldDeadstart )
	{
		l.flags.bHeldDeadstart = 1;
		SuspendDeadstart();
	}
	ReadConfiguration();
	if( l.flags.bHeldDeadstart )
	{
		ResumeDeadstart();
		l.flags.bHeldDeadstart = 0;
	}
	Release( l.config_filename );
	l.config_filename = old_configname;
	return psv;
}

static uintptr_t CPROC SetProducerName( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	sack_set_common_data_producer( name );
	return psv;
}

static uintptr_t CPROC SetApplicationName( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	sack_set_common_data_application( name );
	return psv;
}
#endif
//-----------------------------------------------------------------------
#ifndef __NO_INTERFACE_SUPPORT__
void ReadConfiguration( void )
{
	if( !l.flags.bInterfacesLoaded )
	{
		PCONFIG_HANDLER pch;
		pch = CreateConfigurationHandler();
		AddConfigurationMethod( pch, WIDE( "Producer=%m" ), SetProducerName );
		AddConfigurationMethod( pch, WIDE( "Application=%m" ), SetApplicationName );
		AddConfigurationMethod( pch, WIDE( "enable trace=%b" ), SetTrace );
		AddConfigurationMethod( pch, WIDE( "option default %m=%m" ), SetOptionDefault );
		AddConfigurationMethod( pch, WIDE( "option set %m=%m" ), SetOptionSet );
		AddConfigurationMethod( pch, WIDE( "default option %m=%m" ), SetOptionDefault );
		AddConfigurationMethod( pch, WIDE( "set option %m=%m" ), SetOptionSet );
		AddConfigurationMethod( pch, WIDE( "start directory \"%m\"" ), SetDefaultDirectory );
		AddConfigurationMethod( pch, WIDE( "include \"%m\"" ), IncludeAdditional );
		AddConfigurationMethod( pch, WIDE( "if %m==%m" ), TestOption );
		AddConfigurationMethod( pch, WIDE( "endif" ), EndTestOption );
		AddConfigurationMethod( pch, WIDE( "else" ), ElseTestOption );

		AddConfigurationMethod( pch, WIDE("service=%w library=%w load=%w unload=%w"), HandleLibrary );
		AddConfigurationMethod( pch, WIDE("alias service %w %w"), HandleAlias );
		AddConfigurationMethod( pch, WIDE("module %w"), HandleModule );
		AddConfigurationMethod( pch, WIDE("pmodule %w"), HandlePrivateModule );
		AddConfigurationMethod( pch, WIDE("modulepath %m"), HandleModulePath );

		{
			CTEXTSTR filepath
#ifdef __ANDROID__
				= WIDE(".");
#else
				= GetProgramPath();
#endif
			TEXTSTR loadname;
			size_t len;
			int success = FALSE;
			if( !filepath )
				filepath = WIDE("@");

			if( l.config_filename )
			{
				success = ProcessConfigurationFile( pch, l.config_filename, 0 );
				if( !success )
					lprintf( WIDE("Failed to open custom interface configuration file:%s"), l.config_filename );
				return;
			}
			if( !success )
			{
				CTEXTSTR dot;
				loadname = NewArray( TEXTCHAR, (uint32_t)(len = StrLen( GetProgramName() ) + StrLen( WIDE("interface.conf") ) + 3) );
				tnprintf( loadname, len, WIDE("%s.%s"), GetProgramName(), WIDE("interface.conf") );
				success = ProcessConfigurationFile( pch, loadname, 0 );
				if( !success )
					dot = GetProgramName();
				while( !success )
				{
					dot = StrChr( dot + 1, '.' );
					if( dot )
					{
						tnprintf( loadname, len, WIDE("%s.%s"), dot+1, WIDE("interface.conf") );
						success = ProcessConfigurationFile( pch, loadname, 0 );
					}
					else
						break;
				}
			}
			if( !success )
			{
				success = ProcessConfigurationFile( pch, WIDE( "interface.conf" ), 0 );
			}
			if( !success )
			{
				CTEXTSTR dot;
				loadname = NewArray( TEXTCHAR, (uint32_t)(len = StrLen( filepath ) + StrLen( GetProgramName() ) + StrLen( WIDE("interface.conf") ) + 3) );
				tnprintf( loadname, len, WIDE("%s/%s.%s"), filepath, GetProgramName(), WIDE("interface.conf") );
				success = ProcessConfigurationFile( pch, loadname, 0 );
				if( !success )
					dot = GetProgramName();
				while( !success )
				{
					dot = StrChr( dot + 1, '.' );
					if( dot )
					{
						tnprintf( loadname, len, WIDE("%s/%s.%s"), filepath, dot+1, WIDE("interface.conf") );
						success = ProcessConfigurationFile( pch, loadname, 0 );
					}
					else
						break;
				}
			}
			if( !success )
			{
				tnprintf( loadname, len, WIDE("%s/%s"), filepath, WIDE("interface.conf") );
				success = ProcessConfigurationFile( pch, loadname, 0 );
			}
			if( !success )
			{
				//lprintf( WIDE("Failed to open interface configuration file:%s - assuming it will never exist, and aborting trying this again")
				//		 , l.config_filename?l.config_filename:WIDE("interface.conf") );
			}
			if( loadname )
				Release( loadname );
		}
		DestroyConfigurationHandler( pch );
		//at this point... we should probably NOT
		// dump this information, a vast amount of information may occur.
		// consider impelmenting enumerators and allowing browsing
		//DumpRegisteredNames();
		// if we failed, probably noone will notice, and nooone will
		// get the clue that we need to have an interface.conf
		// for this to preload extra libraries that the program may be
		// requesting.
		l.flags.bInterfacesLoaded = 1;
	}
	//else
	//	lprintf( WIDE( "already loaded." ) );

	if( l.flags.bHeldDeadstart )
	{
		l.flags.bHeldDeadstart = 0;
		ResumeDeadstart();
	}
}
#endif
//-----------------------------------------------------------------------
POINTER GetInterfaceExx( CTEXTSTR pServiceName, LOGICAL ReadConfig DBG_PASS ) {
	return GetInterface_v4( pServiceName, ReadConfig, FALSE DBG_RELAY );
}

POINTER GetInterface_v4( CTEXTSTR pServiceName, LOGICAL ReadConfig, int quietFail DBG_PASS )
{
	TEXTCHAR interface_name[256];
	POINTER (CPROC *load)( void );
	static int reading_configuration;
	// this might be the first clean chance to run deadstarts
	// for ill behaved platforms that have forgotten to do this.
	if( !IsRootDeadstartStarted() )
	{
		InvokeDeadstart();
	}
#ifndef __NO_INTERFACE_SUPPORT__
	if( ReadConfig && !reading_configuration )
	{
		reading_configuration = 1;
		SuspendDeadstart();
		ReadConfiguration();
		ResumeDeadstart();
		reading_configuration = 0;
	}
#endif
	//lprintf( "Load interface [%s]", pServiceName );
	if( pServiceName )
	{
		tnprintf( interface_name, sizeof( interface_name ), WIDE("system/interfaces/%s"), pServiceName );
		load = GetRegisteredProcedure( (PCLASSROOT)interface_name, POINTER, load, (void) );
		//lprintf( WIDE("GetInterface for %s is %p"), pServiceName, load );
		if( load )
		{
			POINTER p = load();
			//lprintf( WIDE("And the laod proc resulted %p"), p );
			return p; //load();
		}
#ifdef _DEBUG
		else if( !quietFail )
		{
			if( l.flags.bInterfacesLoaded )
			{
				if( !GetRegisteredValueExx( (PCLASSROOT)interface_name, NULL, WIDE( "Logged" ), 1 ) )
				{
					_lprintf(DBG_RELAY)( WIDE("Did not find load procedure for:[%s] (dumping names from /system/interface/* so you can see what might be available)"), interface_name );
					DumpRegisteredNamesFrom(GetClassRoot(WIDE( "system/interfaces" )));
					RegisterValueExx( (PCLASSROOT)interface_name, NULL, WIDE( "Logged" ), 1, (CTEXTSTR)1 );
				}
			}
		}
#endif
	}
	return NULL;
}

#undef GetInterfaceEx
POINTER GetInterfaceEx( CTEXTSTR pServiceName, LOGICAL ReadConfig )
{
	return GetInterfaceExx( pServiceName, ReadConfig DBG_SRC );
}


POINTER GetInterfaceDbg( CTEXTSTR pServiceName DBG_PASS )
{
	POINTER result = GetInterfaceExx( pServiceName, FALSE DBG_RELAY );
	if( !result )
	{
		// don't force the issue too much
		if( l.flags.bReadConfiguration )
			result = GetInterfaceExx( pServiceName, TRUE DBG_RELAY );
	}
	return result;
}

#if 0
#undef GetInterface
PUBLIC( POINTER, GetInterface )( CTEXTSTR pServiceName )
{
	return GetInterfaceDbg( pServiceName DBG_SRC );
}
#endif
//-----------------------------------------------------------------------

PROCREG_PROC( void, DropInterface )( CTEXTSTR pServiceName, POINTER interface_drop )
{
	TEXTCHAR interfacename[256];
	void (CPROC *unload)( POINTER );
	tnprintf( interfacename, sizeof(interfacename), WIDE("system/interfaces/%s"), pServiceName );
	unload = GetRegisteredProcedure( (PCLASSROOT)interfacename, void, unload, (POINTER) );
	if( unload )
		unload( interface_drop );
}


//-----------------------------------------------------------------------

PRIORITY_ATEXIT( CloseGlobalRegions, ATEXIT_PRIORITY_SHAREMEM + 1 )
{
	PLIST *global_reference;
	INDEX idx;
	return;
	l.reference_count--;
	if( !l.reference_count )
		LIST_FORALL( l.global_spaces, idx, PLIST*, global_reference )
		{
			INDEX idx2;
			POINTER *ppGlobal;
			SetAllocateLogging( 0 );
			// hold the global reference once more, and then just release
			Hold( global_reference );
			LIST_FORALL( global_reference[0], idx2, POINTER *, ppGlobal )
			{
				// increment count here for number of Releases to do.
				(*ppGlobal) = NULL;
				Release( global_reference );
			}
			DeleteList( global_reference );
			// Release all times; number of holds should match number above...
			// safety check it?
			Release( global_reference );
		}
}

void RegisterAndCreateGlobalWithInit( POINTER *ppGlobal, uintptr_t global_size, CTEXTSTR name, void (CPROC*Open)(POINTER,uintptr_t) )
{
	POINTER *ppGlobalMain;
	POINTER p;

	if( ppGlobal == (POINTER*)&procreg_local_data )
	{
		uintptr_t size = global_size + sizeof( PLIST );
		uint32_t created;
		TEXTCHAR spacename[32];
		if( procreg_local_data != NULL )
		{
			// if local already has something, just return.
			return;
		}
#ifdef DEBUG_GLOBAL_REGISTRATION
		lprintf( WIDE("Opening space...") );
#endif
#ifdef UNICODE
#define _S WIDE("ls")
#else
#define _S WIDE("s")
#endif

#ifdef WIN32
		tnprintf( spacename, sizeof( spacename ), WIDE("%s:%08lX"), name, GetCurrentProcessId() );
#else
		tnprintf( spacename, sizeof( spacename ), WIDE("%")_S WIDE(":%08X"), name, getpid() );
#  ifdef DEBUG_FIRST_UNICODE_OPERATION
		{
			wchar_t buf[32];
			strcpy( (char*)buf, "abcdefghijklmn" );
			swprintf( buf, 32, L"%s", L"some_name" );
			{
				char tmpmsg[256];
				int chars;
				int ofs= 0;
				ofs = snprintf( tmpmsg, 256, "in the beginning(w):" );
				for( chars = 0; chars < 32; chars++ )
					ofs += snprintf( tmpmsg + ofs, 256 - ofs, "%02x ", ((char*)buf)[chars] );

				__android_log_print( ANDROID_LOG_INFO, "org.d3x0r.sack.xxxx", tmpmsg );

				ofs = snprintf( tmpmsg, 256, "in the beginning(w):" );
				for( chars = 0; chars < 32; chars++ )
					ofs += snprintf( tmpmsg + ofs, 256 - ofs, "%c", (buf)[chars] );

				__android_log_print( ANDROID_LOG_INFO, "org.d3x0r.sack.xxxx", tmpmsg );


								ofs = snprintf( tmpmsg, 256, "in the beginning(w):" );
				for( chars = 0; chars < 32; chars++ )
					ofs += snprintf( tmpmsg + ofs, 256 - ofs, "%c", (name)[chars] );

				__android_log_print( ANDROID_LOG_INFO, "org.d3x0r.sack.xxxx", tmpmsg );

				ofs = snprintf( tmpmsg, 256, "in the beginning(w):" );
				for( chars = 0; chars < 32; chars++ )
					ofs += snprintf( tmpmsg + ofs, 256 - ofs, "%c", (spacename)[chars] );

				__android_log_print( ANDROID_LOG_INFO, "org.d3x0r.sack.xxxx", tmpmsg );

			}

		}
#  endif
#endif
		// hmm application only shared space?
		// how do I get that to happen?
		(*ppGlobal) = OpenSpaceExx( spacename, NULL, 0, &size, &created );
		(*ppGlobal) = (POINTER*)( (uintptr_t)(*ppGlobal) + sizeof( PLIST ) );
		// I myself must have a global space, which is kept sepearte from named spaces
		// but then... blah
		if( created )
		{
#ifdef DEBUG_GLOBAL_REGISTRATION
			lprintf( WIDE("(specific procreg global)clearing memory:%s(%p)"), spacename, (*ppGlobal ) );
#endif
			MemSet( (*ppGlobal), 0, global_size );
			{
				// pp global is a double pointer type, I want the pointer before
				PLIST *global_references = (PLIST*)( (uintptr_t)(*ppGlobal) - sizeof( POINTER ) );
				global_references[0] = NULL;
				AddLink( global_references, ppGlobal );
			}
			if( Open )
				Open( (*ppGlobal), global_size );
			p = (POINTER)MakeRegisteredDataTypeEx( NULL, WIDE("system/global data"), name, name, (*ppGlobal), global_size );
		}
		else
		{
#ifdef DEBUG_GLOBAL_REGISTRATION
			lprintf( WIDE("(specific procreg global)using memory untouched:%s(%p)"), spacename, (*ppGlobal ) );
#endif
			{
				// pp global is a double pointer type, I want the pointer before
				PLIST *global_references = (PLIST*)( (uintptr_t)(*ppGlobal) - sizeof( POINTER ) );
				AddLink( global_references, ppGlobal );
			}
		}
		// result is the same as the pointer input...
		return;
	}

	if( ppGlobal && !(*ppGlobal) )
	{
		Init();
		// RTLD_DEFAULT
		ppGlobalMain = &p;
		p = (POINTER)CreateRegisteredDataTypeEx( (PCLASSROOT)l.Names, WIDE("system/global data"), name, name );
		if( !p )
		{
			RegisterDataType( WIDE("system/global data"), name, global_size
								 , Open
								 , NULL );
			p = (POINTER)CreateRegisteredDataTypeEx( (PCLASSROOT)l.Names, WIDE("system/global data"), name, name );
			if( !p )
				ppGlobalMain = NULL;
#ifdef DEBUG_GLOBAL_REGISTRATION
			lprintf( WIDE("Registered and created by registered type. %p"), p );
#endif
			{
				// only need each space once in this list; when it's created.
				POINTER tmp_p = (POINTER)( (uintptr_t)p - sizeof( PLIST ) );
				AddLink( &l.global_spaces, tmp_p );
			}
		}
		else
		{
#ifdef DEBUG_GLOBAL_REGISTRATION
			lprintf( WIDE("Found our shared region by asking politely for it! *********************") );
#endif
		}
		if( !ppGlobalMain )
		{
			lprintf( WIDE("None found in main... no way to mark for a peer...") );
			exit(0);
		}
		if( ppGlobalMain && *ppGlobalMain )
		{
#ifdef DEBUG_GLOBAL_REGISTRATION
			lprintf( WIDE("Resulting with a global space to use... %p"), (*ppGlobalMain) );
#endif
			(*ppGlobal) = (*ppGlobalMain);
			{
				// pp global is a double pointer type, I want the pointer before
				PLIST *global_references = (PLIST*)( (uintptr_t)(*ppGlobal) - sizeof( PLIST ) );
				AddLink( global_references, ppGlobal );
			}
		}
		else
		{
			lprintf( WIDE("Failure to get global_procreg_data block.") );
			exit(0);
		}
	}
	else
	{
		// thing is already apparently initizliaed.. don't do this.
		ppGlobalMain = NULL;
	}
}

void RegisterAndCreateGlobal( POINTER *ppGlobal, uintptr_t global_size, CTEXTSTR name )
{
	RegisterAndCreateGlobalWithInit( ppGlobal, global_size, name, NULL );
}

#ifdef __cplusplus_cli

#include <vcclr.h>

using namespace System;

public ref class ProcReg
{
	static ProcReg()
	{
		InvokeDeadstart();
	}
	
	int Register( System::String^ name_class, String^ proc, STDPROCEDURE Delegate )
	{
		if( name_class )
		{
			PNAME newname = GetFromSet( NAME, &l.NameSet );//Allocate( sizeof( NAME ) );
			TEXTCHAR strippedargs[256];
			pin_ptr<const WCHAR> tmp2 = PtrToStringChars(name_class);
			CTEXTSTR __name_class = DupWideToText( tmp2 );
			pin_ptr<const WCHAR> tmp = PtrToStringChars(proc);
			CTEXTSTR real_name = DupWideToText( tmp );
	
			PTREEDEF class_root = (PTREEDEF)GetClassTree( NULL, (PTREEDEF)__name_class );
			MemSet( newname, 0, sizeof( NAME ) );
			newname->flags.bStdProc = 1;
			// this is kinda messed up...
			newname->name = SaveName( real_name );
			//newname->data.stdproc.library = SaveName( library );
	
	
			newname->data.stdproc.procname = SaveName( real_name );
			//newname->data.proc.ret = SaveName( returntype );
			//newname->data.proc.args = SaveName( StripName( strippedargs, args ) );
			newname->data.proc.name = SaveNameConcatN( StripName( strippedargs, WIDE( "(*)" ) )
																  , WIDE("") //returntype
																  , WIDE("") // library 
																  , real_name
																  , NULL
																  );
			newname->data.stdproc.proc = Delegate;
			if( class_root )
			{
				PNAME oldname;
				oldname = (PNAME)FindInBinaryTree( class_root->Tree, (uintptr_t)newname->name);
				if( oldname )
				{
					if( oldname->data.stdproc.proc == Delegate )
						Log( WIDE("And fortunatly it's the same address... all is well...") );
					else
					{
						xlprintf( 2 )( WIDE("proc %s/%s regisry by %s of %s(%s) conflicts with %s(%s)...")
													  , (CTEXTSTR)__name_class?(CTEXTSTR)__name_class:WIDE("@")
													  , real_name
													  , newname->name
													  , newname->data.proc.name
														//,library
													  , newname->data.proc.procname
													  , oldname->data.proc.name
													  //,library
													  , oldname->data.proc.procname );
						// perhaps it's same in a different library...
						Log( WIDE("All is not well - found same function name in tree with different address. (ignoring second) ") );
						//DebugBreak();
						//DumpRegisteredNames();
					}
					return TRUE;
				}
				else
				{
					newname->parent = class_root;
					if( !AddBinaryNode( class_root->Tree, newname, (uintptr_t)newname->name ) )
					{
						Log( WIDE("For some reason could not add new name to tree!") );
						DeleteFromSet( NAME, l.NameSet, newname );
					}
				}
			}
			else
			{
				lprintf( WIDE("I'm relasing this name!?") );
				DeleteFromSet( NAME, l.NameSet, newname );
			}
			return 1;
		}
	}
};

#endif

PROCREG_NAMESPACE_END

#undef l
