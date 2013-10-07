
if( BUILD_TESTS )
set( project test.clock )
else( BUILD_TESTS )
set( project analog_clock )
endif( BUILD_TESTS )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
    testclock.c
    ${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${BASE_SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  COMPILE_DEFINITIONS  "TARGET_LABEL=${TARGET_LABEL}"
                  FOLDER tests/PSI
)
target_link_libraries( ${project} ${SACK_LIBRARIES})
install_mode_dest( ${project} )

install( FILES clock.png clock_face_1.png DESTINATION bin/images )
install( FILES ${project}.interface.conf DESTINATION bin )
