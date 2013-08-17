
add_subdirectory( concore )


set( LibraryExtraFlags "-DPSICON" )
add_plugin( psicon.nex devices psiconlink psicon  )

my_target_link_libraries( psicon.nex concore )

if( WIN32 )

set( LibraryExtraFlags "-DWINCON" )
add_plugin( wincon.nex devices psiconlink wincon regaccess )
my_target_link_libraries( wincon.nex concore )

endif( WIN32 )
