cmake_minimum_required(VERSION 2.8)

project( set_config )

set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   set_config.c
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source(${PROJECT_NAME} ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}"
                  FOLDER InterShell/Utility
)
target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS}${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} )
if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )


