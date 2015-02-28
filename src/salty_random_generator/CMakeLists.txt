

if( WATCOM )
set (CMAKE_C_STANDARD_LIBRARIES_INIT "library clbr3r.lib library plbr3r.lib")
ADD_DEFINITIONS( -s )
endif( WATCOM )

if( 0 )
set( PROJECT salty_generator )

sack_add_library( ${PROJECT} SHARED salty_generator.c block_shuffle.c )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/salty_generator
)

my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )

endif( 0 )

if( BUILD_TESTS )

set( PROJECT test_salty_generator )

add_program( ${PROJECT}  test_salty_generator.c )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/salty_generator
)

if( ANDROID )
  my_target_link_libraries( ${PROJECT}.code ${SACK_LIBRARIES} )#salty_generator)
else( ANDROID )
  my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )#salty_generator)
endif( ANDROID )
install_mode_dest( ${PROJECT} )
endif( BUILD_TESTS )
