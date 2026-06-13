

 cmake_policy(SET CMP0054 NEW)

#set( project ffmpeg_interface )

ADD_DEFINITIONS( -DFFMPEG_INTERFACE_SOURCE )

# OpenAL is loaded at runtime via dlopen.  The headers are vendored
# (openal-soft-1.15.1/include below), so nothing is needed to compile, but on
# macOS Homebrew's openal-soft is keg-only and not on the default dlopen search
# path - resolve its full dylib path so the runtime can find it.
if( APPLE )
  execute_process( COMMAND brew --prefix openal-soft
                   OUTPUT_VARIABLE OPENAL_SOFT_PREFIX
                   OUTPUT_STRIP_TRAILING_WHITESPACE
                   ERROR_QUIET )
  if( OPENAL_SOFT_PREFIX AND EXISTS "${OPENAL_SOFT_PREFIX}/lib/libopenal.dylib" )
    ADD_DEFINITIONS( -DLIB_OPENAL_PATH="${OPENAL_SOFT_PREFIX}/lib/libopenal.dylib" )
  endif()

  # ffmpeg is also dlopen'd at runtime and lives off the default search path on
  # Homebrew.  The vendored headers are ffmpeg 7.x, so prefer the matching
  # keg-only ffmpeg@7; fall back to mainline ffmpeg (may be a newer major - ABI
  # mismatch risk) if @7 is not installed.
  execute_process( COMMAND brew --prefix ffmpeg@7
                   OUTPUT_VARIABLE FFMPEG_PREFIX
                   OUTPUT_STRIP_TRAILING_WHITESPACE
                   ERROR_QUIET )
  if( NOT FFMPEG_PREFIX OR NOT EXISTS "${FFMPEG_PREFIX}/lib/libavformat.dylib" )
    execute_process( COMMAND brew --prefix ffmpeg
                     OUTPUT_VARIABLE FFMPEG_PREFIX
                     OUTPUT_STRIP_TRAILING_WHITESPACE
                     ERROR_QUIET )
  endif()
  if( FFMPEG_PREFIX AND EXISTS "${FFMPEG_PREFIX}/lib/libavformat.dylib" )
    message( STATUS "ffmpeg (runtime dlopen) resolved to: ${FFMPEG_PREFIX}/lib" )
    ADD_DEFINITIONS(
      -DLIB_AVFORMAT_PATH="${FFMPEG_PREFIX}/lib/libavformat.dylib"
      -DLIB_AVCODEC_PATH="${FFMPEG_PREFIX}/lib/libavcodec.dylib"
      -DLIB_AVDEVICE_PATH="${FFMPEG_PREFIX}/lib/libavdevice.dylib"
      -DLIB_AVUTIL_PATH="${FFMPEG_PREFIX}/lib/libavutil.dylib"
      -DLIB_SWSCALE_PATH="${FFMPEG_PREFIX}/lib/libswscale.dylib"
      -DLIB_SWRESAMPLE_PATH="${FFMPEG_PREFIX}/lib/libswresample.dylib"
    )
  endif()
endif( APPLE )

set( FFMPEG_SOURCES_ROOT ${SOURCES_ROOT}src/streamlib/ffmpeg/ )
set( FFMPEG_SOURCES ${FFMPEG_SOURCES_ROOT}ffmpeg_interface.c )
set( FFMPEG_CPP_SOURCES ${CMAKE_BINARY_DIR}/src/streamlib/ffmpeg/ffmpeg_interface.cpp )
set( INCLUDEDIRS "-I${FFMPEG_SOURCES_ROOT}../../contrib/gsm-1.0-pl13/inc -I${FFMPEG_SOURCES_ROOT}prebuilt/ffmpeg/7.1/include -I${FFMPEG_SOURCES_ROOT}openal-soft-1.15.1/include " )

#message( "OH THIS IS NEW: ${CMAKE_C_COMPILER_ID} ")

if( ( CMAKE_C_COMPILER_ID STREQUAL "GNU" ) OR ( CMAKE_C_COMPILER_ID STREQUAL "Clang" ) )
  set_source_files_properties( ${FFMPEG_CPP_SOURCES} PROPERTIES COMPILE_FLAGS ${INCLUDEDIRS} )
  set_source_files_properties( ${FFMPEG_SOURCES} PROPERTIES COMPILE_FLAGS ${INCLUDEDIRS} )
endif()
if( CMAKE_C_COMPILER_ID STREQUAL "MSVC" )
  set_source_files_properties( ${FFMPEG_CPP_SOURCES} PROPERTIES COMPILE_FLAGS ${INCLUDEDIRS} )
  set_source_files_properties( ${FFMPEG_SOURCES} PROPERTIES COMPILE_FLAGS ${INCLUDEDIRS} )
endif()
if( CMAKE_C_COMPILER_ID STREQUAL "OpenWatcom" )
  STRING( REPLACE "/" "\\" STRING1 ${INCLUDEDIRS} )
  STRING( REPLACE "-I" "/I" STRING1 ${STRING1} )
  set_source_files_properties( ${FFMPEG_CPP_SOURCES} PROPERTIES COMPILE_FLAGS "/I${STRING1}" )
  set_source_files_properties( ${FFMPEG_SOURCES} PROPERTIES COMPILE_FLAGS "/I${STRING1}" )
endif()


if( MAKING_SACK_CORE )
  if( WIN32 )
    install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/windows/x64/ffmpeg/7.1/bin/ DESTINATION bin )
    #install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/windows/x64/openal/1.15.1/bin/ DESTINATION bin )
    set( ffmpeg_libraries avcodec-61.dll avdevice-61.dll avfilter-10.dll avformat-61.dll avutil-59.dll  swresample-5.dll swscale-8.dll )
    #set( openal_libraries OpenAL32.dll )
    # portable projects won't need this anyway.
    if( MAKING_SACK_CORE )
      set( UTILS OFF)
      set( EXAMPLES OFF)
      add_subdirectory( ${FFMPEG_SOURCES_ROOT}openal-soft-1.15.1 openal )
    endif( MAKING_SACK_CORE )
  endif( WIN32 )
endif( MAKING_SACK_CORE )

if( __ANDROID__ )
  install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/android/arm7/ffmpeg/2.1.3/ DESTINATION . )
  set( ffmpeg_libraries libavcodec.so libavdevice.so libavfilter.so libavformat.so libavutil.so libswresample.so libswscale.so )
#  set( openal_libraries libopenal.so )
#  set( openal_libraries libOpenSLES.so )

  # portable projects won't need this anyway.
  if( MAKING_SACK_CORE )
    add_subdirectory( ${FFMPEG_SOURCES_ROOT}openal-soft-1.15.1 openal )
  endif( MAKING_SACK_CORE )

endif( __ANDROID__ )

if( MAKING_SACK_CORE )
INSTALL(
  FILES
    ${SOURCES_ROOT}include/ffmpeg_interface.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}
)
endif( MAKING_SACK_CORE )
