

##########################################################################3
################ Console ######################
##########################################################################3

set( project InterShell_console  )

set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

  if( __ANDROID__ )
    add_definitions( 
	-DLOAD_LIBNAME="libInterShell.core.so" -DMODE=1
     )
  else( __ANDROID__ )

    add_definitions( 
	-DLOAD_LIBNAME="../${COMPILED_INSTALL_PREFIX}/applicationCore/InterShell.core" -DMODE=1
    )
  endif( __ANDROID__ )

add_program(${project} ${SOURCES} )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER InterShell
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




set( project InterShell_console.admin  )

if( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
    SET( SERVICE_MORE_SOURCES ../../utils/service/service.manifest )
    #SET( EXTRACTER_LINK_FLAGS "/MANIFEST:NO" )
endif( WIN32 AND CMAKE_COMPILER_IS_GNUCC )
if( MSVC )
    SET( SERVICE_LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'" )
endif( MSVC )

set(SOURCES
  ${FIRST_GCC_PROGRAM_SOURCE}
	${SERVICE_MORE_SOURCES}
   ../../utils/runner/runwin.c 
  ${LAST_GCC_PROGRAM_SOURCE}
)

add_program(${project} ${SOURCES} )
string( REPLACE "." "_" TARGET_LABEL ${project} )
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
