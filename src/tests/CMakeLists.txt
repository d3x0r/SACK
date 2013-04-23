cmake_minimum_required(VERSION 2.8)

if( WIN32 )

project( test_atexit_threads )

set(BASE_SOURCES
    test_atexit_threads.c
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${PROJECT_NAME} )


project( test_atexit_threads2 )

set(BASE_SOURCES
    test_atexit_threads2.c
)

add_library(${PROJECT_NAME} SHARED ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${PROJECT_NAME} )



project( test_atexit_threads3 )

set(BASE_SOURCES
    test_atexit_threads3.c
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} test_atexit_threads2 )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${PROJECT_NAME} )

project( test_atexit_threads4 )

set(BASE_SOURCES
    test_atexit_threads4.c
)

add_library(${PROJECT_NAME} SHARED ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${PROJECT_NAME} )



project( test_atexit_threads5 )

set(BASE_SOURCES
    test_atexit_threads5.c
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} test_atexit_threads4 )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${PROJECT_NAME} )

endif( WIN32 )

project( test_set_eof )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    test_set_eof.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
                  COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
)

project( test_accept )
set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    test_accept.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
                  COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
)
install_mode_dest( ${PROJECT_NAME} )


project( test_option_database_close )
set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    test_option_database_close.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
                  COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
)
install_mode_dest( ${PROJECT_NAME} )


project( test_openspace_zero )
set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    test_openspace_zero.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
                  COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
)
install_mode_dest( ${PROJECT_NAME} )



project( test_spawn_task )
set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE}
    test_spawn_task.c
${LAST_GCC_PROGRAM_SOURCE}
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
                  COMPILE_DEFINITIONS "TARGET_LABEL=${PROJECT_NAME}" 
)
install_mode_dest( ${PROJECT_NAME} )
