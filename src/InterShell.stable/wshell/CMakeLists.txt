
##########################################################################3
################ Window application    ######################
##########################################################################3
set( PROJECT InterShell_window )


set(SOURCES 
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

if( __ANDROID__ )
add_definitions( -DLOAD_LIBNAME="libInterShell.core.so" -DMODE=0 )
else( __ANDROID__ )
add_definitions( -DLOAD_LIBNAME="../${COMPILED_INSTALL_PREFIX}/applicationCore/InterShell.core" -DMODE=0 )
endif( __ANDROID__ )

add_program(${PROJECT} WIN32 ${SOURCES})
SET_TARGET_PROPERTIES(${PROJECT} PROPERTIES
                  FOLDER InterShell
)

set( INSTALLED_TARGETS ${INSTALLED_TARGETS} ${PROJECT}${CMAKE_EXECUTABLE_SUFFIX} CACHE STRING  "" FORCE )

install( TARGETS ${PROJECT} RUNTIME DESTINATION bin ) 

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




set( project InterShell_window.admin  )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( SERVICE_MORE_SOURCES ../../utils/service/service.manifest )
    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
if( MSVC )
    SET( SERVICE_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )
string( REPLACE "." "_" TARGET_LABEL ${PROJECT} )

set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
	${SERVICE_MORE_SOURCES}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} WIN32 ${SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER InterShell
		COMPILE_FLAGS  "-DTARGET_LABEL=${TARGET_LABEL}" 
		LINK_FLAGS "${SERVICE_LINK_FLAGS}"

)
if( __ANDROID__ )
install( TARGETS ${project} RUNTIME DESTINATION lib
	LIBRARY DESTINATION lib) 
else( __ANDROID__ )
if( __CLR__ )
install( TARGETS ${project} RUNTIME DESTINATION . ) 
else( __CLR__ )
install( TARGETS ${project} RUNTIME DESTINATION bin ) 
endif( __CLR__ )
endif( __ANDROID__ )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
STRING ( REPLACE "/" "\\" THISDIR ${CMAKE_CURRENT_BINARY_DIR} )
STRING ( REPLACE "/" "\\" SRCDIR ${CMAKE_CURRENT_SOURCE_DIR} )
add_custom_command(TARGET
                     ${project}
                   POST_BUILD
                   COMMAND
                     "mt.exe" -manifest \"${SRCDIR}\\..\\..\\utils\\service\\service.manifest\" -outputresource:\"${THISDIR}\\${PROJECT}.exe\"\;\#1
                   COMMENT
                     "Adding custom manifest containing MSVCRT80 dependency..." 
                  )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
