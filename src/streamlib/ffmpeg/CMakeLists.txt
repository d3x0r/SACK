


#set( project ffmpeg_interface )

ADD_DEFINITIONS( -DFFMPEG_INTERFACE_SOURCE )
set( FFMPEG_SOURCES_ROOT ${SOURCES_ROOT}src/streamlib/ffmpeg/ )
set( FFMPEG_SOURCES ${FFMPEG_SOURCES_ROOT}ffmpeg_interface.c )
INCLUDE_DIRECTORIES( ${FFMPEG_SOURCES_ROOT}prebuilt/ffmpeg/2.1.3/include  )
INCLUDE_DIRECTORIES( ${FFMPEG_SOURCES_ROOT}openal-soft-1.15.1/include  )



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

INSTALL( 
  FILES
    ${SOURCES_ROOT}include/ffmpeg_interface.h
  DESTINATION
    include/${CMAKE_PROJECT_NAME}
)



