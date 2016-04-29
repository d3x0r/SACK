set_property(GLOBAL PROPERTY USE_FOLDERS On)
set( CMAKE_INSTALL_MESSAGE "LAZY" )
cmake_minimum_required(VERSION 3.0)

project( SACK )
message( WIN32=${WIN32} )
message( UNIX=${UNIX} )
message( CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} )

SET( MAKING_SACK_CORE ON )
SET( SACK_SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR} )


if( ${CMAKE_GENERATOR} MATCHES "Watcom WMake" )

 SET( CMAKE_RC_COMPILER "rc" )
 # add -aa - which allows non-static struct initializers in functions (init from function params);C only.
 set( CMAKE_C_FLAGS_INIT ${CMAKE_C_FLAGS_INIT} -aa )

 set( ExtraFlags "${ExtraFlags} /fp5 /fpi87" )
 # this flag is for build dll?  emit DllRuntime startup link 
 set( LibraryExtraFlags "-bd" )
 set( ConsoleProgramExtraFlags "-bc" )
 set( WindowsProgramExtraFlags "-bg" )
 #STRING( REPLACE "library clbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES_INIT ${CMAKE_C_STANDARD_LIBRARIES_INIT} )
 #STRING( REPLACE "library plbrdll.lib" "" CMAKE_C_STANDARD_LIBRARIES_INIT ${CMAKE_C_STANDARD_LIBRARIES_INIT} )
 add_definitions( "-D__CMAKE_VERSION__=(${CMAKE_MAJOR_VERSION}*1000000+${CMAKE_MINOR_VERSION}*10000+${CMAKE_PATCH_VERSION}*100+${CMAKE_TWEAK_VERSION})" )
endif( ${CMAKE_GENERATOR} MATCHES "Watcom WMake" )


set( SACK_BASE ${PROJECT_SOURCE_DIR} )

set( CMAKE_MODULE_PATH ${SACK_SOURCE_DIR}/makefiles/cmake )


include( DefaultInstall )

if( NOT CMAKE_CONFIGURATION_TYPES )
	set( CMAKE_CONFIGURATION_TYPES Debug Release )
endif( NOT CMAKE_CONFIGURATION_TYPES )

DEFINE_DEFAULT( CMAKE_BUILD_TYPE Release )
set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Cached cmake build type"  )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )
if( NOT CMAKE_BUILD_TYPE )
	message( FATAL_ERROR "MUST set CMAKE_BUILD_TYPE!" )
endif( NOT CMAKE_BUILD_TYPE )


if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    DEFINE_DEFAULT( __64__ ON )
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    DEFINE_DEFAULT( __64__ OFF )
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
SET( __64__ ${__64__} CACHE BOOL "Enable 64 considerations" )


if( TOOLCHAIN STREQUAL "QNX" )
	DEFINE_DEFAULT( __QNX__ ON )
        SET( __LINUX__ ON )
else( TOOLCHAIN STREQUAL "QNX" )
	DEFINE_DEFAULT( __QNX__ OFF )
endif( TOOLCHAIN STREQUAL "QNX" )
SET( __QNX__ ${__QNX__} CACHE BOOL "Enable Linux specific features" )
DEFINE_DEFAULT( __ARM__ OFF )
SET( __ARM__ ${__ARM__} CACHE BOOL "Enable ARM specific features" )
DEFINE_DEFAULT( __ANDROID__ OFF )
SET( __ANDROID__ ${__ANDROID__} CACHE BOOL "Enable Android specific features" )
DEFINE_DEFAULT( __ANDROID_OLD_PLATFORM_SUPPORT__ OFF )
SET( __ANDROID_OLD_PLATFORM_SUPPORT__ ${__ANDROID_OLD_PLATFORM_SUPPORT__} CACHE BOOL "Enable Android specific features" )
DEFINE_DEFAULT( __LINUX__ OFF )
SET( __LINUX__ ${__LINUX__} CACHE BOOL "Enable Linux specific features" )

if( __LINUX__ AND __64__ )
	set( linux64_default ON )
else( __LINUX__ AND __64__ )
	set( linux64_default OFF )
endif( __LINUX__ AND __64__ )

DEFINE_DEFAULT( __LINUX64__   ${linux64_default} )
SET( __LINUX64__ ${__LINUX64__} CACHE BOOL "Enable Linux-x64 specific features" )


if( NOT __LINUX__ AND NOT __LINUX64__ )
set(WIN32_VERSION "0x0601" CACHE STRING
      "Set minimum windows build version")
  set_property(CACHE WIN32_VERSION PROPERTY STRINGS
               "0x0601" "0x501" "0x0500" "0x0400")
add_definitions( -D_WIN32_WINNT=${WIN32_VERSION} -DWINVER=${WIN32_VERSION})
endif( NOT __LINUX__ AND NOT __LINUX64__ )

DEFINE_DEFAULT( ALLOW_IMAGE_INTERFACES OFF )
DEFINE_DEFAULT( __ANDROID__ OFF )
OPTION( __ANDROID__ "Enable Android Behvaviors" ${__ANDROID__} )
if( __ANDROID__ )
	message( "if you're using a latest NDK you might want to set __ANDROID_OLD_PLATFORM_SUPPORT__" )
	ADD_DEFINITIONS( -DANDROID_NDK_TARGET_PLATFORM=${ANDROID_NDK_TARGET_PLATFORM} )
	if( __ANDROID_OLD_PLATFORM_SUPPORT__ )
        	ADD_DEFINITIONS( -D__ANDROID_OLD_PLATFORM_SUPPORT__ )
	endif( __ANDROID_OLD_PLATFORM_SUPPORT__ )
	SET( __LINUX__ ON )
	SET( __ARM__ ON )
	SET( BUILD_MONOLITHIC OFF )
	SET( __NO_ODBC__ ON )
        SET( NEED_UUID ON )
	DEFINE_DEFAULT( __LINUX__ ON )
	DEFINE_DEFAULT( __ARM__ ON )
	DEFINE_DEFAULT( BUILD_MONOLITHIC OFF )
	DEFINE_DEFAULT( __NO_ODBC__ ON )
        DEFINE_DEFAULT( NEED_UUID ON )
        if( OFF )
        	# FUSE on android is generally root required access.
		FIND_PATH( FUSE_REQUIRED_INCLUDES PATHS ${SYSROOT}/usr/include NAMES sys/statvfs.h linux/fuse.h )
        
        	if( FUSE_REQUIRED_INCLUDES )
	        	set( NEED_FUSE ON )
	        endif( FUSE_REQUIRED_INCLUDES )
	        set( NEED_FUSE ON )
        endif( OFF )
        set( OPENGL_LIBRARY EGL GLESv2 )
        #set( OPENGL_LIBRARY GL )
	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} android log m )
else( __ANDROID__ )
  if( WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
      set( WIN32 ON )
      set( NEED_JPEG ON )
      set( NEED_PNG ON )
      set( NEED_ZLIB ON )
      set( NEED_FREETYPE ON )
      if( NOT __CLR__ )
        set( SOCKET_LIBRARIES MatrixSSL  )
      endif( NOT __CLR__ )
        set( SOCKET_LIBRARIES ${SOCKET_LIBRARIES} ws2_32 iphlpapi )
        set( WIN_SYS_LIBS winmm rpcrt4 )
        if( WATCOM )
	        set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} shfolder psapi )
        endif( WATCOM)
        set( OPENGL_LIBRARY opengl32 )
  else()
     if( UNIX )
     	set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/makefiles/cmake" )
        set( __LINUX__ ON )
	set( CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed" )
	set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed" )
	set( CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed" )
	set( PLATFORM_LIBRARIES m pthread dl )
	if( NOT __NO_ODBC__ )
		set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} odbc )
	endif( NOT __NO_ODBC__ )
	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} )
        set( USE_INTERNAL_STANDARDLIBS 0 )
       if( GCC )
           if( __64__ )
               ADD_DEFINITIONS( -m64 )
           endif( __64__ )
       endif( GCC )
       if( NOT __NO_GUI__ )
            #########  
            find_package( ZLIB )
            if( NOT ${ZLIB_FOUND} )
            	DEFINE_DEFAULT( NEED_ZLIB 1 )
            else( NOT ${ZLIB_FOUND} )
            	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${ZLIB_LIBRARIES} )
            endif( NOT ${ZLIB_FOUND} )
            #########
            find_package( PNG )
            if( NOT ${PNG_FOUND} )
            	DEFINE_DEFAULT( NEED_PNG 1 )
            else( NOT ${PNG_FOUND} )
            	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${PNG_LIBRARIES} )
            endif( NOT ${PNG_FOUND} )

            #########
            find_package( JPEG )
            if( NOT ${JPEG_FOUND} )
            	DEFINE_DEFAULT( NEED_JPEG ON )
            else( NOT ${JPEG_FOUND} )
            	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${JPEG_LIBRARIES} )
            endif( NOT ${JPEG_FOUND} )
            #########
            find_package( Freetype )
            # Freetype Find if not found is blank; but does not work for if( NOT )
            message( "fretype is ${FREETYPE_FOUND}" )
            if( NOT ${FREETYPE_FOUND} )
                DEFINE_DEFAULT( NEED_FREETYPE ON )
            else( NOT ${FREETYPE_FOUND} )
                set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${FREETYPE_LIBRARIES} )
                include_directories( ${FREETYPE_INCLUDE_DIRS} )
            endif( NOT ${FREETYPE_FOUND} )
            #########
            find_package( OpenGL )
            set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${OPENGL_LIBRARIES} )
            #########
            if( NOT __NO_GUI__ )
               find_package( GLEW )
               if( NOT ${GLEW_FOUND} )
               	set( NEED_GLEW ON )
               	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} glew )
               else( NOT ${GLEW_FOUND} )
               	set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${GLEW_LIBRARIES} )
               endif( NOT ${GLEW_FOUND} )
            endif( NOT __NO_GUI__ )
            #########
            find_package( X11 )
            set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} ${X11_LIBRARIES} )

            #message( "Platform_libs:"${PLATFORM_LIBRARIES} )
        endif( NOT __NO_GUI__ )
#	find_package( uuid )
	include( makefiles/cmake/FindSqlite.cmake )
#        find_package( FUSE )
        if( NOT ${FUSE_FOUND} )
#        	set( NEED_FUSE ON )
        endif( NOT ${FUSE_FOUND} )
        
	find_package( Sqlite )
        if( NOT ${SQLITE3_FOUND} )
        	set( NEED_SQLITE ON )
        endif( NOT ${SQLITE3_FOUND} )
	find_package( Threads REQUIRED )
     else()
        Message( "Not WIN32, not UNIX" )
     endif()
  endif()
endif( __ANDROID__ )




set( BULLET_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../bullet" CACHE STRING "bullet source path "  )

OPTION( SACK_BUILD_STATIC_LIBS "Enable building static versions of libraries" OFF )
OPTION( BUILD_EXTRAS "Enable building extras" OFF )
OPTION( INSTALL_EXTRAS "Enable installing extras; build anyway" ${BUILD_EXTRAS} )
OPTION( BUILD_TESTS "Enable building test" OFF )
if( NOT __LINUX__ )
OPTION( __LINUX__ "Enable specific linux features (disable WIN32)" ${UNIX} )
endif( NOT __LINUX__ )
OPTION( __LINUX64__ "Enable specific linux features (disable WIN32)" OFF )
if( NOT __ARM__ )
OPTION( __ARM__ "Enable specific arm-linux features (disable WIN32)" OFF )
endif( NOT __ARM__ )
OPTION( __CLR__ "Enable specific arm-linux features (disable WIN32)" OFF )
OPTION( __INCLUDE_INTERSHELL__ "Include building intershell core in root project" OFF )
if( NOT __NO_ODBC__ )
OPTION( __NO_ODBC__ "Use Odbc" ${__NO_GUI__} )
endif( NOT __NO_ODBC__ )
DEFINE_DEFAULT( __NO_NETWORK__ 0 )
OPTION( __NO_NETWORK__ "Disable network code" __NO_NETWORK__ )
DEFINE_DEFAULT( __NO_MSGSVR__ 0 )
OPTION( __NO_MSGSVR__ "Disable network code" __NO_MSGSVR__ )
OPTION( __ANDROID__ "Enable Android Behvaviors" 0 )
OPTION( ALLOW_IMAGE_INTERFACES "monolithic build, by default, forces direct linking to video/image interfaces; allow proxy" 0 )
OPTION( BUILD_ANDROID_CONSOLE_APPS "Enable building programs as command-line executable" ${__ANDROID__} )
OPTION( USE_OPTIONS "Use Option Code all internal option usage (and don't include option code)" 1 )
if( HOST_UNIX )
   	#cross compiling mingw isn't so lame.
	OPTION( USING_OLD_MINGW "MinGW Has NO UpdateLayeredWindow" OFF )
else( HOST_UNIX )
	OPTION( USING_OLD_MINGW "MinGW Has NO UpdateLayeredWindow" ${MINGW} )
endif( HOST_UNIX )
OPTION( USE_SACK_CUSTOM_MEMORY_ALLOCATION "Enable alternative memory allocator; affects locks on queues" 0 )
OPTION( USE_SQLITE "Use SQLITE (also links static internal)" 1 )
OPTION( USE_SQLITE_EXTERNAL "Use SQLITE (build as external module, included when off)" 0 )
OPTION( EXTREME_CODE_DEBUG "Enable extreme warning levels (mingw)" 0 )
OPTION( FORCE_INTERFACE "Force using image and video interfaces in monolithic mode" 0 )
OPTION( __NO_GUI__ "No GUI or image parts" ${__NO_GUI__} )
OPTION( __NO_DEFAULT_INTERFACES__ "Don't load interface.conf during deadstart" 0 )
OPTION( NEED_JPEG "Build internal jpeg lib (with suitable exports to link against)" ${NEED_JPEG} )
OPTION( NEED_UUID "Build internal uuid lib" ${NEED_UUID} )
OPTION( NEED_PNG "Build internal png lib (with suitable exports to link against)" ${NEED_PNG} )
OPTION( NEED_FREETYPE "Build internal freetype lib (with suitable exports to link against)" ${NEED_FREETYPE} )
OPTION( NEED_ZLIB "Build internal zlib lib (with suitable exports to link against)" ${NEED_ZLIB} )
OPTION( NEED_EXPAT "Build internal Expat library (older version)" ON )
OPTION( __WINDOWS_ARM_CE__ "Build for windows CE 5.0 ARM" 0 )
OPTION( USE_ASSEMBLY "Use Assembly files as available.  NASM required for this." OFF )
OPTION( USE_HARDWARE_INFO "Include Hardware information utilities" ${WIN32} )

OPTION( BUILD_MONOLITHIC "Build one giant library" ${BUILD_MONOLITHIC} )
OPTION( BUILD_UNICODE "Build All as Unicode" ${BUILD_UNICODE} )

# default itnerface output path... (override if __CLR__)
# should make this a common target variable instead also...
set( INTERFACE_OUTPUT_DIR ${BINARY_OUTPUT_DIR} )

if( MSVC )
  if( FORCE_MSVCRT )
    set( extra_link_flags "${extra_link_flags} /NODEFAULTLIB" )
    set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} msvcrt )
  endif( FORCE_MSVCRT )
  #add_definitions( -D_CRT_SECURE_NO_WARNINGS -wd4995 -wd4996 )
  add_definitions( -wd4996 )
  
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP8")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP8")
  
  if( __CLR__ )
    add_definitions( -D__CLR__ )
    set( INTERFACE_OUTPUT_DIR . )
    set( BUILD_UNICODE ON )
    string( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS_INIT ${CMAKE_CXX_FLAGS_INIT} )
    string( REPLACE "/EHsc" "" CMAKE_C_FLAGS_INIT ${CMAKE_C_FLAGS_INIT} )
    string( REPLACE "/EHs" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_INIT} )
    string( REPLACE "/EHs" "" CMAKE_C_FLAGS_INIT ${CMAKE_C_FLAGS_INIT} )
    string( REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )
    # cannot set CLR for C code anyway
    #string( REPLACE "/RTC1" "/CLR" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} )
    set( FORCE_CXX ON )
  endif( __CLR__ )
endif( MSVC )

if( __NO_NETWORK__ )
   set( ExtraDefinitions ${ExtraDefinitions};__NO_NETWORK__ )
endif( __NO_NETWORK__ )

if( __NO_MSGSVR__ )
   set( ExtraDefinitions ${ExtraDefinitions};__NO_MSGSVR__ )
endif( __NO_MSGSVR__ )


if( NOT __NO_ODBC__ OR USE_SQLITE )

 if( NOT __NO_ODBC__ )
   set( ExtraDefinitions ${ExtraDefinitions};USE_ODBC )
   if( WIN32 )
      SET( ODBC_LIBRARY odbc32 )
   else()
      if( NOT NEED_UUID )
	      SET( ODBC_LIBRARY odbc uuid )
      else( NOT NEED_UUID )
	      SET( ODBC_LIBRARY odbc )
      endif( NOT NEED_UUID )

   endif()
 else( NOT __NO_ODBC__ )
   set( ExtraDefinitions ${ExtraDefinitions};__NO_ODBC__ )
 endif( NOT __NO_ODBC__ )

else( NOT __NO_ODBC__ OR USE_SQLITE )
	set( ExtraDefinitions ${ExtraDefinitions};__NO_SQL__;__NO_ODBC__;__NO_OPTIONS__ )
endif( NOT __NO_ODBC__ OR USE_SQLITE )


if( NOT USE_OPTIONS OR ( __NO_ODBC__ AND NOT USE_SQLITE ) )
    set( ExtraDefinitions ${ExtraDefinitions};__NO_OPTIONS__ )
endif( NOT USE_OPTIONS OR ( __NO_ODBC__ AND NOT USE_SQLITE ) )
 


if( USE_SACK_CUSTOM_MEMORY_ALLOCATION )
add_definitions( -DUSE_SACK_CUSTOM_MEMORY_ALLOCATION )
endif( USE_SACK_CUSTOM_MEMORY_ALLOCATION )

if( NOT NEED_JPEG )
        set( JPEG_LIBRARIES jpeg )
endif( NOT NEED_JPEG )

if( NOT NEED_PNG )
        set( PNG_LIBRARIES png )
endif( NOT NEED_PNG )

if( WIN32 AND MINGW )
    set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} winmm ${SOCKET_LIBRARIES}  ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${ODBC_LIBRARY} )
endif( WIN32 AND MINGW )

if( WATCOM OR MSVC )
    set( PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} winmm ${SOCKET_LIBRARIES}  ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${ODBC_LIBRARY} )
        set( USE_INTERNAL_STANDARDLIBS 1 )
else()
        set( USE_INTERNAL_STANDARDLIBS 0 )
endif()
        

if( __QNX__ )
ADD_DEFINITIONS( -D__QNX__ )
endif( __QNX__ )

if( BUILD_UNICODE )
	add_definitions( -D_UNICODE -DUNICODE )
endif( BUILD_UNICODE )

if( __ANDROID__ )
	add_definitions( -D__ANDROID__ )
endif( __ANDROID__ )

if( ALLOW_IMAGE_INTERFACES )
	add_definitions( -DALLOW_IMAGE_INTERFACES )
endif( ALLOW_IMAGE_INTERFACES )

if( __ARM__ )
	add_definitions( -D__ARM__)
endif( __ARM__ )

if( EXTREME_CODE_DEBUG )
	if(CMAKE_COMPILER_IS_GNUCC ) 
		add_definitions(-Wall -Wextra -Wconversion -Wsign-conversion -Wsign-compare -Wtype-limits)
	endif(CMAKE_COMPILER_IS_GNUCC )
endif( EXTREME_CODE_DEBUG )

if( __ARM__ OR __BUILD_ARM_CE__ )
  if( __LINUX__ )
    add_definitions( -D__LINUX__ -DARM -D_ARM_ )
  else(__LINUX__)
    add_definitions( -D_WIN32_WCE=0x501 -DUNDER_CE -DWINCE -D_WINDOWS -DARM -D_ARM_ -DWIN32_PLATFORM_WFSP -D_UNICODE -DUNICODE -DPOCKETPC2003_UI_MODEL -DWIN32 )
  endif( __LINUX__ )
else( )
  if( MSVC )
    if( CMAKE_CL_64 )
      set( __64__ ON )
      add_definitions( -D_AMD64_ -D__64__ -D_WIN64 )
    else( CMAKE_CL_64 )
      add_definitions( -D_X86_ )
    endif( CMAKE_CL_64 )
  endif( MSVC )
endif()

if( __64__ )
set( TARGET_ARCH x64 )
else( __64__ )
set( TARGET_ARCH x86 )
endif( __64__ )


set( ExtraDefinitions ${ExtraDefinitions};SACK_CORE_BUILD )



if( __LINUX__ )
   add_definitions( -D__LINUX__ )
endif( __LINUX__ )
if( __LINUX64__ )
   set( __64__ ON )
   add_definitions( -D__LINUX64__ )
endif( __LINUX64__ )

if( __64__ )
   add_definitions( -D__64__ )
endif( __64__ )


# this is where external image libraries are added for windows native builds
if( NEED_JPEG OR NEED_PNG OR NEED_FREETYPE OR NEED_FUSE )
include( CMakeExternal.txt )
endif()


set(  CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -D_DEBUG_INFO" )
# visual studio headers change with _DEBUG defined... so it can't be release with _DEBUG defined
set(  CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -D_DEBUG_INFO" )
set(  CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -D_DEBUG_INFO" )
# visual studio headers change with _DEBUG defined... so it can't be release with _DEBUG defined
set(  CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_DEBUG_INFO" )

# here, blank/empty is as good as saying 'here'
SET( SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/ )
include( CMakeSources.lst )

add_subdirectory( src/contrib/gsm-1.0-pl13 )
add_subdirectory( src/contrib/libvlc/2.0.1 )


# expat is a 'standard' library.  ripped a copy for ease, it's also incredibly simple.
set( ExternalExtraDefinitions "${ExternalExtraDefinitions};${EXPAT_SOURCES_FLAGS}" )

# genx is searchable by looking for 'genx xml'
set( ExtraDefinitions "${ExtraDefinitions};${GENX_SOURCES_FLAGS}" )

if( WIN32 )
	set( VidlibExtraDefinitions "${VidlibExtraDefinitions};" )
endif( WIN32 )

if( __NO_GUI__ )
  set( ExtraDefinitions ${ExtraDefinitions};__NO_GUI__ )
else( __NO_GUI__ )
  set( PSIExtraDefinitions "${PSIExtraDefinitions};PSI_CONSOLE_SOURCE" )
endif( __NO_GUI__ )


if( NOT __ARM__ )
  set(BASE_SOURCES ${BASE_SOURCES} ${SACK_MSG_SERVICES} ${FILEMONITOR_SOURCES} ${HTML5_SOURCES} )
else()
  set( ExtraDefinitions "${ExtraDefinitions};__NO_MSGSVR__" )
  set( __NO_MSGSVR__ ON )
endif()

EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.c )
EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp )

       add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.c
                           DEPENDS ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES}
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.c
                           COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.c
                           )
       add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp
                           DEPENDS ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES}
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp
                           COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp
                           )
       add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp
                           DEPENDS ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES}
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp
                           COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp
                           )

# ADD_CUSTOM_TARGET( generate_vectlib DEPENDS ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.c)

#EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/${VECTLIB_SOURCES} ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp )
SET_PROPERTY(SOURCE ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp
                 APPEND 
		PROPERTY COMPILE_DEFINITIONS "MAKE_RCOORD_SINGLE" ) 
SET_PROPERTY(SOURCE ${IMAGE_SOURCES}
                 APPEND 
		PROPERTY COMPILE_DEFINITIONS "MAKE_RCOORD_SINGLE" ) 

SET( VECTLIB_PLUSPLUS_SOURCES ${CMAKE_BINARY_DIR}/src/vectlib/vectlib_double.cpp
	${CMAKE_BINARY_DIR}/src/vectlib/vectlib_float.cpp )


set( ExtraDefinitions "${ExtraDefinitions};SACKCOMM_SOURCE" )

include_directories(  ${SACK_BASE}/include )


if( USING_OLD_MINGW )
  add_definitions( -DMINGW_SUX )
  set( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -fshort-wchar )
  set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fshort-wchar )
endif()

if( WIN32 )
  add_definitions( -DWIN32 )
endif(WIN32)


if( WIN32)
if( MINGW )
if( NOT Android )
	if( HOST_UNIX )
        else( HOST_UNIX )
		SET(CMAKE_RC_COMPILER_INIT "windres")
		SET(CMAKE_RC_COMPILER "windres")
		SET(CMAKE_RC_COMPILE_OBJECT "windres <FLAGS> <DEFINES> -O coff -o <OBJECT> <SOURCE>")
	endif( HOST_UNIX )
                
	ENABLE_LANGUAGE(RC)
endif( NOT Android )
elseif( WATCOM )
string( REPLACE "/" "\\" WATCOM_PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR} )
string( REPLACE "/" "\\" WATCOM_CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/all_resources.res
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/all_resources.rc
                     COMMAND echo ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\all_resources.res ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\all_resources.rc
                     COMMAND ${CMAKE_RC_COMPILER} -i${WATCOM_CMAKE_CURRENT_SOURCE_DIR} -fo${WATCOM_PROJECT_BINARY_DIR}\\all_resources.res ${WATCOM_CMAKE_CURRENT_SOURCE_DIR}\\all_resources.rc
 )
 ADD_CUSTOM_TARGET( generate_foo DEPENDS ${PROJECT_BINARY_DIR}/all_resources.res)
endif( MINGW )
endif( WIN32 ) 

if( NOT __NO_CPLUSPLUS )

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

FOREACH(SOURCE ${SACK_PLUSPLUS_HEADERS})
   if( ${SOURCE} MATCHES "${SOURCES_ROOT}(.*)$" )
       set( BASENAME ${CMAKE_MATCH_1} )
       set( plusplus_header_dup ${plusplus_header_dup} ${CMAKE_BINARY_DIR}/${BASENAME} )
       add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}
                           DEPENDS ${SOURCE}
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME} 
                           COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME} 
                           )
       #EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME} )

   else()
   endif()
ENDFOREACH(SOURCE)

endif( NOT __NO_CPLUSPLUS )

macro( COPY_CPLUSPLUS FILE_LIST )
FOREACH(SOURCE ${ARGN} )
   if( ${SOURCE} MATCHES "${SACK_BASE}/(.*)\\.c$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .cpp )
   elseif( ${SOURCE} MATCHES "${SACK_BASE}/(.*)\\.h$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .h )
   elseif( ${SOURCE} MATCHES "(.*)\\.c$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .cpp )
   elseif( ${SOURCE} MATCHES "(.*)\\.h$" )
      set( FILEOK 1 )
      set( BASENAME ${CMAKE_MATCH_1} )
      set( FILEEXT .h )
   else()
      set( FILEOK 0 )
      set( BASENAME "" )
   endif()
   
   if( FILEOK )
       get_source_file_property(SOURCE_FOLDER ${SOURCE} FOLDER)
       
       if( ${SOURCE} MATCHES "^${PROJECT_SOURCE_DIR}.*" )
          if( NOT ${SOURCE_FOLDER} MATCHES "NOTFOUND" )
            #message( "err folder : ${SOURCE_FOLDER} " )
            SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${SOURCE} )
            SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
          endif()
          add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}
                              DEPENDS ${SOURCE}
                              COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                              COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                              )
          #EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
       
       else( ${SOURCE} MATCHES "^${PROJECT_SOURCE_DIR}.*" )
         if( NOT ${SOURCE_FOLDER} MATCHES "NOTFOUND" )
           #message( "err folder : ${SOURCE_FOLDER} " )
           SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${PROJECT_SOURCE_DIR}/${SOURCE} )
           SOURCE_GROUP( ${SOURCE_FOLDER} FILES ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
         endif()
         add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT}
                             DEPENDS ${PROJECT_SOURCE_DIR}/${SOURCE}
                             COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                             COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} 
                             )
         #EXECUTE_PROCESS(COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
         #message( "LIB2_SOURCE2 : ${BASENAME}  ${SOURCE}" )
       endif()
     set( ${FILE_LIST} ${${FILE_LIST}} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
     #set( cplusplus_sources ${cplusplus_sources} ${CMAKE_BINARY_DIR}/${BASENAME}${FILEEXT} )
   else()
      #message( "NOT THING ${SOURCE}" )
   endif()
ENDFOREACH(SOURCE)
endmacro( COPY_CPLUSPLUS )

if( __ANDROID__ )
#set( extra_link_flags  "${extra_link_flags} -Wl,-rpath,${CMAKE_BINARY_DIR}" ) 
endif( __ANDROID__ )


if( BUILD_MONOLITHIC )

  if( NOT FORCE_INTERFACE )
    add_definitions( -DFORCE_NO_INTERFACE )
  endif( NOT FORCE_INTERFACE )

  add_definitions( -DSACK_MONOLITHIC_BUILD )
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
    ${FUSE_SOURCES}
    ${EXPAT_SOURCES}
)
if( NEED_UUID )
set( C_SOURCES ${C_SOURCES} ${UUID_SOURCES} )
endif( NEED_UUID )

Set( ExternalExtraDefinitions ${ExternalExtraDefinitions} PNG_INTERNAL )
set( SACK_SOURCES 
  ${FIRST_GCC_LIBRARY_SOURCE} 
    ${BASE_SOURCES} 
	${VIDLIB_HEADLESS_SOURCES}
    ${VECTLIB_SOURCES}
    ${VECTLIB_MORE_SOURCES}
    ${SNPRINTF_SOURCES}
    ${ODBC_SOURCES}
    ${BAG_SQLITE_INTERFACE}
    ${BAG_SQLITE_SOURCES}
    ${C_SOURCES}
    ${GENX_SOURCES}
  ${LAST_GCC_LIBRARY_SOURCE} 
)


if( NOT __NO_CPLUSPLUS )
if( NOT FORCE_CXX )

set( cplusplus_sources )

set( SACK_PLUSPLUS_SOURCES 
    ${BASE_SOURCES} 
    ${SNPRINTF_SOURCES}
    ${ODBC_SOURCES}
    ${GENX_SOURCES}
    ${BAG_SQLITE_INTERFACE}
	)



COPY_CPLUSPLUS( cplusplus_sources ${SACK_PLUSPLUS_SOURCES} include/sha1.h include/md5.h )

set( cplusplus_sources 
  #${FIRST_GCC_LIBRARY_SOURCE} 
    ${plusplus_header_dup}
    ${cplusplus_sources} 
    ${VECTLIB_PLUSPLUS_SOURCES}
    ${BAG_SQLITE_SOURCES}
  #${LAST_GCC_LIBRARY_SOURCE} 
)

endif( NOT FORCE_CXX )

endif( NOT __NO_CPLUSPLUS )

add_library( sack_bag SHARED ${SACK_SOURCES} )
SET_TARGET_PROPERTIES(sack_bag PROPERTIES
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;${ExtraDefinitions};${ExternalExtraDefinitions};${PSIExtraDefinitions};TARGET_LABEL=sack_bag;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}sack_bag${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
)
target_link_libraries( sack_bag gsm-1.0-pl13 )
if( NOT __CLR__ )
	target_link_libraries( sack_bag MatrixSSL )
endif( NOT __CLR__ )
        

if( SACK_BUILD_STATIC_LIBS )
add_library( sack_bag_static ${SACK_SOURCES} )
SET_TARGET_PROPERTIES(sack_bag_static PROPERTIES
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;${ExtraDefinitions};${ExternalExtraDefinitions};${PSIExtraDefinitions};TARGET_LABEL=sack_bag;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}sack_bag${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
)
target_link_libraries( sack_bag_static gsm-1.0-pl13 )
endif( SACK_BUILD_STATIC_LIBS )

if( NOT __NO_CPLUSPLUS )
if( NOT FORCE_CXX )

add_library( ${SACK_BAG_PLUSPLUS} SHARED ${cplusplus_sources} )
SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS} PROPERTIES
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;${ExtraDefinitions};${ExternalExtraDefinitions};${PSIExtraDefinitions};TARGET_LABEL=sack_bag_plusplus;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${SACK_BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
)
target_link_libraries( ${SACK_BAG_PLUSPLUS} gsm-1.0-pl13 )
if( NOT __CLR__ )
	target_link_libraries( ${SACK_BAG_PLUSPLUS} MatrixSSL )
endif( NOT __CLR__ )

if( SACK_BUILD_STATIC_LIBS )
add_library( ${SACK_BAG_PLUSPLUS}_static ${cplusplus_sources} )
SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS}_static PROPERTIES
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "SACK_BAG_EXPORTS;${ExtraDefinitions};${ExternalExtraDefinitions};${PSIExtraDefinitions};TARGET_LABEL=sack_bag_plusplus;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${SACK_BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
)
target_link_libraries( ${SACK_BAG_PLUSPLUS}_static gsm-1.0-pl13 )
endif( SACK_BUILD_STATIC_LIBS )

endif( NOT FORCE_CXX )
endif( NOT __NO_CPLUSPLUS )


if( __WINDOWS_ARM_CE__ )
  SET_TARGET_PROPERTIES( sack_bag PROPERTIES LINK_LIBRARIES "" )
endif()

if( WATCOM )
  ADD_DEPENDENCIES( sack_bag generate_foo )
  SET_TARGET_PROPERTIES(sack_bag PROPERTIES LINK_FLAGS "option resource=all_resources.res")
  if( SACK_BUILD_STATIC_LIBS )
    SET_TARGET_PROPERTIES(sack_bag_static PROPERTIES LINK_FLAGS "option resource=all_resources.res")
  endif( SACK_BUILD_STATIC_LIBS )
  if( NOT __NO_CPLUSPLUS )
  if( NOT FORCE_CXX )
    SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS} PROPERTIES LINK_FLAGS "option resource=all_resources.res")
    if( SACK_BUILD_STATIC_LIBS )
      SET_TARGET_PROPERTIES(${SACK_BAG_PLUSPLUS}_static PROPERTIES LINK_FLAGS "option resource=all_resources.res")
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )

elseif( MINGW )
  #ADD_DEPENDENCIES( sack_bag generate_foo )
  #if( SACK_BUILD_STATIC_LIBS )
  #  ADD_DEPENDENCIES( sack_bag_static generate_foo )
  #endif( SACK_BUILD_STATIC_LIBS )
  #ADD_DEPENDENCIES( sack_bag all_resources.res )
  #SET_TARGET_PROPERTIES(sack_bag PROPERTIES LINK_FLAGS "all_resources.res")
endif()

set( SACK_LIBRARY sack_bag )
if( NOT __NO_CPLUSPLUS )
set( SACK_LIBRARIES sack_bag ${SACK_BAG_PLUSPLUS} )
endif( NOT __NO_CPLUSPLUS )
set( SACK_CORE_LIBRARY sack_bag )

if( WIN32 )
  if( __ARM__ )
    my_target_link_libraries( sack_bag ws2 mmtimer iphlpapi coredll corelibc.lib ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES} )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( sack_bag_static ws2 mmtimer iphlpapi coredll corelibc.lib ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES} )
    endif( SACK_BUILD_STATIC_LIBS )
    if( NOT __NO_CPLUSPLUS )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS} sack_bag ws2 mmtimer iphlpapi coredll corelibc.lib ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
      if( SACK_BUILD_STATIC_LIBS )
        my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static sack_bag_static ws2 mmtimer iphlpapi coredll corelibc.lib ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
      endif( SACK_BUILD_STATIC_LIBS )
    endif( NOT FORCE_CXX )
    endif( NOT __NO_CPLUSPLUS )
  else()
    my_target_link_libraries( sack_bag ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( sack_bag_static ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
    endif( SACK_BUILD_STATIC_LIBS )
    if( NOT __NO_CPLUSPLUS )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS} ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} sack_bag ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES} )
      if( SACK_BUILD_STATIC_LIBS )
        my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} sack_bag_static ${WIN_SYS_LIBS} ${OPENGL_LIBRARY} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES} )
      endif( SACK_BUILD_STATIC_LIBS )
    endif( NOT FORCE_CXX )
    endif( NOT __NO_CPLUSPLUS )
  endif()
else( WIN32 )
  my_target_link_libraries( sack_bag ${ODBC_LIBRARY} ${SQLITE3_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${PLATFORM_LIBRARIES} )
  if( SACK_BUILD_STATIC_LIBS )
    my_target_link_libraries( sack_bag_static ${ODBC_LIBRARY} ${SQLITE3_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${PLATFORM_LIBRARIES} )
  endif( SACK_BUILD_STATIC_LIBS )
  if( NOT __NO_CPLUSPLUS )
  if( NOT FORCE_CXX )
    my_target_link_libraries( ${SACK_BAG_PLUSPLUS} sack_bag ${ODBC_LIBRARY} ${SQLITE3_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${PLATFORM_LIBRARIES} )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static sack_bag_static ${ODBC_LIBRARY} ${SQLITE3_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${PLATFORM_LIBRARIES} )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
endif( WIN32 )

install_mode_dest( sack_bag )
if( SACK_BUILD_STATIC_LIBS )
  install_mode_dest( sack_bag_static )
endif( SACK_BUILD_STATIC_LIBS )
if( NOT __NO_CPLUSPLUS )
if( NOT FORCE_CXX )
install_mode_dest( ${SACK_BAG_PLUSPLUS} )
if( SACK_BUILD_STATIC_LIBS )
  install_mode_dest( ${SACK_BAG_PLUSPLUS}_static )
endif( SACK_BUILD_STATIC_LIBS )
endif( NOT FORCE_CXX )
endif( NOT __NO_CPLUSPLUS )

if( CMAKE_COMPILER_IS_GNUCC AND NOT MINGW)
if( __CLR__ )
       set( INTERFACE_CONFIGURATION data/interface.conf.new )
else( __CLR__ )
       set( INTERFACE_CONFIGURATION data/interface.conf.new )
endif(__CLR__)
else( CMAKE_COMPILER_IS_GNUCC )
   if( BUILD_UNICODE )
       set( INTERFACE_CONFIGURATION data/interface.conf.unicode )
   else( BUILD_UNICODE )
      if( SUPPORTS_PARALLEL_BUILD_TYPE )
        if( __CLR__ )
          set( INTERFACE_CONFIGURATION data/interface.conf )
        else( __CLR__ )
          set( INTERFACE_CONFIGURATION data/interface.conf )
        endif( __CLR__ )
      else( SUPPORTS_PARALLEL_BUILD_TYPE )
        set( INTERFACE_CONFIGURATION data/interface.conf )
      endif( SUPPORTS_PARALLEL_BUILD_TYPE )
   endif( BUILD_UNICODE )
endif( CMAKE_COMPILER_IS_GNUCC AND NOT MINGW)

INSTALL(
  FILES
    ${INTERFACE_CONFIGURATION}
  DESTINATION
    # this could also be /etc
    ${INTERFACE_OUTPUT_DIR}
  RENAME interface.conf
)



if( NOT NEED_PNG ) 
  my_target_link_libraries( sack_bag png )
  if( SACK_BUILD_STATIC_LIBS )
    my_target_link_libraries( sack_bag_static png )
  endif( SACK_BUILD_STATIC_LIBS )
  if( NOT __NO_CPLUSPLUS )
  if( NOT FORCE_CXX )
    my_target_link_libraries( ${SACK_BAG_PLUSPLUS} png )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static png )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
endif() 
if( NOT NEED_JPEG ) 
  my_target_link_libraries( sack_bag jpeg   )
  if( SACK_BUILD_STATIC_LIBS )
    my_target_link_libraries( sack_bag_static jpeg   )
  endif( SACK_BUILD_STATIC_LIBS )
  if( NOT __NO_CPLUSPLUS )
  if( NOT FORCE_CXX )
    my_target_link_libraries( ${SACK_BAG_PLUSPLUS} jpeg   )
    if( SACK_BUILD_STATIC_LIBS )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static jpeg   )
    endif( SACK_BUILD_STATIC_LIBS )
  endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
endif() 

if( NOT NEED_FREETYPE )
  my_target_link_libraries( sack_bag ${Freetype_LIBRARIES}  )
  if( SACK_BUILD_STATIC_LIBS )
    my_target_link_libraries( sack_bag_static  ${Freetype_LIBRARIES}  )
  endif( SACK_BUILD_STATIC_LIBS )
  if( NOT __NO_CPLUSPLUS )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${SACK_BAG_PLUSPLUS}  ${Freetype_LIBRARIES}  )
      if( SACK_BUILD_STATIC_LIBS )
        my_target_link_libraries( ${SACK_BAG_PLUSPLUS}_static  ${Freetype_LIBRARIES}  )
      endif( SACK_BUILD_STATIC_LIBS )
    endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
endif()


#------------
else( BUILD_MONOLITHIC )
#------------

add_library( bag SHARED 
  ${FIRST_GCC_LIBRARY_SOURCE} 
    ${BASE_SOURCES} 
    ${VECTLIB_SOURCES}
    ${VECTLIB_MORE_SOURCES}
    ${ODBC_SOURCES}
    ${GENX_SOURCES}
    ${BAG_SQLITE_INTERFACE}
    ${BAG_SQLITE_SOURCES}
  ${LAST_GCC_LIBRARY_SOURCE}  
)
target_link_libraries( bag gsm-1.0-pl13 )
if( NOT __CLR__ )
	target_link_libraries( bag MatrixSSL )
endif( NOT __CLR__ )

if( __CLR__ )
#set( ExtraFlags ${ExtraFlags} /CLR )
endif( __CLR__ )

SET_TARGET_PROPERTIES(bag PROPERTIES 
                  DEFINE_SYMBOL SACK_BAG_EXPORTS
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "${ExtraDefinitions};TARGET_LABEL=bag;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}bag${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
)

if( __WINDOWS_ARM_CE__ )
SET_TARGET_PROPERTIES( bag PROPERTIES LINK_LIBRARIES "" )
endif( __WINDOWS_ARM_CE__ )

if( __WINDOWS_ARM_CE__ )
SET( LINK_LIBRARIES "" )
endif( __WINDOWS_ARM_CE__ )

set( SACK_PLUSPLUS_SOURCES 
    ${BASE_SOURCES} 
    ${SNPRINTF_SOURCES}
    ${ODBC_SOURCES}
    ${GENX_SOURCES}
    ${BAG_SQLITE_INTERFACE}
)

if( NOT __NO_CPLUSPLUS )
if( NOT FORCE_CXX )
   set( cplusplus_sources )
   COPY_CPLUSPLUS( cplusplus_sources ${SACK_PLUSPLUS_SOURCES} include/sha1.h include/md5.h )
   if( NEED_SQLITE )
      COPY_CPLUSPLUS( cplusplus_sources ${SACK_PLUSPLUS_SOURCES} src/contrib/sqlite/${SQLITE_VERSION}/sqlite3.h )
   endif( NEED_SQLITE )
if( NEED_UUID )
   COPY_CPLUSPLUS( cplusplus_sources ${SACK_PLUSPLUS_SOURCES} src/contrib/uuid-1.6.2/uuid.h )
endif( NEED_UUID )

set( cplusplus_sources 
    ${plusplus_header_dup}
    ${cplusplus_sources} 
    ${VECTLIB_PLUSPLUS_SOURCES}
    ${BAG_SQLITE_SOURCES}
)

set( psi_cplusplus_sources )
COPY_CPLUSPLUS( psi_cplusplus_sources ${PSI_SOURCES} )

add_library( ${BAG_PLUSPLUS} SHARED ${cplusplus_sources} )
target_link_libraries( ${BAG_PLUSPLUS} gsm-1.0-pl13 )
if( NOT __CLR__ )
	target_link_libraries( ${BAG_PLUSPLUS} MatrixSSL )
endif( NOT __CLR__ )

SET_TARGET_PROPERTIES(${BAG_PLUSPLUS} PROPERTIES
                  DEFINE_SYMBOL SACK_BAG_EXPORTS
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS "${ExtraDefinitions};TARGET_LABEL=bag_plusplus;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  LINK_FLAGS "${extra_link_flags}"
)

endif( NOT FORCE_CXX )
endif( NOT __NO_CPLUSPLUS )

if( __ARM__ )
  if( NOT __ANDROID__ )
    if( WIN32 )
      my_target_link_libraries( bag ws2 mmtimer iphlpapi coredll corelibc.lib )
      if( NOT __NO_CPLUSPLUS )
      if( NOT FORCE_CXX )
        my_target_link_libraries( ${BAG_PLUSPLUS} ws2 mmtimer iphlpapi coredll corelibc.lib )
      endif( NOT FORCE_CXX )
      endif( NOT __NO_CPLUSPLUS )
    endif( WIN32 )
  else( NOT __ANDROID__ )
     my_target_link_libraries( bag ${PLATFORM_LIBRARIES} )
     if( NOT __NO_CPLUSPLUS )
       my_target_link_libraries( ${BAG_PLUSPLUS} ${PLATFORM_LIBRARIES} )
       my_target_link_libraries( ${BAG_PLUSPLUS} bag.externals )
     endif( NOT __NO_CPLUSPLUS )
  endif( NOT __ANDROID__ )
else()
  my_target_link_libraries( bag ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} ${WIN_SYS_LIBS} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
  if( NOT __NO_CPLUSPLUS )
    if( NOT FORCE_CXX )
       my_target_link_libraries( ${BAG_PLUSPLUS} ${ODBC_LIBRARY} ${SOCKET_LIBRARIES} ${WIN_SYS_LIBS} ${SQLITE3_LIBRARIES} ${PLATFORM_LIBRARIES})
       my_target_link_libraries( ${BAG_PLUSPLUS} bag.externals )
    endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
endif()


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

if( NEED_FREETYPE OR NEED_JPEG OR NEED_PNG OR NEED_EXPAT OR USE_SQLITE )

set( SACK_LIBRARIES ${SACK_LIBRARIES} bag.externals )
if( NEED_UUID )
  set( BAG_EXTERNAL_MORE_SOURCES ${UUID_SOURCES} )
endif( NEED_UUID )

add_library( bag.externals SHARED 
    ${JPEG_SOURCE}
    ${ZLIB_SOURCE}
    ${PNG_SOURCE}
    ${FREETYPE_SOURCE}
    ${EXPAT_SOURCES}
    ${FUSE_SOURCES}
    ${BAG_EXTERNAL_SQLITE_SOURCES}
    ${BAG_EXTERNAL_MORE_SOURCES}
)
SET_TARGET_PROPERTIES(bag.externals PROPERTIES
                  COMPILE_FLAGS  "-DTARGET_LABEL=bag_externals -DBAG_EXTERNALS -DPNG_INTERNAL -D__PNG_LIBRARY_SOURCE__"
                  COMPILE_DEFINITIONS "${ExternalExtraDefinitions}"
                  LINK_FLAGS "${extra_link_flags}"
                  FOLDER "Externals"
)

if( __QNX__ )
    my_target_link_libraries( bag socket )
endif( __QNX__ )

if( __ANDROID__ )
    my_target_link_libraries( bag.externals m )
    my_target_link_libraries( bag m )
endif( __ANDROID__ )
my_target_link_libraries( bag bag.externals )

set( EXTERNALS bag.externals )
endif()
###########

add_library( bag.psi SHARED 
  ${FIRST_GCC_LIBRARY_SOURCE} 
  ${PSI_SOURCES} 
  ${LAST_GCC_LIBRARY_SOURCE} 
      )
      
if( __ANDROID__ )
	set( extra_link_flags "${extra_link_flags} -Wl,-rpath,${CMAKE_BINARY_DIR}" )
endif( __ANDROID__ )
SET_TARGET_PROPERTIES(bag.psi PROPERTIES
                  DEFINE_SYMBOL PSI_SOURCE
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS  "TARGET_LABEL=bag_psi;SOURCE_PSI2;SYSTRAY_LIBRARAY;${PSIExtraDefinitions};${ExtraDefinitions};TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}bag${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  FOLDER "Control Library" 
                  LINK_FLAGS "${extra_link_flags}"
)

if( NOT __NO_CPLUSPLUS )
  if( NOT FORCE_CXX )
    add_library( ${BAG_PSI_PLUSPLUS} SHARED 
       ${FIRST_GCC_LIBRARY_SOURCE} 
       ${psi_cplusplus_sources}
       ${LAST_GCC_LIBRARY_SOURCE} 
      )
    SET_TARGET_PROPERTIES(${BAG_PSI_PLUSPLUS} PROPERTIES
                  DEFINE_SYMBOL PSI_SOURCE
                  COMPILE_FLAGS  "${ExtraFlags}"
                  COMPILE_DEFINITIONS  "TARGET_LABEL=bag_psi_pp;SOURCE_PSI2;SYSTRAY_LIBRARAY;${PSIExtraDefinitions};${ExtraDefinitions};TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX}\""
                  FOLDER "Control Library" 
                  LINK_FLAGS "${extra_link_flags}"
    )
    my_target_link_libraries( ${BAG_PSI_PLUSPLUS} bag ${BAG_PLUSPLUS} ${EXTERNALS} ${PLATFORM_LIBRARIES} )
    set( SACK_PSI_LIBRARY ${BAG_PSI_PLUSPLUS} bag.psi )
    if( __ANDROID__ )
        install_mode_dest( bag bag.psi ${BAG_PLUSPLUS} ${BAG_PSI_PLUSPLUS} ${EXTERNALS} )
    else()
        install_mode_dest( bag bag.psi ${BAG_PLUSPLUS} ${BAG_PSI_PLUSPLUS} ${EXTERNALS} )
    endif( __ANDROID__ )
  else( NOT FORCE_CXX )
    set( SACK_PSI_LIBRARY bag.psi )
    install_mode_dest( bag bag.psi ${EXTERNALS} )
  endif( NOT FORCE_CXX )
else( NOT __NO_CPLUSPLUS )
    if( __ANDROID__ )
        install_mode_dest( bag bag.psi ${EXTERNALS} )
    else()
        install_mode_dest( bag bag.psi ${EXTERNALS} )
    endif( __ANDROID__ )
endif( NOT __NO_CPLUSPLUS )

my_target_link_libraries( bag.psi bag ${EXTERNALS} ${PLATFORM_LIBRARIES} )

if( NOT NEED_FREETYPE )
  my_target_link_libraries( bag.psi  ${Freetype_LIBRARIES} )
  if( NOT __NO_CPLUSPLUS )
    if( NOT FORCE_CXX )
      my_target_link_libraries( ${BAG_PLUSPLUS}  ${Freetype_LIBRARIES}  )
    endif( NOT FORCE_CXX )
  endif( NOT __NO_CPLUSPLUS )
  include_directories( ${Freetype_INCLUDE_DIRS} )
endif()

###########


add_subdirectory( src/imglib )

###########

if( NOT __NO_GUI__ )
  if( NOT __ANDROID__ )
     add_subdirectory( src/vidlib )
  endif( NOT __ANDROID__ )
  
else( NOT __NO_GUI__ )
     add_subdirectory( src/vidlib/proxy )
endif( NOT __NO_GUI__ )

###########


endif( BUILD_MONOLITHIC )

install( FILES makefiles/cmake/DefaultInstall.cmake DESTINATION . )

# this SACK_CORE_BUILD is for building before we get 'installed' some of the header locations get moved.
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

    #if( __ANDROID__ )
        #set( FREEGLUT_GLES2 ON )
        #set( FREEGLUT_BUILD_DEMOS OFF )
        #set( ANDROID ON )
        #add_subdirectory( src/contrib/freeglut/freeglut-SVN )
    	#INCLUDE_DIRECTORIES( src/contrib/freeglut/freeglut-SVN/include )
    #endif( __ANDROID__ )


IF(NOT FORCE_CXX AND EXISTS ${BULLET_SOURCE}/CMakeLists.txt AND NOT BUILD_UNICODE )
  if( WATCOM )
    message( "bullet is not watcom friendly" )
  else( WATCOM )
    if( __ANDROID__ )
      message( "bullet is not required; android build trims down bullet compilation" )
    else( __ANDROID__ )
      message( "Congrats, you get Bullet" )
      SET(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Adds a postfix for debug-built libraries.")
      SET(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "Adds a postfix for MinsizeRelease-built libraries.")
      SET(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "Adds a postfix for ReleaseWithDebug-built libraries.")

      set( USE_GLUT OFF CACHE BOOL "Option Use Glut" )
      #set( LIBRARY_OUTPUT_PATH ${CMAKE_INSTALL_PREFIX}/lib )
      set( INTERNAL_ADD_POSTFIX_EXECUTABLE_NAMES OFF )
      set( BUILD_INTEL_OPENCL_DEMOS OFF CACHE BOOL "Bullet option default override"  )
      set( BUILD_MINICL_OPENCL_DEMOS OFF CACHE BOOL "Bullet option default override"  )
      set( BUILD_CPU_DEMOS OFF CACHE BOOL "Bullet option default override"  )
      set( BUILD_DEMOS OFF CACHE BOOL "Bullet option default override" )
      set( BUILD_UNIT_TESTS OFF CACHE BOOL "Bullet option default override" )
      set( BUILD_MULTITHREADING ON CACHE BOOL "Build multithreaded library" )
      set( USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "Enable Using msvc dynamic runtim" )
      set( INSTALL_LIBS ON CACHE BOOL "Build multithreaded library" )
      set( LIB_INSTALL_DIR "lib" CACHE PATH "set library install directory" )
      add_definitions( -D__BULLET_ENABLED__ )
      set( __BULLET_ENABLED__ ON )

      if( __ANDROID__ OR __QNX__ )
          set( FREEGLUT_BUILD_DEMOS OFF )
          set( ANDROID ON )
              MESSAGE("OPENGL NOT FOUND(andrdoid)")
              SET(OPENGL_gl_LIBRARY GLESv2)
              SET(OPENGL_glu_LIBRARY "" )
      else( __ANDROID__ OR __QNX__ )
          add_subdirectory( src/contrib/freeglut/freeglut-2.8.0 )
          INCLUDE_DIRECTORIES( src/contrib/freeglut/freeglut-2.8.0/include )
          	IF (MSVC)
          		SET(GLUT_FOUND TRUE)
          		IF (CMAKE_CL_64)
          			message("Win64 using Glut/glut64.lib")
          			SET(GLUT_glut_LIBRARY ${BULLET_PHYSICS_SOURCE_DIR}/Glut/glut64.lib)
          		ELSE(CMAKE_CL_64)
          			message("Win32 using Glut/glut32.lib")
          			SET(GLUT_glut_LIBRARY ${BULLET_PHYSICS_SOURCE_DIR}/Glut/glut32.lib)
          		ENDIF (CMAKE_CL_64)
          	 	#INCLUDE_DIRECTORIES(${BULLET_PHYSICS_SOURCE_DIR}/Glut)
          	ENDIF()
      endif( __ANDROID__ OR __QNX__ )
      if( EXISTS ${BULLET_SOURCE} )
	      add_subdirectory( ${BULLET_SOURCE} bullet )
      endif( EXISTS ${BULLET_SOURCE} )
              
      # this should be applied to bullet too; but I like the static lib right now.
      #SET( BUILD_SHARED_LIBS ON )
      SET( INSTALL_EXTRA_LIBS ON )
      if( NOT __ANDROID__ )
              #add_subdirectory( ${BULLET_SOURCE}/demos/OpenGL bullet_opengl )
              #add_subdirectory( bullet_opengl )
      endif( NOT __ANDROID__ )
    endif( __ANDROID__ )
  endif( WATCOM )
else()
  set( __BULLET_ENABLED__ OFF )
endIF(NOT FORCE_CXX AND EXISTS ${BULLET_SOURCE}/CMakeLists.txt AND NOT BUILD_UNICODE )

#alternative video system
if( NOT BUILD_MONOLITHIC AND NOT __NO_GUI__ )
	#include vidlib first incase glew is used, then imglib
	add_subdirectory( src/imglib/puregl2 )
	add_subdirectory( src/imglib/vulkan )

if( NOT __QNX__ AND NOT __ANDROID__ )
	#standard OpenGL; mostly windows-centric
	add_subdirectory( src/imglib/puregl )
endif( NOT __QNX__ AND NOT __ANDROID__ )

  #GET_FILENAME_COMPONENT(WIN8_SDK "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]" ABSOLUTE )
  #if( NOT WIN8_SDK OR ( WIN8_SDK STREQUAL "/registry" ) )
  #  GET_FILENAME_COMPONENT(WIN8_SDK "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v8.0;InstallationFolder]" ABSOLUTE )
  #endif( NOT WIN8_SDK OR ( WIN8_SDK STREQUAL "/registry" ) )

if( NOT __NO_NETWORK__ )
  add_subdirectory( src/vidlib/proxy )
endif( NOT __NO_NETWORK__ )
  add_subdirectory( src/vidlib/puregl2 )
  add_subdirectory( src/vidlib/vulkan )

if( NOT __QNX__ AND NOT __ANDROID__ )
  add_subdirectory( src/vidlib/puregl )
endif( NOT __QNX__ AND NOT __ANDROID__ )

  if( __ANDROID__ )
  	add_subdirectory( src/vidlib/android )
  endif( __ANDROID__ )
  if( DIRECTX_SDK )
    if( WIN32 AND NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT WATCOM AND NOT __NO_GUI__ )
      add_subdirectory( src/vidlib/pured3d )
      add_subdirectory( src/imglib/pured3d )
      add_subdirectory( src/imglib/pured3d2 )
      add_subdirectory( src/imglib/pured3d10 )

      if( WIN8_SDK )
        add_subdirectory( src/imglib/pured3d11 )
      endif( WIN8_SDK )

    endif( WIN32 AND NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT WATCOM AND NOT __NO_GUI__ )
  endif( DIRECTX_SDK )

set(DIRECTX_SDK "" CACHE STRING "directx sdk path to build d3d parts") 


if( CMAKE_COMPILER_IS_GNUCC )
    if( __ANDROID__ )
       set( INTERFACE_CONFIGURATION data/interface.conf.puregl.android )
    elseif( __QNX__ )
       set( INTERFACE_CONFIGURATION data/interface.conf.puregl.qnx )
    elseif( WIN32 )
       if( BUILD_UNICODE)
          set( INTERFACE_CONFIGURATION data/interface.conf.puregl.gcc.unicode )
       else( BUILD_UNICODE)
          set( INTERFACE_CONFIGURATION data/interface.conf.puregl.gcc )
       endif( BUILD_UNICODE)
    else( )
        set( INTERFACE_CONFIGURATION data/interface.conf.puregl.lnx )
    endif( __ANDROID__ )

else( CMAKE_COMPILER_IS_GNUCC )
  if( BUILD_UNICODE )
     set( INTERFACE_CONFIGURATION data/interface.conf.puregl.vc.unicode )
  else( BUILD_UNICODE )
     set( INTERFACE_CONFIGURATION data/interface.conf.puregl.vc )
  endif( BUILD_UNICODE )

endif( CMAKE_COMPILER_IS_GNUCC )

if( BUILD_UNICODE )
  macro( DO_CONFIGURE_FILE input output )
    if( $ENV{COMSPEC} MATCHES cmd )
      STRING( REPLACE "/" "\\" s1 ${CMAKE_CURRENT_SOURCE_DIR}/${input} )
      STRING( REPLACE "/" "\\" s2 ${CMAKE_BINARY_DIR}/${output} )
      STRING( REPLACE "/" "\\" finalout ${CMAKE_INSTALL_PREFIX}/${INTERFACE_OUTPUT_DIR}/${output} )
      STRING( REPLACE "/" "\\" leader ${CMAKE_CURRENT_SOURCE_DIR}\\BlankUnicode.txt )
      file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${INTERFACE_OUTPUT_DIR})
      EXECUTE_PROCESS(COMMAND cmd /c type ${s1} OUTPUT_FILE ${s2}.tmp )
      configure_file( ${s2}.tmp ${s2}.configured )
      EXECUTE_PROCESS(COMMAND cmd /U /C type ${s2}.configured OUTPUT_FILE ${s2}.configured.unicode )
      EXECUTE_PROCESS(COMMAND cmd /c copy /b ${leader}+${s2}.configured.unicode ${finalout} OUTPUT_FILE NULL )
    else()
      message( "NO METHOD TO CONFIGURE INTERFACE FILES" )
      abort()
    endif( $ENV{COMSPEC} MATCHES cmd )
  endmacro( DO_CONFIGURE_FILE )
else( BUILD_UNICODE )
  macro( DO_CONFIGURE_FILE input output )
    configure_file( ${input} ${CMAKE_INSTALL_PREFIX}/${INTERFACE_OUTPUT_DIR}/${output} )
  endmacro( DO_CONFIGURE_FILE )
endif( BUILD_UNICODE )

if( UNIX AND NOT __ANDROID__ )
  set( INTERFACE_TARGET opengl1.5 )
else( UNIX AND NOT __ANDROID__ )
  set( INTERFACE_TARGET windows )
endif( UNIX AND NOT __ANDROID__ )

DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} interface.conf )
if( NOT __ANDROID__ )
  set( INTERFACE_TARGET opengl )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
  set( INTERFACE_TARGET opengl1.5 )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
  set( INTERFACE_TARGET opengl2 )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
  if( NOT UNIX )
  set( INTERFACE_TARGET d3d )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
  set( INTERFACE_TARGET d3d2 )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
  set( INTERFACE_TARGET d3d3 )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
  endif( NOT UNIX )
  set( INTERFACE_TARGET headless )
  DO_CONFIGURE_FILE( ${INTERFACE_CONFIGURATION} ${INTERFACE_TARGET}.interface.conf )
endif( NOT __ANDROID__ )



endif( NOT BUILD_MONOLITHIC AND NOT __NO_GUI__ )

# parts of SDK ...
set( INSTALL_FRAME_BORDER ${INTERFACE_OUTPUT_DIR}/images CACHE STRING "Option to install default images actually" )
add_subdirectory( data )
add_subdirectory( include )
add_subdirectory( src/utils/runner )
add_subdirectory( src/utils/virtual_file_system )

if( NOT __CLR__ )
	add_subdirectory( src/contrib/MatrixSSL/3.7.1 )
endif( NOT __CLR__ )

if( BUILD_TESTS )
   add_subdirectory( src/streamlib/ffmpeg/audio_test )

   #add_subdirectory( src/bench/mem )

 if( NOT __NO_GUI__ AND NOT __NO_MSGSVR__ )
  add_subdirectory( src/msgsvr/testdisplay )
   if( NOT __ANDROID__ )
      add_subdirectory( src/msgsvr/client/client_server )
      add_subdirectory( src/msgsvr/client/client_client )
      add_subdirectory( src/filesyslib/filemon/filemonitor )
   endif( NOT __ANDROID__ )
 endif( NOT __NO_GUI__ AND NOT __NO_MSGSVR__ )

   add_subdirectory( src/configlib/test1 )
   # these opengl plugins used GLEW, and did their program at very low level
   # ment to be tests based on binary target output
   #add_subdirectory( src/games/simple_vector_tester )
   #add_subdirectory( src/games/simple_array_tester )
   if( NOT BUILD_MONOLITHIC AND DIRECTX_SDK )   
      # monolithic has interfaces forced off; so we can't get the shader compiler interface
      add_subdirectory( src/games/simple_vector_tester/d3d )
   endif( NOT BUILD_MONOLITHIC AND DIRECTX_SDK )   

if( NOT __NO_NETWORK__ )
   add_subdirectory( src/netlib/html5.websocket/server )
   #add_subdirectory( src/netlib/html5.websocket/client )
   add_subdirectory( src/netlib/html5.websocket/json/tests )
   add_subdirectory( src/netlib/echo )
endif( NOT __NO_NETWORK__ )

   add_subdirectory( src/psilib/console/cmdshell )
   add_subdirectory( src/tests )
   add_subdirectory( src/psilib/console )
   add_subdirectory( src/psilib/editctltest )
   add_subdirectory( src/psilib/edittst )
   add_subdirectory( src/psilib/listtst )
   add_subdirectory( src/psilib/fonttst )
   add_subdirectory( src/psilib/scrolltest )
   add_subdirectory( src/psilib/menutst )
   add_subdirectory( src/psilib/sheettst )
   add_subdirectory( src/psilib/paltst )
   add_subdirectory( src/psilib/calctl )
   IF( NOT BUILD_MONOLITHIC )
      add_subdirectory( src/typelib )
   ENDIF( NOT BUILD_MONOLITHIC )
   add_subdirectory( src/imglib/testsprite )


   if( NOT WATCOM AND NOT UNIX AND NOT MINGW )
      add_subdirectory( src/utils/multi_touch/diagnostic )
      add_subdirectory( src/utils/multi_touch/plotter )
   endif( NOT WATCOM AND NOT UNIX AND NOT MINGW  )
   add_subdirectory( src/utils/test_filescan )
endif( BUILD_TESTS )


if( NOT __ANDROID__ AND NOT __NO_MSGSVR__ )
   add_subdirectory( src/msgsvr/server )
   add_subdirectory( src/msgsvr/servicelist )
endif( NOT __ANDROID__ AND NOT __NO_MSGSVR__ )
if( WIN32 )
   add_subdirectory( src/utils/application_delay )
   if( NOT __CLR__ )
     add_subdirectory( src/utils/debugger_lockout )
   endif( NOT __CLR__ )
endif( WIN32 )
   add_subdirectory( src/salty_random_generator )
if( NOT UNIX )
  #   This doesn't build under watcom either...
  #   it needs to be modified to understand functional syntax so it can do formatting...
  #add_subdirectory( makefiles/prog/ppc )
endif( NOT UNIX )
add_subdirectory( src/utils/seed_random )
add_subdirectory( src/deadstart )
add_subdirectory( src/utils/deploy )
#add_subdirectory( ../allegro allegro )

 if( NOT __NO_GUI__ AND NOT __ANDROID__ )
  add_subdirectory( src/imglib/dump_font_cache )
 endif( NOT __NO_GUI__ AND NOT __ANDROID__ )

 if( USE_OPTIONS AND NOT ( __NO_ODBC__ AND NOT USE_SQLITE ) )
  if( NOT __NO_GUI__ )
   add_subdirectory( src/SQLlib/optlib/editoption )
   add_subdirectory( src/translationlib/edit_translations )
  endif( NOT __NO_GUI__ )
  add_subdirectory( src/utils/loginfo )
  if( NOT __ANDROID__ AND BUILD_EXTRAS )
     add_subdirectory( src/utils/unicode_ascii )
     add_subdirectory( src/SQLlib/optlib/setoptions )
  
     add_subdirectory( src/SQLlib/optlib/importini )
     add_subdirectory( src/SQLlib/optlib/exportini )
     if( WIN32 )
         add_subdirectory( src/printer_lib/printer_test )
     endif( WIN32 ) 
  endif( NOT __ANDROID__ AND BUILD_EXTRAS )
  
 endif( USE_OPTIONS AND NOT ( __NO_ODBC__ AND NOT USE_SQLITE ) )

if( __LINUX__ OR ( MSVC10 AND __64__ ) )
add_subdirectory( src/utils/apache_shell )
endif( __LINUX__ OR ( MSVC10 AND __64__ ) )

if( __INCLUDE_INTERSHELL__ )
add_subdirectory( src/InterShell )
endif( __INCLUDE_INTERSHELL__ )

install( FILES ${SOURCES_ROOT}src/contrib/snprintf_2.2/snprintf.h DESTINATION
include/SACK/snprintf-2.2 )


# get the libraries..
SET( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP 1 )
if( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES 1 )
SET( CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY 1 )
endif( ${CMAKE_BUILD_TYPE} MATCHES "[dD][eE][bB][uU][gG]" )

INCLUDE(InstallRequiredSystemLibraries)
INSTALL( FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${BINARY_OUTPUT_DIR} )

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SACK Layer 1")
SET(CPACK_PACKAGE_VENDOR "Freedom Collective")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/howto.compile")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/read.this")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")

if( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_INSTALL_CONFIG_TYPE} )
else( SUPPORTS_PARALLEL_BUILD_TYPE )
SET( VERSION_TYPE ${CMAKE_BUILD_TYPE} )
endif( SUPPORTS_PARALLEL_BUILD_TYPE )

SET(CPACK_PACKAGE_VERSION_PATCH "485-${CMAKE_GENERATOR}-${VERSION_TYPE}")

#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/doc\\\\BlankButton.jpg")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "${BINARY_OUTPUT_DIR}\\\\sack_bag.dll")
  SET(CPACK_NSIS_DISPLAY_NAME "SACK ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\code.google.com\\\\p\\\\c-system-abstraction-component-gui")
  SET(CPACK_NSIS_CONTACT "d3ck0r@gmail.com")
  SET(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '\$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Set Build Mode.lnk' '\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\${VERSION_TYPE}\\\\sack_deploy.exe'" )
  SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ExecWait '\\\"\$INSTDIR\\\\${BINARY_OUTPUT_DIR}\\\\${VERSION_TYPE}\\\\sack_deploy.exe\\\"'")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "${BINARY_OUTPUT_DIR}/MyExecutable")
  SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_EXECUTABLES "" "My Executable")
INCLUDE(CPack)

