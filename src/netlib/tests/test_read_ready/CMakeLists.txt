
set( project test_read_ready )

set(BASE_SOURCES
${FIRST_GCC_PROGRAM_SOURCE} 
    test.c
${LAST_GCC_PROGRAM_SOURCE}
)

sack_add_executable(${project} ${BASE_SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES FOLDER tests/network )
target_link_libraries( ${project} ${SACK_LIBRARIES} )
install_default_dest( ${project} )
