#define FILESYSTEM_LIBRARY_SOURCE
#define NO_UNICODE_C
#define WINFILE_COMMON_SOURCE
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
//#undef DeleteList
#ifdef WIN32
#include <shlobj.h>
#endif
#include <filesys.h>
#include <sqlgetoption.h>

#ifndef UNDER_CE
//#include <fcntl.h>
//#include <io.h>
#endif

FILESYS_NAMESPACE


struct file{
	TEXTSTR name;
	TEXTSTR fullname;
	int fullname_size;

	PLIST handles; // HANDLE 's
	PLIST files; // FILE *'s
	INDEX group;
	struct file_system_mounted_interface *mount;
};

struct file_interface_tracker
{
	CTEXTSTR name;
	struct file_system_interface *fsi;
};

struct Group {
	TEXTSTR name;
	TEXTSTR base_path;
};

#include "filesys_local.h"

static void UpdateLocalDataPath( void )
{
#ifdef _WIN32
	TEXTCHAR path[MAX_PATH];
	TEXTCHAR *realpath;
	size_t len;
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
	SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, path );
	realpath = NewArray( TEXTCHAR, len = StrLen( path )
							  + StrLen( (*winfile_local).producer?(*winfile_local).producer:WIDE("") )
							  + StrLen( (*winfile_local).application?(*winfile_local).application:WIDE("") ) + 3 ); // worse case +3
	tnprintf( realpath, len, WIDE("%s%s%s%s%s"), path
			  , (*winfile_local).producer?WIDE("/"):WIDE(""), (*winfile_local).producer?(*winfile_local).producer:WIDE("")
			  , (*winfile_local).application?WIDE("/"):WIDE(""), (*winfile_local).application?(*winfile_local).application:WIDE("")
			  );
	if( (*winfile_local).data_file_root )
		Deallocate( TEXTSTR, (*winfile_local).data_file_root );
	(*winfile_local).data_file_root = realpath;
	MakePath( (*winfile_local).data_file_root );
#else
	(*winfile_local).data_file_root = StrDup( WIDE(".") );

#endif
}

void sack_set_common_data_producer( CTEXTSTR name )
{
	(*winfile_local).producer = StrDup( name );
	UpdateLocalDataPath();

}

void sack_set_common_data_application( CTEXTSTR name )
{
	(*winfile_local).application = StrDup( name );
	UpdateLocalDataPath();
}

static void LocalInit( void )
{
	if( !winfile_local )
	{
		SimpleRegisterAndCreateGlobal( winfile_local );
		if( !(*winfile_local).flags.bInitialized )
		{
			InitializeCriticalSec( &(*winfile_local).cs_files );
			(*winfile_local).flags.bInitialized = 1;
			(*winfile_local).flags.bLogOpenClose = 0;
			(*winfile_local).flags.bDeallocateClosedFiles = 1;
			{
#ifdef _WIN32
				sack_set_common_data_producer( WIDE( "Freedom Collective" ) );
				sack_set_common_data_application( GetProgramName() );

#else
				(*winfile_local).data_file_root = StrDup( WIDE( "~" ) );
#endif
			}
		}
	}
}

static void InitGroups( void )
{
	struct Group *group;
	TEXTCHAR tmp[256];
	// known handle '0' is 'default' which is CurrentWorkingDirectory at load.
	group = New( struct Group );
	group->base_path = StrDup( GetCurrentPath( tmp, sizeof( tmp ) ) );
	group->name = StrDup( WIDE( "Default" ) );
	AddLink( &(*winfile_local).groups, group );

	// known handle '1' is the program's load path.
	group = New( struct Group );
#ifdef __ANDROID__
	// assets and other files are in the data directory
	group->base_path = StrDup( GetStartupPath() );
#else
	group->base_path = StrDup( GetProgramPath() );
#endif
	group->name = StrDup( WIDE( "Program Path" ) );
	AddLink( &(*winfile_local).groups, group );

	// known handle '1' is the program's start path.
	group = New( struct Group );
	group->base_path = StrDup( GetStartupPath() );
	group->name = StrDup( WIDE( "Startup Path" ) );
	AddLink( &(*winfile_local).groups, group );
	(*winfile_local).have_default = TRUE;
}

static struct Group *GetGroupFilePath( CTEXTSTR group )
{
	struct Group *filegroup;
	INDEX idx;
	if( !(*winfile_local).groups )
	{
		InitGroups();
	}
	LIST_FORALL( (*winfile_local).groups, idx, struct Group *, filegroup )
	{
		if( StrCaseCmp( filegroup->name, group ) == 0 )
		{
		break;
		}
	}
	return filegroup;
}

INDEX  GetFileGroup ( CTEXTSTR groupname, CTEXTSTR default_path )
{
	struct Group *filegroup = GetGroupFilePath( groupname );
	if( !filegroup && default_path )
	{
		{
			TEXTCHAR tmp_ent[256];
			TEXTCHAR tmp[256];
			tnprintf( tmp_ent, sizeof( tmp_ent ), WIDE( "file group/%s" ), groupname );
			//lprintf( WIDE( "option to save is %s" ), tmp );
#ifdef __NO_OPTIONS__
			tmp[0] = 0;
#else
			if( (*winfile_local).have_default )
				SACK_GetProfileString( GetProgramName(), tmp_ent, default_path?default_path:WIDE( "" ), tmp, sizeof( tmp ) );
#endif
			if( tmp[0] )
				default_path = tmp;
			else if( default_path )
			{
#ifndef __NO_OPTIONS__
				SACK_WriteProfileString( GetProgramName(), tmp_ent, default_path );
#endif
			}
		}
		filegroup = New( struct Group );
		filegroup->name = StrDup( groupname );
		if( default_path )
			filegroup->base_path = StrDup( default_path );
		else
			filegroup->base_path = StrDup( WIDE( "." ) );
		AddLink( &(*winfile_local).groups, filegroup );
	}
	return FindLink( &(*winfile_local).groups, filegroup );
}

TEXTSTR GetFileGroupText ( INDEX group, TEXTSTR path, int path_chars )
{
	struct Group* filegroup = (struct Group *)GetLink( &(*winfile_local).groups, group );
	if( !filegroup )
	{
		path[0] = 0;
		return 0;
	}
	StrCpyEx( path, filegroup->base_path, path_chars );
	return path;
}

TEXTSTR ExpandPathVariable( CTEXTSTR path )
{
	TEXTSTR subst_path = NULL;
	TEXTSTR end = NULL;
	TEXTSTR tmp_path = StrDup( path );
	TEXTSTR tmp = NULL;
	TEXTSTR newest_path = NULL;
	size_t  len;
	size_t  this_length;
	INDEX   group;
	struct  Group* filegroup;

	if( path )
	{
		while( subst_path = (TEXTSTR)StrChr( tmp_path, '%' ) )
		{
			end = (TEXTSTR)StrChr( ++subst_path, '%' );
			//lprintf( WIDE( "Found magic subst in string" ) );
			if( end )
			{
				this_length = StrLen( tmp_path );

				tmp = NewArray( TEXTCHAR, len = ( end - subst_path ) + 1 );

				tnprintf( tmp, len * sizeof( TEXTCHAR ), WIDE( "%*.*s" ), (int)(end-subst_path), (int)(end-subst_path), subst_path );
				
				group = GetFileGroup( tmp, NULL );
				if( group != INVALID_INDEX ) {
					filegroup = (struct Group *)GetLink( &(*winfile_local).groups, group );
					Deallocate( TEXTCHAR*, tmp );  // must deallocate tmp

					newest_path = NewArray( TEXTCHAR, len = (subst_path - tmp_path) + StrLen( filegroup->base_path ) + (this_length - (end - tmp_path)) + 1 );

					//=======================================================================
					// Get rid of the ending '%' AND any '/' or '\' that might come after it
					//=======================================================================
					tnprintf( newest_path, len, WIDE( "%*.*s%s/%s" ), (int)((subst_path - tmp_path) - 1), (int)((subst_path - tmp_path) - 1), tmp_path, filegroup->base_path,
						((end + 1)[0] == '/' || (end + 1)[0] == '\\') ? (end + 2) : (end + 1) );

					Deallocate( TEXTCHAR*, tmp_path );
					tmp_path = ExpandPathVariable( newest_path );
					Deallocate( TEXTCHAR*, newest_path );
				}
				else {
					CTEXTSTR external_var = OSALOT_GetEnvironmentVariable( tmp );

					if( external_var ) {
						Deallocate( TEXTCHAR*, tmp );  // must deallocate tmp

						newest_path = NewArray( TEXTCHAR, len = (subst_path - tmp_path) + StrLen( external_var ) + (this_length - (end - tmp_path)) + 1 );

						//=======================================================================
						// Get rid of the ending '%' AND any '/' or '\' that might come after it
						//=======================================================================
						tnprintf( newest_path, len, WIDE( "%*.*s%s/%s" ), (int)((subst_path - tmp_path) - 1), (int)((subst_path - tmp_path) - 1), tmp_path, external_var,
							((end + 1)[0] == '/' || (end + 1)[0] == '\\') ? (end + 2) : (end + 1) );

						tmp_path = ExpandPathVariable( newest_path );
						Deallocate( TEXTCHAR*, newest_path );
					}
				}

				if( (*winfile_local).flags.bLogOpenClose )
					lprintf( WIDE( "transform subst [%s]" ), tmp_path );
			}
		}
	}
	return tmp_path;
}

TEXTSTR ExpandPathEx( CTEXTSTR path, struct file_system_interface *fsi )
{
	TEXTSTR tmp_path = NULL;
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "input path is [%s]" ), path );

	if( path )
	{
		if( !fsi && !IsAbsolutePath( path ) )
		{
			if( ( path[0] == '.' ) && ( ( path[1] == 0 ) || ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				TEXTCHAR here[256];
				size_t len;
				GetCurrentPath( here, sizeof( here ) );
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				tnprintf( tmp_path, len, WIDE( "%s%s%s" )
						 , here
						 , path[1]?WIDE("/"):WIDE("")
						 , path[1]?(path + 2):WIDE("") );
			}
			else if( ( path[0] == '@' ) && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = GetLibraryPath();
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				tnprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( ( path[0] == '#' ) && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = GetProgramPath();
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				tnprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( ( path[0] == '~' ) && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = OSALOT_GetEnvironmentVariable(WIDE("HOME"));
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				tnprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( ( path[0] == '*' ) && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = (*winfile_local).data_file_root;
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				tnprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( path[0] == '^' && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = GetStartupPath();
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				tnprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( path[0] == '%' )
			{
				tmp_path = ExpandPathVariable( path );
			}
			else
			{
				tmp_path = StrDup( path );
			}
#if __ANDROID__
			{
				int len_base;
				TEXTCHAR here[256];
				size_t len;
				size_t ofs;
				GetCurrentPath( here, sizeof( here ) );
				if( StrStr( tmp_path, here ) )
					len = StrLen( here );
				else
					len = 0;

		/*
				if( (*winfile_local).flags.bLogOpenClose )
					lprintf( "Fix dots in [%s]", tmp_path );
				for( ofs = len+1; tmp_path[ofs]; ofs++ )
				{
					if( tmp_path[ofs] == '/' )
						tmp_path[ofs] = '.';
					if( tmp_path[ofs] == '\\' )
						tmp_path[ofs] = '.';
				}
				if( (*winfile_local).flags.bLogOpenClose )
				lprintf( "Fixed result [%s]", tmp_path );
			*/
			}
#endif
		}
		else if( StrChr( path, '%' ) != NULL )
		{
			tmp_path = ExpandPathVariable( path );
		}
		else
		{
			tmp_path = StrDup( path );
		}
	}

	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "output path is [%s]" ), tmp_path );

	return tmp_path;
}

TEXTSTR ExpandPath( CTEXTSTR path )
{
	return ExpandPathEx( path, NULL );
}

INDEX  SetGroupFilePath ( CTEXTSTR group, CTEXTSTR path )
{
	struct Group *filegroup = GetGroupFilePath( group );
	if( !filegroup )
	{
		TEXTCHAR tmp[256];
		filegroup = New( struct Group );
		filegroup->name = StrDup( group );
		filegroup->base_path = StrDup( path );
		tnprintf( tmp, sizeof( tmp ), WIDE( "file group/%s" ), group );
#ifndef __NO_OPTIONS__
		if( (*winfile_local).have_default )
		{
			TEXTCHAR tmp2[256];
			SACK_GetProfileString( GetProgramName(), tmp, WIDE( "" ), tmp2, sizeof( tmp2 ) );
		if( StrCaseCmp( path, tmp2 ) )
				SACK_WriteProfileString( GetProgramName(), tmp, path );
		}
#endif
		AddLink( &(*winfile_local).groups, filegroup );
		(*winfile_local).have_default = TRUE;
	}
	else
	{
		Deallocate( TEXTCHAR*, filegroup->base_path );
		filegroup->base_path = StrDup( path );
	}
	return FindLink( &(*winfile_local).groups, filegroup );
}


void SetDefaultFilePath( CTEXTSTR path )
{
	TEXTSTR tmp_path = NULL;
	struct Group *filegroup;
	LocalInit();
	filegroup = (struct Group *)GetLink( &(*winfile_local).groups, 0 );
	tmp_path = ExpandPath( path );
	if( (*winfile_local).groups && filegroup )
	{
		Deallocate( TEXTSTR, filegroup->base_path );
		filegroup->base_path = StrDup( tmp_path?tmp_path:path );
	}
	else
	{
		SetGroupFilePath( WIDE( "Default" ), tmp_path?tmp_path:path );
	}
	if( tmp_path )
		Deallocate( TEXTCHAR*, tmp_path );
}

static TEXTSTR PrependBasePathEx( INDEX groupid, struct Group *group, CTEXTSTR filename, LOGICAL expand_path )
{
	TEXTSTR real_filename = filename?ExpandPath( filename ):NULL;
	TEXTSTR fullname;

	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE("Prepend to {%s} %p %") _size_f, real_filename, group, groupid );

	if( (*winfile_local).groups )
	{
		//SetDefaultFilePath( GetProgramPath() );
		if( !group )
		{
			if( groupid >= 0 )
				group = (struct Group *)GetLink( &(*winfile_local).groups, groupid );
		}
	}
	if( !group || ( filename && ( IsAbsolutePath( real_filename ) ) ) )
	{
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE("already an absolute path.  [%s]"), real_filename );
		return real_filename;
	}

	{
		TEXTSTR tmp_path;
		size_t len;
		if( expand_path )
			tmp_path = ExpandPath( group->base_path );
		else
			tmp_path = group->base_path;
		fullname = NewArray( TEXTCHAR, len = StrLen( filename ) + StrLen(tmp_path) + 2 );
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf(WIDE("prepend %s[%s] with %s"), group->base_path, tmp_path, filename );
		tnprintf( fullname, len, WIDE("%s/%s"), tmp_path, real_filename );
#if __ANDROID__
		{
			int len_base;
			static TEXTCHAR here[256];
			static size_t len;
			size_t ofs;
			if( !here[0] )
			{
				GetCurrentPath( here, sizeof( here ) );
			}
			if( StrStr( tmp_path, here ) )
				len = StrLen( here );
			else
				len = 0;

		/*
			if( (*winfile_local).flags.bLogOpenClose )
				lprintf( WIDE("Fix dots in [%s]"), fullname );
			for( ofs = len+1; fullname[ofs]; ofs++ )
			{
				if( fullname[ofs] == '/' )
					fullname[ofs] = '.';
				if( fullname[ofs] == '\\' )
					fullname[ofs] = '.';
			}
		*/
		}
#endif
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE("result %s"), fullname );
		if( expand_path )
			Deallocate( TEXTCHAR*, tmp_path );
		Deallocate( TEXTCHAR*, real_filename );
	}
	return fullname;
}

static TEXTSTR PrependBasePath( INDEX groupid, struct Group *group, CTEXTSTR filename )
{
   return PrependBasePathEx(groupid,group,filename,TRUE );
}


TEXTSTR sack_prepend_path( INDEX group, CTEXTSTR filename )
{
	struct Group *filegroup = (struct Group *)GetLink( &(*winfile_local).groups, group );
	TEXTSTR result = PrependBasePath( group, filegroup, filename );
	return result;
}

#ifdef __LINUX__
#define HANDLE int
#define INVALID_HANDLE_VALUE -1
#endif


HANDLE sack_open( INDEX group, CTEXTSTR filename, int opts, ... )
{
	HANDLE handle;
	struct file *file;
	INDEX idx;
	EnterCriticalSec( &(*winfile_local).cs_files );
	LIST_FORALL( (*winfile_local).files, idx, struct file *, file )
	{
		if( StrCmp( file->name, filename ) == 0 )
		{
			break;
		}
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	if( !file )
	{
		struct Group *filegroup = (struct Group *)GetLink( &(*winfile_local).groups, group );
		file = New( struct file );
		file->name = StrDup( filename );
		file->fullname = PrependBasePath( group, filegroup, filename );
		file->handles = NULL;
		file->files = NULL;
		file->group = group;
		EnterCriticalSec( &(*winfile_local).cs_files );
		AddLink( &(*winfile_local).files,file );
		LeaveCriticalSec( &(*winfile_local).cs_files );
	}
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "Open File: [%s]" ), file->fullname );
#ifdef __LINUX__
#  undef open
	{
#ifdef UNICODE
		char *tmpfile = CStrDup( file->fullname );
		handle = open( tmpfile, opts );
		Deallocate( char *, tmpfile );
#else
		handle = open( file->fullname, opts );
#endif
	}
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "open %s %d %d" ), file->fullname, handle, opts );
#else
	switch( opts & 3 )
	{
	case 0:
	handle = CreateFile( file->fullname
							, GENERIC_READ
							, FILE_SHARE_READ
							, NULL
							, ((opts&O_CREAT)?CREATE_ALWAYS:OPEN_EXISTING)
							, FILE_ATTRIBUTE_NORMAL
							, NULL );
	break;
	case 1:
	handle = CreateFile( file->fullname
							, GENERIC_WRITE
							, FILE_SHARE_READ|FILE_SHARE_WRITE
							, NULL
							, ((opts&O_CREAT)?CREATE_ALWAYS:OPEN_EXISTING)
							, FILE_ATTRIBUTE_NORMAL
							, NULL );
		break;
	case 2:
	case 3:
	handle = CreateFile( file->fullname
							,(GENERIC_READ|GENERIC_WRITE)
							, FILE_SHARE_READ|FILE_SHARE_WRITE
							, NULL
							, ((opts&O_CREAT)?CREATE_ALWAYS:OPEN_EXISTING)
							, FILE_ATTRIBUTE_NORMAL
							, NULL );
	break;
	}
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "open %s %p %08x" ), file->fullname, (POINTER)handle, opts );
#endif
	if( handle == INVALID_HANDLE_VALUE )
	{
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open file [%s]=[%s]" ), file->name, file->fullname );
		return INVALID_HANDLE_VALUE;
	}
	if( handle != INVALID_HANDLE_VALUE )
	{
		HANDLE *holder = New( HANDLE );
		holder[0] = handle;
		AddLink( &file->handles, holder );
	}
	return handle;
}

#ifdef WIN32
HANDLE sack_openfile( INDEX group,CTEXTSTR filename, OFSTRUCT *of, int flags )
{
#ifdef _UNICODE
	char *tmpname = WcharConvert( filename );
#undef OpenFile
	HANDLE result = (HANDLE)OpenFile(tmpname,of,flags);
	Deallocate( char*, tmpname );
	return result;
#else
#undef OpenFile
	return (HANDLE)(uintptr_t)OpenFile(filename,of,flags);
#endif
}
#endif

struct file *FindFileByHandle( HANDLE file_file )
{
	struct file *file;
	INDEX idx;
	EnterCriticalSec( &(*winfile_local).cs_files );
	LIST_FORALL( (*winfile_local).files, idx, struct file *, file )
	{
		INDEX idx2;
		HANDLE* check;
		LIST_FORALL( file->handles, idx2, HANDLE*, check )
		{
			if( check[0] == file_file )
				break;
		}
		if( check )
			break;
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	return file;
}

LOGICAL sack_iset_eof ( INDEX file_handle )
{
	HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, file_handle );
	HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
	return SetEndOfFile( handle );
#else
	return ftruncate( handle, lseek( handle, 0, SEEK_CUR ) );
#endif

}

struct file *FindFileByFILE( FILE *file_file )
{
	struct file *file;
	INDEX idx;
	LocalInit();
	EnterCriticalSec( &(*winfile_local).cs_files );
	LIST_FORALL( (*winfile_local).files, idx, struct file *, file )
	{
		INDEX idx2;
		FILE *check;
		LIST_FORALL( file->files, idx2, FILE *, check )
		{
			if( check == file_file )
				break;
		}
		if( check )
			break;
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	return file;
}

LOGICAL sack_set_eof ( HANDLE file_handle )
{
	struct file *file;
	file = FindFileByFILE( (FILE*)(uintptr_t)file_handle );
	if( file )
	{
		if( file->mount )
		{
			file->mount->fsi->truncate( (void*)(uintptr_t)file_handle );
			//lprintf( WIDE("result is %d"), file->mount->fsi->size( (void*)file_handle ) );
		}
		else
		{
#ifdef _WIN32
			;
#else
			truncate( file->fullname, sack_ftell( (FILE*)(uintptr_t)file_handle ) );
#endif
		}
		return TRUE;
	}
	else
	{
		HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, (INDEX)file_handle );
		HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		return SetEndOfFile( handle );
#else
		return ftruncate( handle, lseek( handle, 0, SEEK_CUR ) );
#endif
	}
}

int sack_ftruncate( FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file )
	{
		if( file->mount && file->mount->fsi )
		{
			file->mount->fsi->truncate( (void*)file_file );
			//lprintf( WIDE("result is %d"), file->mount->fsi->size( (void*)file_file ) );
		}
		else
		{
#ifdef _WIN32
			;
#else
			truncate( file->fullname, sack_ftell( (FILE*)file_file ) );
#endif
		}
		return TRUE;
	}
	return FALSE;
}


long sack_tell( INDEX file_handle )
{
	HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, file_handle );
	HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef WIN32
	uint32_t length = SetFilePointer( handle // must have GENERIC_READ and/or GENERIC_WRITE
								, 0	// do not move pointer 
								, NULL  // hFile is not large enough to need this pointer 
								, FILE_CURRENT);  // provides offset from current position 
	return length;
#else
	return lseek( handle, 0, SEEK_SET );
#endif
}


HANDLE sack_creat( INDEX group, CTEXTSTR file, int opts, ... )
{
	return sack_open( group, file, opts | O_CREAT );
}


int sack_lseek( HANDLE file_handle, int pos, int whence )
{
#ifdef _WIN32
	return SetFilePointer(file_handle,pos,NULL,whence);
#else
	return lseek( file_handle, pos, whence );
#endif
}

int sack_read( HANDLE file_handle, POINTER buffer, int size )
{
#ifdef _WIN32
	DWORD dwLastReadResult;
	//lprintf( WIDE( "..." ) );
	return (ReadFile( (HANDLE)file_handle, buffer, size, &dwLastReadResult, NULL )?dwLastReadResult:-1 );
#else
	return read( file_handle, buffer, size );
#endif
}

int sack_write( HANDLE file_handle, CPOINTER buffer, int size )
{
#ifdef _WIN32
	DWORD dwLastWrittenResult;
	return (WriteFile( (HANDLE)file_handle, (POINTER)buffer, size, &dwLastWrittenResult, NULL )?dwLastWrittenResult:-1 );
#else
	return write( file_handle, buffer, size );
#endif
}

INDEX sack_icreat( INDEX group, CTEXTSTR file, int opts, ... )
{
	return sack_iopen( group, file, opts | O_CREAT );
}

int sack_close( HANDLE file_handle )
{
	struct file *file = FindFileByHandle( (HANDLE)file_handle );
	if( file )
	{
		SetLink( &file->handles, (INDEX)file_handle, NULL );
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE("Close %s"), file->fullname );
		/*
		Deallocate( TEXTCHAR*, file->name );
		Deallocate( TEXTCHAR*, file->fullname );
		Deallocate( TEXTCHAR*, file );
		DeleteLink( &(*winfile_local).files, file );
		*/
	}
	if( file_handle != INVALID_HANDLE_VALUE )
#ifdef _WIN32
		return CloseHandle((HANDLE)file_handle);
#else
		return close( file_handle );
#endif
	return 0;
}

INDEX sack_iopen( INDEX group, CTEXTSTR filename, int opts, ... )
{
	HANDLE h;
	INDEX result;
	h = sack_open( group, filename, opts );
	if( h == INVALID_HANDLE_VALUE )
	{
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open %s" ), filename );
		return INVALID_INDEX;
	}
	EnterCriticalSec( &(*winfile_local).cs_files );
	{
		HANDLE *holder = New( HANDLE );
		holder[0] = h;
		AddLink( &(*winfile_local).handles, holder );
		result = FindLink( &(*winfile_local).handles, holder );
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "return iopen of [%s]=%p(%")_size_f WIDE(")?" ), filename, h, (size_t)result );
	return result;
}

int sack_iclose( INDEX file_handle )
{
	int result;
	EnterCriticalSec( &(*winfile_local).cs_files );
	{
		HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, file_handle );
		HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
		SetLink( &(*winfile_local).handles, file_handle, 0 );
		Deallocate( HANDLE*, holder );
		result = sack_close( handle );
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	return result;
}

int sack_ilseek( INDEX file_handle, size_t pos, int whence )
{
	int result;
	EnterCriticalSec( &(*winfile_local).cs_files );
	{
		 HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, file_handle );
		 HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		result = SetFilePointer(handle,(LONG)pos,NULL,whence);
#else
		result = lseek( handle, pos, whence );
#endif
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	return result;
}

int sack_iread( INDEX file_handle, POINTER buffer, int size )
{
	EnterCriticalSec( &(*winfile_local).cs_files );
	{
		 HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, file_handle );
		 HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		DWORD dwLastReadResult;
		//lprintf( WIDE( "... %p %p" ), file_handle, h );
		LeaveCriticalSec( &(*winfile_local).cs_files );
		return (ReadFile( handle, (POINTER)buffer, size, &dwLastReadResult, NULL )?dwLastReadResult:-1 );
#else
		return read( handle, buffer, size );
#endif
	}
}

int sack_iwrite( INDEX file_handle, CPOINTER buffer, int size )
{
	EnterCriticalSec( &(*winfile_local).cs_files );
	{
		 HANDLE *holder = (HANDLE*)GetLink( &(*winfile_local).handles, file_handle );
		 HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		DWORD dwLastWrittenResult;
		LeaveCriticalSec( &(*winfile_local).cs_files );
		return (WriteFile( handle, (POINTER)buffer, size, &dwLastWrittenResult, NULL )?dwLastWrittenResult:-1 );
#else
		return write( handle, buffer, size );
#endif
	}
}



int sack_unlinkEx( INDEX group, CTEXTSTR filename, struct file_system_mounted_interface *mount )
{
	while( mount )
	{
#ifdef __LINUX__
		int okay;
		TEXTSTR tmp = PrependBasePath( group, NULL, filename );
#  ifdef UNICODE
		char *tmpname = CStrDup( tmp );
		okay = unlink( tmpname );
		Deallocate( char*, tmpname );
#  else
		okay = unlink( filename );
#  endif
		Deallocate( TEXTCHAR*, tmp );
#else
		int okay = 1;
		if( mount->fsi )
		{
#ifdef UNICODE
			char *_filename = CStrDup( filename );
#  define filename _filename
#endif
			if( mount->fsi->exists( mount->psvInstance, filename ) )
			{
				mount->fsi->_unlink( mount->psvInstance, filename );
				okay = 0;
			}
#ifdef UNICODE
			Deallocate( char *, _filename );
#  undef filename
#endif
		}
		else
		{
			TEXTSTR tmp = PrependBasePath( group, NULL, filename );
			okay = DeleteFile(tmp);
			Deallocate( TEXTCHAR*, tmp );
		}
#endif
		if( !okay )
			return !okay;
		mount = mount->next;
	}
	return 0;
}

int sack_unlink( INDEX group, CTEXTSTR filename )
{
	return sack_unlinkEx( group, filename, (*winfile_local).mounted_file_systems );
}

int sack_rmdir( INDEX group, CTEXTSTR filename )
{
#ifdef __LINUX__
	int okay;
	TEXTSTR tmp = PrependBasePath( group, NULL, filename );
#ifdef UNICODE
	char *tmpname = CStrDup( tmp );
	okay = rmdir( tmpname );
	Deallocate( char*, tmpname );
#else
	okay = rmdir( filename );
#endif
	Deallocate( TEXTCHAR*, tmp );
	return !okay; // unlink returns TRUE is 0, else error...
#else
	int okay;
	TEXTSTR tmp = PrependBasePath( group, NULL, filename );
	okay = RemoveDirectory(tmp);
	Deallocate( TEXTCHAR*, tmp );
	return !okay; // unlink returns TRUE is 0, else error...
#endif
}

#undef open
#undef fopen

FILE * sack_fopenEx( INDEX group, CTEXTSTR filename, CTEXTSTR opts, struct file_system_mounted_interface *mount )
{
	FILE *handle = NULL;
	struct file *file;
	INDEX idx;
	LOGICAL memalloc = FALSE;
	LOGICAL single_mount = (mount != NULL );
	LocalInit();
	EnterCriticalSec( &(*winfile_local).cs_files );

	if( !mount )
		mount = (*winfile_local).mounted_file_systems;

	if( !StrChr( opts, 'r' ) && !StrChr( opts, '+' ) )
		while( mount )
		{  // skip roms...
			//lprintf( "check mount %p %d", mount, mount->writeable );
			if( mount->writeable )
				break;
			mount = mount->next;
		}

	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE("open %s %p(%s) %s (%d)"), filename, mount, mount->name, opts, mount?mount->writeable:1 );
	LIST_FORALL( (*winfile_local).files, idx, struct file *, file )
	{
		if( ( file->group == group )
			&& ( StrCmp( file->name, filename ) == 0 ) 
			&& ( file->mount == mount ) )
		{
			break;
		}
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );

	if( !file )
	{
		TEXTSTR tmpname = NULL;
		struct Group *filegroup = (struct Group *)GetLink( &(*winfile_local).groups, group );
		file = New( struct file );
		memalloc = TRUE;

		if( StrChr( filename, '%' ) )
		{
			tmpname = ExpandPathVariable( filename );
			filename = tmpname;
		}
		if( (filename[0] == '@') || (filename[0] == '*') || (filename[0] == '~') )
		{
			tmpname = ExpandPathEx( filename, NULL );
			filename = tmpname;
		}
		file->handles = NULL;
		file->files = NULL;
		file->name = StrDup( filename );
		file->mount = mount;
		if( ( !file->mount || !file->mount->fsi ) && !IsAbsolutePath( filename ) )
		{
			tmpname = ExpandPath( filename );
			file->fullname = PrependBasePath( group, filegroup, tmpname );
			Deallocate( TEXTCHAR*, tmpname );
		}
		else
		{
			if( mount && group == 0 )
			{
				file->fullname = StrDup( file->name );
				if( (*winfile_local).flags.bLogOpenClose )
					lprintf( WIDE("full is %s"), file->fullname );
			}
			else
			{
				file->fullname = PrependBasePathEx( group, filegroup, file->name, !mount );
				if( (*winfile_local).flags.bLogOpenClose )
					lprintf( WIDE("full is %s %d"), file->fullname, (int)group );
			}
			//file->fullname = file->name;
		}
		file->group = group;
		EnterCriticalSec( &(*winfile_local).cs_files );
		AddLink( &(*winfile_local).files,file );
		LeaveCriticalSec( &(*winfile_local).cs_files );
	}
	if( (file->fullname[0] == '@') || (file->fullname[0] == '*') || (file->fullname[0] == '~') )
	{
		TEXTSTR tmpname = ExpandPathEx( file->fullname, NULL );
		Deallocate( TEXTSTR, file->fullname );
		file->fullname = tmpname;
	}
	if( StrChr( file->fullname, '%' ) )
	{
		if( memalloc )
		{
			DeleteLink( &(*winfile_local).files, file );
			Deallocate( TEXTCHAR*, file->name );
			Deallocate( TEXTCHAR*, file->fullname );
			Deallocate( struct file *, file );
		}
		//DebugBreak();
		return NULL;
	}
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "Open File: [%s]" ), file->fullname );

	if( mount && mount->fsi )
	{
		if( StrChr( opts, 'r' ) && !StrChr( opts, '+' ) )
		{
			struct file_system_mounted_interface *test_mount = mount;
			while( !handle && test_mount )
			{
				if( test_mount->fsi )
				{
#if UNICODE
					char *_fullname = CStrDup( file->fullname );
#else
#  define _fullname file->fullname
#endif
					file->mount = test_mount;
					if( (*winfile_local).flags.bLogOpenClose )
						lprintf( WIDE("Call mount %s to check if file exists %s"), test_mount->name, file->fullname );
					if( test_mount->fsi->exists( test_mount->psvInstance, _fullname ) )
					{
						handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, _fullname );
					}
					else if( single_mount )
					{
#if UNICODE
						Deallocate( char *, _fullname );
#else
#  undef _fullname 
#endif
						return NULL;
					}
#if UNICODE
					Deallocate( char *, _fullname );
#endif
				}
				else
					goto default_fopen;
				test_mount = test_mount->next;
			}
		}
		else
		{
			struct file_system_mounted_interface *test_mount = mount;
			//lprintf( "full is %s", file->fullname );
			while( !handle && test_mount )
			{
				file->mount = test_mount;
				if( test_mount->fsi && test_mount->writeable )
				{
#ifdef UNICODE
					char* _fullname = CStrDup( file->fullname );
#else
#  define _fullname file->fullname
#endif
					if( (*winfile_local).flags.bLogOpenClose )
						lprintf( WIDE("Call mount %s to open file %s"), test_mount->name, file->fullname );
					handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, _fullname );
#ifdef UNICODE
					Deallocate( char*, _fullname );
#else
#  undef _fullname 
#endif
				}
				else
					goto default_fopen;
				test_mount = test_mount->next;
			}
		}
	}
	if( !handle )
	{
default_fopen:
		file->mount = NULL;

#ifdef __LINUX__
#  ifdef UNICODE
		char *tmpname = CStrDup( file->fullname );
		char *tmpopts = CStrDup( opts );
		handle = fopen( tmpname, tmpopts );
		Deallocate( char*, tmpname );
		Deallocate( char*, tmpopts );
#  else
		handle = fopen( file->fullname, opts );
#  endif
#else
#  ifdef _STRSAFE_H_INCLUDED_
#    ifdef UNICODE
		char *tmpname = CStrDup( file->fullname );
		char *tmpopts = CStrDup( opts );
		fopen_s( &handle, tmpname, tmpopts );
		Deallocate( char*, tmpname );
		Deallocate( char*, tmpopts );
#    else
		{
			wchar_t *tmp = CharWConvert( file->fullname );
			wchar_t *wopts = CharWConvert( opts );
         handle = _wfopen( tmp, wopts );
			//_wfopen_s( &handle, tmp, wopts );
			Deallocate( wchar_t *, tmp );
			Deallocate( wchar_t *, wopts );
		}
#    endif
#  else
		handle = fopen( file->fullname, opts );
#  endif
#endif
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE("native opened %s"), file->fullname );
	}
	if( !handle )
	{
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open file [%s]=[%s]" ), file->name, file->fullname );
		return NULL;
	}
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "sack_open %s (%s)" ), file->fullname, opts );
	AddLink( &file->files, handle );
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "Added FILE* %p and list is %p" ), handle, file->files );
	return handle;
}

FILE*  sack_fopen ( INDEX group, CTEXTSTR filename, CTEXTSTR opts )
{
	return sack_fopenEx( group, filename, opts, NULL );
}

FILE*  sack_fsopenEx( INDEX group
					 , CTEXTSTR filename
					 , CTEXTSTR opts
					 , int share_mode
					 , struct file_system_mounted_interface *mount )
{
	FILE *handle = NULL;
	struct file *file;
	INDEX idx;
	LOGICAL single_mount = ( mount != NULL );
	LocalInit();
	EnterCriticalSec( &(*winfile_local).cs_files );
	if( !mount )
		mount = (*winfile_local).mounted_file_systems;

	if( !StrChr( opts, 'r' ) && !StrChr( opts, '+' ) )
		while( mount )
		{  // skip roms...
			//lprintf( "check mount %p %d", mount, mount->writeable );
			if( mount->writeable )
				break;
			mount = mount->next;
		}

	LIST_FORALL( (*winfile_local).files, idx, struct file *, file )
	{
		if( ( file->group == group )
			&& ( StrCmp( file->name, filename ) == 0 ) 
			&& ( file->mount == mount ) )
		{
			break;
		}
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );
	if( !file )
	{
		struct Group *filegroup = (struct Group *)GetLink( &(*winfile_local).groups, group );
		file = New( struct file );
		file->handles = NULL;
		file->files = NULL;
		file->name = StrDup( filename );
		file->group = group;
		file->mount = mount;
		if( !mount || !mount->fsi )
			file->fullname = PrependBasePath( group, filegroup, filename );
		else
			file->fullname = StrDup( filename );
		EnterCriticalSec( &(*winfile_local).cs_files );
		AddLink( &(*winfile_local).files,file );
		LeaveCriticalSec( &(*winfile_local).cs_files );
	}
	if( mount && mount->fsi )
	{
		if( StrChr( opts, 'r' ) && !StrChr( opts, '+' ) )
		{
			struct file_system_mounted_interface *test_mount = mount;
			while( !handle && test_mount && test_mount->fsi )
			{
#ifdef UNICODE
				char *_fullname = CStrDup( file->fullname );
#else
#  define _fullname file->fullname
#endif
				file->mount = test_mount;
				if( (*winfile_local).flags.bLogOpenClose )
					lprintf( WIDE("Call mount %s to check if file exists %s"), test_mount->name, file->fullname );
				if( test_mount->fsi->exists( test_mount->psvInstance, _fullname ) )
				{
					handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, _fullname );
				}
#ifdef UNICODE
				Deallocate( char *, _fullname );
#else
#  undef _fullname 
#endif
				if( !handle && single_mount )
				{
					return NULL;
				}
				test_mount = test_mount->next;
			}
		}
		else
		{
			struct file_system_mounted_interface *test_mount = mount;
			//lprintf( "full is %s", file->fullname );
			while( !handle && test_mount )
			{
				file->mount = test_mount;
				if( test_mount->fsi && test_mount->writeable )
				{
#ifdef UNICODE
					char* _fullname = CStrDup( file->fullname );
#else
#  define _fullname file->fullname
#endif
					if( (*winfile_local).flags.bLogOpenClose )
						lprintf( WIDE("Call mount %s to open file %s"), test_mount->name, file->fullname );
					handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, _fullname );
#ifdef UNICODE
					Deallocate( char*, _fullname );
#else
#  undef _fullname 
#endif
				}
				else
					goto default_fopen;
				test_mount = test_mount->next;
			}
		}
			//file->fsi = mount?mount->fsi:NULL;
	}
	if( !handle )
	{
default_fopen:
#ifdef __LINUX__
#  ifdef UNICODE
		char *tmpname = CStrDup( file->fullname );
		char *tmpopts = CStrDup( opts );
		handle = fopen( tmpname, tmpopts );
		Deallocate( char*, tmpname );
		Deallocate( char*, tmpopts );
#  else
		handle = fopen( file->fullname, opts );
#  endif
#else
		{
			wchar_t *tmp = CharWConvert( file->fullname );
			wchar_t *wopts = CharWConvert( opts );
			handle = _wfsopen( tmp, wopts, share_mode );
			Deallocate( wchar_t *, tmp );
			Deallocate( wchar_t *, wopts );
		}
#endif
	}
	if( !handle )
	{
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open file [%s]=[%s]" ), file->name, file->fullname );
		return NULL;
	}
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "sack_open %s (%s)" ), file->fullname, opts );
	EnterCriticalSec( &(*winfile_local).cs_files );
	AddLink( &file->files, handle );
	LeaveCriticalSec( &(*winfile_local).cs_files );
	if( (*winfile_local).flags.bLogOpenClose )
		lprintf( WIDE( "Added FILE* %p and list is %p" ), handle, file->files );
	return handle;
}

FILE*  sack_fsopen( INDEX group, CTEXTSTR filename, CTEXTSTR opts, int share_mode )
{
	return sack_fsopenEx( group, filename, opts, share_mode, NULL/*(*winfile_local).mounted_file_systems*/ );
}

size_t sack_fsize ( FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
		return file->mount->fsi->size( file_file );
	{
		size_t here = ftell( file_file );
		size_t length;
		fseek( file_file, 0, SEEK_END );
		length = ftell( file_file );
		fseek( file_file, (long)here, SEEK_SET );
		return length;
	}
}

size_t sack_ftell ( FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
		return file->mount->fsi->seek( file_file, 0, SEEK_CUR );
	return ftell( file_file );
}

size_t  sack_fseek ( FILE *file_file, size_t pos, int whence )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
	{
		return file->mount->fsi->seek( file_file, pos, whence );
	}
	if( fseek( file_file, (long)pos, whence ) )
		return -1;
	//struct file *file = FindFileByFILE( file_file );
	return ftell( file_file );
}

int  sack_fflush ( FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
	{
		return file->mount->fsi->flush( file_file );
		//DeleteLink( &file->files, file_file );
		//file->fsi->close( file_file );
		//file_file = (FILE*)file->fsi->open( file->fullname );
		//AddLink( &file->files, file_file );
		//return 0;
	}
	return fflush( file_file );
}


int  sack_fclose ( FILE *file_file )
{
	struct file *file;
	EnterCriticalSec( &(*winfile_local).cs_files );
	file = FindFileByFILE( file_file );
	if( file )
	{
		int status;
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE("Closing %s"), file->fullname );
		if( file->mount && file->mount->fsi )
			status = file->mount->fsi->_close( file_file );
		else
			status = fclose( file_file );
		DeleteLink( &file->files, file_file );
		if( !GetLinkCount( file->files ) ) {
			DeleteListEx( &file->files DBG_SRC );
			DeleteLink( &(*winfile_local).files, file );

			Deallocate( TEXTCHAR*, file->name );
			Deallocate( TEXTCHAR*, file->fullname );
			Deallocate( struct file*, file );
		}
		if( (*winfile_local).flags.bLogOpenClose )
			lprintf( WIDE( "deleted FILE* %p and list is %p" ), file_file, file->files );
		LeaveCriticalSec( &(*winfile_local).cs_files );
		return status;
	}
	LeaveCriticalSec( &(*winfile_local).cs_files );

	return fclose( file_file );
}
 size_t  sack_fread ( POINTER buffer, size_t size, int count,FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file->mount && file->mount->fsi )
		return file->mount->fsi->_read( file_file, (char*)buffer, size * count );
	return fread( buffer, size, count, file_file );
}
 size_t  sack_fwrite ( CPOINTER buffer, size_t size, int count,FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
	{
		size_t result;
		if( !file->mount->fsi->copy_write_buffer || file->mount->fsi->copy_write_buffer() )
		{
			POINTER dupbuf = malloc( size*count + 3 );
			memcpy( dupbuf, buffer, size*count );
			result = file->mount->fsi->_write( file_file, (const char*)dupbuf, size * count );
			Deallocate( POINTER, dupbuf );
		}
		else
			result = file->mount->fsi->_write( file_file, (const char*)buffer, size * count );
		return result;
	}
	return fwrite( (POINTER)buffer, size, count, file_file );
}


TEXTSTR sack_fgets ( TEXTSTR buffer, size_t size,FILE *file_file )
{
#ifdef _UNICODE
	//char *tmpbuf = NewArray( char, size+1);
	//TEXTSTR tmp_wbuf;
	fgets( (char*)buffer, size, file_file );
	//tmp_wbuf = CharWConvert( tmpbuf );
	//StrCpyEx( buffer, tmp_wbuf, size );
	return buffer;
#else
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
	{
		size_t n;
		char *output = buffer;
		size = size-1;
		buffer[size] = 0;
		for( n = 0; n < size; n++ )
		{
			if( file->mount->fsi->_read( file_file, output, 1 ) )
			{
				if( output[0] == '\n' )
				{
					output[1] = 0;
					break;
				}
				output++;
			}
			else
			{
				output[0] = 0;
				break;
			}
		}
		if( n )
			return buffer;
		return NULL;
	}
	return fgets( buffer, (int)size, file_file );
#endif
}

LOGICAL sack_existsEx ( const char *filename, struct file_system_mounted_interface *fsi )
{
	FILE *tmp;
	if( fsi && fsi->fsi && fsi->fsi->exists )
	{
		int result = fsi->fsi->exists( fsi->psvInstance, filename );
		return result;
	}
	else if( tmp = fopen( filename, "rb" ) )
	{
		fclose( tmp );
		return TRUE;
	}
	return FALSE;
}

LOGICAL sack_exists( const char * filename )
{
	struct file_system_mounted_interface *mount = (*winfile_local).mounted_file_systems;
	while( mount )
	{
		if( sack_existsEx( filename, mount ) )
		{
			(*winfile_local).last_find_mount = mount;
			return TRUE;
		}
		mount = mount->next;
	}
	return FALSE;
}

int  sack_renameEx ( CTEXTSTR file_source, CTEXTSTR new_name, struct file_system_mounted_interface *mount )
{
	int status;
	if( mount && mount->fsi )
	{
		return mount->fsi->rename( mount->psvInstance, file_source, new_name );
	}
	else
	{
		TEXTSTR tmp_src = ExpandPath( file_source );
		TEXTSTR tmp_dst = ExpandPath( new_name );
#ifdef WIN32
		status = MoveFile( tmp_src, tmp_dst );
#else
#  ifdef UNICODE
		{
			char *tmpnames = CStrDup( tmp_src );
			char *tmpnamed = CStrDup( tmp_dst );
			status = rename( tmpnames, tmpnamed );
			Deallocate( char*, tmpnames );
			Deallocate( char*, tmpnamed );
		}
#  else
		status = rename( tmp_src, tmp_dst );
#  endif
#endif
		Deallocate( TEXTSTR, tmp_src );
		Deallocate( TEXTSTR, tmp_dst );
	}
	return status;
}

int  sack_rename( CTEXTSTR file_source, CTEXTSTR new_name )
{
	return sack_renameEx( file_source, new_name, (*winfile_local).default_mount );
}

size_t GetSizeofFile( TEXTCHAR *name, uint32_t* unused )
{
	size_t size;
#ifdef __LINUX__
#  ifdef UNICODE
	char *tmpname = CStrDup( name );
	int hFile = open( tmpname,		  // open MYFILE.TXT
						  O_RDONLY );			 // open for reading
	Deallocate( char*, tmpname );
#  else
	int hFile = open( name,		  // open MYFILE.TXT
						  O_RDONLY );			 // open for reading
#  endif
	if( hFile >= 0 )
	{
		size = lseek( hFile, 0, SEEK_END );
		close( hFile );
		return size;
	}
	else
		return 0;
#else
	HANDLE hFile = CreateFile( name, 0, 0, NULL, OPEN_EXISTING, 0, NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		size = GetFileSize( hFile, (DWORD*)unused );
		if( sizeof( size ) > 4  && unused )
			size |= (uint64_t)(*unused) << 32;
		CloseHandle( hFile );
		return size;
	}
	else
		return (size_t)-1;
#endif
}

//-------------------------------------------------------------------------

uint32_t GetFileTimeAndSize( CTEXTSTR name
							, LPFILETIME lpCreationTime
							,  LPFILETIME lpLastAccessTime
							,  LPFILETIME lpLastWriteTime
							, int *IsDirectory
							)
{
	uint32_t size;
	uint32_t extra_size;
#ifdef __LINUX__
#  ifdef UNICODE
	char *tmpname = CStrDup( name );
	int hFile = open( tmpname,		  // open MYFILE.TXT
						  O_RDONLY );			 // open for reading
	Deallocate( char*, tmpname );
#  else
	int hFile = open( name,		  // open MYFILE.TXT
						  O_RDONLY );			 // open for reading
#  endif
	if( hFile >= 0 )
	{
		size = lseek( hFile, 0, SEEK_END );
		close( hFile );
		return size;
	}
	else
		return (uint32_t)-1;
#else
	HANDLE hFile = CreateFile( name, 0, 0, NULL, OPEN_EXISTING, 0, NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		size = GetFileSize( hFile, (DWORD*)&extra_size );
		GetFileTime( hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime );
		if( IsDirectory )
		{
			uint32_t dwAttr = GetFileAttributes( name );
			if( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
				(*IsDirectory) = 1;
			else
				(*IsDirectory) = 0;
		}
		CloseHandle( hFile );
		return size;
	}
	else
		return (uint32_t)-1;
#endif
}

struct file_system_interface *sack_get_filesystem_interface( CTEXTSTR name )
{
	struct file_interface_tracker *fit;
	INDEX idx;
	LIST_FORALL( (*winfile_local).file_system_interface, idx, struct file_interface_tracker *, fit )
	{
		if( StrCaseCmp( fit->name, name ) == 0 )
			return fit->fsi;
	}
	return NULL;
}

void sack_set_default_filesystem_interface( struct file_system_interface *fsi )
{
	(*winfile_local).default_file_system_interface = fsi;
}

void sack_register_filesystem_interface( CTEXTSTR name, struct file_system_interface *fsi )
{
	struct file_interface_tracker *fit = New( struct file_interface_tracker );
	fit->name = StrDup( name );
	fit->fsi = fsi;
	LocalInit();
	AddLink( &(*winfile_local).file_system_interface, fit );
}


static void * CPROC sack_filesys_open( uintptr_t psv, const char *filename );
static int CPROC sack_filesys_close( void*file ) { return fclose(  (FILE*)file ); }
static size_t CPROC sack_filesys_read( void*file, char*buf, size_t len ) { return fread( buf, 1, len, (FILE*)file ); }
static size_t CPROC sack_filesys_write( void*file, const char*buf, size_t len ) { return fwrite( buf, 1, len, (FILE*)file ); }
static size_t CPROC sack_filesys_seek( void*file, size_t pos, int whence) { return fseek( (FILE*)file, (long)pos, whence ); }
static void CPROC sack_filesys_truncate( void*file ) { sack_ftruncate( (FILE*)file ); }
static void CPROC sack_filesys_unlink( uintptr_t psv, const char*filename ) { 
#ifdef UNICODE
	TEXTCHAR *_filename = DupCStr( filename );
#  define filename _filename
#endif
	sack_unlink( 0, filename); 
#ifdef UNICODE
#  undef filename
#endif
}
static size_t CPROC sack_filesys_size( void*file ) { return sack_fsize( (FILE*)file ); }
static size_t CPROC sack_filesys_tell( void*file ) { return sack_ftell( (FILE*)file ); }
static int CPROC sack_filesys_flush( void*file ) { return sack_fflush( (FILE*)file ); }
static int CPROC sack_filesys_exists( uintptr_t psv, const char*file );

static struct file_system_interface native_fsi = {
	sack_filesys_open
		, sack_filesys_close
		, sack_filesys_read
		, sack_filesys_write
		, sack_filesys_seek
		, sack_filesys_truncate
		, sack_filesys_unlink
		, sack_filesys_size
		, sack_filesys_tell
		, sack_filesys_flush
		, sack_filesys_exists
		, NULL // same as sack_filesys_copy_write_buffer() { return FALSE; }
		, 
};

PRIORITY_PRELOAD( InitWinFileSysEarly, OSALOT_PRELOAD_PRIORITY - 1 )
{
	LocalInit();
	if( !sack_get_filesystem_interface( WIDE("native") ) )
		sack_register_filesystem_interface( WIDE("native" ), &native_fsi );
	if( !(*winfile_local).default_mount )
		(*winfile_local).default_mount = sack_mount_filesystem( "native", NULL, 1000, (uintptr_t)NULL, TRUE );
}

#ifndef __NO_OPTIONS__
PRELOAD( InitWinFileSys )
{
	(*winfile_local).flags.bLogOpenClose = SACK_GetProfileIntEx( WIDE( "SACK/filesys" ), WIDE( "Log open and close" ), (*winfile_local).flags.bLogOpenClose, TRUE );
	(*winfile_local).flags.bDeallocateClosedFiles = SACK_GetProfileIntEx( WIDE( "SACK/filesys" ), WIDE( "Deallocate closed files" ), (*winfile_local).flags.bLogOpenClose, TRUE );
}
#endif



static void * CPROC sack_filesys_open( uintptr_t psv, const char *filename ) { 
	void *result;
#ifdef UNICODE
	TEXTCHAR *_filename = DupCStr( filename );
#  define filename _filename
#endif
	result = sack_fopenEx( 0, filename, WIDE("wb+"), (*winfile_local).default_mount ); 
#ifdef UNICODE
	Deallocate( TEXTCHAR *, _filename );
#  undef filename
#endif

	return result;
}
static int CPROC sack_filesys_exists( uintptr_t psv, const char *filename ) { 
	int result;
#ifdef UNICODE
	TEXTSTR _filename = DupCStr( filename );
#define filename _filename
#endif
	result = sack_existsEx( filename, (*winfile_local).default_mount );
#ifdef UNICODE
	Deallocate( TEXTSTR, _filename );
#undef filename
#endif
	return result;
}

struct file_system_mounted_interface *sack_get_default_mount( void ) { return (*winfile_local).default_mount; }

struct file_system_mounted_interface *sack_get_mounted_filesystem( const char *name )
{
	struct file_system_mounted_interface *root = (*winfile_local).mounted_file_systems;
	while( root )
	{
		if( stricmp( root->name, name ) == 0 ) break;
		root = NextThing( root );
	}
	return root;
}

void sack_unmount_filesystem( struct file_system_mounted_interface *mount )
{
	UnlinkThing( mount );
}

struct file_system_mounted_interface *sack_mount_filesystem( const char *name, struct file_system_interface *fsi, int priority, uintptr_t psvInstance, LOGICAL writable )
{
	struct file_system_mounted_interface *root = (*winfile_local).mounted_file_systems;
	struct file_system_mounted_interface *mount = New( struct file_system_mounted_interface );
	mount->name = strdup( name );
	mount->priority = priority;
	mount->psvInstance = psvInstance;
	mount->writeable = writable;
	mount->fsi = fsi;
	//lprintf( "Create mount called %s ", name );
	if( !root || ( root->priority >= priority ) )
	{
		if( !root || root == (*winfile_local).mounted_file_systems )
		{
			LinkThing( (*winfile_local).mounted_file_systems, mount );
		}
		else
		{
			LinkThingBefore( root, mount );
		}
	}
	else while( root )
	{
		if( root->priority >= priority )
		{
			LinkThingBefore( root, mount );
			break;
		}
		if( !NextThing( root ) )
		{
			LinkThingAfter( root, mount );
			break;
		}
		root = NextThing( root );
	}
	return mount;
}

int sack_vfprintf( FILE *file_handle, const char *format, va_list args )
{
	PVARTEXT pvt = VarTextCreate();
	PTEXT output;
	struct file *file;
	file = FindFileByFILE( file_handle );

	if( file->mount && file->mount->fsi )
	{
		int r;
#ifdef UNICODE
		TEXTCHAR *_format = DupCStr( format );
#define format _format
#endif
		pvt = VarTextCreate();
		vvtprintf( pvt, format, args );
		output = VarTextGet( pvt );
#ifdef UNICODE
		Deallocate( TEXTCHAR*, _format );
#  undef format
#endif
		r = file->mount->fsi->_write( file_handle, (char*)GetText( output ), GetTextSize( output ) * sizeof( TEXTCHAR ) );
		LineRelease( output );
		return r;
	}	
	else
		return vfprintf( file_handle, format, args );
}

int sack_fprintf( FILE *file, const char *format, ... )
{
	va_list args;
	va_start( args, format );
	return sack_vfprintf( file, format, args );
}

int sack_fputs( const char *format,FILE *file )
{
	if( format )
	{
		size_t len = strlen( format );
		return sack_fwrite( format, 1, len, file );
	}
	return 0;
}



FILESYS_NAMESPACE_END
