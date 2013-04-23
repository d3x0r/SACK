


cmake_minimum_required(VERSION 2.8)

project( InterShell_window )


set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( __CLR__ )
add_definitions( -DLOAD_LIBNAME="InterShell.core.dll" -DMODE=0 )
else( __CLR__ )
add_definitions( -DLOAD_LIBNAME="InterShell.core" -DMODE=0 )
endif( __CLR__ )

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${PROJECT_NAME}"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )
if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )


project( InterShell_window.service )

set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_definitions( -DLOAD_LIBNAME="InterShell.core" -DMODE=0 )

add_executable_force_source(${PROJECT_NAME} WIN32 ${SOURCES})
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  COMPILE_DEFINITIONS "BUILD_SERVICE"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )

if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )
