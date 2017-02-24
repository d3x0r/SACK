
if(MSVC)
# We statically link to reduce dependencies
foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
    if(${flag_var} MATCHES "/MDd")
        string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MDd")
endforeach(flag_var)
endif(MSVC)

set( PROJECT sack_vfs_runner )

set(BASE_SOURCES
    vfs_runner.c memory_dll_loader.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

if( 0 )
set( PROJECT sack_vfs_run_builder )

set(BASE_SOURCES
    vfs_run_builder.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

endif( 0 )


if( 0 )
set( PROJECT sack_vfs_run_builder )

set(BASE_SOURCES
    vfs_run_builder.c
)

sack_add_executable( ${PROJECT} ${BASE_SOURCES} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/vfs
)

target_link_libraries( ${PROJECT} sack_vfs.module ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

endif( 0 )

add_subdirectory( portable )
