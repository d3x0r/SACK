cmake_minimum_required(VERSION 2.8)


project( test_msvc_memory_exit )

set(BASE_SOURCES
    test_msvc_memory_exit.c
)

add_executable(${PROJECT_NAME} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  FOLDER tests
)
#install_default_dest( ${PROJECT_NAME} )

