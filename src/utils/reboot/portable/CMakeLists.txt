
foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        )
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
   if(${flag_var} MATCHES "-br")
      string(REGEX REPLACE "-br" "" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "-br")
endforeach(flag_var)

if( WATCOM )
string( REGEX REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
string( REGEX REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}" )
endif( WATCOM )

set( project rebootnow.portable )


macro( build_resource target rcfile realfile ) 
if( WATCOM )
  if( NOT TARGET generate_foo${rcfile} )
    string( REPLACE ".rc" ".res" rcfile_out ${realfile} )
    add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/${rcfile_out}
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${realfile}
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}\\.. -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${realfile}
                     COMMAND ${CMAKE_RC_COMPILER} -i${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}\\.. -fo${PROJECT_BINARY_DIR}\\${rcfile_out} ${CMAKE_CURRENT_SOURCE_DIR}\\${realfile}
 )
    ADD_CUSTOM_TARGET( generate_foo${rcfile} DEPENDS ${PROJECT_BINARY_DIR}/${rcfile_out})
  endif( NOT TARGET generate_foo${rcfile} )
  ADD_DEPENDENCIES( ${target} generate_foo${rcfile} )
  SET_TARGET_PROPERTIES(${target} PROPERTIES LINK_FLAGS "option resource=${PROJECT_BINARY_DIR}\\${rcfile_out}")
else( WATCOM )
endif( WATCOM )
endmacro( build_resource ) 

set( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ )
include( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../CMakeSources.lst )

set( BASE_SOURCES ../reboot.c )



set( NO_AUTO_DEADSTART_CORE ON )
add_portable_program(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;DEDICATE_REBOOT" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)



set( project reboot.monitor.portable )

set(BASE_SOURCES
    ${FIRST_GCC_PROGRAM_SOURCE}
	../reboot.c
	../reboot.rc
    ${NETWORK_SOURCES}
    ${TYPE_LIBRARY_SOURCES}
    ${UNSORTED_SOURCES}
    ${SYSTEM_LIBRARY_SOURCES}
    ${ExtraSources}
    ${FILESYSTEM_SOURCES}
    ${SYSTRAY_SOURCES}
    ${LAST_GCC_PROGRAM_SOURCE}
)

set( NO_AUTO_DEADSTART_CORE ON )
add_portable_program(${project} WIN32 ${BASE_SOURCES} )
build_resource( ${project} portable.reboot.rc ../reboot.rc )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)



