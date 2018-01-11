cmake_minimum_required(VERSION 2.8)

if( WIN32 )

set( project test_atexit_threads )

set(BASE_SOURCES
    test_atexit_threads.c
)

add_program(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${project} )


set( project test_atexit_threads2 )

set(BASE_SOURCES
    test_atexit_threads2.c
)

add_library(${project} SHARED ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${project} )



set( project test_atexit_threads3 )

set(BASE_SOURCES
    test_atexit_threads3.c
)

add_program(${project} ${BASE_SOURCES} )
target_link_libraries( ${project} test_atexit_threads2 )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${project} )

set( project test_atexit_threads4 )

set(BASE_SOURCES
    test_atexit_threads4.c
)

add_library(${project} SHARED ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
#install_default_dest( ${project} )



set( project test_atexit_threads5 )

set(BASE_SOURCES
    test_atexit_threads5.c
)

add_program(${project} ${BASE_SOURCES} )
target_link_libraries( ${project} test_atexit_threads4 )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
#install_default_dest( ${project} )

endif( WIN32 )

set( project test_set_eof )

set(BASE_SOURCES
    test_set_eof.c
)

add_program(${project} ${BASE_SOURCES} )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )

if( NOT __NO_NETWORK__ )
set( project test_accept )
set(BASE_SOURCES
    test_accept.c
)

add_program(${project} ${BASE_SOURCES} )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )
endif( NOT __NO_NETWORK__ )


set( project test_option_database_close )
set(BASE_SOURCES
    test_option_database_close.c
)

add_program(${project} ${BASE_SOURCES} )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )


set( project test_openspace_zero )
add_program(${project} test_openspace_zero.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )



set( project test_spawn_task )
add_program(${project} test_spawn_task.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )


set( project test_url_parse )
add_program(${project} test_url_parse.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )

set( project test_wakeable_sleeps )
add_program( ${project} test_wakeable_sleep.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )

set( project test_connect_from )
add_program( ${project} test_connect_from.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )

set( project test_critsec )
add_program( ${project} test_critsec.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )

set( project test_critsec2 )
add_program( ${project} test_critsec2.c )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests )
install_default_dest( ${project} )

