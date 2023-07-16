
project( InterShell.Service  )


set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_definitions( 
	-DLOAD_LIBNAME="@/${COMPILED_INSTALL_PREFIX}/SACK/applicationCore/InterShell.core" -DMODE=1 -DCONSOLE_SHELL
)

add_executable_force_source(${PROJECT_NAME} ${SOURCES} )
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
                  COMPILE_DEFINITIONS "TARGET_LABEL=Intershell_service;BUILD_SERVICE;BUILD_SERVICE_THREAD"
                  FOLDER InterShell
)
target_link_libraries( ${PROJECT_NAME} ${SACK_LIBRARIES} )
set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin ) 

