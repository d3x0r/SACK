

 cmake_policy(SET CMP0054 NEW)

#set( project ffmpeg_interface )

ADD_DEFINITIONS( -DFFMPEG_INTERFACE_SOURCE )
set( FFMPEG_SOURCES_ROOT ${SOURCES_ROOT}src/streamlib/ffmpeg/ )
set( FFMPEG_SOURCES ${FFMPEG_SOURCES_ROOT}ffmpeg_interface.c )
set( FFMPEG_CPP_SOURCES ${CMAKE_BINARY_DIR}/src/streamlib/ffmpeg/ffmpeg_interface.cpp )
set( INCLUDEDIRS "-I${FFMPEG_SOURCES_ROOT}../../contrib/gsm-1.0-pl13/inc -I${FFMPEG_SOURCES_ROOT}prebuilt/ffmpeg/2.1.3/include -I${FFMPEG_SOURCES_ROOT}openal-soft-1.15.1/include " )

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
    if( __64__ )
      install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/windows/x64/ffmpeg/2.3.0/bin/ DESTINATION bin )
      #install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/windows/x64/openal/1.15.1/bin/ DESTINATION bin )
      set( ffmpeg_libraries avcodec-56.dll avdevice-56.dll avfilter-5.dll avformat-56.dll avutil-54.dll postproc-53.dll swresample-1.dll swscale-3.dll )
      #set( openal_libraries OpenAL32.dll )
    else( __64__ )
      install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/windows/x86/ffmpeg/2.3.0/bin/ DESTINATION bin )
      #install( DIRECTORY ${FFMPEG_SOURCES_ROOT}prebuilt/windows/x86/openal/1.15.1/bin/ DESTINATION bin )
      set( ffmpeg_libraries avcodec-56.dll avdevice-56.dll avfilter-5.dll avformat-56.dll avutil-54.dll postproc-53.dll swresample-1.dll swscale-3.dll )
      #set( openal_libraries OpenAL32.dll )
    endif( __64__ )
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
