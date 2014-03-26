
set (CMAKE_C_STANDARD_LIBRARIES_INIT "library clbr3r.lib library plbr3r.lib")

if( WATCOM )
ADD_DEFINITIONS( -s )
endif( WATCOM )

set( PROJECT salty_generator )

sack_add_library( ${PROJECT} SHARED salty_generator.c block_shuffle.c )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/salty_generator
)
if( NOT __ANDROID__ )
#  SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
#                  SUFFIX ""
#                  PREFIX ""
#  )
endif( NOT __ANDROID__ )

my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} )
install_mode_dest( ${PROJECT} )


set( PROJECT test_salty_generator )

add_program( ${PROJECT}  test_salty_generator.c )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER utils/salty_generator
)

my_target_link_libraries( ${PROJECT} ${SACK_LIBRARIES} salty_generator)
install_mode_dest( ${PROJECT} )
