
#include <stdhdrs.h>
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
};

struct Group {
    TEXTSTR name;
    TEXTSTR base_path;
};

static struct winfile_local_tag {
    CRITICALSECTION cs_files;
    PLIST files;
    PLIST groups;
    PLIST handles;
    LOGICAL have_default;
    struct {
        BIT_FIELD bLogOpenClose : 1;
        BIT_FIELD bInitialized : 1;
    } flags;
} *winfile_local;

#define l (*winfile_local)

PRIORITY_UNLOAD( LowLevelInit, SYSLOG_PRELOAD_PRIORITY-1 )
{
	Deallocate( POINTER, winfile_local );
}

static void LocalInit( void )
{
	if( !winfile_local )
	{
		SimpleRegisterAndCreateGlobal( winfile_local );
		InitializeCriticalSec( &l.cs_files );
		l.flags.bInitialized = 1;
		l.flags.bLogOpenClose = 0;
	}
}

PRIORITY_PRELOAD( InitWinFileSysEarly, OSALOT_PRELOAD_PRIORITY - 1 )
{
	LocalInit();
}
#ifndef __NO_OPTIONS__
PRELOAD( InitWinFileSys )
{
    LocalInit();
    l.flags.bLogOpenClose = SACK_GetProfileIntEx( WIDE( "SACK/filesys" ), WIDE( "Log open and close" ), l.flags.bLogOpenClose, TRUE );
}
#endif

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
                Release( tmp );  // must deallocate tmp

                newest_path = NewArray( TEXTCHAR, len = ( subst_path - tmp_path ) + StrLen( filegroup->base_path ) + ( this_length - ( end - tmp_path ) ) + 1 );

                //=======================================================================
                // Get rid of the ending '%' AND any '/' or '\' that might come after it
                //=======================================================================
                snprintf( newest_path, len, WIDE( "%*.*s%s/%s" ), (int)((subst_path-tmp_path)-1), (int)((subst_path-tmp_path)-1), tmp_path, filegroup->base_path,
                          ((end + 1)[0] == '/' || (end + 1)[0] == '\\') ? (end + 2) : (end + 1) );

                Release( tmp_path );
                tmp_path = ExpandPath( newest_path );
                Release( newest_path );
            
                if( l.flags.bLogOpenClose )
                    lprintf( WIDE( "transform subst [%s]" ), tmp_path );
            }
        }
    }
    return tmp_path;
}

TEXTSTR ExpandPath( CTEXTSTR path )
{
	TEXTSTR tmp_path = NULL;
	if( l.flags.bLogOpenClose )
		lprintf( WIDE( "input path is [%s]" ), path );

	if( path )
	{
		if( !IsAbsolutePath( path ) )
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
        Release( (POINTER)filegroup->base_path );
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
        Release( (POINTER)filegroup->base_path );
        filegroup->base_path = StrDup( tmp_path?tmp_path:path );
    }
    else
    {
        SetGroupFilePath( WIDE( "Default" ), tmp_path?tmp_path:path );
    }
    if( tmp_path )
        Release( tmp_path );
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

			if( l.flags.bLogOpenClose )
				lprintf( WIDE("Fix dots in [%s]"), fullname );
			for( ofs = len+1; fullname[ofs]; ofs++ )
			{
				if( fullname[ofs] == '/' )
					fullname[ofs] = '.';
				if( fullname[ofs] == '\\' )
					fullname[ofs] = '.';
			}
		}
#endif
		if( l.flags.bLogOpenClose )
			lprintf( WIDE("result %s"), fullname );
		Release( tmp_path );
      Release( real_filename );
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
    handle = open( file->fullname, opts );
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

LOGICAL sack_set_eof ( HANDLE file_handle )
{
	HANDLE *holder = (HANDLE*)GetLink( &l.handles, (INDEX)file_handle );
	HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef _WIN32
	return SetEndOfFile( handle );
#else
	return ftruncate( handle, lseek( handle, 0, SEEK_CUR ) );
#endif

}


long sack_tell( INDEX file_handle )
{
	HANDLE *holder = (HANDLE*)GetLink( &l.handles, file_handle );
	HANDLE handle = holder?holder[0]:INVALID_HANDLE_VALUE;
#ifdef WIN32
	_32 length = SetFilePointer( handle // must have GENERIC_READ and/or GENERIC_WRITE
								, 0     // do not move pointer 
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
         Release( file->name );
         Release( file->fullname );
         Release( file );
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



int sack_unlink( INDEX group, CTEXTSTR filename )
{
#ifdef __LINUX__
    int okay;
    TEXTSTR tmp = PrependBasePath( group, NULL, filename );
    okay = unlink( filename );
    Release( tmp );
    return !okay; // unlink returns TRUE is 0, else error...
#else
    int okay;
	 TEXTSTR tmp = PrependBasePath( group, NULL, filename );
    okay = DeleteFile(tmp);
    Release( tmp );
    return !okay; // unlink returns TRUE is 0, else error...
#endif
}

int sack_rmdir( INDEX group, CTEXTSTR filename )
{
#ifdef __LINUX__
    int okay;
    TEXTSTR tmp = PrependBasePath( group, NULL, filename );
    okay = rmdir( filename );
    Release( tmp );
    return !okay; // unlink returns TRUE is 0, else error...
#else
    int okay;
    TEXTSTR tmp = PrependBasePath( group, NULL, filename );
    okay = RemoveDirectory(tmp);
    Release( tmp );
    return !okay; // unlink returns TRUE is 0, else error...
#endif
}

struct file *FindFileByFILE( FILE *file_file )
{
    struct file *file;
    INDEX idx;
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

#undef fopen
FILE*  sack_fopen ( INDEX group, CTEXTSTR filename, CTEXTSTR opts )
{
    FILE *handle;
    struct file *file;
    INDEX idx;
    LOGICAL memalloc = FALSE;

    LocalInit();
    EnterCriticalSec( &l.cs_files );
    LIST_FORALL( l.files, idx, struct file *, file )
    {
        if( ( file->group == group )
            && ( StrCmp( file->name, filename ) == 0 ) )
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
		  tmpname = ExpandPath( filename );
		  if( !IsAbsolutePath( tmpname ) )
		  {
			  file->fullname = PrependBasePath( group, filegroup, tmpname );
           Release( tmpname );
		  }
		  else
           file->fullname = tmpname;
        file->group = group;
        EnterCriticalSec( &l.cs_files );
        AddLink( &l.files,file );
        LeaveCriticalSec( &l.cs_files );
    }
    if( StrChr( file->fullname, '%' ) )
    {
        if( memalloc )
        {
            Release( file->name );
            Release( file->fullname );
            Release( file );
        }
        //DebugBreak();
        return NULL;
    }
    if( l.flags.bLogOpenClose )
        lprintf( WIDE( "Open File: [%s]" ), file->fullname );

#ifdef UNICODE
    handle = _wfopen( file->fullname, opts );
#else
#ifdef _STRSAFE_H_INCLUDED_
    fopen_s( &handle, file->fullname, opts );
#else
    handle = fopen( file->fullname, opts );
#endif
#endif
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

FILE*  sack_fsopen( INDEX group, CTEXTSTR filename, CTEXTSTR opts, int share_mode )
{
    FILE *handle;
    struct file *file;
    INDEX idx;
    LocalInit();
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
        file->handles = NULL;
        file->files = NULL;
        file->name = StrDup( filename );
        file->group = group;
        file->fullname = PrependBasePath( group, filegroup, filename );
        EnterCriticalSec( &l.cs_files );
        AddLink( &l.files,file );
        LeaveCriticalSec( &l.cs_files );
    }

#ifdef UNICODE
    handle = _wfopen( file->fullname, opts );
#else
#ifdef _STRSAFE_H_INCLUDED_
    handle = _fsopen( file->fullname, opts, share_mode );
#else
    handle = fopen( file->fullname, opts );
#endif
#endif
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

int  sack_fseek ( FILE *file_file, int pos, int whence )
{
    if( fseek( file_file, pos, whence ) )
		return -1;
    //struct file *file = FindFileByFILE( file_file );
	return ftell( file_file );
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
    Release( file->name );
	Release( file->fullname );
    Release( file );
    DeleteLink( &files, file );
   */
	return fclose( file_file );
}
 size_t  sack_fread ( POINTER buffer, size_t size, int count,FILE *file_file )
{
	return fread( buffer, size, count, file_file );
}
 size_t  sack_fwrite ( CPOINTER buffer, size_t size, int count,FILE *file_file )
{
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
    return fgets( buffer, size, file_file );
#endif
}


int  sack_rename ( CTEXTSTR file_source, CTEXTSTR new_name )
{
	int status;
	TEXTSTR tmp_src = ExpandPath( file_source );
	TEXTSTR tmp_dst = ExpandPath( new_name );
#ifdef WIN32
	status = MoveFile( tmp_src, tmp_dst );
#else
	status = rename( tmp_src, tmp_dst );
#endif
	Release( tmp_src );
	Release( tmp_dst );
	return status;
}


_32 GetSizeofFile( TEXTCHAR *name, P_32 unused )
{
    _32 size;
#ifdef __LINUX__
    int hFile = open( name,           // open MYFILE.TXT
                              O_RDONLY );              // open for reading
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
        CloseHandle( hFile );
        return size;
    }
    else
		return (_32)-1;
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
    int hFile = open( name,           // open MYFILE.TXT
                              O_RDONLY );              // open for reading
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



FILESYS_NAMESPACE_END
