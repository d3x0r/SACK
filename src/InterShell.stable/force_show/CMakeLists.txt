
if( WIN32 )
project( InterShell_window.show )


set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../force_canvas_show.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source(${PROJECT_NAME} ${SOURCES})
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} winmm )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )
if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )
endif( WIN32 )

project( InterShell_console.show )


set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../force_canvas_show.c
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_executable_force_source(${PROJECT_NAME} ${SOURCES})
string( REPLACE "." "_" TARGET_LABEL ${PROJECT_NAME} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}"
                  FOLDER InterShell
)
if( WIN32 )
set( WINMM winmm )
endif( WIN32 )
target_link_libraries( ${PROJECT_NAME}  ${SACK_LIBRARIES} ${WINMM} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )
if( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 
endif( __CLR__ )

