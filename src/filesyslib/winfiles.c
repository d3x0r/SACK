#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#undef DeleteList
#ifdef WIN32
#include <ShlObj.h>
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
	int len;
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif
	SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, path );
	realpath = NewArray( TEXTCHAR, len = StrLen( path )
							  + StrLen( l.producer?l.producer:WIDE("") )
							  + StrLen( l.application?l.application:WIDE("") ) + 3 ); // worse case +3
	snprintf( realpath, len, WIDE("%s%s%s%s%s"), path
			  , l.producer?WIDE("/"):WIDE(""), l.producer?l.producer:WIDE("")
			  , l.application?WIDE("/"):WIDE(""), l.application?l.application:WIDE("")
			  );
	if( l.data_file_root )
		Deallocate( TEXTSTR, l.data_file_root );
	l.data_file_root = realpath;
	MakePath( l.data_file_root );
#else
	l.data_file_root = StrDup( WIDE(".") );

#endif
}

void sack_set_common_data_producer( CTEXTSTR name )
{
	l.producer = StrDup( name );
	UpdateLocalDataPath();

}

void sack_set_common_data_application( CTEXTSTR name )
{
	l.application = StrDup( name );
	UpdateLocalDataPath();
}

static void LocalInit( void )
{
	if( !winfile_local )
	{
		SimpleRegisterAndCreateGlobal( winfile_local );
		if( !l.flags.bInitialized )
		{
			InitializeCriticalSec( &l.cs_files );
			l.flags.bInitialized = 1;
			l.flags.bLogOpenClose = 0;
			{
#ifdef _WIN32
				sack_set_common_data_producer( WIDE( "Freedom Collective" ) );
				sack_set_common_data_application( GetProgramName() );

#else
				l.data_file_root = StrDup( WIDE( "~" ) );
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
	AddLink( &l.groups, group );

	// known handle '1' is the program's load path.
	group = New( struct Group );
#ifdef __ANDROID__
	// assets and other files are in the data directory
	group->base_path = StrDup( GetStartupPath() );
#else
	group->base_path = StrDup( GetProgramPath() );
#endif
	group->name = StrDup( WIDE( "Program Path" ) );
	AddLink( &l.groups, group );

	// known handle '1' is the program's start path.
	group = New( struct Group );
	group->base_path = StrDup( GetStartupPath() );
	group->name = StrDup( WIDE( "Startup Path" ) );
	AddLink( &l.groups, group );
	l.have_default = TRUE;
}

static struct Group *GetGroupFilePath( CTEXTSTR group )
{
	struct Group *filegroup;
	INDEX idx;
	if( !l.groups )
	{
		InitGroups();
	}
	LIST_FORALL( l.groups, idx, struct Group *, filegroup )
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
	if( !filegroup )
	{
		{
			TEXTCHAR tmp_ent[256];
			TEXTCHAR tmp[256];
			snprintf( tmp_ent, sizeof( tmp_ent ), WIDE( "file group/%s" ), groupname );
			//lprintf( WIDE( "option to save is %s" ), tmp );
#ifdef __NO_OPTIONS__
			tmp[0] = 0;
#else
			if( l.have_default )
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
		AddLink( &l.groups, filegroup );
	}
	return FindLink( &l.groups, filegroup );
}

TEXTSTR GetFileGroupText ( INDEX group, TEXTSTR path, int path_chars )
{
	struct Group* filegroup = (struct Group *)GetLink( &l.groups, group );
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

				snprintf( tmp, len * sizeof( TEXTCHAR ), WIDE( "%*.*s" ), (int)(end-subst_path), (int)(end-subst_path), subst_path );
				
				group = GetFileGroup( tmp, NULL );
				filegroup = (struct Group *)GetLink( &l.groups, group );
				Deallocate( TEXTCHAR*, tmp );  // must deallocate tmp

				newest_path = NewArray( TEXTCHAR, len = ( subst_path - tmp_path ) + StrLen( filegroup->base_path ) + ( this_length - ( end - tmp_path ) ) + 1 );

				//=======================================================================
				// Get rid of the ending '%' AND any '/' or '\' that might come after it
				//=======================================================================
				snprintf( newest_path, len, WIDE( "%*.*s%s/%s" ), (int)((subst_path-tmp_path)-1), (int)((subst_path-tmp_path)-1), tmp_path, filegroup->base_path,
						 ((end + 1)[0] == '/' || (end + 1)[0] == '\\') ? (end + 2) : (end + 1) );

				Deallocate( TEXTCHAR*, tmp_path );
				tmp_path = ExpandPath( newest_path );
				Deallocate( TEXTCHAR*, newest_path );
			
				if( l.flags.bLogOpenClose )
					lprintf( WIDE( "transform subst [%s]" ), tmp_path );
			}
		}
	}
	return tmp_path;
}

TEXTSTR ExpandPathEx( CTEXTSTR path, struct file_system_interface *fsi )
{
	TEXTSTR tmp_path = NULL;
	if( l.flags.bLogOpenClose )
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
				snprintf( tmp_path, len, WIDE( "%s%s%s" )
						 , here
						 , path[1]?WIDE("/"):WIDE("")
						 , path[1]?(path + 2):WIDE("") );
			}
			else if( ( path[0] == '@' ) && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = GetProgramPath();
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				snprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( ( path[0] == '*' ) && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = l.data_file_root;
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				snprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
			}
			else if( path[0] == '^' && ( ( path[1] == '/' ) || ( path[1] == '\\' ) ) )
			{
				CTEXTSTR here;
				size_t len;
				here = GetStartupPath();
				tmp_path = NewArray( TEXTCHAR, len = ( StrLen( here ) + StrLen( path ) ) );
				snprintf( tmp_path, len, WIDE( "%s/%s" ), here, path + 2 );
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
				if( l.flags.bLogOpenClose )
					lprintf( "Fix dots in [%s]", tmp_path );
				for( ofs = len+1; tmp_path[ofs]; ofs++ )
				{
					if( tmp_path[ofs] == '/' )
						tmp_path[ofs] = '.';
					if( tmp_path[ofs] == '\\' )
						tmp_path[ofs] = '.';
				}
				if( l.flags.bLogOpenClose )
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

	if( l.flags.bLogOpenClose )
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
		snprintf( tmp, sizeof( tmp ), WIDE( "file group/%s" ), group );
#ifndef __NO_OPTIONS__
		if( l.have_default )
		{
			TEXTCHAR tmp2[256];
			SACK_GetProfileString( GetProgramName(), tmp, WIDE( "" ), tmp2, sizeof( tmp2 ) );
		if( StrCaseCmp( path, tmp2 ) )
				SACK_WriteProfileString( GetProgramName(), tmp, path );
		}
#endif
		AddLink( &l.groups, filegroup );
		l.have_default = TRUE;
	}
	else
	{
		Deallocate( TEXTCHAR*, filegroup->base_path );
		filegroup->base_path = StrDup( path );
	}
	return FindLink( &l.groups, filegroup );
}


void SetDefaultFilePath( CTEXTSTR path )
{
	TEXTSTR tmp_path = NULL;
	struct Group *filegroup;
	LocalInit();
	filegroup = (struct Group *)GetLink( &l.groups, 0 );
	tmp_path = ExpandPath( path );
	if( l.groups && filegroup )
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

static TEXTSTR PrependBasePath( INDEX groupid, struct Group *group, CTEXTSTR filename )
{
	TEXTSTR real_filename = filename?ExpandPath( filename ):NULL;
	TEXTSTR fullname;

	if( l.flags.bLogOpenClose )
		lprintf( WIDE("Prepend to {%s} %p %") _size_f, real_filename, group, groupid );

	if( l.groups )
	{
		//SetDefaultFilePath( GetProgramPath() );
		if( !group )
		{
			if( groupid >= 0 )
				group = (struct Group *)GetLink( &l.groups, groupid );
		}
	}
	if( !group || ( filename && ( IsAbsolutePath( real_filename ) ) ) )
	{
		if( l.flags.bLogOpenClose )
			lprintf( WIDE("already an absolute path.  [%s]"), real_filename );
		return real_filename;
	}

	{
		TEXTSTR tmp_path;
		size_t len;
		tmp_path = ExpandPath( group->base_path );
		fullname = NewArray( TEXTCHAR, len = StrLen( filename ) + StrLen(tmp_path) + 2 );
		if( l.flags.bLogOpenClose )
			lprintf(WIDE("prepend %s[%s] with %s"), group->base_path, tmp_path, filename );
		snprintf( fullname, len * sizeof( TEXTCHAR ), WIDE("%s/%s"), tmp_path, real_filename );
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
			if( l.flags.bLogOpenClose )
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
		if( l.flags.bLogOpenClose )
			lprintf( WIDE("result %s"), fullname );
		Deallocate( TEXTCHAR*, tmp_path );
	 Deallocate( TEXTCHAR*, real_filename );
	}
	return fullname;
}

TEXTSTR sack_prepend_path( INDEX group, CTEXTSTR filename )
{
	struct Group *filegroup = (struct Group *)GetLink( &l.groups, group );
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
	EnterCriticalSec( &l.cs_files );
	LIST_FORALL( l.files, idx, struct file *, file )
	{
		if( StrCmp( file->name, filename ) == 0 )
		{
			break;
		}
	}
	LeaveCriticalSec( &l.cs_files );
	if( !file )
	{
		struct Group *filegroup = (struct Group *)GetLink( &l.groups, group );
		file = New( struct file );
		file->name = StrDup( filename );
		file->fullname = PrependBasePath( group, filegroup, filename );
		file->handles = NULL;
		file->files = NULL;
		file->group = group;
		EnterCriticalSec( &l.cs_files );
		AddLink( &l.files,file );
		LeaveCriticalSec( &l.cs_files );
	}
	if( l.flags.bLogOpenClose )
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
	if( l.flags.bLogOpenClose )
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
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "open %s %p %08x" ), file->fullname, (POINTER)handle, opts );
#endif
	if( handle == INVALID_HANDLE_VALUE )
	{
		if( l.flags.bLogOpenClose )
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
	return (HANDLE)OpenFile(filename,of,flags);
#endif
}
#endif

struct file *FindFileByHandle( HANDLE file_file )
{
	struct file *file;
	INDEX idx;
	EnterCriticalSec( &l.cs_files );
	LIST_FORALL( l.files, idx, struct file *, file )
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
	LeaveCriticalSec( &l.cs_files );
	return file;
}

LOGICAL sack_iset_eof ( INDEX file_handle )
{
	HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
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
	EnterCriticalSec( &l.cs_files );
	LIST_FORALL( l.files, idx, struct file *, file )
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
	LeaveCriticalSec( &l.cs_files );
	return file;
}

LOGICAL sack_set_eof ( HANDLE file_handle )
{
	struct file *file;
	file = FindFileByFILE( (FILE*)file_handle );
	if( file )
	{
		if( file->mount )
		{
			file->mount->fsi->truncate( (void*)file_handle );
			lprintf( WIDE("result is %d"), file->mount->fsi->size( (void*)file_handle ) );
		}
		else
		{
#ifdef _WIN32
			;
#else
			truncate( file->fullname, sack_ftell( (FILE*)file_handle ) );
#endif
		}
		return TRUE;
	}
	else
	{
		HANDLE *holder = (HANDLE*)GetLink( &l.handles, (INDEX)file_handle );
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
	HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
	HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef WIN32
	_32 length = SetFilePointer( handle // must have GENERIC_READ and/or GENERIC_WRITE
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
		if( l.flags.bLogOpenClose )
			lprintf( WIDE("Close %s"), file->fullname );
		/*
		Deallocate( TEXTCHAR*, file->name );
		Deallocate( TEXTCHAR*, file->fullname );
		Deallocate( TEXTCHAR*, file );
		DeleteLink( &l.files, file );
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
		if( l.flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open %s" ), filename );
		return INVALID_INDEX;
	}
	EnterCriticalSec( &l.cs_files );
	{
		HANDLE *holder = New( HANDLE );
		holder[0] = h;
		AddLink( &l.handles, holder );
		result = FindLink( &l.handles, holder );
	}
	LeaveCriticalSec( &l.cs_files );
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "return iopen of [%s]=%d(%")_size_f WIDE(")?" ), filename, h, result );
	return result;
}

int sack_iclose( INDEX file_handle )
{
	int result;
	EnterCriticalSec( &l.cs_files );
	{
		HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
		HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
		SetLink( &l.handles, file_handle, 0 );
		Deallocate( HANDLE*, holder );
		result = sack_close( handle );
	}
	LeaveCriticalSec( &l.cs_files );
	return result;
}

int sack_ilseek( INDEX file_handle, size_t pos, int whence )
{
	int result;
	EnterCriticalSec( &l.cs_files );
	{
		 HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
		 HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		result = SetFilePointer(handle,pos,NULL,whence);
#else
		result = lseek( handle, pos, whence );
#endif
	}
	LeaveCriticalSec( &l.cs_files );
	return result;
}

int sack_iread( INDEX file_handle, POINTER buffer, int size )
{
	EnterCriticalSec( &l.cs_files );
	{
		 HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
		 HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		DWORD dwLastReadResult;
		//lprintf( WIDE( "... %p %p" ), file_handle, h );
		LeaveCriticalSec( &l.cs_files );
		return (ReadFile( handle, (POINTER)buffer, size, &dwLastReadResult, NULL )?dwLastReadResult:-1 );
#else
		return read( handle, buffer, size );
#endif
	}
}

int sack_iwrite( INDEX file_handle, CPOINTER buffer, int size )
{
	EnterCriticalSec( &l.cs_files );
	{
		 HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
		 HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
		DWORD dwLastWrittenResult;
		LeaveCriticalSec( &l.cs_files );
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
			if( mount->fsi->exists( mount->psvInstance, filename ) )
			{
				mount->fsi->unlink( mount->psvInstance, filename );
				okay = 0;
			}
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
	return sack_unlinkEx( group, filename, l.mounted_file_systems );
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
	struct file_system_mounted_interface *test_mount;
	LocalInit();
	EnterCriticalSec( &l.cs_files );

	if( !mount )
		mount = l.mounted_file_systems;

	if( !strchr( opts, 'r' ) && !strchr( opts, '+' ) )
		while( mount )
		{  // skip roms...
			//lprintf( "check mount %p %d", mount, mount->writeable );
			if( mount->writeable )
				break;
			mount = mount->next;
		}

	if( l.flags.bLogOpenClose )
		lprintf( "open %s %p(%s) %s (%d)", filename, mount, mount->name, opts, mount?mount->writeable:1 );
	LIST_FORALL( l.files, idx, struct file *, file )
	{
		if( ( file->group == group )
			&& ( StrCmp( file->name, filename ) == 0 ) 
			&& ( file->mount == mount ) )
		{
			break;
		}
	}
	LeaveCriticalSec( &l.cs_files );

	if( !file )
	{
		TEXTSTR tmpname;
		struct Group *filegroup = (struct Group *)GetLink( &l.groups, group );
		file = New( struct file );
		memalloc = TRUE;

		file->handles = NULL;
		file->files = NULL;
		file->name = StrDup( filename );
		file->mount = mount;
		if( !file->mount->fsi && !IsAbsolutePath( filename ) )
		{
			tmpname = ExpandPath( filename );
			file->fullname = PrependBasePath( group, filegroup, tmpname );
			Deallocate( TEXTCHAR*, tmpname );
		}
		else
		{
			if( mount && group == 0 )
			{
				file->fullname = file->name;
				if( l.flags.bLogOpenClose )
					lprintf( "full is %s", file->fullname );
			}
			else
			{
				file->fullname = PrependBasePath( group, filegroup, file->name );
				if( l.flags.bLogOpenClose )
					lprintf( "full is %s %d", file->fullname, group );
			}
			//file->fullname = file->name;
		}
		file->group = group;
		EnterCriticalSec( &l.cs_files );
		AddLink( &l.files,file );
		LeaveCriticalSec( &l.cs_files );
	}
	if( StrChr( file->fullname, '%' ) )
	{
		if( memalloc )
		{
			Deallocate( TEXTCHAR*, file->name );
			Deallocate( TEXTCHAR*, file->fullname );
			Deallocate( struct file *, file );
		}
		//DebugBreak();
		return NULL;
	}
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "Open File: [%s]" ), file->fullname );

	if( mount && mount->fsi )
	{
		if( strchr( opts, 'r' ) && !strchr( opts, '+' ) )
		{
			struct file_system_mounted_interface *test_mount = mount;
			while( !handle && test_mount )
			{
				if( test_mount->fsi )
				{
					file->mount = test_mount;
					if( l.flags.bLogOpenClose )
						lprintf( "Call mount %s to check if file exists %s", test_mount->name, file->fullname );
					if( test_mount->fsi->exists( test_mount->psvInstance, file->fullname ) )
					{
						handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, file->fullname );
					}
					else if( single_mount )
						return NULL;
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
					if( l.flags.bLogOpenClose )
						lprintf( "Call mount %s to open file %s", test_mount->name, file->fullname );
					handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, file->fullname );
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
		fopen_s( &handle, file->fullname, opts );
#    endif
#  else
		handle = fopen( file->fullname, opts );
#  endif
#endif
		if( l.flags.bLogOpenClose )
			lprintf( "native opened %s", file->fullname );
	}
	if( !handle )
	{
		if( l.flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open file [%s]=[%s]" ), file->name, file->fullname );
		return NULL;
	}
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "sack_open %s (%s)" ), file->fullname, opts );
	AddLink( &file->files, handle );
	if( l.flags.bLogOpenClose )
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
	EnterCriticalSec( &l.cs_files );
	if( !mount )
		mount = l.mounted_file_systems;
	LIST_FORALL( l.files, idx, struct file *, file )
	{
		if( ( file->group == group )
			&& ( StrCmp( file->name, filename ) == 0 ) 
			&& ( file->mount == mount ) )
		{
			break;
		}
	}
	LeaveCriticalSec( &l.cs_files );
	if( !file )
	{
		struct Group *filegroup = (struct Group *)GetLink( &l.groups, group );
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
		EnterCriticalSec( &l.cs_files );
		AddLink( &l.files,file );
		LeaveCriticalSec( &l.cs_files );
	}
	if( mount )
	{
			struct file_system_mounted_interface *test_mount = mount;
			while( !handle && test_mount && test_mount->fsi )
			{
				file->mount = test_mount;
				handle = (FILE*)test_mount->fsi->open( test_mount->psvInstance, file->fullname );
				if( !handle && single_mount )
				{
					return NULL;
				}
				test_mount = test_mount->next;
			}
			//file->fsi = mount?mount->fsi:NULL;
	}
	if( !handle )
	{
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
#     ifdef UNICODE
		char *tmpname = CStrDup( file->fullname );
		char *tmpopts = CStrDup( opts );
		handle = _fsopen( tmpname, tmpopts, share_mode );
		Deallocate( char*, tmpname );
		Deallocate( char*, tmpopts );
#     else
		handle = _fsopen( file->fullname, opts, share_mode );
#     endif
#  else
		handle = fopen( file->fullname, opts );
#  endif
#endif
	}
	if( !handle )
	{
		if( l.flags.bLogOpenClose )
			lprintf( WIDE( "Failed to open file [%s]=[%s]" ), file->name, file->fullname );
		return NULL;
	}
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "sack_open %s (%s)" ), file->fullname, opts );
	EnterCriticalSec( &l.cs_files );
	AddLink( &file->files, handle );
	LeaveCriticalSec( &l.cs_files );
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "Added FILE* %p and list is %p" ), handle, file->files );
	return handle;
}

FILE*  sack_fsopen( INDEX group, CTEXTSTR filename, CTEXTSTR opts, int share_mode )
{
	return sack_fsopenEx( group, filename, opts, share_mode, NULL/*l.mounted_file_systems*/ );
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
		fseek( file_file, here, SEEK_SET );
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
	if( fseek( file_file, pos, whence ) )
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
	file = FindFileByFILE( file_file );
	if( file )
	{
		if( l.flags.bLogOpenClose )
			lprintf( WIDE("Closing %s"), file->fullname );
		DeleteLink( &file->files, file_file );
		if( l.flags.bLogOpenClose )
			lprintf( WIDE( "deleted FILE* %p and list is %p" ), file_file, file->files );
	}
	/*
	Deallocate( TEXTCHAR*, file->name );
	Deallocate( TEXTCHAR*, file->fullname );
	Deallocate( TEXTCHAR*, file );
	DeleteLink( &files, file );
	*/
	if( file->mount && file->mount->fsi )
		return file->mount->fsi->close( file_file );
	return fclose( file_file );
}
 size_t  sack_fread ( POINTER buffer, size_t size, int count,FILE *file_file )
{
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file->mount && file->mount->fsi )
		return file->mount->fsi->read( file_file, (char*)buffer, size * count );
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
			result = file->mount->fsi->write( file_file, (const char*)dupbuf, size * count );
			Deallocate( POINTER, dupbuf );
		}
		else
			result = file->mount->fsi->write( file_file, (const char*)buffer, size * count );
		return result;
	}
	return fwrite( (POINTER)buffer, size, count, file_file );
}


TEXTSTR sack_fgets ( TEXTSTR buffer, size_t size,FILE *file_file )
{
#ifdef _UNICODE
	//char *tmpbuf = NewArray( char, size+1);
	//TEXTSTR tmp_wbuf;
	fgets( buffer, size, file_file );
	//tmp_wbuf = CharWConvert( tmpbuf );
	//StrCpyEx( buffer, tmp_wbuf, size );
	return buffer;
#else
	struct file *file;
	file = FindFileByFILE( file_file );
	if( file && file->mount && file->mount->fsi )
	{
		int n;
		char *output = buffer;
		size = size-1;
		buffer[size] = 0;
		for( n = 0; n < size; n++ )
		{
			if( file->mount->fsi->read( file_file, output, 1 ) )
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
	return fgets( buffer, size, file_file );
#endif
}

LOGICAL sack_existsEx ( CTEXTSTR filename, struct file_system_mounted_interface *fsi )
{
	FILE *tmp;
	if( fsi && fsi->fsi && fsi->fsi->exists )
	{
#ifdef UNICODE
		lprintf( WIDE( "FAILED" ) );
		return fsi->fsi->exists( fsi->psvInstance, (char *)filename );
#else
		return fsi->fsi->exists( fsi->psvInstance, filename );
#endif
	}
	else if( tmp = fopen( filename, "rb" ) )
	{
		fclose( tmp );
		return TRUE;
	}
	return FALSE;
}

LOGICAL sack_exists( CTEXTSTR filename )
{
	struct file_system_mounted_interface *mount = l.mounted_file_systems;
	while( mount )
	{
		if( sack_existsEx( filename, mount ) )
		{
			l.last_find_mount = mount;
			return TRUE;
		}
		mount = mount->next;
	}
	return FALSE;
}

int  sack_rename ( CTEXTSTR file_source, CTEXTSTR new_name )
{
	int status;
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
	return status;
}


size_t GetSizeofFile( TEXTCHAR *name, P_32 unused )
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
		size = GetFileSize( hFile, unused );
		if( sizeof( size ) > 4  && unused )
			size |= (_64)(*unused) << 32;
		CloseHandle( hFile );
		return size;
	}
	else
		return (size_t)-1;
#endif
}

//-------------------------------------------------------------------------

_32 GetFileTimeAndSize( CTEXTSTR name
							, LPFILETIME lpCreationTime
							,  LPFILETIME lpLastAccessTime
							,  LPFILETIME lpLastWriteTime
							, int *IsDirectory
							)
{
	_32 size;
	_32 extra_size;
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
		return (_32)-1;
#else
	HANDLE hFile = CreateFile( name, 0, 0, NULL, OPEN_EXISTING, 0, NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		size = GetFileSize( hFile, &extra_size );
		GetFileTime( hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime );
		if( IsDirectory )
		{
			_32 dwAttr = GetFileAttributes( name );
			if( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
				(*IsDirectory) = 1;
			else
				(*IsDirectory) = 0;
		}
		CloseHandle( hFile );
		return size;
	}
	else
		return (_32)-1;
#endif
}

struct file_system_interface *sack_get_filesystem_interface( CTEXTSTR name )
{
	struct file_interface_tracker *fit;
	INDEX idx;
	LIST_FORALL( l.file_system_interface, idx, struct file_interface_tracker *, fit )
	{
		if( StrCaseCmp( fit->name, name ) == 0 )
			return fit->fsi;
	}
	return NULL;
}

void sack_set_default_filesystem_interface( struct file_system_interface *fsi )
{
	l.default_file_system_interface = fsi;
}

void sack_register_filesystem_interface( CTEXTSTR name, struct file_system_interface *fsi )
{
	struct file_interface_tracker *fit = New( struct file_interface_tracker );
	fit->name = StrDup( name );
	fit->fsi = fsi;
	LocalInit();
	AddLink( &l.file_system_interface, fit );
}


static void * CPROC sack_filesys_open( PTRSZVAL psv, const char *filename );
static int CPROC sack_filesys_close( void*file ) { return fclose(  (FILE*)file ); }
static size_t CPROC sack_filesys_read( void*file, char*buf, size_t len ) { return fread( buf, 1, len, (FILE*)file ); }
static size_t CPROC sack_filesys_write( void*file, const char*buf, size_t len ) { return fwrite( buf, 1, len, (FILE*)file ); }
static size_t CPROC sack_filesys_seek( void*file, size_t pos, int whence) { return fseek( (FILE*)file, pos, whence ); }
static void CPROC sack_filesys_truncate( void*file ) { sack_ftruncate( (FILE*)file ); }
static void CPROC sack_filesys_unlink( PTRSZVAL psv, const char*filename ) { sack_unlink( 0, filename); }
static size_t CPROC sack_filesys_size( void*file ) { return sack_fsize( (FILE*)file ); }
static size_t CPROC sack_filesys_tell( void*file ) { return sack_ftell( (FILE*)file ); }
static int CPROC sack_filesys_flush( void*file ) { return sack_fflush( (FILE*)file ); }
static int CPROC sack_filesys_exists( PTRSZVAL psv, const char*file );
//static int CPROC sack_filesys_( FILE*filename, ) { return ( ); }
//static int CPROC sack_filesys_( FILE*filename, ) { return ( ); }
//static int CPROC sack_filesys_( FILE*filename, ) { return ( ); }
//static int CPROC sack_filesys_( FILE*filename, ) { return ( ); }

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
	if( !sack_get_mounted_filesystem( "native" ) )
		sack_register_filesystem_interface( "native", &native_fsi );
	if( !l.default_mount )
		l.default_mount = sack_mount_filesystem( "native", NULL, 1000, (PTRSZVAL)NULL, TRUE );
}

#ifndef __NO_OPTIONS__
PRELOAD( InitWinFileSys )
{
	l.flags.bLogOpenClose = SACK_GetProfileIntEx( WIDE( "SACK/filesys" ), WIDE( "Log open and close" ), l.flags.bLogOpenClose, TRUE );
}
#endif



static void * CPROC sack_filesys_open( PTRSZVAL psv, const char *filename ) { return sack_fopenEx( 0, filename, "wb+", l.default_mount ); }
static int CPROC sack_filesys_exists( PTRSZVAL psv, const char *filename ) { return sack_existsEx( filename, l.default_mount ); }

struct file_system_mounted_interface *sack_get_default_mount( void ) { return l.default_mount; }

struct file_system_mounted_interface *sack_get_mounted_filesystem( const char *name )
{
	struct file_system_mounted_interface *root = l.mounted_file_systems;
	while( root )
	{
		if( StrCaseCmp( root->name, name ) == 0 ) break;
		root = NextThing( root );
	}
	return root;
}

void sack_unmount_filesystem( struct file_system_mounted_interface *mount )
{
	UnlinkThing( mount );
}

struct file_system_mounted_interface *sack_mount_filesystem( const char *name, struct file_system_interface *fsi, int priority, PTRSZVAL psvInstance, LOGICAL writable )
{
	struct file_system_mounted_interface *root = l.mounted_file_systems;
	struct file_system_mounted_interface *mount = New( struct file_system_mounted_interface );
	mount->name = SaveText( name );
	mount->priority = priority;
	mount->psvInstance = psvInstance;
	mount->writeable = writable;
	mount->fsi = fsi;
	//lprintf( "Create mount called %s ", name );
	if( !root || ( root->priority >= priority ) )
	{
		if( !root || root == l.mounted_file_systems )
		{
			LinkThing( l.mounted_file_systems, mount );
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
	file = FindFileByFILE( (FILE*)file_handle );

	if( file->mount && file->mount->fsi )
	{
		int r;
		pvt = VarTextCreate();
		vvtprintf( pvt, format, args );
		output = VarTextGet( pvt );
		r = file->mount->fsi->write( file_handle, GetText( output ), GetTextSize( output ) );
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
