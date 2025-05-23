cmake_minimum_required(VERSION 3.0)

PROJECT( reboot.portable )

if( PLATFORM_CMAKE_DEFINES )
   STRING( REPLACE "," ";" PLATFORM_CMAKE_DEFINES ${PLATFORM_CMAKE_DEFINES} )
   foreach( DEF IN LISTS PLATFORM_CMAKE_DEFINES)
       STRING( REPLACE " " ";" SETDEF ${DEF} )
       set( ${SETDEF} )
   endforeach()
endif( PLATFORM_CMAKE_DEFINES )

include( ${SACK_BASE}/share/SACK/build/DefaultInstall.cmake )
include_directories( ${SACK_BASE}/include/SACK )
STRING( REPLACE " " ";" PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
STRING( REPLACE " " ";" PLATFORM_DEFINES ${PLATFORM_DEFINES} )
add_definitions( ${PLATFORM_DEFINES} )

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
set( NO_SSL ON )
set( NO_SSH ON )
include( ${CMAKE_CURRENT_SOURCE_DIR}/../../../../CMakeSources.lst )

set( BASE_SOURCES ../reboot.c )



set( NO_AUTO_DEADSTART_CORE ON )
add_portable_program(${project} ${BASE_SOURCES} )
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBAL__;DEDICATE_REBOOT" )
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
    ${JSON_SOURCES}
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
SET_PROPERTY(TARGET ${project} APPEND PROPERTY COMPILE_DEFINITIONS "__STATIC__;__STATIC_GLOBALS__;SACK_BAG_EXPORTS;__NO_OPTIONS__;__NO_DEFAULT_INTERFACES__;NO_SSL" )
SET_TARGET_PROPERTIES(${project} PROPERTIES
                  FOLDER utils
)



