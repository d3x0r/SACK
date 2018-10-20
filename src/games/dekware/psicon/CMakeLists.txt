
add_subdirectory( concore )


set( LibraryExtraFlags "-DPSICON" )
add_plugin( psicon.nex devices.c psiconlink.c psicon.c )

my_target_link_libraries( psicon.nex concore )

if( WIN32 )
	
set( LibraryExtraFlags "-DWINCON" )
add_plugin( wincon.nex devices.c psiconlink.c wincon.c regaccess.c wincon.nex.rc )
my_target_link_libraries( wincon.nex concore )

endif( WIN32 )
