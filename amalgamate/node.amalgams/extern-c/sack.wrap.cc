

#include "sack.h"

extern "C"
	PGENERICSET GetFromSetPoolEx( GENERICSET **pSetSet
													 , int setsetsize, int setunitsize, int setmaxcnt
													 , GENERICSET **pSet
										 , int setsize, int unitsize, int maxcnt DBG_PASS ) {
		return sack::containers::sets::GetFromSetPoolEx( pSetSet, setsetsize, setunitsize, setmaxcnt, pSet, setsize, unitsize, maxcnt DBG_RELAY );
	}

extern "C"
	POINTER  GetSetMemberEx( GENERICSET **pSet, INDEX nMember, int setsize, int unitsize, int maxcnt DBG_PASS ) {
		return sack::containers::sets::GetSetMemberEx( pSet, nMember, setsize, unitsize, maxcnt DBG_RELAY );
	}

extern "C"
#undef GetMemberIndex
	INDEX GetMemberIndex(GENERICSET **set, POINTER unit, int unitsize, int max ) {
      return sack::containers::sets::GetMemberIndex( set, unit, unitsize, max );
	}

extern "C"
	void DeleteFromSetExx( GENERICSET *set, POINTER unit, int unitsize, int max DBG_PASS ) {
		sack::containers::sets::DeleteFromSetExx( set, unit, unitsize, max DBG_RELAY );
	}
extern "C"
	char *  WcharConvertEx ( const wchar_t *wch DBG_PASS ) {
		return sack::containers::text::WcharConvertEx( wch DBG_RELAY );
	}


extern "C"
	POINTER ReleaseEx ( POINTER pData DBG_PASS ) {
      return sack::memory::ReleaseEx( pData DBG_RELAY );
	}


extern "C"
  int   ScanFilesEx ( CTEXTSTR base
           , CTEXTSTR mask
           , void **pInfo
           , void CPROC Process( uintptr_t psvUser, CTEXTSTR name, enum ScanFileProcessFlags flags )
           , enum ScanFileFlags flags
		   , uintptr_t psvUser, LOGICAL begin_sub_path, struct file_system_mounted_interface *mount ){
	return sack::filesys::ScanFilesEx( base, mask, pInfo, Process, flags, psvUser, begin_sub_path, mount );
}

extern "C"
  int   ScanFiles ( CTEXTSTR base
           , CTEXTSTR mask
           , void **pInfo
           , void CPROC Process( uintptr_t psvUser, CTEXTSTR name, enum ScanFileProcessFlags flags )
           , enum ScanFileFlags flags
           , uintptr_t psvUser ) {
	return sack::filesys::ScanFiles( base, mask, pInfo, Process, flags, psvUser);
}


extern "C"
	FILE* sack_fopen ( INDEX group, CTEXTSTR filename, CTEXTSTR opts ){
      return sack::filesys::sack_fopen( group, filename, opts );
	}

extern "C"
	int sack_fclose ( FILE *file_file ){
      return sack::filesys::sack_fclose( file_file );
	}

extern "C"
	size_t sack_unlink ( INDEX group, CTEXTSTR filename ){
      return sack::filesys::sack_unlink( group, filename );
	}

extern "C"
	size_t sack_rmdir ( INDEX group, CTEXTSTR filename ){
      return sack::filesys::sack_rmdir( group, filename );
	}

extern "C"
	size_t sack_mkdir ( INDEX group, CTEXTSTR filename ){
      return sack::filesys::sack_mkdir( group, filename );
	}

extern "C"
	size_t sack_fsize ( FILE *file_file ){
      return sack::filesys::sack_fsize( file_file );
	}

extern "C"
	LOGICAL sack_exists ( const char *file_file ){
      return sack::filesys::sack_exists( file_file );
	}

extern "C"
	LOGICAL sack_isPath( const char * filename ){
	return sack::filesys::sack_isPath( filename );
}

extern "C"
	LOGICAL IsPath ( CTEXTSTR path ){
      return sack::filesys::IsPath( path );
	}

extern "C"
	size_t sack_fread ( POINTER buffer, size_t size, int count,FILE *file_file ){
      return sack::filesys::sack_fread( buffer, size, count, file_file );
	}

extern "C"
	size_t sack_fwrite ( CPOINTER buffer, size_t size, int count,FILE *file_file ){
      return sack::filesys::sack_fwrite( buffer, size, count, file_file );
	}

extern "C"
	int sack_fflush ( FILE *file ){
         return sack::filesys::sack_fflush( file );
		}

extern "C"
	int sack_ftruncate ( FILE *file ){
		return sack::filesys::sack_ftruncate( file );
	}

extern "C"
	generic_function  LoadFunctionEx ( CTEXTSTR library, CTEXTSTR function DBG_PASS) {
		return sack::system::LoadFunctionEx( library, function DBG_RELAY );
	}


extern "C"
	LOGICAL IsMappedLibrary ( CTEXTSTR libname ){
	return sack::system::IsMappedLibrary( libname );
}


extern"C"
	 PLIST         AddLinkEx      ( PLIST *pList, POINTER p DBG_PASS ){
	return sack::containers::list::AddLinkEx( pList, p DBG_RELAY );
}

extern "C"
	LOGICAL       DeleteLink     ( PLIST *pList, CPOINTER value ){
	return sack::containers::list::DeleteLink( pList, value );
}

extern "C"
	RealLogFunction   _xlprintf ( uint32_t level DBG_PASS ){
	return sack::logging::_xlprintf( level DBG_RELAY );
}

extern "C"
	 void SetExternalLoadLibrary( LOGICAL (CPROC*f)(const char *) ){
	return sack::system::SetExternalLoadLibrary( f );
}

extern "C"
POINTER   HeapAllocateEx ( PMEM pHeap, uintptr_t nSize DBG_PASS )
{
	return sack::memory::HeapAllocateEx( pHeap, nSize DBG_RELAY );
}


extern "C"
	CTEXTSTR   StrStr ( CTEXTSTR s1, CTEXTSTR s2 ){
	return sack::memory::StrStr( s1, s2 );
}

extern "C"
	TEXTSTR   StrDupEx ( CTEXTSTR original DBG_PASS ){
	return sack::memory::StrDupEx( original DBG_RELAY );
}

extern "C"
	void WakeableSleepEx ( uint32_t milliseconds DBG_PASS ){
	return sack::timers::WakeableSleepEx( milliseconds DBG_RELAY );
}


extern "C"
	void   RegisterPriorityStartupProc( void(CPROC*f)(void), CTEXTSTR name,int pr,void* unused DBG_PASS){
	return sack::app::deadstart::RegisterPriorityStartupProc( f, name, pr, unused DBG_RELAY );
}
extern "C"
	void   InvokeDeadstart (void){
 	return sack::app::deadstart::InvokeDeadstart();
}
extern "C"
	CTEXTSTR   pathrchr ( CTEXTSTR path ){
 	return sack::filesys::pathrchr ( path );
}

extern "C"
	int   MakePath ( CTEXTSTR path ){
 	return sack::filesys::MakePath( path );
}
extern "C"
	TEXTSTR  ExpandPath( CTEXTSTR path ){
 	return sack::filesys::ExpandPath( path );
}

extern "C"
	struct file_system_mounted_interface *  sack_mount_filesystem( const char *name, struct file_system_interface *i, int priority, uintptr_t psvInstance, LOGICAL writable ){
	return sack::filesys:: sack_mount_filesystem ( name, i, priority, psvInstance, writable );
}

extern "C" 
	struct file_system_mounted_interface *  sack_get_default_mount( void ){
 	return sack::filesys::sack_get_default_mount ();
}

extern "C"
	FILE*   sack_fopenEx( INDEX group, CTEXTSTR filename, CTEXTSTR opts, struct file_system_mounted_interface *fsi ){
 	return sack::filesys::sack_fopenEx( group, filename, opts, fsi );
}

extern "C"
	struct file_system_interface * FILESYS_API sack_get_filesystem_interface( CTEXTSTR name ){
 	return sack::filesys::sack_get_filesystem_interface ( name );
}

extern "C"
	LOGICAL   sack_existsEx ( const char * filename, struct file_system_mounted_interface *mount ){
 	return sack::filesys::sack_existsEx ( filename, mount );
}

extern "C"
	void  sack_set_common_data_producer( CTEXTSTR name ){
 	sack::filesys::sack_set_common_data_producer ( name );
}

extern "C"
	 struct sack_vfs_volume *  sack_vfs_load_volume( CTEXTSTR filepath ){
 	return sack::SACK_VFS::sack_vfs_load_volume( filepath );
}
extern "C"
POINTER OpenSpace ( CTEXTSTR pWhat, CTEXTSTR pWhere, size_t *dwSize ){
	return sack::memory::OpenSpace( pWhat, pWhere, dwSize );
}

extern "C"
struct sack_vfs_volume * sack_vfs_load_crypt_volume( CTEXTSTR filepath, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey ){
	return sack::SACK_VFS::sack_vfs_load_crypt_volume( filepath, version, userkey, devkey );
}
extern "C"
struct sack_vfs_volume * sack_vfs_use_crypt_volume( POINTER filemem, size_t size, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey ){
	return sack::SACK_VFS::sack_vfs_use_crypt_volume( filemem, size, version, userkey, devkey );
}
