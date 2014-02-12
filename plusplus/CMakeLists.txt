
SET( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../ )
include( ../CMakeSources.lst )


if( WATCOM )
  set( SACK_BAG_PLUSPLUS sack_bag_pp )
  set( BAG_PLUSPLUS bag_pp )
  set( BAG_PSI_PLUSPLUS bag.psi_pp )
else(WATCOM)
  if( NOT __CLR__ )
    set( SACK_BAG_PLUSPLUS sack_bag++ )
    set( BAG_PLUSPLUS bag++ )
    set( BAG_PSI_PLUSPLUS bag.psi++ )
  endif( NOT __CLR__ )
endif(WATCOM)


EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${SOURCES_ROOT}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp )
EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${SOURCES_ROOT}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp )
SET_PROPERTY(SOURCE ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp
                 APPEND 
		PROPERTY COMPILE_DEFINITIONS "MAKE_RCOORD_SINGLE" ) 
SET_PROPERTY(SOURCE ${IMAGE_SOURCES}
                 APPEND 
		PROPERTY COMPILE_DEFINITIONS "MAKE_RCOORD_SINGLE" ) 
SET_PROPERTY(SOURCE ${VECTLIB_SOURCES}
                 APPEND 
		PROPERTY COMPILE_DEFINITIONS "MAKE_RCOORD_SINGLE" ) 

SET( VECTLIB_PLUSPLUS_SOURCES ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp
	${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp )


set( ExtraDefinitions "${ExtraDefinitions};SACKCOMM_SOURCE" )

if( WIN32)
if( MINGW )
if( NOT ${Android} )
SET(CMAKE_RC_COMPILER_INIT "windres")
SET(CMAKE_RC_COMPILER "windres")
SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
ENABLE_LANGUAGE(RC)
endif( NOT ${Android} )
elseif( WATCOM )
string( REPLACE "/" "\\" WATCOM_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR} )
string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/all_resources.res
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../all_resources.rc
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\all_resources.res ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\..\\all_resources.rc
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\all_resources.res ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\..\\all_resources.rc
 )
 ADD_CUSTOM_TARGET( generate_foo DEPENDS ${PROJECT_BINARY_DIR}/../all_resources.res)
endif( MINGW )
endif( WIN32 ) 

if( WATCOM )
  set( SACK_BAG_PLUSPLUS sack_bag_pp )
  set( BAG_PLUSPLUS bag_pp )
  set( BAG_PSI_PLUSPLUS bag.psi_pp )
else(WATCOM)
  if( NOT __CLR__ )
    set( SACK_BAG_PLUSPLUS sack_bag++ )
    set( BAG_PLUSPLUS bag++ )
    set( BAG_PSI_PLUSPLUS bag.psi++ )
  endif( NOT __CLR__ )
endif(WATCOM)


if( BUILD_MONOLITHIC )

  set( SACK_LIBRARIES ${SACK_LIBRARIES} ${SACK_BAG_PLUSPLUS} )

  if( NOT __NO_GUI__ )
    set( BASE_SOURCES ${BASE_SOURCES} 
         ${IMAGE_SOURCES} 
         ${VIDLIB_SOURCES} 
         ${PSI_SOURCES}
    )
  endif( NOT __NO_GUI__ )

  set( C_SOURCES
    ${JPEG_SOURCE}
    ${ZLIB_SOURCE}
    ${PNG_SOURCE}
    ${FREETYPE_SOURCE}
    ${EXPAT_SOURCES}
  )
  if( NEED_UUID )
    set( C_SOURCES ${C_SOURCES} ${UUID_SOURCES} )
  endif( NEED_UUID )

Set( ExternalExtraDefinitions ${ExternalExtraDefinitions} PNG_INTERNAL )
set( SACK_SOURCES 
  ${FIRST_GCC_LIBRARY_SOURCE} 
    ${BASE_SOURCES} 
    ${VECTLIB_SOURCES}
    ${SNPRINTF_SOURCES}
    ${ODBC_SOURCES}
    ${BAG_SQLITE_INTERFACE}
    ${BAG_SQLITE_SOURCES}
    ${C_SOURCES}
    ${GENX_SOURCES}
  ${LAST_GCC_LIBRARY_SOURCE} 
)


if( NOT FORCE_CXX )

set( cplusplus_sources )

set( SACK_PLUSPLUS_SOURCES 
    ${BASE_SOURCES} 
    ${SNPRINTF_SOURCES}
    ${ODBC_SOURCES}
    ${GENX_SOURCES})


set_source_files_properties( ${SACK_PLUSPLUS_SOURCES} PROPERTIES LANGUAGE CXX )

set( cplusplus_sources 
  #${FIRST_GCC_LIBRARY_SOURCE} 
    ${cplusplus_sources} 
    ${VECTLIB_PLUSPLUS_SOURCES}
  #${LAST_GCC_LIBRARY_SOURCE} 
)

endif( NOT FORCE_CXX )

if( NOT FORCE_CXX )

  add_library( ${SACK_BAG_PLUSPLUS} SHARED ${cplusplus_sources} )
  SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS} PROPERTIES
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;${ExtraDefinitions};${ExternalExtraDefinitions};${PSIExtraDefinitions};TARGET_LABEL=sack_bag_plusplus;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${SACK_BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
  )

  if( SACK_BUILD_STATIC_LIBS )
    add_library( ${SACK_BAG_PLUSPLUS}_static ${cplusplus_sources} )
    SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS}_static PROPERTIES
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;${ExtraDefinitions};${ExternalExtraDefinitions};${PSIExtraDefinitions};TARGET_LABEL=sack_bag_plusplus;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${SACK_BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
    )
  endif( SACK_BUILD_STATIC_LIBS )
endif( NOT FORCE_CXX )


if( WATCOM )

    SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS} PROPERTIES LINK_FLAGS "option resource=all_resources.res")
    if( SACK_BUILD_STATIC_LIBS )
      SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS}_static PROPERTIES LINK_FLAGS "option resource=all_resources.res")
    endif( SACK_BUILD_STATIC_LIBS )

elseif( MINGW )
  ADD_DEPENDENCIES( ${SACK_BAG_PLUSPLUS} generate_foo )
  if( SACK_BUILD_STATIC_LIBS )
    ADD_DEPENDENCIES( ${SACK_BAG_PLUSPLUS}_static generate_foo )
  endif( SACK_BUILD_STATIC_LIBS )
endif()

set( SACK_LIBRARIES sack_bag ${SACK_BAG_PLUSPLUS} )

if( WIN32 )
  if( __ARM__ )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS} sack_bag ws2 mmtimer iphlpapi coredll corelibc.lib ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
      if( SACK_BUILD_STATIC_LIBS )
        my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static sack_bag_static ws2 mmtimer iphlpapi coredll corelibc.lib ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
      endif( SACK_BUILD_STATIC_LIBS )
    endif( NOT FORCE_CXX )
  else()
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS} ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} sack_bag ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES} )
      if( SACK_BUILD_STATIC_LIBS )
        my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} sack_bag_static ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES} )
      endif( SACK_BUILD_STATIC_LIBS )
    endif( NOT FORCE_CXX )
  endif()
else( WIN32 )
  if( NOT FORCE_CXX )
    my_target_link_libraries( ${SACK_BAG_PLUSPLUS} sack_bag ${ODBC_LIBRARY} ${SQLITE3_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${PLATFORM_LIBRARIES} )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static sack_bag_static ${ODBC_LIBRARY} ${SQLITE3_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${PLATFORM_LIBRARIES} )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
endif( WIN32 )

if( NOT __NO_CPLUSPLUS )
  if( NOT FORCE_CXX )
    install_mode_dest( ${SACK_BAG_PLUSPLUS} )
    if( SACK_BUILD_STATIC_LIBS )
      install_mode_dest( ${SACK_BAG_PLUSPLUS}_static )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
endif( NOT __NO_CPLUSPLUS )


if( NOT NEED_PNG ) 
  if( NOT FORCE_CXX )
    my_target_link_libraries( ${SACK_BAG_PLUSPLUS} png )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static png )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
endif() 
if( NOT NEED_JPEG ) 
  if( NOT FORCE_CXX )
    my_target_link_libraries( ${SACK_BAG_PLUSPLUS} jpeg   )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static jpeg   )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
endif() 
if( NOT NEED_FREETYPE )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}  freetype  )
      if( SACK_BUILD_STATIC_LIBS )
        my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static  freetype  )
      endif( SACK_BUILD_STATIC_LIBS )
    endif( NOT FORCE_CXX )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
endif()

#------------
else( BUILD_MONOLITHIC )
#------------

if( FORCE_CXX )
set( ExtraFlags ${ExtraFlags} /CLR )
endif( FORCE_CXX )


set( SACK_PLUSPLUS_SOURCES 
    ${BASE_SOURCES} 
    ${SNPRINTF_SOURCES}
    ${ODBC_SOURCES}
    ${GENX_SOURCES}
)

set_source_files_properties( ${SACK_PLUSPLUS_SOURCES} PROPERTIES LANGUAGE CXX )

if( NOT FORCE_CXX )

set( cplusplus_sources 
  #${FIRST_GCC_LIBRARY_SOURCE} 
    ${SACK_PLUSPLUS_SOURCES} 
    ${VECTLIB_PLUSPLUS_SOURCES}
  #${LAST_GCC_LIBRARY_SOURCE} 
)

SET_SOURCE_FILES_PROPERTIES( ${PSI_SOURCES} PROPERTIES LANGUAGE CXX )

sack_add_library( ${BAG_PLUSPLUS} SHARED ${cplusplus_sources} )
SET_PROPERTY( TARGET ${BAG_PLUSPLUS} APPEND PROPERTY COMPILE_DEFINITIONS "${ExtraDefinitions};${ExtraRenderFlags}" )
SET_PROPERTY( TARGET ${BAG_PLUSPLUS} APPEND PROPERTY COMPILE_FLAGS "${ExtraFlags}" )
SET_TARGET_PROPERTIES(${BAG_PLUSPLUS} PROPERTIES
                  DEFINE_SYMBOL SACK_BAG_EXPORTS
                  LINK_FLAGS "${extra_link_flags}"
)

endif( NOT FORCE_CXX )

if( __ARM__ )
  if( NOT __ANDROID__ )
    if( WIN32 )
      if( NOT FORCE_CXX )
        my_target_link_libraries( ${BAG_PLUSPLUS} ws2 mmtimer iphlpapi coredll corelibc.lib )
      endif( NOT FORCE_CXX )
    endif( WIN32 )
  else( NOT __ANDROID__ )
       my_target_link_libraries( ${BAG_PLUSPLUS} ${PLATFORM_LIBRARIES} )
       my_target_link_libraries( ${BAG_PLUSPLUS} bag.externals )
  endif( NOT __ANDROID__ )
else( __ARM__ )
    if( NOT FORCE_CXX )
       my_target_link_libraries( ${BAG_PLUSPLUS} ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} ${WIN_SYS_LIBS} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
       my_target_link_libraries( ${BAG_PLUSPLUS} bag.externals )
    endif( NOT FORCE_CXX )
endif( __ARM__ )


if( NOT __NO_CPLUSPLUS )
  if( FORCE_CXX )
    set( SACK_LIBRARIES bag bag.psi )
    set( SACK_CORE_LIBRARY bag bag.externals )
  else( FORCE_CXX )
    set( SACK_LIBRARIES bag bag.psi )
    set( SACK_LIBRARIES_PLUSPLUS ${BAG_PLUSPLUS} ${BAG_PSI_PLUSPLUS} )
    set( SACK_CORE_LIBRARY bag bag.externals )
    set( SACK_CORE_LIBRARY_PLUSPLUS ${BAG_PLUSPLUS} )
  endif( FORCE_CXX )
else( NOT __NO_CPLUSPLUS )	
    set( SACK_LIBRARIES bag bag.externals bag.psi )
    set( SACK_CORE_LIBRARY bag bag.externals )
endif( NOT __NO_CPLUSPLUS )

###########

  if( NOT FORCE_CXX )
    add_library( ${BAG_PSI_PLUSPLUS} SHARED 
       ${FIRST_GCC_LIBRARY_SOURCE} 
       ${PSI_SOURCES}
       ${LAST_GCC_LIBRARY_SOURCE} 
      )
    SET_TARGET_PROPERTIES(${BAG_PSI_PLUSPLUS} PROPERTIES
                  DEFINE_SYMBOL PSI_SOURCE
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS  "TARGET_LABEL=bag_psi_pp;SOURCE_PSI2;SYSTRAY_LIBRARAY;${PSIExtraDefinitions};TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  FOLDER "Control Library" 
                  LINK_FLAGS "${extra_link_flags}"
    )
    my_target_link_libraries( ${BAG_PSI_PLUSPLUS} bag ${BAG_PLUSPLUS} ${EXTERNALS} ${PLATFORM_LIBRARIES} )
    set( SACK_PSI_LIBRARY ${BAG_PSI_PLUSPLUS} bag.psi )
    if( __ANDROID__ )
        install_mode_dest( ${BAG_PLUSPLUS} ${BAG_PSI_PLUSPLUS} )
    else()
        install_mode_dest( ${BAG_PLUSPLUS} ${BAG_PSI_PLUSPLUS} )
    endif( __ANDROID__ )
  else( NOT FORCE_CXX )
    install_mode_dest( bag bag.psi ${EXTERNALS} )
  endif( NOT FORCE_CXX )

if( NOT NEED_FREETYPE )
  if( NOT __NO_CPLUSPLUS )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${BAG_PLUSPLUS}  freetype  )
    endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
endif()

endif( BUILD_MONOLITHIC )

if( WATCOM )
   set( ExtraDefinitions "${ExtraDefinitions};__OBSCURE_STREAM_INTERNALS" )
endif( WATCOM )


  if( USE_SQLITE_EXTERNAL )
   set( sqlite_external bag.sqlite3.external )
   set( ExtraDefinitions "${ExtraDefinitions};USE_SQLITE_INTERFACE" )
   add_library( ${sqlite_external} SHARED 
      ${FIRST_GCC_LIBRARY_SOURCE} 
      ${SQLITE_SOURCES}
      ${LAST_GCC_LIBRARY_SOURCE} )
   # required link back to bag for register interface
   my_target_link_libraries( ${sqlite_external} ${SACK_LIBRARIES} ${PLATFORM_LIBRARIES} )
   install_mode_dest( ${sqlite_external} )
   SET_TARGET_PROPERTIES( ${sqlite_external} PROPERTIES 
                  COMPILE_FLAGS  "-DTARGET_LABEL=bag_sqlite3 -DSQLITE_SOURCE"
                  LINK_FLAGS "${extra_link_flags}"
                  SUFFIX ""
                  PREFIX ""
                  FOLDER sqlite
   )
  else(USE_SQLITE_EXTERNAL)
  
  endif(USE_SQLITE_EXTERNAL)




