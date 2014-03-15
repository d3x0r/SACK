
##########################################################################3
################ Window application    ######################
##########################################################################3
set( PROJECT InterShell_window )


set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( __CLR__ )
add_definitions( -DLOAD_LIBNAME="InterShell.core.dll" -DMODE=0 )
elseif( __ANDROID__ )
add_definitions( -DLOAD_LIBNAME="libInterShell.core.so" -DMODE=0 )
else( __CLR__ )
add_definitions( -DLOAD_LIBNAME="InterShell.core" -DMODE=0 )
endif( __CLR__ )

add_program(${PROJECT} WIN32 ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER InterShell
)

set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )
if( __CLR__ )
install( TARGETS ${PROJECT} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT} RUNTIME DESTINATION bin ) 
endif( __CLR__ )

##########################################################################3
################ Service ######################
##########################################################################3

if( NOT __ANDROID__ )

set( PROJECT InterShell_window.service )

set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_definitions( -DLOAD_LIBNAME="InterShell.core" -DMODE=0 )

add_executable_force_source(${PROJECT} WIN32 ${SOURCES})
string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )

SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  COMPILE_DEFINITIONS "BUILD_SERVICE"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT}  ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )

if( __CLR__ )
install( TARGETS ${PROJECT} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT} RUNTIME DESTINATION bin ) 
endif( __CLR__ )

endif( NOT __ANDROID__ )
