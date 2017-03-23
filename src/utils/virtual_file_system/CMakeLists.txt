if( NOT __CLR__ )

#############  vfs #####################

set( PROJECT sack_vfs.module )

set(BASE_SOURCES
    vfs.c
)

sack_add_library( ${PROJECT} SHARED ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)
if( NOT __ANDROID__ )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
  )
endif( NOT __ANDROID__ )

target_link_libraries( ${PROJECT} ${SACK_CORE_LIBRARY} )
install_mode_dest( ${PROJECT} )

#############  vfs++ #####################


set( PROJECT sack_vfs.pp.module )
set( VFSPP ${CMAKE_BINARY_DIR}/src/utils/virtual_file_system/vfs.cc )
set( VFSPPH ${CMAKE_BINARY_DIR}/src/utils/virtual_file_system/vfs_internal.h )

       add_custom_command( OUTPUT ${VFSPP}
                           DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vfs.c
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vfs.c ${VFSPP}
                           COMMAND ${CMAKE_COMMAND} -E touch ${VFSPP}
                           )
       add_custom_command( OUTPUT ${VFSPPH}
                           DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vfs_internal.h
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/vfs_internal.h ${VFSPPH}
                           COMMAND ${CMAKE_COMMAND} -E touch ${VFSPPH}
                           )


sack_add_library( ${PROJECT} SHARED ${VFSPP} ${VFSPPH} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils
)
if( NOT __ANDROID__ )
  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  SUFFIX ""
                  PREFIX ""
  )
endif( NOT __ANDROID__ )

target_link_libraries( ${PROJECT} ${SACK_CORE_LIBRARY_PLUSPLUS} ${SACK_CORE_LIBRARY})
install_mode_dest( ${PROJECT} )


#############  TESTS #####################

if( BUILD_TESTS ) 

set( PROJECT sack_vfs.test )

set(BASE_SOURCES
    vfs_tester.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/tests
)

target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} sack_vfs.module )
install_mode_dest( ${PROJECT} )

if( NEED_FUSE OR FUSE_FOUND )
	sack_add_executable( sack_vfs.fuse.test fuse_private.c )
	target_link_libraries( sack_vfs.fuse.test ${SACK_LIBRARIES} ${FUSE_LIBRARIES} sack_vfs.module )
	install_mode_dest( sack_vfs.fuse.test )
        #SET_PROPERTY( sack_vfs.fuse.test APPEND PROPERTY 
        #	COMPILE_DEFINITIONS "
	SET_TARGET_PROPERTIES(sack_vfs.fuse.test PROPERTIES
                  
                  FOLDER utils/tests
	)
endif( NEED_FUSE OR FUSE_FOUND )

endif( BUILD_TESTS ) 

#############  EXTRAS #####################

# these became not so 'extra'
#if( BUILD_EXTRAS )

add_subdirectory( vfs_command )
add_subdirectory( vfs_extract )

# some assembly required...
#if( WIN32 )
   add_subdirectory( vfs_runner )
   add_subdirectory( vfs_node_runner )
#endif( WIN32 )

#endif( BUILD_EXTRAS )
endif( NOT __CLR__ )
