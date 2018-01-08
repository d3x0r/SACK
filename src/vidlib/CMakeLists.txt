
# because sources are common to monolithic build mode
# they are already defined in a variable calld VIDLIB_SOURCES (defined in CMakeSources.lst)

if( FORCE_CXX )
    set_source_files_properties( ${VIDLIB_SOURCES}  PROPERTIES LANGUAGE CXX )
endif( FORCE_CXX )

if( __NO_ODBC__ OR NOT USE_OPTIONS )
   ADD_DEFINITIONS( -D__NO_OPTIONS__ )
endif( __NO_ODBC__ OR NOT USE_OPTIONS )

if( __MAC__ )
   INCLUDE_DIRECTORIES( /System/Library/Frameworks/OpenGL.framework/Headers)
endif( __MAC__ )

add_library( bag.video SHARED
  ${FIRST_GCC_LIBRARY_SOURCE}
  ${VIDLIB_SOURCES}
  ${LAST_GCC_LIBRARY_SOURCE}
)
SET_TARGET_PROPERTIES(bag.video PROPERTIES
                  DEFINE_SYMBOL VIDEO_LIBRARY_SOURCE
                  COMPILE_FLAGS  "-DTARGET_LABEL=bag_video -DRENDER_LIBRARY_SOURCE"
                  COMPILE_DEFINITIONS "TARGET_LABEL=bag_video;TARGETNAME=\"${CMAKE_SHARED_LIBRARY_PREFIX}bag.video${CMAKE_SHARED_LIBRARY_SUFFIX}\";${ExtraRenderFlags}"
                  FOLDER "Video Renderer"
                  LINK_FLAGS "${extra_link_flags}"
)
my_target_link_libraries( bag.video bag ${EXTERNALS} ${OPENGL_LIBRARY}  ${PLATFORM_LIBRARIES})

# this is linked to bag.image because of obsolete sprite methods
my_target_link_libraries( bag.video bag.image )


set( SACK_VIDEO_LIBRARY bag.video )
install_mode_dest( bag.video )
