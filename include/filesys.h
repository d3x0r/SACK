/*
 *  Created By Jim Buckeyne
 *
 *  Purpose:
 *    Provides some cross platform/library functionatlity for
 *  filesystem activities.
 *  - File dates, times, stuff like that
 *  - make paths, change paths
 *  - path parsing (like strchr, strrchr, but looking for closest / or \)
 *  - scan a directory for a set of files... using a recursive callback method
 */

#ifndef FILESYSTEM_UTILS_DEFINED
/* Header multiple inclusion protection symbol. */
#define FILESYSTEM_UTILS_DEFINED
#include <stdhdrs.h>

#if _MSC_VER >= 1600 
#include <share.h>
#endif

#if !defined( UNDER_CE ) ||defined( ELTANIN)
#include <fcntl.h>
#if !defined( __LINUX__ )
#include <io.h>
#else
#define LPFILETIME P_64
#define FILETIME _64
#endif
#endif

/* uhmm in legacy usage this was not CPROC, but was unspecified */
#define FILESYS_API CPROC 
// DOM-IGNORE-BEGIN
#ifdef FILESYSTEM_LIBRARY_SOURCE
#  define FILESYS_PROC EXPORT_METHOD
#else
#  define FILESYS_PROC IMPORT_METHOD
#endif
// DOM-IGNORE-END

#ifdef __cplusplus
/* defined the file system partial namespace (under
   SACK_NAMESPACE probably)                         */
#define _FILESYS_NAMESPACE  namespace filesys {
/* Define the ending symbol for file system namespace. */
#define _FILESYS_NAMESPACE_END }
/* Defined the namespace of file montior utilities. File monitor
   provides event notification based on file system changes.     */
#define _FILEMON_NAMESPACE  namespace monitor {
/* Define the end symbol for file monitor namespace. */
#define _FILEMON_NAMESPACE_END }
#else
#define _FILESYS_NAMESPACE 
#define _FILESYS_NAMESPACE_END
#define _FILEMON_NAMESPACE 
#define _FILEMON_NAMESPACE_END
#endif
/* define the file system namespace end. */
#define FILESYS_NAMESPACE_END _FILESYS_NAMESPACE_END SACK_NAMESPACE_END 
/* define the file system namespace. */
#define FILESYS_NAMESPACE SACK_NAMESPACE _FILESYS_NAMESPACE 
/* Define end file monitor namespace. */
#define FILEMON_NAMESPACE_END _FILEMON_NAMESPACE_END _FILESYS_NAMESPACE_END SACK_NAMESPACE_END 
/* Defines the file montior namespace when compiling C++. */
#define FILEMON_NAMESPACE SACK_NAMESPACE _FILESYS_NAMESPACE _FILEMON_NAMESPACE

SACK_NAMESPACE

/* \File system abstractions. A few things like get current path
   may or may not exist on a function.
   
   
   
   Primarily this defines functions 'pathchr' and 'pathrchr'
   which resemble 'strchr' and 'strrchr' but search a string for
   a path character. A path character is either a / or a \\.
   
   
   
   Also in this area is file monitoring functions which support
   methods on windows and linux to get event notifications when
   directories and, by filtering, files that have changed.
   
   
                                                                 */
_FILESYS_NAMESPACE

	enum ScanFileFlags {

SFF_SUBCURSE    = 1, // go into subdirectories
SFF_DIRECTORIES = 2, // return directory names also
SFF_NAMEONLY    = 4, // don't concatenate base with filename to result.
SFF_IGNORECASE  = 8, // when matching filename - do not match case.
SFF_SUBPATHONLY    = 16, // don't concatenate base with filename to result, but do build path relative to root specified
	};

 // flags sent to Process when called with a matching name
enum ScanFileProcessFlags{
SFF_DIRECTORY  = 1, // is a directory...
		SFF_DRIVE      = 2, // this is a drive...
};

/* Extended external file system interface to be able to use external file systems */
struct file_system_interface {
	void* (CPROC *open)(const TEXTCHAR *);                                                  //filename
	int (CPROC *close)(void *);                                                 //file *
	size_t (CPROC *read)(void *,char *, size_t);                    //file *, buffer, length (to read)
	size_t (CPROC *write)(void*,const char *, size_t);                    //file *, buffer, length (to write)
	size_t (CPROC *seek)( void *, size_t, int whence);
	void  (CPROC *truncate)( void *);
	void (CPROC *unlink)( const TEXTCHAR *);
	size_t (CPROC *size)( void *); // get file size
	size_t (CPROC *tell)( void *); // get file current position
	int (CPROC *flush )(void *kp);
	int (CPROC *exists)( const char *file );
	LOGICAL (CPROC*copy_write_buffer)(void );
	struct find_cursor *(CPROC *find_create_cursor )( void );
	int (CPROC *find_first)( const char *filemask, struct find_cursor *cursor );
	int (CPROC *find_close)( int ffd );
	int (CPROC *find_next)( int ffd, struct find_cursor *cursor );
	char * (CPROC *find_get_name)( struct find_cursor *cursor );
	size_t (CPROC *find_get_size)( struct find_cursor *cursor );
	// ftell can be done with seek( file, 0, SEEK_CUR );
};

/* \ \ 
   Parameters
   mask :      This is the mask used to compare 
   name :      this is the name to compare against using the mask.
   keepcase :  if TRUE, must match case also.
   
   Returns
   TRUE if name is matched by mask. Otherwise returns FALSE.
   Example
   <code lang="c++">
   if( CompareMask( "*.exe", "program.exe", FALSE ) )
   {
       // then program.exe is matched by the mask.
   }
   </code>
   Remarks
   The mask support standard 'globbing' characters.
   
   ? matches one character
   
   \* matches 0 or more characters
   
   otherwise the literal character must match, unless comparing
   case insensitive, in which case 'A' == 'a' also.                */
FILESYS_PROC  int FILESYS_API  CompareMask ( CTEXTSTR mask, CTEXTSTR name, int keepcase );

// ScanFiles usage:
//   base - base path to scan
//   mask - file mask to process if NULL or "*" is everything "*.*" must contain a .
//   pInfo is a pointer to a void* - this pointer is used to maintain
//        internal information... 
//   Process is called with the full name of any matching files
//   subcurse is a flag - set to go into all subdirectories looking for files.
// There is no way to abort the scan... 
FILESYS_PROC  int FILESYS_API  ScanFiles ( CTEXTSTR base
           , CTEXTSTR mask
           , void **pInfo
           , void CPROC Process( PTRSZVAL psvUser, CTEXTSTR name, int flags )
           , int flags 
           , PTRSZVAL psvUser );

 FILESYS_PROC int FILESYS_API  ScanFilesExx ( CTEXTSTR base
           , CTEXTSTR mask
           , void **pInfo
           , void CPROC Process( PTRSZVAL psvUser, CTEXTSTR name, int flags )
           , int flags 
           , PTRSZVAL psvUser 
		   , LOGICAL begin_sub_path 
		   , struct file_system_interface *fsi
		   );

FILESYS_PROC  void FILESYS_API  ScanDrives ( void (CPROC *Process)(PTRSZVAL user, CTEXTSTR letter, int flags)
										  , PTRSZVAL user );
// result is length of name filled into pResult if pResult == NULL && nResult = 0
// the result will the be length of the name matching the file.
FILESYS_PROC  int FILESYS_API  GetMatchingFileName ( CTEXTSTR filemask, int flags, TEXTSTR pResult, int nResult );

// searches a path for the last '/' or '\'
FILESYS_PROC  CTEXTSTR FILESYS_API  pathrchr ( CTEXTSTR path );
#ifdef __cplusplus
FILESYS_PROC  TEXTSTR FILESYS_API  pathrchr ( TEXTSTR path );
#endif
// searches a path for the first '/' or '\'
FILESYS_PROC  CTEXTSTR FILESYS_API  pathchr ( CTEXTSTR path );

// returns pointer passed (if it worked?)
FILESYS_PROC  TEXTSTR FILESYS_API  GetCurrentPath ( TEXTSTR path, int buffer_len );
FILESYS_PROC  int FILESYS_API  SetCurrentPath ( CTEXTSTR path );
/* Creates a directory. If parent peices of the directory do not
   exist, those parts are created also.
   
   
   Example
   <code lang="c#">
   MakePath( "c:\\where\\I'm/going/to/store/data" ); 
   </code>                                                       */
FILESYS_PROC  int FILESYS_API  MakePath ( CTEXTSTR path );
/* A boolean result function whether a specified name is a
   directory or not. (if not, assumes it's a file).
   
   
   Example
   <code lang="c#">
   if( IsPath( "c:/windows" ) )
   {
       // if yes, then c:\\windows is a directory.
   }
   </code>                                                 */
FILESYS_PROC LOGICAL  FILESYS_API  IsPath ( CTEXTSTR path );

FILESYS_PROC LOGICAL  FILESYS_API  IsAbsolutePath( CTEXTSTR path );

FILESYS_PROC  _64     FILESYS_API  GetFileWriteTime ( CTEXTSTR name );
FILESYS_PROC  _64     FILESYS_API  GetTimeAsFileTime ( void );
FILESYS_PROC  LOGICAL FILESYS_API  SetFileWriteTime( CTEXTSTR name, _64 filetime ); 
FILESYS_PROC  LOGICAL FILESYS_API  SetFileTimes( CTEXTSTR name
															  , _64 filetime_create  // last modification time.
															  , _64 filetime_modify // last modification time.
															  , _64 filetime_access  // last modification time.
															  );


FILESYS_PROC  void    FILESYS_API  SetDefaultFilePath ( CTEXTSTR path );
FILESYS_PROC  INDEX   FILESYS_API  SetGroupFilePath ( CTEXTSTR group, CTEXTSTR path );
FILESYS_PROC  TEXTSTR FILESYS_API  sack_prepend_path ( INDEX group, CTEXTSTR filename );


/* This is a new feature added for supporting systems without a
   current file location. This gets an integer ID of a group of
   files by name.
   
   the name 'default' is used to specify files to go into the
   'current working directory'
	
	There are some special symbols.
	. = use CurrentPath
	@ = use program path base
   ^ = use program startup path (may not be current)
   
   Parameters
   groupname :     name of the group
   default_path :  the path of the group, if the name is not
                   found.
   
   Returns
   the ID of a file group.
   Example
   <code lang="c++">
   int group = GetFileGroup( "fonts", "./fonts" );
   </code>                                                      */
FILESYS_PROC INDEX FILESYS_API  GetFileGroup ( CTEXTSTR groupname, CTEXTSTR default_path );

FILESYS_PROC TEXTSTR FILESYS_API GetFileGroupText ( INDEX group, TEXTSTR path, int path_chars );

FILESYS_PROC TEXTSTR FILESYS_API ExpandPath( CTEXTSTR path );

FILESYS_PROC LOGICAL FILESYS_API SetFileLength( CTEXTSTR path, size_t length );
/* \Returns the size of the file.
   
   
   Parameters
   name :  name of the file to get information about
   
   Returns
   \Returns the size of the file. or -1 if the file did not
   exist.                                                   */
FILESYS_PROC  size_t FILESYS_API  GetSizeofFile ( TEXTCHAR *name, P_32 unused );
#ifndef __ANDROID__
/* An extended function, which returns a _64 bit time
   appropriate for the current platform. This is meant to
   replace 'stat'. It can get all commonly checked attributes of
   a file.
   
   
   Parameters
   name :              name of the file to get information about
   lpCreationTime :    pointer to a FILETIME type to get creation
                       time. can be NULL.
   lpLastAccessTime :  pointer to a FILETIME type to get access
                       time. can be NULL.
   lpLastWriteTime :   pointer to a FILETIME type to get write
                       time. can be NULL.
   IsDirectory :       pointer to a LOGICAL to receive indicator
                       whether the file was a directory. can be
                       NULL.
   
   Returns
   \Returns the size of the file. or -1 if the file did not
	exist.                                                         */
FILESYS_PROC  _32 FILESYS_API  GetFileTimeAndSize ( CTEXTSTR name
													, LPFILETIME lpCreationTime
													,  LPFILETIME lpLastAccessTime
													,  LPFILETIME lpLastWriteTime
													, int *IsDirectory
													);

FILESYS_PROC void FILESYS_API ConvertFileIntToFileTime( _64 int_filetime, FILETIME *filetime );
FILESYS_PROC _64 FILESYS_API ConvertFileTimeToInt( const FILETIME *filetime );

#endif

// can use 0 as filegroup default - single 'current working directory'
#ifndef NEED_OLDNAMES
#define _NO_OLDNAMES
#endif
//#ifdef UNDER_CE
# ifndef O_RDONLY


#define O_RDONLY       0x0000  /* open for reading only */
#define O_WRONLY       0x0001  /* open for writing only */
#define O_RDWR         0x0002  /* open for reading and writing */
#define O_APPEND       0x0008  /* writes done at eof */

#define O_CREAT        0x0100  /* create and open file */
#define O_TRUNC        0x0200  /* open and truncate */
#define O_EXCL         0x0400  /* open only if file doesn't already exist */
#endif

#ifndef __ANDROID__
#  ifndef S_IRUSR
#    define S_IRUSR 1
#    define S_IWUSR 2
#  endif
#endif
//# endif

#ifndef __LINUX__
FILESYS_PROC  HANDLE FILESYS_API  sack_open ( INDEX group, CTEXTSTR filename, int opts, ... );
FILESYS_PROC  LOGICAL FILESYS_API  sack_set_eof ( HANDLE file_handle );
FILESYS_PROC  long  FILESYS_API   sack_tell( INDEX file_handle );
FILESYS_PROC  HANDLE FILESYS_API  sack_openfile ( INDEX group, CTEXTSTR filename, OFSTRUCT *of, int flags );
FILESYS_PROC  HANDLE FILESYS_API  sack_creat ( INDEX group, CTEXTSTR file, int opts, ... );
FILESYS_PROC  int FILESYS_API  sack_close ( HANDLE file_handle );
FILESYS_PROC  int FILESYS_API  sack_lseek ( HANDLE file_handle, int pos, int whence );
FILESYS_PROC  int FILESYS_API  sack_read ( HANDLE file_handle, POINTER buffer, int size );
FILESYS_PROC  int FILESYS_API  sack_write ( HANDLE file_handle, CPOINTER buffer, int size );
#endif

FILESYS_PROC  INDEX FILESYS_API  sack_iopen ( INDEX group, CTEXTSTR filename, int opts, ... );
FILESYS_PROC  INDEX FILESYS_API  sack_iopenfile ( INDEX group, CTEXTSTR filename, int opts, int flags );
FILESYS_PROC  INDEX FILESYS_API  sack_icreat ( INDEX group, CTEXTSTR file, int opts, ... );
FILESYS_PROC  LOGICAL FILESYS_API  sack_iset_eof ( INDEX file_handle );
FILESYS_PROC  int FILESYS_API  sack_iclose ( INDEX file_handle );
FILESYS_PROC  int FILESYS_API  sack_ilseek ( INDEX file_handle, size_t pos, int whence );
FILESYS_PROC  int FILESYS_API  sack_iread ( INDEX file_handle, POINTER buffer, int size );
FILESYS_PROC  int FILESYS_API  sack_iwrite ( INDEX file_handle, CPOINTER buffer, int size );

FILESYS_PROC  FILE* FILESYS_API  sack_fopenEx( INDEX group, CTEXTSTR filename, CTEXTSTR opts, struct file_system_interface *fsi );
FILESYS_PROC  FILE* FILESYS_API  sack_fopen ( INDEX group, CTEXTSTR filename, CTEXTSTR opts );
FILESYS_PROC  FILE* FILESYS_API  sack_fsopenEx ( INDEX group, CTEXTSTR filename, CTEXTSTR opts, int share_mode, struct file_system_interface *fsi );
FILESYS_PROC  FILE* FILESYS_API  sack_fsopen ( INDEX group, CTEXTSTR filename, CTEXTSTR opts, int share_mode );
FILESYS_PROC  struct file_system_interface * FILESYS_API sack_get_filesystem_interface( CTEXTSTR name );
FILESYS_PROC  void FILESYS_API sack_set_default_filesystem_interface( struct file_system_interface *fsi );

FILESYS_PROC  void FILESYS_API sack_register_filesystem_interface( CTEXTSTR name, struct file_system_interface *fsi );
FILESYS_PROC  int FILESYS_API  sack_fclose ( FILE *file_file );
FILESYS_PROC  size_t FILESYS_API  sack_fseek ( FILE *file_file, size_t pos, int whence );
FILESYS_PROC  size_t FILESYS_API  sack_ftell ( FILE *file_file );
FILESYS_PROC  size_t FILESYS_API  sack_fsize ( FILE *file_file );
FILESYS_PROC  size_t FILESYS_API  sack_fread ( POINTER buffer, size_t size, int count,FILE *file_file );
FILESYS_PROC  size_t FILESYS_API  sack_fwrite ( CPOINTER buffer, size_t size, int count,FILE *file_file );
FILESYS_PROC  TEXTSTR FILESYS_API  sack_fgets ( TEXTSTR  buffer, size_t size,FILE *file_file );
FILESYS_PROC  int FILESYS_API  sack_fflush ( FILE *file );
FILESYS_PROC  int FILESYS_API  sack_ftruncate ( FILE *file );

FILESYS_PROC  int FILESYS_API  sack_unlink ( INDEX group, CTEXTSTR filename );
FILESYS_PROC  int FILESYS_API  sack_rmdir( INDEX group, CTEXTSTR filename );
FILESYS_PROC  int FILESYS_API  sack_rename ( CTEXTSTR file_source, CTEXTSTR new_name );

FILESYS_PROC  void FILESYS_API sack_set_common_data_application( CTEXTSTR name );
FILESYS_PROC  void FILESYS_API sack_set_common_data_producer( CTEXTSTR name );


#ifndef NO_FILEOP_ALIAS

#  ifndef NO_OPEN_MACRO
# define open(a,...) sack_iopen(0,a,##__VA_ARGS__)
# define set_eof(a)  sack_iset_eof(a)
#  endif



#ifdef WIN32
#if !defined( SACK_BAG_EXPORTS ) && !defined( BAG_EXTERNALS )
# define _lopen(a,...) sack_open(0,a,##__VA_ARGS__)
# define tell(a)      sack_tell(a)
# define lseek(a,b,c) sack_ilseek(a,b,c)
# define _llseek(a,b,c) sack_lseek(a,b,c)
# define HFILE HANDLE
# undef HFILE_ERROR
# define HFILE_ERROR INVALID_HANDLE_VALUE

# define creat(a,...)  sack_icreat( 0,a,##__VA_ARGS__ )
# define close(a)  sack_iclose(a)
# define OpenFile(a,b,c) sack_openfile(0,a,b,c)
# define _lclose(a)  sack_close(a)
# define read(a,b,c) sack_iread(a,b,c)
# define write(a,b,c) sack_iwrite(a,b,c)
# define _lread(a,b,c) sack_read(a,b,c)
# define _lwrite(a,b,c) sack_write(a,b,c)
# define _lcreat(a,b) sack_creat(0,a,b)

# define remove(a)   sack_unlink(0,a)
# define unlink(a)   sack_unlink(0,a)
#endif
#endif

#endif //NO_FILEOP_ALIAS

#ifdef __LINUX__
#define SYSPATHCHAR WIDE("/")
#else
#define SYSPATHCHAR WIDE("\\")
#endif

FILESYS_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::filesys;
#endif

#endif

//-------------------------------------------------------------------
// $Log: filesys.h,v $
// Revision 1.21  2005/05/06 18:15:33  jim
// Add ability to set file write time.
//
// Revision 1.20  2005/03/26 02:50:53  panther
// Define a fancier function to get similar filenames
//
// Revision 1.19  2004/05/27 21:37:38  d3x0r
// Syncpoint.
//
// Revision 1.18  2004/05/06 08:11:51  d3x0r
// It's bad form to return the const char * cause there's too many others that take char * inut..
//
// Revision 1.17  2004/01/06 08:17:25  panther
// const char * on pathchar funcs
//
// Revision 1.16  2003/08/21 14:54:47  panther
// Update callback definitions
//
// Revision 1.15  2003/08/08 11:16:56  panther
// Define SYSPATHCHAR based on system
//
// Revision 1.14  2003/04/21 19:59:43  panther
// Option to return filename only
//
// Revision 1.13  2003/04/02 16:01:21  panther
// Export CompareMask
//
// Revision 1.12  2003/03/25 08:38:11  panther
// Add logging
//
// Revision 1.11  2003/02/21 16:28:13  panther
// Added user flag to scanfiles
//
// Revision 1.10  2002/08/21 19:20:12  panther
// Minor updates removing a define check unneeded, and set NULL on accesses
// to LIST_FORALL
//
// Revision 1.9  2002/08/12 22:23:08  panther
// Added support for 16 bit borland compiles.
//
//
// $Log: filesys.h,v $
// Revision 1.21  2005/05/06 18:15:33  jim
// Add ability to set file write time.
//
// Revision 1.20  2005/03/26 02:50:53  panther
// Define a fancier function to get similar filenames
//
// Revision 1.19  2004/05/27 21:37:38  d3x0r
// Syncpoint.
//
// Revision 1.18  2004/05/06 08:11:51  d3x0r
// It's bad form to return the const char * cause there's too many others that take char * inut..
//
// Revision 1.17  2004/01/06 08:17:25  panther
// const char * on pathchar funcs
//
// Revision 1.16  2003/08/21 14:54:47  panther
// Update callback definitions
//
// Revision 1.15  2003/08/08 11:16:56  panther
// Define SYSPATHCHAR based on system
//
// Revision 1.14  2003/04/21 19:59:43  panther
// Option to return filename only
//
// Revision 1.13  2003/04/02 16:01:21  panther
// Export CompareMask
//
// Revision 1.12  2003/03/25 08:38:11  panther
// Add logging
//
