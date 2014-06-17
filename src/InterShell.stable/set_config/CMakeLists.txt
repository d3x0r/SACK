cmake_minimum_required(VERSION 2.8)

set( PROJECT set_config )

set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   set_config.c
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${PROJECT} ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT}"
                  FOLDER InterShell/Utility
)
target_link_libraries( ${PROJECT}  ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )

if( NOT __ANDROID__ )
  if( __CLR__ )
    install( TARGETS ${PROJECT} RUNTIME DESTINATION . ) 
  else( __CLR__ )
    install( TARGETS ${PROJECT} RUNTIME DESTINATION bin ) 
  endif( __CLR__ )
endif( NOT __ANDROID__ )


